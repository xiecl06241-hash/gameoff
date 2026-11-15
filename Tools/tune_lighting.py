import unreal as u
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
mat=u.load_asset('/Game/ShiftStore/Materials/M_ColdLight')
emission=u.MaterialEditingLibrary.get_material_property_input_node(mat,u.MaterialProperty.MP_EMISSIVE_COLOR)
emission.set_editor_property('constant',u.LinearColor(66,92,86,1))
u.MaterialEditingLibrary.recompile_material(mat)
u.EditorAssetLibrary.save_loaded_asset(mat)
for state,iso,shutter in [('Normal',100,60),('Shift',1600,30)]:
    levels.load_level('/Game/ShiftStore/Maps/'+state)
    pp=next(a for a in actors.get_all_level_actors() if a.get_actor_label()=='StoreColorAndExposure')
    settings=pp.settings
    for k,v in dict(auto_exposure_bias=0,override_auto_exposure_apply_physical_camera_exposure=True,auto_exposure_apply_physical_camera_exposure=True,override_camera_iso=True,camera_iso=iso,override_camera_shutter_speed=True,camera_shutter_speed=shutter,override_depth_of_field_fstop=True,depth_of_field_fstop=2).items():
        settings.set_editor_property(k,v)
    pp.set_editor_property('settings',settings)
    levels.save_current_level()
