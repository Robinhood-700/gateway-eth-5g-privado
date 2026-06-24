# 06 - Puerto USB minimo para PC (alimentacion + debug/programacion)

## Objetivo
Un unico conector USB-C para:
- alimentar placa desde PC (`+5V_USB`), y
- programar/depurar por UART (`CP2102N` <-> `UART0 ESP32`).

## Configuracion minima
1. `J2` USB-C UFP
- `CC1` -> `5.1k` -> GND
- `CC2` -> `5.1k` -> GND
- `VBUS` -> `F2` (500 mA) -> `+5V_USB`
- `D+`/`D-` -> `U6 CP2102N D+`/`D-`

2. OR-ing de alimentacion
- `D6` Schottky: `+5V_POE -> +5V_SYS`
- `D7` Schottky: `+5V_USB -> +5V_SYS`

3. USB-UART
- `CP2102N_TXD` -> `ESP32 U0RXD (GPIO3)`
- `CP2102N_RXD` <- `ESP32 U0TXD (GPIO1)`

4. Auto-programacion ESP32
- `DTR` -> transistor NPN -> `GPIO0`
- `RTS` -> transistor NPN -> `EN`
- `GPIO0` con pull-up `10k` a `3V3`
- `EN` con pull-up `10k` a `3V3` + `1uF` a GND

## Resultado esperado en PC
- Puerto serie para `idf.py flash monitor`
- Flash automatico sin pulsar botones
- Alimentacion alternativa por USB cuando no hay PoE
