"""Generate woven fabric PBR textures and author skeletal-mesh customer materials:
  /Game/ShiftStore/Materials/M_CustomerTorso    (slot 1: matte woven jacket/shirt)
  /Game/ShiftStore/Materials/M_CustomerHeadLegs (slot 0: height-zoned shoes, pants, pale skin, dark hair)

Run via:
  UnrealEditor-Cmd.exe ShiftStore.uproject -run=pythonscript -script=Tools/author_customer_materials.py -unattended -NullRHI -nosplash
"""
import math
import struct
import zlib
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "SourceArt" / "Characters" / "Textures"
SRC_DIR.mkdir(parents=True, exist_ok=True)
TEX_DEST = "/Game/ShiftStore/Textures/Characters"
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


def generate_fabric_textures():
    size = 256
    # Deterministic woven textile height field + subtle wrinkle mottling
    heights = [[0.0] * size for _ in range(size)]
    for y in range(size):
        for x in range(size):
            # Fine cross-hatch twill weave
            warp = math.sin(x * math.pi * 0.5) * 0.5 + 0.5
            weft = math.sin(y * math.pi * 0.5) * 0.5 + 0.5
            checker = warp if ((x // 2 + y // 2) % 2 == 0) else weft
            # Low-frequency cloth fold mottling
            fold = (
                math.sin(x * 2.0 * math.pi / size * 3.0 + y * 2.0 * math.pi / size * 1.0) * 0.25
                + math.cos(x * 2.0 * math.pi / size * 2.0 - y * 2.0 * math.pi / size * 4.0) * 0.20
                + math.sin((x + y) * 2.0 * math.pi / size * 7.0) * 0.12
            )
            # Hash micro-grain
            h = ((x * 73856093) ^ (y * 19349663)) & 0xFF
            grain = (h / 255.0 - 0.5) * 0.18
            heights[y][x] = max(0.0, min(1.0, 0.52 + (checker - 0.5) * 0.34 + fold * 0.30 + grain))

    rough_buf = bytearray(size * size * 3)
    norm_buf = bytearray(size * size * 3)
    for y in range(size):
        ym = (y - 1) % size
        yp = (y + 1) % size
        for x in range(size):
            xm = (x - 1) % size
            xp = (x + 1) % size
            v = heights[y][x]
            # Cloth variation map (R = albedo micro-shade 0.80..1.12, G = roughness 0.80..0.96)
            shade = int(max(0, min(255, (0.82 + v * 0.28) * 255)))
            rough = int(max(0, min(255, (0.78 + (1.0 - v) * 0.18) * 255)))
            idx = (y * size + x) * 3
            rough_buf[idx] = shade
            rough_buf[idx + 1] = rough
            rough_buf[idx + 2] = shade

            dx = (heights[y][xp] - heights[y][xm]) * 2.4
            dy = (heights[yp][x] - heights[ym][x]) * 2.4
            nx, ny, nz = -dx, -dy, 1.0
            inv = 1.0 / math.sqrt(nx * nx + ny * ny + nz * nz)
            norm_buf[idx] = int((nx * inv * 0.5 + 0.5) * 255)
            norm_buf[idx + 1] = int((ny * inv * 0.5 + 0.5) * 255)
            norm_buf[idx + 2] = int((nz * inv * 0.5 + 0.5) * 255)

    rough_png = SRC_DIR / "T_CustomerFabric_M.png"
    norm_png = SRC_DIR / "T_CustomerFabric_N.png"
    write_png(rough_png, size, size, bytes(rough_buf))
    write_png(norm_png, size, size, bytes(norm_buf))
    return rough_png, norm_png


def import_tex(png_path: Path, asset_name: str, is_normal: bool):
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
    if is_normal:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    else:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", False)
    u.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def create_material(name: str):
    path = f"{MAT_DEST}/{name}"
    if u.EditorAssetLibrary.does_asset_exist(path):
        mat = u.load_asset(path)
        u.MaterialEditingLibrary.delete_all_material_expressions(mat)
    else:
        factory = u.MaterialFactoryNew()
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(name, MAT_DEST, u.Material, factory)
    try:
        mat.set_editor_property("used_with_skeletal_mesh", True)
    except Exception:
        mat.set_editor_property("bUsedWithSkeletalMesh", True)
    return mat


def expr(mat, cls, x, y, **props):
    e = u.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def conn(src, sp, dst, dp):
    if not u.MaterialEditingLibrary.connect_material_expressions(src, sp, dst, dp):
        raise RuntimeError(f"Connect failed: {src}.{sp} -> {dst}.{dp}")


def conn_prop(src, sp, prop):
    if not u.MaterialEditingLibrary.connect_material_property(src, sp, prop):
        raise RuntimeError(f"ConnectProp failed: {src}.{sp} -> {prop}")


def build_torso_material(fabric_m, fabric_n):
    mat = create_material("M_CustomerTorso")

    uv = expr(mat, u.MaterialExpressionTextureCoordinate, -900, 0, u_tiling=10.0, v_tiling=10.0)
    tex_m = expr(mat, u.MaterialExpressionTextureSample, -620, -120, texture=fabric_m, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    tex_n = expr(mat, u.MaterialExpressionTextureSample, -620, 220, texture=fabric_n, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    conn(uv, "", tex_m, "UVs")
    conn(uv, "", tex_n, "UVs")

    jacket_col = expr(mat, u.MaterialExpressionVectorParameter, -620, -340, parameter_name="JacketColor", default_value=u.LinearColor(0.14, 0.17, 0.22, 1.0))
    mul_col = expr(mat, u.MaterialExpressionMultiply, -300, -240)
    conn(jacket_col, "", mul_col, "A")
    conn(tex_m, "R", mul_col, "B")

    zero_metal = expr(mat, u.MaterialExpressionConstant, -300, 40, r=0.0)
    spec = expr(mat, u.MaterialExpressionConstant, -300, 110, r=0.18)

    conn_prop(mul_col, "", u.MaterialProperty.MP_BASE_COLOR)
    conn_prop(zero_metal, "", u.MaterialProperty.MP_METALLIC)
    conn_prop(spec, "", u.MaterialProperty.MP_SPECULAR)
    conn_prop(tex_m, "G", u.MaterialProperty.MP_ROUGHNESS)
    conn_prop(tex_n, "", u.MaterialProperty.MP_NORMAL)

    u.MaterialEditingLibrary.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    u.log("[CustomerMaterials] Built M_CustomerTorso")


def build_head_legs_material(fabric_m, fabric_n):
    """Height-zoned skeletal material for Manny slot 0 (Head + Legs + Feet):
    Relative Z = WorldPos.Z - ActorPos.Z (capsule center is ~+88 cm above floor):
      Z < -72 cm  -> Shoes (dark matte)
      -72 .. +25  -> Pants (woven cloth)
      +25 .. +80  -> Neck & Face (pale late-night human skin)
      Z > +80 cm  -> Hair (dark matte hair cap)
    """
    mat = create_material("M_CustomerHeadLegs")

    # Compute Z relative to ActorPosition (works reliably on SkeletalMesh with zero skinning drift)
    wpos = expr(mat, u.MaterialExpressionWorldPosition, -1400, -300)
    apos = expr(mat, u.MaterialExpressionActorPositionWS, -1400, -140)
    sub_pos = expr(mat, u.MaterialExpressionSubtract, -1180, -220)
    conn(wpos, "", sub_pos, "A")
    conn(apos, "", sub_pos, "B")
    mask_z = expr(mat, u.MaterialExpressionComponentMask, -1000, -220, r=False, g=False, b=True, a=False)
    conn(sub_pos, "", mask_z, "")

    def make_step(x, y, edge_lo, edge_hi):
        c_lo = expr(mat, u.MaterialExpressionConstant, x - 180, y - 40, r=edge_lo)
        c_hi = expr(mat, u.MaterialExpressionConstant, x - 180, y + 30, r=edge_hi)
        ss = expr(mat, u.MaterialExpressionSmoothStep, x, y)
        conn(c_lo, "", ss, "Min")
        conn(c_hi, "", ss, "Max")
        conn(mask_z, "", ss, "Value")
        return ss

    is_pants = make_step(-720, -420, -74.0, -70.0)
    is_skin = make_step(-720, -220, 42.0, 50.0)
    is_hair = make_step(-720, -20, 79.0, 84.0)

    shoe_col = expr(mat, u.MaterialExpressionVectorParameter, -720, -680, parameter_name="ShoeColor", default_value=u.LinearColor(0.05, 0.05, 0.055, 1.0))
    pants_col = expr(mat, u.MaterialExpressionVectorParameter, -720, -560, parameter_name="PantsColor", default_value=u.LinearColor(0.08, 0.11, 0.16, 1.0))
    skin_col = expr(mat, u.MaterialExpressionVectorParameter, -720, 140, parameter_name="SkinColor", default_value=u.LinearColor(0.62, 0.54, 0.49, 1.0))
    hair_col = expr(mat, u.MaterialExpressionVectorParameter, -720, 320, parameter_name="HairColor", default_value=u.LinearColor(0.045, 0.038, 0.035, 1.0))

    uv = expr(mat, u.MaterialExpressionTextureCoordinate, -1100, 460, u_tiling=10.0, v_tiling=10.0)
    tex_m = expr(mat, u.MaterialExpressionTextureSample, -820, 440, texture=fabric_m, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    tex_n = expr(mat, u.MaterialExpressionTextureSample, -820, 660, texture=fabric_n, sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    conn(uv, "", tex_m, "UVs")
    conn(uv, "", tex_n, "UVs")

    # Apply woven shade to pants
    pants_shaded = expr(mat, u.MaterialExpressionMultiply, -480, -520)
    conn(pants_col, "", pants_shaded, "A")
    conn(tex_m, "R", pants_shaded, "B")

    # Blend Shoe -> Pants -> Skin -> Hair
    l1 = expr(mat, u.MaterialExpressionLinearInterpolate, -320, -440)
    conn(shoe_col, "", l1, "A")
    conn(pants_shaded, "", l1, "B")
    conn(is_pants, "", l1, "Alpha")

    l2 = expr(mat, u.MaterialExpressionLinearInterpolate, -140, -260)
    conn(l1, "", l2, "A")
    conn(skin_col, "", l2, "B")
    conn(is_skin, "", l2, "Alpha")

    l3 = expr(mat, u.MaterialExpressionLinearInterpolate, 40, -140)
    conn(l2, "", l3, "A")
    conn(hair_col, "", l3, "B")
    conn(is_hair, "", l3, "Alpha")

    # Roughness: cloth ~0.88, skin ~0.62, hair ~0.78
    r_skin = expr(mat, u.MaterialExpressionConstant, -320, 260, r=0.62)
    r_hair = expr(mat, u.MaterialExpressionConstant, -320, 340, r=0.78)
    lr1 = expr(mat, u.MaterialExpressionLinearInterpolate, -140, 220)
    conn(tex_m, "G", lr1, "A")
    conn(r_skin, "", lr1, "B")
    conn(is_skin, "", lr1, "Alpha")
    lr2 = expr(mat, u.MaterialExpressionLinearInterpolate, 40, 260)
    conn(lr1, "", lr2, "A")
    conn(r_hair, "", lr2, "B")
    conn(is_hair, "", lr2, "Alpha")

    # Flatten normal on skin so face is smooth instead of woven cloth
    flat_n = expr(mat, u.MaterialExpressionConstant3Vector, -320, 560, constant=u.LinearColor(0.0, 0.0, 1.0, 1.0))
    ln = expr(mat, u.MaterialExpressionLinearInterpolate, -80, 560)
    conn(tex_n, "", ln, "A")
    conn(flat_n, "", ln, "B")
    conn(is_skin, "", ln, "Alpha")

    zero_metal = expr(mat, u.MaterialExpressionConstant, 40, 60, r=0.0)
    spec = expr(mat, u.MaterialExpressionConstant, 40, 140, r=0.22)

    conn_prop(l3, "", u.MaterialProperty.MP_BASE_COLOR)
    conn_prop(zero_metal, "", u.MaterialProperty.MP_METALLIC)
    conn_prop(spec, "", u.MaterialProperty.MP_SPECULAR)
    conn_prop(lr2, "", u.MaterialProperty.MP_ROUGHNESS)
    conn_prop(ln, "", u.MaterialProperty.MP_NORMAL)

    u.MaterialEditingLibrary.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    u.log("[CustomerMaterials] Built M_CustomerHeadLegs")


def main():
    rough_png, norm_png = generate_fabric_textures()
    fabric_m = import_tex(rough_png, "T_CustomerFabric_M", is_normal=False)
    fabric_n = import_tex(norm_png, "T_CustomerFabric_N", is_normal=True)
    build_torso_material(fabric_m, fabric_n)
    build_head_legs_material(fabric_m, fabric_n)


if __name__ == "__main__":
    main()
