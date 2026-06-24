# 03 - Firmware minimo (ESP-IDF)

## Ubicacion
- Proyecto: `kicad_complete_2026-04-22/firmware/esp32_gateway`

## Modulos y archivos
- `main/app_main.c`
  - Inicializa NVS, red (`esp_netif` + event loop), Ethernet, DECT, bridge y supervisor.
- `components/gw_eth/gw_eth.c/.h`
  - Driver W5500 por SPI (ESP-IDF 6.x con `espressif/w5500`).
  - Estado de enlace/IP y reinicio basico del enlace Ethernet.
- `components/gw_dect/gw_dect.c/.h`
  - Enlace host ESP32 <-> nRF9151 seleccionable por software: `UART` o `SPI`.
  - Comando AT de healthcheck (`AT` + espera de `OK`).
  - TX con prefijo configurable (`CONFIG_GW_DECT_TX_PREFIX`, default `AT+DATA=`).
  - RX con filtro por prefijo configurable (`CONFIG_GW_DECT_RX_PREFIX`, default `+DATA:`).
- `components/gw_bridge/gw_bridge.c/.h`
  - Tarea `Ethernet UDP -> DECT`.
  - Tarea `DECT -> Ethernet UDP`.
  - Mecanismo de peer remoto fijo o dinamico (ultimo emisor UDP).
- `components/gw_json/gw_json.c/.h`
  - Valida JSON de entrada.
  - Si no es JSON, encapsula como `{"enc":"hex","data":"..."}`.
- `components/gw_watchdog/gw_watchdog.c/.h`
  - Supervisor periodico de Ethernet y DECT con reconexion basica.
- `components/gw_diag/gw_diag.c/.h`
  - Consola de debugging por USB (`gwdbg`).
  - Comandos de prueba para Ethernet, DECT, PoE, estadisticas del bridge.
  - Generador de datos aleatorios de ejemplo en ambos sentidos.
- `main/Kconfig.projbuild`
  - Configuracion minima (pines, puertos UDP, timeouts, interfaz UART/SPI).
- `main/idf_component.yml`
  - Dependencia gestionada: `espressif/w5500`.

## Flujo de datos (MVP)
1. Ethernet recibe UDP en `CONFIG_GW_UDP_LISTEN_PORT` (default `5000`).
2. Se garantiza salida JSON hacia DECT:
   - JSON valido: pasa directo.
   - No JSON/binario: encapsulado hex.
3. Mensajes DECT recibidos (prefijo RX configurado) se envian por UDP Ethernet.
4. Tambien se garantiza salida JSON hacia Ethernet:
   - JSON valido: pasa directo.
   - No JSON/binario: encapsulado hex.
5. Destino UDP de salida:
   - `CONFIG_GW_UDP_REMOTE_IP` vacio: ultimo peer UDP visto.
   - `CONFIG_GW_UDP_REMOTE_IP` definido: peer fijo + `CONFIG_GW_UDP_REMOTE_PORT`.

## Seleccion UART/SPI por software
- Via `idf.py menuconfig`:
  - `Gateway MVP configuration` -> `nRF9151 host interface` -> `UART` o `SPI`.
- Alternativa automatizada:
  - Overlay `sdkconfig.uart` para generar build UART sin tocar la configuracion base SPI.

## Watchdog y reconexion
- Supervisor periodico cada `CONFIG_GW_SUPERVISOR_PERIOD_MS`.
- Si Ethernet cae mas de `CONFIG_GW_ETH_RESTART_SEC`, reinicia W5500.
- Si fallan healthchecks AT consecutivos, reinicia enlace DECT.

## Debug USB y trafico aleatorio de ejemplo
- Consola por USB-UART con prompt `gwdbg>`.
- Comandos:
  - `status`, `bridge_stats`, `reset_stats`
  - `at`, `poe`
  - `set_peer <ipv4> <port>`
  - `rand_eth <count>` (simula ingreso por Ethernet UDP)
  - `rand_dect <count>` (simula ingreso por DECT)
  - `fw_update`, `reboot`
- Configuracion relevante:
  - `CONFIG_GW_USB_DEBUG_ENABLE`
  - `CONFIG_GW_POE_PGOOD_GPIO` / `CONFIG_GW_POE_PGOOD_ACTIVE_LEVEL`
  - `CONFIG_GW_EXAMPLE_RANDOM_ENABLE` y parametros de burst/periodo

## Suposiciones del lado nRF9151
- UART: firmware del nRF9151 responde `OK` al comando `AT`.
- SPI: firmware puente propietario (stream ASCII por SPI) para encapsular comandos AT/URC.
