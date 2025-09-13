import argparse
import time
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import serial.tools.list_ports

from generate_gray import generate_gray
from luxmeter import luxmeter

def fit_power_law(x: np.ndarray, y: np.ndarray):
	"""
	Ajusta y = a * x^b usando regresión lineal en espacio log-log.
	Devuelve (a, b). Maneja x/y no positivas con pequeños epsilons.
	"""
	x_fit = x.astype(float).copy()
	y_fit = y.astype(float).copy()
	# Evitar ceros para el log; similar al fix en MATLAB para x(1)
	x_fit[x_fit <= 0] = 1e-5
	y_fit[y_fit <= 0] = 1e-8
	slope, intercept = np.polyfit(np.log(x_fit), np.log(y_fit), 1)
	a = float(np.exp(intercept))
	b = float(slope)
	return a, b

def list_ports():
	"""Lista los puertos serie disponibles con descripciones breves."""
	ports = serial.tools.list_ports.comports()
	if not ports:
		print("No se encontraron puertos serie disponibles.")
		return
	print("Puertos serie disponibles:")
	for port in ports:
		desc = f"{port.device}: {port.description} (Fabricante: {port.manufacturer or 'Desconocido'})"
		print(f"  - {desc}")

def run_test(Nmeas: int, port: str = None, fullscreen: bool = False):
	input_levels = np.linspace(0.0, 1.0, 11)  # 0:0.1:1
	luxmed: List[float] = []

	total = len(input_levels)
	print("Starting...")
	for i, level in enumerate(input_levels, start=1):
		# Cerrar ventanas anteriores y mostrar patch gris
		plt.close("all")
		generate_gray(level, fullscreen=fullscreen)  # Pass fullscreen argument
		time.sleep(1.0)  # espera a que la ventana esté visible

		# Medir N veces y promediar
		vals = []
		for _ in range(Nmeas):
			vals.append(luxmeter(port=port))
		avg = float(np.mean(vals))
		luxmed.append(avg)

		perc = int(i * 100 / total)
		print(f"{perc}% Done")
	
	plt.close("all") # Cerrar la ventana de gris al final

	# Normalizar y ajustar
	luxmed = np.array(luxmed, dtype=float)
	if luxmed.max() <= 0:
		raise ValueError("Valores de lux no positivos; no se puede normalizar.")
	lux_norm = luxmed / luxmed.max()

	# Ajuste de potencia: y = a * x^b ; Gamma = b
	a, gamma = fit_power_law(input_levels, lux_norm)

	# Graficar datos y ajuste
	plt.figure()
	plt.scatter(input_levels, lux_norm, label="Datos", color="C0")
	xx = np.linspace(0.0, 1.0, 200)
	yy = np.zeros_like(xx)
	mask = xx > 0.0 # Evitar evaluar 0**gamma (puede dar divide-by-zero si gamma<0)
	yy[mask] = a * np.power(xx[mask], gamma)
	plt.plot(xx, yy, label=f"Ajuste: y = {a:.3g}·x^{gamma:.2f}", color="C1")
	plt.ylim(-0.05, 1.05)
	plt.xlim(-0.02, 1.02)
	plt.xlabel("Nivel de entrada")
	plt.ylabel("Luminancia normalizada")
	plt.title("Curva de Gamma")
	plt.text(0.2, 0.6, f"Gamma = {gamma:.2f}", color="red", fontsize=14, fontweight="bold")
	plt.legend()
	plt.show()

def main():
	parser = argparse.ArgumentParser(
		description="Medición de gamma con fotómetro por puerto serie. "
					"Genera niveles de gris en pantalla, mide luminancia y ajusta una curva de potencia para calcular gamma.",
		epilog="Ejemplo: python luxtest.py --port COM3 --nmeas 5 --fullscreen"
	)
	parser.add_argument("--nmeas", type=int, default=2, help="Número de mediciones por nivel (default: 2)")
	parser.add_argument("--port", type=str, default=None, help="Puerto serie (p.ej. COM3 o /dev/ttyUSB0). Si no se especifica, se intenta detectar automáticamente.")
	parser.add_argument("--list-ports", action="store_true", help="Lista los puertos serie disponibles y sale")
	parser.add_argument("--fullscreen", action="store_true", help="Habilita modo fullscreen para las ventanas de gris")
	args = parser.parse_args()
	
	if args.list_ports:
		list_ports()
		return
	
	run_test(Nmeas=args.nmeas, port=args.port, fullscreen=args.fullscreen)

if __name__ == "__main__":
	main()
