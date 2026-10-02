"""Pure reader controls: real archives read-only, tiny synthetic payload, memory corruptions."""
import copy
import json
from pathlib import Path
import re
import struct
from unittest.mock import patch

import check as reader


def main():
    folder = reader.HERE/'sweep2'
    receipt,worker,data = reader.old.read_capture(folder)
    manifest,mhash = reader.old.inputs(folder,receipt)
    _,builds,_ = reader.q4.judge_matrix(data)
    full = reader.js(data[reader.BENCH])
    previous = reader.js((reader.baseline.HERE/'full3/full.json').read_bytes())
    positives,refused = 0,0

    def reject(action):
        nonlocal refused
        try: action()
        except reader.REFUSALS+(ValueError,KeyError,TypeError,IndexError,OSError):
            refused += 1; return
        raise ValueError('corruption accepted')

    def judge(value):
        return reader.report(value,manifest,mhash,builds,data)

    reader.need(judge(full)['conforming'] is False,'calendar_failure_positive'); positives += 1
    reader.need(reader.compare_baseline(full,previous)==13,'baseline_positive'); positives += 1
    changed = copy.deepcopy(previous)
    for row in changed['runs']:
        for order in row['events'][2]['orders']:
            order['work']['ancestor_hops'] += 1
            order['work']['part_meb_presentations'] += 1
    reader.need(reader.compare_baseline(full,changed)==13,'nonconserved_counters_not_compared'); positives += 1
    reader.need(reader.mutants(data)==221,'mutants_positive'); positives += 1
    reader.required_gates(data); positives += 1

    mutations = [
        lambda v:v.update(conforming=True),lambda v:v.update(complete=False),
        lambda v:v.update(full_schedule_completed=True),lambda v:v.update(work_schema='ehgp.v11.full_work.v3'),
        lambda v:v.update(schema='ehgp.v11.full_campaign.v4'),lambda v:v.update(optimizations=0),
        lambda v:v['runs'].pop(),lambda v:v['runs'].append(copy.deepcopy(v['runs'][0])),
        lambda v:v['runs'].reverse(),lambda v:v['not_run'].pop(),
        lambda v:v['not_run'][0].update(reason='same_profile_K5_first_attempt_failed'),
        lambda v:v['not_run'][0].update(coord_bits=18),lambda v:v['requested'][0].update(coord_bits=True),
        lambda v:v['launch_intents'].pop(),lambda v:v['launch_intents'][0].update(input_sha256='0'*64),
        lambda v:v.update(manifest_sha256='0'*64),lambda v:v.update(qualification_sha256='0'*64),
        lambda v:v.update(supplement_sha256='0'*64),lambda v:v['builds'][0].update(sha256='0'*64),
        lambda v:v['builds'][0].update(bytes=True),lambda v:v.update(campaign_wall_seconds=float('nan')),
        lambda v:v.update(campaign_wall_seconds=1),lambda v:v['runs'][0].update(process_wall_seconds=-1),
        lambda v:v['runs'][0].update(semantic_wall_seconds=True),lambda v:v['runs'][0].update(status='timeout'),
        lambda v:v['runs'][0].update(exit_code=True),lambda v:v['runs'][0].update(stderr='diagnostic'),
        lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),lambda v:v['runs'][0]['argv'].__setitem__(11,'0'),
        lambda v:v['runs'][0].update(stdout=v['runs'][0]['stdout']+'{}\n'),
        lambda v:v['runs'][0].update(full_within_200ms=True),lambda v:v['runs'][0]['semantic'].update(bytes=1),
        lambda v:v['runs'][0]['semantic'].update(nodes=v['runs'][0]['semantic']['nodes']+1),
        lambda v:v['runs'][0]['semantic']['orders'][0].update(root=2**32-1),
        lambda v:v['runs'][0]['order_stage_ms'][0].update(classify=1e12),
        lambda v:v['comparisons'][0].update(status='equal')]
    for mutate in mutations:
        value = copy.deepcopy(full); mutate(value); reject(lambda:judge(value))

    # Keep stdout/events consistent so these controls reach native diagnostics.
    for mutate in (
        lambda e:e[2].update(forest_ns=True),lambda e:e[2].update(cpu_seconds=float('nan')),
        lambda e:e[2].update(optimizations=0),lambda e:e[2].update(peak_reserved_bytes=2**64),
        lambda e:e[2]['orders'][0]['timings'].update(classify_ns=True),
        lambda e:e[2]['orders'][0]['timings'].update(classify_ns=2**63),
        lambda e:e[2]['orders'][0]['timings'].update(verticals_ns=1),
        lambda e:e[2]['orders'][1]['work'].update(classification_meb_calls=2**63),
        lambda e:e[2]['orders'][1]['work'].update(classification_examined=2**63),
        lambda e:e[2]['orders'][1]['work'].update(replay_meb_calls=2**63),
        lambda e:e[2]['orders'][1]['work'].update(ancestor_hops=1),
        lambda e:e[2]['orders'][1]['work'].update(ancestor_queries=0),
        lambda e:e[2]['orders'][1]['work'].update(ancestor_activations=2**32),
        lambda e:e[2]['orders'][1]['work'].update(ancestor_unions=2**32),
        lambda e:e[2]['orders'][1]['work'].update(ancestor_find_steps=True),
        lambda e:e[2]['orders'][1]['work'].update(vertical_checks=0,ancestor_queries=e[2]['orders'][1]['births']),
        lambda e:e[2]['orders'][1].update(node_capacity=0)):
        value = copy.deepcopy(full); mutate(value['runs'][0]['events'])
        value['runs'][0]['stdout'] = ''.join(json.dumps(e)+'\n' for e in value['runs'][0]['events'])
        reject(lambda:judge(value))

    for mutate in (
        lambda r:r['semantic'].update(raw_sha256='0'*64),lambda r:r['semantic'].update(sha256='0'*64),
        lambda r:r['events'][1].update(catalogue_balls=1),
        lambda r:r['events'][2]['orders'][1]['work'].update(descent_steps=1),
        lambda r:r['events'][2]['orders'][1]['work'].update(vertical_checks=1)):
        value = copy.deepcopy(full); mutate(value['runs'][0]); reject(lambda:reader.compare_baseline(value,previous))

    # A tiny real encoding exercises the optional archived-payload path; no live bench import.
    words = [21,1,1,1,0,0,0,1,7,1,1,1,0,0,2**32-1,0,0]
    for value in (0,1,0,0,0,1): words += [0,1,value]
    raw = b'MHGP11FUL1'+struct.pack('<'+'Q'*len(words),*words)
    semantic = reader.driver.semantic.decode(raw,21,1,1)
    row = copy.deepcopy(full['runs'][0]); row.update(kmax=1,count=1,semantic=semantic,errors=[])
    case = dict(manifest['cases'][0],name=row['case'],count=1)
    case.update(coordinates=Path(row['argv'][1]).name,point_ids=Path(row['argv'][2]).name)
    row['argv'][3] = str(Path(row['argv'][3]).with_name('%s_b21_k1_w48_r0_o3.bin' % row['case']))
    row['argv'][4] = '1'; row['events'][0].update(points=1,sites=1)
    event = row['events'][2]; event.update(kmax=1)
    order = copy.deepcopy(event['orders'][0])
    order.update(k=1,births=1,nodes=1,edges=0,verticals=0,node_capacity=1,edge_capacity=0,
                 timings={k:0 for k in reader.driver.ORDER_TIMINGS},work={k:0 for k in reader.driver.WORK})
    event['orders'] = [order]
    with patch.object(reader.driver.semantic,'inspect',lambda *_:semantic):
        reader.driver.collect(row,case,None,21)
    row['stdout'] = ''.join(json.dumps(e)+'\n' for e in row['events'])
    request = {k:row[k] for k in reader.driver.schedule(3)[0]}
    intent = dict(full['launch_intents'][0],**request,count=1,argv=row['argv'],
                  input_sha256=case['sha256'],ids_sha256=case['ids_sha256'])
    build = next(b for b in full['builds'] if b['coord_bits']==21)
    path = 'results/cmd/002_full/files/'+Path(row['argv'][3]).name
    reader.need(reader.attempt(row,intent,request,case,build,{path:raw})==1,'archived_positive'); positives += 1
    reject(lambda:reader.attempt(row,intent,request,case,build,{path:raw+b'!'}))
    reject(lambda:reader.attempt(row,intent,request,case,build,{path:raw,path.replace('/files/','/duplicate/'):raw}))

    # Raw and compact receipt are altered consistently: rejection reaches closure,
    # rather than relying on their preexisting hash mismatch.
    original_read = Path.read_bytes
    rawpath = Path(receipt['raw_receipt_local'])
    for change in (lambda v:v.update(closure='running'),lambda v:v.update(targeted_shutdown_certified=False),
                   lambda v:v.update(results_verified=False),
                   lambda v:v['observed_after'].update(lastStartTimestamp='wrong-generation')):
        raw_receipt = reader.js(original_read(rawpath)); change(raw_receipt)
        raw_bytes = json.dumps(raw_receipt).encode(); compact = copy.deepcopy(receipt); change(compact)
        compact['original_receipt_sha256'] = reader.sha(raw_bytes)
        def read_bytes(path):
            if path == rawpath: return raw_bytes
            if path == folder/'receipt.json': return json.dumps(compact).encode()
            return original_read(path)
        with patch.object(Path,'read_bytes',read_bytes): reject(lambda:reader.old.read_capture(folder))
    for name in ('source_contract.json','source_12f49/full_campaign.py'):
        target = reader.HERE/name
        with patch.object(Path,'read_bytes',lambda p:original_read(p)+(b'\n' if p==target else b'')):
            reject(reader.frozen)
    changed = dict(data)
    for path,raw in data.items():
        if path.startswith(reader.BASE+'mutants/'):
            changed[path] = re.sub(rb'(order_timings_prematures\s+TUE\s+)code',rb'\1signal',raw)
    reader.need(changed != data,'causal_mutation_applied')
    reject(lambda:reader.mutants(changed))
    for key in ('private_key_deleted','oslogin_key_removed','reserve_released'):
        altered = dict(receipt,**{key:False})
        with patch.object(reader.old,'read_capture',lambda *_:(altered,worker,data)):
            reject(reader.check)
    for key in ('results/cmd/002_full/meta.txt','results/cmd/000_matrice/meta.txt'):
        altered = dict(data); altered[key] = altered[key].replace(b'group_closed=1',b'group_closed=0')
        inventory = []
        for path,raw in altered.items():
            if path != 'results/MANIFEST.sha256': inventory.append(reader.sha(raw)+'  ./'+path[len('results/'):])
        altered['results/MANIFEST.sha256'] = ('\n'.join(inventory)+'\n').encode()
        with patch.object(reader.old,'read_capture',lambda *_:(receipt,worker,altered)):
            reject(reader.check)
    reader.need(positives==6 and refused==72,'selftest_inventory')
    print(json.dumps(dict(verdict='conforme',positives=positives,corruptions=refused,native=0),sort_keys=True))


if __name__ == '__main__':
    main()
