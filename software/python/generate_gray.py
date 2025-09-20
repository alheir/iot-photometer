import matplotlib
matplotlib.use("TkAgg")  # Use TkAgg backend for better window control

import numpy as np
import matplotlib.pyplot as plt
import tkinter as tk  # Added for screen dimensions

def generate_gray(level: float, N: int = 600, fullscreen: bool = False):
	img = np.full((N, N), float(level), dtype=np.float32)
	plt.figure()
	plt.imshow(img, cmap="gray", vmin=0.0, vmax=1.0)
	plt.axis("off")
	plt.title(f"Gray level = {level:.2f}")
	plt.show(block=False)
	plt.pause(0.1)  # Increased pause to allow window to stabilize
	
	root = tk.Tk()
	screen_width = root.winfo_screenwidth()
	screen_height = root.winfo_screenheight()
	root.destroy()
	
	fig = plt.gcf()
	manager = fig.canvas.manager
	window = manager.window
	
	if fullscreen:
		window.attributes('-fullscreen', True)
		window.update()  # Force refresh to apply fullscreen
	else:
		# Approximate centering based on image size N
		x = (screen_width - N) // 2
		y = (screen_height - N) // 2
		window.geometry(f"{N}x{N}+{x}+{y}")
		window.update()  # Force refresh to apply geometry
	
	return plt.gcf()
