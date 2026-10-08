#!/usr/bin/env python3
"""Contre-JSON MES-FULL ; aucun moteur, appareil ou payload reel execute/lu."""
import argparse
import copy
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load(data, filename):
    namespace = {'__name__': 'audit_only'}
    exec(compile(data, filename, 'exec'), namespace)
    return namespace


def streams(passes=10, device=True):
    rows = [dict(phase='open', status='ok', reason='none', wall_ns=1)] if device else []
    for i in range(passes):
        rows += [dict(phase='full', pass_=i, trame='ng00', voie='device' if device else 'cpu', status='ok',
                      coord_bits=21, kmax=5, threads=48, sites=39885, wall_ns=100,
                      etapes_ns=dict(P=10, C=20, G=10, raccord=1, TMVR=50, T=20, M=10, V=10, R=10),
                      c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=1, publication=1),
                      g_ns=dict(tables=1, resolution=1), hors_mur_ns=dict(validation=1, empreinte=1),
                      pic_octets=1000, full_sha256='a' * 64),
                 dict(phase='liberation', pass_=i, liberation_ns=1)]
    for row in rows:
        if 'pass_' in row:
            row['pass'] = row.pop('pass_')
    return rows + [dict(phase='exit', status='ok', reason='none')]


def proof(pilot, gate):
    base = streams()
    variants = [('complet_synthetique', base)]
    neg = copy.deepcopy(base)
    over = copy.deepcopy(base)
    meta = copy.deepcopy(base)
    for row in neg:
        if row['phase'] == 'full':
            row['wall_ns'] = -1
    for row in over:
        if row['phase'] == 'full':
            row['etapes_ns']['P'] = 1000
    for row in meta:
        if row['phase'] == 'full':
            row.update(trame='autre', coord_bits=18, threads=1)
    variants += [('mur_negatif', neg), ('etape_superieure_au_mur', over),
                 ('metadonnees_differentes_commande', meta),
                 ('liberations_absentes', [r for r in base if r['phase'] != 'liberation'])]
    parsed = []
    for name, rows in variants:
        full, refusal = pilot['parse_process'](0, '\n'.join(json.dumps(r) for r in rows), 10, 5, True)
        fragment = pilot['contract']({'ng00': pilot['frame_stats']([full], 1)}) if full else None
        parsed.append(dict(case=name, accepted_by_parse=not refusal, rows=len(full),
                           contract_fragment_tenu=fragment['tenu'] if fragment else None))
    require(all(x['accepted_by_parse'] and x['contract_fragment_tenu'] for x in parsed), 'contre-JSON different')

    # L'environnement est sonde par le vrai helper, toutes commandes doublees : aucune commande systeme appelee.
    original_run = pilot['run']
    pilot['environment'].__globals__['run'] = lambda argv, delay: (1, '', 'echec simule')
    env = pilot['environment'](None)
    pilot['environment'].__globals__['run'] = original_run
    require(env['gpu_apps'] is None, 'environnement different')

    # Archive miniature de METADONNEES incompletes ; membres de donnees vides, aucun moteur appele dessus.
    cohorts = []
    with tempfile.TemporaryDirectory(prefix='audit-full-cohorte-') as folder:
        folder = Path(folder)
        for number, names in enumerate((['unique'], ['doublon', 'doublon'])):
            archive = folder / f'{number}.tar'
            members = {'bundle_manifest.json': json.dumps({'cases': [{'name': n} for n in names]}).encode()}
            for name in set(names):
                members[name + '.u32le'] = b''
                members[name + '.ids.u32le'] = b''
            with tarfile.open(archive, 'w') as tar:
                for name, data in members.items():
                    info = tarfile.TarInfo(name)
                    info.size = len(data)
                    tar.addfile(info, io.BytesIO(data))
            got = pilot['unpack'](archive, folder / str(number))
            require(got == names, 'cohorte maintenant refusee')
            cohorts.append(dict(case='une_seule_trame' if number == 0 else 'noms_repetes',
                                returned=len(got), unique=len(set(got)), accepted_by_unpack=True))

    # Porte DE CORRECTION livree : appels directs, hors main et hors subprocess.
    rows = streams(3, False)
    for row in rows:
        if row['phase'] == 'full':
            row.update(kmax=3, threads=3)
            row.pop('wall_ns')
            row.pop('etapes_ns')
        if row['phase'] == 'liberation':
            row.clear()
            row['phase'] = 'liberation'
    errors = []
    digests = gate['check_passes'](rows, 3, errors, 'contreflux')
    require(not errors and len(digests) == 3, 'porte maintenant stricte')
    return dict(pilot_variants=parsed, environment_failure_yields_gpu_apps_null=True,
                current_isolation_if_sees_truthy_failure=bool(env['gpu_apps']),
                incomplete_cohorts=cohorts, correction_gate_accepts_missing_measurements=True,
                full_campaign_executed=False, native_executed=False, real_payloads_read=False, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    data = {}
    for path, expected in cap['git_sources_sha256'].items():
        value = subprocess.run(['git', 'show', cap['commit'] + ':' + path], cwd=args.repo,
                               capture_output=True, check=True).stdout
        require(sha(value) == expected, 'source Git differente')
        data[path] = value
    path = cap['pilot_path']
    pilot_bytes = (args.repo / path).read_bytes()
    require(sha(pilot_bytes) == cap['pilot_sha256'], 'pilote en cours modifie')
    gate_path = 'morsehgp3D_v12/tests/tower/full_probe_check.py'
    result = proof(load(pilot_bytes, path), load(data[gate_path], gate_path))
    require(sha((args.repo / path).read_bytes()) == cap['pilot_sha256'], 'pilote modifie pendant lecture')
    require(result == cap['result'], 'resultat different')
    print(json.dumps(result, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
