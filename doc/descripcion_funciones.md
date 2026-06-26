# Descripción de funciones y variables globales

## app_main.c

### Variables globales

* **TAG:** Cadena utilizada para identificar los mensajes mostrados por el log.

### Funciones

#### init_nvs()

Inicializa la memoria NVS del ESP32. Si detecta algún problema durante la inicialización, la borra y la vuelve a crear.

#### app_main()

Función principal del firmware. Inicializa todos los módulos del sistema, realiza una comprobación inicial de la comunicación DECT y pone en marcha el bridge, el watchdog y el sistema de diagnóstico.

---

# gw_bridge.c

### Variables globales

* **TAG:** Identificador utilizado en los mensajes de log.
* **s_dect_rx_q:** Cola donde se almacenan los mensajes recibidos desde DECT.
* **s_peer_mutex:** Mutex para proteger el acceso a la dirección del cliente UDP.
* **s_have_last_peer:** Indica si existe un cliente UDP registrado.
* **s_last_peer:** Último cliente UDP que ha enviado datos.
* **s_use_fixed_peer:** Indica si se utiliza una dirección IP fija.
* **s_fixed_peer:** Dirección IP configurada manualmente.
* **s_stats:** Contadores de estadísticas del bridge.
* **s_stats_lock:** Protección para acceder a las estadísticas.

### Funciones

#### stats_inc()

Incrementa uno de los contadores de estadísticas.

#### stats_copy()

Copia las estadísticas actuales.

#### dect_rx_cb()

Recibe mensajes desde DECT y los almacena para enviarlos posteriormente por Ethernet.

#### udp_rx_task()

Escucha paquetes UDP y los reenvía al módulo DECT.

#### resolve_peer()

Obtiene la dirección IP del destinatario al que enviar los datos.

#### udp_tx_task()

Envía por UDP los mensajes recibidos desde DECT.

#### load_fixed_peer()

Carga la dirección IP fija configurada para el bridge.

#### gw_bridge_start()

Inicializa el bridge y crea las tareas de transmisión y recepción.

#### gw_bridge_get_stats()

Devuelve las estadísticas del bridge.

#### gw_bridge_reset_stats()

Reinicia todos los contadores.

#### gw_bridge_set_runtime_peer()

Permite cambiar el destino UDP durante la ejecución.

#### gw_bridge_inject_dect_json()

Introduce manualmente un mensaje DECT para realizar pruebas.

---

# gw_dect.c

### Variables globales

* **s_rx_cb:** Callback utilizado para entregar los datos recibidos.
* **s_rx_ctx:** Contexto asociado al callback.
* **s_at_ok_sem:** Semáforo utilizado durante el healthcheck.
* **s_rx_task:** Tarea encargada de la recepción de datos.
* **s_spi:** Dispositivo SPI utilizado cuando esa interfaz está habilitada.
* **s_spi_bus_init:** Indica si el bus SPI ya ha sido inicializado.

### Funciones

#### publish_payload_line()

Procesa las líneas recibidas desde el módulo DECT y las entrega al callback correspondiente.

#### iface_write()

Envía datos mediante la interfaz de comunicación configurada.

#### uart_rx_task()

Lee continuamente los datos recibidos por UART.

#### spi_xfer()

Realiza una transferencia SPI.

#### spi_rx_task()

Lee datos del módulo DECT utilizando SPI.

#### iface_init()

Inicializa la interfaz física de comunicación.

#### send_line()

Envía una línea de texto al módulo DECT.

#### gw_dect_init()

Inicializa la comunicación con el módulo DECT.

#### gw_dect_restart()

Reinicia la interfaz DECT.

#### gw_dect_set_rx_callback()

Registra el callback encargado de recibir los mensajes.

#### gw_dect_send_json()

Envía un mensaje JSON al módulo DECT.

#### gw_dect_healthcheck()

Comprueba que la comunicación con DECT sigue funcionando correctamente.

#### gw_dect_test_inject_rx_line()

Simula la recepción de una línea para realizar pruebas.

---

# gw_diag.c

### Variables globales

* **s_eth_rand_seq:** Contador de mensajes Ethernet generados.
* **s_dect_rand_seq:** Contador de mensajes DECT generados.

### Funciones

#### print_line()

Imprime una línea en la consola de depuración.

#### print_help()

Muestra los comandos disponibles.

#### parse_positive_int()

Convierte una cadena a un número entero válido.

#### make_random_json()

Genera un mensaje JSON con datos aleatorios.

#### send_udp_to_gateway()

Envía un paquete UDP al propio gateway.

#### print_bridge_stats()

Muestra las estadísticas del bridge.

#### cmd_status()

Muestra el estado general del gateway.

#### cmd_poe()

Comprueba el estado de la alimentación PoE.

#### cmd_at()

Ejecuta un healthcheck del módulo DECT.

#### cmd_set_peer()

Permite cambiar el destino UDP.

#### run_eth_random_burst()

Genera tráfico Ethernet de prueba.

#### run_dect_random_burst()

Genera tráfico DECT de prueba.

#### cmd_rand_eth()

Ejecuta el envío de tráfico Ethernet aleatorio.

#### cmd_rand_dect()

Ejecuta el envío de tráfico DECT aleatorio.

#### cmd_fw_update()

Muestra información para actualizar el firmware.

#### execute_command()

Procesa los comandos introducidos por el usuario.

#### usb_debug_task()

Implementa la consola de depuración por USB.

#### random_example_task()

Genera tráfico de prueba de forma periódica.

#### gw_diag_start()

Inicializa el sistema de diagnóstico.

---

# gw_eth.c

### Variables globales

* **s_eth_handle:** Manejador del controlador Ethernet.
* **s_eth_netif:** Interfaz de red Ethernet.
* **s_eth_glue:** Unión entre el driver Ethernet y la pila TCP/IP.
* **s_link_up:** Estado del enlace Ethernet.
* **s_have_ip:** Indica si existe una dirección IP asignada.
* **s_link_down_since_ms:** Tiempo desde que el enlace dejó de estar activo.

### Funciones

#### now_ms()

Obtiene el tiempo actual en milisegundos.

#### on_eth_event()

Gestiona los eventos del controlador Ethernet.

#### on_ip_event()

Gestiona la obtención o pérdida de dirección IP.

#### install_eth_driver()

Inicializa el controlador Ethernet.

#### gw_eth_init()

Inicializa la interfaz Ethernet.

#### gw_eth_restart()

Reinicia el controlador Ethernet.

#### gw_eth_link_up()

Indica si el enlace Ethernet está activo.

#### gw_eth_has_ip()

Indica si existe una dirección IP asignada.

#### gw_eth_link_down_ms()

Devuelve cuánto tiempo lleva el enlace desconectado.

#### gw_eth_get_ipv4()

Obtiene la dirección IPv4 actual.

---

# gw_json.c

### Funciones

#### skip_ws()

Ignora los espacios en blanco de una cadena.

#### gw_json_is_valid()

Comprueba si un bloque de datos tiene formato JSON.

#### hex_wrap()

Convierte datos binarios en una representación hexadecimal dentro de un objeto JSON.

#### gw_json_ensure()

Garantiza que la salida tenga formato JSON, convirtiendo los datos cuando es necesario.

---

# gw_watchdog.c

### Funciones

#### supervisor_task()

Supervisa continuamente el estado del gateway y reinicia Ethernet o DECT cuando detecta fallos.

#### gw_watchdog_start()

Crea la tarea encargada de supervisar el funcionamiento del sistema.

