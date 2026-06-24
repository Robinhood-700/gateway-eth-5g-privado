# 01 - Configuracion minima de hardware (70 x 50 mm)

## Alcance de esta revision
- Formato PCB: `70 mm x 50 mm`.
- Bloques incluidos: `PoE 802.3af`, `ESP32`, `Ethernet SPI (W5500)`, `nRF9151`, `USB-C PC (power + UART debug/program)`.
- Bloques excluidos: LTE/LoRa/RS-485/SDI-12/4-20 mA/pulsos/GPS/gestion solar-bateria.

## Diagrama de bloques
1. `RJ45 MagJack PoE` -> `PD 802.3af (Ag9905M)` -> `+5V_POE`
2. `USB-C (PC)` -> fusible -> `+5V_USB`
3. `+5V_POE` OR `+5V_USB` -> `+5V_SYS`
4. `+5V_SYS` -> `Buck 3V3 (R-78B3.3-2.0)` -> `+3V3_SYS`
5. `ESP32-WROOM-32E` alimentado por `+3V3_SYS`
6. `ESP32 SPI1` <-> `W5500`
7. `ESP32` <-> `nRF9151` por `UART` o `SPI` (seleccion por jumpers)
8. `USB-C D+/D-` <-> `CP2102N` <-> `UART0 ESP32` (+ auto-programacion por DTR/RTS)

## Componentes base ya cargados en KiCad
- `U1`: ESP32 `RF_Module:ESP32-WROOM-32E`
- `U2`: W5500 `Package_QFP:LQFP-48_7x7mm_P0.5mm`
- `U3`: nRF9151 (base mecanica) `Package_LGA:Nordic_nRF9160-SIxx_LGA-102-59EP_16.0x10.5mm_P0.5mm`
- `J1`: RJ45 PoE `Connector_RJ:RJ45_Amphenol_RJMG1BD3B8K1ANR`
- `U4`: PD PoE `Ag9905M` (`Converter_DCDC:Converter_DCDC_Silvertel_Ag99xxLP_THT`)
- `U5`: Buck `R-78B3.3-2.0` (`Converter_DCDC:Converter_DCDC_RECOM_R-78B-2.0_THT`)
- `J3`: RF SMA `Connector_Coaxial:SMA_Amphenol_132291_Vertical`
- `J2`: USB-C UFP 16 pines (hoja `usb_pc_iface.kicad_sch`)
- `U6`: USB-UART `CP2102N` (hoja `usb_pc_iface.kicad_sch`)
- `U7`: bloque de auto-programacion (`DTR/RTS` -> `BOOT0/EN`)
- `F2`: fusible `500 mA` para rama `+5V_USB`
- `D7`: Schottky para inyeccion `+5V_USB` hacia rail de sistema 5V
- `R201`, `R202`: `5k1` en `CC1`/`CC2` a GND

## Notas electricas minimas
- Desacoplo minimo por rail: `100 nF` cerca de cada VDD + `10 uF` por bloque.
- Plano de GND continuo bajo ESP32/W5500; keep-out RF para zona de antena de `U3`.
- `PWR_EN` de nRF9151 fijado a `+3V3_SYS` en MVP.
- Si el MagJack seleccionado no integra rectificacion PoE completa, agregar puente de diodos segun hoja de datos del PD.
- USB-C en modo dispositivo/UFP con `CC1` y `CC2` a GND mediante `5.1 kOhm`.
- Recomendado TVS ESD en `D+`/`D-` junto a `J2`.

## Estado de verificacion KiCad
- ERC de esquema: `0 violaciones` (archivo `erc_latest.rpt`).
