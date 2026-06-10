#!/usr/bin/env python3
"""Generate the procedural perlin-noise backgrounds added in v1.3.0
(IMAGE_BG21 .. IMAGE_BG25).

Each theme is rendered natively for every target platform:
  basalt 144x168, chalk 180x180, emery 200x228

The output matches the hand-made originals' look: a white canvas covered in
small colored glyphs/strokes whose hue, density and orientation are driven by
a 2D Perlin noise field. Runs are seeded, so re-running the script reproduces
the shipped artwork exactly.

Usage:  python3 tools/generate_backgrounds.py   (writes into resources/images/)
Needs:  pillow, numpy
"""

import colorsys
import math
import os
import random

import numpy as np
from PIL import Image, ImageDraw

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "resources", "images")

PLATFORMS = {
    "basalt": (144, 168),
    "chalk": (180, 180),
    "emery": (200, 228),
}


# ---------------------------------------------------------------------------
# Classic 2D Perlin noise (gradient noise with smoothstep interpolation).
# ---------------------------------------------------------------------------
def perlin_grid(width, height, scale, rng):
    gx = int(width / scale) + 2
    gy = int(height / scale) + 2
    angles = rng.uniform(0, 2 * math.pi, (gy, gx))
    grads = np.stack([np.cos(angles), np.sin(angles)], axis=-1)

    xs = np.arange(width) / scale
    ys = np.arange(height) / scale
    xi = xs.astype(int)
    yi = ys.astype(int)
    xf = xs - xi
    yf = ys - yi

    xf2, yf2 = np.meshgrid(xf, yf)
    xi2, yi2 = np.meshgrid(xi, yi)

    def dot_corner(ox, oy):
        g = grads[yi2 + oy, xi2 + ox]
        return g[..., 0] * (xf2 - ox) + g[..., 1] * (yf2 - oy)

    def fade(t):
        return t * t * t * (t * (t * 6 - 15) + 10)

    u, v = fade(xf2), fade(yf2)
    n00, n10 = dot_corner(0, 0), dot_corner(1, 0)
    n01, n11 = dot_corner(0, 1), dot_corner(1, 1)
    nx0 = n00 * (1 - u) + n10 * u
    nx1 = n01 * (1 - u) + n11 * u
    n = nx0 * (1 - v) + nx1 * v
    return (n - n.min()) / (n.max() - n.min())  # normalized 0..1


def hue_color(h, s=0.85, v=0.85):
    r, g, b = colorsys.hsv_to_rgb(h % 1.0, s, v)
    return (int(r * 255), int(g * 255), int(b * 255))


# ---------------------------------------------------------------------------
# Glyph primitives, in the spirit of the original hand-made art.
# ---------------------------------------------------------------------------
def glyph_arrow(d, x, y, size, color, angle):
    x2 = x + size * math.cos(angle)
    y2 = y + size * math.sin(angle)
    d.line([(x, y), (x2, y2)], fill=color, width=2)
    for side in (-1, 1):
        a = angle + math.pi + side * 0.5
        hx = x2 + size * 0.45 * math.cos(a)
        hy = y2 + size * 0.45 * math.sin(a)
        d.line([(x2, y2), (hx, hy)], fill=color, width=2)


def glyph_box(d, x, y, size, color):
    d.rectangle([x, y, x + size, y + size], outline=color, width=2)


def glyph_cross(d, x, y, size, color):
    h = size / 2
    d.line([(x - h, y), (x + h, y)], fill=color, width=2)
    d.line([(x, y - h), (x, y + h)], fill=color, width=2)


def glyph_zig(d, x, y, size, color, angle):
    pts = [(x, y)]
    a = angle
    for _ in range(3):
        x += size * 0.6 * math.cos(a)
        y += size * 0.6 * math.sin(a)
        pts.append((x, y))
        a += math.pi / 2 * random.choice((-1, 1))
    d.line(pts, fill=color, width=2)


# ---------------------------------------------------------------------------
# The five new themes. Each fills a white canvas with noise-driven glyphs.
# ---------------------------------------------------------------------------
def theme_rainbow_arrows(d, w, h, noise, rng):
    """BG21: rainbow arrows whose hue and direction follow the noise field."""
    step = 13
    for gy in range(4, h, step):
        for gx in range(4, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.18:
                continue
            jx, jy = gx + rng.uniform(-3, 3), gy + rng.uniform(-3, 3)
            glyph_arrow(d, jx, jy, 9, hue_color(n), n * 4 * math.pi)


def theme_teal_boxes(d, w, h, noise, rng):
    """BG22: nested teal/blue box outlines, denser where the noise is high."""
    step = 11
    for gy in range(2, h, step):
        for gx in range(2, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.30:
                continue
            c = hue_color(0.45 + n * 0.25, 0.9, 0.55 + n * 0.4)
            size = 4 + n * 8
            glyph_box(d, gx + rng.uniform(-2, 2), gy + rng.uniform(-2, 2), size, c)
            if n > 0.72:
                glyph_box(d, gx + 3, gy + 3, size - 5, c)


def theme_warm_flow(d, w, h, noise, rng):
    """BG23: warm flow-field strokes following the noise gradient."""
    for _ in range(int(w * h / 55)):
        x, y = rng.uniform(0, w), rng.uniform(0, h)
        n = noise[int(min(y, h - 1)), int(min(x, w - 1))]
        if n < 0.15:
            continue
        c = hue_color(0.02 + n * 0.13, 0.9, 0.95)  # red -> orange -> yellow
        a = n * 6 * math.pi
        pts = [(x, y)]
        for _ in range(3):
            x += 5 * math.cos(a)
            y += 5 * math.sin(a)
            nn = noise[int(min(max(y, 0), h - 1)), int(min(max(x, 0), w - 1))]
            a = nn * 6 * math.pi
            pts.append((x, y))
        d.line(pts, fill=c, width=2)


def theme_contour_bands(d, w, h, noise, rng):
    """BG24: thin multicolor contour lines of the noise field (marble)."""
    levels = 14
    px = (noise * levels).astype(int)
    for y in range(h - 1):
        for x in range(w - 1):
            if px[y, x] != px[y, x + 1] or px[y, x] != px[y + 1, x]:
                d.point((x, y), fill=hue_color(px[y, x] / levels))


def theme_pastel_stitch(d, w, h, noise, rng):
    """BG25: pastel crosses + zigzag stitches on a loose grid."""
    step = 12
    for gy in range(4, h, step):
        for gx in range(4, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.22:
                continue
            c = hue_color(n, 0.55, 0.95)
            if n > 0.6:
                glyph_zig(d, gx, gy, 9, c, n * 4 * math.pi)
            else:
                glyph_cross(d, gx + rng.uniform(-2, 2), gy + rng.uniform(-2, 2), 7, c)


THEMES = [
    ("perlin-gen-0001", theme_rainbow_arrows, 30, 2101),
    ("perlin-gen-0002", theme_teal_boxes, 38, 2202),
    ("perlin-gen-0003", theme_warm_flow, 34, 2303),
    ("perlin-gen-0004", theme_contour_bands, 42, 2404),
    ("perlin-gen-0005", theme_pastel_stitch, 36, 2505),
]


def main():
    for name, theme, scale, seed in THEMES:
        for platform, (w, h) in PLATFORMS.items():
            rng = np.random.default_rng(seed)
            random.seed(seed)
            noise = perlin_grid(w, h, scale, rng)
            img = Image.new("RGB", (w, h), (255, 255, 255))
            theme(ImageDraw.Draw(img), w, h, noise, rng)
            path = os.path.join(OUT_DIR, "%s~%s.png" % (name, platform))
            img.save(path, optimize=True)
            print(path)


if __name__ == "__main__":
    main()
