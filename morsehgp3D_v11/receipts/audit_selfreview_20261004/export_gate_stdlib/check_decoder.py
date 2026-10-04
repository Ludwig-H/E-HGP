#!/usr/bin/env python3
"""Execute only frozen read_levels via AST, on synthetic buffers, using stdlib.
No exporter execution, product import, NumPy, fit, native test or GCP.
"""
import ast
from fractions import Fraction
import hashlib
import json
from pathlib import Path
import struct
import tempfile
HERE=Path(__file__).resolve().parent
checks=0
def need(c,m):
 global checks
 checks+=1
 if not c: raise RuntimeError(m)
source=HERE/'source/morsehgp3D_v11/bench/points_export_width_gate.py'
raw=source.read_bytes();tree=ast.parse(raw)
imports=set()
for node in tree.body:
 if isinstance(node,ast.Import): imports.update(a.name for a in node.names)
 elif isinstance(node,ast.ImportFrom): imports.add(node.module)
need(imports<= {'argparse','fractions','pathlib','struct','subprocess','sys'},'stdlib-only frozen imports')
fn=next(n for n in tree.body if isinstance(n,ast.FunctionDef) and n.name=='read_levels')
env={'Path':Path,'struct':struct}
exec(compile(ast.Module(body=[fn],type_ignores=[]),str(source),'exec'),env)
read_levels=env['read_levels']
mask=(1<<64)-1

def encode(version,bits,n,orders,levels):
 words=3 if version==1 else 4
 prefix=[version,bits,4,n,len(levels),len(orders)]+list(orders)
 # Nonzero patterns prevent an accidental site/header read from matching the expected levels.
 prefix += [((j+1)*0x100000001)&mask for j in range(4*n)]
 data=b'MHGP11PH'+struct.pack('<%dQ'%len(prefix),*prefix)
 for num,den in levels:
  need(0<=num<1<<(64*words) and 0<den<1<<(64*words),'synthetic values representable')
  data+=struct.pack('<%dQ'%(2*words),*[(value>>(64*j))&mask for value in (num,den) for j in range(words)])
 return data
fixtures=0;tetra=[]
with tempfile.TemporaryDirectory(prefix='mhgp-width-stdlib-') as td:
 path=Path(td)/'synthetic.ph'
 for bits in (18,21,24):
  version=2 if bits==24 else 1; words=4 if version==2 else 3
  side=(1<<bits)-1
  num,den=12*side**8,16*side**6
  need(Fraction(num,den)==Fraction(3*side*side,4),'raw tetra ratio')
  if bits==24: need((num.bit_length(),den.bit_length())==(196,148),'actual wide tetra words')
  tetra.append({'bits':bits,'version':version,'words':words,'num_bits':num.bit_length(),'den_bits':den.bit_length()})
  levels=[(0,1),(1,1),(num,den),((1<<(64*words))-1,(1<<(64*words-1))+123)]
  for n in (0,1,4,10):
   for orders in ((),(1,),(2,4),(1,2,3,4)):
    data=encode(version,bits,n,orders,levels)
    path.write_bytes(data)
    out=read_levels(path)
    need(out==dict(version=version,bits=bits,words=words,levels=levels),'offset and little-endian limbs')
    fixtures+=1
    # Every profile must reject an incomplete last limb; struct.error is expected, no success.
    path.write_bytes(data[:-1])
    try: read_levels(path)
    except struct.error: pass
    else: raise RuntimeError('truncated last limb accepted')
    need(True,'truncated limb refused')
 # Named malformed cases.
 for data,kind in ((b'BADMAGIC',ValueError),(b'MHGP11PH',struct.error),
                   (b'MHGP11PH'+struct.pack('<6Q',3,24,4,0,0,0),ValueError)):
  path.write_bytes(data)
  try: read_levels(path)
  except kind: pass
  else: raise RuntimeError('malformed case accepted')
  need(True,'bad magic/header/version refuses')
 # Width regression: the complete u24 raw numerator does not fit three words.
 need(12*((1<<24)-1)**8>=1<<192,'u24 numerator really exceeds old three words')
print(json.dumps({'scope':'AST source decoder on synthetic buffers only','checks':checks,'valid_fixtures':fixtures,
 'truncated_fixtures':fixtures,'profiles':tetra,'source_sha256':hashlib.sha256(raw).hexdigest(),
 'stdlib_only_imports':sorted(imports),'native_execution':False,'G4_qualification_transferred':False},indent=2,sort_keys=True))
