import argparse
import time
from typing import List

import numpy as np
import matplotlib.pyplot as plt
import serial.tools.list_ports

from generate_gray import generate_gray
from luxmeter import luxmeter

def fit_power_law(x: np.ndarray, y: np.ndarray, with_offset: bool = False):
    """
    Ajusta una ley de potencia usando scipy.curve_fit.

    Parametros:
      x, y: arrays de la misma forma
      with_offset: si True ajusta y = a * x^b + d, si False ajusta y = a * x^b

    Devuelve:
      (a, b, d) siempre; d será 0.0 si with_offset=False.

    Lanza ImportError si scipy no está disponible o ValueError para entradas inválidas.
    """
    x = np.asarray(x, dtype=float).copy()
    y = np.asarray(y, dtype=float).copy()
    if x.size == 0 or y.size == 0 or x.shape != y.shape:
        raise ValueError("x e y deben tener la misma forma no vacía")

    # Small positive value to avoid 0**gamma problems
    eps = 1e-8
    x_fit = x.copy()
    x_fit[x_fit <= 0.0] = eps

    try:
        from scipy.optimize import curve_fit
    except Exception as e:
        raise ImportError("scipy required for fit_power_law. Install it (pip install scipy).") from e

    # Basic checks: enough variation in data
    if not np.isfinite(x_fit).all() or not np.isfinite(y).all():
        raise ValueError("x or y contain non-finite values")

    if with_offset:
        def model(xv, a, b, d):
            xv_safe = np.where(xv <= 0.0, eps, xv)
            return a * np.power(xv_safe, b) + d

        # Initial guesses
        a0 = max(np.max(y) - np.min(y), 1e-6)
        b0 = 1.0
        d0 = float(np.min(y))
        p0 = (a0, b0, d0)

        # Fit
        popt, _ = curve_fit(model, x_fit, y, p0=p0, maxfev=20000)
        a, b, d = map(float, popt)
        return a, b, d
    else:
        def model(xv, a, b):
            xv_safe = np.where(xv <= 0.0, eps, xv)
            return a * np.power(xv_safe, b)

        a0 = max(np.max(y), 1e-6)
        b0 = 1.0
        p0 = (a0, b0)

        popt, _ = curve_fit(model, x_fit, y, p0=p0, maxfev=20000)
        a, b = map(float, popt)
        return a, b, 0.0

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

	# Ajuste de potencia: y = a * x^b [+ d] ; Gamma = b
	a, gamma, offset = fit_power_law(input_levels, lux_norm)

	# Graficar datos y ajuste
	plt.figure()
	plt.scatter(input_levels, lux_norm, label="Datos", color="C0")
	xx = np.linspace(0.0, 1.0, 200)
	yy = np.zeros_like(xx)
	mask = xx > 0.0 # Evitar evaluar 0**gamma (puede dar divide-by-zero si gamma<0)
	yy[mask] = a * np.power(xx[mask], gamma) + offset
	plt.plot(xx, yy, label=f"Ajuste: y = {a:.3g}·x^{gamma:.2f}", color="C1")
	plt.ylim(-0.05, 1.05)
	plt.xlim(-0.02, 1.02)
	plt.xlabel("Nivel de entrada")
	plt.ylabel("Luminancia normalizada")
	plt.title("Curva de Gamma")
	plt.text(0.2, 0.6, f"Gamma = {gamma:.2f}\nOffset = {offset:.3g}", color="red", fontsize=12, fontweight="bold")
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
