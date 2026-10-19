from pathlib import Path
import sys
import numpy as np
from PIL import Image


def components(mask, with_runs=False):
    parent, runs = [], []
    previous = []

    def root(n):
        while parent[n] != n:
            parent[n] = parent[parent[n]]
            n = parent[n]
        return n

    for y, row in enumerate(mask):
        edges = np.diff(np.pad(row.astype(np.int8), (1, 1)))
        starts = np.flatnonzero(edges == 1)
        ends = np.flatnonzero(edges == -1)
        current = []
        k = 0
        for x0, x1 in zip(starts, ends):
            while k < len(previous) and previous[k][1] < x0 - 1:
                k += 1
            touching = []
            j = k
            while j < len(previous) and previous[j][0] <= x1 + 1:
                touching.append(previous[j][2])
                j += 1
            label = len(parent)
            parent.append(label)
            for old in touching:
                a, b = root(label), root(old)
                if a != b:
                    parent[a] = b
            current.append((int(x0), int(x1), label))
            runs.append((int(x0), int(x1), y, label))
        previous = current

    result = {}
    for x0, x1, y, label in runs:
        r = root(label)
        if r not in result:
            result[r] = [x0, y, x1, y + 1, 0, []]
        item = result[r]
        item[0] = min(item[0], x0)
        item[1] = min(item[1], y)
        item[2] = max(item[2], x1)
        item[3] = max(item[3], y + 1)
        item[4] += x1 - x0
        if with_runs:
            item[5].append((x0, x1, y))
    return sorted(result.values(), key=lambda x: x[4], reverse=True)


if __name__ == "__main__":
    for filename in sys.argv[1:]:
        image = Image.open(Path("Sprites") / filename).convert("RGBA")
        alpha = np.asarray(image.getchannel("A"))
        found = components(alpha >= 180)
        print(filename, "components", len(found), "large", sum(c[4] > 500 for c in found))
        print(" top:", found[:20])
