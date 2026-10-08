#!/usr/bin/env python3
"""Porte de terminaison de la region de la Session recouverte (src/tower/pipeline_run.cpp, run_region).

Constat de l'auditeur Codex du 8 octobre (receipts/audit_reponses_20261008/a_terminaison, source 86d7e39d8) : quand
tout le travail est fini, deux fils pouvaient se reannoncer indefiniment, chacun relisant in_flight apres l'annonce de
l'autre (cycle sequentiellement coherent de seize transitions, equitable pour les fils). Correctif applique : le fil
dont le retrait fait passer in_flight a zero (valeur rendue par fetch_sub) sort si son scan sans reclamation est vide ;
une annonce concurrente n'efface plus ce droit.

La porte ne chronometre rien : un entrelacement de cette sorte ne se provoque pas par une campagne de temps. Elle
verifie deux choses, et echoue si l'une manque :
  1. le source : la regle de sortie corrigee figure exactement une fois, et la relecture d'in_flight dans le test de
     sortie n'y figure plus (une copie mutee qui revient a l'ancienne regle est refusee) ;
  2. le modele borne de l'auditeur (port de model.py, SHA-256 epingle ci-dessous), deux fils, six graphes de zero a
     trois taches (coupe vide, tache unique, chaine, fourche, jonction, refus sans epoque), explore exhaustivement :
     l'ancienne regle a une composante fortement connexe equitable non terminale dans CHACUN des six cas (le modele
     discrimine), la regle corrigee n'en a aucune, et aucune fin globale n'abandonne une tache reclamable ; le cycle
     exact de seize transitions de l'ancienne regle est atteignable et ne passe jamais par relax.
Limites (celles du modele) : coherence sequentielle, deux fils, taches abstraites ; ni modele memoire faible, ni CAS
faible, ni arithmetique, ni callbacks natifs ; aucun delai maximal ni equite du systeme n'est prouve.

Usage : pipeline_terminaison.py <src/tower/pipeline_run.cpp>. Codes : 0 conforme ; 1 ecart ; 2 usage.
Python 3.10 nu, aucun assert (tient sous -O).
"""
from collections import deque
import os
import sys

# Modele : port de receipts/audit_reponses_20261008/a_terminaison/model.py (commit 6a8b6f9a8, SHA-256
# da26d0e8eb54b34f...), reecrit en bibliotheque ; transitions, graphes et criteres inchanges.
FIXED_WITHDRAW = 'const bool last = p.in_flight.fetch_sub(1, std::memory_order_acq_rel) == 1;'
FIXED_EXIT = 'if (last && !find_job(p, false, job)) return {};'
OLD_EXIT = 'p.in_flight.load(std::memory_order_acquire) == 0 && !find_job(p, false, job)'

# PC : annonce, scan de reclamation, publication, epoque, retrait sans travail, retrait apres travail, lecture de
# l'epoque, test de sortie, scan de sortie, scan de reprise, attente d'epoque, attente du compte, sorti.
A, C, P, E, RF, RJ, S, X, D0, D1, WE, WN, OUT = range(13)


class Ecart(Exception):
    pass


def need(ok, why):
    if not ok:
        raise Ecart(why)


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
            raise Ecart('pc')
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
    """Kosaraju iteratif (aucune recursion)."""
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
    """Le cycle de seize transitions de l'ancienne regle, atteignable, sans relax (idle constant)."""
    start = ((), 0, ((WN, 0, 0, -1, False, False), (RF, 0, 0, -1, True, False)))
    schedule = (1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1)
    state, labels = start, []
    for who in schedule:
        nexts = [(s, label) for s, w, label in transitions(state, (), False, -1) if w == who]
        need(len(nexts) == 1, 'cycle : transition ambigue ou absente')
        state, label = nexts[0]
        labels.append(label)
    need(state == start, 'cycle : non ferme')
    nodes, _, _ = graph((), False)
    need(start in nodes, 'cycle : inaccessible')
    need('relax' not in labels, 'cycle : passe par relax')
    return len(schedule)


def check_source(path):
    with open(path, encoding='utf-8') as handle:
        text = handle.read()
    need(text.count(FIXED_WITHDRAW) == 1, 'source : retrait qui garde son passage par zero absent ou repete')
    need(text.count(FIXED_EXIT) == 1, 'source : regle de sortie corrigee absente ou repetee')
    need(OLD_EXIT not in text, 'source : relecture d\'in_flight dans le test de sortie (ancienne regle)')


def check_model():
    cases = {'vide': (), 'une_tache': ((),), 'chaine': ((), (0,), (1,)), 'fourche': ((), (0,), (0,)),
             'jonction': ((), (), (0, 1)), 'refus_sans_epoch': ((), (0,))}
    for name, deps in cases.items():
        for fixed in (False, True):
            nodes, edges, _ = graph(deps, fixed, 0 if name == 'refus_sans_epoch' else -1)
            cycles = fair_cycles(nodes, edges)
            need(abandoned(nodes, deps) == 0, 'modele %s : fin avec travail abandonne' % name)
            need(bool(cycles) != fixed, 'modele %s : %s' % (
                name, 'cycle equitable non terminal avec la regle corrigee' if fixed else
                'ancienne regle sans cycle : le modele ne discrimine plus'))
    return len(cases), exact_cycle()


def main(argv):
    if len(argv) != 2 or not os.path.isfile(argv[1]):
        print('pipeline_terminaison : usage', file=sys.stderr)
        return 2
    try:
        check_source(argv[1])
        cases, period = check_model()
    except Ecart as error:
        print('pipeline_terminaison : %s' % error, file=sys.stderr)
        return 1
    print('pipeline_terminaison_ok cas=%d periode_ancienne=%d source=conforme' % (cases, period))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
