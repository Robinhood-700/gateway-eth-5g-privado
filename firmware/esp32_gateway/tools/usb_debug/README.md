# USB Debug Tools

## 1) Flash/monitor por USB
```bash
./tools/usb_debug/flash_usb.sh /dev/ttyUSB0 460800
```

## 2) Enviar comando a la consola USB (`gwdbg`)
```bash
python3 tools/usb_debug/gateway_usb_debug.py cmd \
  --serial /dev/ttyUSB0 \
  status
```

## 3) Test E2E Ethernet <-> DECT
```bash
python3 tools/usb_debug/gateway_usb_debug.py e2e \
  --serial /dev/ttyUSB0 \
  --gateway-ip 192.168.1.80 \
  --listen-port 5000 \
  --remote-port 5000 \
  --count 20
```

## Dependencias host
- Python 3.9+
- `pyserial`

Instalacion:
```bash
pip install pyserial
```
