#!/usr/bin/env python3
"""Relit l'archive K avec le contre-lecteur figé ; aucun moteur ni contrôleur."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import statistics
import subprocess
import tarfile

HERE = Path(__file__).resolve().parent
READER = HERE.parent / 'mes_full_contrelecture'
COLUMNS = ['trame', 'sites', 'valeurs', 'mediane_ns', 'max_medianes_ns', 'max_ns',
           'premiere_ns', 'pic_octets', 'P', 'C', 'G', 'raccord', 'TMVR', 'T', 'M', 'V', 'R']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def canonical(obj):
    return json.dumps(obj, sort_keys=True, separators=(',', ':'), ensure_ascii=False).encode()


def load_reader():
    spec = importlib.util.spec_from_file_location('strict_full_k', READER / 'reader.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def examine(archive, repo, plan):
    r = load_reader()
    meta = r.decode((READER / 'capture.json').read_bytes())
    r.require(sha(plan.read_bytes()) == meta['plan_sha256'], 'plan K different')
    for pin in meta['sources']:
        body = subprocess.check_output(['git', '-C', str(repo), 'show', pin['commit'] + ':' + pin['path']])
        r.require(sha(body) == pin['sha256'], 'source epinglee differente')
    before = sha(archive.read_bytes())
    prefix = 'results/cmd/000_mes_full/files/full/'
    bodies = {}
    with tarfile.open(archive) as tar:
        for member in tar:
            if member.name == prefix + 'rapport_full.json' or (
                    member.name.startswith(prefix + 'brut/') and member.name.endswith('.jsonl')):
                r.require(member.isfile() and member.name not in bodies, 'membre non regulier/duplique')
                bodies[member.name] = tar.extractfile(member).read()
    report = r.decode(bodies.pop(prefix + 'rapport_full.json'))
    raws = {name[len(prefix + 'brut/'):]: body for name, body in bodies.items()}
    computed = r.reconstruct(raws, meta['cohort'], meta['ng_sites'])
    review = r.review(report, computed)
    r.require(review['bruts_admis'] and not review['conditions_non_satisfaites'], 'contrelecture refusee')
    r.require(sha(archive.read_bytes()) == before, 'archive modifiee pendant lecture')
    summary = {}
    for group, frames in computed['statistiques'].items():
        summary[group] = [[name] + [v[k] for k in COLUMNS[1:8]] + [v['etapes_ns'][k] for k in COLUMNS[8:]]
                          for name, v in sorted(frames.items())]
    # Diagnostic comptable, calcul par passe avant les medianes : aucune acceleration supposee.
    without_r = {}
    specs = r.specs(meta['cohort'])
    for name in r.FRAMES:
        rows = []
        for filename, spec in specs.items():
            if spec['group'] == 'k5_appareil' and spec['names'][0] == name:
                rows.extend(r.admit_process(raws[filename], spec, meta['ng_sites'])[1:])
        without_r[name] = dict(valeurs=len(rows),
            mediane_mur_moins_R_ns=statistics.median(v['wall_ns']-v['etapes_ns']['R'] for v in rows),
            minimum_mur_moins_R_ns=min(v['wall_ns']-v['etapes_ns']['R'] for v in rows))
    return dict(archive_sha256=before, lecteur_sha256=sha((READER/'reader.py').read_bytes()),
                metadonnees_lecteur_sha256=sha((READER/'capture.json').read_bytes()),
                rapport_canonique_sha256=sha(canonical(report)),
                bruts_sha256={name:sha(body) for name,body in sorted(raws.items())},
                contrelecture_sha256=sha(canonical(review)),
                verdict=review['verdict_independant'], contrats=computed['contrat'],
                colonnes=COLUMNS, mesures=summary, diagnostic_sans_R=without_r,
                processus=len(raws), passes=610, passes_chaudes=392,
                native_executed=False, cloud_called=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--plan', type=Path, required=True)
    args = parser.parse_args()
    result = examine(args.archive, args.repo, args.plan)
    expected = json.loads((HERE/'mesures.json').read_text())
    r = load_reader()
    r.require(r.same(result, expected), 'resultat different de la capture')
    print(json.dumps(dict(verdict=result['verdict'], processus=result['processus'], passes=result['passes'],
                         passes_chaudes=result['passes_chaudes'], contrats=result['contrats'],
                         identique_capture=True, native_executed=False), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
