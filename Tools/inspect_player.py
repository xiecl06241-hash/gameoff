import unreal as u
from pathlib import Path
import json
b=u.load_asset('/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter')
c=u.get_default_object(b.generated_class())
cap=c.get_component_by_class(u.CapsuleComponent)
cams=c.get_components_by_class(u.CameraComponent)
r={'capsule_radius':cap.get_unscaled_capsule_radius(),'capsule_half_height':cap.get_unscaled_capsule_half_height(),'cameras':[{'name':x.get_name(),'relative_location':str(x.get_editor_property('relative_location')),'parent':str(x.get_attach_parent())} for x in cams]}
(Path(u.Paths.project_dir())/'Saved/player_dimensions.json').write_text(json.dumps(r,indent=2))
