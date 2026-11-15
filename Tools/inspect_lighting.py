import unreal as u
from pathlib import Path
import json
r={}
for name in ('ColdLight','White'):
    m=u.load_asset('/Game/ShiftStore/Materials/M_'+name)
    n=u.MaterialEditingLibrary.get_material_property_input_node(m,u.MaterialProperty.MP_EMISSIVE_COLOR)
    r[name]={'emission':str(n.get_editor_property('constant')) if n else None}
sm=u.load_asset('/Game/ShiftStore/Meshes/SM_LightFixture')
r['fixture_materials']=[str(s.material_interface.get_name()) for s in sm.static_materials]
r['fixture_sections']=[sm.get_material_index('ColdLight')]
l=u.get_editor_subsystem(u.LevelEditorSubsystem)
a=u.get_editor_subsystem(u.EditorActorSubsystem)
for state in ('Normal','Shift'):
    l.load_level('/Game/ShiftStore/Maps/'+state)
    p=next(x for x in a.get_all_level_actors() if x.get_actor_label()=='StoreColorAndExposure')
    r[state]={k:str(p.settings.get_editor_property(k)) for k in ('auto_exposure_bias','override_auto_exposure_bias','auto_exposure_method','override_auto_exposure_method','auto_exposure_apply_physical_camera_exposure')}
(Path(u.Paths.project_dir())/'Saved/lighting_audit.json').write_text(json.dumps(r,indent=2))
