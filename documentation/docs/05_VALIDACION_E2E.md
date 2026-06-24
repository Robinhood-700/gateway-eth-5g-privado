# 05 - Pruebas de validacion E2E

## Preparacion
- Pasarela energizada por PoE (`802.3af`) y conectada a LAN con DHCP.
- nRF9151 conectado y con firmware AT operativo (UART o SPI segun `menuconfig`).
- PC de prueba en la misma LAN.

## T1 - Ethernet con IP valida
1. Arrancar pasarela.
2. Verificar en logs `Ethernet link up` + `Ethernet got IP`.
3. Criterio OK: IP IPv4 asignada y estable.

## T1b - Alimentacion PoE (PGOOD)
1. Configurar `CONFIG_GW_POE_PGOOD_GPIO` al pin de power-good del PD.
2. En consola USB (`gwdbg>`) ejecutar:
   - `poe`
3. Criterio OK: `result=ok`.

## T2 - Enlace nRF9151 y AT
1. Verificar log de `AT link validated`.
2. Forzar reset nRF9151 (hardware o comando) y esperar reconexion.
3. Criterio OK: supervisor detecta fallo AT y recupera enlace.

## T3 - Ethernet -> nRF9151 (JSON)
1. Desde PC enviar UDP JSON:
```bash
echo '{"dir":"eth_to_nrf","seq":1}' | nc -u -w1 <IP_GATEWAY> 5000
```
2. Criterio OK: log de forward sin error hacia nRF9151.

## T4 - nRF9151 -> Ethernet (JSON)
1. Inyectar desde nRF9151 una linea con prefijo configurado (default `+DATA:`), por ejemplo:
   - `+DATA:{"dir":"nrf_to_eth","seq":1}`
2. Escuchar en PC:
```bash
nc -u -l 5000
```
3. Criterio OK: JSON recibido en PC.

## T5 - Resiliencia basica
1. Desconectar cable Ethernet 10 s y reconectar.
2. Criterio OK: recuperación de link/IP y continuidad de bridge.
3. Bloquear temporalmente respuesta AT del nRF9151.
4. Criterio OK: reinicializacion automatica del enlace nRF.

## T6 - Integridad JSON
1. Enviar payload no JSON por UDP.
2. Criterio OK: payload reenviado encapsulado como:
   - `{"enc":"hex","data":"..."}`

## T7 - Script E2E automatizado (USB + Ethernet)
1. Instalar dependencia:
```bash
pip install pyserial
```
2. Ejecutar:
```bash
python3 firmware/esp32_gateway/tools/usb_debug/gateway_usb_debug.py e2e \
  --serial /dev/ttyUSB0 \
  --gateway-ip <IP_GATEWAY> \
  --listen-port 5000 \
  --remote-port 5000 \
  --count 20
```
3. Criterio OK:
   - `ETH->DECT forwarded` >= `count`
   - `DECT->ETH UDP received` >= `count`
   - `RESULT: PASS`
