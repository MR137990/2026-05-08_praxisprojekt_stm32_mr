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
    Serial.println("I2C gestartet");
  } else {
    Serial.println("Ooops, kein BMP280! Ich bin der Empfänger :)))");
//    while(1); 
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

  radio.startReceive();

}

//String meinEmpfang = "";
//String letzterEmpfang = "";

void loop(){

  if(digitalRead(LR_IRQ) == HIGH) {
    
    String str = "";
    int state = radio.readData(str);

    if(state == RADIOLIB_ERR_NONE) {
      // packet was successfully received
      Serial.println(F("[LR1121] Received packet!"));

      // print data of the packet
      Serial.print(F("[LR1121] Data:\t\t"));
      Serial.println(str);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[LR1121] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[LR1110] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));

    } 
  }
}

/*void loop() {
  // RadioLib liest die Bytes direkt in den Speicher ein
  int state = radio.receive(meinEmpfang);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("SUCCESS: radio.receive() erfolgreich durchgeführt!");
    Serial.println("TP1");
    
    // --- NEU: DEBUG-PRINTS UM DEN FEHLER ZU FINDEN ---
    Serial.print("DEBUG: Empfangene Laenge = "); Serial.println(meinEmpfang.length());
    Serial.print("DEBUG: Inhalt = '"); Serial.print(meinEmpfang); Serial.println("'");
    Serial.print("DEBUG: Letzter Stand war = '"); Serial.print(letzterEmpfang); Serial.println("'");
    // -------------------------------------------------

    // HIER DIE PRÜFUNG: Hat sich der Wert geändert?
    if (meinEmpfang != letzterEmpfang) {
      Serial.println("TP2");
      Serial.println(F("\n=== NEUE DATEN EMPFANGEN ==="));
      Serial.print(F("Temperatur: ")); Serial.print(meinEmpfang); Serial.println(F(" °C"));
      Serial.println(F("============================"));

      // Aktuelle Daten für den nächsten Vergleich merken
      letzterEmpfang = meinEmpfang;
      
    } else {
      // Daten sind identisch mit dem letzten Paket -> Ignorieren
      Serial.print("."); Serial.println("TP3");
    }
  } else {
    // Da wir das while(1) auskommentiert haben, loggen wir das Timeout nur als kleines 't'
    if (state == RADIOLIB_ERR_RX_TIMEOUT) {
      Serial.print("t");
    } else {
      Serial.print("\nRadioLib Fehler! Code: ");
      Serial.println(state);
    }
  }
}*/