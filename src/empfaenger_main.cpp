#include "config.h"
#include <RadioLib.h>

int lastState = HIGH; //Pull-up auf High heißt Button nicht gedrückt, LOW heißt Button gedrückt

LR1121 radio = new Module(LR_NSS, LR_IRQ, LR_RESET, LR_BUSY);

LoRaConfig activeConfig;

// --- Globaler Schlüssel für die XOR-Entschlüsselung (Muss identisch zum Sender sein!) ---
const uint8_t xorKey[] = { 0x5A, 0xA5, 0x1F, 0xE1, 0x3C, 0xC3, 0x7F, 0xF7 };
const size_t keySize = sizeof(xorKey);

void wait_busy() {
  // Der LR1121 signalisiert "Beschäftigt" durch ein HIGH Signal
  while(digitalRead(LR_BUSY) == HIGH) {
    yield(); // Erlaubt dem System Hintergrundaufgaben (wichtig für Stabilität)
  }
}

void setup() {
Serial.begin(115200);
  while(!Serial);

  Serial.println("===========EMPFAENGER!==============");
  Serial.println("Serial gestartet!");

  // SPI-Bus starten
  SPI.begin();

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
  delay(350); // Dem Chip exakt 300-350ms Zeit geben, um die Spannungsregler intern hochzufahren
  Serial.println("LR1121 Hardware Reset abgeschlossen.");

  // start RadioLib mit TCXO Konfiguration für PCB_E655V01A
  Serial.println("Starte radio.begin().");
  // Da dein Shield einen TCXO nutzt, stellen wir XTAL auf false
  radio.XTAL = true; 
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

  radio.startReceive();
}

void loop(){
  if(digitalRead(LR_IRQ) == HIGH) {
    package empfangeneDaten;
    int state = radio.readData((uint8_t*)&empfangeneDaten, sizeof(empfangeneDaten));

    if(state == RADIOLIB_ERR_NONE) {
      
      // =========================================================================
      // INLINE XOR-ENTSCHLÜSSELUNG (Klartext wiederherstellen)
      // =========================================================================
      // Wir betrachten das empfangene Struct als reines Byte-Array
      uint8_t* bytePointer = (uint8_t*)&empfangeneDaten;
      size_t datenGroesse = sizeof(empfangeneDaten);

      // Symmetrische Rückrechnung: Ciphertext XOR Key = Klartext
      for (size_t i = 0; i < datenGroesse; i++) {
        bytePointer[i] ^= xorKey[i % keySize];
      }
      // =========================================================================

      // Ab hier liegen die Daten in 'empfangeneDaten' wieder im originalen Klartext vor!
      
      // packet was successfully received
      Serial.println(F("[LR1121] Received packet!"));

      // print data of the packet
      Serial.println(F("[LR1121] Data:"));
      Serial.println("Temperature:");
      Serial.println(empfangeneDaten.temp);
      Serial.println("X:");
      Serial.println(empfangeneDaten.acce_x, 4);
      Serial.println("Y:");
      Serial.println(empfangeneDaten.acce_y, 4);
      Serial.println("Z:");
      Serial.println(empfangeneDaten.acce_z, 4);

      // print RSSI (Received Signal Strength Indicator)
      Serial.print(F("[LR1121] RSSI:\t\t"));
      Serial.print(radio.getRSSI());
      Serial.println(F(" dBm"));

      // print SNR (Signal-to-Noise Ratio)
      Serial.print(F("[LR1110] SNR:\t\t"));
      Serial.print(radio.getSNR());
      Serial.println(F(" dB"));
      
      // WICHTIG: Nach dem Empfang den Chip wieder explizit in den Empfangsmodus versetzen!
      radio.startReceive();
    } 
  }
}