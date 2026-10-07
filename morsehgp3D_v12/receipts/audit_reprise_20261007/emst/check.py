#!/usr/bin/env python3
"""Replay the pinned independent EMST audit with CST-0232's new refusal, then identity permutations."""
import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[4]
HISTORICAL = ROOT / 'morsehgp3D_v12/receipts/audit_juges_emst_20261007/emst/check.py'
HISTORICAL_SHA = '0fdf408bd8dc5af28bdecd9e0ef55acac94105864e5a4563f0847340c1859be7'
PIN = 'f601b36ace16bcc8f7ac9bc532e45ab5079f9667'


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise RuntimeError('historical replay anchor changed: ' + old)
    return source.replace(old, new, 1)


# The complete graph, Kruskal, canonical FULL writer and 24 clouds remain byte-for-byte the old script.
# Only its pinned sources, corrected expectation, and handling of pre-comparison refusals change.
raw = HISTORICAL.read_bytes()
if hashlib.sha256(raw).hexdigest() != HISTORICAL_SHA:
    raise RuntimeError('historical independent script changed')
source = raw.decode()
source = replace_once(source, "PIN = '1f7642e105aebd76632c58c63fdfd5b5c0824779'", 'PIN = ' + repr(PIN))
source = replace_once(source,
                      "('ids_duplicate_with_reference',full(points,fs,[17,17]),[17,17],0)",
                      "('ids_duplicate_with_reference',full(points,fs,[17,17]),[17,17],3)")
source = replace_once(source, "'ids':labels,'code':code,'verdict':doc['vidage']}",
                      "'ids':labels,'code':code,'verdict':doc['vidage'] if doc else None,'stderr':err}")

EXTRA = r'''
    from itertools import permutations
    identity_rows=[]
    def identity(name,payload,labels,want,input_points):
        cloud.write_bytes(b''.join(struct.pack('<III',*p) for p in input_points))
        dump.write_bytes(payload)
        arguments=[cloud,'--vidage',dump]
        if labels is not None:
            ids.write_bytes(labels if isinstance(labels,bytes) else b''.join(struct.pack('<I',x) for x in labels))
            arguments+=['--ids',ids]
        code,doc,err=run(arguments)
        require(code==want,name+' unexpected code '+str(code)+' '+err)
        identity_rows.append(dict(name=name,code=code,verdict=doc['vidage'] if doc else None,
                                  stderr=err.replace(str(work),'<temporary>'),
                                  payload_sha256=sha(payload),reference_present=labels is not None))
    points=[(0,0,2),(0,2,0),(2,0,0)]
    fs=graph_tree(points)
    external=[0,NONE,17]
    repeated=[17,NONE,17]
    good=full(points,fs,external)
    bad=full(points,fs,repeated)
    for perm in permutations(range(3)):
        suffix=''.join(map(str,perm))
        inp=[points[i] for i in perm]
        ref=[external[i] for i in perm]
        dup=[repeated[i] for i in perm]
        wrong=ref[1:2]+ref[:1]+ref[2:]
        identity('permutation_positive_'+suffix,good,ref,0,inp)
        identity('permutation_no_reference_'+suffix,good,None,0,inp)
        identity('permutation_wrong_reference_'+suffix,good,wrong,1,inp)
        identity('duplicate_reference_good_dump_'+suffix,good,dup,3,inp)
        identity('duplicate_dump_unique_reference_'+suffix,bad,ref,3,inp)
        identity('duplicate_dump_no_reference_'+suffix,bad,None,3,inp)
    identity('reference_short',good,external[:2],3,points)
    identity('reference_long',good,external+[5],3,points)
    identity('reference_trailing_byte',good,b''.join(struct.pack('<I',x) for x in external)+b'x',3,points)
    identity('reference_empty',good,b'',3,points)
    identity('dump_id_above_u32_without_reference',full(points,fs,[0,2**32,17]),None,3,points)
    identity('dump_id_above_u32_with_reference',full(points,fs,[0,2**32,17]),external,3,points)
    # The new reference guard also applies without a --vidage.
    cloud.write_bytes(b''.join(struct.pack('<III',*p) for p in points))
    ids.write_bytes(b''.join(struct.pack('<I',x) for x in repeated))
    code,doc,err=run([cloud,'--ids',ids])
    require(code==3 and doc is None,'duplicate reference without dump refused before EMST')
    identity_rows.append(dict(name='duplicate_reference_without_dump',code=code,verdict=None,stderr=err))
    official_cmd=[sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
    official_cmd += [str(BASE/'tests/temoins.py'),'--juge',str(binary),'--travail',str(work/'official')]
    official=subprocess.run(official_cmd,capture_output=True,text=True,timeout=30)
    require(official.returncode==0,'official bounded witnesses '+official.stdout+official.stderr)
    require('141' in official.stdout,'official control floor')
    official_result=dict(code=official.returncode,stdout=official.stdout,stderr=official.stderr)
'''
source = replace_once(source, '\nafter={p:sha((ROOT/p).read_bytes()) for p in sources}',
                      EXTRA + '\nafter={p:sha((ROOT/p).read_bytes()) for p in sources}')
source = replace_once(source, "'historical_nine_dumps_replayed':False}",
                      "'historical_nine_dumps_replayed':False,'identity_checks':identity_rows," +
                      "'additional_cli_calls':len(identity_rows),'official_bounded_witnesses':official_result}")

capture = io.StringIO()
namespace = {'__file__': __file__, '__name__': '__independent_replay__', 'sys': sys}
with contextlib.redirect_stdout(capture):
    exec(compile(source, str(HISTORICAL), 'exec'), namespace)
result = json.loads(capture.getvalue())
result['historical_script_sha256'] = HISTORICAL_SHA
result['schema'] = 'ehgp.v12.audit_reprise.emst.v1'
result['total_independent_cli_calls'] = result['actual_cli_calls'] + result['additional_cli_calls']
print(json.dumps(result, sort_keys=True, separators=(',', ':')))
