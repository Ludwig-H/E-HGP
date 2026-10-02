"""Execute only the pinned input-writer ASTs; no native program and no live product import."""
import ast
import json
from pathlib import Path
import struct
import tempfile

HERE = Path(__file__).resolve().parent/'full1'


def writer(path,name,scope):
    tree = ast.parse(path.read_text())
    definitions = [n for n in ast.walk(tree) if isinstance(n,ast.FunctionDef) and n.name == name]
    if len(definitions) != 1: raise ValueError('writer inventory')
    module = ast.Module(body=definitions,type_ignores=[])
    exec(compile(module,str(path),'exec'),scope)
    return scope[name]


def main():
    points,names = [(0,0,0),(2,0,0),(4,0,0)],[2**32-1,7,99]
    with tempfile.TemporaryDirectory(prefix='full-io-model-') as directory:
        xyz,ids = Path(directory)/'xyz',Path(directory)/'ids'
        scope = dict(xyz=xyz,ids=ids,points=points,names=names,struct=struct)
        old = writer(HERE/'source_80_full_bench_io.py','write',scope)
        old(reversed(range(3)))
        sizes = [xyz.stat().st_size,ids.stat().st_size]
        if sizes != [36,0]: raise ValueError('old single-use failure not reproduced')
        fixed = writer(HERE/'fixed_c6_full_bench_io.py','write_inputs',dict(struct=struct))
        for order in (range(3),reversed(range(3)),iter((2,0,1))):
            expected = tuple(order)
            fixed(xyz,ids,points,names,iter(expected))
            if xyz.read_bytes() != b''.join(struct.pack('<III',*points[i]) for i in expected):
                raise ValueError('wrong coordinates')
            if ids.read_bytes() != b''.join(struct.pack('<I',names[i]) for i in expected):
                raise ValueError('wrong identities')
    print(json.dumps(dict(positives=3,old_xyz_bytes=36,old_ids_bytes=0,
                          fixed_xyz_bytes=36,fixed_ids_bytes=12,native=0),sort_keys=True))


if __name__ == '__main__': main()
