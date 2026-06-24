# Verificacion HW Exhaustiva - ESP32 <-> Ethernet <-> DECT NR+ (nRF9151)

> Nota 2026-04-22 (recheck): este documento fue superado por `HW_recheck_full_2026-04-22_SPI_option2.md`.
> Estado actual de ensamblaje: SPI fijo por hardware (`SJ201-204=0R_MOUNTED`, `SJ101-104=DNP`).

Fecha: 2026-04-22
Proyecto: `esp32_eth_dect_nrf9151_gateway_min`
Ruta: `kicad_complete_2026-04-22/project_snapshot`

## 1) Resultado global

- DRC PCB: PASS (0 violaciones, 0 pads sin conectar).
- Paridad SCH<->PCB: PASS (0 issues).
- ERC: PASS con warnings no electricos (21 avisos de `footprint_link_issues` por simbolos de libreria local con `lib ""`).
- Conectividad de pads a net en PCB: PASS (0 pads en net 0 para footprints electricos).

## 2) Evidencia automatica

Archivos generados:

- `kicad_complete_2026-04-22/erc_drc/erc_exhaustive_2026-04-22.rpt`
- `kicad_complete_2026-04-22/erc_drc/drc_exhaustive_2026-04-22.rpt`
- `kicad_complete_2026-04-22/erc_drc/drc_exhaustive_parity_2026-04-22.rpt`
- `kicad_complete_2026-04-22/erc_drc/netlist_exhaustive_2026-04-22.net`

## 3) Matriz de conectividad por subsistema

### 3.1 Power PoE + Arbol de alimentacion

PASS - cadena validada:

- J1 PoE: `POE_VP/POE_VN` conectados a U4 `VIN+`/`VIN-`.
- U4 salida `+5V_POE` conectada a U5 `VIN`.
- U5 `VOUT` genera `+3V3_SYS` para U1/U2/U3/U6/U7.

Nets clave (netlist):

- `+5V_POE`: D7-K, U4(+VDC_1,+VDC_2), U5-VIN.
- `POE_VP`: J1-5, U4-5, U4-6.
- `POE_VN`: J1-6, U4-7, U4-8.
- `+3V3_SYS`: U1-1, U2-1, U3-1/U3-16, U5-3, U6-5, U7-3.

### 3.2 USB PC (alimentacion + debug + auto-program)

PASS - rutas validadas:

- `+5V_USB_RAW`: J2-VBUS -> F2-IN.
- `+5V_USB`: F2-OUT -> U6-VBUS -> D7-A.
- OR-ing a rail de sistema: D7-K -> `+5V_POE`.
- USB datos: J2 D+/D- -> U6 USB_DP/USB_DM.
- UART debug ESP32: U6 TXD/RXD -> ESP_U0RXD/ESP_U0TXD.
- Auto-program: U6 DTR/RTS -> U7 -> ESP_BOOT0/ESP_EN.

### 3.3 Ethernet gateway

PASS - enlace fisico + host SPI validados:

- U1 <-> U2 por SPI1 (`SCLK/MISO/MOSI/CS_N`) + `ETH_INT_N` + `ETH_RST_N`.
- U2 <-> J1 por pares Ethernet (`ETH_TXP/TXN/RXP/RXN`).

Metrica de routing (aprox por segmentos):

- TX+: ~50.0 mm, TX-: ~51.7 mm (desfase ~1.7 mm).
- RX+: ~54.0 mm, RX-: ~55.5 mm (desfase ~1.5 mm).

### 3.4 DECT NR+ (nRF9151)

PASS (software-selectable con enlaces montados):

- U3 alimentacion y control: `+3V3_SYS`, `GND`, `NRF_IRQ`, `NRF_RST_N`.
- U3 UART y SPI presentes en nets dedicadas.
- Enlaces de interfaz configurados como `0R_MOUNTED` en SJ101..SJ104 y SJ201..SJ204 para habilitar ambas rutas en hardware.
- Seleccion final del canal por firmware (UART o SPI) sin rework de jumpers.

### 3.5 RF antena DECT

PASS CONDICIONAL (matching final de produccion):

- Topologia Pi presente: C301 (shunt) - L301 (serie) - C302 (shunt).
- Conector SMA externo J3 con GND conectado (pin 2 a GND).
- Neta RF separada en dos tramos: `DECT_RF` y `DECT_RF_ANT`.
- Reglas RF activas (DRU):
  - ancho 0.45 mm en `DECT_RF` y `DECT_RF_ANT`.
  - sin vias RF.
  - clearance minimo RF.
- Verificacion de layout:
  - segmentos RF en 0.45 mm (cumple).
  - vias RF: no detectadas (cumple).

Keep-out antena:

- Keepout cobre/vias/tracks bajo antena en las 4 capas (`F.Cu`, `In1.Cu`, `In2.Cu`, `B.Cu`).

## 4) Comprobacion de pads/componentes en PCB

Resumen footprints electricos parseados:

- 25 footprints electricos detectados.
- Todos los pads tienen net asignada distinta de net 0.
- Sin pads unconnected segun DRC.

## 5) Riesgos residuales / acciones obligatorias antes de fabricacion

1. IMPORTANTE: huellas/simbolos placeholder en bloques principales (U1/U2/U3/J1/J2/U4/U5/U6/U7).
   - La conectividad logica/EDA es consistente, pero no valida por si sola pinout mecanico final de componentes reales.
   - Accion: congelar footprints finales manufacturer-accurate y rerun ERC/DRC/paridad.

2. IMPORTANTE: SWD de nRF9151 (`SWDIO`/`SWDCLK`) quedan en nets de un solo nodo (sin header/testpoint).
   - Accion recomendada: agregar header/testpads SWD si se requiere debug/programacion nRF in-circuit.

3. IMPORTANTE RF: C301/C302 estan como DNP (correcto para tuning inicial), pero requiere plan de ajuste VNA.
   - Accion: definir ventana de componentes de matching y procedimiento de tuning en EVT.

## 6) Conclusión de verificación

- La conectividad HW del diseño actual queda validada a nivel EDA (esquematico+PCB) para:
  - Gateway Ethernet <-> ESP32 <-> nRF9151 (DECT NR+).
  - Alimentacion PoE + inyeccion USB.
  - Puerto USB de alimentacion/debug/autoprogramacion ESP32.
  - Camino RF con reglas y keepouts definidos.

- El diseño es funcionalmente coherente para prototipo, con condiciones de montaje (jumpers + L301 poblado) y con riesgos de industrializacion pendientes (footprints finales reales y tuning RF).
