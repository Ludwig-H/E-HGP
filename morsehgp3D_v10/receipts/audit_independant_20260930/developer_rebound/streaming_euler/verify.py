"""Offline local-hash and exact-oracle rejudge; no compiler/native execution."""
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def need(ok,text):
    if not ok:
        raise RuntimeError(text)


def main():
    inventory = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        digest,name = line.split('  ',1)
        need(name not in inventory and not Path(name).is_absolute() and '..' not in Path(name).parts,
             'invalid manifest path')
        need(hashlib.sha256((ROOT/name).read_bytes()).hexdigest() == digest,'changed artifact '+name)
        inventory[name] = digest
    need(all(name in inventory for name in ('check.py','extrema.hpp','probe.cpp','capture_receipt.json')),
         'missing source/capture pin')
    spec = importlib.util.spec_from_file_location('streaming_euler_oracle',ROOT/'check.py')
    reference = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reference)
    cases = reference.make_cases()
    counts = {'cases':len(cases),'comparisons_per_mode':0}
    for mode in ('normal','optimized'):
        receipt = json.loads((ROOT/mode/'receipt.json').read_text())
        need(receipt['sources_before'] == receipt['sources_after'],'consumer source closure mismatch')
        for backend in ('release','ubsan'):
            values = [json.loads(line) for line in (ROOT/mode/(backend+'.stdout')).read_text().splitlines()]
            need(len(values) == len(cases),'wrong archived batch length')
            for i,(case,value) in enumerate(zip(cases,values)):
                need(value['case'] == i and len(value['states']) == 6 and
                     all(state == case['expected'] for state in value['states']),
                     'archived native value changed or oracle mismatch')
        mutant = json.loads((ROOT/mode/'mutant_drop_tin.stdout').read_text())['states'][0]
        need(mutant == [1,2,0],'causal mutant lost')
    a = json.loads((ROOT/'normal/receipt.json').read_text())
    b = json.loads((ROOT/'optimized/receipt.json').read_text())
    need([a.pop('optimize_flag'),b.pop('optimize_flag')] == [0,1] and a == b,'mode disagreement')
    capture = json.loads((ROOT/'capture_receipt.json').read_text())
    need(capture['dependencies_before'] == capture['dependencies_after'] and
         capture['binary_and_runtime_before'] == capture['binary_and_runtime_after'],
         'capture closure mismatch')
    print(json.dumps({'status':'PASS','cases':len(cases),'normal_optimized_equal':True,
                      'native_executions':0,'GCP_used':False},sort_keys=True))


if __name__ == '__main__':
    main()
