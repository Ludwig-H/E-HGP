"""Replay a closed Fraction audit only after verifying its full manifest."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys

sys.dont_write_bytecode = True
root = Path(__file__).resolve().parent


def require(ok,message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    hashes = {}
    for line in (root/'SHA256SUMS').read_text().splitlines():
        digest,name = line.split('  ',1)
        require(name not in hashes and not Path(name).is_absolute() and '..' not in Path(name).parts,
                'manifest path')
        hashes[name] = digest
    files = {str(p.relative_to(root)) for p in root.rglob('*') if p.is_file() and p!=root/'SHA256SUMS'}
    require(set(hashes)==files and len(files)>=14,'manifest completeness')
    require(all(sha(root/name)==digest for name,digest in hashes.items()),'archive hash')
    receipt = json.loads((root/'receipt.json').read_text())
    require(set(receipt['sources_frozen_before_execution'])=={'PROTOCOL.txt','check.py'} and
            receipt['sources_unchanged_after_execution'] is True and
            all(sha(root/name)==digest for name,digest in receipt['sources_frozen_before_execution'].items()),
            'pre-execution source pins')
    runs = receipt['runs']
    require(len(runs)==4 and [r['return_code'] for r in runs]==[0,0,1,1],'run closure')
    raw = (root/'normal.stdout.json').read_bytes()
    require(raw==(root/'optimized.stdout.json').read_bytes() and
            hashlib.sha256(raw).hexdigest()==receipt['normal_and_optimized_stdout_sha256'] and
            not (root/'normal.stderr').read_bytes() and not (root/'optimized.stderr').read_bytes(),
            'normal/optimized pair')
    expected = json.loads(raw)
    require(expected['status']=='PASS' and expected['checks']==5793,'control floor')
    spec = importlib.util.spec_from_file_location('moment_group_archive',root/'check.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.CHECKS = 0
    fixtures = [module.check_main_fixture('q3_narrow',29,(3,15)),
                module.check_main_fixture('q4_wide',23,(3,21))]
    equality,overlap = module.check_equality(False),module.check_overlap(False)
    require(module.enc(fixtures)==expected['fixtures'] and module.enc(equality)==expected['equality'] and
            module.enc(overlap)==expected['overlap'],'exact archived oracle')
    for name,func,message in (
        ('equality',module.check_equality,'equality mutant rejects an admitted K3 q2'),
        ('overlap',module.check_overlap,'overlap mutant double-counts a real interior')):
        failed = False
        try:
            func(True)
        except ValueError as error:
            failed = str(error)==message
        require(failed,'logical mutation cause')
        captured = json.loads((root/(name+'_mutant.stderr')).read_text())
        require(captured['status']=='FAIL' and captured['reason']==message,'mutation archive')
    print(json.dumps(dict(status='ARCHIVE_PASS',hashed_files=len(files),Fraction_fixture_checks=5793,
                         fixtures=2,logical_mutants=2,native_invocations=0,GCP_used=False),sort_keys=True))


if __name__=='__main__':
    main()
