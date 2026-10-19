import bpy
import addon_utils
import os

addon_utils.disable("fast64", default_set=False)
if hasattr(bpy.app.handlers, 'frame_change_pre'):
    bpy.app.handlers.frame_change_pre.clear()

bpy.ops.wm.read_factory_settings(use_empty=True)

DRUID_DIR = r"C:\Desenvolvimento\Nintendo 64\Modelos\Druid"

# Import Idle
bpy.ops.import_scene.fbx(filepath=os.path.join(DRUID_DIR, "Idle.fbx"))
arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
mesh = [o for o in bpy.data.objects if o.type == 'MESH'][0]

print("BEFORE APPLY:")
print("Arm scale:", arm.scale)
print("Mesh scale:", mesh.scale)
print("Mesh dims:", mesh.dimensions)
print("Hips bone head:", arm.data.bones['mixamorig:Hips'].head)

# Apply scale
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True)
mesh.select_set(True)
bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

print("AFTER APPLY:")
print("Arm scale:", arm.scale)
print("Mesh scale:", mesh.scale)
print("Mesh dims:", mesh.dimensions)
print("Hips bone head:", arm.data.bones['mixamorig:Hips'].head)

# Test evaluation at frame 1 and 30
bpy.context.scene.frame_set(1)
dg = bpy.context.evaluated_depsgraph_get()
eval_mesh = mesh.evaluated_get(dg)
eval_verts_z = [v.co.z for v in eval_mesh.data.vertices]
print(f"Frame 1 Evaluated mesh Z: min={min(eval_verts_z):.3f}, max={max(eval_verts_z):.3f}, height={max(eval_verts_z)-min(eval_verts_z):.3f}")

bpy.context.scene.frame_set(30)
dg = bpy.context.evaluated_depsgraph_get()
eval_mesh = mesh.evaluated_get(dg)
eval_verts_z = [v.co.z for v in eval_mesh.data.vertices]
print(f"Frame 30 Evaluated mesh Z: min={min(eval_verts_z):.3f}, max={max(eval_verts_z):.3f}, height={max(eval_verts_z)-min(eval_verts_z):.3f}")
