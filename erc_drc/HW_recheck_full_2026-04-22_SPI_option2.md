# Recheck HW completo (SPI opción 2)

Fecha: 2026-04-22
Proyecto: `esp32_eth_dect_nrf9151_gateway_min`

## 1) Estado global EDA

- DRC PCB: PASS (0 violaciones, 0 pads sin conectar)
  - `erc_drc/drc_recheck_full_2026-04-22.rpt`
- DRC Paridad SCH/PCB: PASS (0 issues)
  - `erc_drc/drc_recheck_full_parity_2026-04-22.rpt`
- ERC SCH: PASS eléctrico con warnings no funcionales de librería (`footprint_link_issues`, 21)
  - `erc_drc/erc_recheck_full_2026-04-22.rpt`

## 2) Comprobación de conexiones por funcionalidad

### A) Gateway Ethernet (ESP32 <-> W5500 <-> RJ45)

PASS

- `ESP_SPI1_SCLK/MISO/MOSI/CS_N`: U1 <-> U2
- `ETH_INT_N`, `ETH_RST_N`: U1 <-> U2
- `ETH_TXP/TXN`, `ETH_RXP/RXN`: U2 <-> J1

Longitudes aproximadas (suma de segmentos PCB):
- `ETH_TXP`: 50.0 mm
- `ETH_TXN`: 51.7 mm (skew TX: 1.7 mm)
- `ETH_RXP`: 54.0 mm
- `ETH_RXN`: 55.5 mm (skew RX: 1.5 mm)

### B) PoE + árbol de alimentación

PASS

- J1 PoE: `POE_VP/POE_VN` -> U4 (PD 802.3af)
- U4 salida -> `+5V_POE`
- U5: `+5V_POE` -> `+3V3_SYS`
- OR-ing con USB: D7 (`+5V_USB` -> `+5V_POE`)

### C) USB PC (alimentación + debug + auto-program)

PASS

- J2 VBUS -> F2 -> `+5V_USB`
- J2 D+/D- -> U6 (`USB_DP/USB_DM`)
- U6 UART -> ESP32: `ESP_U0RXD`, `ESP_U0TXD`
- U6 `USB_DTR/USB_RTS` -> U7 -> `ESP_BOOT0/ESP_EN`

### D) nRF9151 (DECT NR+) control host

PASS (forzado a SPI por ensamblaje)

- SPI activo (puentes montados):
  - `SJ201=0R_MOUNTED` (`ESP_SPI2_SCLK` <-> `NRF_SPI_SCLK`)
  - `SJ202=0R_MOUNTED` (`ESP_SPI2_MISO` <-> `NRF_SPI_MISO`)
  - `SJ203=0R_MOUNTED` (`ESP_SPI2_MOSI` <-> `NRF_SPI_MOSI`)
  - `SJ204=0R_MOUNTED` (`ESP_SPI2_CS_N` <-> `NRF_SPI_CSN`)
- UART deshabilitado por ensamblaje:
  - `SJ101..SJ104 = DNP`

### E) RF DECT + antena externa

PASS (layout/conectividad)

- Cadena RF: U3 `DECT_RF` -> L301 (0R montado) -> `DECT_RF_ANT` -> J3 SMA
- Pi-network presente con shunts DNP:
  - `C301 = DNP` (`DECT_RF` a GND)
  - `C302 = DNP` (`DECT_RF_ANT` a GND)
- Reglas RF observadas:
  - width RF detectado: 0.45 mm
  - vias en nets RF: 0
  - keep-out antena presente en 4 capas: `F.Cu`, `In1.Cu`, `In2.Cu`, `B.Cu`

## 3) Conclusión

Conectividad HW y funcionalidades mínimas quedan validadas a nivel de diseño (SCH/PCB) para:
- Ethernet gateway
- PoE + USB power/debug
- nRF9151 en SPI (opción 2 fija)
- Ruta RF hacia SMA externa

Nota: esta verificación es EDA (diseño). No sustituye pruebas en banco (link Ethernet real, tráfico SPI real, RSSI/cobertura de antena en campo).
