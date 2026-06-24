# ESP32 Gateway Firmware (Ethernet <-> DECT NR+)

Firmware MVP en ESP-IDF para pasarela bidireccional:
- Ethernet (W5500 por SPI) <-> DECT NR+ (nRF9151 host por UART o SPI)
- Payload siempre en JSON
- Watchdog + reconexion basica de ambos enlaces

## Estructura
- `main/app_main.c`: secuencia de arranque
- `main/Kconfig.projbuild`: configuracion de pines/interfaz/puertos
- `components/gw_eth`: link Ethernet W5500 + estado + restart
- `components/gw_dect`: link DECT host (UART/SPI), AT health-check y RX callback
- `components/gw_json`: validacion JSON y envoltura hex para payload no JSON
- `components/gw_bridge`: bridge UDP ETH->DECT y DECT->UDP
- `components/gw_watchdog`: supervisor de enlace Ethernet/DECT
- `components/gw_diag`: consola de debugging USB + trafico aleatorio de ejemplo

## Flujo
1. UDP RX en `CONFIG_GW_UDP_LISTEN_PORT`.
2. Si llega JSON, se reenvia al enlace DECT; si no es JSON, se encapsula como:
   `{"enc":"hex","data":"..."}`.
3. Datos desde DECT se envian por UDP a:
   - `CONFIG_GW_UDP_REMOTE_IP:CONFIG_GW_UDP_REMOTE_PORT` (si IP fija), o
   - ultimo peer UDP que envio trafico al gateway.

## Debug USB y pruebas
- Consola USB disponible en `idf.py monitor` con prompt `gwdbg>`.
- Comandos principales:
  - `status`, `bridge_stats`, `reset_stats`
  - `at`, `poe`, `set_peer <ip> <port>`
  - `rand_eth <count>`, `rand_dect <count>`
  - `fw_update`, `reboot`
- Scripts host en `tools/usb_debug/`:
  - `flash_usb.sh`: flash + monitor por USB
  - `gateway_usb_debug.py`: comandos USB y test E2E

## Build (ESP-IDF 6.x)
```bash
cd /Users/antonio/Documents/esp32-eth-dect-nrf9151-gateway-hw-docs/kicad_complete_2026-04-22/firmware/esp32_gateway
source $IDF_PATH/export.sh
idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Build UART alternativo (sin tocar build SPI base):
```bash
idf.py -B build-uart -DSDKCONFIG_DEFAULTS='sdkconfig.defaults;sdkconfig.uart' set-target esp32 build
```

## Notas DECT
- `CONFIG_GW_DECT_TX_PREFIX` define prefijo para uplink (`AT+DATA=` por defecto).
- `CONFIG_GW_DECT_RX_PREFIX` define prefijo URC para downlink (`+DATA:` por defecto).
- UART: parser por lineas `\n`.
- SPI: modo polling full-duplex (requiere firmware puente en nRF9151 que exponga stream ASCII por SPI).
- `sdkconfig.uart` incluido para activar UART por software en un build separado.
- `CONFIG_GW_EXAMPLE_RANDOM_ENABLE` habilita generacion automatica de datos aleatorios en ambos sentidos (demo).
