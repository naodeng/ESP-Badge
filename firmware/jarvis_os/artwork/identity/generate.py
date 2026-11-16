#!/usr/bin/env python3
"""Generate review assets and the native geometry header from the same SVG geometry."""
from pathlib import Path
import math

ROOT = Path(__file__).resolve().parent
INK = "#17191b"
RED = "#e54230"
WHITE = "#f4f5f5"
REACTOR_CYAN = "#00d9f2"
REACTOR_ICE = "#befaff"
REACTOR_DIM = "#174550"
REACTOR_RIM = "#0b3038"
REACTOR_SIZE = 176
VIEWBOX = 80.0
RING_CX, RING_CY, RING_RADIUS = 37.25, 42.75, 22.375
RING_COUNT = 48
RING_DOT = 2.4
DOT_CX, DOT_CY, DOT_RADIUS = 62.75, 17.25, 3.4
J_PATTERN = ("00111", "00010", "00010", "00010", "00010", "10010", "01100")
J_PITCH, J_DOT = 4.2, 2.2

def ring_points(start=-45):
    for index in range(RING_COUNT):
        angle = math.radians(start + index * 360 / RING_COUNT)
        yield RING_CX + math.cos(angle) * RING_RADIUS, RING_CY + math.sin(angle) * RING_RADIUS

def matrix_points():
    for row, bits in enumerate(J_PATTERN):
        for column, bit in enumerate(bits):
            if bit == "1":
                yield (RING_CX + (column - 2) * J_PITCH,
                       RING_CY + (row - 3) * J_PITCH)

def svg_dot(x, y, radius, color, extra=""):
    return f'<circle cx="{x:.3f}" cy="{y:.3f}" r="{radius:.3f}" fill="{color}"{extra}/>'

def mark(color):
    ring = "\n".join(svg_dot(x, y, RING_DOT / 2, color) for x, y in ring_points())
    matrix = "\n".join(svg_dot(x, y, J_DOT / 2, color) for x, y in matrix_points())
    return f'''<g aria-label="JARVIS OS dot-matrix mark">
{ring}
{matrix}
{svg_dot(DOT_CX, DOT_CY, DOT_RADIUS, RED)}
</g>'''

def logo(color):
    return f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 80 80" role="img" aria-label="Jarvis OS 点阵标志">
{mark(color)}
</svg>'''

def about(english=False):
    title = "About" if english else "关于本机"
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="466" height="466" viewBox="0 0 466 466" role="img" aria-label="Jarvis OS {title}设计稿">
<circle cx="233" cy="233" r="233" fill="#000"/>
<circle cx="233" cy="233" r="225" fill="none" stroke="#101517" stroke-width="8"/>
<path d="M233 8 A225 225 0 1 1 8.44 247.13" fill="none" stroke="{WHITE}" stroke-width="8" stroke-linecap="round"/>
<g font-family="Helvetica Neue, Arial, PingFang SC, sans-serif" text-anchor="middle">
<circle cx="123" cy="74" r="22" fill="#181818"/>
<path d="M126 67 L119 74 L126 81" fill="none" stroke="{WHITE}" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
<text x="233" y="77" font-size="24" fill="{WHITE}">{title}</text>
<g transform="translate(163 105) scale(1.75)">{mark(WHITE)}</g>
<text x="233" y="279" font-size="34" font-weight="500" letter-spacing="-.6" fill="{WHITE}">JARVIS OS</text>
<text x="233" y="315" font-size="19" fill="#a1a4a6">v1.7-beta.27</text>
<path d="M180 342 H286" stroke="#24282a" stroke-width="1"/>
<text x="233" y="376" font-size="19" fill="#a1a4a6">ESP32-S3</text>
<text x="233" y="406" font-size="16" fill="#757b7e">466 × 466 AMOLED</text>
</g>
</svg>'''

def point_in_polygon(x, y, points):
    inside = False
    for index, (xi, yi) in enumerate(points):
        xj, yj = points[index - 1]
        if (yi > y) != (yj > y) and x < (xj - xi) * (y - yi) / (yj - yi) + xi:
            inside = not inside
    return inside

def inside_triangle(x, y, a, b, c):
    def edge(p, q):
        return (x - p[0]) * (q[1] - p[1]) - (y - p[1]) * (q[0] - p[0])
    sides = (edge(a, b), edge(b, c), edge(c, a))
    return all(side >= 0 for side in sides) or all(side <= 0 for side in sides)

def reactor_points():
    top = [(-22, -59), (22, -59), (48, -40), (-48, -40)]
    left = [(-60, -28), (-44, -38), (-12, 20), (-14, 51), (-26, 54), (-56, 15)]
    right = [(60, -28), (44, -38), (12, 20), (14, 51), (26, 54), (56, 15)]
    for y in range(-56, 57, 8):
        for x in range(-56, 57, 8):
            if x * x + y * y > 61 * 61:
                continue
            if inside_triangle(x, y, (-25, -15), (25, -15), (0, 31)):
                yield x, y, REACTOR_ICE
            elif point_in_polygon(x, y, top):
                yield x, y, REACTOR_CYAN
            elif point_in_polygon(x, y, left) or point_in_polygon(x, y, right):
                yield x, y, REACTOR_CYAN

def boot():
    center = REACTOR_SIZE / 2
    ring = [(center + math.cos(math.radians(-90 + index * 360 / 72)) * 74,
             center + math.sin(math.radians(-90 + index * 360 / 72)) * 74)
            for index in range(72)]
    ring_dim = "\n".join(svg_dot(x, y, 1.8, REACTOR_DIM) for x, y in ring)
    ring_lit = "\n".join(
        svg_dot(x, y, 1.8, REACTOR_CYAN,
                f' opacity="0"><animate attributeName="opacity" from="0" to="1" begin="{0.10 + index * 1.45 / len(ring):.3f}s" dur=".10s" fill="freeze"/></circle>')
        for index, (x, y) in enumerate(ring))
    points = list(reactor_points())
    matrix_dim = "\n".join(svg_dot(center + x, center + y, 1.8, REACTOR_DIM)
                            for x, y, _ in points)
    matrix_lit = "\n".join(
        svg_dot(center + x, center + y, 1.8 if color == REACTOR_CYAN else 2.3, color,
                f' opacity="0"><animate attributeName="opacity" from="0" to="1" begin="{0.22 + index * 1.20 / max(1, len(points) - 1):.3f}s" dur=".09s" fill="freeze"/></circle>')
        for index, (x, y, color) in enumerate(points))
    rim = [(center + math.cos(math.radians(-90 + index * 360 / 48)) * 55,
            center + math.sin(math.radians(-90 + index * 360 / 48)) * 55)
           for index in range(48)]
    rim_dim = "\n".join(svg_dot(x, y, 1.3, REACTOR_RIM) for x, y in rim)
    rim_lit = "\n".join(
        svg_dot(x, y, 1.3, REACTOR_DIM,
                f' opacity="0"><animate attributeName="opacity" from="0" to="1" begin="{0.52 + index * .82 / len(rim):.3f}s" dur=".09s" fill="freeze"/></circle>')
        for index, (x, y) in enumerate(rim))
    core_points = [(center - 25, center - 15), (center + 25, center - 15),
                   (center, center + 31)]
    core_lit = "\n".join(
        svg_dot(x, y, 2.8, REACTOR_ICE,
                ' opacity="0"><animate attributeName="opacity" from="0" to="1" begin=".55s" dur=".35s" fill="freeze"/></circle>')
        for x, y in core_points)
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="466" height="466" viewBox="0 0 466 466" role="img" aria-label="Jarvis OS 点阵开机动画，至少4秒并等待设备初始化">
<circle cx="233" cy="233" r="233" fill="#000"/>
<g transform="translate(145 145)">
{ring_dim}
{ring_lit}
{matrix_dim}
{matrix_lit}
{rim_dim}
{rim_lit}
{core_lit}
</g>
<text x="233" y="342" text-anchor="middle" font-family="Helvetica Neue, Arial, sans-serif" font-size="26" font-weight="500" fill="{WHITE}" opacity="0">
JARVIS OS
<animate attributeName="opacity" from="0" to="1" begin="1.82s" dur=".48s" fill="freeze"/>
<animate attributeName="y" from="349" to="342" begin="1.82s" dur=".48s" calcMode="spline" keyTimes="0;1" keySplines=".22 .61 .36 1" fill="freeze"/>
</text>
<text x="233" y="391" text-anchor="middle" font-family="PingFang SC, Arial, sans-serif" font-size="18" fill="#9a9a9e" opacity="0">System check...
<animate attributeName="opacity" from="0" to="1" begin="2.5s" dur=".3s" fill="freeze"/>
</text>
</svg>'''

def main():
    assets = {"logo.svg": logo(INK), "logo-dark.svg": logo(WHITE),
              "about.svg": about(), "about-en.svg": about(True), "boot.svg": boot()}
    for name, contents in assets.items():
        (ROOT / name).write_text(contents + "\n", encoding="utf-8")
    # The firmware geometry is derived from the approved vector, not traced from a raster.
    geometry = {"VIEWBOX": VIEWBOX, "RING_CX": RING_CX, "RING_CY": RING_CY,
                "RING_RADIUS": RING_RADIUS, "RING_DOT_DIAMETER": RING_DOT,
                "RING_DOT_COUNT": RING_COUNT, "J_PITCH": J_PITCH,
                "J_DOT_DIAMETER": J_DOT, "DOT_CX": DOT_CX, "DOT_CY": DOT_CY,
                "DOT_RADIUS": DOT_RADIUS}
    header = "#pragma once\n// Generated by artwork/identity/generate.py from logo-dark.svg.\n"
    header += "".join(
        f"#define IDENTITY_{key} {value if key == 'RING_DOT_COUNT' else f'{float(value):.3f}f'}\n"
        for key, value in geometry.items())
    header += f"#define IDENTITY_WHITE 0x{WHITE[1:]}\n#define IDENTITY_RED 0x{RED[1:]}\n"
    (ROOT.parent.parent / "main" / "identity_geometry.h").write_text(header, encoding="utf-8")
    template = (ROOT / "preview.template.html").read_text(encoding="utf-8")
    for key, value in {"{{LOGO}}": assets["logo.svg"], "{{LOGO_DARK}}": assets["logo-dark.svg"],
                       "{{ABOUT}}": assets["about.svg"], "{{ABOUT_EN}}": assets["about-en.svg"],
                       "{{BOOT}}": assets["boot.svg"]}.items():
        template = template.replace(key, value)
    (ROOT / "index.html").write_text(template, encoding="utf-8")
    print("Generated 5 SVG assets, including the arc-reactor boot mark, the review page and native geometry header.")

if __name__ == "__main__":
    main()
