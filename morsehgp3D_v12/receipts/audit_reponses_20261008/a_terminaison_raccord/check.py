#!/usr/bin/env python3
"""Raccord du correctif et de la porte Python, sans compilation ni moteur."""
import argparse
import ast
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(data):
    return hashlib.sha256(data).hexdigest()


class Normalize(ast.NodeTransformer):
    def visit_FunctionDef(self, node):
        node = self.generic_visit(node)
        if node.body and isinstance(node.body[0], ast.Expr) and isinstance(node.body[0].value, ast.Constant) \
                and isinstance(node.body[0].value.value, str):
            node.body = node.body[1:]
        return node

    def visit_Name(self, node):
        if node.id == 'Ecart':
            node.id = 'ValueError'
        return node


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--repo', type=Path, required=True)
    p.add_argument('--snapshot', type=Path)
    p.add_argument('--source-pin')
    a = p.parse_args()
    need(a.snapshot is not None or a.source_pin is not None, 'snapshot ou pin requis')
    capture = json.loads((HERE / 'capture.json').read_text())
    sources = {}
    for path, expected in capture['sources'].items():
        data = subprocess.check_output(['git', '-C', str(a.repo), 'show', a.source_pin + ':' + path]) \
            if a.source_pin else (a.snapshot / Path(path).name).read_bytes()
        need(sha(data) == expected, 'source changée : ' + path)
        sources[Path(path).name] = data
    proposal = json.loads((HERE.parent / 'a_terminaison/capture.json').read_text())
    need(sha(sources['pipeline_run.cpp']) == proposal['proposed_sha256'], 'correctif différent de la proposition')
    original = subprocess.check_output(['git', '-C', str(a.repo), 'show',
        '6a8b6f9a8:morsehgp3D_v12/receipts/audit_reponses_20261008/a_terminaison/model.py'])
    need(sha(original) == capture['initial_model_sha256'], 'modèle original changé')
    oldtree, newtree = ast.parse(original), ast.parse(sources['pipeline_terminaison.py'])
    for name in capture['ast_functions_equal_modulo_exception_and_docstring']:
        nodes = [next(x for x in tree.body if isinstance(x, ast.FunctionDef) and x.name == name)
                 for tree in (oldtree, newtree)]
        need(ast.dump(Normalize().visit(copy.deepcopy(nodes[0]))) ==
             ast.dump(Normalize().visit(copy.deepcopy(nodes[1]))), 'port différent : ' + name)
    mutant = next(x for x in json.loads(sources['tower.json'])['mutants']
                  if x['id'] == 'terminaison_relecture_du_compte')
    need(mutant['porte'] == 'mhgp12_tower_pipeline_terminaison', 'raccord mutant/porte')
    cmake = sources['tests.cmake'].decode()
    need('mhgp12_python_gate(mhgp12_tower_pipeline_terminaison 0 pipeline_terminaison.py' in cmake and
         '${PROJECT_SOURCE_DIR}/src/tower/pipeline_run.cpp' in cmake, 'raccord CMake')
    oldrule = sources['pipeline_run.cpp'].decode()
    for replacement in [mutant] + mutant.get('aussi', []):
        need(replacement['fichier'] == 'src/tower/pipeline_run.cpp' and
             oldrule.count(replacement['cherche']) == 1, 'mutation non ciblée')
        oldrule = oldrule.replace(replacement['cherche'], replacement['remplace'])
    runs = []
    with tempfile.TemporaryDirectory(prefix='audit-terminaison-raccord-') as tmp:
        folder = Path(tmp)
        gate = folder / 'pipeline_terminaison.py'
        gate.write_bytes(sources['pipeline_terminaison.py'])
        fixed, reverted = folder / 'fixed.cpp', folder / 'old.cpp'
        fixed.write_bytes(sources['pipeline_run.cpp'])
        reverted.write_text(oldrule)
        for name, flags, source in [('normal', [], fixed), ('-O', ['-O'], fixed),
                                    ('mutant normal', [], reverted), ('mutant -O', ['-O'], reverted)]:
            out = subprocess.run([sys.executable, '-B', *flags, str(gate), str(source)],
                                 text=True, capture_output=True, timeout=120)
            runs.append(dict(mode=name, code=out.returncode, stdout=out.stdout.strip(), stderr=out.stderr.strip()))
    need(runs == capture['commands'], 'résultats différents')
    print(json.dumps(dict(correctif_exact=True, fonctions_modele_identiques=5, raccord_mutant_cmake=True,
                          commandes=runs, mutant_tue_par='contrôle textuel du source, avant le modèle',
                          qualification_native=False), ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
