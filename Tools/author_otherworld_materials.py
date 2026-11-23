"""Generate procedural weathered/molded/blood-stained Chinese-horror Otherworld PBR textures
and author 3 StaticMesh materials for instant Normal <-> Otherworld architectural transformation:
  /Game/ShiftStore/Materials/M_OtherworldWall
  /Game/ShiftStore/Materials/M_OtherworldFloor
  /Game/ShiftStore/Materials/M_OtherworldCeiling

Run via:
  UnrealEditor-Cmd.exe ShiftStore.uproject -run=pythonscript -script=Tools/author_otherworld_materials.py -unattended -NullRHI -nosplash
"""
import math
import struct
import zlib
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "SourceArt" / "Otherworld"
SRC_DIR.mkdir(parents=True, exist_ok=True)
TEX_DEST = "/Game/ShiftStore/Textures/Otherworld"
MAT_DEST = "/Game/ShiftStore/Materials"


def write_png(path: Path, width: int, height: int, rgb_bytes: bytes):
    def chunk(tag: bytes, data: bytes) -> bytes:
        crc = zlib.crc32(tag + data) & 0xFFFFFFFF
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", crc)

    raw = bytearray()
    stride = width * 3
    for y in range(height):
        raw.append(0)
        raw.extend(rgb_bytes[y * stride : (y + 1) * stride])
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    path.write_bytes(png)


def fbm(x, y, period):
    # Seamless tileable pseudo-fBm using harmonic waves
    val = 0.0
    amp = 0.5
    for freq, phx, phy in ((1, 0.3, 1.1), (2, 1.7, 0.5), (4, 2.9, 2.1), (8, 0.8, 3.7)):
        ang_x = 2.0 * math.pi * x / period * freq
        ang_y = 2.0 * math.pi * y / period * freq
        n = math.sin(ang_x + phx) * math.cos(ang_y + phy) + 0.5 * math.sin(ang_x + ang_y + phx * 1.3)
        val += n * amp
        amp *= 0.52
    return max(0.0, min(1.0, 0.5 + val * 0.42))


def clamp255(v):
    return int(max(0, min(255, round(v * 255.0))))


def generate_wall_textures():
    """Decayed greenish-grey peeling plaster wall with vertical dark blood/rust seepage streaks."""
    size = 256
    alb = bytearray(size * size * 3)
    rgh = bytearray(size * size * 3)
    nrm = bytearray(size * size * 3)
    hfield = [[0.0] * size for _ in range(size)]

    for y in range(size):
        vy = y / size
        for x in range(size):
            n = fbm(x, y, size)
            n2 = fbm(x * 2, y * 2, size)
            # Vertical seepage streaks dripping from top
            streak_wave = math.sin(2.0 * math.pi * x / size * 5.0 + 0.7) * 0.5 + 0.5
            streak_fine = math.sin(2.0 * math.pi * x / size * 13.0 + n * 2.5) * 0.5 + 0.5
            drip = max(0.0, (streak_wave * streak_fine - 0.36) * 2.2) * (1.0 - vy * 0.55)
            # Dark mold patches near bottom & top edges
            mold = max(0.0, (n - 0.46) * 2.1)
            peel = 1.0 if n2 > 0.66 else 0.0

            # Base sickly aged plaster (dark muted jade-ochre)
            r = 0.16 + n * 0.11 - mold * 0.07 - peel * 0.05
            g = 0.18 + n * 0.12 - mold * 0.05 - peel * 0.06
            b = 0.14 + n * 0.08 - mold * 0.06 - peel * 0.04

            # Blend dark dried crimson/rust seepage streaks
            if drip > 0.02:
                k = min(0.85, drip)
                r = r * (1.0 - k) + 0.28 * k
                g = g * (1.0 - k) + 0.03 * k
                b = b * (1.0 - k) + 0.03 * k

            idx = (y * size + x) * 3
            alb[idx] = clamp255(r)
            alb[idx + 1] = clamp255(g)
            alb[idx + 2] = clamp255(b)

            rough = max(0.28, min(0.95, 0.86 - drip * 0.45 + peel * 0.08))
            rgh[idx] = clamp255(rough)
            rgh[idx + 1] = clamp255(rough)
            rgh[idx + 2] = clamp255(rough)
            hfield[y][x] = n * 0.5 + n2 * 0.35 - peel * 0.25

    for y in range(size):
        ym, yp = (y - 1) % size, (y + 1) % size
        for x in range(size):
            xm, xp = (x - 1) % size, (x + 1) % size
            dx = (hfield[y][xp] - hfield[y][xm]) * 3.0
            dy = (hfield[yp][x] - hfield[ym][x]) * 3.0
            nx, ny, nz = -dx, -dy, 1.0
            inv = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
            idx = (y * size + x) * 3
            nrm[idx] = clamp255(nx * inv * 0.5 + 0.5)
            nrm[idx + 1] = clamp255(ny * inv * 0.5 + 0.5)
            nrm[idx + 2] = clamp255(nz * inv * 0.5 + 0.5)

    p_d = SRC_DIR / "T_OtherworldWall_D.png"
    p_r = SRC_DIR / "T_OtherworldWall_R.png"
    p_n = SRC_DIR / "T_OtherworldWall_N.png"
    write_png(p_d, size, size, bytes(alb))
    write_png(p_r, size, size, bytes(rgh))
    write_png(p_n, size, size, bytes(nrm))
    return p_d, p_r, p_n


def generate_floor_textures():
    """Grimy cracked dark old floor tiles with wet blood-darkened puddles reflecting red lanterns."""
    size = 256
    alb = bytearray(size * size * 3)
    rgh = bytearray(size * size * 3)
    nrm = bytearray(size * size * 3)
    hfield = [[0.0] * size for _ in range(size)]
    tile = 64

    for y in range(size):
        for x in range(size):
            n = fbm(x, y, size)
            n2 = fbm(x * 3, y * 3, size)
            gx = x % tile
            gy = y % tile
            grout = 1.0 if (gx < 3 or gy < 3) else 0.0
            puddle = max(0.0, (n - 0.56) * 2.8)
            blood_tint = max(0.0, (n2 - 0.58) * 2.4)

            r = 0.09 + n * 0.07 - grout * 0.05
            g = 0.10 + n * 0.07 - grout * 0.05
            b = 0.09 + n * 0.06 - grout * 0.04
            if blood_tint > 0.05:
                k = min(0.75, blood_tint)
                r = r * (1.0 - k) + 0.20 * k
                g = g * (1.0 - k) + 0.025 * k
                b = b * (1.0 - k) + 0.025 * k

            idx = (y * size + x) * 3
            alb[idx] = clamp255(r)
            alb[idx + 1] = clamp255(g)
            alb[idx + 2] = clamp255(b)

            # Wet reflective puddles (roughness ~0.16) vs gritty dry tiles (~0.84)
            rough = max(0.14, min(0.92, 0.84 - puddle * 0.68 + grout * 0.10))
            rgh[idx] = clamp255(rough)
            rgh[idx + 1] = clamp255(rough)
            rgh[idx + 2] = clamp255(rough)
            hfield[y][x] = (1.0 - grout) * (0.5 + n2 * 0.3)

    for y in range(size):
        ym, yp = (y - 1) % size, (y + 1) % size
        for x in range(size):
            xm, xp = (x - 1) % size, (x + 1) % size
            dx = (hfield[y][xp] - hfield[y][xm]) * 2.8
            dy = (hfield[yp][x] - hfield[ym][x]) * 2.8
            nx, ny, nz = -dx, -dy, 1.0
            inv = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
            idx = (y * size + x) * 3
            nrm[idx] = clamp255(nx * inv * 0.5 + 0.5)
            nrm[idx + 1] = clamp255(ny * inv * 0.5 + 0.5)
            nrm[idx + 2] = clamp255(nz * inv * 0.5 + 0.5)

    p_d = SRC_DIR / "T_OtherworldFloor_D.png"
    p_r = SRC_DIR / "T_OtherworldFloor_R.png"
    p_n = SRC_DIR / "T_OtherworldFloor_N.png"
    write_png(p_d, size, size, bytes(alb))
    write_png(p_r, size, size, bytes(rgh))
    write_png(p_n, size, size, bytes(nrm))
    return p_d, p_r, p_n


def import_tex(png_path: Path, asset_name: str, mode: str):
    task = u.AssetImportTask()
    task.filename = str(png_path)
    task.destination_path = TEX_DEST
    task.destination_name = asset_name
    task.automated = True
    task.replace_existing = True
    task.save = True
    u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    tex = u.load_asset(f"{TEX_DEST}/{asset_name}")
    if not tex:
        raise RuntimeError(f"Failed to import {png_path}")
    if mode == "normal":
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif mode == "rough":
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", False)
    else:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", True)
    u.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def build_pbr_mat(mat_name: str, tex_d, tex_r, tex_n, tiling: float, tint_rgb=(1.0, 1.0, 1.0)):
    path = f"{MAT_DEST}/{mat_name}"
    if u.EditorAssetLibrary.does_asset_exist(path):
        mat = u.load_asset(path)
        u.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        factory = u.MaterialFactoryNew()
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(mat_name, MAT_DEST, u.Material, factory)

    def expr(cls, x, y, **props):
        e = u.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        return e

    def conn(s, sp, d, dp):
        if not u.MaterialEditingLibrary.connect_material_expressions(s, sp, d, dp):
            raise RuntimeError(f"Connect failed {s}.{sp} -> {d}.{dp}")

    def cprop(s, sp, prop):
        if not u.MaterialEditingLibrary.connect_material_property(s, sp, prop):
            raise RuntimeError(f"ConnectProp failed {s}.{sp} -> {prop}")

    uv = expr(u.MaterialExpressionTextureCoordinate, -820, 0, u_tiling=tiling, v_tiling=tiling)
    sd = expr(u.MaterialExpressionTextureSample, -520, -240, texture=tex_d, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_COLOR)
    sr = expr(u.MaterialExpressionTextureSample, -520, 40, texture=tex_r, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    sn = expr(u.MaterialExpressionTextureSample, -520, 300, texture=tex_n, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    conn(uv, "", sd, "UVs")
    conn(uv, "", sr, "UVs")
    conn(uv, "", sn, "UVs")

    tint = expr(u.MaterialExpressionConstant3Vector, -520, -420, constant=u.LinearColor(tint_rgb[0], tint_rgb[1], tint_rgb[2], 1.0))
    mul = expr(u.MaterialExpressionMultiply, -220, -240)
    conn(sd, "", mul, "A")
    conn(tint, "", mul, "B")

    metal = expr(u.MaterialExpressionConstant, -220, 120, r=0.02)
    cprop(mul, "", u.MaterialProperty.MP_BASE_COLOR)
    cprop(metal, "", u.MaterialProperty.MP_METALLIC)
    cprop(sr, "R", u.MaterialProperty.MP_ROUGHNESS)
    cprop(sn, "", u.MaterialProperty.MP_NORMAL)

    u.MaterialEditingLibrary.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    u.log(f"[OtherworldMaterials] Built {mat_name}")


def main():
    wd, wr, wn = generate_wall_textures()
    fd, fr, fn = generate_floor_textures()

    tw_d = import_tex(wd, "T_OtherworldWall_D", "albedo")
    tw_r = import_tex(wr, "T_OtherworldWall_R", "rough")
    tw_n = import_tex(wn, "T_OtherworldWall_N", "normal")

    tf_d = import_tex(fd, "T_OtherworldFloor_D", "albedo")
    tf_r = import_tex(fr, "T_OtherworldFloor_R", "rough")
    tf_n = import_tex(fn, "T_OtherworldFloor_N", "normal")

    build_pbr_mat("M_OtherworldWall", tw_d, tw_r, tw_n, tiling=2.5, tint_rgb=(1.0, 1.0, 1.0))
    build_pbr_mat("M_OtherworldFloor", tf_d, tf_r, tf_n, tiling=2.0, tint_rgb=(1.0, 1.0, 1.0))
    build_pbr_mat("M_OtherworldCeiling", tw_d, tw_r, tw_n, tiling=2.0, tint_rgb=(0.45, 0.42, 0.40))


if __name__ == "__main__":
    main()
