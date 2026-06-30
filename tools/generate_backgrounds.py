#!/usr/bin/env python3
"""Generate the procedural perlin-noise backgrounds (IMAGE_BG1 .. IMAGE_BG<NUM_THEMES>).

Since v1.5.0 every background is procedurally generated (the original
hand-made art was replaced in favor of these). Five glyph styles, each in
five palette variants, rendered natively for every target platform:

  basalt 144x168, chalk 180x180, emery 200x228

Each image is quantized to at most 16 colors. That is deliberate: the SDK
then packs them as 4-bit palettized GBitmaps, which the watch code can
invert in place via gbitmap_get_palette() when Bluetooth disconnects.

A 2D Perlin noise field drives glyph hue, density and orientation. Runs are
seeded, so re-running the script reproduces the shipped artwork exactly.

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

MAX_COLORS = 16   # keep 4-bit palettized so the watch can palette-invert


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


def make_pal(h0, span, s=0.85, v=0.85):
    """Map a noise value 0..1 to an RGB color along a hue ramp."""
    def pal(n):
        r, g, b = colorsys.hsv_to_rgb((h0 + n * span) % 1.0, s, v)
        return (int(r * 255), int(g * 255), int(b * 255))
    return pal


# ---------------------------------------------------------------------------
# Glyph primitives.
# ---------------------------------------------------------------------------
# Stroke weight for the line-based glyphs. The original art used width=2,
# which reads as thin specks once the SDK downsamples; 3px strokes stay
# legible on all three target sizes while still packing to <=16 colors.
STROKE = 3


def glyph_arrow(d, x, y, size, color, angle):
    x2 = x + size * math.cos(angle)
    y2 = y + size * math.sin(angle)
    d.line([(x, y), (x2, y2)], fill=color, width=STROKE)
    for side in (-1, 1):
        a = angle + math.pi + side * 0.5
        hx = x2 + size * 0.5 * math.cos(a)
        hy = y2 + size * 0.5 * math.sin(a)
        d.line([(x2, y2), (hx, hy)], fill=color, width=STROKE)


def glyph_box(d, x, y, size, color):
    d.rectangle([x, y, x + size, y + size], outline=color, width=2)


def glyph_cross(d, x, y, size, color):
    h = size / 2
    d.line([(x - h, y), (x + h, y)], fill=color, width=STROKE)
    d.line([(x, y - h), (x, y + h)], fill=color, width=STROKE)


def glyph_zig(d, x, y, size, color, angle):
    pts = [(x, y)]
    a = angle
    for _ in range(4):
        x += size * 0.6 * math.cos(a)
        y += size * 0.6 * math.sin(a)
        pts.append((x, y))
        a += math.pi / 2 * random.choice((-1, 1))
    d.line(pts, fill=color, width=STROKE, joint="curve")


# ---------------------------------------------------------------------------
# The five styles. Each fills a white canvas with noise-driven glyphs.
# ---------------------------------------------------------------------------
def style_arrows(d, w, h, noise, rng, pal):
    # Tighter grid + lower cutoff so arrows tile the field instead of
    # scattering as isolated specks; size scales with the noise value.
    step = 9
    for gy in range(4, h, step):
        for gx in range(4, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.10:
                continue
            jx, jy = gx + rng.uniform(-2, 2), gy + rng.uniform(-2, 2)
            glyph_arrow(d, jx, jy, 8 + n * 4, pal(n), n * 4 * math.pi)


def style_boxes(d, w, h, noise, rng, pal):
    step = 9
    for gy in range(2, h, step):
        for gx in range(2, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.16:
                continue
            size = 5 + n * 8
            glyph_box(d, gx + rng.uniform(-2, 2), gy + rng.uniform(-2, 2), size, pal(n))
            if n > 0.6:
                glyph_box(d, gx + 3, gy + 3, size - 5, pal(n))


def style_flow(d, w, h, noise, rng, pal):
    # More, longer streamlines: denser coverage and continuous ribbons
    # rather than the short 3-segment dashes that read as spots.
    for _ in range(int(w * h / 22)):
        x, y = rng.uniform(0, w), rng.uniform(0, h)
        n = noise[int(min(y, h - 1)), int(min(x, w - 1))]
        if n < 0.10:
            continue
        c = pal(n)
        a = n * 6 * math.pi
        pts = [(x, y)]
        for _ in range(6):
            x += 5 * math.cos(a)
            y += 5 * math.sin(a)
            nn = noise[int(min(max(y, 0), h - 1)), int(min(max(x, 0), w - 1))]
            a = nn * 6 * math.pi
            pts.append((x, y))
        d.line(pts, fill=c, width=STROKE, joint="curve")


def style_contours(d, w, h, noise, rng, pal):
    # More levels packs the rings closer together, and each boundary pixel
    # is drawn as a 2px block so the lines actually read instead of fading.
    levels = 18
    px = (noise * levels).astype(int)
    for y in range(h - 1):
        for x in range(w - 1):
            if px[y, x] != px[y, x + 1] or px[y, x] != px[y + 1, x]:
                d.rectangle([x, y, x + 1, y + 1], fill=pal(px[y, x] / levels))


def style_stitch(d, w, h, noise, rng, pal):
    step = 9
    for gy in range(4, h, step):
        for gx in range(4, w, step):
            n = noise[min(gy, h - 1), min(gx, w - 1)]
            if n < 0.12:
                continue
            c = pal(n)
            if n > 0.6:
                glyph_zig(d, gx, gy, 10, c, n * 4 * math.pi)
            else:
                glyph_cross(d, gx + rng.uniform(-2, 2), gy + rng.uniform(-2, 2), 9, c)


STYLES = [
    ("arrows", style_arrows, 30),
    ("boxes", style_boxes, 38),
    ("flow", style_flow, 34),
    ("contours", style_contours, 42),
    ("stitch", style_stitch, 36),
]

# Five palette variants per style; (h0, span, s, v).
PALETTES = {
    "arrows":   [(0.00, 1.00, 0.85, 0.85), (0.50, 0.30, 0.85, 0.80),
                 (0.93, 0.22, 0.90, 0.90), (0.68, 0.25, 0.80, 0.80),
                 (0.20, 0.28, 0.85, 0.75)],
    "boxes":    [(0.45, 0.25, 0.90, 0.80), (0.93, 0.14, 0.85, 0.85),
                 (0.58, 0.22, 0.85, 0.80), (0.06, 0.14, 0.90, 0.90),
                 (0.33, 0.20, 0.85, 0.75)],
    "flow":     [(0.02, 0.13, 0.90, 0.95), (0.55, 0.15, 0.85, 0.85),
                 (0.83, 0.14, 0.75, 0.95), (0.16, 0.16, 0.85, 0.85),
                 (0.58, 0.10, 0.45, 0.75)],
    "contours": [(0.00, 1.00, 0.85, 0.85), (0.55, 0.25, 0.85, 0.80),
                 (0.98, 0.18, 0.90, 0.90), (0.70, 0.22, 0.80, 0.80),
                 (0.25, 0.25, 0.85, 0.75)],
    "stitch":   [(0.00, 1.00, 0.55, 0.95), (0.52, 0.25, 0.50, 0.95),
                 (0.90, 0.20, 0.50, 0.95), (0.40, 0.22, 0.50, 0.95),
                 (0.70, 0.22, 0.50, 0.95)],
}


# Hand-picked selection: the 10 strongest of the 25 style x palette
# combinations, weighted toward the contour and flow styles (the densest,
# most legible) while keeping all five styles represented and the hues
# spread out. Listed by their original index in the 5x5 grid
# (style = (i-1)%5, palette = (i-1)//5) so the seed -- and therefore the
# exact artwork -- is preserved; they ship renumbered as BG1..BG10 in this
# order. Edit this list to change the rotation.
CHOSEN = [4, 14, 19, 24, 13, 23, 8, 11, 7, 20]


def themes():
    """Yield (out_index, style_fn, scale, seed, palette) for each shipped BG.

    out_index is the BG number written to disk (1..len(CHOSEN)); the seed is
    derived from the source index so re-running reproduces the picked art.
    """
    for out_index, src in enumerate(CHOSEN, start=1):
        sname, sfn, scale = STYLES[(src - 1) % 5]
        pp = PALETTES[sname][(src - 1) // 5]
        yield out_index, sfn, scale, 3000 + src, make_pal(*pp)


def main():
    for n, style_fn, scale, seed, pal in themes():
        for platform, (w, h) in PLATFORMS.items():
            rng = np.random.default_rng(seed)
            random.seed(seed)
            noise = perlin_grid(w, h, scale, rng)
            img = Image.new("RGB", (w, h), (255, 255, 255))
            style_fn(ImageDraw.Draw(img), w, h, noise, rng, pal)
            img = img.quantize(colors=MAX_COLORS, method=Image.MEDIANCUT,
                               dither=Image.NONE)
            path = os.path.join(OUT_DIR, "perlin-gen-%04d~%s.png" % (n, platform))
            img.save(path, optimize=True)
            print(path)


if __name__ == "__main__":
    main()
