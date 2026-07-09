#include "config.h"
#include <RadioLib.h>

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

LR1121 radio = new Module(LR_NSS, LR_IRQ, LR_RESET, LR_BUSY);

// Globale Variablen für das blockierungsfreie Zeitmanagement
unsigned long naechsterSendetermin = 0;
unsigned long letzterKnopfDruck = 0;
int lastButtonState = HIGH;
bool sendingOperation = false;
LoRaConfig activeConfig;

// --- Ringpuffer & Filter Variablen ---
const int BUFFER_SIZE = 50;        // 50 Samples * 10ms = 500ms Filterfenster (niquist)
float buffer_X[BUFFER_SIZE];
float buffer_Y[BUFFER_SIZE];
float buffer_Z[BUFFER_SIZE];

int bufferIndex = 0;               // Zeigt auf die aktuelle Schreibposition
unsigned long letzteSensorAbfrage = 0; // Speicher für den 10-ms-Takt (Niquist, physikalische Schwingung)

// --- Globaler Schlüssel für die XOR-Verschlüsselung ---
const uint8_t xorKey[] = { 0x5A, 0xA5, 0x1F, 0xE1, 0x3C, 0xC3, 0x7F, 0xF7 };
const size_t keySize = sizeof(xorKey);

void wait_busy() {
  while(digitalRead(LR_BUSY) == HIGH) {
    yield(); // Erlaubt dem System Hintergrundaufgaben (wichtig für Stabilität)
    Serial.println("Der LR1121 signalisiert Beschäftigt durch ein HIGH Signal");
  }
}

void setup() {
  Serial.begin(115200);
  while(!Serial); // Warten bis LR1121 startet
  Serial.println("Serial gestartet!");
  SPI.begin();

  delay(200); // <-- GENAU HIER: Den Sensoren Zeit zum Aufwachen geben!

  if (!bmp.begin(0x76)) {
    Serial.println(F("[ERROR] BMP280 nicht gefunden!"));
    while (1);
  }
  Serial.println(F("[OK] BMP280 erfolgreich initialisiert.")); // wofür steht das F()?

  if (!bno.begin()) {
    Serial.println(F("[ERROR] BNO055 nicht gefunden!"));
    while (1);
  }
  Serial.println(F("[OK] BNO055 erfolgreich initialisiert."));

  delay(1000);

  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);
  pinMode(LR_BUSY, INPUT);
  pinMode(LR_RESET, OUTPUT);
  pinMode(LR_NSS, OUTPUT);
  pinMode(USER_BTN, INPUT_PULLUP);

  // NSS muss vor dem Reset zwingend HIGH sein!
  digitalWrite(LR_NSS, HIGH); 
  // LR1121 Hardware-Reset für Semtech-Shields
  Serial.println("Semtech LR1121 Hardware Reset...");
  digitalWrite(LR_RESET, LOW);
  delay(50);                 
  digitalWrite(LR_RESET, HIGH);
  // Dem Chip exakt 300-350ms Zeit geben, um die Spannungsregler intern hochzufahren
  delay(350); 
  Serial.println("LR1121 Hardware Reset abgeschlossen.");

  // start RadioLib mit TCXO Konfiguration für PCB_E655V01A
  Serial.println("Starte radio.begin() mit Semtech-Parametern...");
  radio.XTAL = true;  // Da dein Shield keinen TCXO nutzt, stellen wir XTAL auf true
  int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS: radio.begin() erfolgreich durchgeführt!");
  
    Serial.println("Push User Button to choose 433MHz");
    delay(1000);
    if (digitalRead(USER_BTN) == LOW) {
      activeConfig = config433;
      Serial.println("Chosen LoRa parameter: 433MHz");
    }
    else {
      activeConfig = config868;
      Serial.println("Chosen LoRa parameter: 868MHz");
    }
    delay(1000);

    radio.setFrequency(activeConfig.frequency);
    radio.setOutputPower(activeConfig.txPower);
    radio.setBandwidth(activeConfig.bandwidth);
    radio.setSpreadingFactor(activeConfig.spreadingFactor);
    radio.setCodingRate(activeConfig.codingRate);
  } 
  else {
    Serial.print("RadioLib Fehler! Code: ");
    Serial.println(state);
    while(1);
  }
}

void loop() {
  // =========================================================================
  // 1. TASTER-ABFRAGE (Völlig blockierungsfrei, reagiert IMMER sofort)
  // =========================================================================
  int currentButtonState = digitalRead(USER_BTN);
  // Flankenerkennung: Knopf wird frisch gedrückt (Wechsel von HIGH auf LOW)
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    // Software-Entprellung: Nur reagieren, wenn der letzte Druck > 200ms her ist
    if (millis() - letzterKnopfDruck > 200) {
      sendingOperation = !sendingOperation; // Zustand toggeln
      letzterKnopfDruck = millis();         // Zeitstempel merken

      if(sendingOperation) {
        Serial.println("Sender activated");
      } else {
        Serial.println("Sender deactivated");
      }
    }
  }
  lastButtonState = currentButtonState; // Zustand für den nächsten Durchlauf merken

    // =========================================================================
    // 2. EDGE-Computing (Nur wenn aktiv UND die gesetzliche ToA-Pause abgelaufen ist)
    // =========================================================================
    if (millis() - letzteSensorAbfrage >= 10) {
      letzteSensorAbfrage = millis();

      // Beschleunigung von BNO055 auslesen
      sensors_event_t accelerometerData;
      bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);

      // Werte in die aktuellen Schubladen des Arrays legen
      buffer_X[bufferIndex] = accelerometerData.acceleration.x;
      buffer_Y[bufferIndex] = accelerometerData.acceleration.y;
      buffer_Z[bufferIndex] = accelerometerData.acceleration.z;


      // JETZT den Zeiger um eins erhöhen, und bei 50 automatisch auf 0 zurückwerfen
      bufferIndex = (bufferIndex + 1) % BUFFER_SIZE; // Modolo
    }

  // =========================================================================
  // 3. Senden (Nur wenn aktiv UND die gesetzliche ToA-Pause abgelaufen ist)
  // =========================================================================
  if (sendingOperation == true) {
    if (millis() >= naechsterSendetermin){
      // Sensoren auslesen
      sensors_event_t temp_event, pressure_event;
      bmp_temp->getEvent(&temp_event);
      bmp_pressure->getEvent(&pressure_event);

      // =========================================================================
      // 4. MATHEMATIK: MITTELWERT BERECHNEN (Genau im Moment des Sendens)
      // =========================================================================
      float summeX = 0.0;
      float summeY = 0.0;
      float summeZ = 0.0;

      // Alle 50 Werte aus dem Array addieren
      for (int i = 0; i < BUFFER_SIZE; i++) {
        summeX = summeX + buffer_X[i];
        summeY = summeY + buffer_Y[i];
        summeZ = summeZ + buffer_Z[i];

      }

      // Durch 50 teilen = Der fertige, geglättete Gleichanteil (Neigung!)
      float mean_X = summeX / BUFFER_SIZE;
      float mean_Y = summeY / BUFFER_SIZE;
      float mean_Z = summeZ / BUFFER_SIZE;

      package sendeDaten = {
        temp_event.temperature, 
        mean_X,  // Gefilterter Wert X
        mean_Y,  // Gefilterter Wert Y
        mean_Z   // Gefilterter Wert Z
      };

      Serial.println("--- Sende Datenpaket ---");
      Serial.print("Temp: "); Serial.println(sendeDaten.temp);
      Serial.print("Mean X: "); Serial.println(sendeDaten.acce_x, 4);
      Serial.print("Mean Y: "); Serial.println(sendeDaten.acce_y, 4);      
      Serial.print("Mean Z: "); Serial.println(sendeDaten.acce_z, 4);

      // =========================================================================
      // 4b. INLINE XOR-VERSCHLÜSSELUNG
      // =========================================================================
      // Wir casten das Struct im Speicher zu einem Byte-Array
      uint8_t* bytePointer = (uint8_t*)&sendeDaten;
      size_t datenGroesse = sizeof(sendeDaten);

      // Jedes einzelne Byte wird direkt im Arbeitsspeicher mit dem Key XOR-verknüpft
      for (size_t i = 0; i < datenGroesse; i++) {
        bytePointer[i] ^= xorKey[i % keySize];
      }

      digitalWrite(LED_TX, HIGH);
      int state = radio.transmit((uint8_t*)&sendeDaten, datenGroesse);
      digitalWrite(LED_TX, LOW);
      
      // =========================================================================
      // 5. CHECK UND NAECHSTE SENDEZEIT BERECHNEN MIT TOA
      // =========================================================================
      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS: Paket erfolgreich gesendet!");
        
        // --- Dynamische Berechnung der gesetzlichen Pause (Duty Cycle) ---
        // getTimeOnAir liefert Mikrosekunden (us), wir teilen durch 1000 für Millisekunden (ms)
        unsigned long toaMs = radio.getTimeOnAir(datenGroesse) / 1000;
        
        // Bei 1% Duty Cycle (868 MHz) muss die Pause 99-mal so lang sein wie die Sendezeit
        unsigned long pauseMs = toaMs * 99; 
        
        // Den zukünftigen Zeitpunkt berechnen, ab wann wir wieder senden dürfen
        naechsterSendetermin = millis() + pauseMs;
        
        Serial.print("Time-on-Air (ToA): "); Serial.print(toaMs); Serial.println(" ms");
        Serial.print("Gesetzliche Sperrzeit aktiv fuer: "); Serial.print(pauseMs / 1000.0); Serial.println(" Sekunden.");
      } 
      else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
        Serial.println("Fehler: Sende-Timeout!");
        naechsterSendetermin = millis() + 1000; // Bei Fehler nach 1 Sekunde neu versuchen
      } 
      else {
        Serial.print("Fehler beim Senden! Code: "); Serial.println(state);
        naechsterSendetermin = millis() + 1000; 
      }
    }
  }
}