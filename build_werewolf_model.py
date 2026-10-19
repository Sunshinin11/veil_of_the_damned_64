import bpy
import addon_utils
import os

# Disable fast64 to prevent frame_change_pre errors during export
addon_utils.disable("fast64", default_set=False)

# Start clean
bpy.ops.wm.read_factory_settings(use_empty=True)

WEREWOLF_DIR = r"C:\Desenvolvimento\Nintendo 64\Veil of the Damned 64\Modelos\Werewolf"
OUT_GLB = r"C:\Desenvolvimento\Nintendo 64\Veil of the Damned 64\assets\models\werewolf.glb"

idle_fbx = os.path.join(WEREWOLF_DIR, "Mutant Idle.fbx")
print(f"Importing base Werewolf FBX: {idle_fbx}")
bpy.ops.import_scene.fbx(filepath=idle_fbx)

# Find armature and mesh
armature_obj = None
mesh_obj = None

for obj in bpy.data.objects:
    if obj.type == 'ARMATURE':
        armature_obj = obj
    elif obj.type == 'MESH':
        mesh_obj = obj
    elif obj.type in {'LIGHT', 'CAMERA'}:
        bpy.data.objects.remove(obj)

if not armature_obj or not mesh_obj:
    raise RuntimeError(f"Could not find Armature and Mesh in {idle_fbx}")

print(f"Base Armature: {armature_obj.name}, Mesh: {mesh_obj.name}")

# Rename base idle action
if armature_obj.animation_data and armature_obj.animation_data.action:
    idle_act = armature_obj.animation_data.action
    idle_act.name = "idle"
    idle_act.use_fake_user = True
    print("Found base idle action, renamed to 'idle'")
else:
    raise RuntimeError("No animation action found in Mutant Idle.fbx")

# Mapping of other animations:
#   walk   = Mutant Run.fbx
#   attack = Mutant Swiping.fbx
#   cast   = Mutant Swiping.fbx (reuse attack)
#   hurt   = Standing React Small From Left.fbx
#   death  = Mutant Dying.fbx
OTHER_ANIMS = [
    ("walk", os.path.join(WEREWOLF_DIR, "Mutant Run.fbx")),
    ("attack", os.path.join(WEREWOLF_DIR, "Mutant Swiping.fbx")),
    ("hurt", os.path.join(WEREWOLF_DIR, "Standing React Small From Left.fbx")),
    ("death", os.path.join(WEREWOLF_DIR, "Mutant Dying.fbx")),
]

for clip_name, fbx_path in OTHER_ANIMS:
    if not os.path.exists(fbx_path):
        raise FileNotFoundError(f"Missing animation FBX: {fbx_path}")
    
    print(f"Importing {clip_name}: {fbx_path}")
    objs_before = set(bpy.data.objects)
    actions_before = set(bpy.data.actions)
    
    bpy.ops.import_scene.fbx(filepath=fbx_path)
    
    new_objs = set(bpy.data.objects) - objs_before
    new_actions = set(bpy.data.actions) - actions_before
    
    anim_action = None
    for act in new_actions:
        anim_action = act
        break
    
    if not anim_action:
        for o in new_objs:
            if o.type == 'ARMATURE' and o.animation_data and o.animation_data.action:
                anim_action = o.animation_data.action
                break
                
    if not anim_action:
        raise RuntimeError(f"Could not find action in {fbx_path}")
        
    anim_action.name = clip_name
    anim_action.use_fake_user = True
    print(f"  Mapped action '{anim_action.name}' (frames: {anim_action.frame_range[0]} to {anim_action.frame_range[1]})")
    
    # Clean up imported duplicate objects
    for o in new_objs:
        bpy.data.objects.remove(o)

# Create 'cast' animation by copying 'attack' (werewolf doesn't cast magic, reuses swipe attack)
attack_act = bpy.data.actions.get("attack")
if not attack_act:
    raise RuntimeError("Action 'attack' not found to create 'cast'")

cast_act = attack_act.copy()
cast_act.name = "cast"
cast_act.use_fake_user = True
print(f"Created duplicate action 'cast' from 'attack' (frames: {cast_act.frame_range[0]} to {cast_act.frame_range[1]})")

# Setup NLA Tracks on the base armature
if not armature_obj.animation_data:
    armature_obj.animation_data_create()

for track in list(armature_obj.animation_data.nla_tracks):
    armature_obj.animation_data.nla_tracks.remove(track)

ALL_CLIPS = ["idle", "walk", "attack", "cast", "hurt", "death"]
for clip_name in ALL_CLIPS:
    act = bpy.data.actions.get(clip_name)
    if not act:
        raise RuntimeError(f"Missing action '{clip_name}' in bpy.data.actions")
    track = armature_obj.animation_data.nla_tracks.new()
    track.name = clip_name
    track.strips.new(clip_name, int(act.frame_range[0]), act)
    print(f"Created NLA Track '{track.name}' with action '{act.name}'")

# Clear active action so NLA tracks are evaluated
armature_obj.animation_data.action = None

# Ensure material exists
if len(mesh_obj.data.materials) == 0:
    mat = bpy.data.materials.new(name="WerewolfMaterial")
    mat.use_nodes = True
    mesh_obj.data.materials.append(mat)
    print("Created fallback WerewolfMaterial")
else:
    print(f"Existing materials: {[m.name for m in mesh_obj.data.materials]}")

# Ensure 1 bone influence per vertex
bpy.context.view_layer.objects.active = mesh_obj
mesh_obj.select_set(True)
bpy.ops.object.vertex_group_limit_total(limit=1)
bpy.ops.object.vertex_group_normalize_all()

# Generate vertex colors based on bone regions
vg_map = {vg.index: vg.name for vg in mesh_obj.vertex_groups}

# Gothic werewolf color palette
HEAD_COLOR   = (0.26, 0.19, 0.15, 1.0) # Head, neck (wolf face/mane)
MANE_COLOR   = (0.22, 0.16, 0.12, 1.0) # Spine, shoulders
LIMBS_COLOR  = (0.18, 0.13, 0.10, 1.0) # Arms, legs
CLAWS_COLOR  = (0.12, 0.09, 0.08, 1.0) # Hands, feet, toes
BODY_COLOR   = (0.20, 0.14, 0.11, 1.0) # Hips, torso

vert_colors = []
for v in mesh_obj.data.vertices:
    best_name = ""
    max_w = -1.0
    for g in v.groups:
        if g.group in vg_map and g.weight > max_w:
            max_w = g.weight
            best_name = vg_map[g.group]
    
    nl = best_name.lower()
    if any(k in nl for k in ["head", "neck"]):
        col = HEAD_COLOR
    elif any(k in nl for k in ["hand", "foot", "toe"]):
        col = CLAWS_COLOR
    elif any(k in nl for k in ["arm", "leg"]):
        col = LIMBS_COLOR
    elif any(k in nl for k in ["spine", "shoulder"]):
        col = MANE_COLOR
    else:
        col = BODY_COLOR
    vert_colors.append(col)

# Set vertex colors on mesh
if "COLOR_0" not in mesh_obj.data.color_attributes:
    color_layer = mesh_obj.data.color_attributes.new(name="COLOR_0", type='FLOAT_COLOR', domain='CORNER')
else:
    color_layer = mesh_obj.data.color_attributes["COLOR_0"]

for poly in mesh_obj.data.polygons:
    for loop_idx in poly.loop_indices:
        v_idx = mesh_obj.data.loops[loop_idx].vertex_index
        color_layer.data[loop_idx].color = vert_colors[v_idx]

print(f"Assigned COLOR_0 to {len(mesh_obj.data.loops)} loops across {len(mesh_obj.data.vertices)} vertices")

# Select only armature and mesh
bpy.ops.object.select_all(action='DESELECT')
armature_obj.select_set(True)
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = armature_obj

# Export GLB
os.makedirs(os.path.dirname(OUT_GLB), exist_ok=True)
print(f"Exporting Werewolf GLB to: {OUT_GLB}")

bpy.ops.export_scene.gltf(
    filepath=OUT_GLB,
    export_format='GLB',
    use_selection=True,
    export_armature_object_remove=True,
    export_influence_nb=1,
    export_extras=True,
    export_yup=True,
    export_animation_mode='NLA_TRACKS',
    export_all_vertex_colors=True
)

print(f"Successfully generated {OUT_GLB} (size: {os.path.getsize(OUT_GLB)} bytes)")
