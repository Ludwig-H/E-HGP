#!/usr/bin/env python3
"""Substitutions CMake et contrelecture source, stdlib ; aucun test natif."""
import ast
import hashlib
import json
import re
from pathlib import Path


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    root = Path(__file__).resolve().parent
    proof = json.loads((root / 'proof.json').read_text())
    raw = (root / 'tests.cmake').read_bytes()
    need(hashlib.sha256(raw).hexdigest() == proof['source_files']['cmake']['sha256'],
         'source CMake différente')
    text = raw.decode()
    section = text[text.index('# Empreintes de MHGP11SP'):text.index('foreach(case "scale8000;')]
    profiles = {}
    expression = (r'(?:if|elseif)\(MHGP11_COORD_BITS EQUAL (18|21|24)\)\n'
                  r'(.*?)(?=elseif\(MHGP11_COORD_BITS|endif\(\))')
    for match in re.finditer(expression, section, re.S):
        values = dict(re.findall(r'  set\((mhgp11_route_\w+) ([0-9a-f]{16})\)', match.group(2)))
        need(len(values) == 12, 'douze empreintes par profil attendues')
        profiles[match.group(1)] = values
    need(profiles == proof['profiles'], 'tables de profils différentes')
    need(sum(len(v) for v in profiles.values()) == 36, '36 empreintes attendues')
    start = text.index('foreach(case "scale8000;')
    end = text.index('  list(GET case 0 label)', start)
    cases = [c.split(';') for c in re.findall(r'"([^"\n]+)"', text[start:end])]
    need(len(cases) == 6, 'six cas attendus')
    expanded = {}
    for bits, values in profiles.items():
        expanded[bits] = {}
        for case in cases:
            need(len(case) == 6, 'six champs par cas attendus')
            fields = [re.sub(r'\$\{(\w+)\}', lambda m: values[m.group(1)], x) for x in case]
            n = fields[1]
            name = fields[0] if fields[0] != 'lidar' else fields[2].removeprefix('data=lidar_')
            expanded[bits][n] = dict(case=name,
                line='supports_route_verdict conforme k=5 n=%s %s fils=1,4' % (n, fields[3]),
                min_balls=int(fields[4]), min_cells=int(fields[5]))
    need(expanded == proof['expanded_cases'], 'substitutions différentes')
    need(all(expanded['21'][n]['line'] == line for n, line in proof['legacy_u21_lines'].items()),
         'attentes u21 modifiées')
    witnesses = set()
    for record in proof['archived_observations']:
        need(record['line'] == expanded[str(record['bits'])][str(record['n'])]['line'],
             'attente différente du témoin archivé')
        witnesses.add((record['bits'], record['n']))
    need(len(witnesses) == 13, 'treize témoins historiques uniques attendus')
    need(proof['probe_exactly_published_baseline'] and
         proof['source_files']['probe']['sha256'] == proof['source_files']['probe']['baseline_sha256'],
         'probe différent du publié')
    mutant = proof['cli_mutant']
    need(mutant['target']['porte'] == 'mhgp11_cli_points' and mutant['floor'] == 28 and
         mutant['count'] == 28 and mutant['mutation_preserved'], 'mutant ou plancher différent')
    bench = proof['bench']
    need(bench['constants']['L2B_DELIVERED'] is True, 'garde L2b absente')
    source = bench['function_source']
    need(hashlib.sha256(source.encode()).hexdigest() == bench['function_sha256'], 'decide différent')
    tree = ast.parse(source)
    need(len(tree.body) == 1 and isinstance(tree.body[0], ast.FunctionDef) and
         tree.body[0].name == 'decide', 'une fonction decide attendue')
    scope = dict(bench['constants'])
    exec(compile(tree, '<decide épinglé>', 'exec'), scope)
    for value in [True, False]:
        ratios = [dict(frame=f, workers=48, within_rule=value) for f in scope['RULE_FRAMES']]
        need(scope['decide'](ratios, [], 3, 5)['decision'] == 'sans_objet_post_l2b',
             'choix historique encore possible')
    print(json.dumps(dict(verdict='conforme', source_commit=proof['source_commit'],
                         empreintes=36, cas_par_profil=6, temoins_historiques=13,
                         mutants_cli=28, choix_historique='desactive', native_or_cloud_actions=0),
                     sort_keys=True))


if __name__ == '__main__':
    main()
