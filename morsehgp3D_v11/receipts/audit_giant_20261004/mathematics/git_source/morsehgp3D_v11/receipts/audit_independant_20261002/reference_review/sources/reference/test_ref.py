#!/usr/bin/env python3
"""Portes de la reference exacte de la v11 : l'etage B (voie du moteur) doit EGALER l'etage A (definition).

    python3 test_ref.py --suite=fast                          faits graves, puis B contre A sur la suite rapide
    python3 test_ref.py --suite=S --jobs=J                    la meme chose, la suite repartie sur J processus
    python3 test_ref.py --suite=S --shard=i/N --report=F      une tranche : nuages de rang i modulo N, rapport JSON
    python3 test_ref.py --suite=S --collect=DIR --shards=N    somme les rapports DIR/S_i_N.json, faits et planchers
    python3 test_ref.py --inject=NOM                          mutant : correctif applique a une copie de hgp11_ref
    python3 test_ref.py --list-mutants

La suite complete (S = full) est faite pour N tranches jouees en parallele (une porte CTest par tranche, puis la
porte qui les somme) : ses compteurs ne dependent pas de N.

Codes de sortie (docs/ARCHITECTURE.md, paragraphe 5) : 0 conforme ; 1 desaccord d'un juge ; 2 refus avant calcul
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

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ref_mutants  # noqa: E402
from hgp11_ref import Definition, Reference, dumps, families, judge  # noqa: E402
from hgp11_ref import intgeom as G  # noqa: E402
from hgp11_ref.model import members  # noqa: E402

OK, DISAGREEMENT, REFUSAL, FLOOR, MUTANT_KILLED = 0, 1, 2, 3, 4
FACTS = 11  # methodes de EngravedFacts

# Compteurs EXACTS de la suite rapide : toute derive est un plancher viole. La suite est deterministe (generateur
# ecrit dans families.py ; memes nombres sous toute graine de hachage, sous python3 -O, en 1 ou 3 processus). Ces
# nombres ne changent que si une famille, une fixture ou le juge change, et se regravent alors en connaissance de
# cause.
FAST_EXACT = dict(
    clouds=341, orders=1359, cuts=48204, levels=21756, nodes=13016, births=8196, merges=4820, nary_merges=1578,
    plateau_levels=142, mixed_levels=392, verticals=9634, core_entries=8220, core_at_node_level=3123,
    cover_entries=8220, cover_ties=593, balls=9308, extended_balls=760, weighted_balls=264, general_births=794,
    general_joins=954, general_inert=1113, jumps=145, descents=56357, steps=10492, adhoc_spheres=34,
    weighted_clouds=34)
# Suite complete : elle n'a pas ete jouee en entier ici (calcul reserve a la VM G4). Sont exacts par construction les
# nombres de nuages, d'ordres, d'entrees et de nuages a doublons. Les autres compteurs ont pour plancher huit fois le
# total de la suite rapide : la suite complete a 16,5 fois plus de nuages, et plus grands (une tranche d'un
# quatre-vingt-seizieme, jouee le 2 octobre 2026, donne 2 a 10 fois ces taux par nuage). A remplacer par les totaux
# exacts apres le premier passage sur G4.
FULL_EXACT = dict(clouds=5615, orders=38626, core_entries=373581, cover_entries=373581, weighted_clouds=514)
FULL_FLOORS = dict((name, 8 * FAST_EXACT[name]) for name in judge.COUNTERS if name not in FULL_EXACT)
EXACT = {'fast': FAST_EXACT, 'full': FULL_EXACT}

FIXTURE = dict((c.name, c) for c in families.fixtures())


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
                         [('249978000484/187489', (0, 1, 4)), ('249978000484/187489', (2, 3, 6)), ('3731956', (5, 7, 8))])
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
        self.assertEqual([(str(n.level), n.children) for n in a.order(2).nodes], [('1', ())] * 4 + [('2', (0, 1, 2, 3))])
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
            ball = [b for b in ref.balls if b.qmin == qmin and len(b.u_sites) == qmin and b.m == len(c.points)]
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

    def test_generator_is_engraved(self):
        """SplitMix64 : premieres sorties de la graine 1 (valeurs de reference publiees) et premier nuage."""
        rng = families.Rng(1)
        self.assertEqual([rng.next() for _ in range(3)],
                         [10451216379200822465, 13757245211066428519, 17911839290282890590])
        self.assertEqual(families.family('grid3', 1, 5, 5, 3, 7)[0].points,
                         [(2, 2, 1), (2, 0, 2), (1, 2, 1), (2, 1, 2), (1, 0, 0)])
        self.assertEqual((len(families.fast_suite()), len(families.full_suite())), (341, 5615))

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
        only = [b for b in Reference(pts, 1).balls
                if (b.center, b.level) not in set((c.center, c.level) for c in Reference(pts, 1, admission='single').balls)]
        self.assertEqual([(b.qmin, b.p, b.m, b.level) for b in only], [(3, 0, 4, Fraction(289, 25))])
        self.assertEqual((extra, towers), (14, 326))

    def test_python_310_syntax(self):
        """Les sources se lisent avec la grammaire de Python 3.10 (celui de la VM G4)."""
        seen = 0
        for folder in (HERE, os.path.join(HERE, 'hgp11_ref')):
            for name in sorted(os.listdir(folder)):
                if name.endswith('.py'):
                    with open(os.path.join(folder, name), encoding='ascii') as f:
                        ast.parse(f.read(), filename=name, feature_version=(3, 10))
                    seen += 1
        self.assertGreaterEqual(seen, 10)


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
    folder = tempfile.mkdtemp(prefix='hgp11_ref_')
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
    print('reference_%s_ok nuages=%d ordres=%d coupes=%d noeuds=%d' % (suite, cnt['clouds'], cnt['orders'],
                                                                      cnt['cuts'], cnt['nodes']))
    return OK


# ---------------------------------------------------------------- mutants

def run_mutant(name):
    """Fixtures du mutant jouees sur les sources intactes (temoin), puis sur la copie mutee. Rend le code."""
    mutant = ref_mutants.MUTANTS[name]
    clouds = [FIXTURE[f] for f in mutant['fixtures']]
    for cloud in clouds:
        found = judge.compare_cloud(cloud.points, cloud.kmax)[0]
        if found:
            print('temoin non conforme sur %s : %s' % (cloud.name, found[0]))
            return DISAGREEMENT
    try:
        mutated = ref_mutants.load(name, os.path.join(HERE, 'hgp11_ref'))
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
        print('mutant %s tue par %d fixture(s) sur %d ; premiere : %s' % (name, len(killers), len(clouds),
                                                                         killers[0][:300]))
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
        print('reference_%s tranche=%d/%d nuages=%d ecarts=%d' % (suite, report['shard'], report['shards'],
                                                                 report['counters']['clouds'], len(report['errors'])))
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
