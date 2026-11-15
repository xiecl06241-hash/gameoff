"""Editor checks against the baked assets and actual collision world, not generator inputs."""
import unreal as u
from pathlib import Path
import json
import traceback

ROOT=Path(u.Paths.project_dir())
BASE='/Game/ShiftStore'
levels=u.get_editor_subsystem(u.LevelEditorSubsystem)
actors=u.get_editor_subsystem(u.EditorActorSubsystem)
report={'maps':{},'errors':[]}
player=u.get_default_object(u.load_asset('/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter').generated_class())
cap=player.get_component_by_class(u.CapsuleComponent)
RADIUS=cap.get_unscaled_capsule_radius()
HALF_HEIGHT=cap.get_unscaled_capsule_half_height()
report['player_capsule_cm']={'radius':RADIUS,'half_height':HALF_HEIGHT}

def capsule(world,a,b):
    hit=u.SystemLibrary.capsule_trace_single(world,u.Vector(a[0],a[1],HALF_HEIGHT+4),u.Vector(b[0],b[1],HALF_HEIGHT+4),RADIUS,HALF_HEIGHT,u.TraceTypeQuery.ECC_VISIBILITY,False,[],u.DrawDebugTrace.NONE,True)
    if hit is not None:
        return str(hit)
    return None

paths={
    'entrance':[(800,-90),(800,280)],
    'left_aisle':[(600,280),(600,920)],
    'right_aisle':[(1000,280),(1000,920)],
    'rear_sales':[(600,900),(1455,900)],
    'corridor_entry':[(1455,900),(1455,1125)],
    'back_corridor':[(100,1125),(1500,1125)],
    'warehouse':[(395,1125),(395,1380)],
    'office':[(1035,1125),(1035,1330),(1170,1370)],
    'restroom':[(1435,1125),(1435,1420)],
}
try:
    report['interactive_mesh_collision']={}
    catalog=json.loads((ROOT/'SourceArt/asset_catalog.json').read_text(encoding='utf-8'))
    for key,entry in catalog.items():
        if entry.get('collision_type')!='simple_boxes_and_complex': continue
        sm=u.load_asset(entry['asset'])
        count=u.GeometryScript_Collision.get_simple_collision_shape_count(u.GeometryScript_Collision.get_simple_collision_from_static_mesh(sm))
        mode=sm.get_editor_property('body_setup').get_editor_property('collision_trace_flag')
        report['interactive_mesh_collision'][key]={'simple_shapes':count,'mode':str(mode)}
        if count<1 or mode!=u.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX:
            report['errors'].append('Invalid dynamic collision '+key)
    report['printed_materials']={}
    for key in ('Water','Milk','Soda','Noodles','Chips','Biscuits','Can','Bread','Tissue','Meal','Sauce','Cleaner'):
        mat=u.load_asset(BASE+'/Materials/M_Print_'+key)
        root=u.MaterialEditingLibrary.get_material_property_input_node(mat,u.MaterialProperty.MP_BASE_COLOR)
        valid=isinstance(root,u.MaterialExpressionTextureSample) and root.texture is not None
        usage=mat.get_editor_property('used_with_instanced_static_meshes')
        report['printed_materials'][key]={'connected_texture':valid,'supports_instancing':usage}
        if not valid or not usage:
            report['errors'].append('Invalid printed material '+key)
    for state in ('Normal','Shift'):
        levels.load_level(BASE+'/Maps/'+state)
        world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
        all_actors=actors.get_all_level_actors()
        lookup={a.get_actor_label():a for a in all_actors}
        checks={'actor_count':len(all_actors),'paths':{},'missing_materials':[],'empty_meshes':[],'gondolas':{}}
        checks['interactive_actor_mobility']={}
        for a in all_actors:
            if isinstance(a,u.StaticMeshActor) and any(str(t) in ('Interactive.Grabbable','Interactive.Tool','Interactive.Door','Interactive.FridgeDoor') for t in a.tags):
                valid=a.static_mesh_component.mobility==u.ComponentMobility.MOVABLE
                checks['interactive_actor_mobility'][a.get_actor_label()]=valid
                if not valid: report['errors'].append(state+': static interactive actor '+a.get_actor_label())
            for c in a.get_components_by_class(u.StaticMeshComponent):
                if c.static_mesh:
                    # Commandlet editor worlds may not have created collision state on first map load.
                    enabled=c.get_collision_enabled()
                    if enabled!=u.CollisionEnabled.NO_COLLISION:
                        c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
                        c.set_collision_enabled(enabled)
                    for i in range(c.get_num_materials()):
                        if c.get_material(i) is None:
                            checks['missing_materials'].append(a.get_actor_label()+':'+str(i))
                else:
                    checks['empty_meshes'].append(a.get_actor_label())
        checks['collision_negative_control']=capsule(world,(400,280),(400,650)) is not None
        # Re-evaluate after refreshing component physics state above.
        report['errors']=[e for e in report['errors'] if e!=state+': collision negative control did not hit shelf']
        if not checks['collision_negative_control']:
            report['errors'].append(state+': collision negative control did not hit shelf')
        for key,points in paths.items():
            results=[]
            for a,b in zip(points,points[1:]):
                blocked=capsule(world,a,b)
                results.append({'from':a,'to':b,'clear':blocked is None,'hit':blocked})
                if blocked:
                    report['errors'].append(state+': blocked '+key)
            checks['paths'][key]=results
        # Measure aisle widths from the generated mesh bounds.
        for row in (0,1):
            boxes=[]
            for col in (0,1,2):
                a=lookup[f'Gondola_{row}_{col}']
                center,extent=a.get_actor_bounds(False)
                boxes.append((center.x-extent.x,center.x+extent.x))
                checks['gondolas'][a.get_actor_label()]=list(a.get_actor_location().to_tuple())
            gaps=[boxes[i+1][0]-boxes[i][1] for i in (0,1)]
            checks[f'row_{row}_aisles_cm']=gaps
            minimum=160 if state=='Normal' else 120
            if min(gaps)<minimum-.1:
                report['errors'].append(state+f': aisle under {minimum}cm')
        if checks['missing_materials'] or checks['empty_meshes']:
            report['errors'].append(state+': invalid mesh/material assignment')
        checks['hero_props']={key:list(lookup['Hero_'+key].get_actor_location().to_tuple()) for key in ('Water','Milk','Noodles','Chips')}
        checks['cold_lamps']={a.get_actor_label():a.point_light_component.intensity for a in all_actors if a.get_actor_label().startswith('ColdLamp')}
        report['maps'][state]=checks
    levels.load_level(BASE+'/Maps/Normal')
    restored={a.get_actor_label():a for a in actors.get_all_level_actors()}
    report['round_trip_restores_normal']=all(
        list(restored[name].get_actor_location().to_tuple())==pos
        for name,pos in report['maps']['Normal']['gondolas'].items()
    )
    if not report['round_trip_restores_normal']:
        report['errors'].append('Normal map did not restore original shelf positions')
except Exception:
    report['errors'].append(traceback.format_exc())
(ROOT/'Saved/validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
u.log('SHIFT_VALIDATION '+json.dumps({'errors':report['errors'],'maps':list(report['maps'])}))
if report['errors']:
    raise RuntimeError('Scene validation failed; see Saved/validation.json')
