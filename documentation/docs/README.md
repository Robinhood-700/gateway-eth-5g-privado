# ESP32 <-> nRF9151 Gateway MVP (KiCad + Firmware minimo)

## Estado
- Proyecto KiCad 9 creado en formato `70 x 50 mm`.
- Esquematico jerarquico con PoE + ESP32 + W5500 + nRF9151.
- Hoja USB integrada: `usb_pc_iface.kicad_sch` (USB-C + CP2102N + auto-programacion).
- Seleccion fisica UART/SPI para nRF9151 por jumpers.
- Definicion minima de puerto USB-C para PC (power + UART + auto-programacion).
- ERC actual del esquema: `0 violaciones`.

## Entregables
1. Hardware minimo: `01_HARDWARE_MIN.md`
2. Pinmap minimo: `02_PINMAP_MIN.md`
3. Firmware minimo (modulos y archivos): `03_FIRMWARE_MIN.md`
4. Kconfig/sdkconfig minimo: carpeta `../../Hardware/firmware/esp32_gateway`
5. Build/flash: `04_BUILD_FLASH.md`
6. Validacion E2E: `05_VALIDACION_E2E.md`
7. Puerto USB PC minimo: `06_USB_PC_PORT_MIN.md`
8. Paquete release KiCad: `../../Hardware/release/README.md`

## Arbol principal
- `../../Hardware/esp32_eth_dect_nrf9151_gateway_min.kicad_sch`
- `../../Hardware/power_poe.kicad_sch`
- `../../Hardware/esp32_eth.kicad_sch`
- `../../Hardware/nrf9151_iface.kicad_sch`
- `../../Hardware/usb_pc_iface.kicad_sch`
- `../../Hardware/esp32_eth_dect_nrf9151_gateway_min.kicad_pcb`
- `../../Hardware/firmware/esp32_gateway`
