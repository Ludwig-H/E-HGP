"""Read-only archive replay; hash every input before importing an oracle."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def hash_file(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def is_digest(value):
    return isinstance(value,str) and len(value)==64 and all(c in '0123456789abcdef' for c in value)


def manifest():
    entries = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        digest, name = line.split('  ', 1)
        path = Path(name)
        require(len(digest)==64 and all(c in '0123456789abcdef' for c in digest) and
                not path.is_absolute() and '..' not in path.parts and name not in entries,
                'invalid manifest entry')
        entries[name] = digest
    files = {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file() and
             p != ROOT/'SHA256SUMS'}
    require(set(entries)==files and len(entries)>=80, 'incomplete archive manifest')
    require(all(hash_file(ROOT/name)==digest for name,digest in entries.items()), 'hash mismatch')
    return len(entries)


def load_oracle(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    files = manifest()
    native = ROOT/'native'
    receipt = json.loads((native/'receipt.json').read_text())
    require(receipt['status']=='EXPECTED_API_DIFFERENCE_CONFIRMED' and
            receipt['native_invocations']==6 and receipt['oracle_invocations']==12 and
            receipt['GCP_used'] is False and receipt['errors']==[], 'native receipt scope')
    pins = receipt['sources_before']
    expected_sources = {'source/'+name for name in ('head/head.cpp','head/head.hpp',
        'points/dendrogram.cpp','points/dendrogram.hpp','core/status.hpp','core/types.hpp',
        'core/reasons.def')} | {'probe.cpp','probe_under_root.cpp','probe_tripled.cpp',
                                'check.py','check_under_root.py','record.py'}
    require(set(pins)==expected_sources and pins==receipt['sources_after'], 'native closure')
    require(all(hash_file(native/name)==digest for name,digest in pins.items()), 'native source pin')
    commands = receipt['commands']
    expected_commands = {'compiler'} | {kind+'_'+stem+'_'+mode for kind in ('compile','native')
        for stem in ('base','under_root','tripled') for mode in ('normal','ubsan')} | {
        'judge_'+stem+'_'+mode+'_'+python_mode for stem in ('base','under_root','tripled')
        for mode in ('normal','ubsan') for python_mode in ('normal','optimized')}
    require(len(commands)==25 and {c['name'] for c in commands}==expected_commands and
            all(c['exit_code']==c['expected_exit_code']==0 for c in commands),
            'native command closure')
    binaries = receipt['binary_hashes']
    require(len(binaries)==6 and {Path(name).name for name in binaries}=={
        stem+'_'+mode for stem in ('base','under_root','tripled') for mode in ('normal','ubsan')}
        and all(is_digest(value['before']) and value['before']==value['after']
                for value in binaries.values()), 'historical binary closure')
    for command in commands:
        for stream in ('stdout','stderr'):
            require(hash_file(native/(command['name']+'.'+stream))==command[stream+'_sha256'],
                    'native captured stream')
    base = load_oracle('point_exit_base_archive', native/'check.py')
    nested = load_oracle('point_exit_nested_archive', native/'check_under_root.py')
    for mode in ('normal','ubsan'):
        reports = [base.judge(native/('native_base_'+mode+'.stdout')),
                   nested.judge(native/('native_under_root_'+mode+'.stdout')),
                   nested.judge(native/('native_tripled_'+mode+'.stdout'),3)]
        require(sum(r['rows'] for r in reports)==24 and
                sum(r['differing_rows'] for r in reports)==8 and
                sum(r['EOM_flips'] for r in reports)==4, 'native replay coverage')
    sklearn = ROOT/'sklearn'
    fit = json.loads((sklearn/'sklearn_capture/receipt.json').read_text())
    require(fit['status']=='PASS' and fit['actual_fit_calls']==32 and fit['unique_configurations']==16
            and fit['preflight_fit_calls']==16 and fit['native_HGP_calls']==0 and
            fit['GCP_used'] is False and fit['before']==fit['after'], 'fit receipt')
    require(len(fit['before'])==2 and {Path(name).name for name in fit['before']}==
            {'record_sklearn.py','sklearn_probe.py'}, 'fit script identities')
    require(len(fit['commands'])==2 and {c['mode'] for c in fit['commands']}=={'normal','optimized'}
            and all(c['exit_code']==0 for c in fit['commands']), 'fit command identities')
    for name,digest in fit['before'].items():
        require(hash_file(sklearn/Path(name).name)==digest, 'fit script pin')
    normal = json.loads((sklearn/'sklearn_capture/normal.stdout').read_text())
    optimized = json.loads((sklearn/'sklearn_capture/optimized.stdout').read_text())
    require(normal['rows']==optimized['rows'] and normal['before']==normal['after']==
            optimized['before']==optimized['after'] and len(normal['before'])==6,
            'paired fit/dependency pins')
    event_receipt = json.loads((sklearn/'event_capture/receipt.json').read_text())
    require(event_receipt['status']=='PASS' and event_receipt['exact_configurations']==16 and
            type(event_receipt['new_fit_calls']) is int and event_receipt['new_fit_calls']==0 and
            type(event_receipt['native_calls']) is int and event_receipt['native_calls']==0 and
            event_receipt['GCP_used'] is False and
            event_receipt['before']==event_receipt['after'], 'event receipt scope')
    event_pins = {str(Path(name).relative_to(Path(name).parents[1])) if
                  Path(name).parent.name=='sklearn_capture' else Path(name).name: digest
                  for name,digest in event_receipt['before'].items()}
    require(len(event_receipt['before'])==5 and set(event_pins)=={
        'record_event_judge.py','judge_sklearn_events.py','sklearn_capture/normal.stdout',
        'sklearn_capture/optimized.stdout','sklearn_capture/receipt.json'}, 'event pin identities')
    require(all(hash_file(sklearn/name)==digest for name,digest in event_pins.items()),
            'event archived source/input pin')
    require(len(event_receipt['commands'])==2 and
            {c['mode'] for c in event_receipt['commands']}=={'normal','optimized'} and
            all(c['exit_code']==0 for c in event_receipt['commands']), 'event commands')
    events = load_oracle('strict_cut_archive', sklearn/'judge_sklearn_events.py')
    event_result = events.judge(sklearn/'sklearn_capture')
    event_normal = json.loads((sklearn/'event_capture/normal.stdout').read_text())
    event_optimized = json.loads((sklearn/'event_capture/optimized.stdout').read_text())
    require(event_result==event_normal==event_optimized, 'independent event replay')
    print(json.dumps(dict(status='ARCHIVE_PASS',hashed_files=files,native_invocations_now=0,
                         sklearn_fit_calls_now=0,GCP_used=False,api_configs_per_native_mode=24,
                         native_differences_per_mode=8,native_EOM_flips_per_mode=4,
                         equivalent_ultrametric_configs=16),sort_keys=True))


if __name__=='__main__':
    main()
