#!/usr/bin/env python3
"""Contre-preuves MES-D6, JSON publics fabriques ; aucun moteur ni temps mesure.

Charge uniquement le pilote au commit epingle, sauf --pilot explicite.
Code 0 : les observations annoncees sur ce pilote ont ete reproduites.
Les wall_ns sont des sentinelles de schema, jamais des mesures HGP.
"""
import argparse
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import types

PIN = '9b2747eff364d56215b589c782b1a4e51d59a576'
SOURCE = 'morsehgp3D_v12/microbancs/mes_d6_profils/pilote_d6.py'
DIGEST = 'a' * 64


def load(path):
    if path:
        body = Path(path).read_bytes()
    else:
        body = subprocess.check_output(['git', 'show', PIN + ':' + SOURCE])
    module = types.ModuleType('d6_snapshot')
    exec(compile(body, path or SOURCE, 'exec'), module.__dict__)
    return module, hashlib.sha256(body).hexdigest()


def lines(phase, count=2):
    result = []
    key = 'catalogue_sha256' if phase == 'catalogue' else 'resolution_sha256'
    for number in range(count):
        row = dict(phase=phase, status='ok', reason='none', pass_sentinel=number,
                   coord_bits=21, kmax=2, threads=1, sites=2, wall_ns=10)
        row['pass'] = row.pop('pass_sentinel')
        if phase == 'catalogue':
            row.update(balls=3, incidences=4, levels=2)
        result.append(row)
        if phase == 'catalogue':
            result.append(dict(phase='digest', **{key: DIGEST}))
    if phase == 'tour_g':
        for k in (1, 2):
            result.append(dict(phase='ordre', k=k, objet=dict(
                births=3-k, cells=0, inert_cells=0, extended_cells=0, representatives=0)))
        result.append(dict(phase='digest', **{key: DIGEST}))
    result.append(dict(phase='exit', status='ok', reason='none'))
    return result


def entry(pilot, catalogue, tower, code=0):
    cat = pilot.summarize(code, catalogue, 'catalogue', 'catalogue_sha256')
    cat['corps_sha256'] = DIGEST
    return dict(cas='public_fixture', profil=21, facteur=1, tour=0, catalogue=cat,
                tour_g=pilot.summarize(code, tower, 'tour_g', 'resolution_sha256'))


def verdict(pilot, value):
    problems = pilot.checks([value])
    return dict(catalogue_ok=value['catalogue']['ok'], tour_g_ok=value['tour_g']['ok'],
                ordres=list(value['tour_g']['objet']), controle_conforme=not problems)


def proof(pilot):
    out = {}
    normal_cat, normal_g = lines('catalogue'), lines('tour_g')
    normal = entry(pilot, normal_cat, normal_g)
    out['schema_success_two_passes'] = verdict(pilot, normal)

    cat, tower = copy.deepcopy(normal_cat), copy.deepcopy(normal_g)
    for row in cat + tower:
        if row['phase'] in ('catalogue', 'tour_g', 'exit'):
            row.update(status='resource_exhausted', reason='memory_budget')
    out['explicit_refusal_code_zero'] = verdict(pilot, entry(pilot, cat, tower))

    cat, tower = copy.deepcopy(normal_cat), copy.deepcopy(normal_g)
    for row in cat + tower:
        if row['phase'] in ('catalogue', 'tour_g'):
            row['wall_ns'] = True
        if row['phase'] == 'digest':
            for key in ('catalogue_sha256', 'resolution_sha256'):
                if key in row:
                    row[key] = True
    out['bool_wall_and_bool_digest'] = verdict(pilot, entry(pilot, cat, tower))

    scaled = copy.deepcopy(normal)
    scaled['facteur'] = 8
    scaled['tour_g']['empreinte'] = 'b' * 64
    out['dilation_changed_digest_same_counts'] = dict(
        controle_conforme=not pilot.checks([normal, scaled]),
        empreintes_distinctes=normal['tour_g']['empreinte'] != scaled['tour_g']['empreinte'])

    # Test du vrai run et du vrai take ; seuls les executables sont des doubles Python.
    # Le double ne calcule aucune geometrie. On demande P=5, il imprime deux passes,
    # aucun ordre, une ligne non JSON et un faux export exactement long de 64 octets.
    with tempfile.TemporaryDirectory(prefix='d6_math_') as scratch:
        root = Path(scratch)
        bins = root / 'bin'
        bins.mkdir()
        stub_cat = lines('catalogue')
        stub_g = [x for x in lines('tour_g') if x['phase'] != 'ordre']
        for name, payload in (('mhgp12_catalogue_probe', stub_cat), ('mhgp12_tower_probe', stub_g)):
            program = ('#!' + sys.executable + '\nimport json, pathlib, sys\n'
                       'for arg in sys.argv:\n'
                       ' if arg.startswith("--out="):\n'
                       '  p = pathlib.Path(arg[6:]); p.mkdir(parents=True)\n'
                       '  (p / "cat.bin").write_bytes(b"X" * 64)\n'
                       'print("NOT_JSON")\n'
                       'for row in ' + repr(payload) + ': print(json.dumps(row))\n')
            target = bins / name
            target.write_text(program)
            target.chmod(0o700)
        xyz, ids = root / 'public.u32le', root / 'public.ids.u32le'
        xyz.write_bytes(struct.pack('<6I', 0, 0, 0, 2, 0, 0))
        ids.write_bytes(struct.pack('<2I', 0, 1))
        args = types.SimpleNamespace(k=2, fils=1, passes=5, case_now='public_fixture',
                                     racine=str(root), delai=10)
        value = pilot.take({21: str(bins)}, {1: (str(xyz), str(ids), 2)}, (21, 1),
                           args, str(root), 0)
        out['take_P5_two_passes_no_orders_invalid_json_invalid_export'] = dict(
            **verdict(pilot, value), requested_passes=5,
            admitted_passes=len(value['tour_g']['passes_ms']),
            empty_body_hash=value['catalogue']['corps_sha256'] == hashlib.sha256(b'').hexdigest())

        # Main reel, sans compilation : seul build est remplace par les doubles.
        # Les vrais dilate, run, take, summarize et checks traitent les trois facteurs.
        pilot.build = lambda *_: str(bins)
        data = root / 'data'
        data.mkdir()
        (data / 'lidar_public_fixture.u32le').write_bytes(xyz.read_bytes())
        (data / 'lidar_public_fixture.ids.u32le').write_bytes(ids.read_bytes())
        with contextlib.redirect_stdout(io.StringIO()):
            code = pilot.main(['pilot', '--src', str(root), '--racine', str(root / 'build'),
                               '--donnees', str(data), '--sortie', str(root / 'out'),
                               '--cas', 'public_fixture', '--profils', '21', '--passes', '5',
                               '--tours', '1', '--k', '2', '--fils', '1'])
        report = json.loads((root / 'out' / 'mes_d6.json').read_text())
        out['main_small_doubles'] = dict(code=code, prises=len(report['prises']),
                                        controles=report['controles'])
    return out


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pilot')
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    pilot, sha = load(args.pilot)
    result = dict(pin=PIN, pilot_sha256=sha, observations=proof(pilot))
    observed = result['observations']
    expected = all(observed[name]['controle_conforme'] for name in (
        'schema_success_two_passes', 'explicit_refusal_code_zero', 'bool_wall_and_bool_digest',
        'dilation_changed_digest_same_counts',
        'take_P5_two_passes_no_orders_invalid_json_invalid_export'))
    expected = expected and observed['main_small_doubles'] == dict(code=0, prises=3, controles=[])
    expected = expected and observed['take_P5_two_passes_no_orders_invalid_json_invalid_export'][
        'empty_body_hash']
    result['observations_reproduites'] = expected
    encoded = json.dumps(result, indent=2, sort_keys=True) + '\n'
    if args.out:
        args.out.write_text(encoded)
    else:
        sys.stdout.write(encoded)
    return 0 if expected else 1


if __name__ == '__main__':
    sys.exit(main())
