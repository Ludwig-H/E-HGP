#!/usr/bin/env python3
"""Rejoue les lecteurs Python sur les sorties existantes ; aucun natif/build."""
import argparse
import hashlib
import json
import runpy
import subprocess
from pathlib import Path


def need(ok, message):
    if not ok:
        raise SystemExit(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encoded(x):
    return json.dumps(x, sort_keys=True, allow_nan=False)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--scratch', required=True, type=Path)
    root = ap.parse_args().scratch.resolve()
    here = Path(__file__).resolve().parent
    cap = json.loads((here / 'capture.json').read_text())
    old = here.parent.parent / 'audit_reponses_20261007/gc_rebase_portes/check.py'
    need(sha(old) == cap['tree_reader_sha256'], 'lecteur arbre')
    helper = runpy.run_path(str(old))
    source = root / 'repo3/morsehgp3D_v12'
    out = root / 'pilote_final/sortie'
    raw_hashes, binary_hashes = {}, {}

    def verify():
        for rel, h in cap['artifact_hashes'].items():
            need(sha(root / rel) == h, 'artefact: ' + rel)
        for path, h in raw_hashes.items():
            need(sha(path) == h, 'journal modifie')
        for path, h in binary_hashes.items():
            need(sha(path) == h, 'binaire modifie')
        need(helper['current_tree'](source) == (359, cap['source_tree_sha256']), 'sources')

    verify()
    pilot = runpy.run_path(str(source / 'microbancs/mes_t2c_g/pilote_t2c.py'))
    report = json.loads((out / 'rapport_t2c.json').read_text())
    camp = report['campagne_k5']
    need((camp['fils'], camp['passes'], camp['tours_demandes']) == (3, 6, 2), 'configuration')
    judged = pilot['juger'](report, str(out))
    need(encoded(judged) == encoded(report['jugement']), 'jugement archive/recalcule')
    need(judged['refus'] == [f'{f} : 2 tours valides sur 10 exiges' for f in ('ng00', 'ng01', 'ng02')], 'motifs')
    need(len(judged['verdicts']) == 4 and set(judged['verdicts'].values()) == {'refuse'}, 'verdicts')
    takes = passes = 0

    def take(row, arm, k, threads, count):
        nonlocal takes, passes
        path = (out / row['journal']).resolve()
        need(out.resolve() in path.parents and path not in raw_hashes, 'journal externe/reutilise')
        raw_hashes[path] = row['journal_sha256']
        need(sha(path) == raw_hashes[path], 'hash journal')
        need(row['valide'] is True, 'prise invalide')
        checked = pilot['lire_prise'](str(path), row['code'], k, threads, count, pilot['schema_bras'](arm), arm == 'profil')
        for key in ('murs_ns', 'empreinte', 'g_ns', 'diagnostics', 'profil', 'sites'):
            need(encoded(row[key]) == encoded(checked[key]), 'resume/brut: ' + key)
        takes += 1
        passes += len(checked['murs_ns'])

    for turns in camp['trames'].values():
        for turn in turns:
            for arm, row in turn.items():
                take(row, arm, 5, 3, 6)
    need(takes == 30 and passes == 180, 'cohorte K5')
    info_counts = {}
    for label, plan in report['informations'].items():
        if not isinstance(plan, dict) or 'tours' not in plan:
            continue
        before = takes
        for turns in plan['tours'].values():
            for turn in turns:
                for arm, row in turn.items():
                    take(row, arm, plan['k'], plan['fils'], plan['passes'])
        info_counts[label] = takes - before
    need(info_counts == {'k10_w3': 18, 'k5_w1_ng00': 12, 'profil_k5': 1, 'profil_k5_w3': 1, 'uniformes_k5_w3': 6}, 'infos')
    need(takes == 68 and passes == 308, 'effectif')
    for binary in report['construction']['binaires'].values():
        path = Path(binary['chemin'])
        need(sha(path) == binary['sha256'], 'binaire courant')
        binary_hashes[path] = binary['sha256']
    need(len(binary_hashes) == 5, 'cinq bras construits, A/A alias du meme binaire')
    driver = (root / 'runs/final_checks.log').read_text()
    need('01:00:20 pilote=0\n' in driver and driver.endswith('01:00:20 fin\n'), 'cloture')
    # Faux commandes uniquement : prouve le defaut du conducteur, sans rejouer son moteur.
    shell = subprocess.check_output(['bash', '--noprofile', '--norc', '-c',
        'false; echo "$(date -u +%H:%M:%S) lost=$?"; '
        'false; saved=$?; echo "$(date -u +%H:%M:%S) saved=$saved"'], text=True).splitlines()
    need(len(shell) == 2 and shell[0].endswith(' lost=0') and shell[1].endswith(' saved=1'), 'temoin shell')
    verify()
    print(encoded({'status': 'ok', 'prototype_pin': cap['prototype_pin'], 'source_files': 359,
        'takes': takes, 'passes': passes, 'k5_takes': 30, 'informative_takes': 38,
        'raw_journals_verified_before_after': len(raw_hashes), 'binary_paths_verified': len(binary_hashes),
        'judgement_recomputed_identically': True, 'verdicts': judged['verdicts'], 'expected_refusals': judged['refus'],
        'driver_exit_authenticated': False, 'expected_pilot_exit_from_source': 3,
        'shell_status_loss_reproduced_with_false': True,
        'native_executed_by_auditor': False, 'performance_adopted': False, 'g4_campaign': False}))


if __name__ == '__main__':
    main()
