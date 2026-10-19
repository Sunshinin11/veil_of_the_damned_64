import struct

data = open('filesystem/applied_ibp.t3dm', 'rb').read()
c_vert = struct.unpack_from('>I', data, 16)[0]
off_pos = 32 + c_vert * 4
raw = struct.unpack_from('>I', data, off_pos)[0]
type_char = chr(data[off_pos])
off = int.from_bytes(data[off_pos+1 : off_pos+4], 'big')
total_v = struct.unpack_from('>H', data, 8)[0]
print(f"c_vert index: {c_vert}, off_pos: 0x{off_pos:x}")
print(f"raw: 0x{raw:08x}, Type: '{type_char}', offset: 0x{off:x} ({off}), totalVerts: {total_v}")

min_x, max_x = 99999, -99999
min_y, max_y = 99999, -99999
min_z, max_z = 99999, -99999

for v in range(total_v):
    vx, vy, vz = struct.unpack_from('>3h', data, off + v*16)
    min_x = min(min_x, vx); max_x = max(max_x, vx)
    min_y = min(min_y, vy); max_y = max(max_y, vy)
    min_z = min(min_z, vz); max_z = max(max_z, vz)

print(f"BOUNDS: X: [{min_x}, {max_x}], Y: [{min_y}, {max_y}], Z: [{min_z}, {max_z}]")
print(f"Height in T3D units: {max_y - min_y} (or Z: {max_z - min_z})")
