import bpy
import addon_utils
import os

# Disable fast64
addon_utils.disable("fast64", default_set=False)
if hasattr(bpy.app.handlers, 'frame_change_pre'):
    bpy.app.handlers.frame_change_pre.clear()

# Start clean
bpy.ops.wm.read_factory_settings(use_empty=True)

MODEL_DIR = r"C:\Desenvolvimento\Nintendo 64\Modelos"
OUT_GLB = r"C:\Desenvolvimento\Nintendo 64\assets\models\spirit_wolf.glb"

wolf_fbx = os.path.join(MODEL_DIR, "Spirit Wolf.fbx")
print(f"Importing Spirit Wolf FBX: {wolf_fbx}")
bpy.ops.import_scene.fbx(filepath=wolf_fbx)

mesh_obj = None
for obj in bpy.data.objects:
    if obj.type == 'MESH':
        mesh_obj = obj
    elif obj.type in {'LIGHT', 'CAMERA'}:
        bpy.data.objects.remove(obj)

if not mesh_obj:
    raise RuntimeError(f"Could not find Mesh in {wolf_fbx}")

print(f"Spirit Wolf Mesh: {mesh_obj.name}, Vertices: {len(mesh_obj.data.vertices)}, Polys: {len(mesh_obj.data.polygons)}")

# Ensure material exists
if len(mesh_obj.data.materials) == 0:
    mat = bpy.data.materials.new(name="SpiritWolfMaterial")
    mat.use_nodes = True
    mesh_obj.data.materials.append(mat)
    print("Created fallback SpiritWolfMaterial")
else:
    print(f"Existing materials: {[m.name for m in mesh_obj.data.materials]}")

# Generate spectral vertex colors (COLOR_0)
# Height and length bounding box
z_coords = [v.co.z for v in mesh_obj.data.vertices]
y_coords = [v.co.y for v in mesh_obj.data.vertices]
min_z, max_z = min(z_coords), max(z_coords)
min_y, max_y = min(y_coords), max(y_coords)
range_z = max(max_z - min_z, 1e-4)
range_y = max(max_y - min_y, 1e-4)

# Spectral Cyan / Teal color palette:
#   Paws / belly (lower): darker ghost cyan (0.22, 0.40, 0.46, 1.0)
#   Back / crest (higher): bright spectral cyan (0.44, 0.76, 0.82, 1.0)
#   Muzzle / head (front): ethereal glow (0.50, 0.85, 0.90, 1.0)
vert_colors = []
for v in mesh_obj.data.vertices:
    nz = (v.co.z - min_z) / range_z # 0 to 1 from bottom to top
    ny = (v.co.y - min_y) / range_y # 0 to 1 from back to front
    
    # Base height gradient
    r = 0.22 + 0.22 * nz
    g = 0.40 + 0.36 * nz
    b = 0.46 + 0.36 * nz
    
    # Front / snout glow boost
    if ny > 0.65 and nz > 0.4:
        glow = (ny - 0.65) / 0.35 * 0.12
        r += glow
        g += glow * 1.1
        b += glow * 1.1
        
    vert_colors.append((min(r, 1.0), min(g, 1.0), min(b, 1.0), 1.0))

# Set vertex colors on mesh
if "COLOR_0" not in mesh_obj.data.color_attributes:
    color_layer = mesh_obj.data.color_attributes.new(name="COLOR_0", type='FLOAT_COLOR', domain='CORNER')
else:
    color_layer = mesh_obj.data.color_attributes["COLOR_0"]

for poly in mesh_obj.data.polygons:
    for loop_idx in poly.loop_indices:
        v_idx = mesh_obj.data.loops[loop_idx].vertex_index
        color_layer.data[loop_idx].color = vert_colors[v_idx]

print(f"Assigned spectral COLOR_0 to {len(mesh_obj.data.loops)} loops across {len(mesh_obj.data.vertices)} vertices")

# Select only the mesh
bpy.ops.object.select_all(action='DESELECT')
mesh_obj.select_set(True)
bpy.context.view_layer.objects.active = mesh_obj

# Export GLB
os.makedirs(os.path.dirname(OUT_GLB), exist_ok=True)
print(f"Exporting Spirit Wolf GLB to: {OUT_GLB}")

bpy.ops.export_scene.gltf(
    filepath=OUT_GLB,
    export_format='GLB',
    use_selection=True,
    export_extras=True,
    export_yup=True,
    export_animations=False,
    export_all_vertex_colors=True
)

print(f"Successfully generated {OUT_GLB} (size: {os.path.getsize(OUT_GLB)} bytes)")
