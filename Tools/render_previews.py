"""Runs in the full editor, captures engine-rendered images after warm-up frames."""
import unreal as u
from pathlib import Path
import time
import json

u.EditorPythonScripting.set_keep_python_script_alive(True)

ROOT=Path(u.Paths.project_dir())
OUT=ROOT/'Previews'
OUT.mkdir(exist_ok=True)
LEVELS=u.get_editor_subsystem(u.LevelEditorSubsystem)
EAS=u.get_editor_subsystem(u.EditorActorSubsystem)
STEPS=[(state,view) for state in ('Normal','Shift') for view in ('Entrance','Shop','Checkout','Corridor','Stockroom','Office','Restroom')]
STATE={'index':0,'time':time.monotonic(),'stage':'load','map':None,'done':[]}

def tick_body(dt):
    now=time.monotonic()
    i=STATE['index']
    if i>=len(STEPS):
        (OUT/'capture_manifest.json').write_text(json.dumps(STATE['done'],indent=2),encoding='utf-8')
        u.unregister_slate_post_tick_callback(HANDLE)
        u.SystemLibrary.quit_editor()
        return
    state,view=STEPS[i]
    if STATE['stage']=='load':
        changed=STATE['map']!=state
        if changed:
            LEVELS.load_level('/Game/ShiftStore/Maps/'+state)
            STATE['map']=state
        a=next((a for a in EAS.get_all_level_actors() if a.get_actor_label()=='View_'+view),None)
        if a is None:
            pos,yaw={'Office':((1140,1300,166),105),'Restroom':((1410,1280,166),60)}[view]
            a=EAS.spawn_actor_from_class(u.CameraActor,u.Vector(*pos),u.Rotator(yaw=yaw))
            a.set_actor_label('View_'+view)
        u.EditorLevelLibrary.set_level_viewport_camera_info(a.get_actor_location(),a.get_actor_rotation())
        STATE['time']=time.monotonic()
        STATE['wait']=12 if changed else 4
        STATE['stage']='warmup'
    elif STATE['stage']=='warmup' and now-STATE['time']>STATE['wait']:
        filename=OUT/f'{state}_{view}.png'
        u.AutomationLibrary.take_high_res_screenshot(1920,1080,str(filename))
        STATE['done'].append({'state':state,'view':view,'file':str(filename)})
        STATE['time']=now; STATE['stage']='capture'
    elif STATE['stage']=='capture' and now-STATE['time']>4:
        STATE['index']+=1; STATE['stage']='load'

def tick(dt):
    if STATE.get('busy'):
        return
    STATE['busy']=True
    try:
        tick_body(dt)
    except Exception:
        import traceback
        (OUT/'capture_error.txt').write_text(traceback.format_exc(),encoding='utf-8')
        u.unregister_slate_post_tick_callback(HANDLE)
        u.SystemLibrary.quit_editor()
    finally:
        STATE['busy']=False

HANDLE=u.register_slate_post_tick_callback(tick)
