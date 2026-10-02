"""Explicit small source snapshot and scalar capacity table; no large allocation."""
from pathlib import Path
import hashlib,json,subprocess,sys
ROOT=Path(__file__).resolve().parent
REPO=Path('/workspaces/E-HGP/build/v9-open-worktree')
P=REPO/'morsehgp3D_v10'
FILES=sorted((P/'src').rglob('*.hpp'))+sorted((P/'src').rglob('*.cpp'))+sorted((P/'src').rglob('*.def'))+sorted((P/'cli').glob('*.cpp'))
FILES += [P/'README.md',P/'PASSATION.md',P/'docs/DEVELOPPEMENT_FRONTIERE_ET_PRECISION_20260930.md',P/'audits/AUDIT_MASSIF_LIDAR_20260930.md',P/'receipts/audit_independant_20260930/massif/representation/SOURCE_MANIFEST.json',P/'receipts/audit_independant_20260930/massif/representation/FORMULES.json']
def sha(b):return hashlib.sha256(b).hexdigest()
mode=sys.argv[1]
entries=[]
for f in FILES:
    b=f.read_bytes(); rel=f.relative_to(REPO); dst=ROOT/'sources'/rel
    if mode=='open':
        if dst.exists() and dst.read_bytes()!=b:raise SystemExit('existing source snapshot differs')
        dst.parent.mkdir(parents=True,exist_ok=True);dst.write_bytes(b)
    entries.append({'path':str(rel),'sha256':sha(b),'bytes':len(b),'copy_sha256':sha(dst.read_bytes())})
old=json.loads((P/'receipts/audit_independant_20260930/massif/representation/SOURCE_MANIFEST.json').read_text())['artifacts_before']
comp={str(f.relative_to(REPO)):sha(f.read_bytes())==old[str(f.relative_to(REPO))]['sha256'] for f in FILES if str(f.relative_to(REPO)) in old and '/src/' in str(f)}
latest={str(f.relative_to(REPO)):sha(f.read_bytes())==sha(subprocess.check_output(['git','-C',str(REPO),'show','4b7d70422:'+str(f.relative_to(REPO))])) for f in FILES if '/src/' in str(f) or '/cli/' in str(f)}
changed=[e['path'] for e in entries if e['sha256']!=e['copy_sha256']]
allowed={'morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md'}
out={'mode':mode,'head_readonly':subprocess.check_output(['git','-C',str(REPO),'rev-parse','HEAD'],text=True).strip(),'files':entries,'old_product_source_hash_comparison':comp,'source_vs_4b7d70422':latest,'all_named_sources_match_copy':not changed,'changed_named_paths':changed,'product_and_compilation_sources_match_copy':all(e['sha256']==e['copy_sha256'] for e in entries if '/src/' in e['path'] or '/cli/' in e['path']),'coordination_note_change_scope':'Root intentionally updated live AUDIT_MASSIF during closure; initial note bytes preserved, current hash recorded; not used by native compilation.'}
(ROOT/f'SOURCE_{"BEFORE" if mode=="open" else "AFTER"}.json').write_text(json.dumps(out,indent=2)+'\n')
if set(changed)-allowed:raise SystemExit('unexpected source changed')
print(json.dumps({'mode':mode,'files':len(entries),'old_source_hashes_compared':len(comp),'unchanged':all(comp.values())}))
