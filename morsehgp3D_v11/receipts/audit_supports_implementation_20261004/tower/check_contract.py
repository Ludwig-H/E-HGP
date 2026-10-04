"""Gardes autonomes du contrat S3, sans import ni execution du produit.

Le modele de journal lit une foret deja close : naissance -> ancetre au
rang precedent -> parent au plateau courant. Les dates initiales ne sont
pas une precondition de la requete du journal. Le contre-exemple geometrique
D2 complet est conserve, sans alteration, dans supports_followup/.../tower.
Ici on verifie son transport sur la foret, puis un plateau a quatre cellules
et une continuation. Ce modele ne qualifie pas le port C++ en chantier.

E2 : pour une boule positive faible, K=p+qmin-1 et t=qmin-1 ; aucune
t-partie de U ne contient son centre dans son enveloppe convexe, par
minimalite de qmin. Les traces comprimees I union A sont donc strictes
(T2). Il y en a au moins une : une boule faible n'est pas une naissance.
Toute naissance de boule positive est forte. Une K-partie ARBITRAIRE de
P peut omettre un interieur et inclure Q : sa MEB peut egaler lambda.
Le juge ferme peut la descendre avec initial<=lambda, jamais l'enregistrer
comme trace stricte sous cette seule hypothese.

Capacite : les 150 sites entiers de la sphere de rayon15, translatés de15,
sont distincts et u21. Centre(15,15,15), p0, qmin2 par antipodes, m150.
Au K8, chacune des C(69,8) parties dont les points ont x>15 se trouve dans
un demi-espace ouvert excluant le centre. Sa MEB est strictement plus
petite : T2 fournit au moins 8 361 453 672 traces strictes. Le catalogue
FULL ne limite pas les coquilles a24. Ce minorant ne requiert ni allocation
des traces ni parcours combinatoire. Le cast u32 n'est donc justifie qu'avec
une garde de capacite, ou un contrat m<=24 effectivement controle.
Un budget insuffisant peut refuser ce cas avant le cast : aucun refus ou
succes natif, ni consommation de plusieurs centaines de Gio, n'est rejoue.
"""

from fractions import Fraction as F
from itertools import permutations
from math import comb
from pathlib import Path
import hashlib
import json


CHECKS = 0


def require(ok, label):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(label)


def ancestor(nodes, seed, level):
    birth, parent = nodes[seed]
    if birth > level:
        raise ValueError('seed not active')
    while parent is not None and nodes[parent][0] <= level:
        seed = parent
        birth, parent = nodes[seed]
    return seed


def attached(nodes, seeds, previous, level):
    prior = sorted(set(ancestor(nodes, seed, previous) for seed in seeds))
    closed = []
    for node in prior:
        parent = nodes[node][1]
        closed.append(parent if parent is not None and nodes[parent][0] == level else node)
    require(len(set(closed)) == 1, 'all traces closed owner')
    owner = closed[0]
    role = 'merge' if nodes[owner][0] == level else 'internal'
    require(role != 'internal' or len(prior) == 1, 'internal one branch')
    return owner, role, prior


def check_d2():
    # Exact dates/forest proved by the autonomous Gram/Gamma receipt D2.
    nodes = [(F(1), 5), (F(49, 2), 5), (F(49, 2), 5),
             (F(41), 6), (F(41), 6), (F(65, 2), 6), (F(1681, 25), None)]
    previous, initial, level = F(41), F(64), F(1681, 25)
    require(previous < initial < level, 'D2 gap')
    require(ancestor(nodes, 0, previous) == 5, 'old seed at previous cut')
    require(ancestor(nodes, 0, initial) == 5, 'same H0 between events')
    require(not initial <= previous, 'reject false D2 bound only')
    answers = []
    for seeds in permutations([0, 3, 4]):
        answer = attached(nodes, seeds, previous, level)
        require(answer == (6, 'merge', [3, 4, 5]), 'D2 correct attachment')
        answers.append(answer)
    for seeds in permutations([0, 1]):
        answer = attached(nodes, seeds, F(49, 2), F(65, 2))
        require(answer == (5, 'merge', [0, 1]), 'first plateau cell')
    for seeds in permutations([0, 2]):
        answer = attached(nodes, seeds, F(49, 2), F(65, 2))
        require(answer == (5, 'merge', [0, 2]), 'same plateau second cell')
    # At a later internal ball, a seed already fused must query the top,
    # not publish the old birth or artificially manufacture a new node.
    answer = attached(nodes, [0, 3, 4], level, F(145, 2))
    require(answer == (6, 'internal', [6]), 'continuation after final merger')
    return {'previous': str(previous), 'initial': str(initial),
            'event': str(level), 'owner': answers[0][0], 'prior': answers[0][2]}


def check_multifusion():
    level = F(800, 3)
    nodes = [(F(200), 6)] * 6 + [(level, None)]
    cells = [[0, 1, 3], [0, 2, 4], [1, 2, 5], [3, 4, 5]]
    snapshots = []
    for order in permutations(range(4)):
        parent = list(range(6))
        def find(x):
            while parent[x] != x:
                x = parent[x]
            return x
        performed, touched, journal = [], set(), []
        for index in order:
            seeds = cells[index]
            owner, role, prior = attached(nodes, seeds, F(200), level)
            require((owner, role, prior) == (6, 'merge', seeds), 'closed plateau role')
            touched.update(prior)
            journal.append((index, prior))
            first, unions = find(seeds[0]), 0
            for seed in seeds[1:]:
                root = find(seed)
                if root != first:
                    first, root = min(first, root), max(first, root)
                    parent[root] = first
                    unions += 1
            performed.append(unions)
        require(sum(performed) == 5, 'six components five unions')
        require(len(touched) == 6, 'deduplicated open components')
        require(len(set(find(x) for x in range(6))) == 1, 'single final component')
        require(sum(len(prior) for _, prior in journal) == 12, 'per-ball prior count')
        require([prior for _, prior in sorted(journal)] == cells, 'invariant priors')
        snapshots.append(performed)
    require([2, 2, 1, 0] in snapshots, 'historical performed-union pattern')
    return {'orders': len(snapshots), 'strict_traces_per_ball': 3,
            'prior_count_total': 12, 'merge_children': 6, 'performed_unions_total': 5}


def check_e2():
    # Line(0,1,2): AC is weak at K2, compressed traces are AB and BC.
    points = [F(0), F(1), F(2)]
    def beta(indices):
        xs = [points[i] for i in indices]
        return (max(xs) - min(xs)) ** 2 / 4
    level = beta([0, 2])
    require(level == 1, 'weak level')
    require(beta([0, 1]) == F(1, 4), 'first compressed trace')
    require(beta([1, 2]) == F(1, 4), 'second compressed trace')
    require(beta([0, 2]) == level, 'arbitrary weak K-part not strict')
    require(1 + 2 == 2 + 1, 'weak admission')
    return {'p': 1, 'qmin': 2, 'k': 2, 'event': str(level),
            'compressed_trace': '1/4', 'arbitrary_trace': '1'}


def check_capacity():
    shell = [(x + 15, y + 15, z + 15)
             for x in range(-15, 16) for y in range(-15, 16)
             for z in range(-15, 16) if x*x + y*y + z*z == 225]
    require(len(shell) == 150, 'complete integer shell150')
    require(len(set(shell)) == len(shell), 'distinct unit sites')
    require(all(0 <= v < 2 ** 21 for s in shell for v in s), 'u21 domain')
    require(all(sum((v - 15) ** 2 for v in s) == 225 for s in shell), 'one exact sphere')
    require((0, 15, 15) in shell and (30, 15, 15) in shell, 'qmin2 antipodes')
    positive = [s for s in shell if s[0] > 15]
    require(len(positive) == 69, 'open positive hemisphere69')
    lower = comb(len(positive), 8)
    require(lower == 8361453672, 'strict trace lower bound')
    require(lower > 2 ** 32 - 1, 'u32 insufficient')
    require(0 + 2 - 1 <= 8 <= 0 + 150, 'window order8')
    require(0 + 2 <= 8 + 1, 'Cat8 admission')
    require(comb(24, 12) < 2 ** 32, 'future shell24 contract sufficient')
    return {'shell': 150, 'open_hemisphere': 69, 'k': 8, 'p': 0, 'qmin': 2,
            'strict_trace_lower_bound': lower, 'uint32_max': 2**32-1,
            'native_allocations_or_execution': False}


def main():
    data = {'d2': check_d2(), 'multifusion': check_multifusion(),
            'e2': check_e2(), 'capacity': check_capacity()}
    data.update(status='PASS', checks=CHECKS,
                scope='autonomous exact scalar/forest model; WIP source review; not native qualification',
                script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    print(json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
