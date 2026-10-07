#!/usr/bin/env python3
"""Oracle borne de l'etage G (CONTRAT_TOUR.md, paragraphe 9.2) : la resolution de la sonde contre la reference exacte
(reference/hgp12_ref/constructive.py, entiers Python, force brute) sur la suite rapide (n <= 14).

    g_oracle.py <mhgp12_tower_probe> [--min-clouds=N] [--min-targets=N] [--min-cell-targets=N]

Pour chaque nuage de families.fast_suite() : la sonde calcule Cat_K puis l'etage G (K du nuage, feuille K + 3, deux
fils) et les exporte (cat.bin, res.bin) ; la reference (politique de saut v12_indices, celle de la v11 et du moteur)
juge, ordre par ordre :
  1. naissances : meme ensemble de boules (k >= 2 : boule de la reference par centre exact et rayon carre recalcules
     depuis S* ; k = 1 : les sites, cle = SiteIdx) ;
  2. cellules : exactement les cellules de fenetre non naissances de la reference (jonctions et cellules inertes),
     meme genre (inerte ou jonction), rang de la boule ;
  3. traces : exactement les t-parties separables de la coquille (separabilite de la reference), dans l'ordre
     lexicographique des A ;
  4. cibles : celle de resolve_v12 (meme regle : arret sur la premiere cellule de fenetre), genre et boule ;
  5. composante : la cible est dans la composante de la trace a la coupe OUVERTE du niveau de sa jonction (foret de
     l'etage B ; une cible << cellule >> se lit par une trace de sa cellule, de niveau strictement inferieur) ;
  6. compteurs de l'objet egaux aux comptes de la reference.
Un nuage a doublons doit etre refuse (code 2, multiplicity_unsupported, decision D8).
Codes : 0 conforme ; 1 ecart ; 2 usage ou sonde absente ; 3 plancher non atteint ou invariant de la reference.
Python 3.10 nu, aucun assert.
"""
import os
import struct
import subprocess
import sys
import tempfile
from itertools import combinations

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'reference'))
import g_dump  # noqa: E402
from hgp12_ref import families  # noqa: E402
from hgp12_ref.constructive import Reference  # noqa: E402
from hgp12_ref.model import InvariantError  # noqa: E402

MAX_SITES = 14
FLOORS = {'--min-clouds': 300, '--min-targets': 20000, '--min-cell-targets': 1000}


def run_probe(probe, folder, points, kmax, ordinal):
    xyz, ids = os.path.join(folder, 'in.u32le'), os.path.join(folder, 'in.ids.u32le')
    with open(xyz, 'wb') as handle:
        handle.write(b''.join(struct.pack('<3I', *p) for p in points))
    with open(ids, 'wb') as handle:
        handle.write(struct.pack('<%dI' % len(points), *range(len(points))))
    out = os.path.join(folder, 'out%d' % ordinal)  # dossier transactionnel neuf
    cmd = [probe, xyz, ids, '--k=%d' % kmax, '--leaf=%d' % (kmax + 3), '--threads=2', '--out=%s' % out]
    done = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    text = done.stdout.decode('ascii', 'replace')
    if done.returncode != 0:
        return done.returncode, text, None, None
    return 0, text, g_dump.read_catalogue(os.path.join(out, 'cat.bin')), g_dump.read_resolution(
        os.path.join(out, 'res.bin'))


class Judge(object):
    """Correspondances entre la sonde et la reference d'un nuage, et juge d'un ordre."""

    def __init__(self, points, kmax, cat, res, name, errors, totals):
        self.ref = Reference(points, kmax, resolution='v12_indices')
        self.cat, self.res, self.name, self.errors, self.totals = cat, res, name, errors, totals
        site_of_pos = dict((pos, s) for s, pos in enumerate(self.ref.sites))
        self.ref_site = [site_of_pos[cat.pos[s]] for s in range(cat.sites)]  # SiteIdx -> site de la reference
        self.internal = [self.ref.site_points[s][0] for s in self.ref_site]  # SiteIdx -> identifiant interne
        self.ball_cache = {}

    def gap(self, message):
        self.errors.append('%s : %s' % (self.name, message))

    def ref_ball(self, b):
        """Boule de la reference de la boule b de la sonde (sphere de S*, cle exacte)."""
        if b not in self.ball_cache:
            sph = self.ref._sphere(tuple(sorted(self.ref_site[s] for s in self.cat.sstar[b])))
            self.ball_cache[b] = None if sph is None else self.ref._by_key.get(sph[2])
        return self.ball_cache[b]

    def site_ball(self, site):
        """Boule de rayon nul du site (naissance de l'ordre 1 dans la reference)."""
        return self.ref._by_key[self.ref._sphere((self.ref_site[site],))[2]]

    def expected_cells(self, k):
        births, cells = set(), {}
        for ball in self.ref.balls:
            if ball.lo <= k <= ball.hi and ball.level > 0:
                kind, reps = self.ref.cell(ball, k)
                if kind == 'birth':
                    births.add(ball.index)
                else:
                    cells[ball.index] = (kind, len(reps))
        return births, cells

    def traces(self, order, c, ball):
        """Traces de la cellule c : identifiants internes tries, et masques sur la coquille de la reference."""
        b = order.cell_balls[c]
        population = list(self.cat.population(b))
        inner, shell = population[:self.cat.p[b]], population[self.cat.p[b]:]
        rank = dict((x, j) for j, x in enumerate(ball.shell))
        out = []
        for r in order.traces(c):
            chosen = [shell[j] for j in g_dump.bits_of(order.masks[r])]
            part = tuple(sorted(self.internal[s] for s in inner + chosen))
            mask = 0
            for s in chosen:
                mask |= 1 << rank[self.internal[s]]
            out.append((r, part, mask, tuple(g_dump.bits_of(order.masks[r]))))
        return out

    def target_ball(self, order, k, target):
        if target & g_dump.CELL_BIT:
            return 'cellule', self.ref_ball(order.cell_balls[target & g_dump.INDEX_MASK])
        key = order.birth_keys[target]
        return 'naissance', self.site_ball(key) if k == 1 else self.ref_ball(key)

    def open_component(self, k, part, level):
        tree = self.ref._trees[k]
        node = self.ref._birth_node[k][self.ref.descend(part, k)]
        while tree.parent[node] >= 0 and tree.nodes[tree.parent[node]].level < level:
            node = tree.parent[node]
        return node

    def judge_cell(self, order, k, c, ball):
        """Traces, cibles et composantes de la cellule c (boule de la reference ball)."""
        t = k - ball.p
        separable = self.ref._separable(ball)
        expected = sum(1 for idx in combinations(range(ball.m), t) if separable(sum(1 << i for i in idx)))
        traces = self.traces(order, c, ball)
        if len(traces) != expected:
            self.gap('ordre %d cellule %d : %d traces, %d parties separables' % (k, c, len(traces), expected))
        previous = None
        for r, part, mask, positions in traces:
            if len(part) != k or not separable(mask) or (previous is not None and not previous < positions):
                self.gap('ordre %d cellule %d : trace %d non separable, mal formee ou hors ordre' % (k, c, r))
                return
            previous = positions
            kind, index = self.ref.resolve_v12(part, k)
            got_kind, got = self.target_ball(order, k, order.targets[r])
            if got is None or (got_kind, got.index) != (kind, index):
                self.gap('ordre %d cellule %d trace %d : cible %s, resolve_v12 %r' % (k, c, r, got_kind, (kind, index)))
                return
            self.totals['targets'] += 1
            self.totals['cell_targets'] += got_kind == 'cellule'
            target_part = part if got_kind == 'naissance' else None
            if got_kind == 'naissance':
                node = self.ref._birth_node[k][got.index]
                tree = self.ref._trees[k]
                while tree.parent[node] >= 0 and tree.nodes[tree.parent[node]].level < ball.level:
                    node = tree.parent[node]
            else:
                _kind, reps = self.ref.cell(got, k)
                target_part = tuple(sorted(got.inner + reps[0]))
                node = self.open_component(k, target_part, ball.level)
            if node != self.open_component(k, part, ball.level):
                self.gap('ordre %d cellule %d trace %d : cible hors de la composante a la coupe ouverte' % (k, c, r))
                return

    def judge_order(self, k):
        order = self.res['orders'][k - 1]
        self.ref.order(k)  # foret de l'etage B (regle v12, jugee egale a la definition par test_resolution_v12.py)
        births, cells = self.expected_cells(k)
        if k == 1:
            if list(order.birth_keys) != list(range(self.cat.sites)):
                self.gap('ordre 1 : les naissances ne sont pas les sites')
        elif set(self.ref_ball(b).index for b in order.birth_keys) != births or len(order.birth_keys) != len(births):
            self.gap('ordre %d : naissances differentes' % k)
        got = {}
        for c in range(len(order.cell_balls)):
            ball = self.ref_ball(order.cell_balls[c])
            if ball is None or ball.index in got or order.cell_ranks[c] != self.cat.rank[order.cell_balls[c]]:
                self.gap('ordre %d cellule %d : boule absente, en double ou de rang faux' % (k, c))
                return
            got[ball.index] = 'inert' if order.cell_flags[c] & g_dump.INERT else 'join'
            if (order.cell_flags[c] & g_dump.EXTENDED != 0) != (not ball.regular):
                self.gap('ordre %d cellule %d : drapeau de coquille etendue faux' % (k, c))
            self.judge_cell(order, k, c, ball)
        if got != dict((b, kind) for b, (kind, _n) in cells.items()):
            self.gap('ordre %d : cellules de fenetre differentes de la reference' % k)
        counters = order.counters
        want = (len(births) if k > 1 else self.cat.sites, len(cells),
                sum(1 for kind, _n in cells.values() if kind == 'inert'))
        if (counters['births'], counters['cells'], counters['inert_cells']) != want:
            self.gap('ordre %d : compteurs de l\'objet %r, attendu %r' % (k, (counters['births'], counters['cells'],
                                                                              counters['inert_cells']), want))
        for key in ('cells', 'inert_cells', 'extended_cells', 'census_saturated', 'census_complete', 'inert_steps',
                    'jumps_catalogue', 'jumps_census', 'cell_stops', 'birth_stops', 'route_t1', 'route_cert_census'):
            self.totals[key] = self.totals.get(key, 0) + counters[key]


def main(argv):
    if len(argv) < 2 or not os.path.isfile(argv[1]):
        print('g_oracle_refus usage : g_oracle.py <mhgp12_tower_probe> [--min-clouds=N] [--min-targets=N] ...')
        return 2
    floors = dict(FLOORS)
    for item in argv[2:]:
        key, _, value = item.partition('=')
        if key not in floors or not value.isdigit():
            print('g_oracle_refus option %s' % item)
            return 2
        floors[key] = int(value)
    errors, clouds, refused = [], 0, 0
    totals = {'targets': 0, 'cell_targets': 0}
    with tempfile.TemporaryDirectory() as folder:
        for cloud in families.fast_suite():
            if len(cloud.points) > MAX_SITES:
                continue
            code, text, cat, res = run_probe(argv[1], folder, cloud.points, cloud.kmax, clouds)
            clouds += 1
            if len(set(cloud.points)) != len(cloud.points):
                if code != 2 or '"reason":"multiplicity_unsupported"' not in text:
                    errors.append('%s : doublons non refuses (code %d)' % (cloud.name, code))
                refused += 1
                continue
            if code != 0:
                errors.append('%s : sonde en refus (code %d) %s' % (cloud.name, code, text[-200:]))
                continue
            try:
                judge = Judge(cloud.points, cloud.kmax, cat, res, cloud.name, errors, totals)
                if len(res['orders']) != judge.ref.orders:
                    errors.append('%s : %d ordres, attendu %d' % (cloud.name, len(res['orders']), judge.ref.orders))
                    continue
                for k in range(1, judge.ref.orders + 1):
                    judge.judge_order(k)
            except InvariantError as error:
                print('g_oracle_invariant %s : %s' % (cloud.name, error))
                return 3
    for error in errors[:20]:
        print('ecart %s' % error)
    if errors:
        print('g_oracle_ecart ecarts=%d' % len(errors))
        return 1
    print('totaux ' + ' '.join('%s=%d' % (key, totals[key]) for key in sorted(totals)))
    floors_met = (clouds >= floors['--min-clouds'] and totals['targets'] >= floors['--min-targets'] and
                  totals['cell_targets'] >= floors['--min-cell-targets'])
    for key in ('inert_cells', 'extended_cells', 'census_saturated', 'census_complete', 'inert_steps', 'jumps_census',
                'jumps_catalogue', 'birth_stops'):
        floors_met = floors_met and totals.get(key, 0) >= 1
    if not floors_met:
        print('g_oracle_plancher nuages=%d cibles=%d' % (clouds, totals['targets']))
        return 3
    print('g_oracle_ok nuages=%d doublons_refuses=%d cibles=%d cibles_cellule=%d cellules=%d inertes=%d'
          % (clouds, refused, totals['targets'], totals['cell_targets'], totals['cells'], totals['inert_cells']))
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
