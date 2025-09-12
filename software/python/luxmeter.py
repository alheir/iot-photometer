import os
import time
from typing import Optional

import serial
import serial.tools.list_ports

DEFAULT_BAUD = 115200
DEFAULT_TIMEOUT = 4.0

def _auto_detect_port() -> Optional[str]:
	# Intenta encontrar un puerto típico
	candidates = []
	for p in serial.tools.list_ports.comports():
		dev = p.device
		if dev:
			candidates.append(dev)
	# Prioriza Windows COM*, luego /dev/ttyUSB*, luego cualquiera
	win = [p for p in candidates if p.upper().startswith("COM")]
	linux = [p for p in candidates if "/ttyUSB" in p or "/ttyACM" in p]
	if win:
		return win[0]
	if linux:
		return linux[0]
	return candidates[0] if candidates else None

def luxmeter(port: Optional[str] = None, baudrate: int = DEFAULT_BAUD, timeout: float = DEFAULT_TIMEOUT) -> float:
	"""
	Lee una línea con formato 'ILLUMINANCE=###...' desde el puerto serie y devuelve el valor en lux.
	"""
	port = port or os.environ.get("LUXMETER_PORT") or _auto_detect_port() or "/dev/ttyUSB0"
	ser = serial.Serial(port=port, baudrate=baudrate, bytesize=serial.EIGHTBITS,
	                    parity=serial.PARITY_NONE, stopbits=serial.STOPBITS_ONE,
	                    timeout=timeout)
	try:
		ser.reset_input_buffer()
		ser.reset_output_buffer()

		start = time.time()
		deadline = start + max(timeout, 1.0) + 6.0  # margen similar al MATLAB
		while time.time() < deadline:
			# El equipo termina con CR (13). Leemos hasta CR.
			data = ser.read_until(expected=b"\r", size=256)
			if not data:
				continue
			text = data.decode(errors="ignore").strip()
			if "ILLUMINANCE=" in text:
				val_str = text.split("ILLUMINANCE=", 1)[1].strip()
				# recorta cualquier sufijo extraño
				val_str = val_str.split()[0].strip().strip("\r\n")
				return float(val_str)
		raise TimeoutError(f"No se recibió 'ILLUMINANCE=' desde {port} en {timeout}s.")
	finally:
		try:
			ser.close()
		except Exception:
			pass
