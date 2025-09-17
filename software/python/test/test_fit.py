import matplotlib.pyplot as plt
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))  # .../software/python

import numpy as np
from luxtest import fit_power_law

data_points = [
    (0.0, 0.03),
    (0.1, 0.04),
    (0.2, 0.06),
    (0.3, 0.08),
    (0.4, 0.13),
    (0.5, 0.21),
    (0.6, 0.28),
    (0.7, 0.42),
    (0.8, 0.58),
    (0.9, 0.78),
    (1.0, 1.00)
]

x = np.array([p[0] for p in data_points], dtype=float)
y = np.array([p[1] for p in data_points], dtype=float)

# fit_power_law ahora devuelve (a, gamma, offset)
a, gamma, offset = fit_power_law(x, y, with_offset=False)

x_safe = np.where(x > 0.0, x, 1e-6)
y_pred = a * np.power(x_safe, gamma) + offset
mse = np.mean((y - y_pred) ** 2)

print(f"a = {a:.6g}")
print(f"gamma = {gamma:.6g}")
print(f"offset = {offset:.6g}")
print(f"MSE = {mse:.6g}")

plt.figure(figsize=(6,4))
plt.scatter(x, y, label="Data", color="C0")
xx = np.linspace(0.0, 1.0, 400)
xx_safe = np.where(xx > 0.0, xx, 1e-6)
yy = a * np.power(xx_safe, gamma) + offset
plt.plot(xx, yy, label=f"Fit: y = {a:.3g}·x^{gamma:.3g} + {offset:.3g}", color="C1")
plt.xlabel("Input level")
plt.ylabel("Luminance")
plt.title(f"Power-law fit (MSE={mse:.3e})")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.show()