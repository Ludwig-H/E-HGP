#!/usr/bin/env python3
"""Pure publication binding check, no native execution."""
import hashlib
import json
from math import comb
from pathlib import Path

root=Path(__file__).resolve().parent
binding=json.loads((root/'BINDINGS.json').read_text())
checks=0

def check(ok,message):
    global checks
    checks+=1
    if not ok:
        raise RuntimeError(message)

check(binding['commit']=='77db5738eb2dd5bc84ecdc4d85ade833124c58f8','commit pin')
check(len(binding['rows'])==12 and len({r['path'] for r in binding['rows']})==12,'12 distinct paths')
for row in binding['rows']:
    check(row['identical']==(row['sha256_snapshot']==row['sha256_published']),'identity table')
check(binding['identical_count']==11,'eleven identical')
check(binding['different_paths']==['src/catalogue/leaf_device_predicates.hpp'],'only predicate header changed')
previous=(root/'predicates.snapshot.hpp').read_bytes()
current=(root/'predicates.published.hpp').read_bytes()
row=next(r for r in binding['rows'] if not r['identical'])
check(hashlib.sha256(previous).hexdigest()==row['sha256_snapshot'],'snapshot header hash')
check(hashlib.sha256(current).hexdigest()==row['sha256_published'],'published header hash')
start=current.index(b'// Compteurs d\'une feuille de m<=kMaxSites sites')
end=current.index(b'inline constexpr u32 kNoSite',start)
addition=current[start:end]
check(current[:start]+current[end:]==previous,'all remaining bytes identical')
check(len(addition.splitlines())==8,'exact eight line insertion')
check(b'inline constexpr u64 kPrefixBound = 32 + 496 + 4960 + 35960;' in addition,'prefix constant')
prefix=sum(comb(32,q) for q in (1,2,3,4))
count=32*3*prefix
check(prefix==41448 and count==3979008,'counter arithmetic')
check(count<2**22 and count*2**32<2**54,'batch bound conditional on at most2^32 leaves')
print(json.dumps({'status':'PASS','checks':checks,'commit':binding['commit'],'identical_sources':11,
                  'total_sources':12,'inserted_lines':8,'remaining_bytes_identical':True,
                  'prefix_bound':prefix,'leaf_counter_bound':count,
                  'conditional_batch_bound_bits':54,
                  'scope':'publication/source binding and scalar count arithmetic; no native qualification'},
                 sort_keys=True,indent=2))
