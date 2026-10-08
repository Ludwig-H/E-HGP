#!/usr/bin/env python3
"""Modele SC borne de run_region ; pas un interprete C++ ni une preuve weak-memory."""
from collections import deque
import json

# PC : annonce, scan claim, publication, epoque, retrait sans job, retrait apres job,
# lecture epoque, test sortie, scan dry de sortie, scan dry de reprise, attente epoque,
# attente compte, sorti. Le scan est decompose : aucune photographie atomique de la file.
A, C, P, E, RF, RJ, S, X, D0, D1, WE, WN, OUT = range(13)


def need(ok, why):
    if not ok:
        raise ValueError(why)


def transitions(state, deps, fixed, fail_job):
    status, epoch, workers = state
    count = sum(w[4] for w in workers)
    n = len(status)

    def ready(j):
        return status[j] == 0 and all(status[d] == 2 for d in deps[j])

    for i, old in enumerate(workers):
        pc, cursor, seen, owned, counted, last = old
        if pc == OUT:
            continue
        states, ep = list(status), epoch
        w = list(old)
        label = str(pc)
        if pc == A:
            w = [C, 0, seen, -1, True, False]
            label = 'announce'
        elif pc in (C, D0, D1):
            if cursor == n:
                w[0] = {C: RF, D0: OUT, D1: WE}[pc]
                label = {C: 'claim_empty', D0: 'return', D1: 'dry_empty'}[pc]
            else:
                j = n - 1 - cursor
                if ready(j):
                    if pc == C:
                        states[j] = 1
                        w[0], w[3] = P, j
                        label = 'claim_%d' % j
                    else:
                        w[0], w[1] = (D1 if pc == D0 else A), 0
                        label = 'dry_hit'
                else:
                    w[1] += 1
                    label = 'scan_skip_%d' % j
        elif pc == P:
            states[owned] = 3 if owned == fail_job else 2
            w[0] = RJ if owned == fail_job else E
            label = 'publish_failure' if owned == fail_job else 'publish_success'
        elif pc == E:
            ep += 1
            w[0] = RJ
            label = 'epoch'
        elif pc in (RF, RJ):
            need(counted, 'retrait sans annonce')
            w[4], w[5], w[3] = False, count == 1 if fixed else False, -1
            w[0] = S if pc == RF else A
            label = 'withdraw_last' if count == 1 else 'withdraw_other'
        elif pc == S:
            w[0], w[2] = X, epoch
            label = 'read_epoch'
        elif pc == X:
            w[0], w[1] = (D0 if (last if fixed else count == 0) else D1), 0
            label = 'test_last' if fixed else 'read_count_exit'
        elif pc == WE:
            w[0] = WN if epoch == seen else A
            label = 'wait_epoch'
        elif pc == WN:
            w[0] = A if count == 0 else WE
            label = 'wait_zero' if count == 0 else 'relax'
        else:
            raise ValueError('pc')
        ws = list(workers)
        ws[i] = tuple(w)
        yield (tuple(states), ep, tuple(ws)), i, label


def graph(deps, fixed, fail_job=-1, width=2):
    initial = (tuple(0 for _ in deps), 0, tuple((A, 0, 0, -1, False, False) for _ in range(width)))
    nodes, index, edges, parent = [initial], {initial: 0}, [], [None]
    todo = deque([0])
    while todo:
        i = todo.popleft()
        row = []
        for state, worker, label in transitions(nodes[i], deps, fixed, fail_job):
            if state not in index:
                index[state] = len(nodes)
                nodes.append(state)
                parent.append((i, worker, label))
                todo.append(len(nodes) - 1)
            row.append((index[state], worker, label))
        edges.append(row)
        need(len(nodes) <= 100000, 'modele trop grand : ne pas elargir silencieusement')
    return nodes, edges, parent


def components(edges):
    # Kosaraju iteratif : pas de limite de recursion ni parcours natif.
    seen, order, reverse = set(), [], [[] for _ in edges]
    for i, row in enumerate(edges):
        for j, _, _ in row:
            reverse[j].append(i)
    for root in range(len(edges)):
        if root in seen:
            continue
        seen.add(root)
        stack = [(root, 0)]
        while stack:
            v, cursor = stack[-1]
            if cursor < len(edges[v]):
                stack[-1] = (v, cursor + 1)
                target = edges[v][cursor][0]
                if target not in seen:
                    seen.add(target)
                    stack.append((target, 0))
            else:
                order.append(v)
                stack.pop()
    seen.clear()
    for root in reversed(order):
        if root in seen:
            continue
        seen.add(root)
        comp, todo = set(), [root]
        while todo:
            v = todo.pop()
            comp.add(v)
            for target in reverse[v]:
                if target not in seen:
                    seen.add(target)
                    todo.append(target)
        yield comp


def fair_cycles(nodes, edges):
    found = []
    for comp in components(edges):
        internal = [(i, j, w, label) for i in comp for j, w, label in edges[i] if j in comp]
        cyclic = len(comp) > 1 or any(i == j for i, j, _, _ in internal)
        active = {w for i in comp for w, state in enumerate(nodes[i][2]) if state[0] != OUT}
        # Dans une SCC finie, une marche fermee peut visiter les aretes de chacun de ces fils.
        if cyclic and active and active <= {w for _, _, w, _ in internal}:
            found.append(comp)
    return found


def abandoned(nodes, deps):
    def blocked(j, status):
        return status[j] == 3 or any(blocked(d, status) for d in deps[j])
    return sum(all(w[0] == OUT for w in ws) and
               any(v != 2 and not blocked(j, status) for j, v in enumerate(status))
               for status, _, ws in nodes)


def exact_cycle():
    # Coupe sans travail restant. Deux fils; aucune iteration relax, donc idle ne change pas.
    start = ((), 0, ((WN, 0, 0, -1, False, False), (RF, 0, 0, -1, True, False)))
    schedule = (1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1)
    state, trace = start, []
    for who in schedule:
        nexts = [(s, label) for s, w, label in transitions(state, (), False, -1) if w == who]
        need(len(nexts) == 1, 'transition')
        state, label = nexts[0]
        trace.append({'fil': who, 'action': label, 'in_flight': sum(w[4] for w in state[2])})
    need(state == start, 'cycle non ferme')
    nodes, _, parents = graph((), False)
    need(start in nodes, 'cycle inaccessible')
    at, prefix = nodes.index(start), []
    while parents[at] is not None:
        at, worker, label = parents[at]
        prefix.append({'fil': worker, 'action': label})
    prefix.reverse()
    need(all(t['action'] != 'relax' for t in trace), 'idle non constant')
    return {'prefixe': prefix, 'periode_16': trace}


def main():
    cases = {'vide': (), 'une_tache': ((),), 'chaine': ((), (0,), (1,)),
             'fourche': ((), (0,), (0,)), 'jonction': ((), (), (0, 1)),
             'refus_sans_epoch': ((), (0,))}
    result = {}
    for name, deps in cases.items():
        row = {}
        for fixed in (False, True):
            nodes, edges, _ = graph(deps, fixed, 0 if name == 'refus_sans_epoch' else -1)
            cycles = fair_cycles(nodes, edges)
            lost = abandoned(nodes, deps)
            need(lost == 0, 'travail abandonne')
            need(bool(cycles) == (not fixed), 'cycle attendu/absent')
            row['proposition' if fixed else 'livre'] = {'etats': len(nodes), 'transitions': sum(map(len, edges)),
                                                       'scc_equitables_non_terminales': len(cycles),
                                                       'fins_avec_travail_abandonne': lost}
        result[name] = row
    print(json.dumps({'regime': 'SC, deux fils, 0..3 taches abstraites finies', 'cas': result,
                      'cycle_sans_relax': exact_cycle(), 'weak_memory_prouve': False,
                      'moteur_execute': False}, ensure_ascii=False, sort_keys=True))


if __name__ == '__main__':
    main()
