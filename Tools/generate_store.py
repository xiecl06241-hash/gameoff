"""UE editor-only deterministic authoring. All generated assets are baked for runtime.

Run through UnrealEditor-Cmd -run=pythonscript -script=.../generate_store.py.
Units are centimeters. Coordinate convention: X east, Y into the store, Z up.
"""
import unreal as u
from pathlib import Path
import json
import math
import random
import traceback

ROOT = Path(u.Paths.project_dir())
BASE = '/Game/ShiftStore'
TOOLS = u.AssetToolsHelpers.get_asset_tools()
EAS = u.get_editor_subsystem(u.EditorActorSubsystem)
LEVELS = u.get_editor_subsystem(u.LevelEditorSubsystem)
MATS, ASSETS, CATALOG = {}, {}, {}
ACTORS, INSTANCES, STATES = {}, {}, {}
RNG = random.Random(2026)
P = u.GeometryScript_Primitives

def log(msg):
    u.log('SHIFTSTORE: ' + msg)

def asset(name, folder, cls, factory):
    path = f'{BASE}/{folder}/{name}'
    obj = u.load_asset(path)
    return obj or TOOLS.create_asset(name, f'{BASE}/{folder}', cls, factory)

def node(mat, cls, **props):
    n = u.MaterialEditingLibrary.create_material_expression(mat, cls)
    n.set_editor_properties(props)
    return n

def constant(mat, value):
    if isinstance(value, (list, tuple)):
        return node(mat, u.MaterialExpressionConstant3Vector, constant=u.LinearColor(*value, 1))
    return node(mat, u.MaterialExpressionConstant, r=float(value))

def connect(n, mat, prop):
    if not u.MaterialEditingLibrary.connect_material_property(n, '', prop):
        raise RuntimeError(f'Material connection failed: {mat.get_name()} {prop}')

def material(name, color, rough=.6, metal=0, emissive=0, style='plain', opacity=None):
    mat = asset('M_' + name, 'Materials', u.Material, u.MaterialFactoryNew())
    u.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    if name == 'Charcoal':
        mat.set_editor_property('used_with_skeletal_mesh', True)
    mat.set_editor_property('two_sided', False)
    if style != 'plain':
        world = node(mat, u.MaterialExpressionWorldPosition)
        custom = node(mat, u.MaterialExpressionCustom)
        inp = u.CustomInput()
        inp.set_editor_property('input_name','P')
        custom.set_editor_property('inputs', [inp])
        custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
        c = ','.join(str(v) for v in color)
        common = f'float3 C=float3({c}); float h=frac(sin(dot(floor(P*3),float3(12.9898,78.233,37.719)))*43758.5453); '
        if style == 'floor':
            code = 'float2 t=frac(P.xy/60); float g=step(min(min(t.x,1-t.x),min(t.y,1-t.y)),0.013); float fleck=step(.976,h); return lerp(C*(.90+.12*h)-fleck*.09,float3(.20,.23,.22),g*.5);'
        elif style == 'wall':
            code = 'float n=frac(sin(dot(floor(P*.38),float3(5.3,7.7,3.3)))*7541.13); return C*(.96+.05*n);'
        elif style == 'wood':
            code = 'float v=sin(P.x*.38+sin(P.z*.04)*3)*.04; return C*(.94+v+.05*h);'
        elif style == 'metal':
            code = 'float s=sin(P.z*45)*.015; return C*(.95+s+.04*h);'
        else:
            code = 'return C*(.94+.08*h);'
        custom.set_editor_property('code', common + code)
        u.MaterialEditingLibrary.connect_material_expressions(world, '', custom, 'P')
        connect(custom, mat, u.MaterialProperty.MP_BASE_COLOR)
    else:
        connect(constant(mat, color), mat, u.MaterialProperty.MP_BASE_COLOR)
    connect(constant(mat, rough), mat, u.MaterialProperty.MP_ROUGHNESS)
    connect(constant(mat, metal), mat, u.MaterialProperty.MP_METALLIC)
    if emissive:
        connect(constant(mat, [v*emissive for v in color]), mat, u.MaterialProperty.MP_EMISSIVE_COLOR)
    if opacity is not None:
        mat.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
        mat.set_editor_property('two_sided', True)
        connect(constant(mat, opacity), mat, u.MaterialProperty.MP_OPACITY)
    u.MaterialEditingLibrary.recompile_material(mat)
    MATS[name] = mat
    return mat

def label_material(name, emissive=False):
    task = u.AssetImportTask()
    task.set_editor_properties(dict(filename=str(ROOT/'SourceArt/Labels'/f'{name}.png'), destination_path=BASE+'/Textures', automated=True, replace_existing=True, save=True))
    TOOLS.import_asset_tasks([task])
    texture = u.load_asset(f'{BASE}/Textures/{name}')
    mat = asset('M_Print_' + name, 'Materials', u.Material, u.MaterialFactoryNew())
    u.MaterialEditingLibrary.delete_all_material_expressions(mat)
    mat.set_editor_property('used_with_instanced_static_meshes',True)
    sample = node(mat, u.MaterialExpressionTextureSample, texture=texture)
    connect(sample,mat,u.MaterialProperty.MP_BASE_COLOR)
    if emissive:
        connect(sample,mat,u.MaterialProperty.MP_EMISSIVE_COLOR)
    connect(constant(mat, .55), mat, u.MaterialProperty.MP_ROUGHNESS)
    u.MaterialEditingLibrary.recompile_material(mat)
    MATS['Print_' + name] = mat

class Model:
    def __init__(self, name):
        self.name, self.mesh, self.slots = name, u.DynamicMesh(), []

    def options(self, mat):
        if mat not in self.slots:
            self.slots.append(mat)
        return u.GeometryScriptPrimitiveOptions(material_id=self.slots.index(mat), uv_mode=u.GeometryScriptPrimitiveUVMode.SCALE_TO_FILL)

    def box(self, size, pos, mat='Ivory', rot=(0,0,0)):
        P.append_box(self.mesh, self.options(mat), u.Transform(location=u.Vector(*pos), rotation=u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2])), *size, origin=u.GeometryScriptPrimitiveOriginMode.CENTER)
        return self

    def cyl(self, radius, height, pos, mat='Metal', rot=(0,0,0), sides=24):
        P.append_cylinder(self.mesh, self.options(mat), u.Transform(location=u.Vector(*pos), rotation=u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2])), radius, height, radial_steps=sides, origin=u.GeometryScriptPrimitiveOriginMode.CENTER)
        return self

    def cone(self, r0, r1, height, pos, mat='Ivory'):
        P.append_cone(self.mesh, self.options(mat), u.Transform(location=u.Vector(*pos)), r0, r1, height, radial_steps=28, height_steps=1, origin=u.GeometryScriptPrimitiveOriginMode.CENTER)
        return self

    def finish(self, bevel=.14, collision=True):
        if bevel:
            opts=u.GeometryScriptMeshBevelOptions(bevel_distance=bevel, subdivisions=1, infer_material_id=True)
            u.GeometryScript_MeshModeling.apply_mesh_polygroup_bevel(self.mesh, opts)
        opts = u.GeometryScriptCreateNewStaticMeshAssetOptions(enable_collision=collision, collision_mode=u.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE, enable_recompute_tangents=True)
        path=f'{BASE}/Meshes/SM_{self.name}'
        existing=u.load_asset(path)
        if existing:
            cop=u.GeometryScriptCopyMeshToAssetOptions(enable_recompute_tangents=True, replace_materials=True, new_materials=[MATS[x] for x in self.slots])
            u.GeometryScript_AssetUtils.copy_mesh_to_static_mesh(self.mesh, existing, cop, u.GeometryScriptMeshWriteLOD())
            sm=existing
        else:
            sm, outcome=u.GeometryScript_NewAssetUtils.create_new_static_mesh_asset_from_mesh(self.mesh, path, opts)
            if not sm:
                raise RuntimeError(f'Failed mesh {self.name}: {outcome}')
            for i, mat in enumerate(self.slots):
                sm.set_material(i, MATS[mat])
        dynamic=self.name.startswith('Product_') or self.name in ('Broom','Mop','Bucket','InteriorDoor','EntranceDoor','FridgeDoor')
        if dynamic:
            opts=u.GeometryScriptCollisionFromMeshOptions(method=u.GeometryScriptCollisionGenerationMethod.ALIGNED_BOXES)
            u.GeometryScript_Collision.set_static_mesh_collision_from_mesh(self.mesh,sm,opts)
            sm.get_editor_property('body_setup').set_editor_property('collision_trace_flag',u.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
        ASSETS[self.name]=sm
        pivot='left_bottom_hinge' if self.name in ('EntranceDoor','InteriorDoor','FridgeDoor') else ('panel_center' if self.name.startswith('Sign_') else 'floor_center')
        CATALOG[self.name]={'asset': path, 'materials': self.slots, 'collision':collision, 'collision_type':'simple_boxes_and_complex' if dynamic else 'complex_static', 'triangles': self.mesh.get_triangle_count(), 'pivot':pivot}
        return sm

def build_materials():
    specs = [
        ('Ivory',(.73,.74,.67),.54,0,0,'wall'), ('Floor',(.48,.51,.46),.48,0,0,'floor'),
        ('Teal',(.035,.14,.13),.4,0,0,'plain'), ('Mint',(.22,.50,.43),.45,0,0,'plain'),
        ('Charcoal',(.025,.036,.038),.55,0,0,'plain'), ('Metal',(.43,.48,.49),.29,.85,0,'metal'),
        ('DarkMetal',(.07,.09,.09),.4,.7,0,'metal'), ('White',(.82,.84,.79),.37,0,0,'plain'),
        ('Wood',(.36,.21,.10),.56,0,0,'wood'), ('Cardboard',(.43,.29,.15),.86,0,0,'paper'),
        ('Tape',(.60,.45,.25),.43,0,0,'plain'), ('Rubber',(.024,.026,.023),.88,0,0,'plain'),
        ('WarmLight',(1,.79,.48),.4,0,5,'plain'), ('ColdLight',(.66,.92,.86),.4,0,100,'plain'),
        ('Screen',(.03,.32,.26),.4,0,1.8,'plain'), ('Warning',(.75,.34,.12),.52,0,0,'plain'),
        ('Ceramic',(.72,.75,.71),.21,0,0,'plain'), ('Asphalt',(.035,.045,.048),.96,0,0,'floor'),
        ('Water',(.13,.23,.14),.22,0,0,'plain'), ('Food',(.68,.45,.20),.64,0,0,'plain'),
        ('Red',(.38,.055,.035),.4,0,0,'plain'), ('Bristles',(.49,.33,.16),.9,0,0,'plain'),
    ]
    for spec in specs:
        material(*spec)
    material('Glass',(.20,.39,.36),.12,0,opacity=.16)
    for file in sorted((ROOT/'SourceArt/Labels').glob('*.png')):
        label_material(file.stem, file.stem in ('Storefront','Exit'))

def build_models():
    # Building modules have centered pivots. Furnishings have floor-level pivots.
    Model('Wall_400').box((400,16,320),(0,0,160)).finish(.3)
    Model('Partition_100').box((100,12,320),(0,0,160)).finish(.25)
    Model('Floor_400').box((400,400,12),(0,0,-6),'Floor').finish(.12)
    Model('Ceiling_400').box((400,400,8),(0,0,4),'Ivory').finish(.1)
    Model('Skirting_400').box((400,2,12),(0,0,6),'Charcoal').finish(.12)
    Model('GlassPanel').box((100,2,240),(0,0,120),'Glass').finish(0)
    # Shelf: 240 wide, 90 deep, 170 high. Visible floor feet and thin pressed trays.
    m=Model('Gondola')
    m.box((240,82,12),(0,0,9),'Charcoal').box((238,4,156),(0,0,88),'Ivory')
    for x in (-116,116):
        m.box((5,6,164),(x,0,88),'DarkMetal')
        for y in (-35,35):
            m.box((8,8,6),(x,y,3),'Rubber')
    for z in (25,61,97,133):
        for y in (-24,24):
            m.box((238,42,2.0),(0,y,z),'White')
            m.box((238,1.7,4.5),(0,y+(-21 if y<0 else 21),z+1),'Teal')
            m.box((238,1.9,1.5),(0,y+(-22 if y<0 else 22),z+1),'Metal')
    m.box((240,8,12),(0,0,171),'Teal').finish(.18)
    m=Model('WallShelf').box((220,6,190),(0,0,95),'Ivory')
    for x in (-105,105):
        m.box((4,42,180),(x,-18,90),'DarkMetal')
    for z in (18,53,88,123,158):
        m.box((220,44,2),(0,-22,z),'White').box((220,2,5),(0,-44,z+1),'Teal')
    m.finish(.16)
    # Open refrigerator body, glass doors are authored separately with a hinge pivot.
    m=Model('Refrigerator').box((132,78,14),(0,0,7),'Charcoal')
    m.box((132,5,213),(0,37,118),'White').box((132,78,14),(0,0,231),'Charcoal')
    for x in (-63,63):
        m.box((6,78,220),(x,0,121),'White').box((2,2,193),(x*.94,-36,119),'ColdLight')
    for z in (27,65,103,141,179):
        m.box((120,66,2),(0,0,z),'White').box((120,2,5),(0,-34,z+1),'Metal')
    m.box((127,3,14),(0,-39,231),'Teal').finish(.22)
    m=Model('FridgeDoor').box((61,2,206),(30.5,0,103),'Glass')
    for x in (1,60):
        m.box((2,4,208),(x,0,104),'DarkMetal')
    for z in (1,207):
        m.box((61,4,2),(30.5,0,z),'DarkMetal')
    m.box((2,6,54),(53,-4,105),'Metal').finish(.1)
    m=Model('Counter').box((350,78,86),(0,0,43),'Teal')
    m.box((354,82,5),(0,0,88),'Metal').box((338,2,52),(0,-40,50),'Wood')
    m.box((352,80,8),(0,0,4),'Charcoal')
    for x in (-120,0,120):
        m.box((2,2,52),(x,-41,50),'DarkMetal')
    m.finish(.6)
    m=Model('Register').box((38,34,10),(0,0,5),'Charcoal')
    m.cyl(2,28,(0,7,20),'Metal').box((40,5,26),(0,4,37),'Charcoal',(-12,0,0))
    m.box((35,1,21),(0,.8,37),'Screen',(-12,0,0)).box((24,16,3),(0,-4,12),'DarkMetal')
    m.finish(.3)
    Model('Scanner').box((15,19,12),(0,0,6),'Charcoal').box((10,1,7),(0,-10,9),'Red').finish(.4)
    m=Model('OdenStation').box((88,62,36),(0,0,18),'Metal').box((80,54,3),(0,0,38),'DarkMetal')
    for x in (-19,19):
        for y in (-12,12):
            m.box((35,21,2),(x,y,39),'Water')
            for dx in (-8,8):
                m.cyl(4,5,(x+dx,y,41),'Food')
    for x in (-38,38):
        m.cyl(1.3,42,(x,25,57),'Metal')
    m.box((86,60,3),(0,0,80),'Glass').finish(.25)
    m=Model('StockRack')
    for x in (-95,95):
        for y in (-29,29):
            m.box((4,4,220),(x,y,110),'DarkMetal')
    for z in (18,77,136,195):
        m.box((200,64,3),(0,0,z),'Wood')
        m.box((200,3,7),(0,-31,z-3),'Metal')
    m.finish(.2)
    m=Model('Carton').box((45,34,30),(0,0,15),'Cardboard')
    m.box((8,34.3,.2),(0,0,30.2),'Tape').box((8,.2,16),(0,-17.2,22),'Tape')
    m.box((18,.2,10),(9,-17.4,16),'White').finish(.2)
    m=Model('Desk').box((150,70,4),(0,0,76),'Wood')
    for x in (-68,68):
        for y in (-27,27):
            m.box((5,5,74),(x,y,37),'DarkMetal')
    m.box((43,60,45),(48,0,50),'Ivory')
    for z in (35,50,65):
        m.box((34,2,1.8),(48,-32,z),'Metal')
    m.finish(.45)
    m=Model('Chair').box((46,44,6),(0,0,46),'Charcoal').box((46,5,48),(0,20,73),'Teal')
    m.cyl(3,40,(0,0,23),'Metal')
    for yaw in (0,72,144,216,288):
        r=math.radians(yaw)
        m.box((34,4,3),(math.cos(r)*13, math.sin(r)*13,6),'DarkMetal',(0,yaw,0))
        m.cyl(3,4,(math.cos(r)*28,math.sin(r)*28,4),'Rubber',(90,0,0))
    m.finish(.3)
    m=Model('Computer').box((40,20,3),(0,0,1.5),'Charcoal').box((5,5,30),(0,6,16),'Metal')
    m.box((54,4,33),(0,7,38),'Charcoal').box((50,.5,28),(0,4.5,38),'Screen')
    m.box((42,16,2),(0,-20,1),'DarkMetal').finish(.25)
    m=Model('Locker').box((45,45,180),(0,0,90),'Teal')
    m.box((41,2,172),(0,-23,91),'Ivory').box((2,4,20),(13,-25,93),'Metal')
    for z in range(145,163,4):
        m.box((22,.5,1),(0,-24.4,z),'Charcoal')
    m.finish(.3)
    m=Model('Sink').box((65,48,11),(0,0,81),'Ceramic').box((55,41,66),(0,0,36),'Ivory')
    m.box((44,30,1),(0,-2,87),'DarkMetal').cyl(1.8,30,(0,16,96),'Metal')
    m.cyl(1.8,15,(0,9,110),'Metal',(90,0,0)).finish(.5)
    m=Model('Toilet').cone(14,21,35,(0,0,24),'Ceramic').cyl(22,8,(0,-6,44),'Ceramic')
    m.cyl(15,1,(0,-6,48.4),'DarkMetal').box((41,17,42),(0,24,54),'Ceramic')
    m.box((44,20,3),(0,24,77),'White').box((6,1,3),(13,14,68),'Metal').finish(.5)
    m=Model('Bin').cone(17,21,61,(0,0,32),'Charcoal').cyl(21.4,5,(0,0,65),'Metal')
    m.box((22,2,9),(0,-20,58),'Rubber').finish(.3)
    m=Model('Broom').box((34,8,6),(0,0,10),'Teal').cyl(1.4,138,(0,0,82),'Wood')
    for x in range(-15,16,2):
        m.box((1.2,6,11),(x,0,4.8),'Bristles')
    m.finish(.08)
    m=Model('Mop').cyl(1.2,135,(0,0,78),'Metal').box((34,13,3),(0,0,10),'Teal')
    for x in range(-15,16,3):
        m.box((1.8,15,8),(x,0,5),'White')
    m.finish(.07)
    m=Model('Bucket').cone(15,20,32,(0,0,18),'Warning').cyl(16.5,.5,(0,0,34),'DarkMetal')
    m.box((35,3,2),(0,0,45),'Metal').finish(.2)
    m=Model('FireExtinguisher').cyl(8,42,(0,0,27),'Red').cone(8,3,8,(0,0,52),'Red')
    m.box((14,4,3),(2,0,59),'Metal').cyl(1.5,35,(9,0,34),'Rubber').finish(.2)
    # Hinge pivot lies on left edge; all doors remain distinct editable meshes.
    Model('InteriorDoor').box((100,5,220),(50,0,110),'Wood').box((13,8,3),(83,-3,103),'Metal').finish(.3)
    Model('EntranceDoor').box((85,2,240),(42.5,0,120),'Glass').box((4,5,244),(0,0,122),'Metal').box((4,5,244),(85,0,122),'Metal').box((85,5,4),(42.5,0,2),'Metal').box((85,5,4),(42.5,0,242),'Metal').box((3,10,66),(72,-4,120),'Metal').finish(.12)
    Model('LightFixture').box((125,32,6),(0,0,0),'White').box((119,26,1),(0,0,-3.7),'ColdLight').finish(.25)
    Model('PriceTag').box((15,.3,5),(0,0,0),'White').box((2,.4,5),(-6,0,0),'Mint').finish(.05,False)
    # SKU silhouettes: modeled shoulders, necks, caps, lids, seams and separate printed zones.
    for i, product in enumerate(json.loads((ROOT/'SourceArt/products.json').read_text())):
        key=product['id']; printmat='Print_'+key; m=Model('Product_'+key)
        if key in ('Water','Sauce','Cleaner'):
            r,h=(3.2,19) if key=='Water' else ((2.6,17) if key=='Sauce' else (4.0,19))
            body='White' if key=='Water' else ('DarkMetal' if key=='Sauce' else 'Mint')
            m.cyl(r,h*.62,(0,0,h*.31),body).cone(r,r*.42,h*.22,(0,0,h*.73),body)
            m.cyl(r*.42,h*.15,(0,0,h*.91),'Teal').cyl(r+.03,h*.46,(0,0,h*.37),printmat)
            if key=='Cleaner':
                m.box((10,2,3),(2,0,h+1),'Charcoal').box((2,2,5),(5,0,h-1),'Charcoal')
        elif key in ('Soda','Can'):
            r,h=(3.2,12) if key=='Soda' else (4,10)
            m.cyl(r,h,(0,0,h/2),printmat).cyl(r+.12,.5,(0,0,.4),'Metal').cyl(r+.12,.5,(0,0,h),'Metal')
            m.box((1.4,2.6,.18),(0,0,h+.4),'DarkMetal')
        elif key=='Noodles':
            m.cone(4,5.5,10,(0,0,5),printmat).cyl(5.7,.5,(0,0,10.3),'White')
        elif key=='Milk':
            m.box((6.7,6.7,12),(0,0,6),printmat).box((6.7,4.7,4),(0,0,13.5),'White',(0,0,0)).box((6.8,.7,2),(0,0,16),'Teal')
        else:
            sizes={'Chips':(13,4,19),'Biscuits':(12,5,16),'Bread':(12,6,17),'Tissue':(21,11,8),'Meal':(20,15,6)}
            w,d,h=sizes[key]
            m.box((w,d,h),(0,0,h/2),printmat)
            if key in ('Chips','Bread'):
                m.box((w+.5,d+.4,.7),(0,0,.7),'White').box((w+.5,d+.4,.7),(0,0,h-.2),'White')
            if key=='Meal':
                m.box((w+.6,d+.6,.6),(0,0,h),'White')
        m.finish(.07)
    # Flat graphic panels use explicit box UVs; their backing and frame are physical.
    for key, w,h in [('Storefront',650,122),('Welcome',180,67.5),('Exit',65,24.4),('Office',40,20),('Warehouse',40,20),('Restroom',40,20),('Poster',72,96),('ShiftNotice',180,67.5)]:
        Model('Sign_'+key).box((w,1,h),(0,0,0),'Print_'+key).finish(0,False)
    log(f'MODELS_READY {len(ASSETS)}')

def spawn(name, mesh, pos, yaw=0, scale=(1,1,1), folder='Environment', collision=True, tags=()):
    actor=EAS.spawn_actor_from_class(u.StaticMeshActor, u.Vector(*pos), u.Rotator(yaw=yaw))
    actor.set_actor_label(name)
    actor.set_folder_path(folder)
    comp=actor.static_mesh_component
    comp.set_static_mesh(ASSETS[mesh])
    comp.set_mobility(u.ComponentMobility.MOVABLE if any(t in ('Interactive.Grabbable','Interactive.Tool','Interactive.Door','Interactive.FridgeDoor') for t in tags) else u.ComponentMobility.STATIC)
    comp.set_collision_enabled(u.CollisionEnabled.QUERY_AND_PHYSICS if collision else u.CollisionEnabled.NO_COLLISION)
    actor.set_actor_scale3d(u.Vector(*scale))
    actor.set_editor_property('tags',[u.Name(x) for x in tags])
    ACTORS[name]=actor
    return actor

def box(name, size, pos, mat, folder='Architecture', collision=True):
    # A library mesh per material; dimensions remain editable through actor scale.
    key='Unit_'+mat
    if key not in ASSETS:
        Model(key).box((100,100,100),(0,0,0),mat).finish(.15,collision)
    return spawn(name,key,pos,scale=tuple(v/100 for v in size),folder=folder,collision=collision)

def stock(mesh, pos, yaw=0):
    # Repeated display stock is instanced. Individually marked hero props are separate actors.
    if mesh not in INSTANCES:
        owner=EAS.spawn_actor_from_class(u.Actor,u.Vector())
        owner.set_actor_label('DisplayStock_'+mesh)
        owner.set_folder_path('Merchandise/Instanced')
        subs=u.get_engine_subsystem(u.SubobjectDataSubsystem)
        handles=subs.k2_gather_subobject_data_for_instance(owner)
        handle, failure=subs.add_new_subobject(u.AddNewSubobjectParams(parent_handle=handles[0],new_class=u.InstancedStaticMeshComponent))
        c=u.SubobjectDataBlueprintFunctionLibrary.get_associated_object(u.SubobjectDataBlueprintFunctionLibrary.get_data(handle))
        if not c:
            raise RuntimeError('Cannot create stock instances: '+str(failure))
        owner.get_editor_property('root_component').set_mobility(u.ComponentMobility.STATIC)
        c.set_static_mesh(ASSETS[mesh])
        c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
        c.set_mobility(u.ComponentMobility.STATIC)
        INSTANCES[mesh]=c
    INSTANCES[mesh].add_instance(u.Transform(location=u.Vector(*pos),rotation=u.Rotator(yaw=yaw)),world_space=True)

def text(name, value, pos, size=10, yaw=-90, color=(.8,.85,.78), folder='Signage'):
    a=EAS.spawn_actor_from_class(u.TextRenderActor,u.Vector(*pos),u.Rotator(yaw=yaw))
    a.set_actor_label(name); a.set_folder_path(folder)
    c=a.get_component_by_class(u.TextRenderComponent)
    c.set_text(value); c.set_world_size(size)
    c.set_horizontal_alignment(u.HorizTextAligment.EHTA_CENTER)
    c.set_text_render_color(u.Color(*(int(v*255) for v in color),255))
    c.set_collision_enabled(u.CollisionEnabled.NO_COLLISION)
    return a

def point(name,pos,color,intensity,radius=400,source=8,shadow=False):
    a=EAS.spawn_actor_from_class(u.PointLight,u.Vector(*pos))
    a.set_actor_label(name); a.set_folder_path('Lighting')
    c=a.point_light_component; c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_editor_property('intensity_units',u.LightUnits.LUMENS)
    c.set_light_color(u.LinearColor(*color,1)); c.set_intensity(intensity)
    c.set_attenuation_radius(radius); c.set_source_radius(source)
    c.set_cast_shadows(shadow)
    ACTORS[name]=a
    return a

def rect(name,pos,intensity=6000):
    a=EAS.spawn_actor_from_class(u.RectLight,u.Vector(*pos),u.Rotator(pitch=-90))
    a.set_actor_label(name); a.set_folder_path('Lighting/Main')
    c=a.rect_light_component; c.set_mobility(u.ComponentMobility.MOVABLE)
    c.set_editor_property('intensity_units',u.LightUnits.LUMENS)
    c.set_intensity(intensity); c.set_light_color(u.LinearColor(.80,.90,.84,1))
    c.set_editor_property('source_width',125); c.set_editor_property('source_height',32)
    c.set_editor_property('attenuation_radius',850)
    c.set_cast_shadows(False)
    ACTORS[name]=a
    return a

def door_open(name,pos,yaw=90):
    return spawn(name,'InteriorDoor',pos,yaw,folder='Architecture/Doors',tags=('Interactive.Door','Pivot.Hinge','Preview.Open'))

def wall_y(name,y,x0,x1,z0=0,z1=320,mat='Ivory',thick=12):
    return box(name,(x1-x0,thick,z1-z0),((x0+x1)/2,y,(z0+z1)/2),mat)

def doorway_wall(name,y,lo,hi,doorx):
    wall_y(name+'_L',y,lo,doorx)
    wall_y(name+'_R',y,doorx+110,hi)
    wall_y(name+'_Lintel',y,doorx,doorx+110,230)
    for x in (doorx,doorx+110):
        box(name+'_Jamb_'+str(x),(5,19,231),(x,y,115.5),'DarkMetal')
    box(name+'_FrameTop',(115,19,5),(doorx+55,y,232),'DarkMetal')

def build_scene(state):
    ACTORS.clear(); INSTANCES.clear(); STATES.clear()
    map_path=BASE+'/Maps/'+state
    if u.EditorAssetLibrary.does_asset_exist(map_path):
        LEVELS.load_level(map_path)
    else:
        LEVELS.new_level(map_path)
    # Rebuild only this project's authored map. Preserve editor world infrastructure.
    for a in list(EAS.get_all_level_actors()):
        if not isinstance(a,(u.WorldSettings,u.LevelScriptActor,u.Brush)):
            EAS.destroy_actor(a)
    for ix in range(4):
        for iy in range(4):
            spawn(f'Floor_{ix}_{iy}','Floor_400',(200+ix*400,200+iy*400,0),folder='Architecture/Floor')
            spawn(f'Ceiling_{ix}_{iy}','Ceiling_400',(200+ix*400,200+iy*400,320),folder='Architecture/Ceiling')
    box('ExteriorApron',(2300,2400,20),(800,700,-28),'Asphalt',folder='Exterior')
    box('EntranceStep',(1800,220,12),(800,-100,-6),'Floor',folder='Exterior')
    for x in (0,1600):
        box('SideWall_'+str(x),(20,1600,320),(x,800,160),'Ivory')
        box('SideSkirting_'+str(x),(3,1600,14),(x+(11 if x==0 else -11),800,7),'Charcoal')
    wall_y('RearWall',1600,0,1600,thick=20)
    # Storefront: glass bays, actual door opening and overhead fascia.
    for lo,hi in ((0,700),(900,1600)):
        wall_y('FrontBase_'+str(lo),0,lo,hi,0,42,'Teal',18)
        wall_y('FrontGlass_'+str(lo),0,lo,hi,42,278,'Glass',2)
        for x in range(lo,hi+1,175):
            box('WindowMullion_'+str(x),(4,8,240),(x,0,160),'DarkMetal')
    wall_y('FrontHeader',0,0,1600,278,320,'Teal',20)
    spawn('EntranceLeft','EntranceDoor',(710,-2,0),-88,folder='Architecture/Doors',tags=('Interactive.Door','Pivot.Hinge','Preview.Open'))
    spawn('EntranceRight','EntranceDoor',(890,-2,0),-92,scale=(-1,1,1),folder='Architecture/Doors',tags=('Interactive.Door','Pivot.Hinge','Preview.Open'))
    spawn('ExteriorSign','Sign_Storefront',(800,-13,304),folder='Exterior/Brand',scale=(1,1,.55),collision=False)
    wall_y('StoreFascia',-6,0,1600,283,330,'Teal',8)
    # Explicit corridor access, open doors for all first-pass traversal.
    doorway_wall('SalesRear',1050,0,1600,1400)
    doorway_wall('StockroomFront',1200,0,800,340)
    doorway_wall('OfficeFront',1200,800,1300,980)
    doorway_wall('RestroomFront',1200,1300,1600,1380)
    for x in (800,1300):
        box('RoomDivider_'+str(x),(12,400,320),(x,1400,160),'Ivory')
    for name,pos in [('CorridorDoor',(1405,1050,0)),('WarehouseDoor',(345,1200,0)),('OfficeDoor',(985,1200,0)),('RestroomDoor',(1385,1200,0))]:
        door_open(name,pos,-90 if name=='CorridorDoor' else 90)
    for key,x in [('Warehouse',395),('Office',1035),('Restroom',1435)]:
        spawn(key+'Sign','Sign_'+key,(x,1191,260),folder='Signage',collision=False)
    # A continuous back-of-house strip makes every service room reachable.
    for x in (400,1050,1450):
        rect('RearLight_'+str(x),(x,1420,304),2300)
        spawn('RearFixture_'+str(x),'LightFixture',(x,1420,312),folder='Lighting/Fixtures')
    rect('CorridorLight',(900,1125,304),1900)
    # Retail layout: double gondolas, 1.6m x clearances; 2.1m row aisles.
    for row,y in enumerate((450,750)):
        for col,x in enumerate((400,800,1200)):
            ident=f'Gondola_{row}_{col}'
            spawn(ident,'Gondola',(x,y,0),folder='Fixtures/Gondolas',tags=('Fixture.Shelf','Shift.Movable' if col==1 else 'Fixture.Fixed'))
            if col==1:
                STATES[ident]={'normal':[x,y,0], 'shift':[x+(30 if row==0 else -30),y,0]}
            # Stock matches each shelf; shifting instances move with the shelf group below.
            sku_base=(row*3+col)*2
            for tier,z in enumerate((26.1,62.1,98.1,134.1)):
                for side in (-1,1):
                    for slot,dx in enumerate((-95,-57,-19,19,57,95)):
                        sku=(sku_base+tier+slot//2+(0 if side<0 else 3))%12
                        key=PRODUCT_IDS[sku]
                        for depth in (0,1):
                            stock('Product_'+key,(x+dx,y+side*(29-depth*13),z),0 if side<0 else 180)
                    for dx in (-80,0,80):
                        spawn(f'Tag_{row}_{col}_{tier}_{side}_{dx}','PriceTag',(x+dx,y+side*46,z+1),0 if side<0 else 180,folder='Merchandise/Tags',collision=False)
                        text(f'Price_{row}_{col}_{tier}_{side}_{dx}','%02d.90' % (2+tier+col),(x+dx,y+side*46.5,z),size=2.5,yaw=-90 if side<0 else 90,color=(.08,.14,.13),folder='Merchandise/Tags')
    # Two wall gondolas, against the right wall, facing west.
    for i,y in enumerate((460,780)):
        spawn('WallShelf_'+str(i),'WallShelf',(1580,y,0),-90,folder='Fixtures/WallShelves')
        for tier,z in enumerate((19.1,54.1,89.1,124.1,159.1)):
            for dy in (-85,-45,-5,35,75):
                stock('Product_'+PRODUCT_IDS[(i*4+tier)%12],(1548,y+dy,z),-90)
    # Four cold cabinets: back wall; bodies extend toward rear; fronts face entry.
    for i,x in enumerate((110,270,430,590)):
        spawn('Fridge_'+str(i),'Refrigerator',(x,1002,0),folder='Fixtures/ColdStorage')
        for door in range(2):
            spawn(f'FridgeDoor_{i}_{door}','FridgeDoor',(x-60+door*62,961,16),folder='Fixtures/ColdStorage/Doors',tags=('Interactive.FridgeDoor','Pivot.Hinge'))
        for z in (28.1,66.1,104.1,142.1,180.1):
            for dx in (-42,-21,0,21,42):
                stock('Product_'+('Water' if i<2 else 'Milk'),(x+dx,985,z))
        point('ColdLamp_'+str(i),(x,939,153),(.40,.80,.68),150,280)
    # Drinks case on left wall: rotated so the front faces the aisles.
    spawn('DrinksCabinet','Refrigerator',(52,470,0),90,folder='Fixtures/ColdStorage')
    for door in range(2):
        spawn('DrinksDoor_'+str(door),'FridgeDoor',(93,410+door*62,16),90,folder='Fixtures/ColdStorage/Doors',tags=('Interactive.FridgeDoor','Pivot.Hinge'))
    for z in (28.1,66.1,104.1,142.1,180.1):
        for dy in (-42,-21,0,21,42):
            stock('Product_Soda',(75,470+dy,z),90)
    point('DrinksLamp',(125,470,155),(.42,.78,.72),130,280)
    spawn('Checkout','Counter',(1260,180,0),folder='Fixtures/Checkout')
    spawn('CashRegister','Register',(1270,180,91),folder='Props/Checkout')
    spawn('Scanner','Scanner',(1198,165,91),folder='Props/Checkout',tags=('Interactive.Scanner',))
    spawn('OdenStation','OdenStation',(1380,180,91),folder='Props/Checkout',tags=('Interactive.Oden',))
    spawn('CheckoutBin','Bin',(1500,210,0),folder='Props/Cleaning')
    spawn('EntranceBin','Bin',(120,115,0),folder='Props/Cleaning')
    for i,key in enumerate(('Water','Milk','Noodles','Chips')):
        a=spawn('Hero_'+key,'Product_'+key,(1110+i*27,178,91),folder='Merchandise/Hero',tags=('Interactive.Grabbable','SKU.'+key))
        STATES['Hero_'+key]={'normal':[1110+i*27,178,91], 'shift':[1140+i*22,216,91]}
    # Back rooms, deliberately furnished with the same reusable asset library.
    for x in (120,400,680):
        spawn('StockRack_'+str(x),'StockRack',(x,1515,0),folder='Backrooms/Warehouse')
        for z in (19.6,78.6,137.6,196.6):
            for dx in (-65,0,65):
                spawn(f'Carton_{x}_{z}_{dx}','Carton',(x+dx,1512,z),folder='Backrooms/Warehouse/Cartons')
    for i in range(5):
        spawn('FloorCarton_'+str(i),'Carton',(100+i*55,1300,0),yaw=(0 if i%2 else 90),folder='Backrooms/Warehouse/Cartons')
    spawn('OfficeDesk','Desk',(1070,1510,0),folder='Backrooms/Office')
    spawn('OfficeComputer','Computer',(1040,1510,79),folder='Backrooms/Office')
    spawn('OfficeChair','Chair',(1070,1410,0),folder='Backrooms/Office')
    for x in (845,900):
        spawn('StaffLocker_'+str(x),'Locker',(x,1525,0),folder='Backrooms/Office')
    spawn('RestroomSink','Sink',(1560,1350,0),-90,folder='Backrooms/Restroom')
    spawn('Toilet','Toilet',(1515,1510,0),folder='Backrooms/Restroom')
    box('Mirror',(1,62,72),(1591,1350,158),'Metal',folder='Backrooms/Restroom',collision=False)
    spawn('RestroomBin','Bin',(1350,1510,0),folder='Backrooms/Restroom')
    spawn('Broom','Broom',(65,1260,0),folder='Props/Cleaning',tags=('Interactive.Tool','Tool.Broom'))
    spawn('Mop','Mop',(100,1260,0),folder='Props/Cleaning',tags=('Interactive.Tool','Tool.Mop'))
    spawn('Bucket','Bucket',(150,1260,0),folder='Props/Cleaning')
    for x,y in ((1550,105),(1550,1140),(40,1130)):
        spawn('Extinguisher_'+str(x)+'_'+str(y),'FireExtinguisher',(x,y,0),folder='Props/Safety')
    # Ceiling coffers/seams, vents and luminous panels create a legible architectural rhythm.
    for x in (200,600,1000,1400):
        box('CeilingRail_'+str(x),(2,1050,2),(x,525,318),'DarkMetal',folder='Architecture/Ceiling',collision=False)
    for row,y in enumerate((220,550,890)):
        for col,x in enumerate((360,800,1240)):
            spawn(f'Fixture_{row}_{col}','LightFixture',(x,y,312),folder='Lighting/Fixtures')
            light=rect(f'MainLight_{row}_{col}',(x,y,302),5200)
            if col==1:
                light.rect_light_component.set_cast_shadows(True)
    for x in (100,1500):
        box('VentFrame_'+str(x),(70,30,3),(x,720,316),'Charcoal',folder='Architecture/Ceiling',collision=False)
        for k in range(10):
            box(f'VentSlat_{x}_{k}',(64,1,2),(x,707+k*2.7,313),'Metal',folder='Architecture/Ceiling',collision=False)
    # Graphics: mostly physical signs, with a few large readable editorial type elements.
    spawn('WelcomeSign','Sign_Welcome',(1080,1042,256),folder='Signage',collision=False)
    spawn('ExitSign','Sign_Exit',(800,-7,263),folder='Signage',collision=False)
    spawn('CorridorExitSign','Sign_Exit',(1455,1040,258),folder='Signage',collision=False)
    spawn('Poster','Sign_Poster',(125,11,172),180,folder='Signage',collision=False)
    text('CheckoutText','CHECKOUT / 01',(1260,138,66),15,color=(.83,.85,.74))
    text('Aisle01','01 / DAILY ESSENTIALS',(400,445.7,166.5),7.5)
    text('Aisle02','02 / FOOD & DRINK',(800,445.7,166.5),7.5)
    text('Aisle03','03 / HOME CARE',(1200,445.7,166.5),7.5)
    text('FloorBrand','SHIFT / 24',(800,22,36),20)
    # A small pool of light on the threshold keeps the exterior visible at night.
    point('EntranceCanopyLight',(800,-90,290),(.72,.81,.79),1800,600)
    point('EmergencyCounter',(1250,180,180),(.65,.43,.18),120,360)
    point('EmergencyCorridor',(1450,1125,190),(.25,.59,.45),100,360)
    point('FrontMoonFill',(800,-420,250),(.20,.32,.55),1200,1100)
    # Scene state labels and audio attachment markers are plain Actors, not runtime sound dependencies.
    for name,pos in [('Refrigeration',(350,1000,170)),('PlayerBreathing',(800,200,160)),('ShelfFootsteps',(800,620,0))]:
        a=EAS.spawn_actor_from_class(u.TargetPoint,u.Vector(*pos))
        a.set_actor_label('AudioAnchor_'+name); a.set_folder_path('AudioAnchors')
        a.set_editor_property('tags',[u.Name('Audio.Anchor'),u.Name('Audio.'+name)])
    player=EAS.spawn_actor_from_class(u.PlayerStart,u.Vector(800,150,100),u.Rotator(yaw=90))
    player.set_actor_label('WalkthroughStart')
    gm=u.load_class(None,'/Script/ShiftStore.ShiftStoreGameMode') or u.load_class(None,'/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode.BP_FirstPersonGameMode_C')
    world=u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',gm)
    # Static display-only bodies do not need the template character full-body mesh.
    char=u.load_asset('/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter')
    if char:
        cdo=u.get_default_object(char.generated_class())
        cdo.character_movement.max_walk_speed=260
        cdo.character_movement.jump_z_velocity=0
        for component in cdo.get_components_by_class(u.SkeletalMeshComponent):
            component.set_editor_property('visible',False)
        u.BlueprintEditorLibrary.compile_blueprint(char)
    # Post process stays explicitly exposed in the editor for later lighting iteration.
    pp=EAS.spawn_actor_from_class(u.PostProcessVolume,u.Vector(800,800,160))
    pp.set_actor_label('StoreColorAndExposure'); pp.set_folder_path('Lighting')
    pp.set_editor_property('unbound',True)
    settings=pp.get_editor_property('settings')
    for k,v in dict(override_auto_exposure_method=True,auto_exposure_method=u.AutoExposureMethod.AEM_MANUAL,override_auto_exposure_bias=True,auto_exposure_bias=0,override_auto_exposure_apply_physical_camera_exposure=True,auto_exposure_apply_physical_camera_exposure=True,override_camera_iso=True,camera_iso=100,override_camera_shutter_speed=True,camera_shutter_speed=60,override_depth_of_field_fstop=True,depth_of_field_fstop=2,override_vignette_intensity=True,vignette_intensity=.18,override_bloom_intensity=True,bloom_intensity=.18,override_motion_blur_amount=True,motion_blur_amount=0).items():
        settings.set_editor_property(k,v)
    pp.set_editor_property('settings',settings)
    # Fixed preview viewpoints are shared by Normal and Shift screenshots.
    for name,pos,rot in [('Entrance',(800,145,166),(0,90,0)),('Shop',(210,230,166),(0,42,0)),('Checkout',(940,390,166),(-7,-47,0)),('Corridor',(1470,1100,166),(0,177,0)),('Stockroom',(400,1280,166),(0,90,0))]:
        cam=EAS.spawn_actor_from_class(u.CameraActor,u.Vector(*pos),u.Rotator(pitch=rot[0],yaw=rot[1],roll=rot[2]))
        cam.set_actor_label('View_'+name); cam.set_folder_path('PreviewCameras')
        cam.camera_component.set_field_of_view(80)
    LEVELS.save_current_level()
    u.EditorAssetLibrary.save_directory(BASE, only_if_is_dirty=False, recursive=True)
    u.EditorAssetLibrary.save_loaded_asset(char)
    if state=='Normal':
        log('NORMAL_MAP_SAVED')
        return
    # Rebuild a Shift map from the same authoring world, with explicit transforms.
    normal_stock=[]
    for mesh, comp in INSTANCES.items():
        for i in range(comp.get_instance_count()):
            t=comp.get_instance_transform(i,world_space=True)
            normal_stock.append((comp,i,t))
    for name,cfg in STATES.items():
        ACTORS[name].set_actor_location(u.Vector(*cfg['shift']),False,False)
    # Stock belonging to either moving gondola follows its fixture. Tags/labels follow too.
    for comp,i,t in normal_stock:
        p=t.translation
        if 675<=p.x<=925 and (400<=p.y<=500 or 700<=p.y<=800):
            t.translation=u.Vector(p.x+(30 if p.y<600 else -30),p.y,p.z)
            comp.update_instance_transform(i,t,world_space=True,mark_render_state_dirty=True,teleport=True)
    for name,a in ACTORS.items():
        if name.startswith('Tag_') and name.split('_')[2]=='1':
            row=int(name.split('_')[1]); p=a.get_actor_location()
            a.set_actor_location(p+u.Vector(30 if row==0 else -30,0,0),False,False)
        if name.startswith('MainLight_'):
            a.rect_light_component.set_intensity(45)
        if name.startswith('RearLight_') or name=='CorridorLight':
            a.rect_light_component.set_intensity(140)
    # Fixture lenses become dark rather than emitting full GI in the abnormal state.
    settings=pp.get_editor_property('settings')
    settings.set_editor_property('camera_iso',1600)
    settings.set_editor_property('camera_shutter_speed',30)
    pp.set_editor_property('settings',settings)
    for a in EAS.get_all_level_actors():
        if a.get_actor_label()=='Aisle02':
            a.set_actor_location(a.get_actor_location()+u.Vector(30,0,0),False,False)
        if a.get_actor_label().startswith(('Fixture_','RearFixture_')):
            a.static_mesh_component.set_material(1,MATS['Charcoal'])
        if a.get_actor_label().startswith('Price_'):
            parts=a.get_actor_label().split('_')
            if parts[2]=='1':
                row=int(parts[1]); a.set_actor_location(a.get_actor_location()+u.Vector(30 if row==0 else -30,0,0),False,False)
    ACTORS['WelcomeSign'].static_mesh_component.set_static_mesh(ASSETS['Sign_ShiftNotice'])
    LEVELS.save_current_level()
    # Explicit data files retain normal and abnormal transforms for future runtime integration.
    (ROOT/'SourceArt/scene_states.json').write_text(json.dumps({'units':'centimeters','normal_map':BASE+'/Maps/Normal','shift_map':BASE+'/Maps/Shift','fixtures_and_hero_props':STATES,'changes':{'shelf_offsets_cm':[30,-30],'hero_product_changes':4,'lighting':'main dimmed; refrigeration and exit remain'}},indent=2),encoding='utf-8')
    u.EditorAssetLibrary.save_directory(BASE,only_if_is_dirty=False,recursive=True)
    LEVELS.load_level(BASE+'/Maps/Normal')
    LEVELS.save_current_level()
    log('BOTH_MAPS_SAVED')

def main():
    (ROOT/'Saved/generation_error.txt').unlink(missing_ok=True)
    build_materials()
    build_models()
    u.EditorAssetLibrary.save_directory(BASE,only_if_is_dirty=False,recursive=True)
    build_scene('Normal')
    build_scene('Shift')
    (ROOT/'SourceArt/asset_catalog.json').write_text(json.dumps(CATALOG,indent=2),encoding='utf-8')
    if u.EditorAssetLibrary.does_directory_exist(BASE+'/Probe'):
        u.EditorAssetLibrary.delete_directory(BASE+'/Probe')
    log('GENERATION_COMPLETE')

PRODUCT_IDS=['Water','Milk','Soda','Noodles','Chips','Biscuits','Can','Bread','Tissue','Meal','Sauce','Cleaner']

if __name__=='__main__':
    try:
        main()
    except Exception:
        failure=traceback.format_exc()
        (ROOT/'Saved/generation_error.txt').write_text(failure,encoding='utf-8')
        u.log_error(failure)
        raise
