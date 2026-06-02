#include "config.h"
#include <RadioLib.h>

Adafruit_BMP280 bmp; // use I2C interface
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();

Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28, &Wire);

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

  // check I2C, BMP starten
  if (!bmp.begin(0x76)) { // Falls 0x76 nicht klappt, probiere 0x77
    Serial.println(F("[ERROR] BMP280 nicht gefunden!"));
    while (1);
  }
  Serial.println(F("[OK] BMP280 erfolgreich initialisiert."));

  // check I2C, BNO starten
  if (!bno.begin()) {
    Serial.println(F("[ERROR] BNO055 nicht gefunden!"));
    while (1);
  }
  Serial.println(F("[OK] BNO055 erfolgreich initialisiert."));

  delay(1000);

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
  radio.XTAL = true;  // Da dein Shield keinen TCXO nutzt, stellen wir XTAL auf true
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
  static bool sendingOperation = false;

  if (buttonState == LOW) {
  sendingOperation = !sendingOperation; // Toggle
  
  if(sendingOperation) {
    Serial.println("Sender operating");
  } else {
    Serial.println("Sender operation shut off");
  }
  delay(2000); // Entprellen/Warten
}

  if (sendingOperation == true) {
    // Temperatur vom BMP280 auslesen
    sensors_event_t temp_event, pressure_event;
    bmp_temp->getEvent(&temp_event);
    bmp_pressure->getEvent(&pressure_event);

    // Beschleunigung von BNO055 auslesen
    sensors_event_t accelerometerData;
    bno.getEvent(&accelerometerData, Adafruit_BNO055::VECTOR_ACCELEROMETER);

    package sendeDaten = {temp_event.temperature, accelerometerData.acceleration.x, accelerometerData.acceleration.y, accelerometerData.acceleration.z};
    Serial.println("Sende Datenpaket mit: ");
    Serial.println("Temperature: ");
    Serial.println(sendeDaten.temp);
    Serial.println("X: ");
    Serial.println(sendeDaten.acce_x, 4);
    Serial.println("Y: ");
    Serial.println(sendeDaten.acce_y, 4);
    Serial.println("Z: ");
    Serial.println(sendeDaten.acce_z, 4);

    digitalWrite(LED_TX, HIGH); // Indication: sending
    int state = radio.transmit((uint8_t*)&sendeDaten, sizeof(sendeDaten));
    digitalWrite(LED_TX, LOW);  // Indication: sending off

    // Überprüfen, ob das Senden erfolgreich war
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
    delay(3000);
  }
}