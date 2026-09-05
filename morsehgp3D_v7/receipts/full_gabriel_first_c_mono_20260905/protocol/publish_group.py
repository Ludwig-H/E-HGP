#!/usr/bin/env python3
"""Inert by default; publish create-only closed first-C attempt packets.

Inputs are PATH=SHA256SUMS_SHA256, one per previously closed attempt. This only
copies captured bytes. It executes no judge, engine, Git operation or GCP tool.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re

ROOT = Path('/workspaces/E-HGP')
BASE = ROOT / 'build/v7_first_c_20260905_controller'
CAPTURE = BASE / 'check.py'
CAPTURE_SHA = '662d648193f56fa4f2343ad4880aa06fe46840fadc4d629f0e492a114bf488be'
PUBLIC = ROOT / 'morsehgp3D_v7/receipts/full_gabriel_first_c_mono_20260905'
SCHEMA = 'mhgp7-first-c-supplement-capture-v1'


def require(ok: bool, reason: str) -> None:
    if not ok:
        raise ValueError(reason)


def sha(raw: bytes) -> str:
    return hashlib.sha256(raw).hexdigest()


def encode(value: object) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()


def files(directory: Path) -> dict[str, bytes]:
    require(directory.is_dir() and not directory.is_symlink(), 'nonregular directory')
    result = {}
    for path in directory.rglob('*'):
        require(not path.is_symlink(), 'symlink input')
        if path.is_file():
            raw = path.read_bytes()
            require(len(raw) < 8 * 1024 * 1024 and not raw.startswith(b'\x7fELF'),
                    'unbounded or executable input')
            result[str(path.relative_to(directory))] = raw
        else:
            require(path.is_dir(), 'nonregular input')
    return result


def check_sums(packet: dict[str, bytes], expected: str) -> None:
    require(sha(packet['SHA256SUMS']) == expected, 'packet pin differs')
    indexed = set()
    for line in packet['SHA256SUMS'].decode().splitlines():
        match = re.fullmatch(r'([0-9a-f]{64})  ([a-zA-Z0-9_./-]+)', line)
        require(match is not None, 'malformed checksum line')
        pin, name = match.groups()
        require(str(PurePosixPath(name)) == name and not name.startswith('/')
                and '..' not in PurePosixPath(name).parts and name not in indexed
                and name != 'SHA256SUMS' and name in packet, 'unsafe checksum path')
        require(sha(packet[name]) == pin, 'checksum bytes differ')
        indexed.add(name)
    require(indexed | {'SHA256SUMS'} == set(packet), 'incomplete exact inventory')


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--expected-publisher-sha256')
    parser.add_argument('--packet', action='append', default=[], metavar='PATH=SHA256')
    args = parser.parse_args()
    if not args.execute:
        print('prepared_not_executed; no input read or publication')
        return 0
    own = Path(__file__).read_bytes()
    capture = CAPTURE.read_bytes()
    require(sha(own) == args.expected_publisher_sha256, 'publisher pin differs')
    require(sha(capture) == CAPTURE_SHA, 'capture protocol pin differs')
    require(1 <= len(args.packet) <= 8, 'one to eight explicitly pinned packets required')
    require(not PUBLIC.exists(), 'create-only public destination exists')
    payload = {'protocol/publish_group.py': own, 'protocol/check.py': capture}
    observations, watches, labels = [], [], set()
    for argument in args.packet:
        written, sep, pin = argument.rpartition('=')
        require(sep and re.fullmatch('[0-9a-f]{64}', pin) is not None, 'PATH=SHA256 required')
        path = Path(written)
        require(path.is_absolute() and path.parent == BASE and path.resolve().parent == BASE,
                'packet must be direct private child')
        label = path.name
        require(re.fullmatch('[a-z0-9_]+', label) is not None and label not in labels,
                'duplicate or unsafe packet label')
        labels.add(label)
        packet = files(path)
        check_sums(packet, pin)
        record = json.loads(packet['receipt.json'])
        require(record['schema'] == SCHEMA and record['kind'] == 'attempt'
                and record['status'] in ('completed', 'failed') and record['ended']
                and record['engine_executed'] is False and record['gcp_used'] is False,
                'not a closed read-only attempt check')
        require(sha(packet['protocol/check.py']) == CAPTURE_SHA, 'archived protocol differs')
        inputs = [name for name in packet if name.startswith('inputs/attempt/')
                  and name.endswith('.receipt.json')]
        require(len(inputs) == 1, 'attempt input inventory')
        original = json.loads(packet[inputs[0]])
        original_sha = sha(packet[inputs[0]])
        summary = record['summary']
        if record['status'] == 'completed':
            require(summary['source_receipt_sha256'] == original_sha
                    and summary['normal_and_optimized_results_equal'] is True
                    and summary['total_commands'] == len(record['commands']) == 4
                    and all(command['passed'] and command['exit_code'] == 0
                            for command in record['commands']), 'completed check binding')
        observations.append({
            'label': label, 'check_status': record['status'], 'check_error': record['error'],
            'check_summary': summary, 'source_packet_sha256sums': pin,
            'original_receipt_sha256': original_sha,
            'original_engine_exit_code': original['exit_code'],
            'original_terminal': original['terminal'],
        })
        for name, raw in packet.items():
            payload['attempts/' + label + '/' + name] = raw
        watches.append((path, packet))
    payload['publication.json'] = encode({
        'schema': 'mhgp7-first-c-group-publication-v1',
        'public_status': 'not_claimed', 'observations': observations,
        'capture_checks_completed': all(o['check_status'] == 'completed' for o in observations),
        'all_original_engines_succeeded': all(o['original_engine_exit_code'] == 0 for o in observations),
        'engine_reexecuted': False, 'judge_reexecuted_during_publication': False,
        'gcp_used': False, 'scope': 'first_C_counters_not_geometry_or_catalogue_completeness',
    })
    payload['README.md'] = ('# Supplément first-C — captures mono\n\n'
        '5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.\n\n'
        'Publication groupée des contrôles en lecture seule, sans relancer de moteur.\n'
        'Chaque dossier conserve ses captures, sources épinglées, commandes et sommes originales.\n'
        'Un contrôle valide peut décrire un moteur refusé : voir les deux statuts séparés.\n'
        'Le supplément first-C ne remplace pas le juge FULLv2 et ne certifie ni la géométrie,\n'
        'ni la complétude des catalogues, ni une tour inter-K intégrée, ni un contrat 50k/G4.\n\n'
        '[Observations explicites](publication.json), [sommes](SHA256SUMS). GCP non utilisé.\n').encode()
    payload['manifest.json'] = encode({name: {'bytes': len(raw), 'sha256': sha(raw)}
                                       for name, raw in sorted(payload.items())})
    payload['SHA256SUMS'] = ''.join(sha(raw) + '  ' + name + '\n'
                                    for name, raw in sorted(payload.items())).encode()
    for path, packet in watches:
        require(files(path) == packet, 'private input drift before publication')
    require(CAPTURE.read_bytes() == capture and Path(__file__).read_bytes() == own,
            'protocol drift before publication')
    PUBLIC.mkdir()
    for name, raw in sorted(payload.items()):
        target = PUBLIC / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with target.open('xb') as stream:
            stream.write(raw)
    require(files(PUBLIC) == payload, 'published bytes or inventory differ')
    for path, packet in watches:
        require(files(path) == packet, 'private input drift after publication')
    print(json.dumps({'published': str(PUBLIC), 'files': len(payload),
                      'SHA256SUMS': sha(payload['SHA256SUMS']),
                      'git_index_check_pending_ROOT': True}, sort_keys=True))
    return 0


if __name__ == '__main__':
    try:
        raise SystemExit(main())
    except (ValueError, OSError, KeyError, TypeError) as error:
        print(json.dumps({'publication': 'refused', 'reason': str(error)}, sort_keys=True))
        raise SystemExit(1)
