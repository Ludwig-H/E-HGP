"""Contre-tests du lecteur full3, archives en lecture et mutations en memoire seulement."""
import copy
import json
from pathlib import Path
from unittest.mock import patch

import check_full as reader


def main():
    receipt, _, data = reader.old.read_capture(reader.HERE/'full3')
    manifest, manifest_hash = reader.old.inputs(reader.HERE/'full3',receipt)
    _, builds, _ = reader.q4.judge_matrix(data)
    full = reader.js(data[reader.BENCH+'files/full.json'])
    diff = reader.js(data[reader.DIFF+'summary.json'])
    positives, refused = 0, 0

    def judge(value):
        return reader.report(value,manifest,manifest_hash,builds,data)

    def reject(action):
        nonlocal refused
        try:
            action()
        except (reader.old.foundation.Refusal,ValueError,KeyError,TypeError,IndexError):
            refused += 1
            return
        raise ValueError('corruption non refusee')

    result = judge(copy.deepcopy(full)); reader.need(result['conforming'] is False,'failed_positive'); positives += 1
    reader.need(reader.differential(copy.deepcopy(diff),data,builds) == 42,'diff_positive'); positives += 1
    reader.supplement_provenance(data); positives += 1
    mutations = [
        lambda v:v.update(conforming=True), lambda v:v.update(complete=False),
        lambda v:v.update(full_schedule_completed=True), lambda v:v['runs'].pop(),
        lambda v:v['runs'].append(copy.deepcopy(v['runs'][0])), lambda v:v['runs'].reverse(),
        lambda v:v['not_run'].pop(), lambda v:v['not_run'][0].update(reason='same_profile_K5_first_attempt_failed'),
        lambda v:v['requested'][0].update(coord_bits=True), lambda v:v['launch_intents'].pop(),
        lambda v:v['launch_intents'][0].update(input_sha256='0'*64),
        lambda v:v.update(manifest_sha256='0'*64), lambda v:v.update(qualification_sha256='0'*64),
        lambda v:v['builds'][0].update(sha256='0'*64), lambda v:v['builds'][0].update(bytes=True),
        lambda v:v.update(campaign_wall_seconds=float('nan')),
        lambda v:v.update(campaign_wall_seconds=1), lambda v:v['runs'][0].update(process_wall_seconds=-1),
        lambda v:v['runs'][0].update(semantic_wall_seconds=True),
        lambda v:v['runs'][0].update(status='timeout'), lambda v:v['runs'][0].update(exit_code=True),
        lambda v:v['runs'][0].update(stderr='diagnostic'), lambda v:v['runs'][0]['argv'].__setitem__(10,'8'),
        lambda v:v['runs'][0].update(stdout=v['runs'][0]['stdout']+'{}\n'),
        lambda v:v['runs'][0]['events'][2].update(forest_ns=True),
        lambda v:v['runs'][0].update(full_within_200ms=True),
        lambda v:v['runs'][0]['semantic'].update(bytes=1),
        lambda v:v['runs'][0]['semantic'].update(nodes=v['runs'][0]['semantic']['nodes']+1),
        lambda v:v['runs'][0]['semantic']['orders'][0].update(root=2**32-1),
        lambda v:v['runs'][0]['semantic'].update(sha256='0'*64),
        lambda v:v['comparisons'][0].update(status='equal'),
    ]
    for mutate in mutations:
        value = copy.deepcopy(full); mutate(value); reject(lambda:judge(value))
    for mutate in (
        lambda v:v.update(archive_sha256='0'*64), lambda v:v['cases'].pop(), lambda v:v['cases'][0]['v11'].pop(),
        lambda v:v['cases'][0]['v11'][0].update(equal=False),
        lambda v:v['cases'][0]['v11'][0].update(sha256='0'*64),
        lambda v:v['commands'][0].update(returncode=1),
        lambda v:v['commands'].pop(), lambda v:v.update(complete=False),
        lambda v:v['v11_binaries']['21'].update(coord_bits=24)):
        value = copy.deepcopy(diff); mutate(value)
        reject(lambda:reader.differential(value,data,builds))
    changed = dict(data)
    changed[reader.DIFF+'00_singleton.v11_18.common'] += b'!'
    reject(lambda:reader.differential(diff,changed,builds))
    key = reader.SUPP+'gcc_asan_ubsan18/build_provenance.json'
    for mutate in (lambda v:v.update(complete=False),lambda v:v['files'].append(v['files'][0]),
                   lambda v:v['files'][0].update(sha256='0'*64)):
        changed = dict(data); proof = reader.js(changed[key]); mutate(proof); changed[key] = json.dumps(proof).encode()
        reject(lambda:reader.supplement_provenance(changed))
    read_bytes = Path.read_bytes
    for name in ('full3_contract.json','source_c6/full_campaign.py'):
        target = reader.HERE/name
        with patch.object(Path,'read_bytes',lambda self:read_bytes(self)+(b'\n' if self == target else b'')):
            reject(reader.frozen_driver)
    reader.need(positives == 3 and refused == 46,'selftest_inventory')
    print(json.dumps(dict(verdict='conforme',positives=positives,corruptions=refused,native=0),sort_keys=True))


if __name__ == '__main__':
    main()
