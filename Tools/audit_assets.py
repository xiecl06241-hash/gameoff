import unreal as u
from pathlib import Path
import json
r={}
for key in ('Water','Soda','Chips'):
    m=u.load_asset('/Game/ShiftStore/Materials/M_Print_'+key)
    mesh=u.load_asset('/Game/ShiftStore/Meshes/SM_Product_'+key)
    r[key]={'textures':[t.get_path_name() for t in u.MaterialEditingLibrary.get_used_textures(m)],'slots':[str(s.material_interface) for s in mesh.static_materials]}
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/ShiftStore/Maps/Normal')
for a in u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() in ('View_Entrance','WalkthroughStart','MainLight_0_1','StoreColorAndExposure'):
        r[a.get_actor_label()]={'location':str(a.get_actor_location()),'rotation':str(a.get_actor_rotation())}
        if isinstance(a,u.PostProcessVolume):
            s=a.settings
            r[a.get_actor_label()]['exposure']={'bias':s.auto_exposure_bias,'method':str(s.auto_exposure_method),'physical':s.auto_exposure_apply_physical_camera_exposure}
(Path(u.Paths.project_dir())/'Saved/asset_audit.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
