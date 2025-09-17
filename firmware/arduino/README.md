# Firmware / Arduino — Overview

- `dummy_tsl2591/`
  Emula un TSL2591 y envía por serie líneas "ILLUMINANCE=<valor>\r" (uso: pruebas sin hardware). Baud: 115200.
- `tsl2591/`
  Lee sensor Adafruit TSL2591 y emite lecturas por serial (función sendLuminosity imprime "ILLUMINANCE=<valor>").
- `tsl2591_web/`
  Lee el sensor y publica a la webapp (Firebase Realtime DB) vía HTTPS.
- `simple_logger/`
  Sketch de prueba que envía valores aleatorios a Firebase (útil para verificar conexión HTTPS y la DB).

Notas:
- Formato serial: los lectores Python/MATLAB esperan líneas como `ILLUMINANCE=<valor>` terminadas en CR.
- Baud por defecto: 115200.
- Si el script Python no detecta puerto, usa --list-ports o comprueba el Administrador de dispositivos / ls /dev.