import struct

data = open('raw_druid.t3dm', 'rb').read()
magic = data[:4]
chunk_count, total_v, total_idx, c_vert, c_idx, c_mat = struct.unpack_from('<IHHIII', data, 4)
print(f'Magic: {magic}, Chunks: {chunk_count}, totalVerts={total_v}, totalIdx={total_idx}')
print(f'c_vert={c_vert}, c_idx={c_idx}, c_mat={c_mat}')

# chunkOffsets array starts at 0x1C
chunk_offsets_start = 0x1C
vert_raw = struct.unpack_from('<I', data, chunk_offsets_start + c_vert*4)[0]
vert_off = vert_raw & 0x00FFFFFF
print(f'Vertex chunk offset: 0x{vert_off:x} ({vert_off})')

print("First 10 Vertices:")
for v in range(10):
    vx, vy, vz, norm, rgba = struct.unpack_from('<3hHI', data, vert_off + v*16)
    print(f"  V{v}: ({vx}, {vy}, {vz}) rgba=0x{rgba:08X}")

# Min and Max of all vertices:
min_x, max_x = 99999, -99999
min_y, max_y = 99999, -99999
min_z, max_z = 99999, -99999
for v in range(total_v):
    vx, vy, vz = struct.unpack_from('<3h', data, vert_off + v*16)
    min_x = min(min_x, vx)
    max_x = max(max_x, vx)
    min_y = min(min_y, vy)
    max_y = max(max_y, vy)
    min_z = min(min_z, vz)
    max_z = max(max_z, vz)

print(f"All Verts Bounds: X: [{min_x}, {max_x}], Y: [{min_y}, {max_y}], Z: [{min_z}, {max_z}]")
