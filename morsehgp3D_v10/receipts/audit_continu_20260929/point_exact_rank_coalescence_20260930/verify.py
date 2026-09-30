from pathlib import Path
from fractions import Fraction as F
import contextlib, hashlib, io, json, runpy

r=Path(__file__).resolve().parent
manifest=(r/'SHA256SUMS').read_text().splitlines()
seen=set()
for line in manifest:
    digest,name=line.split('  ',1)
    if name in seen or '/' in name or not (r/name).is_file() or hashlib.sha256((r/name).read_bytes()).hexdigest()!=digest:
        raise SystemExit('archive hash mismatch: '+name)
    seen.add(name)
expected={p.name for p in r.iterdir() if p.is_file() and p.name not in {'SHA256SUMS','probe','probe_oblique','probe_scan'}}
if expected!=seen or (r/'native_pins_before.sha256').read_bytes()!=(r/'native_pins_after.sha256').read_bytes():
    raise SystemExit('inventory or native pin subset mismatch')
out=io.StringIO()
with contextlib.redirect_stdout(out): runpy.run_path(str(r/'fraction_oblique.py'),run_name='__main__')
if out.getvalue()!=(r/'fraction_oblique.stdout.json').read_text(): raise SystemExit('fraction replay mismatch')
d=json.loads(out.getvalue())
if not F(d['base'])<F(d['high']) or d['coordinates']!=[[0,0,0],[261120,2,0],[1,512,0]]: raise SystemExit('wrong geometry')
lines=(r/'scan.stdout.txt').read_text().splitlines()
if (r/'scan.code').read_text()!='0\n' or lines[0]!='chosen b=261120,2 z=1,512' or 'same_double=1' not in lines[1] or len(lines)!=11 or lines[-1]!='coalescence_ok entries=2 native_full_exact_comparison_preserved=1 point_ranks_coalesced=1':
    raise SystemExit('native capture mismatch')
for entry,rank in [('core','3'),('cover','2')]:
    nodes=[s for s in lines if s.startswith('node entry='+entry+' ')]
    pair=[s for s in nodes if s.endswith('pair=1 triangle=0')]
    tri=[s for s in nodes if s.endswith('pair=0 triangle=1')]
    if len(nodes)!=4 or len(pair)!=1 or len(tri)!=1 or 'exact_rank=3 ' not in pair[0] or 'exact_rank=4 ' not in tri[0] or any('rendered_rank='+rank+' ' not in s for s in pair+tri):
        raise SystemExit('rank capture mismatch')
print(json.dumps(dict(status='ARCHIVE_POINT_RANK_COALESCENCE_OBSERVED',files=len(seen),fraction_replays=1,native_replays=0,GCP=False)))
