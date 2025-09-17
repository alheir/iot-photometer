# Luxtest — Instrucciones de uso

Este README explica cómo preparar un entorno Python, instalar dependencias y ejecutar `luxtest.py` (medición de gamma con fotómetro conectado por puerto serie). El script genera parches grises en pantalla (módulo `generate_gray`) y lee un fotómetro vía serie (módulo `luxmeter`).

Resumen rápido de argumentos (ver `python luxtest.py -h`):
- `--nmeas N`     Número de mediciones por nivel (default: 2)
- `--port PORT`   Puerto serie (p. ej. COM3 o /dev/ttyUSB0). Si no se indica, el script intenta detectar uno.
- `--list-ports`  Lista puertos serie disponibles y sale.
- `--fullscreen`  Muestra los parches grises en fullscreen.

Requisitos básicos
- Python 3.7+ instalado (usa `python3` o `py -3` en Windows).
- Acceso al puerto serie donde está conectado el dispositivo.
- Archivo `requirements.txt` en este directorio con las dependencias del proyecto.

1) Crear y activar virtualenv (venv)

- macOS / Linux (bash/zsh):
  - Crear: `python3 -m venv venv`
  - Activar: `source venv/bin/activate`

- Windows (PowerShell):
  - Crear: `py -3 -m venv venv`
  - Activar: `.\venv\Scripts\Activate.ps1`
  - Si PowerShell bloquea la activación, ejecutar en PowerShell como administrador:
    `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`

- Windows (cmd):
  - Activar: `.\venv\Scripts\activate.bat`

2) Instalación de dependencias


```
scipy
numpy
matplotlib
pyserial
```

- Actualizar pip:
  - `python -m pip install --upgrade pip`

- Instalar desde  `requirements.txt`:
  - `pip install -r requirements.txt`


3) Ejecutar luxtest

- Comandos genéricos (ajusta nombre/ruta del script según el repo):
  - `python luxtest.py [--nmeas N] [--port PORT] [--list-ports] [--fullscreen]`

- Ejemplos:
  - macOS / Linux (puerto USB típico):  
    `python luxtest.py --port /dev/ttyUSB0 `
  - macOS (dispositivo Arduino/Micro):  
    `python luxtest.py --port /dev/ttyACM0`
  - Windows (COM3):  
    `python luxtest.py --port COM3`

4) Notas de permisos y troubleshooting

- macOS / Linux:
  - Si no tienes acceso al puerto serie: añade tu usuario al grupo `dialout` / `uucp` según la distro, o ejecuta con `sudo` (no recomendado).
    - Ejemplo (Linux): `sudo usermod -a -G dialout $USER` (logout/login necesario).
  - En macOS, confirma el nombre del dispositivo con `ls /dev/tty.*` o `ls /dev/cu.*`.

- Windows:
  - Confirma el número de COM en el Administrador de dispositivos.
  - Asegúrate de que ningún otro programa (IDE) esté usando el puerto serie.

Fin.
