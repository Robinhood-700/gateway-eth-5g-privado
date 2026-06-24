# 04 - Procedimiento minimo de build/flash

## Prerrequisitos
- ESP-IDF 6.1 instalado.
- Toolchain `xtensa-esp-elf` para `esp32`.
- Puerto USB-UART/debug conectado al ESP32.

## Build base (SPI, default)
```bash
cd /Users/antonio/Documents/esp32-eth-dect-nrf9151-gateway-hw-docs/kicad_complete_2026-04-22/firmware/esp32_gateway
source /Users/antonio/esp/esp-idf/export.sh
idf.py set-target esp32
idf.py build
```

## Build alternativo (UART por software)
```bash
cd /Users/antonio/Documents/esp32-eth-dect-nrf9151-gateway-hw-docs/kicad_complete_2026-04-22/firmware/esp32_gateway
source /Users/antonio/esp/esp-idf/export.sh
idf.py -B build-uart -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.uart' set-target esp32 build
```

## Configuracion minima por menuconfig
```bash
idf.py menuconfig
```
- `Gateway MVP configuration` -> `nRF9151 host interface` (`UART` o `SPI`).
- `Gateway MVP configuration` -> pines W5500 y nRF9151.
- `Gateway MVP configuration` -> puertos UDP y timeouts (`ETH_RESTART`, `AT_TIMEOUT`, `SUPERVISOR_PERIOD`).

## Flash + monitor
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Actualizacion por USB con script
```bash
cd /Users/antonio/Documents/esp32-eth-dect-nrf9151-gateway-hw-docs/kicad_complete_2026-04-22/firmware/esp32_gateway
./tools/usb_debug/flash_usb.sh /dev/ttyUSB0 460800
```

## Consola de debugging USB
- En monitor aparece prompt `gwdbg>`.
- Comandos utiles:
  - `status`
  - `bridge_stats`
  - `at`
  - `poe`
  - `fw_update`

## Logs esperados de arranque
- `Ethernet link up`
- `Ethernet got IP: ...`
- `AT link validated` (o reintentos de supervisor)
- `Gateway bridge started (ETH<->DECT)`
