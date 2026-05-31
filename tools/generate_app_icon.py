#!/usr/bin/env python3
from __future__ import annotations

import math
import struct
from pathlib import Path


SIZES = (16, 24, 32, 48, 64, 128, 256)
SUPERSAMPLE = 4


def blend(dst: tuple[int, int, int, int], src: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    sr, sg, sb, sa = src
    dr, dg, db, da = dst
    sa_f = sa / 255.0
    da_f = da / 255.0
    out_a = sa_f + da_f * (1.0 - sa_f)
    if out_a <= 0.0:
        return (0, 0, 0, 0)
    r = (sr * sa_f + dr * da_f * (1.0 - sa_f)) / out_a
    g = (sg * sa_f + dg * da_f * (1.0 - sa_f)) / out_a
    b = (sb * sa_f + db * da_f * (1.0 - sa_f)) / out_a
    return (round(r), round(g), round(b), round(out_a * 255.0))


class Canvas:
    def __init__(self, size: int) -> None:
        self.size = size
        self.pixels = [(0, 0, 0, 0)] * (size * size)

    def put(self, x: int, y: int, color: tuple[int, int, int, int]) -> None:
        if 0 <= x < self.size and 0 <= y < self.size:
            index = y * self.size + x
            self.pixels[index] = blend(self.pixels[index], color)

    def disc(self, cx: float, cy: float, radius: float, color: tuple[int, int, int, int]) -> None:
        left = math.floor(cx - radius)
        right = math.ceil(cx + radius)
        top = math.floor(cy - radius)
        bottom = math.ceil(cy + radius)
        radius_sq = radius * radius
        for y in range(top, bottom + 1):
            for x in range(left, right + 1):
                if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= radius_sq:
                    self.put(x, y, color)

    def line(self, ax: float, ay: float, bx: float, by: float, width: float, color: tuple[int, int, int, int]) -> None:
        steps = max(1, int(math.hypot(bx - ax, by - ay) * 1.4))
        for i in range(steps + 1):
            t = i / steps
            self.disc(ax + (bx - ax) * t, ay + (by - ay) * t, width / 2.0, color)

    def polygon(self, points: list[tuple[float, float]], color: tuple[int, int, int, int]) -> None:
        min_x = math.floor(min(point[0] for point in points))
        max_x = math.ceil(max(point[0] for point in points))
        min_y = math.floor(min(point[1] for point in points))
        max_y = math.ceil(max(point[1] for point in points))
        for y in range(min_y, max_y + 1):
            for x in range(min_x, max_x + 1):
                if point_in_polygon(x + 0.5, y + 0.5, points):
                    self.put(x, y, color)


def point_in_polygon(x: float, y: float, points: list[tuple[float, float]]) -> bool:
    inside = False
    j = len(points) - 1
    for i, point in enumerate(points):
        xi, yi = point
        xj, yj = points[j]
        if (yi > y) != (yj > y):
            intersect_x = (xj - xi) * (y - yi) / (yj - yi) + xi
            if x < intersect_x:
                inside = not inside
        j = i
    return inside


def inside_rounded_rect(x: float, y: float, size: int, margin: float, radius: float) -> bool:
    left = margin
    top = margin
    right = size - margin
    bottom = size - margin
    cx = min(max(x, left + radius), right - radius)
    cy = min(max(y, top + radius), bottom - radius)
    return (x - cx) ** 2 + (y - cy) ** 2 <= radius * radius


def fill_card(canvas: Canvas) -> None:
    size = canvas.size
    margin = size * 0.07
    radius = size * 0.16
    inner_margin = margin + size * 0.018
    orange = (231, 91, 20, 255)
    orange_hi = (255, 147, 48, 255)
    black = (7, 9, 13, 255)
    black_hi = (31, 36, 46, 255)

    for y in range(size):
        for x in range(size):
            if not inside_rounded_rect(x + 0.5, y + 0.5, size, margin, radius):
                continue
            split = (x / size) + (y / size) * 0.26
            if split < 0.63:
                mix = y / size
                color = (
                    round(orange_hi[0] * (1 - mix) + orange[0] * mix),
                    round(orange_hi[1] * (1 - mix) + orange[1] * mix),
                    round(orange_hi[2] * (1 - mix) + orange[2] * mix),
                    255,
                )
            else:
                mix = y / size
                color = (
                    round(black_hi[0] * (1 - mix) + black[0] * mix),
                    round(black_hi[1] * (1 - mix) + black[1] * mix),
                    round(black_hi[2] * (1 - mix) + black[2] * mix),
                    255,
                )
            canvas.put(x, y, color)

            if not inside_rounded_rect(x + 0.5, y + 0.5, size, inner_margin, radius * 0.86):
                canvas.put(x, y, (255, 230, 196, 170))

    canvas.line(size * 0.18, size * 0.82, size * 0.82, size * 0.18, size * 0.018, (255, 255, 255, 90))
    canvas.line(size * 0.20, size * 0.18, size * 0.80, size * 0.18, size * 0.012, (255, 255, 255, 86))
    canvas.line(size * 0.20, size * 0.82, size * 0.80, size * 0.82, size * 0.012, (255, 145, 48, 122))


def draw_rebel_mark(canvas: Canvas) -> None:
    size = canvas.size
    cx = size * 0.34
    cy = size * 0.38
    sx = size * 0.20
    sy = size * 0.21
    white = (255, 255, 255, 225)

    def p(x: float, y: float) -> tuple[float, float]:
        return (cx + x * sx, cy + y * sy)

    canvas.polygon([p(-0.08, -0.92), p(-0.72, -0.32), p(-0.78, 0.06), p(-0.20, 0.86), p(-0.34, 0.05)], white)
    canvas.polygon([p(0.08, -0.92), p(0.72, -0.32), p(0.78, 0.06), p(0.20, 0.86), p(0.34, 0.05)], white)
    canvas.polygon([p(0.0, -0.98), p(0.18, 0.80), p(0.0, 1.04), p(-0.18, 0.80)], white)
    canvas.disc(cx, cy + sy * 0.06, size * 0.025, (255, 255, 255, 245))


def draw_empire_mark(canvas: Canvas) -> None:
    size = canvas.size
    cx = size * 0.66
    cy = size * 0.38
    radius = size * 0.145
    white = (255, 255, 255, 230)

    for i in range(8):
        angle = math.pi * 2.0 * i / 8.0
        canvas.line(
            cx + math.cos(angle) * radius * 0.34,
            cy + math.sin(angle) * radius * 0.34,
            cx + math.cos(angle) * radius * 0.98,
            cy + math.sin(angle) * radius * 0.98,
            size * 0.018,
            white,
        )
    ring_width = size * 0.018
    for y in range(math.floor(cy - radius - ring_width), math.ceil(cy + radius + ring_width) + 1):
        for x in range(math.floor(cx - radius - ring_width), math.ceil(cx + radius + ring_width) + 1):
            dist = math.hypot(x + 0.5 - cx, y + 0.5 - cy)
            if radius - ring_width <= dist <= radius + ring_width:
                canvas.put(x, y, white)
            if radius * 0.42 - ring_width * 0.62 <= dist <= radius * 0.42 + ring_width * 0.62:
                canvas.put(x, y, white)
    canvas.disc(cx, cy, size * 0.024, (255, 255, 255, 245))


FONT = {
    "G": (
        "01110",
        "10001",
        "10000",
        "10111",
        "10001",
        "10001",
        "01110",
    ),
    "D": (
        "11110",
        "10001",
        "10001",
        "10001",
        "10001",
        "10001",
        "11110",
    ),
}


def draw_text(canvas: Canvas, text: str) -> None:
    size = canvas.size
    cell = max(1, round(size * 0.018))
    gap = cell
    glyph_width = 5 * cell
    glyph_height = 7 * cell
    text_width = len(text) * glyph_width + (len(text) - 1) * gap
    start_x = round((size - text_width) / 2)
    start_y = round(size * 0.60)
    shadow = (0, 0, 0, 120)
    white = (255, 242, 220, 245)

    for dx, dy, color in ((cell, cell, shadow), (0, 0, white)):
        x_cursor = start_x + dx
        for ch in text:
            pattern = FONT[ch]
            for row, bits in enumerate(pattern):
                for col, bit in enumerate(bits):
                    if bit == "1":
                        for py in range(cell):
                            for px in range(cell):
                                canvas.put(x_cursor + col * cell + px, start_y + dy + row * cell + py, color)
            x_cursor += glyph_width + gap


def downsample(canvas: Canvas, target_size: int) -> list[tuple[int, int, int, int]]:
    source = canvas.size
    factor = source // target_size
    pixels: list[tuple[int, int, int, int]] = []
    for y in range(target_size):
        for x in range(target_size):
            total = [0, 0, 0, 0]
            for sy in range(factor):
                for sx in range(factor):
                    r, g, b, a = canvas.pixels[(y * factor + sy) * source + (x * factor + sx)]
                    total[0] += r
                    total[1] += g
                    total[2] += b
                    total[3] += a
            count = factor * factor
            pixels.append(tuple(round(channel / count) for channel in total))
    return pixels


def render_icon(size: int) -> list[tuple[int, int, int, int]]:
    canvas = Canvas(size * SUPERSAMPLE)
    fill_card(canvas)
    draw_rebel_mark(canvas)
    draw_empire_mark(canvas)
    draw_text(canvas, "GD")
    return downsample(canvas, size)


def dib_from_pixels(size: int, pixels: list[tuple[int, int, int, int]]) -> bytes:
    xor = bytearray()
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r, g, b, a = pixels[y * size + x]
            xor.extend((b, g, r, a))

    mask_stride = ((size + 31) // 32) * 4
    mask = bytearray(mask_stride * size)
    for out_y, y in enumerate(range(size - 1, -1, -1)):
        for x in range(size):
            if pixels[y * size + x][3] < 128:
                mask[out_y * mask_stride + x // 8] |= 0x80 >> (x % 8)

    header = struct.pack(
        "<IIIHHIIIIII",
        40,
        size,
        size * 2,
        1,
        32,
        0,
        len(xor) + len(mask),
        0,
        0,
        0,
        0,
    )
    return header + bytes(xor) + bytes(mask)


def write_ico(path: Path) -> None:
    images = [(size, dib_from_pixels(size, render_icon(size))) for size in SIZES]
    offset = 6 + len(images) * 16
    directory = bytearray(struct.pack("<HHH", 0, 1, len(images)))
    body = bytearray()
    for size, data in images:
        width = 0 if size == 256 else size
        directory.extend(struct.pack("<BBBBHHII", width, width, 0, 0, 1, 32, len(data), offset))
        body.extend(data)
        offset += len(data)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(bytes(directory + body))


def main() -> None:
    repo_root = Path(__file__).resolve().parents[1]
    write_ico(repo_root / "resources" / "icons" / "app_icon.ico")


if __name__ == "__main__":
    main()
