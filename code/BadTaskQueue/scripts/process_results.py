import sys
import numpy as np
import json
import matplotlib.pyplot as plt

def parseBench(text):
    spl = text.split('/')
    return spl[0], int(spl[1]), int(spl[2])

with open(sys.argv[1], "r") as read_file:
    results = json.load(read_file)

parsed = {}
for bench in results["benchmarks"]:
    name, str_len, num_str = parseBench(bench["name"])
    hash_rate = float(bench["Hash Rate"]) / 1.e+9
    if name in parsed:
        parsed[name].append((str_len, num_str, hash_rate))
    else:
        parsed[name] = [(str_len, num_str, hash_rate)]

def makeGrid(entry):
    sl = set()
    ns = set()
    for l, n, _ in entry:
        sl.add(l);
        ns.add(n);
    sl = sorted(sl)
    ns = sorted(ns)
    grid = np.zeros((len(sl), len(ns)))
    for l, n, rate in entry:
        grid[sl.index(l), ns.index(n)] = rate
    return sl, ns, grid

fig = plt.figure(figsize=(16, 9))
for i, name in enumerate(parsed.keys()):
    xt, yt, grid = makeGrid(parsed[name])
    plt.subplot(2, 3, i + 1)
    plt.pcolormesh(grid, cmap="turbo")
    plt.title(name.replace("_", " ") + " [GB/s]")
    plt.xlabel("Number of strings")
    plt.ylabel("String length")
    plt.xticks(np.arange(.5, len(xt) + .5), xt)
    plt.yticks(np.arange(.5, len(yt) + .5), yt)
    plt.tick_params(axis=u'both', which=u'both', length=0)
    plt.colorbar()
plt.tight_layout()
plt.show()
