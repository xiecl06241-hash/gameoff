"""Import procedural floor spill textures and author /Game/ShiftStore/Materials/M_FloorSpill.

Run via UnrealEditor-Cmd.exe -run=pythonscript -script=... -unattended -NullRHI -nosplash
"""
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
SRC_DIR = ROOT / "SourceArt" / "Spills"
TEX_DEST = "/Game/ShiftStore/Textures/Spills"
MAT_DEST = "/Game/ShiftStore/Materials"
MAT_PATH = f"{MAT_DEST}/M_FloorSpill"
VARIANTS = ("Coffee", "Ichor", "Mud")


def import_texture(png_path: Path, asset_name: str, kind: str):
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
        raise RuntimeError(f"Failed to import texture: {png_path}")

    if kind == "normal":
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_NORMALMAP)
        tex.set_editor_property("srgb", False)
    elif kind == "mask":
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_MASKS)
        tex.set_editor_property("srgb", False)
    else:
        tex.set_editor_property("compression_settings", u.TextureCompressionSettings.TC_DEFAULT)
        tex.set_editor_property("srgb", True)

    tex.set_editor_property("address_x", u.TextureAddress.TA_CLAMP)
    tex.set_editor_property("address_y", u.TextureAddress.TA_CLAMP)
    u.EditorAssetLibrary.save_loaded_asset(tex)
    return tex


def node(mat, cls, x, y, **props):
    expr = u.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        expr.set_editor_property(k, v)
    return expr


def conn(src, src_pin, dst, dst_pin):
    if not u.MaterialEditingLibrary.connect_material_expressions(src, src_pin, dst, dst_pin):
        raise RuntimeError(f"Failed connecting {src.get_name()}.{src_pin} -> {dst.get_name()}.{dst_pin}")


def conn_prop(src, src_pin, prop):
    if not u.MaterialEditingLibrary.connect_material_property(src, src_pin, prop):
        raise RuntimeError(f"Failed connecting {src.get_name()}.{src_pin} -> {prop}")


def build_material(default_albedo, default_normal, default_mask):
    mat = u.load_asset(MAT_PATH)
    if not mat:
        mat = u.AssetToolsHelpers.get_asset_tools().create_asset(
            "M_FloorSpill", MAT_DEST, u.Material, u.MaterialFactoryNew()
        )
    u.MaterialEditingLibrary.delete_all_material_expressions(mat)

    mat.set_editor_property("material_domain", u.MaterialDomain.MD_DEFERRED_DECAL)
    mat.set_editor_property("blend_mode", u.BlendMode.BLEND_TRANSLUCENT)

    # Texture parameters
    albedo_tex = node(
        mat, u.MaterialExpressionTextureSampleParameter2D, -900, -320,
        parameter_name="AlbedoTex",
        texture=default_albedo,
        sampler_type=u.MaterialSamplerType.SAMPLERTYPE_COLOR,
    )
    normal_tex = node(
        mat, u.MaterialExpressionTextureSampleParameter2D, -900, 40,
        parameter_name="NormalTex",
        texture=default_normal,
        sampler_type=u.MaterialSamplerType.SAMPLERTYPE_NORMAL,
    )
    mask_tex = node(
        mat, u.MaterialExpressionTextureSampleParameter2D, -900, 360,
        parameter_name="MaskTex",
        texture=default_mask,
        sampler_type=u.MaterialSamplerType.SAMPLERTYPE_MASKS,
    )

    # Scalar & Vector parameters
    tint = node(
        mat, u.MaterialExpressionVectorParameter, -640, -480,
        parameter_name="TintColor",
        default_value=u.LinearColor(1.0, 1.0, 1.0, 1.0),
    )
    clean_progress = node(
        mat, u.MaterialExpressionScalarParameter, -900, 680,
        parameter_name="CleanProgress",
        default_value=0.0,
    )
    wetness_scale = node(
        mat, u.MaterialExpressionScalarParameter, -640, 240,
        parameter_name="WetnessScale",
        default_value=1.0,
    )

    # 1. BaseColor = AlbedoTex.RGB * TintColor.RGB
    base_mul = node(mat, u.MaterialExpressionMultiply, -320, -340)
    conn(albedo_tex, "", base_mul, "A")
    conn(tint, "", base_mul, "B")
    conn_prop(base_mul, "", u.MaterialProperty.MP_BASE_COLOR)

    # 2. Normal = NormalTex.RGB
    conn_prop(normal_tex, "", u.MaterialProperty.MP_NORMAL)

    # 3. Roughness = Lerp(Clamp(MaskTex.G / max(WetnessScale, 0.25), 0.04, 0.9), 0.55, CleanProgress * 0.65)
    # Simpler & robust: Lerp(MaskTex.G, 0.55, Clamp(CleanProgress * 0.65 + (1 - WetnessScale) * 0.25, 0, 1))
    dry_rough = node(mat, u.MaterialExpressionConstant, -360, 180, r=0.55)
    clean_dry = node(mat, u.MaterialExpressionMultiply, -560, 260)
    conn(clean_progress, "", clean_dry, "A")
    conn(node(mat, u.MaterialExpressionConstant, -720, 300, r=0.65), "", clean_dry, "B")

    one_minus_wet = node(mat, u.MaterialExpressionOneMinus, -480, 340)
    conn(wetness_scale, "", one_minus_wet, "")
    wet_offset = node(mat, u.MaterialExpressionMultiply, -340, 340)
    conn(one_minus_wet, "", wet_offset, "A")
    conn(node(mat, u.MaterialExpressionConstant, -480, 410, r=0.25), "", wet_offset, "B")

    rough_alpha_add = node(mat, u.MaterialExpressionAdd, -220, 280)
    conn(clean_dry, "", rough_alpha_add, "A")
    conn(wet_offset, "", rough_alpha_add, "B")
    rough_alpha = node(mat, u.MaterialExpressionClamp, -120, 280)
    conn(rough_alpha_add, "", rough_alpha, "")

    rough_lerp = node(mat, u.MaterialExpressionLinearInterpolate, 20, 200)
    conn(mask_tex, "G", rough_lerp, "A")
    conn(dry_rough, "", rough_lerp, "B")
    conn(rough_alpha, "", rough_lerp, "Alpha")
    conn_prop(rough_lerp, "", u.MaterialProperty.MP_ROUGHNESS)

    # 4. Specular = Lerp(0.32, 0.72, MaskTex.B * WetnessScale)
    spec_wet = node(mat, u.MaterialExpressionMultiply, -320, 480)
    conn(mask_tex, "B", spec_wet, "A")
    conn(wetness_scale, "", spec_wet, "B")
    spec_lerp = node(mat, u.MaterialExpressionLinearInterpolate, -100, 460)
    conn(node(mat, u.MaterialExpressionConstant, -260, 420, r=0.32), "", spec_lerp, "A")
    conn(node(mat, u.MaterialExpressionConstant, -260, 520, r=0.72), "", spec_lerp, "B")
    conn(spec_wet, "", spec_lerp, "Alpha")
    conn_prop(spec_lerp, "", u.MaterialProperty.MP_SPECULAR)

    # 5. Dynamic cleaning dissolve Opacity:
    # Edge0 = CleanProgress * 1.15 - 0.22
    # Dissolve = Clamp((MaskTex.R - Edge0) / 0.22, 0, 1)
    # Fade = 1.0 - CleanProgress * 0.92
    # Opacity = Clamp(AlbedoTex.A * Dissolve * Fade, 0, 1)
    prog_scaled = node(mat, u.MaterialExpressionMultiply, -640, 680)
    conn(clean_progress, "", prog_scaled, "A")
    conn(node(mat, u.MaterialExpressionConstant, -800, 760, r=1.15), "", prog_scaled, "B")

    edge0 = node(mat, u.MaterialExpressionSubtract, -480, 680)
    conn(prog_scaled, "", edge0, "A")
    conn(node(mat, u.MaterialExpressionConstant, -640, 780, r=0.22), "", edge0, "B")

    mask_minus_edge = node(mat, u.MaterialExpressionSubtract, -320, 620)
    conn(mask_tex, "R", mask_minus_edge, "A")
    conn(edge0, "", mask_minus_edge, "B")

    dissolve_div = node(mat, u.MaterialExpressionDivide, -180, 620)
    conn(mask_minus_edge, "", dissolve_div, "A")
    conn(node(mat, u.MaterialExpressionConstant, -320, 740, r=0.22), "", dissolve_div, "B")

    dissolve_clamp = node(mat, u.MaterialExpressionClamp, -40, 620)
    conn(dissolve_div, "", dissolve_clamp, "")

    fade_mul = node(mat, u.MaterialExpressionMultiply, -480, 860)
    conn(clean_progress, "", fade_mul, "A")
    conn(node(mat, u.MaterialExpressionConstant, -640, 920, r=0.92), "", fade_mul, "B")
    fade = node(mat, u.MaterialExpressionOneMinus, -320, 860)
    conn(fade_mul, "", fade, "")

    op_a = node(mat, u.MaterialExpressionMultiply, 100, 560)
    conn(albedo_tex, "A", op_a, "A")
    conn(dissolve_clamp, "", op_a, "B")

    op_b = node(mat, u.MaterialExpressionMultiply, 240, 620)
    conn(op_a, "", op_b, "A")
    conn(fade, "", op_b, "B")

    op_final = node(mat, u.MaterialExpressionClamp, 380, 620)
    conn(op_b, "", op_final, "")
    conn_prop(op_final, "", u.MaterialProperty.MP_OPACITY)

    u.MaterialEditingLibrary.recompile_material(mat)
    u.EditorAssetLibrary.save_loaded_asset(mat)
    return mat


def main():
    imported = {}
    for v in VARIANTS:
        imported[v] = {
            "D": import_texture(SRC_DIR / f"T_Spill_{v}_D.png", f"T_Spill_{v}_D", "albedo"),
            "N": import_texture(SRC_DIR / f"T_Spill_{v}_N.png", f"T_Spill_{v}_N", "normal"),
            "M": import_texture(SRC_DIR / f"T_Spill_{v}_M.png", f"T_Spill_{v}_M", "mask"),
        }

    build_material(
        imported["Coffee"]["D"],
        imported["Coffee"]["N"],
        imported["Coffee"]["M"],
    )
    print("SPILL_MATERIAL_READY", MAT_PATH, len(VARIANTS))


if __name__ == "__main__":
    main()
