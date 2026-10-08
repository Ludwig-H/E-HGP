#!/usr/bin/env python3
"""Audit abstrait borné, sans charger ni exécuter le moteur."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent


def require(ok, message):
    if not ok:
        raise ValueError(message)


def reachability(edges):
    ancestors = {}
    def visit(n, stack):
        require(n not in stack, 'cycle')
        if n not in ancestors:
            seen = set(edges[n])
            for p in edges[n]:
                seen.update(visit(p, stack | {n}))
            ancestors[n] = seen
        return ancestors[n]
    for node in edges:
        visit(node, set())
    return ancestors


def resource_graph(deps, orders, remove=None):
    # Les noeuds représentent la FIN réussie des corps. kRows libère au début :
    # son commencement attend déjà tous ses prédécesseurs.
    g = {(i, s): set() for i in range(orders) for s in deps
         if i or s not in ('kLower', 'kBirths', 'kMerges')}
    for (i, s), prev in g.items():
        for p, below in deps[s]:
            if below and i == 0:
                continue
            edge = ((i - int(below), p), (i, s))
            if edge != remove:
                prev.add(edge[0])
    return g


def resource_needs(orders):
    # Table issue des ressources réellement lues, distincte de StepDeps.
    needs = []
    for i in range(orders):
        for reader in ('kHistory', 'kSlices', 'kClasses', 'kNodes', 'kParents', 'kChildren'):
            needs += [((i, 'kKernel'), (i, reader), 'events écrits'),
                      ((i, reader), (i, 'kRows'), 'events vivants')]
        needs += [((i, 'kFinish'), (i, 'kCollect'), 'forme/cell_node prêts'),
                  ((i, 'kHistory'), (i, 'kCollect'), 'CSR survivants prête'),
                  ((i, 'kCollect'), (i, 'kPlaceRows'), 'comptes complets'),
                  ((i, 'kPlaceRows'), (i, 'kFill'), 'sortie réservée')]
        if i:
            needs += [((i - 1, 'kFinish'), (i, 'kBirths'), 'forme basse prête'),
                      ((i - 1, 'kNumber'), (i, 'kBirths'), 'birth_node bas prêt'),
                      ((i, 'kBirths'), (i, 'kMerges'), 'lower des naissances prête'),
                      ((i - 1, 'kHistory'), (i, 'kMerges'), 'CSR basse prête'),
                      ((i - 1, 'kFinish'), (i, 'kMerges'), 'event_node/rank bas prêts')]
    return needs


def graph_errors(deps, orders, remove=None):
    a = reachability(resource_graph(deps, orders, remove))
    return [label for x, y, label in resource_needs(orders) if x not in a[y]]


def leaves_states(n, early_free=False, early_publish=False, overread=False):
    # Micro-transitions : numérotation, calcul G, choix pré-passe, publication,
    # pré-passe du noyau puis consommation sérielle. G: 0 neuf,1 calculé,
    # 2 pré-passe choisie,3 publié sans feuilles,4 publié avec feuilles.
    # Le nombre de workers est abstrait : tous les entrelacements autorisés
    # par ces dépendances sont explorés, pas le modèle mémoire C++ complet.
    initial = (False, (0,) * n, (False,) * n, 0, False, False)
    stack, seen, terminal, violations = [initial], set(), 0, set()
    while stack:
        state = stack.pop()
        if state in seen:
            continue
        seen.add(state)
        numbered, stages, ready, cursor, pending, freed = state
        successors = []
        def add(num=numbered, gs=stages, rs=ready, cur=cursor, pend=pending, free=freed):
            successors.append((num, tuple(gs), tuple(rs), cur, pend, free))
        if not numbered:
            add(num=True)
        for s, stage in enumerate(stages):
            gs = list(stages)
            if stage == 0:
                gs[s] = 1
                add(gs=gs)
            elif stage == 1:
                gs[s] = 2 if numbered else 3
                add(gs=gs)
            elif stage == 2:
                if freed:
                    violations.add('pré-passe après libération')
                else:
                    rs = list(ready)
                    rs[s] = True
                    gs[s] = 4
                    add(gs=gs, rs=rs)
        published = cursor < n and (stages[cursor] >= 3 or
                                    (early_publish and stages[cursor] >= 1))
        if numbered and not freed and published:
            if ready[cursor]:
                if overread and cursor + 1 < n and not ready[cursor + 1]:
                    violations.add('préchargement hors tranche prête')
                add(cur=cursor + 1, pend=False)
            elif pending:
                rs = list(ready)
                rs[cursor] = True
                add(rs=rs, pend=False)
            else:
                # Si publié trop tôt, un writer G peut encore écrire cette ligne.
                if stages[cursor] < 3:
                    violations.add('deux auteurs de feuilles possibles')
                add(pend=True)
        if numbered and not freed and (cursor == n or (early_free and cursor == 1)):
            if cursor != n or any(s < 3 for s in stages):
                violations.add('libération avant dernier lecteur/auteur')
            add(free=True)
        if not successors:
            terminal += 1
            if not violations:
                require(freed and cursor == n and all(ready), 'fin incomplète')
        stack.extend(successors)
    return {'states': len(seen), 'terminal_states': terminal, 'violations': sorted(violations)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', default='/workspaces/E-HGP')
    args = parser.parse_args()
    capture = json.loads((HERE / 'capture.json').read_text())
    blobs = {}
    for path, digest in capture['sources'].items():
        raw = subprocess.check_output(['git', '-C', args.repo, 'show', capture['commit'] + ':' + path])
        require(hashlib.sha256(raw).hexdigest() == digest, 'source modifiée ' + path)
        blobs[path] = raw.decode()
    body = blobs['morsehgp3D_v12/src/tower/pipeline_run.cpp'].split('StepDeps step_dependencies', 1)[1].split('u32 refusal_rank', 1)[0]
    deps = {}
    for name, count, tail in re.findall(r'case (k\w+): return \{(\d+), (.*?)\};', body):
        deps[name] = [(p, flag == 'true') for p, flag in re.findall(r'\{(k\w+), (true|false)\}', tail)]
        require(len(deps[name]) == int(count), 'lecture de StepDeps')
    require(len(deps) == 19, 'cohorte des étapes')
    requirements = 0
    for k in range(1, 13):
        require(not graph_errors(deps, k), 'durée de vie graphe')
        requirements += len(resource_needs(k))
        a = reachability(resource_graph(deps, k))
        for i in range(k):
            require((i, 'kHistory') not in a[(i, 'kFinish')] and
                    (i, 'kFinish') not in a[(i, 'kHistory')], 'concurrence H/M conservée')
    mutations = {}
    for name, edge in {
        'liberer_events_sans_historique': ((0, 'kHistory'), (0, 'kRows')),
        'fusion_verticale_sans_historique_bas': ((0, 'kHistory'), (1, 'kMerges')),
        'naissance_verticale_sans_contraction_basse': ((0, 'kFinish'), (1, 'kLower')),
        'registre_sans_contraction': ((0, 'kFinish'), (0, 'kRows')),
    }.items():
        errors = graph_errors(deps, 2, edge)
        require(errors, 'mutation non détectée ' + name)
        mutations[name] = errors[0]
    leaves = {str(n): leaves_states(n) for n in range(4)}
    require(all(not r['violations'] for r in leaves.values()), 'feuilles')
    for name, option in [('liberation_premiere_tranche', 'early_free'),
                         ('publication_avant_prepasse', 'early_publish'),
                         ('prefetch_tranche_suivante', 'overread')]:
        result = leaves_states(2, **{option: True})
        require(result['violations'], 'mutation feuilles non détectée ' + name)
        mutations[name] = result['violations']
    print(json.dumps({'sources': len(blobs), 'orders_tested': list(range(1, 13)),
                      'resource_requirements': requirements, 'leaves': leaves,
                      'model_mutations': mutations, 'native_execution': False}, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
