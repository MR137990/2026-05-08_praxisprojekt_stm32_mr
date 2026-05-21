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

#endif