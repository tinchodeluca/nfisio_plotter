# nFisio Plotter

![Estado del Proyecto](https://img.shields.io/badge/Estado-En%20Desarrollo-yellow.svg)

**nFisio Plotter** es una aplicación de escritorio desarrollada en Qt para visualizar y analizar señales en tiempo real recibidas a través de un puerto serial. Está diseñada para mostrar dos canales de datos (ECG y PPG) en gráficos interactivos, con opciones de configuración para personalizar la conexión, la visualización y la exportación de datos.

## Descripción

Esta aplicación permite:
- Conectar a un dispositivo mediante un puerto serial para recibir datos en tiempo real.
- Visualizar dos canales de señales (ECG y PPG) en gráficos separados usando `QCustomPlot`.
- Configurar parámetros como el puerto serial, baud rate, inversión de canales, autorange y rangos manuales.
- Enviar comandos predefinidos al dispositivo (por ejemplo, `MODE1`, `MODE4`, `DBG01`).
- Grabar los datos recibidos en un archivo CSV con cabeceras personalizables.
- Interactuar con los gráficos (zoom y desplazamiento).

La interfaz está diseñada para ser intuitiva, con botones para conectar/desconectar, reproducir/pausar, grabar datos, enviar comandos y configurar opciones.

## Capturas de Pantalla

*(Pendiente: Agregar capturas de pantalla de la interfaz una vez que esté finalizada.)*

## Requisitos

- **Qt**: Versión 6.9 o superior (módulos requeridos: `core`, `gui`, `widgets`, `serialport`, `printsupport`, `opengl`, `openglwidgets`).
- **Sistema Operativo**: Windows, Linux o macOS.
- **Hardware**: Dispositivo que envíe datos por puerto serial con formato `EFBEADDE` (header) + 2 valores de 32 bits.

## Instalación y Uso

### 1. Clonar el Repositorio

```bash
git clone https://github.com/tinchodeluca/nfisio_plotter.git
cd nfisio_plotter
```

### 2. Abrir el Proyecto en Qt Creator

Abre el archivo serial-plotter.pro en Qt Creator.

### 3. Compilar y Ejecutar

1. Compila el proyecto (`Ctrl + B` en Qt Creator).
2. Ejecuta la aplicación (`Ctrl + R`).

## Uso Básico

| Acción | Descripción |
|--------|-------------|
| **Conectar/Desconectar** | Botón con icono de enlace (configura puerto y baud rate primero). |
| **Reproducir/Pausar** | Botón de reproducción para controlar la visualización. |
| **Grabar Datos** | Botón de grabación para exportar a CSV (cabeceras personalizables). |
| **Enviar Comandos** | Diálogo con comandos predefinidos (`MODE1`, `DBG01`, etc.). |
| **Configuración** | Ajusta puerto serial, rangos de canales, y formato de exportación. |

## Funcionalidades Actuales

### Visualización de Señales
* Gráficos duales con `QCustomPlot` (ECG y PPG).
* Ventana de tiempo desplazable (10 segundos).
* Zoom y desplazamiento interactivo.

### Conexión Serial
* Configuración de puerto y baud rate (ej: COM9 @ 230400 bauds).
* Diálogo de monitoreo en tiempo real.

### Exportación de Datos
* Grabación en CSV con cabeceras personalizadas.
* Opción para pausar/reanudar la grabación.

## Formato de Datos Esperado

Cada paquete serial debe tener este formato:

```
EF BE AD DE   [4 bytes: Header]
XX XX XX XX   [4 bytes: Canal 1 (ECG)]
YY YY YY YY   [4 bytes: Canal 2 (PPG)]
```

*(12 bytes total por paquete)*

## Pendientes (TODO)

### Mejoras de Conexión

* Configuración avanzada (paridad, bits de parada).

### Procesamiento de Señales
* Filtros digitales (pasa-bajos, eliminar ruido).
* Ajuste dinámico de ventana de tiempo.

## Licencia

Este proyecto está bajo la Licencia MIT. Consulta el archivo LICENSE para más detalles.

## Autor

Martín De Luca  
GitHub: https://github.com/tinchodeluca

