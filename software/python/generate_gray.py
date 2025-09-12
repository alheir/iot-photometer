import numpy as np
import matplotlib.pyplot as plt

def generate_gray(level: float, N: int = 600):
	img = np.full((N, N), float(level), dtype=np.float32)
	plt.figure()
	plt.imshow(img, cmap="gray", vmin=0.0, vmax=1.0)
	plt.axis("off")
	plt.title(f"Gray level = {level:.2f}")
	plt.show(block=False)
	plt.pause(0.001)  # asegurar que se dibuje la ventana
	return plt.gcf()
