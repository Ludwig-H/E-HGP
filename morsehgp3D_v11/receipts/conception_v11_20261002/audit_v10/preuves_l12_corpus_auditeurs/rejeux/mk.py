import struct, sys
def write(path, pts, extra=b''):
    with open(path, 'wb') as f:
        for p in pts:
            f.write(struct.pack('<3I', *p))
        f.write(extra)
write('three.u32le', [(0,0,0),(2,0,0),(5,0,0)])
two = [(0,0,0),(8,0,0)]
for extra in (1,4,8,11):
    write('two_plus_%d.u32le' % extra, two, b'\x01' * extra)
write('two.u32le', two)
L = 262143
write('corners.u32le', [(a,b,c) for a in (0,L) for b in (0,L) for c in (0,L)])
write('four.u32le', [(0,0,0),(8,0,0),(0,8,0),(0,0,8)])
print('ok')
