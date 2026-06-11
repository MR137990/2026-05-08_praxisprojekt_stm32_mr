// include/config.h
#ifndef CONFIG_H
#define CONFIG_H

// 1. Bibliotheken einbinden
#include <Arduino.h> 
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BNO055.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_Sensor.h>

// 2. Pin-Definitionen (Semtech Shield Pin Konfiguration)
#define LR_NSS    D7
#define LR_RESET  A0
#define LR_BUSY   D3
#define LR_IRQ    D5
#define LED_RX    A5
#define LED_TX    A4

// Falls noch nicht definiert, füge deine Button-Pins hier hinzu:
//#define USER_BTN  D2  // Passe den Pin an dein Nucleo-Board an!

// 3. Datentypen
struct __attribute__((packed)) package {
    // LoRa Identifier 
    //int ident; //Ist das der richtige Weg? Übergebe über main
    // --- BMP280 Daten ---
    float temp;       // 4 Bytes: Temperatur in °C
    //float pres;       // 4 Bytes: Luftdruck in hPa

    // --- BNO055 Orientierungsdaten (Euler-Winkel) ---
    //float orie_x;  // 4 Bytes: Heading / Gierwinkel (0° bis 360°)
    //float orie_y;  // 4 Bytes: Rollwinkel (-90° bis +90°)
    //float orie_z;  // 4 Bytes: Nickwinkel / Pitch (-180° bis +180°)

    // --- BNO055 Beschleunigungsdaten (Linear) ---
    float acce_x; // 4 Bytes: Beschleunigung X-Achse in m/s²
    float acce_y; // 4 Bytes: Beschleunigung Y-Achse in m/s²
    float acce_z; // 4 Bytes: Beschleunigung Z-Achse in m/s²
};

struct LoRaConfig {
    float frequency;
    int8_t txPower;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
};

// Preset 1: Europa 868 MHz (Max 14 dBm)
const LoRaConfig config868 = { 868.1, 14, 125.0, 7, 7 };

// Preset 2: Europa 433 MHz (Max 10 dBm)
const LoRaConfig config433 = { 433.175, 10, 125.0, 9, 7 };

#endif