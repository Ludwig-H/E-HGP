#!/usr/bin/env python3
"""Rejoue MES-P publié dans une copie temporaire ; aucun calcul HGP ni build."""
import hashlib
import json
import pathlib
import subprocess
import sys
import tempfile


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    here = pathlib.Path(__file__).resolve().parent
    root = next(p for p in here.parents if (p / '.git').exists())
    capture = json.loads((here / 'capture.json').read_text())
    sources = {}
    for path, expected in capture['source_sha256'].items():
        data = subprocess.check_output(['git', 'show', capture['commit'] + ':' + path], cwd=root)
        require(digest(data) == expected, 'empreinte publiée différente : ' + path)
        sources[path] = data
    analysis = 'morsehgp3D_v12/microbancs/mes_p_petits/analyse_p.py'
    require(digest(sources[analysis]) == capture['proposed_analysis_sha256'], 'proposition non intégrée exactement')
    prefix = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-mesp-integration-') as folder:
        base = pathlib.Path(folder)
        product = base / 'product'
        product.mkdir()
        written = {}
        for path, data in sources.items():
            target = (base if path.endswith('/check.py') else product) / pathlib.Path(path).name
            target.write_bytes(data)
            written[target] = digest(data)
        commands = {
            'audit': prefix + [str(base / 'check.py'), '--source-dir', str(product), '--require-common'],
            'official': prefix + [str(product / 'test_pilote_p.py')],
        }
        outputs = {}
        for name, command in commands.items():
            process = subprocess.run(command, capture_output=True, text=True, timeout=20)
            require(process.returncode == 0, name + ' refus : ' + process.stderr + process.stdout)
            outputs[name] = json.loads(process.stdout)
        require(outputs['audit']['cst_0238_common_cohort_contract_pass'], 'cohorte non conforme')
        require(outputs['official'] == {'porte': 'mes_p', 'cas': 5, 'ecarts': 0}, 'porte officielle incomplète')
        require(all(digest(path.read_bytes()) == expected for path, expected in written.items()),
                'copie temporaire modifiée pendant les tests')
    print(json.dumps(dict(commit=capture['commit'], published_analysis_equals_proposal=True,
                          source_sha256=capture['source_sha256'], **outputs), sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
