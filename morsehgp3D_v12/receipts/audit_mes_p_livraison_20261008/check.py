#!/usr/bin/env python3
"""MES-P livre : memes octets que la proposition, Python et sorties historiques seulement."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import sys
import tempfile
import types

HERE = Path(__file__).resolve().parent
PRODUCT = 'morsehgp3D_v12/microbancs/mes_p_petits/'
PRIOR = 'morsehgp3D_v12/receipts/audit_reponses_20261007/mes_p_admission/'
ARCHIVE = 'morsehgp3D_v12/receipts/g4_t0g_20261007/resultats/cmd/001_mes_p/files/mes_p/'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def inspect(repo, capture):
    def git(path, expected):
        data = subprocess.check_output(['git', 'show', capture['commit'] + ':' + path], cwd=repo)
        require(hashlib.sha256(data).hexdigest() == expected, 'source differente : ' + path)
        return data

    sources = {name: git(PRODUCT + name, sha) for name, sha in capture['delivered_sha256'].items()}
    report = json.loads(git(ARCHIVE + 'mes_p.json', capture['archive_report_sha256']))
    require(report['passes'] == 4 and report['masque'] == '802811' and len(report['prises']) == 318,
            'configuration historique')
    with tempfile.TemporaryDirectory(prefix='audit-mes-p-livraison-') as directory:
        root = Path(directory)
        prior = root / 'prior'
        prior.mkdir()
        for name, sha in capture['prior_sha256'].items():
            (prior / name).write_bytes(git(PRIOR + name, sha))
        pins = json.loads((prior / 'sources.json').read_text())
        require(capture['delivered_sha256']['pilote_p.py'] == pins['proposed_sha256'] and
                capture['delivered_sha256']['analyse_p.py'] == pins['proposed_analyse_sha256'] and
                capture['delivered_sha256']['test_pilote_p.py'] == pins['inputs'][1]['sha256'],
                'livraison differente de la proposition eprouvee')
        proof = runpy.run_path(str(prior / 'check.py'))['proof'](repo)
        require(proof == json.loads((prior / 'results.json').read_text()), 'contretemoins differents')
        pilot = types.ModuleType('mes_p_livre')
        exec(compile(sources['pilote_p.py'], 'pilote_p.py', 'exec'), pilot.__dict__)
        stub = root / 'replay'
        stub.write_text('#!' + sys.executable + '\nimport pathlib,sys\n'
                        'sys.stdout.buffer.write(pathlib.Path(__file__).with_name("stdout").read_bytes())\n')
        stub.chmod(0o700)
        accepted, refused, same_warm = 0, [], 0
        for filename, sha in capture['selected_jsonl_sha256'].items():
            data = git(ARCHIVE + 'brut/' + filename, sha)
            name, kpart, fpart = Path(filename).stem.rsplit('_', 2)
            k, threads = int(kpart[1:]), int(fpart[1:])
            original = [t for t in report['prises'] if (t['nuage'], t['k'], t['fils']) == (name, k, threads)]
            require(len(original) == 1, 'prise historique ambigue')
            original = original[0]
            (root / 'stdout').write_bytes(data)
            # Le double rend toujours code 0 : meme le flux historiquement expire doit etre refuse par son protocole.
            result = pilot.run_cloud(str(stub), str(root), str(root), name, k, threads, 4, 5)
            require((root / filename).read_bytes() == data, 'octets modifies par le rejeu')
            if original['code'] == 0:
                require(result['admission'] == 'conforme' and result['chaud'] == original['chaud'],
                        'succes historique refuse ou temps modifie : ' + filename)
                accepted += 1
                same_warm += 1
            else:
                require(original['code'] == 'expire' and result['code'] == 0 and result['chaud'] is None and
                        result['admission'] == 'processus_ou_protocole_invalide', 'expiration admise')
                refused.append(dict(file=filename, original_code='expire', replay_code=0,
                                    replay_warm=None, original_warm_was_present=original['chaud'] is not None))
        return dict(delivered_equals_proposal=True, admission_cases=len(proof['cases']),
                    positive_cases=sum(c['proposed_admitted'] for c in proof['cases']),
                    negative_cases=sum(not c['proposed_admitted'] for c in proof['cases']),
                    official_gate=proof['existing_gate'], code_zero_null_warm_analysis=proof['analysis_cli'],
                    historical_selection=dict(count=accepted + len(refused), admitted=accepted, refused=refused,
                                              warm_values_unchanged=same_warm),
                    native_runs=0, builds=0, gcp_used=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    result = inspect(args.repo, capture)
    require(result == capture['result'], 'resultat different')
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
