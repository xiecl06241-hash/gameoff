"""Author the original unlit guide material, including packaged instanced-mesh support. Run in UE Python."""
import unreal as u

PATH = '/Game/ShiftStore/Materials/M_Guide'
material = u.load_asset(PATH)
if not material:
    material = u.AssetToolsHelpers.get_asset_tools().create_asset(
        'M_Guide', '/Game/ShiftStore/Materials', u.Material, u.MaterialFactoryNew())
u.MaterialEditingLibrary.delete_all_material_expressions(material)
material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
u.MaterialEditingLibrary.set_material_usage(material, u.MaterialUsage.MATUSAGE_INSTANCED_STATIC_MESHES)
color = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionVectorParameter, -400, 0)
color.set_editor_property('parameter_name', 'GuideColor')
color.set_editor_property('default_value', u.LinearColor(1.0, 0.72, 0.10, 1.0))
brightness = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionScalarParameter, -400, 160)
brightness.set_editor_property('parameter_name', 'Brightness')
brightness.set_editor_property('default_value', 3.0)
multiply = u.MaterialEditingLibrary.create_material_expression(material, u.MaterialExpressionMultiply, -160, 0)
u.MaterialEditingLibrary.connect_material_expressions(color, '', multiply, 'A')
u.MaterialEditingLibrary.connect_material_expressions(brightness, '', multiply, 'B')
u.MaterialEditingLibrary.connect_material_property(multiply, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
u.MaterialEditingLibrary.recompile_material(material)
u.EditorAssetLibrary.save_loaded_asset(material)
print('GUIDE_MATERIAL_READY', PATH)
