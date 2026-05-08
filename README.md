# Sensorknoten Hub - SS_2026 (HSD Düsseldorf)

Dieses Projekt umfasst die Entwicklung eines integrierten Sensorknotens im **Chip-Down Design**. 
Ziel ist die Kombination aus moderner Funktechnologie (Semtech LR1121) und leistungsstarker 
Rechenpower (RP2350 / STM32L4).

## Kernkomponenten
* **MCU:** STM32L476RG (Prototyp) -> Migration auf RP2350 (Ziel)
* **Radio:** Semtech LR1121 (LoRa, Wi-Fi Sniffing, GNSS)
* **Sensorik:** Bosch BMP280 (Luftdruck & Temperatur)

## Projektstatus
- [x] SPI-Verbindung zwischen MCU und LR1121 erfolgreich getestet
- [ ] I2C-Integration BMP280
- [ ] LoRa Ping-Pong Kommunikation
- [ ] PCB-Design & Hardware-Routing
- [ ] Finales Chip-Down Assembly

## Setup
Dieses Projekt nutzt **PlatformIO** innerhalb von VS Code. 
Die verschiedenen Test-Umgebungen können über die `platformio.ini` ausgewählt werden.
