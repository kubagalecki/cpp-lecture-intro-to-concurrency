import numpy as np
import matplotlib.pyplot as plt

plt.figure(figsize=(8, 5))
workers = np.linspace(1, 64, 50);
par = (1., .99, .95, .9, .8)
for p in par:
    spdup = 1. / ((1 - p) + p / workers)
    plt.plot(workers, spdup, label="Available parallelism: " + str(p*100) + "%")

plt.xlabel("Workers")
plt.ylabel("Speedup")
plt.grid()
plt.legend()
plt.show()
