#!/usr/bin/env python3
"""Cohorte du pilote apparié : Git épinglé, sondes Python synthétiques uniquement."""
import argparse
import copy
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import types
from contextlib import redirect_stdout

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
REL = 'morsehgp3D_v12/microbancs/mes_apparie/pilote_apparie.py'


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    need(sha((HERE / 'proposition.patch').read_bytes()) == capture['patch_sha256'], 'patch changé')
    with tempfile.TemporaryDirectory(prefix='audit-apparie-cohorte-') as tmp:
        root = Path(tmp)
        for path, expected in capture['sources'].items():
            data = subprocess.check_output(['git', '-C', str(args.repo), 'show',
                                            capture['source_commit'] + ':' + path])
            need(sha(data) == expected, 'source changée : ' + path)
            target = root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
        source = root / REL
        original = source.read_bytes()
        subprocess.run(['git', 'apply', '--check', str(HERE / 'proposition.patch')], cwd=root,
                       check=True, capture_output=True)
        subprocess.run(['git', 'apply', str(HERE / 'proposition.patch')], cwd=root,
                       check=True, capture_output=True)
        candidate = source.read_bytes()
        need(sha(candidate) == capture['proposed_sha256'], 'postimage changée')
        source.write_bytes(original)
        spec = importlib.util.spec_from_file_location('official_apparie_fixture',
                                                      source.parent / 'test_pilote_apparie.py')
        fixture = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(fixture)
        old = fixture.pa
        new = types.ModuleType('apparie_cohorte_proposed')
        new.__file__ = str(source)
        exec(compile(candidate, str(source), 'exec'), new.__dict__)
        logs = {}
        for name, module in [('avant', old), ('propose', new)]:
            output = io.StringIO()
            with redirect_stdout(output):
                code = module.self_test()
            need(code == 0, 'auto-test échoué : ' + name)
            logs[name] = output.getvalue().strip()
        fixture.prepare(tmp)
        code, report, folder = fixture.campaign(tmp, 'ok', extra=['--tours', '10', '--passes', '6'])
        need(code == 0 and report is not None, 'campagne simulée échouée')
        # Ce changement ne contourne aucune vérification matérielle : la fixture est CPU et le reste demeure simulé.
        report['parametres']['essai'] = False
        baseline = old.judge(report, folder)
        need(baseline['verdict'] == 'juge' and baseline['cas']['cache']['verdict'] == 'adopte'
             and baseline['cas']['seq']['verdict'] == 'rejete', 'positif historique')
        need(new.judge(report, folder) == baseline, 'positif changé')
        cases = {}

        def check_case(name, altered, raw=True):
            before = old.judge(altered, folder if raw else None)
            after = new.judge(altered, folder if raw else None)
            need(before['verdict'] == 'juge', 'témoin ancien non reproduit : ' + name)
            need(after['verdict'] == 'refuse' and after['refus'], 'témoin encore admis : ' + name)
            cases[name] = dict(avant=before['verdict'], bras_avant={k:v['verdict'] for k,v in before['cas'].items()},
                               propose=after['verdict'], refus=after['refus'], relecture_brute=raw)

        altered = copy.deepcopy(report)
        altered['parametres']['trames'] = []
        check_case('cohorte_vide_adopte_meme_le_bras_lent', altered)
        altered = copy.deepcopy(report)
        altered['parametres']['trames'] = ['ng00']
        check_case('deux_trames_decisives_ignorees', altered)
        altered = copy.deepcopy(report)
        for label, rounds in altered['campagne'].items():
            row = copy.deepcopy(rounds[0])
            for arm, take in row.items():
                source_log = Path(folder) / 'journaux/campagne' / label / (arm + '_t00.jsonl')
                destination = source_log.with_name(arm + '_t10.jsonl')
                destination.write_bytes(source_log.read_bytes())
                take['journal'] = destination.name
                take['journal_sha256'] = sha(destination.read_bytes())
            rounds.append(row)
        check_case('onzieme_tour_non_prevu_rehache', altered)
        for label in altered['campagne']:
            for path in (Path(folder) / 'journaux/campagne' / label).glob('*_t10.jsonl'):
                path.unlink()
        altered = copy.deepcopy(report)
        altered['regle_bras']['aa'] = ['--cache=1024']
        check_case('aa_options_differentes_reference', altered)
        # Minima : essais du juge sur son générateur de résumés ; aucune prétention de rejeu brut dans ces deux cas.
        for field, value in [('passes', 2), ('tours', 4)]:
            altered = old.synthetic_report({'levier': 0.9})
            altered['parametres'][field] = value
            if field == 'tours':
                for label in altered['campagne']:
                    altered['campagne'][label] = altered['campagne'][label][:value]
            check_case('minimum_' + field, altered, raw=False)
        need(new.judge(report, folder) == baseline, 'restauration incorrecte')
        print(json.dumps(dict(source_commit=capture['source_commit'], candidate_sha256=sha(candidate),
                              positif='cache adopte, seq rejete, A/A controle, inchangé', auto_tests=logs,
                              contre_exemples=cases,
                              scope='sondes Python simulées seulement ; aucune mesure réelle, aucun moteur'),
                         ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
