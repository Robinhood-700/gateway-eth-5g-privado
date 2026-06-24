# Gateway privado Ethernet-5G
Este proyecto contiene el diseño del PCB para la implementación de un gateway privado ethernet-dect, además del firmware correspondiente y un firmware simulado. Además contiene en la carpeta "resultados" la salida del proceso de build, flash y la ejecución del programa.
## Build
A continuación se describe el procedimiento para hacer el build del proyecto.
1. Cambiar de directorio a la carpeta padre del firmware:
```
// (desde la carpeta padre del proyecto)
cd firmware/esp32_gateway 
```
2. Activar el entorno ESP-IDF:
```
// En mi caso instalado en /opt. Cambia al directorio correspondiente $ESP_DIR
source /opt/esp/esp-idf/export.sh 
```
3. Realizar el comando build:
```
idf.py set-target esp32
idf.py reconfigure
idf.py fullclean build
```
### Para guardar el build en un archivo
Para guardar la salida del build en un fichero de texto, simplemente hice un pipe de la salida a un nuevo archivo .txt:
`idf.py fullclean build >> resultado_build.txt`
## Flash
1. Repetir pasos 1 y 2 del proceso build si no se ha ejecutado
```
// (desde la carpeta padre del proyecto)
cd firmware/esp32_gateway 
// En mi caso instalado en /opt. Cambia al directorio correspondiente $ESP_DIR
source /opt/esp/esp-idf/export.sh 
idf.py set-target esp32
idf.py reconfigure
```
2. Realizar el flash
```
// En el caso de mi portatil y la placa de desarrollo ESP32-Ethernet-Kit v1.2, el puerto es el
// /dev/ttyUSB1 y según la documentación de espressif, 115200 es el valor de baudios adecuado
idf.py -p /dev/ttyUSB1 -b 115200 flash monitor
// Este comando concatena el comando monitor con el flash para poder visualizar inmediatamente los resultados en pantalla.
```
### Para guardar el flash en un archivo
Igual que el build, simplemente hace falta un pipe en una terminal de bash:
`idf.py -p /dev/ttyUSB1 -b 115200 flash >> resultado_flash.txt`
### Para guardar la ejecución del programa en un archivo
`idf.py -p /dev/ttyUSB1 -b 115200 monitor >> resultado_monitor.txt`

