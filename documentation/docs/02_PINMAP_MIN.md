# 02 - Pinmap minimo

## Ethernet W5500 (SPI dedicado)
- `GPIO18` -> `W5500_SCLK`
- `GPIO19` <- `W5500_MISO`
- `GPIO23` -> `W5500_MOSI`
- `GPIO21` -> `W5500_CS`
- `GPIO22` <- `W5500_INT`
- `GPIO2`  -> `W5500_RST`

## nRF9151 opcion UART (MVP recomendado)
- `GPIO17` ESP32 TX -> nRF9151 RX
- `GPIO16` ESP32 RX <- nRF9151 TX
- `GPIO14` ESP32 RTS -> nRF9151 CTS (opcional)
- `GPIO15` ESP32 CTS <- nRF9151 RTS (opcional)
- `GPIO4`  -> `NRF_RST`
- `GPIO27` <- `NRF_IRQ`

## nRF9151 opcion SPI (requiere FW puente)
- `GPIO25` -> `NRF_SPI_SCLK`
- `GPIO34` <- `NRF_SPI_MISO`
- `GPIO32` -> `NRF_SPI_MOSI`
- `GPIO33` -> `NRF_SPI_CS`
- `GPIO4`  -> `NRF_RST` (por defecto en firmware; configurable)
- `GPIO27` <- `NRF_IRQ`

## USB-UART para PC (programacion + logs)
- `CP2102N_TXD` -> `GPIO3/U0RXD` (ESP32)
- `CP2102N_RXD` <- `GPIO1/U0TXD` (ESP32)
- `DTR` (CP2102N) -> `GPIO0/BOOT` mediante transistor NPN
- `RTS` (CP2102N) -> `EN/CHIP_PU` mediante transistor NPN
- `GPIO0`: pull-up `10k` a `3V3`
- `EN`: pull-up `10k` a `3V3` y `1uF` a GND

## Seleccion fisica UART/SPI en hardware
- UART activo por defecto: `SJ101-SJ104` cerrados.
- SPI inactivo por defecto: `SJ201-SJ204` abiertos.
- Para SPI: abrir `SJ101-SJ104` y cerrar `SJ201-SJ204`.
