import unreal as u
from pathlib import Path
import json

out = Path(u.Paths.project_dir()) / "Saved/probe.txt"
lines = []
for name in ("GeometryScript_Primitives", "GeometryScript_NewAssetUtils", "GeometryScript_MeshModeling", "GeometryScript_MeshNormals", "GeometryScript_Collision", "GeometryScriptMeshBevelOptions"):
    obj = getattr(u, name, None)
    lines.append(name + " = " + str(obj))
    if obj:
        lines.append(str([x for x in dir(obj) if not x.startswith('_')]))
mesh = u.DynamicMesh()
u.GeometryScript_Primitives.append_box(mesh, u.GeometryScriptPrimitiveOptions(), u.Transform(), 100, 100, 100)
opt = u.GeometryScriptCreateNewStaticMeshAssetOptions()
opt.set_editor_property("enable_collision", True)
opt.set_editor_property("collision_mode", u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
res = u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(mesh, "/Game/ShiftStore/Probe/SM_Box", opt)
lines.append("create result: " + str(res))
lines.append("material classes: " + str([x for x in dir(u) if 'MaterialExpressionNoise' in x]))
out.parent.mkdir(exist_ok=True, parents=True)
out.write_text('\n'.join(lines), encoding='utf-8')
u.EditorAssetLibrary.save_directory('/Game/ShiftStore', only_if_is_dirty=False, recursive=True)
u.log('SHIFT_PROBE_OK')
