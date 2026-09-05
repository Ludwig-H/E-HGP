#!/usr/bin/env python3
"""Create-only singleton mono publication; inert until ROOT's explicit GO.

Only closed byte-pinned captures are copied. No engine, judge, compiler, Git or
GCP command is executed. The old module is reused solely for pure projections.
ELF omission is limited to the exact new-build binary, whose hash is preserved.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import types

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_singleton_20260905_mono_controller'
ADAPTER = BASE / 'capture.py'
ADAPTER_SHA = '910a67bfb014ca0a213483046430a9ca1d29f49b8117c7db4979cd4a9ff9f05f'
PUBLIC_REL = 'morsehgp3D_v7/receipts/full_gabriel_singleton_mono_20260905'
CHECKER = ROOT / 'tools/check_v7_receipt_publication.py'
CHECKER_SHA = '32420385f487260e0706b3e649befca25cc95a9d45f17d22472c333870729580'


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encode(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def imported(path: Path, pin: str):
    raw = path.read_bytes()
    require(sha(raw) == pin, 'protocol pin differs: ' + str(path))
    module = types.ModuleType('inert_singleton_publisher_' + path.stem)
    module.__file__ = str(path)
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module, raw


def names(path: Path) -> set[str]:
    output = set()
    require(path.is_dir() and not path.is_symlink(), 'nonregular capture directory')
    for item in path.rglob('*'):
        require(not item.is_symlink(), 'capture symlink forbidden')
        if item.is_file():
            output.add(str(item.relative_to(path)))
        else:
            require(item.is_dir(), 'nonregular capture artifact')
    return output


class Reader:
    def __init__(self):
        self.watches: dict[Path, bytes] = {}
        self.payload: dict[str, bytes] = {}
        self.inventory: dict[Path, set[str]] = {}
        self.omissions: list[dict] = []

    def read(self, path: Path, pin: str | None = None) -> bytes:
        require(path.is_file() and not path.is_symlink() and path.stat().st_size <= 32 << 20,
                'input nonregular or too large')
        raw = path.read_bytes()
        require(pin is None or sha(raw) == pin, 'input pin differs: ' + str(path))
        require(path not in self.watches or self.watches[path] == raw, 'input drift')
        self.watches[path] = raw
        return raw

    def collect(self, argument: str, kind: str, prefix: str) -> tuple[Path, dict]:
        written, sep, pin = argument.rpartition('=')
        require(sep and re.fullmatch('[0-9a-f]{64}', pin) is not None, 'PATH=SHA256 required')
        path = Path(written)
        require(path.is_absolute() and path.name == 'receipt.json'
                and path.resolve().is_relative_to(BASE), 'receipt scope')
        raw = self.read(path, pin)
        record = json.loads(raw)
        expected_schema = ('mhgp7-full-lazy-probe-controller-v1'
                           if kind in ('build', 'micro') else 'mhgp7-singleton-mono-adapter-v1')
        require(record['schema'] == expected_schema and record['kind'] == kind
                and record['status'] in ('completed', 'failed') and record['ended'],
                'not a closed expected capture')
        if kind != 'paired':
            require(record['status'] == 'completed' and record['sources_stable'] is True,
                    'failed admission is not an admitted mono campaign')
        inventory = names(path.parent)
        require(inventory == set(record['artifacts']) | {'receipt.json'},
                'closed capture inventory differs')
        self.inventory[path.parent] = inventory
        self.payload[prefix + '/receipt.json'] = raw
        binary = ROOT / record['binary'] if kind == 'build' else None
        for name, expected in record['artifacts'].items():
            require(str(PurePosixPath(name)) == name and not name.startswith('/')
                    and '..' not in PurePosixPath(name).parts, 'unsafe capture artifact')
            source = path.parent / name
            content = self.read(source, expected)
            if source == binary:
                require(content.startswith(b'\x7fELF') and sha(content) == record['binary_sha256'],
                        'exact omitted binary binding')
                self.omissions.append({'path': str(source), 'sha256': sha(content),
                    'bytes': len(content), 'reason': 'only_exact_new_build_ELF_omitted_no_receipt_repair'})
                continue
            require(not content.startswith(b'\x7fELF'), 'unexpected ELF forbidden')
            self.payload[prefix + '/' + name] = content
        return path, record

    def stable(self) -> None:
        require(all(path.read_bytes() == raw and not path.is_symlink()
                    for path, raw in self.watches.items()), 'captured byte drift')
        require(all(names(path) == inventory for path, inventory in self.inventory.items()),
                'captured inventory drift')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-publisher-sha256')
    parser.add_argument('--build', required=True, metavar='PATH=SHA256')
    parser.add_argument('--admission', required=True, metavar='PATH=SHA256')
    parser.add_argument('--paired', required=True, metavar='PATH=SHA256')
    args = parser.parse_args()
    if not args.execute:
        print('prepared_not_executed; no capture read or publication')
        return 0
    reader = Reader()
    own = reader.read(Path(__file__).resolve(), args.expected_publisher_sha256)
    require(re.fullmatch('[0-9a-f]{64}', args.expected_publisher_sha256 or '') is not None,
            'reviewed publisher SHA required')
    adapter, adapter_bytes = imported(ADAPTER, ADAPTER_SHA)
    checker, checker_bytes = imported(CHECKER, CHECKER_SHA)
    reader.read(ADAPTER, ADAPTER_SHA)
    reader.read(CHECKER, CHECKER_SHA)
    public = ROOT / PUBLIC_REL
    require(not public.exists() and not public.is_symlink(), 'create-only publication exists')
    _, build = reader.collect(args.build, 'build', 'build')
    _, admission = reader.collect(args.admission, 'admission', 'admission')
    require(admission['new_build_argument'] == args.build, 'build/admission binding')
    _, micro = reader.collect(admission['new_micro_argument'], 'micro', 'new_micro')
    paired_path, paired = reader.collect(args.paired, 'paired', 'paired')
    paired_admission = json.loads(reader.read(paired_path.parent / 'admission.json'))
    require(paired_admission['micro_argument'] == args.admission, 'paired/admission binding')
    require(micro['binary_sha256'] == build['binary_sha256']
            and len(micro['attempts']) == len(admission['comparisons']) == 24
            and micro['parser_rejects'] == 11, 'micro admission counts')
    # Recheck all stored equalities as a pure byte projection. This is neither
    # a rerun of a judge nor reconstruction of an omitted engine terminal.
    attempts = {}
    for path, raw in reader.watches.items():
        if path.name.endswith('.receipt.json'):
            value = json.loads(raw)
            if 'orders' in value:
                attempts[sha(raw)] = (path, value)
    comparisons = admission['comparisons'] + paired.get('comparisons', [])
    for comparison in comparisons:
        rows = []
        records = []
        for key in ('old_receipt_sha256', 'new_receipt_sha256'):
            path, value = attempts[comparison[key]]
            require(value['status'] == 'completed' and value['exit_code'] == 0
                    and value['terminal']['exit_code'] == 0
                    and value['terminal']['terminal_status'] == 'completed'
                    and value['terminal']['complete_requested_horizontal_orders'] is True,
                    'a refusal cannot be a successful comparison')
            raw_path = path.with_name(path.name.removesuffix('.receipt.json') + '.raw.txt')
            raw = reader.read(raw_path, value['streams'][raw_path.name]['sha256'])
            require(len(raw) == value['streams'][raw_path.name]['bytes'], 'raw stream byte count')
            rows.append(adapter.projected(json.loads(raw.decode().splitlines()[0]), value))
            records.append(value)
        require(rows[0] == rows[1] and comparison['all_non_measure_fields_equal'] is True,
                'stored equality differs from original captures')
        old, new = records
        expected = {
            'old_receipt_sha256': comparison['old_receipt_sha256'],
            'new_receipt_sha256': comparison['new_receipt_sha256'],
            'orders_equal': len(old['orders']), 'all_non_measure_fields_equal': True,
            'old_elapsed_ms': old['terminal']['elapsed_before_terminal_ms'],
            'new_elapsed_ms': new['terminal']['elapsed_before_terminal_ms'],
            'old_full_ms': old['terminal']['stage_ms']['full'],
            'new_full_ms': new['terminal']['stage_ms']['full'],
            'input_digest': old['terminal']['input_digest'],
            'certificate_digest': old['terminal']['certificate_digest'],
            'scope': 'same_policy_same_caps_all_reported_algorithm_fields_not_geometry_proof',
        }
        require(encode(comparison) == encode(expected),
                'comparison summary does not match its two original receipts')
    all_successful = (paired['status'] == 'completed'
                     and paired.get('sources_stable') is True
                     and paired.get('all_successful') is True
                     and len(paired.get('attempts', [])) == 6
                     and all(row['status'] == 'completed' and row['attempt_success'] is True
                             for row in paired.get('attempts', [])))
    if all_successful:
        require(len(paired['comparisons']) == 3
                and sum(row['orders_equal'] for row in paired['comparisons']) == 30,
                'complete heavy comparison nonvacuum')
    reader.payload.update({'protocol/publish.py': own, 'protocol/adapter.py': adapter_bytes,
                           'protocol/check_v7_receipt_publication.py': checker_bytes})
    reader.payload['publication.json'] = encode({
        'schema': 'mhgp7-singleton-mono-publication-v1', 'public_status': 'not_claimed',
        'build_argument': args.build, 'admission_argument': args.admission,
        'paired_argument': args.paired, 'qualification': paired_admission['qualification'],
        'paired_status': paired['status'], 'paired_reason': paired.get('reason'),
        'all_planned_heavy_attempts_successful': all_successful,
        'paired_comparisons': paired.get('comparisons', []),
        'micro_receipts': 48, 'paired_micro_orders': 156,
        'pure_comparisons_rechecked': len(comparisons), 'omissions': reader.omissions,
        'engine_reexecuted': False, 'judge_reexecuted': False, 'gcp_used': False,
        'scope': 'horizontal_relative_orders_not_integrated_inter_K_tower',
        'allocations_measured': False, 'hermetic': False,
    })
    reader.payload['README.md'] = ('# Lots singleton — comparaison mono\n\n'
        '5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.\n\n'
        'Captures closes avant/après, même politique lazy C=1000000, même sonde et mêmes caps.\n'
        'Les 40 dépendances MMD compilées du binaire historique restent distinctes des sources\n'
        'vivantes du nouveau header ; les enregistrements originaux ne sont jamais réparés.\n'
        'Tous les champs rapportés sont comparés sauf la liste explicite des timers/RSS.\n'
        'Ces empreintes ne prouvent ni la géométrie ni la complétude des catalogues.\n\n'
        'Seul le binaire ELF exact du nouveau build est omis ; son hash est conservé.\n'
        'Aucun ELF ancien, code Boost ni résultat historique reconverti en qualification nouvelle.\n'
        'Le build est local, non hermétique ; les allocations ne sont pas mesurées.\n'
        'Ce paquet ne qualifie ni une tour inter-K intégrée, ni 50k en 1 s/100 ms, ni G4 massif.\n\n'
        '[Statuts et observations](publication.json), [inventaire](manifest.json),\n'
        '[sommes](SHA256SUMS). GCP non utilisé.\n').encode()
    reader.payload['manifest.json'] = encode({name: {'bytes': len(raw), 'sha256': sha(raw)}
        for name, raw in sorted(reader.payload.items())})
    reader.payload['SHA256SUMS'] = ''.join(sha(raw) + '  ' + name + '\n'
        for name, raw in sorted(reader.payload.items())).encode()

    def fetch(path: str) -> bytes:
        require(path.startswith(PUBLIC_REL + '/'), 'manifest escaped publication')
        return reader.payload[path[len(PUBLIC_REL) + 1:]]

    checker.verify_manifest(PUBLIC_REL + '/SHA256SUMS', reader.payload['SHA256SUMS'], fetch)
    reader.stable()
    public.mkdir()
    for name, raw in sorted(reader.payload.items()):
        target = public / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(raw)
        require(target.read_bytes() == raw, 'published bytes differ')
    require(names(public) == set(reader.payload), 'published inventory differs')
    reader.stable()
    print(json.dumps({'published': str(public), 'files': len(reader.payload),
                      'SHA256SUMS': sha(reader.payload['SHA256SUMS']),
                      'git_index_check_pending_ROOT': True}, sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(json.dumps({'publication': 'refused', 'reason': str(error)}, sort_keys=True))
        raise SystemExit(1)
