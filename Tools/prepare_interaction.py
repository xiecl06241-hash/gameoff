"""Add reusable simple collision and movable components without enabling gameplay."""
import unreal as u
from pathlib import Path
import json
r=Path(u.Paths.project_dir())
catalog=json.loads((r/'SourceArt/asset_catalog.json').read_text())
result={}
for name,entry in catalog.items():
    if not(name.startswith('Product_') or name in ('Broom','Mop','Bucket','InteriorDoor','EntranceDoor','FridgeDoor')): continue
    sm=u.load_asset(entry['asset'])
    dm=u.DynamicMesh()
    u.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(sm,dm,u.GeometryScriptCopyMeshFromAssetOptions(),u.GeometryScriptMeshReadLOD())
    opts=u.GeometryScriptCollisionFromMeshOptions(method=u.GeometryScriptCollisionGenerationMethod.ALIGNED_BOXES)
    u.GeometryScript_Collision.set_static_mesh_collision_from_mesh(dm,sm,opts)
    sm.get_editor_property('body_setup').set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    u.EditorAssetLibrary.save_loaded_asset(sm)
    entry['collision_type']='simple_boxes_and_complex'
    shapes=u.GeometryScript_Collision.get_simple_collision_from_static_mesh(sm)
    result[name]={'simple_collision_count':u.GeometryScript_Collision.get_simple_collision_shape_count(shapes),'trace_mode':str(sm.get_editor_property('body_setup').get_editor_property('collision_trace_flag'))}
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
for state in ('Normal','Shift'):
    levels.load_level('/Game/ShiftStore/Maps/'+state)
    for a in actors.get_all_level_actors():
        if isinstance(a,u.StaticMeshActor) and any(str(t) in ('Interactive.Grabbable','Interactive.Tool','Interactive.Door','Interactive.FridgeDoor') for t in a.tags):
            a.static_mesh_component.set_mobility(u.ComponentMobility.MOVABLE)
    levels.save_current_level()
(r/'SourceArt/asset_catalog.json').write_text(json.dumps(catalog,indent=2))
(r/'Saved/interaction_checks.json').write_text(json.dumps(result,indent=2))
assert len(result)==18 and all(v['simple_collision_count']>0 for v in result.values())
