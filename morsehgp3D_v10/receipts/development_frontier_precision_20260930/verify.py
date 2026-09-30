"""Read-only receipt judge, archive integrity only, equally strict under -O."""
import hashlib
import json
from pathlib import Path


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def main():
    root = Path(__file__).resolve().parent
    lines = (root / 'SHA256SUMS').read_text().splitlines()
    declared = set()
    for line in lines:
        digest, relative = line.split('  ', 1)
        path = Path(relative)
        require(not path.is_absolute() and '..' not in path.parts, 'unsafe manifest path')
        require(relative not in declared and len(digest) == 64, 'duplicate or invalid digest')
        declared.add(relative)
        require(hashlib.sha256((root / path).read_bytes()).hexdigest() == digest, 'changed ' + relative)
    actual = {str(p.relative_to(root)) for p in root.rglob('*')
              if p.is_file() and p.name != 'SHA256SUMS' and '__pycache__' not in p.parts}
    # Nested closed manifests are themselves files of the outer manifest.
    actual |= {str(p.relative_to(root)) for p in root.rglob('SHA256SUMS') if p.parent != root}
    require(actual == declared, 'missing or extra receipt paths')
    execution = json.loads((root / 'qualification/execution.json').read_text())
    require(execution['status'] == 'PASS' and len(execution['commands']) == 11, 'execution scope')
    require(execution['source_before'] == execution['source_after'], 'source changed')
    for command in execution['commands']:
        require(command['returncode'] == command['expected'], 'unexpected command exit')
    require([c['name'] for c in execution['commands'] if c['expected'] == 1] ==
            ['mutant_mid', 'mutant_product'], 'mutation scope')
    a = json.loads((root / 'qualification/native_normal/receipt.json').read_text())
    b = json.loads((root / 'qualification/native_optimized/receipt.json').read_text())
    require(a['status'] == b['status'] == 'PASS' and len(a['clouds']) == len(b['clouds']) == 6, 'native scope')
    require(a['checks'] == b['checks'] == 815, 'native floor')
    require(a['optimize_flag'] == 0 and b['optimize_flag'] == 1, 'optimization modes')
    for x, y in zip(a['clouds'], b['clouds']):
        for key in ('case', 'K', 'n', 'export_sha256', 'cloud_sha256', 'point0_date',
                    'point0_expected', 'examined', 'selected'):
            require(x[key] == y[key], 'paired result mismatch')
        require(x['point0_date'] == x['point0_expected'], 'Gamma expected date mismatch')
    require(execution['GCP_used'] is False, 'unexpected GCP claim')
    grid = json.loads((root / 'grid32_primitives/receipt.json').read_text())
    require(grid['original_R_sources_before_after_equal'] is True and
            grid['captured_sources_and_mutants_before_after_equal'] is True, 'grid closure')
    require([r['name'] for r in grid['runs']] ==
            ['normal', 'ubsan', 'morton21', 'distance_u64'], 'grid run scope')
    for run, expected in zip(grid['runs'], (0, 0, 1, 1)):
        require(type(run['compile_exit_code']) is int and run['compile_exit_code'] == 0,
                'grid compile failed')
        require(type(run['run_exit_code']) is int and run['run_exit_code'] == expected,
                'grid run exit mismatch')
        require(run['checks'] == 212684 and run['distances'] == 10162 and
                run['roundtrips'] == 10203 and run['refused_keys'] == 64, 'grid panel changed')
        require(run['signal_exit_observed'] is False and
                run['ubsan_runtime_diagnostics_observed'] is False, 'grid crash or sanitizer failure')
        if expected:
            require(run['result'] == 'CAUSALLY_REJECTED' and run['first_errors'],
                    'grid mutant not causally rejected')
    require((root / 'grid32_primitives/normal.run.log').read_bytes() ==
            (root / 'grid32_primitives/ubsan.run.log').read_bytes(), 'grid paired stdout mismatch')
    for before, after in [('source_hashes_before.sha256', 'source_hashes_after.sha256'),
                          ('original_R_hashes_before.sha256', 'original_R_hashes_after.sha256')]:
        require((root / 'grid32_primitives' / before).read_bytes() ==
                (root / 'grid32_primitives' / after).read_bytes(), 'grid source hashes changed')
    for source in grid['sources']:
        require(hashlib.sha256((root / 'grid32_primitives' / source['path']).read_bytes()).hexdigest()
                == source['sha256'], 'grid snapshot mismatch')
    require(grid['notes']['gcp_used'] is False and grid['notes']['engine_used'] is False, 'grid scope')
    cmake = json.loads((root / 'grid32_cmake/receipt.json').read_text())
    require(cmake['status'] == 'PASS' and cmake['source_before'] == cmake['source_after'] and
            len(cmake['commands']) == 3 and
            all(c['returncode'] == 0 for c in cmake['commands']), 'targeted CMake gates')
    print(json.dumps({'status': 'PASS', 'files': len(declared),
                      'scope': 'archive_integrity_only'}, sort_keys=True))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
