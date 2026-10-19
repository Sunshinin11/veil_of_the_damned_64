import bpy
import addon_utils
import os

addon_utils.disable("fast64", default_set=False)
if hasattr(bpy.app.handlers, 'frame_change_pre'):
    bpy.app.handlers.frame_change_pre.clear()

bpy.ops.wm.read_factory_settings(use_empty=True)

DRUID_DIR = r"C:\Desenvolvimento\Nintendo 64\Modelos\Druid"

# Check FBX importer options:
# If we set global_scale or use transform_apply
bpy.ops.import_scene.fbx(filepath=os.path.join(DRUID_DIR, "Idle.fbx"))
arm = [o for o in bpy.data.objects if o.type == 'ARMATURE'][0]
mesh = [o for o in bpy.data.objects if o.type == 'MESH'][0]

print("Imported:")
print("  Arm scale:", arm.scale)
print("  Mesh scale:", mesh.scale)
print("  Mesh parent:", mesh.parent)
print("  Mesh parent_type:", mesh.parent_type)
print("  Mesh matrix_parent_inverse:")
print(mesh.matrix_parent_inverse)
