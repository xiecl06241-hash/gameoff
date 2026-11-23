"""Generate original 1024x1024 procedural floor spill texture sets (Albedo+Alpha, Normal, Mask).

Produces three distinct, realistic night-shift convenience store spill variants:
  - Spill_Coffee: dark spilled coffee / soy sauce puddle with coffee-ring edge and radial splashes
  - Spill_Ichor:  viscous dark otherworldly ooze with organic tendrils and subtle surface bubbles
  - Spill_Mud:    rainy-night muddy bootprints, smear streaks, and gritty splatter

All textures are 100% procedurally generated via NumPy + Pillow (no external assets).
"""
from pathlib import Path
import numpy as np
from PIL import Image, ImageFilter

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "SourceArt" / "Spills"
SIZE = 1024


def smoothstep(edge0, edge1, x):
    t = np.clip((x - edge0) / np.maximum(edge1 - edge0, 1e-6), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def value_noise_2d(h, w, grid, rng):
    """Bilinear-interpolated smooth 2D value noise in [0, 1]."""
    base = rng.random((grid + 1, grid + 1)).astype(np.float32)
    y = np.linspace(0.0, grid - 1e-5, h, dtype=np.float32)
    x = np.linspace(0.0, grid - 1e-5, w, dtype=np.float32)
    yi = y.astype(np.int32)
    xi = x.astype(np.int32)
    yf = y - yi
    xf = x - xi
    # Quintic fade for smooth derivatives (important for clean normal maps)
    u = xf * xf * xf * (xf * (xf * 6.0 - 15.0) + 10.0)
    v = yf * yf * yf * (yf * (yf * 6.0 - 15.0) + 10.0)

    c00 = base[yi[:, None], xi[None, :]]
    c10 = base[yi[:, None], xi[None, :] + 1]
    c01 = base[yi[:, None] + 1, xi[None, :]]
    c11 = base[yi[:, None] + 1, xi[None, :] + 1]

    nx0 = c00 * (1.0 - u[None, :]) + c10 * u[None, :]
    nx1 = c01 * (1.0 - u[None, :]) + c11 * u[None, :]
    return nx0 * (1.0 - v[:, None]) + nx1 * v[:, None]


def fbm(h, w, octaves, seed):
    """Multi-octave fractal noise normalized to [0, 1]."""
    rng = np.random.default_rng(seed)
    acc = np.zeros((h, w), dtype=np.float32)
    amp = 0.5
    total = 0.0
    for grid in octaves:
        acc += amp * value_noise_2d(h, w, grid, rng)
        total += amp
        amp *= 0.52
    return acc / max(total, 1e-6)


def make_grid(size=SIZE):
    lin = np.linspace(-1.0, 1.0, size, dtype=np.float32)
    x, y = np.meshgrid(lin, lin)
    r = np.sqrt(x * x + y * y)
    theta = np.arctan2(y, x)
    return x, y, r, theta


def height_to_normal(height, strength=8.0):
    """Compute tangent-space normal map (UE DirectX green-down convention) from height [0, 1]."""
    # Sobel-like central differences
    dx = np.zeros_like(height)
    dy = np.zeros_like(height)
    dx[:, 1:-1] = (height[:, 2:] - height[:, :-2]) * 0.5
    dy[1:-1, :] = (height[2:, :] - height[:-2, :]) * 0.5

    nx = -dx * strength
    ny = -dy * strength  # UE tangent space normal
    nz = np.ones_like(height)
    inv_len = 1.0 / np.sqrt(nx * nx + ny * ny + nz * nz + 1e-8)
    nx *= inv_len
    ny *= inv_len
    nz *= inv_len

    rgb = np.stack([(nx * 0.5 + 0.5), (ny * 0.5 + 0.5), (nz * 0.5 + 0.5)], axis=-1)
    return np.clip(rgb * 255.0, 0, 255).astype(np.uint8)


def add_droplets(field, x, y, count, rng, warp_x, warp_y, r_min=0.30, r_max=0.86, size_min=0.006, size_max=0.032, stretch=2.2):
    """Add organic, noise-warped radial splash droplets clustered naturally around the puddle."""
    # Bias droplets closer to the main puddle with a few far splashes
    for i in range(count):
        ang = rng.uniform(0.0, 2.0 * np.pi)
        # Power distribution clusters more droplets near the splash rim
        u = rng.random() ** 1.45
        dist = r_min + u * (r_max - r_min)
        cx = np.cos(ang) * dist
        cy = np.sin(ang) * dist
        rad = rng.uniform(size_min, size_max) * (1.15 - 0.45 * u)
        elong = rng.uniform(1.05, stretch)

        dx = (x - cx) + warp_x * 0.14
        dy = (y - cy) + warp_y * 0.14
        ca, sa = np.cos(-ang), np.sin(-ang)
        rx = (dx * ca - dy * sa) / elong
        ry = dx * sa + dy * ca
        # Slight teardrop taper away from center
        taper = 1.0 + 0.35 * np.clip(rx / max(rad, 1e-4), -0.8, 0.8)
        d = np.sqrt(rx * rx + (ry * taper) ** 2) / rad
        drop = 1.0 - smoothstep(0.76, 1.0, d)
        field = np.maximum(field, drop * rng.uniform(0.80, 0.98))
    return field


def save_variant(name, albedo_rgb, alpha, height, dissolve_mask, roughness, wetness):
    OUT.mkdir(parents=True, exist_ok=True)
    # 1. Albedo + Alpha (_D.png)
    rgba = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    rgba[..., :3] = np.clip(albedo_rgb * 255.0, 0, 255).astype(np.uint8)
    rgba[..., 3] = np.clip(alpha * 255.0, 0, 255).astype(np.uint8)
    Image.fromarray(rgba, mode="RGBA").save(OUT / f"T_Spill_{name}_D.png")

    # 2. Normal (_N.png)
    normal_rgb = height_to_normal(height, strength=14.0)
    Image.fromarray(normal_rgb, mode="RGB").save(OUT / f"T_Spill_{name}_N.png")

    # 3. Mask (_M.png): R = dissolve priority (high = stays longest), G = roughness, B = wetness
    mask = np.zeros((SIZE, SIZE, 3), dtype=np.uint8)
    mask[..., 0] = np.clip(dissolve_mask * 255.0, 0, 255).astype(np.uint8)
    mask[..., 1] = np.clip(roughness * 255.0, 0, 255).astype(np.uint8)
    mask[..., 2] = np.clip(wetness * 255.0, 0, 255).astype(np.uint8)
    Image.fromarray(mask, mode="RGB").save(OUT / f"T_Spill_{name}_M.png")


def build_coffee_spill():
    """Dark spilled coffee / soy sauce puddle with crisp liquid rim, amber depth, and organic splashes."""
    x, y, r, theta = make_grid()
    rng = np.random.default_rng(1042)

    warp_x = (fbm(SIZE, SIZE, [4, 8, 16, 32, 64], 101) - 0.5) * 0.38
    warp_y = (fbm(SIZE, SIZE, [4, 8, 16, 32, 64], 102) - 0.5) * 0.38
    wx = x + warp_x
    wy = y + warp_y
    wr = np.sqrt((wx * 0.90) ** 2 + (wy * 1.10) ** 2)
    wtheta = np.arctan2(wy, wx)

    # Irregular multi-lobed puddle boundary with crisp surface-tension edge
    lobe = (
        0.48
        + 0.12 * np.sin(wtheta * 3.0 + 0.5)
        + 0.08 * np.cos(wtheta * 5.0 - 1.1)
        + 0.05 * np.sin(wtheta * 8.0 + 2.2)
    )
    detail = fbm(SIZE, SIZE, [8, 16, 32, 64, 128], 103)
    boundary = lobe + (detail - 0.5) * 0.24

    puddle = 1.0 - smoothstep(boundary - 0.012, boundary + 0.005, wr)

    # Secondary smaller satellite pool connected or nearby
    sx, sy = wx - 0.36, wy + 0.25
    sr = np.sqrt(sx * sx + sy * sy)
    sat = 1.0 - smoothstep(0.145, 0.162, sr + (detail - 0.5) * 0.09)
    puddle = np.maximum(puddle, sat * 0.96)

    # Organic radial splash droplets + tiny micro-specks
    puddle = add_droplets(puddle, x, y, count=48, rng=rng, warp_x=warp_x, warp_y=warp_y,
                          r_min=0.36, r_max=0.86, size_min=0.007, size_max=0.032, stretch=2.4)
    puddle = add_droplets(puddle, x, y, count=55, rng=rng, warp_x=warp_x, warp_y=warp_y,
                          r_min=0.40, r_max=0.90, size_min=0.003, size_max=0.011, stretch=1.8)

    border_fade = 1.0 - smoothstep(0.88, 0.98, np.maximum(np.abs(x), np.abs(y)))
    alpha = np.clip(puddle * border_fade, 0.0, 1.0)

    # Coffee-ring effect: crisp dark rim + warm translucent amber-brown interior
    dist_inside = np.clip((boundary - wr) * 3.2, 0.0, 1.0) * alpha
    edge_ring = smoothstep(0.05, 0.45, alpha) * (1.0 - smoothstep(0.06, 0.26, dist_inside))
    swirl = fbm(SIZE, SIZE, [6, 12, 24, 48], 104)
    sediment = smoothstep(0.28, 0.82, swirl * 0.7 + dist_inside * 0.45) * alpha

    core_col = np.array([0.055, 0.022, 0.008], dtype=np.float32)
    amber_col = np.array([0.215, 0.102, 0.032], dtype=np.float32)
    ring_col = np.array([0.038, 0.014, 0.005], dtype=np.float32)

    t_depth = np.clip(dist_inside * 0.55 + sediment * 0.50, 0.0, 1.0)
    rgb = amber_col * (1.0 - t_depth[..., None]) + core_col * t_depth[..., None]
    rgb = rgb * (1.0 - edge_ring[..., None] * 0.65) + ring_col * (edge_ring[..., None] * 0.65)

    final_alpha = np.clip(alpha * (0.74 + 0.22 * sediment + 0.22 * edge_ring), 0.0, 0.95)

    # Crisp meniscus height + delicate liquid surface undulation
    meniscus = smoothstep(0.02, 0.65, alpha) * (1.0 - 0.14 * smoothstep(0.15, 0.45, dist_inside))
    micro_ripple = (fbm(SIZE, SIZE, [16, 32, 64], 105) - 0.5) * 0.05 * alpha
    height = np.clip(meniscus * 0.60 + micro_ripple, 0.0, 1.0)

    roughness = np.clip(0.06 + 0.05 * swirl + 0.16 * edge_ring + 0.25 * (1.0 - alpha), 0.05, 0.60)
    wetness = np.clip(alpha * (1.0 - 0.20 * edge_ring), 0.0, 1.0)

    center_weight = 1.0 - smoothstep(0.15, 0.92, r)
    dissolve = np.clip(alpha * (0.45 + 0.40 * center_weight + 0.15 * swirl), 0.0, 1.0)

    save_variant("Coffee", rgb, final_alpha, height, dissolve, roughness, wetness)


def build_ichor_spill():
    """Viscous dark otherworldly residue with sharp creeping tendrils and glossy bubbles."""
    x, y, r, theta = make_grid()
    rng = np.random.default_rng(2088)

    warp_x = (fbm(SIZE, SIZE, [4, 8, 16, 32, 64], 201) - 0.5) * 0.44
    warp_y = (fbm(SIZE, SIZE, [4, 8, 16, 32, 64], 202) - 0.5) * 0.44
    wx = x + warp_x
    wy = y + warp_y
    wr = np.sqrt(wx * wx + wy * wy)
    wtheta = np.arctan2(wy, wx)

    tendrils = (
        0.40
        + 0.20 * np.maximum(0.0, np.sin(wtheta * 5.0 + 0.8)) ** 2
        + 0.15 * np.maximum(0.0, np.cos(wtheta * 3.0 - 0.6)) ** 2
        + 0.07 * np.sin(wtheta * 9.0 + 1.5)
    )
    noise_hi = fbm(SIZE, SIZE, [8, 16, 32, 64, 128], 203)
    boundary = tendrils + (noise_hi - 0.5) * 0.22

    body = 1.0 - smoothstep(boundary - 0.012, boundary + 0.005, wr)
    body = add_droplets(body, x, y, count=36, rng=rng, warp_x=warp_x, warp_y=warp_y,
                        r_min=0.35, r_max=0.86, size_min=0.007, size_max=0.028, stretch=2.6)
    body = add_droplets(body, x, y, count=40, rng=rng, warp_x=warp_x, warp_y=warp_y,
                        r_min=0.38, r_max=0.90, size_min=0.003, size_max=0.010, stretch=2.0)

    border_fade = 1.0 - smoothstep(0.88, 0.98, np.maximum(np.abs(x), np.abs(y)))
    alpha = np.clip(body * border_fade, 0.0, 1.0)

    bubbles = np.zeros((SIZE, SIZE), dtype=np.float32)
    for _ in range(48):
        bx = rng.uniform(-0.46, 0.46)
        by = rng.uniform(-0.46, 0.46)
        brad = rng.uniform(0.010, 0.028)
        d = np.sqrt((x - bx) ** 2 + (y - by) ** 2) / brad
        dome = np.sqrt(np.clip(1.0 - d * d, 0.0, 1.0))
        bubbles = np.maximum(bubbles, dome * 0.72)
    bubbles *= smoothstep(0.50, 0.90, alpha)

    oil_swirl = fbm(SIZE, SIZE, [6, 14, 28, 56], 204)
    base_col = np.array([0.016, 0.022, 0.024], dtype=np.float32)
    sheen_col = np.array([0.052, 0.088, 0.078], dtype=np.float32)
    edge_col = np.array([0.085, 0.062, 0.036], dtype=np.float32)

    dist_inside = np.clip((boundary - wr) * 3.5, 0.0, 1.0) * alpha
    edge_band = smoothstep(0.05, 0.45, alpha) * (1.0 - smoothstep(0.05, 0.22, dist_inside))
    rgb = base_col * (1.0 - oil_swirl[..., None] * 0.75) + sheen_col * (oil_swirl[..., None] * 0.75)
    rgb = rgb * (1.0 - edge_band[..., None] * 0.55) + edge_col * (edge_band[..., None] * 0.55)

    final_alpha = np.clip(alpha * (0.88 + 0.12 * oil_swirl), 0.0, 0.98)

    thickness = smoothstep(0.02, 0.65, alpha) * 0.68
    height = np.clip(thickness + bubbles * 0.35 + (noise_hi - 0.5) * 0.09 * alpha, 0.0, 1.0)

    roughness = np.clip(0.05 + 0.06 * oil_swirl + 0.20 * edge_band + 0.28 * (1.0 - alpha), 0.04, 0.68)
    wetness = np.clip(alpha, 0.0, 1.0)

    center_weight = 1.0 - smoothstep(0.10, 0.90, r)
    dissolve = np.clip(alpha * (0.42 + 0.43 * center_weight + 0.15 * oil_swirl), 0.0, 1.0)

    save_variant("Ichor", rgb, final_alpha, height, dissolve, roughness, wetness)


def stamp_bootprint(field, x, y, cx, cy, angle, scale=1.0):
    """Stamp a crisp muddy bootprint with realistic sole outline and horizontal lug bars."""
    dx = x - cx
    dy = y - cy
    ca, sa = np.cos(-angle), np.sin(-angle)
    lx = (dx * ca - dy * sa) / scale
    ly = (dx * sa + dy * ca) / scale

    # Forefoot and heel sole silhouettes
    fore_d = np.sqrt(((lx - 0.11) / 0.195) ** 2 + (ly / 0.115) ** 2)
    heel_d = np.sqrt(((lx + 0.19) / 0.115) ** 2 + (ly / 0.092) ** 2)
    fore = 1.0 - smoothstep(0.92, 1.02, fore_d)
    heel = 1.0 - smoothstep(0.90, 1.02, heel_d)
    sole = np.maximum(fore, heel)

    # Outer rubber rim + crisp chevron/horizontal tread bars (no polka-dots)
    rim = np.maximum(
        smoothstep(0.76, 0.88, fore_d) * fore,
        smoothstep(0.72, 0.86, heel_d) * heel,
    )
    chevron = np.sin((lx + 0.18 * np.abs(ly)) * 125.0)
    lugs = smoothstep(-0.15, 0.30, chevron)
    tread = np.maximum(rim * 0.95, lugs * 0.88)
    boot = sole * tread
    return np.maximum(field, boot)


def build_mud_spill():
    """Rainy-night muddy bootprints, wet tracked-in slush, and gritty mud splatter."""
    x, y, r, theta = make_grid()
    rng = np.random.default_rng(3099)

    warp_x = (fbm(SIZE, SIZE, [6, 12, 24, 48], 303) - 0.5) * 0.24
    warp_y = (fbm(SIZE, SIZE, [6, 12, 24, 48], 304) - 0.5) * 0.24
    grit = fbm(SIZE, SIZE, [16, 32, 64, 128], 301)
    smear_noise = fbm(SIZE, SIZE, [4, 10, 22, 44], 302)

    # Crisp-edged tracked-in muddy slush puddle underneath
    wx = x + warp_x * 1.1
    wy = y + warp_y * 0.9
    smear_r = np.sqrt((wx * 0.82) ** 2 + (wy * 1.18) ** 2)
    slush_edge = 0.44 + (smear_noise - 0.5) * 0.26
    slush = 1.0 - smoothstep(slush_edge - 0.025, slush_edge + 0.015, smear_r)

    # Muddy drag streaks
    streaks = smoothstep(0.25, 0.78, np.sin(wy * 22.0 + smear_noise * 6.0) * 0.5 + 0.5)
    mud_puddle = slush * (0.52 + 0.42 * streaks)

    # Three realistic muddy bootprints
    boots = np.zeros((SIZE, SIZE), dtype=np.float32)
    boots = stamp_bootprint(boots, x, y, -0.18, -0.12, 0.28, scale=1.06)
    boots = stamp_bootprint(boots, x, y, 0.22, 0.14, 0.14, scale=1.04)
    boots = stamp_bootprint(boots, x, y, -0.02, 0.29, -0.42, scale=0.90)
    # Break up bootprints with realistic grit wear
    boots *= smoothstep(0.14, 0.56, grit * 0.65 + smear_noise * 0.45)

    field = np.maximum(mud_puddle * 0.84, boots * 0.98)
    field = add_droplets(field, x, y, count=68, rng=rng, warp_x=warp_x, warp_y=warp_y,
                         r_min=0.22, r_max=0.88, size_min=0.004, size_max=0.020, stretch=1.6)

    border_fade = 1.0 - smoothstep(0.88, 0.98, np.maximum(np.abs(x), np.abs(y)))
    alpha = np.clip(field * border_fade, 0.0, 1.0)

    # Rich dark wet soil brown vs earthy silt
    wet_mud = np.array([0.072, 0.048, 0.028], dtype=np.float32)
    dry_mud = np.array([0.175, 0.128, 0.078], dtype=np.float32)
    boot_col = np.array([0.045, 0.030, 0.018], dtype=np.float32)

    wet_mask = smoothstep(0.28, 0.75, slush * 0.78 + boots * 0.55)
    rgb = dry_mud * (1.0 - wet_mask[..., None]) + wet_mud * wet_mask[..., None]
    rgb = rgb * (1.0 - boots[..., None] * 0.55) + boot_col * (boots[..., None] * 0.55)
    rgb *= (0.80 + 0.36 * grit[..., None])

    final_alpha = np.clip(alpha * (0.76 + 0.24 * boots), 0.0, 0.96)

    height = np.clip(alpha * 0.36 + boots * 0.36 + grit * 0.26 * alpha, 0.0, 1.0)
    roughness = np.clip(0.64 - 0.46 * wet_mask + 0.14 * grit, 0.14, 0.82)
    wetness = np.clip(wet_mask * alpha, 0.0, 1.0)

    center_weight = 1.0 - smoothstep(0.15, 0.92, r)
    dissolve = np.clip(alpha * (0.38 + 0.34 * boots + 0.28 * center_weight), 0.0, 1.0)

    save_variant("Mud", rgb, final_alpha, height, dissolve, roughness, wetness)


def main():
    build_coffee_spill()
    build_ichor_spill()
    build_mud_spill()
    print("SPILL_TEXTURES_OK", OUT)


if __name__ == "__main__":
    main()
