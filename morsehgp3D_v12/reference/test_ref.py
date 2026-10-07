#!/usr/bin/env python3
"""Portes de la reference exacte de la v12 (port de la v11) : l'etage B (voie du moteur) doit EGALER l'etage A
(definition).

    python3 test_ref.py --suite=fast                          faits graves, puis B contre A sur la suite rapide
    python3 test_ref.py --suite=S --jobs=J                    la meme chose, la suite repartie sur J processus
    python3 test_ref.py --suite=S --shard=i/N --report=F      une tranche : nuages de rang i modulo N, rapport JSON
    python3 test_ref.py --suite=S --collect=DIR --shards=N    somme les rapports DIR/S_i_N.json, faits et planchers
    python3 test_ref.py --inject=NOM                          mutant : correctif applique a une copie de hgp12_ref
    python3 test_ref.py --list-mutants
    python3 -m unittest test_ref                              depuis ce dossier : faits graves et suite rapide

La suite complete (S = full) est faite pour N tranches jouees en parallele (une porte CTest par tranche, puis la
porte qui les somme) : ses compteurs ne dependent pas de N.

Codes de sortie (ARCHITECTURE.md de la v11, paragraphe 5) : 0 conforme ; 1 desaccord d'un juge ; 2 refus avant calcul
(usage, rapport de tranche absent ou d'une autre suite) ; 3 plancher ou invariant viole ; 4 mutant tue (--inject
seulement ; un mutant qui survit rend 0).

Python 3.10 nu, aucune dependance ; aucun test ne repose sur l'instruction d'assertion du langage : la porte rend le
meme code sous python3 -O.
"""
import ast
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from fractions import Fraction
from itertools import combinations_with_replacement

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import interval_oracle  # noqa: E402
import ref_mutants  # noqa: E402
from hgp12_ref import Definition, Reference, dumps, families, judge  # noqa: E402
from hgp12_ref import intgeom as G  # noqa: E402
from hgp12_ref.model import Cut, OrderResult, members  # noqa: E402

OK, DISAGREEMENT, REFUSAL, FLOOR, MUTANT_KILLED = 0, 1, 2, 3, 4
FACTS = 15  # methodes de EngravedFacts

# Compteurs EXACTS de la suite rapide : toute derive est un plancher viole. La suite est deterministe (generateur
# ecrit dans families.py ; memes nombres sous toute graine de hachage, sous python3 -O, en 1 ou 3 processus). Ces
# nombres ne changent que si une famille, une fixture ou le juge change, et se regravent alors en connaissance de
# cause.
FAST_EXACT = dict(
    clouds=342, orders=1362, cuts=48234, levels=21771, nodes=13029, births=8205, merges=4824, nary_merges=1580,
    plateau_levels=142, mixed_levels=392, verticals=9641, core_entries=8232, core_at_node_level=3127,
    cover_entries=8232, cover_ties=595, balls=9318, extended_balls=760, weighted_balls=264, general_births=794,
    general_joins=954, general_inert=1113, jumps=145, descents=56405, steps=10501, adhoc_spheres=34,
    weighted_clouds=34)
# Suite complete : elle n'a pas ete jouee en entier ici (calcul reserve a la VM G4). Sont exacts par construction les
# nombres de nuages, d'ordres, d'entrees et de nuages a doublons. Les autres compteurs ont pour plancher huit fois le
# total de la suite rapide : la suite complete a 16,5 fois plus de nuages, et plus grands (une tranche d'un
# quatre-vingt-seizieme, jouee le 2 octobre 2026, donne 2 a 10 fois ces taux par nuage). A remplacer par les totaux
# exacts apres le premier passage sur G4.
FULL_EXACT = dict(clouds=5617, orders=38633, core_entries=373609, cover_entries=373609, weighted_clouds=514)
FULL_FLOORS = dict((name, 8 * FAST_EXACT[name]) for name in judge.COUNTERS if name not in FULL_EXACT)
EXACT = {'fast': FAST_EXACT, 'full': FULL_EXACT}

FIXTURE = dict((c.name, c) for c in families.fixtures())
INTERVAL_EXACT = (41112, 39012, 25762)  # coupes, composantes et images verticales jugees par l'attendu d'intervalles


def labels(mask):
    return ''.join('ABCDEFGHIJKLMNOP'[i] for i in members(mask))


class EngravedFacts(unittest.TestCase):
    """Faits graves : valeurs exactes ecrites ici, independantes des deux etages (elles gardent contre une faute
    commune) ; puis controles unitaires de la reference."""

    def test_two_triangles_full2(self):
        """These, section 6.1, K = 2 : quatre paires obliques a 999956, sept paires a 10^6, puis ABC | CD | DEF au
        cercle circonscrit, puis tout. Valeurs retrouvees par l'oracle independant de l'audit L03 (2 octobre 2026)."""
        want = {
            'two_triangles': [('999956', 'AC BC DE DF'), ('1000000', 'AB AC BC CD DE DF EF'),
                              ('249978000484/187489', 'ABC CD DEF'), ('3731956', 'ABCDEF')],
            'two_triangles_1998': [('998001', 'CD'), ('999956', 'AC BC CD DE DF'),
                                   ('1000000', 'AB AC BC CD DE DF EF'), ('249978000484/187489', 'ABC CD DEF'),
                                   ('3728225', 'ABCDEF')],
            'two_triangles_1700': [('722500', 'CD'), ('999956', 'AC BC CD DE DF'),
                                   ('1000000', 'AB AC BC CD DE DF EF'), ('249978000484/187489', 'ABC CD DEF'),
                                   ('3194656', 'ABCDEF')],
        }
        for name, expected in want.items():
            pts = FIXTURE[name].points
            for stage in (Definition(pts), Reference(pts, 2)):
                got, last = [], None
                for cut in stage.order(2).cuts:
                    cur = ' '.join(sorted(labels(cov) for _v, cov, _cor in cut.closed))
                    if cur != last:
                        got.append((str(cut.level), cur))
                        last = cur
                self.assertEqual(got, expected, '%s, etage %s' % (name, type(stage).__name__))
        a = Definition(FIXTURE['two_triangles'].points).order(2)
        self.assertEqual([(str(n.level), n.children) for n in a.nodes],
                         [('999956', ())] * 4 + [('1000000', ())] * 3 +
                         [('249978000484/187489', (0, 1, 4)), ('249978000484/187489', (2, 3, 6)),
                          ('3731956', (5, 7, 8))])
        self.assertEqual(set(str(e.level) for e in a.core), set(['3999824']))  # core : rien avant 1999,956
        self.assertEqual([len(e.nodes) for e in a.cover], [1, 1, 2, 2, 1, 1])  # C et D : deux composantes couvrantes

    def test_e5(self):
        """E5 de la v10 : tailles des arbres et niveaux des racines (dump du binaire fige du 29 septembre)."""
        a = Definition(FIXTURE['e5'].points)
        self.assertEqual([(len(a.order(k).nodes), a.order(k).nodes[-1].level) for k in range(1, 5)],
                         [(8, Fraction(62, 4)), (10, Fraction(335544, 14252)), (8, Fraction(183168, 6848)),
                          (5, Fraction(5043456, 186624))])

    def test_square_and_octahedron(self):
        """Coquilles etendues : le carre nait une seule fois a l'ordre 3 ; l'octaedre a douze naissances d'ordre 2
        (ses aretes), huit d'ordre 3 (ses faces), une d'ordre 4, et une seule fusion par ordre."""
        a = Definition(FIXTURE['square'].points)
        self.assertEqual([(str(n.level), n.children) for n in a.order(2).nodes],
                         [('1', ())] * 4 + [('2', (0, 1, 2, 3))])
        self.assertEqual([(str(n.level), n.children) for n in a.order(3).nodes], [('2', ())])
        o = Definition(FIXTURE['octa'].points)
        shape = [[(str(n.level), len(n.children)) for n in o.order(k).nodes] for k in (2, 3, 4)]
        self.assertEqual(shape, [[('25/2', 0)] * 12 + [('50/3', 12)], [('50/3', 0)] * 8 + [('25', 8)], [('25', 0)]])

    def test_line5_entry_at_merge_level(self):
        """line5, ordre 2 : trois naissances au niveau 1 ; les points 0, 2 et 4 entrent a D_2 = 4, exactement au
        niveau de la fusion de {0, 2} et {2, 4}, donc dans le noeud de cette fusion (coupe fermee)."""
        a = Definition(FIXTURE['line5'].points).order(2)
        self.assertEqual([(str(n.level), n.children) for n in a.nodes],
                         [('1', ()), ('1', ()), ('1', ()), ('9', ()), ('4', (0, 1)), ('16', (2, 3, 4))])
        self.assertEqual([(str(e.level), e.nodes) for e in a.core], [('4', 4), ('4', 4), ('4', 4), ('4', 2), ('4', 2)])

    def test_multiplicities(self):
        """Doublons comptes comme des points (L01, annexe B) : un site de poids 4 nait au niveau nul a chaque ordre ;
        la paire de poids (3, 1) est critique aux ordres 1 et 4 ; le triangle aigu de poids (3, 1, 1) aux ordres
        2, 4 et 5 ; celui de poids (2, 1, 1) aux ordres 2, 3 et 4."""
        a = Definition(FIXTURE['all_equal'].points)
        self.assertEqual([[(n.level, n.children) for n in a.order(k).nodes] for k in range(1, 5)], [[(0, ())]] * 4)
        for name, qmin, want in (('pair_weighted', 2, [(1, 'join'), (2, 'inert'), (3, 'inert'), (4, 'birth')]),
                                 ('triangle_weighted_311', 3, [(2, 'join'), (3, 'inert'), (4, 'join'), (5, 'birth')]),
                                 ('triangle_weighted', 3, [(2, 'join'), (3, 'join'), (4, 'birth')])):
            c = FIXTURE[name]
            ref = Reference(c.points, c.kmax)
            ball = [b for b in ref.balls if b.qmin == qmin and len(b.shell_sites) == qmin and b.m == len(c.points)]
            self.assertEqual(len(ball), 1, name)
            self.assertEqual([(k, ref.cell(ball[0], k)[0]) for k in range(ball[0].lo, ball[0].hi + 1)], want, name)

    def test_level_collisions(self):
        """Niveaux : meme niveau exact ecrit de deux facons (paire et triangle) ; deux niveaux exacts distincts de
        meme double ; boule q_min = 3 a coquille etendue ecrite dans la forme d'un tetraedre."""
        ref = Reference(FIXTURE['circle25_pair'].points, 3)
        forms = set(dumps.emitted_level(ref, b) for b in ref.balls if b.level == 25)
        self.assertEqual(forms, set([(100, 4), (409600, 16384)]))
        cat = dumps.EngineCatalogue(Reference(FIXTURE['double_collision'].points, 3))
        low, high = sorted(lv for lv in cat.rank_of if float(lv) == 1728896403.0)
        self.assertEqual((low, high), (Fraction(1728896403), Fraction(1328481229830955201, 768398400)))
        self.assertEqual(cat.rank_of[high], cat.rank_of[low] + 1)
        ref = Reference(FIXTURE['q3_tetra_form'].points, 5)
        forms = [(dumps.emitted_level(ref, b), G.level3(*[ref.sites[s] for s in b.support]))
                 for b in ref.balls if b.qmin == 3 and b.extended]
        self.assertEqual([f for f in forms if f[0] != f[1]], [((82944, 9216), (18432, 2048))])

    def test_cover_tie(self):
        """Egalite exacte de premiere couverture (fixture historique de la v10, n = 4, K = 3) : au niveau 5 deux
        composantes couvrent chacun des deux premiers points ; la verite est cet ensemble, et la regle de la v10
        (premiere boule couvrante) en retient un element."""
        pts = FIXTURE['cover_tie_n4'].points
        a, ref = Definition(pts), Reference(pts, 3)
        self.assertEqual([a.beta(part) for part in ((0, 1, 2), (0, 1, 3), (0, 2, 3), (1, 2, 3), (0, 1, 2, 3))],
                         [5, 5, Fraction(25, 2), Fraction(25, 2), Fraction(25, 2)])
        cover = a.order(3).cover
        self.assertEqual([(e.level, len(e.nodes)) for e in cover], [(5, 2), (5, 2), (5, 1), (5, 1)])
        ref.order(3)
        chosen = [node for _ball, node in ref.cover_choice[3]]
        self.assertTrue(all(node in cover[x].nodes for x, node in enumerate(chosen)))
        self.assertEqual(chosen[0], chosen[1])

    def test_interval_oracle(self):
        """Troisieme attendu, independant des deux etages et de leurs representations : sur une droite, la region
        ou D_k <= r^2 est la reunion des intervalles [x_(j+k-1) - r, x_j + r] des fenetres de k points consecutifs
        (inegalites strictes a la coupe ouverte). Sur les 251 multiensembles d'au plus 5 points de {0, 1, 2, 4, 7},
        a tous les ordres, aux rayons d'evenement, entre eux et au-dela : composantes, points couverts, points de
        coeur et images verticales des deux etages egalent cet attendu. Ni boule minimale, ni graphe, ni numerotation,
        ni lecture de coupe partagee : la coupe d'un etage est relue ici sur ses enregistrements."""
        counts = dict(clouds=0, weighted=0, cuts=0, components=0, verticals=0)
        for size in range(1, 6):
            for positions in combinations_with_replacement((0, 1, 2, 4, 7), size):
                points = [(x, 0, 0) for x in positions]
                for stage in (Definition(points), Reference(points, size)):
                    results = [stage.order(k) for k in range(1, size + 1)]
                    fault = interval_oracle.interval_check(positions, results, counts)
                    if fault:
                        self.fail('%r, etage %s : %s' % (positions, type(stage).__name__, fault))
                counts['clouds'] += 1
                counts['weighted'] += len(set(positions)) < size
        self.assertEqual((counts['clouds'], counts['weighted']), (251, 220))
        self.assertEqual((counts['cuts'], counts['components'], counts['verticals']), INTERVAL_EXACT)
        # sensibilite : un resultat dont les coupes fermees sont remplacees par les coupes ouvertes est refuse
        res = Definition([(0, 0, 0), (2, 0, 0), (4, 0, 0)]).order(1)
        tampered = OrderResult(1, res.nodes, None, res.core, res.cover,
                               [Cut(c.level, c.opened, c.opened) for c in res.cuts])
        self.assertIsNone(interval_oracle.interval_check((0, 2, 4), [res], dict(counts)))
        self.assertIsNotNone(interval_oracle.interval_check((0, 2, 4), [tampered], dict(counts)))

    def test_generator_is_engraved(self):
        """SplitMix64 : premieres sorties de la graine 1 (valeurs de reference publiees) et premier nuage."""
        rng = families.Rng(1)
        self.assertEqual([rng.next() for _ in range(3)],
                         [10451216379200822465, 13757245211066428519, 17911839290282890590])
        self.assertEqual(families.family('grid3', 1, 5, 5, 3, 7)[0].points,
                         [(2, 2, 1), (2, 0, 2), (1, 2, 1), (2, 1, 2), (1, 0, 0)])
        self.assertEqual((len(families.fast_suite()), len(families.full_suite())), (342, 5617))

    def test_minimal_balls_two_routes(self):
        """Boule minimale : enumeration en fractions (etage A) contre formules entieres (etage B), sur toutes les
        parties de cinq fixtures."""
        checked = 0
        for name in ('e5', 'octa', 'cube', 'q3_tetra_form', 'square_weighted'):
            pts = FIXTURE[name].points
            a, b = Definition(pts), Reference(pts, 2)
            for mask in range(1, 1 << len(pts)):
                part = tuple(members(mask))
                level, center, _closed = a.meb(part)
                _anchor, _ctr, key = b._meb(tuple(sorted(b.internal[i] for i in part)))
                if key != (center, level):
                    self.fail('%s %r : %r contre %r' % (name, part, (center, level), key))
                checked += 1
        self.assertEqual(checked, 31 + 63 + 255 + 31 + 255)

    def test_regular_shortcut_is_gordan(self):
        """Raccourci analytique des coquilles regulieres (naissance a t = m, m morceaux a t = m - 1, un seul en
        dessous) contre l'enumeration generale des morceaux, sur toutes les boules regulieres de la suite rapide."""
        cells = joins = 0
        for cloud in families.fast_suite():
            ref = Reference(cloud.points, cloud.kmax)
            for ball in ref.balls:
                if not ball.regular or ball.level == 0:
                    continue
                for t in range(1, ball.m + 1):
                    short = ref._local(ball, t)
                    ball.regular = False
                    general = ref._local(ball, t)
                    ball.regular = True
                    same = short[0] == general[0] and (short[0] != 'join' or set(short[1]) == set(general[1]))
                    if not same:
                        self.fail('%s boule %d t = %d : %r contre %r' % (cloud.name, ball.index, t, short, general))
                    cells += 1
                    joins += short[0] == 'join'
        self.assertGreaterEqual(cells, 15000)
        self.assertGreaterEqual(joins, 5000)

    def test_admission_rules(self):
        """Regle d'admission 'single' (p + q_min <= K + 1 partout) contre la regle 'v10' (p <= K - 1 si la coquille
        est ponderee) : meme catalogue sans doublon, sous-catalogue avec doublons, et meme tour dans tous les cas.
        Fait grave : a K = 1, la boule du triangle aigu de poids (2, 1, 1) n'est admise que par la regle 'v10'."""
        extra = towers = 0
        for cloud in families.fixtures():
            for kmax in range(1, cloud.kmax + 1):
                v10, single = Reference(cloud.points, kmax), Reference(cloud.points, kmax, admission='single')
                keys = [set((b.center, b.level) for b in r.balls) for r in (v10, single)]
                if not keys[1] <= keys[0] or (len(v10.sites) == v10.n and keys[0] != keys[1]):
                    self.fail('%s K=%d : catalogues des deux regles' % (cloud.name, kmax))
                extra += len(keys[0]) - len(keys[1])
                for k in range(1, v10.orders + 1):
                    if judge.compare_orders(v10.order(k), single.order(k)):
                        self.fail('%s K=%d ordre %d : tours des deux regles' % (cloud.name, kmax, k))
                    towers += 1
        pts = FIXTURE['triangle_weighted'].points
        single = set((b.center, b.level) for b in Reference(pts, 1, admission='single').balls)
        only = [b for b in Reference(pts, 1).balls if (b.center, b.level) not in single]
        self.assertEqual([(b.qmin, b.p, b.m, b.level) for b in only], [(3, 0, 4, Fraction(289, 25))])
        self.assertEqual((extra, towers), (14, 332))

    def test_mutant_table_is_current(self):
        """Table des mutants : chaque motif est present une seule fois dans son fichier, change le texte, et cite
        des fixtures connues ; le manifeste au format du socle porte les mutants reels."""
        for table in (ref_mutants.MUTANTS, ref_mutants.DUMP_MUTANTS):
            for name, mutant in sorted(table.items()):
                with open(os.path.join(HERE, 'hgp12_ref', mutant['file']), encoding='ascii') as f:
                    text = f.read()
                self.assertEqual(text.count(mutant['old']), 1, name)
                self.assertNotEqual(mutant['old'], mutant['new'], name)
                self.assertGreaterEqual(len(ref_mutants.clouds_of(mutant, families.fast_suite())), 1, name)
        real = sorted(name for name, m in ref_mutants.MUTANTS.items() if not m['equivalent'])
        manifest = ref_mutants.socle_manifest()
        self.assertEqual([m['id'] for m in manifest['mutants']], real)
        self.assertEqual((len(real), len(ref_mutants.MUTANTS), len(ref_mutants.DUMP_MUTANTS)), (22, 25, 10))

    def test_floors_detect_drift(self):
        """Planchers : les compteurs exacts de la suite rapide passent ; un compteur derive d'une unite, ou une
        suite complete vide, sont refuses."""
        self.assertEqual(floors('fast', dict(FAST_EXACT)), [])
        drifted = dict(FAST_EXACT, nary_merges=FAST_EXACT['nary_merges'] - 1)
        self.assertEqual(floors('fast', drifted),
                         ['nary_merges %d != %d' % (drifted['nary_merges'], FAST_EXACT['nary_merges'])])
        self.assertEqual(len(floors('full', judge.new_counters())), len(judge.COUNTERS))

    def test_python_310_syntax(self):
        """Les sources se lisent avec la grammaire de Python 3.10 (celui de la VM G4)."""
        seen = 0
        for folder in (HERE, os.path.join(HERE, 'hgp12_ref')):
            for name in sorted(os.listdir(folder)):
                if name.endswith('.py'):
                    with open(os.path.join(folder, name), encoding='ascii') as f:
                        ast.parse(f.read(), filename=name, feature_version=(3, 10))
                    seen += 1
        self.assertGreaterEqual(seen, 10)


class FastCampaign(unittest.TestCase):
    """La suite rapide sous unittest (python3 -m unittest test_ref) : B egale A, compteurs exacts. La porte CTest
    joue la meme campagne par main(), avec les codes de sortie des portes."""

    def test_b_equals_a_on_fast_suite(self):
        report = run_shard('fast', 0, 1)
        self.assertEqual(report['errors'], [])
        self.assertEqual(floors('fast', report['counters']), [])


# ---------------------------------------------------------------- campagne B contre A

def suite_digest(suite):
    """Empreinte de la definition d'une suite : deux tranches ne se somment que si elles viennent de la meme."""
    h = hashlib.sha256()
    for cloud in families.SUITES[suite]():
        h.update(repr((cloud.name, cloud.points, cloud.kmax)).encode('ascii'))
    return h.hexdigest()


def run_shard(suite, shard, shards):
    """Joue les nuages de rang i tel que i mod shards == shard. Rend le rapport de la tranche."""
    cnt = judge.new_counters()
    errors = []
    for i, cloud in enumerate(families.SUITES[suite]()):
        if i % shards != shard:
            continue
        found = judge.compare_cloud(cloud.points, cloud.kmax, cnt)[0]
        if found:
            errors.append('%s K=%d %r : %s' % (cloud.name, cloud.kmax, cloud.points, found[0]))
    return dict(suite=suite, shard=shard, shards=shards, digest=suite_digest(suite), errors=errors, counters=cnt)


def report_path(folder, suite, shard, shards):
    return os.path.join(folder, '%s_%d_%d.json' % (suite, shard, shards))


def collect(folder, suite, shards):
    """Lit les rapports des tranches. Rend (ecarts, compteurs), ou None si un rapport manque ou est etranger."""
    errors, cnt = [], judge.new_counters()
    digest = suite_digest(suite)
    for shard in range(shards):
        path = report_path(folder, suite, shard, shards)
        try:
            with open(path) as f:
                report = json.load(f)
        except (OSError, ValueError) as exc:
            print('refus : rapport de tranche illisible %s : %s' % (path, exc))
            return None
        if [report.get(key) for key in ('suite', 'shard', 'shards', 'digest')] != [suite, shard, shards, digest]:
            print('refus : rapport %s d\'une autre suite ou d\'une autre tranche' % path)
            return None
        errors.extend(report['errors'])
        for name in judge.COUNTERS:
            cnt[name] += report['counters'][name]
    return errors, cnt


def run_jobs(suite, jobs):
    """Une tranche par processus fils, rapports dans un dossier temporaire, puis la somme. Rend comme collect."""
    folder = tempfile.mkdtemp(prefix='hgp12_ref_')
    try:
        procs = [subprocess.Popen([sys.executable, os.path.abspath(__file__), '--suite=' + suite,
                                   '--shard=%d/%d' % (shard, jobs),
                                   '--report=' + report_path(folder, suite, shard, jobs)], stdout=subprocess.DEVNULL)
                 for shard in range(jobs)]
        codes = [proc.wait() for proc in procs]
        if any(code not in (OK, DISAGREEMENT) for code in codes):
            print('refus : tranche terminee par les codes %r' % (codes,))
            return None
        return collect(folder, suite, jobs)
    finally:
        shutil.rmtree(folder, ignore_errors=True)


def floors(suite, cnt):
    """Planchers de non-vacuite : liste des manques (vide si tout est tenu)."""
    low = ['%s %d != %d' % (name, cnt[name], want) for name, want in sorted(EXACT[suite].items()) if cnt[name] != want]
    if suite == 'full':
        low.extend('%s %d < %d' % (name, cnt[name], want) for name, want in sorted(FULL_FLOORS.items())
                   if cnt[name] < want)
    return low


def verdict(suite, errors, cnt):
    """Faits graves, ecarts, planchers ; ligne finale ; code."""
    facts = unittest.TextTestRunner(stream=sys.stdout, verbosity=1).run(
        unittest.defaultTestLoader.loadTestsFromTestCase(EngravedFacts))
    for line in errors[:10]:
        print('ECART %s' % line)
    print('reference_%s faits=%d ecarts=%d %s' % (suite, facts.testsRun, len(errors),
                                                  ' '.join('%s=%d' % (k, cnt[k]) for k in judge.COUNTERS)))
    if errors or not facts.wasSuccessful():
        return DISAGREEMENT
    low = floors(suite, cnt) + (['faits %d != %d' % (facts.testsRun, FACTS)] if facts.testsRun != FACTS else [])
    if low:
        print('PLANCHER : %s' % ', '.join(low))
        return FLOOR
    print('reference_%s_ok nuages=%d ordres=%d coupes=%d noeuds=%d'
          % (suite, cnt['clouds'], cnt['orders'], cnt['cuts'], cnt['nodes']))
    return OK


# ---------------------------------------------------------------- mutants

def run_mutant(name):
    """Fixtures du mutant jouees sur les sources intactes (temoin), puis sur la copie mutee. Rend le code."""
    mutant = ref_mutants.MUTANTS[name]
    clouds = ref_mutants.clouds_of(mutant, families.fast_suite())
    for cloud in clouds:
        found = judge.compare_cloud(cloud.points, cloud.kmax)[0]
        if found:
            print('temoin non conforme sur %s : %s' % (cloud.name, found[0]))
            return DISAGREEMENT
    try:
        mutated = ref_mutants.load(name, os.path.join(HERE, 'hgp12_ref'))
    except ref_mutants.StalePatch as exc:
        print('PLANCHER : mutant %s inapplicable : %s' % (name, exc))
        return FLOOR
    killers = []
    for cloud in clouds:
        try:
            found = mutated.judge.compare_cloud(cloud.points, cloud.kmax)[0]
        except Exception as exc:  # une exception hors invariant tue aussi le mutant
            found = ['exception %s : %s' % (type(exc).__name__, exc)]
        if found:
            killers.append('%s : %s' % (cloud.name, found[0]))
    if killers:
        print('mutant %s tue par %d fixture(s) sur %d ; premiere : %s'
              % (name, len(killers), len(clouds), killers[0][:300]))
        if mutant['silent'] and any('etage' in k or 'exception' in k for k in killers):
            print('PLANCHER : mutant %s tue par un invariant, pas par la comparaison des deux etages' % name)
            return FLOOR
        print('mutant_killed %s' % name)
        return MUTANT_KILLED
    print('mutant_survives %s' % name)
    return OK


# ---------------------------------------------------------------- lancement

def usage(message):
    print('refus : %s' % message)
    print(__doc__)
    return REFUSAL


def parse(argv):
    """Options lues ; None et un message si l'usage est faux."""
    opts = dict(suite=None, shard=None, jobs=None, report=None, collect=None, shards=None, inject=None, listing=False)
    for arg in argv:
        key, _eq, value = arg.partition('=')
        if key == '--suite' and value in families.SUITES:
            opts['suite'] = value
        elif key == '--shard' and len(value.split('/')) == 2 and all(p.isdigit() for p in value.split('/')):
            opts['shard'] = tuple(int(p) for p in value.split('/'))
        elif key in ('--jobs', '--shards') and value.isdigit() and 1 <= int(value) <= 256:
            opts[key[2:]] = int(value)
        elif key in ('--report', '--collect') and value:
            opts[key[2:]] = value
        elif key == '--inject' and value in ref_mutants.MUTANTS:
            opts['inject'] = value
        elif arg == '--list-mutants':
            opts['listing'] = True
        else:
            return None, 'argument %r' % arg
    given = sorted(key for key, value in opts.items() if value)
    allowed = (['suite'], ['jobs', 'suite'], ['report', 'shard', 'suite'], ['collect', 'shards', 'suite'], ['inject'],
               ['listing'])
    if given not in allowed:
        return None, 'combinaison d\'options %r' % (given,)
    if opts['shard'] and not opts['shard'][0] < opts['shard'][1] <= 256:
        return None, 'tranche %r' % (opts['shard'],)
    return opts, None


def main(argv):
    opts, why = parse(argv)
    if opts is None:
        return usage(why)
    if opts['listing']:
        for name in sorted(ref_mutants.MUTANTS):
            print('%s %s' % (name, 'equivalent' if ref_mutants.MUTANTS[name]['equivalent'] else 'reel'))
        return OK
    if opts['inject']:
        return run_mutant(opts['inject'])
    suite = opts['suite']
    if opts['shard']:
        report = run_shard(suite, opts['shard'][0], opts['shard'][1])
        folder = os.path.dirname(os.path.abspath(opts['report']))
        os.makedirs(folder, exist_ok=True)
        with open(opts['report'] + '.tmp', 'w') as f:
            json.dump(report, f, sort_keys=True)
        os.replace(opts['report'] + '.tmp', opts['report'])
        for line in report['errors'][:10]:
            print('ECART %s' % line)
        print('reference_%s tranche=%d/%d nuages=%d ecarts=%d'
              % (suite, report['shard'], report['shards'], report['counters']['clouds'], len(report['errors'])))
        if report['errors']:
            return DISAGREEMENT
        return OK if report['counters']['clouds'] > 0 else FLOOR
    if opts['collect']:
        result = collect(opts['collect'], suite, opts['shards'])
    elif opts['jobs']:
        result = run_jobs(suite, opts['jobs'])
    else:
        report = run_shard(suite, 0, 1)
        result = report['errors'], report['counters']
    if result is None:
        return REFUSAL
    return verdict(suite, result[0], result[1])


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
