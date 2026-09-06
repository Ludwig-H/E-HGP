#!/usr/bin/env python3
"""Create-only publication plus one small relocated Python-only replay."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parent
ROOT = BASE.parents[1]
STAGE = BASE / 'stage_r1'
DEST = ROOT / 'morsehgp3D_v7/receipts/full_minima_quotient_20260906'
MATH = ROOT / 'build/v7_gabriel_minima_quotient_20260906'
V4 = ROOT / 'build/v7_meb_probe_20260906_controller'
HISTORIES = {
    'quotient': (MATH / 'rational_capture', '24210dfeb475dfa75ecef65dc13aadc2a6d8336cd76e663b3cec79c352a98c2a',
                 '0b29966b87a40874cf92592064c96e9e6b6bc99f9bb39bb40a0748eae8fb4488'),
    'descent': (MATH / 'descent_permanent_capture', '7e2c6c99f5c7bc31d9baef7244ad7ba6e3a46e0f64605bb68fa002838f241318',
                '35692f7f788e0023ca0c164041f085a23b3c7282c45219d756fc338a7a3c13e4'),
    'lower_bound': (V4 / 'math_r1', '3efe537e423e4f14c59ae5d8d8a2190a49c5c381658f69f33bd266b3d0d88a43', None),
}
EXTRA = {
    'replay.py': (BASE / 'replay.py', '6e9c6b3e05da76bf7a98c3d4c59fc368586e8e1b01a4b11678984d497440bb8e'),
    'proofs/quotient.md': (MATH / 'RESULTATS.md', 'e5cad93a310d420c0a466e2b03cf2082850c4f52388dce7a83d4601e2ea33448'),
    'proofs/descent.md': (MATH / 'descent_review.md', '5439d5f48738340adca6c66dc90a916b929242cb0ee2cc2eccb42aad0f913281'),
    'proofs/lower_bound.md': (ROOT / 'morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md',
                               'a3350584db1d7e98cc7e5cae6a5ff17d3b13238388ed83b8780b754ea7a2ff67'),
    'provenance/math_capture.py': (ROOT / 'build/v7_meb_probe_20260906_publication/math_capture.py',
                                  '74f78203c92fc7719e846e86560aea239f4bd90766a127a710c7afafcdcd96ed'),
    'provenance/v4_sources_before.json': (V4 / 'build_r1/sources_before.json',
                                         '13d2607aded889716c55ddfc42c90e83bae3a2ec3206e1ff2c9e19d4fabb87c5'),
    'provenance/v4_capture.py': (V4 / 'build_r1_sources/source_snapshot/build/v7_meb_probe_20260906_controller/capture.py',
                                'ee9d4640452c81bc4bd4d872630da2a334cf49da078c84dd74c82c8b4c20058d'),
    'provenance/imported_capture.py': (V4 / 'build_r1_sources/source_snapshot/build/v7_full_lazy_20260905_probe_controller/capture.py',
                                      '417ccc3b47bb7591405f3af99bf7591bf2019794aa4535077436ce4889c4adfa'),
}
GATES = {
    'full_gabriel_minima_quotient_gate.py': (HISTORIES['quotient'][0], 'bee615b5f8b937e11104597fd674d868828d6b850616582f5163b44454ab9434'),
    'full_gabriel_descent_comparison_gate.py': (HISTORIES['descent'][0], '5e357ead0d626121cf66e15d17f0817475520663db7fe5237d9cbd7f25448a16'),
    'full_output_lower_bound_gate.py': (ROOT / 'morsehgp3D_v7/tests', '01fd40103f89d878e1d89bb08fcc7592f4c684f3f5db9ef70443a2660f7dcb04'),
}


def require(ok, why):
    if not ok:
        raise ValueError(why)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, sort_keys=True, indent=2) + '\n')


def regular(path):
    require(path.is_file() and not any(p.is_symlink() for p in (path, *path.parents)), 'regular_file')
    require(not path.read_bytes().startswith(b'\x7fELF'), 'ELF_forbidden')


def copy(source, target, pin, inputs):
    regular(source)
    require(sha(source) == pin and not target.exists(), 'pin_or_fresh_target:' + str(source))
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)
    require(sha(target) == pin, 'copy_hash')
    inputs[str(source.relative_to(ROOT))] = pin


def manifest(directory, filename):
    directory.joinpath(filename).write_text(''.join(sha(path) + '  ' + str(path.relative_to(directory)) + '\n'
        for path in sorted(directory.rglob('*')) if path.is_file() and path != directory / filename))


def history(kind, source, receipt_pin, sums_pin, inputs):
    require(sha(source / 'receipt.json') == receipt_pin, 'closed_receipt_pin')
    receipt = json.loads((source / 'receipt.json').read_bytes())
    require(receipt['status'] == 'completed' and receipt['engine_invoked'] is False, 'closed_scope')
    if sums_pin:
        require(sha(source / 'SHA256SUMS') == sums_pin, 'sealed_manifest_pin')
        files = dict((name, pin) for pin, name in
                     (line.split('  ', 1) for line in (source / 'SHA256SUMS').read_text().splitlines()))
        files['SHA256SUMS'] = sums_pin
    else:
        files = dict(receipt['artifacts'], **{'receipt.json': receipt_pin})
    require(set(files) == {p.name for p in source.iterdir()} and all('/' not in name and name not in ('.', '..')
                                                                  for name in files), 'closed_flat_history')
    for name, pin in files.items():
        copy(source / name, STAGE / 'payload/history' / kind / name, pin, inputs)


def qualify(name, payload, optimized=False):
    output = STAGE / 'qualification' / name
    argv = [sys.executable, '-I', '-B'] + (['-O'] if optimized else [])
    argv += [str(payload / 'replay.py'), '--execute', '--output', str(output)]
    result = subprocess.run(argv, cwd=payload, capture_output=True, timeout=30, check=False)
    (STAGE / 'qualification' / (name + '.stdout')).write_bytes(result.stdout)
    (STAGE / 'qualification' / (name + '.stderr')).write_bytes(result.stderr)
    command = dict(argv=argv, cwd=str(payload), expected=0, exit_code=result.returncode,
                   timeout_seconds=30, engine_invoked=False,
                   stdout_sha256=sha(STAGE / 'qualification' / (name + '.stdout')),
                   stderr_sha256=sha(STAGE / 'qualification' / (name + '.stderr')))
    save(STAGE / 'qualification' / (name + '.command.json'), command)
    require(result.returncode == 0 and not result.stderr, 'portable_replay:' + name)
    receipt = json.loads((output / 'receipt.json').read_bytes())
    require(receipt['status'] == 'completed' and len(receipt['commands']) == 12
            and all(c['historical_stdout_equal'] for c in receipt['commands']), 'twelve_replays:' + name)
    return dict(name=name, receipt_sha256=sha(output / 'receipt.json'), commands=12, status='completed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-self-sha256')
    args = parser.parse_args()
    require(args.execute and sha(Path(__file__).resolve()) == args.expected_self_sha256, 'inert_without_reviewed_pin')
    require(not STAGE.exists() and not DEST.exists(), 'create_only_stage_and_public_destination')
    STAGE.mkdir()
    (STAGE / 'qualification').mkdir()
    inputs = {}
    for kind, (source, receipt_pin, sums_pin) in HISTORIES.items():
        history(kind, source, receipt_pin, sums_pin, inputs)
    for name, (source, pin) in EXTRA.items():
        copy(source, STAGE / 'payload' / name, pin, inputs)
    for filename, (directory, pin) in GATES.items():
        copy(directory / filename, STAGE / 'payload/sources/morsehgp3D_v7/tests' / filename, pin, inputs)
    copy(BASE / 'README.md', STAGE / 'README.md',
         'e003dbb115e84c2335194e4c4e8db6990736cba2816e80439767d2ac1a60dd95', inputs)
    copy(Path(__file__).resolve(), STAGE / 'payload/provenance/publish.py', args.expected_self_sha256, inputs)
    save(STAGE / 'payload/provenance/publication_inputs.json', inputs)
    manifest(STAGE / 'payload', 'PAYLOAD_SHA256SUMS')
    relocated = Path(tempfile.mkdtemp(prefix='mhgp7_minima_math_relocation_')) / 'payload'
    shutil.copytree(STAGE / 'payload', relocated)
    qualifications = [qualify('relocated', relocated)]
    require(all(sha(ROOT / name) == pin for name, pin in inputs.items()), 'input_drift')
    save(STAGE / 'publication.json', dict(schema='mhgp7-public-minima-math-packet-v1',
         status='completed', public_status='not_claimed', engine_invoked=False, latency_qualified=False,
         historical_qualification_relabelled=False, sources_before_after_stable=True,
         portable_payload_manifest_sha256=sha(STAGE / 'payload/PAYLOAD_SHA256SUMS'),
         qualifications=qualifications, relocated_readonly_copy_retained=str(relocated)))
    manifest(STAGE, 'SHA256SUMS')
    require(not DEST.exists(), 'public_destination_still_absent')
    shutil.copytree(STAGE, DEST)
    for path in STAGE.rglob('*'):
        if path.is_file():
            require(sha(path) == sha(DEST / path.relative_to(STAGE)), 'public_copy_hash')
    print(json.dumps(dict(status='completed', files=sum(p.is_file() for p in DEST.rglob('*')),
          root_manifest_sha256=sha(DEST / 'SHA256SUMS'), publication_sha256=sha(DEST / 'publication.json'),
          destination=str(DEST), engine_invoked=False), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError, KeyError, OSError, subprocess.TimeoutExpired) as error:
        print(json.dumps(dict(status='failed', reason=str(error)), sort_keys=True))
        sys.exit(1)
