#include "config.h"
#include <RadioLib.h>

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
//Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

int lastState = HIGH; //Pull-up auf High heißt Button nicht gedrückt, LOW heißt Button gedrückt

LR1121 radio = new Module(LR_NSS, LR_IRQ, LR_RESET, LR_BUSY);

void wait_busy() {
  // Der LR1121 signalisiert "Beschäftigt" durch ein HIGH Signal
  while(digitalRead(LR_BUSY) == HIGH) {
    yield(); // Erlaubt dem System Hintergrundaufgaben (wichtig für Stabilität)
  }
}

void setup() {
  // start UART
  Serial.begin(115200);
  while(!Serial);

  Serial.println("=========================");
  Serial.println("Serial gestartet!");

  // SPI-Bus starten
  SPI.begin();

  // check I2C
  if(bmp.begin(0x76)){
    Serial.println("I2C gestartet! Ich bin der Sender :)))");
  } else {
    Serial.print("Ooops, kein BMP280!");
    while(1); 
  }

  // Pins deklarieren
  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);
  pinMode(LR_BUSY, INPUT);
  pinMode(LR_RESET, OUTPUT);
  pinMode(LR_NSS, OUTPUT);
  pinMode(USER_BTN, INPUT_PULLUP); // Senden

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
  
  // Da dein Shield einen TCXO nutzt, stellen wir XTAL auf false
  radio.XTAL = true; 

  // Die Funktion benötigt beim LR1121 für den TCXO-Start explizite Parameter.
  // Standard-Parameter: Frequenz (868.0 MHz), Bandbreite (125.0 kHz), SpreadingFactor (9), 
  // CodingRate (7), SyncWord (0x12), Sendeleistung (10 dBm), Preamble (8), TCXO-Spannung (1.8 V!)
 int state = radio.begin();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS: radio.begin() erfolgreich durchgeführt!");
  } else {
    Serial.print("RadioLib Fehler! Code: ");
    Serial.println(state);
    while(1);
  }
}

void loop() {

  int buttonState = digitalRead(USER_BTN);

  if (buttonState == LOW && buttonState != lastState) {
    
    Serial.println("Button Pressed!");

    // 1. Temperatur vom BMP280 auslesen
    sensors_event_t temp_event;
    bmp_temp->getEvent(&temp_event);
    float temperatur = temp_event.temperature;

    // 2. Den float-Wert in einen String umwandeln
    // (Der zweite Parameter "2" sorgt für zwei Nachkommastellen, z.B. "23.45")
    String sendeText = String(temperatur, 2);

    Serial.print("Sende Temperaturwert: ");
    Serial.print(sendeText);
    Serial.println(" °C ...");

    // 3. LED für die Sende-Dauer einschalten (Visuelles Feedback)
    digitalWrite(LED_TX, HIGH);

    // 4. Daten über den LR1121 abschicken
    int state = radio.transmit(sendeText);
    //int state = radio.transmit(sendeText.c_str());
    //int state = radio.startTransmit("Hello World!");
    // 5. LED wieder ausschalten
    digitalWrite(LED_TX, LOW);

    // 6. Überprüfen, ob das Senden erfolgreich war
    if (state == RADIOLIB_ERR_NONE) {
      Serial.println("SUCCESS: Paket erfolgreich gesendet!");
    } 
    else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
      Serial.println("Fehler: Sende-Timeout!");
    } 
    else {
      Serial.print("Fehler beim Senden! Code: ");
      Serial.println(state);
    }

  }
  else if (buttonState == HIGH && buttonState != lastState){
    
    Serial.println("Button Released!");
    digitalWrite(LED_BUILTIN, LOW);
  }

  lastState = buttonState;
  
  delay(10);
}