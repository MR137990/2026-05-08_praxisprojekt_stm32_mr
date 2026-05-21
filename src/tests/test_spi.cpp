#include <Arduino.h>
#include <SPI.h>

// Deine Pin-Konfiguration
#define LR_NSS    7 
#define LR_RESET  A0
#define LR_BUSY   3
#define LED_RX    A5
#define LED_TX    A4

void wait_busy() {
  // Der LR1121 signalisiert "Beschäftigt" durch ein HIGH Signal
  while(digitalRead(LR_BUSY) == HIGH) {
    yield(); // Erlaubt dem System Hintergrundaufgaben (wichtig für Stabilität)
  }
}

void setup() {
  Serial.begin(115200);
  
  // Pins initialisieren
  pinMode(LR_NSS, OUTPUT);
  pinMode(LR_RESET, OUTPUT);
  pinMode(LR_BUSY, INPUT);
  pinMode(LED_RX, OUTPUT);
  pinMode(LED_TX, OUTPUT);
  
  digitalWrite(LR_NSS, HIGH);
  
  // Manueller Hardware-Reset (Essenziell für LR1121 nach dem Power-Up)
  Serial.println("LR1121 Reset...");
  digitalWrite(LR_RESET, LOW);
  delay(50);
  digitalWrite(LR_RESET, HIGH);
  delay(100); // Zeit zum Booten geben

  // SPI starten
  SPI.begin();
  
  Serial.println("Sende GetVersion Kommando...");
}

void loop() {
  // 1. Warten bis Chip bereit
  wait_busy();

  // 2. SPI Transaktion starten
  digitalWrite(LR_NSS, LOW);
  
  // Kommando: GetVersion (Opcode 0x01 0x01)
  SPI.transfer(0x01); 
  SPI.transfer(0x01);
  
  // Status-Byte lesen (Semtech Chips senden immer zuerst ein Status-Byte)
  byte status = SPI.transfer(0x00); 
  
  // Antwort-Bytes (4 Bytes für Hardware-Typ, Version, etc.)
  byte hw_type = SPI.transfer(0x00);
  byte hw_vers = SPI.transfer(0x00);
  byte fw_boot = SPI.transfer(0x00);
  byte fw_vers = SPI.transfer(0x00);
  
  digitalWrite(LR_NSS, HIGH);

  // Ergebnisse ausgeben
  Serial.print("Status: 0x"); Serial.println(status, HEX);
  Serial.printf("Chip-Info: Typ 0x%02X, Version 0x%02X, FW %d.%d\n", hw_type, hw_vers, fw_boot, fw_vers);

  // LED-Test
  digitalWrite(LED_TX, HIGH);
  delay(500);
  digitalWrite(LED_TX, LOW);
  digitalWrite(LED_RX, HIGH);
  delay(500);
  digitalWrite(LED_RX, LOW);

  delay(2000); // Alle 2 Sekunden wiederholen
}