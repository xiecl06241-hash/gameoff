import unreal as u
from pathlib import Path
import time
u.EditorPythonScripting.set_keep_python_script_alive(True)
root=Path(u.Paths.project_dir())
u.get_editor_subsystem(u.LevelEditorSubsystem).load_level('/Game/ShiftStore/Maps/Normal')
aa=u.get_editor_subsystem(u.EditorActorSubsystem).get_all_level_actors()
p=next(a for a in aa if a.get_actor_label()=='StoreColorAndExposure')
c=next(a for a in aa if a.get_actor_label()=='View_Shop')
u.EditorLevelLibrary.set_level_viewport_camera_info(c.get_actor_location(),c.get_actor_rotation())
s={'i':0,'time':time.monotonic(),'stage':0,'busy':False}
biases=[-.5,1.5,3.5,5.5]
def tick(dt):
    if s['busy']: return
    s['busy']=True
    try:
        if time.monotonic()-s['time']<4: return
        if s['i']==len(biases):
            u.unregister_slate_post_tick_callback(handle)
            u.SystemLibrary.quit_editor()
            return
        b=biases[s['i']]
        if s['stage']==0:
            settings=p.settings
            settings.set_editor_property('auto_exposure_bias',b)
            p.set_editor_property('settings',settings)
            s['stage']=1
        else:
            u.AutomationLibrary.take_high_res_screenshot(1280,720,str(root/'Saved'/f'Calibration_{b}.png'))
            s['i']+=1;s['stage']=0
        s['time']=time.monotonic()
    finally: s['busy']=False
handle=u.register_slate_post_tick_callback(tick)
