#!/usr/bin/env python3
"""Porte differentielle : les serialisations de la reference egalent, octet pour octet, les dumps du binaire fige
de la v10 (mhgp10_catalogue et mhgp10_tower du 29 septembre 2026, commit c764e121a).

    python3 test_dump_v10.py --frozen-dir=DIR                 DIR contient mhgp10_catalogue et mhgp10_tower
    python3 test_dump_v10.py --frozen-dir=DIR --large         grands nuages (24 a 32 points), etage B seul
    python3 test_dump_v10.py --frozen-dir=DIR --inject=NOM    mutant de serialisation (ref_mutants.DUMP_MUTANTS)
    python3 test_dump_v10.py --list-mutants

Par nuage et par K : le dump du catalogue (doublons compris) ; sans doublon, le dump de la tour en entree core et
en entree cover, serialise depuis l'etage B et, jusqu'a 9 points, depuis l'etage A (la verite, ecrite dans les
conventions de la v10) ; avec doublons, le binaire refuse la tour et la serialisation aussi. Les nombres de fils du
binaire alternent (1, 2, 4) : ses dumps n'en dependent pas.

Codes : 0 conforme ; 1 un dump differe ; 2 refus avant calcul (usage, binaire fige absent) ; 3 plancher viole ;
4 mutant tue. Python 3.10 nu ; les nuages sont ecrits dans un dossier temporaire.
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import ref_mutants  # noqa: E402
import hgp11_ref  # noqa: E402
from hgp11_ref import dumps, families  # noqa: E402

OK, DISAGREEMENT, REFUSAL, FLOOR, MUTANT_KILLED = 0, 1, 2, 3, 4
TOOLS = ('mhgp10_catalogue', 'mhgp10_tower')
THREADS = (1, 2, 4)
DEFINITION_MAX_POINTS = 9  # au-dela, seule la serialisation de l'etage B est comparee

# Grands nuages de l'option --large : famille, nombre de points, K. Hors de portee de l'etage A : c'est l'etage B
# que le binaire fige confirme ici, dans le regime ou la descente saute (au moins k points interieurs a une boule
# minimale).
LARGE_ONLY = (('generic', 24, 10), ('clusters', 28, 5), ('grid4', 26, 5), ('generic_u18', 32, 5), ('generic', 32, 10),
              ('coplanar', 30, 8))
LARGE_EXACT = dict(clouds=6, catalogues=6, ball_lines=6110, extended_balls=533, weighted_balls=0,
                   levels_with_two_writings=10, tower_refusals=0, towers_A=0, towers_B=12, node_lines=16128,
                   point_lines=2460, unreduced_levels=12, cover_ties=220, jumps=1256)

# Compteurs exacts de la campagne (nuages graves, binaire fige) : toute derive est un plancher viole.
EXACT = dict(clouds=190, catalogues=190, ball_lines=4806, extended_balls=499, weighted_balls=316,
             levels_with_two_writings=26, tower_refusals=27, towers_A=314, towers_B=326, node_lines=14284,
             point_lines=7440, unreduced_levels=316, cover_ties=397, jumps=426)


# Grands nuages (14 a 22 points, etage B seul) : famille et K. Les familles cospheriques restent aux petites tailles :
# le quotient local d'une grande coquille etendue coute des secondes en Python.
LARGE = (('generic', 10), ('generic_u18', 12), ('corner_u18', 5), ('grid4', 5), ('coplanar', 5), ('clusters', 10),
         ('duplicates', 12))


def campaign():
    """Nuages de la porte : chaque fixture a K = 1, 2, son ordre grave et min(n, 10) ; six nuages de chaque famille
    de la suite rapide ; un grand nuage de sept familles."""
    out, seen = [], set()

    def add(name, points, kmax):
        if (name, kmax) not in seen:
            seen.add((name, kmax))
            out.append(families.Cloud(name, points, kmax))
    for cloud in families.fixtures():
        for kmax in (1, 2, cloud.kmax, min(len(cloud.points), 10)):
            add(cloud.name, cloud.points, kmax)
    for name, _draw in families.FAMILIES:
        for cloud in families.family(name, 28, 4, 8, 4, 11)[:6]:
            add(cloud.name, cloud.points, cloud.kmax)
    for name, kmax in LARGE:
        cloud = families.family(name, 1, 14, 22, kmax, 31)[0]
        add(cloud.name, cloud.points, cloud.kmax)
    return out


class Frozen(object):
    """Binaire fige : ecrit le nuage, lance l'outil, rend (code, dump ou None)."""

    def __init__(self, folder, tmp):
        self.folder, self.tmp, self.calls = folder, tmp, 0

    def run(self, tool, points, kmax, extra=()):
        src = os.path.join(self.tmp, 'cloud.u32le')
        dump = os.path.join(self.tmp, 'dump.txt')
        dumps.write_u32le(src, points)
        if os.path.exists(dump):
            os.remove(dump)
        threads = THREADS[self.calls % len(THREADS)]
        self.calls += 1
        done = subprocess.run([os.path.join(self.folder, tool), src, '--k=%d' % kmax, '--threads=%d' % threads,
                               '--dump=' + dump] + list(extra), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if done.returncode != 0 or not os.path.exists(dump):
            return done.returncode, None
        with open(dump) as f:
            return 0, f.read()


def first_difference(got, want):
    a, b = got.splitlines(), want.splitlines()
    for i, (x, y) in enumerate(zip(a, b)):
        if x != y:
            return 'ligne %d : fige %r, reference %r' % (i + 1, x, y)
    return '%d lignes contre %d' % (len(a), len(b))


def compare_cloud(package, frozen, cloud, cnt, reduced=False):
    """Compare les dumps d'un nuage. Rend la liste des ecarts. reduced : tours comparees apres reduction des
    niveaux (la v10 et la reference a egalite de l'objet, sans la convention d'ecriture)."""
    errors = []
    ref = package.Reference(cloud.points, cloud.kmax)
    code, got = frozen.run('mhgp10_catalogue', cloud.points, cloud.kmax)
    want = package.dumps.catalogue_dump(ref)
    if code != 0 or got != want:
        errors.append('catalogue : %s' % ('code %d' % code if got is None else first_difference(got, want)))
    cnt['catalogues'] += 1
    cnt['ball_lines'] += want.count('\n')
    engine = package.dumps.EngineCatalogue(ref)
    cnt['extended_balls'] += sum(1 for b in engine.balls if b.extended)
    cnt['weighted_balls'] += sum(1 for b in engine.balls if b.weighted)
    for k in range(1, ref.orders + 1):
        ref.order(k)
    cnt['jumps'] += ref.stats['jumps']
    written = {}
    for b in engine.balls:
        written.setdefault(b.level, set()).add(package.dumps.emitted_level(ref, b))
    cnt['levels_with_two_writings'] += sum(1 for forms in written.values() if len(forms) > 1)
    if len(ref.sites) != ref.n:  # doublons : la tour de la v10 refuse, la serialisation aussi
        code, got = frozen.run('mhgp10_tower', cloud.points, cloud.kmax)
        refused = False
        try:
            package.dumps.tower_dump(ref)
        except ValueError:
            refused = True
        if code != 2 or got is not None or not refused:
            errors.append('tour d\'un nuage a doublons : code %d du binaire, refus de la reference %s'
                          % (code, refused))
        cnt['tower_refusals'] += 1
        return errors
    towers = [('B', ref)]
    if ref.n <= DEFINITION_MAX_POINTS:
        towers.append(('A', package.Definition(cloud.points)))
    for entry in ('core', 'cover'):
        code, got = frozen.run('mhgp10_tower', cloud.points, cloud.kmax, ['--entry=' + entry])
        for stage, tower in towers:
            want = package.dumps.tower_dump(ref, tower, entry=entry)
            if got is not None and reduced:
                got_cmp, want_cmp = package.dumps.reduce_tower_levels(got), package.dumps.reduce_tower_levels(want)
            else:
                got_cmp, want_cmp = got, want
            if code != 0 or got_cmp != want_cmp:
                what = 'code %d' % code if got is None else first_difference(got_cmp, want_cmp)
                errors.append('tour %s, etage %s : %s' % (entry, stage, what))
            cnt['towers_' + stage] += 1
            if stage == 'B':
                cnt['node_lines'] += want.count('\nnode ')
                cnt['point_lines'] += want.count('\npoint ')
                cnt['unreduced_levels'] += package.dumps.reduce_tower_levels(want) != want
    for k in range(2, ref.orders + 1):
        cnt['cover_ties'] += sum(1 for e in ref.order(k).cover if len(e.nodes) >= 2)
    return errors


def new_counters():
    return dict((name, 0) for name in ('clouds', 'catalogues', 'ball_lines', 'extended_balls', 'weighted_balls',
                                       'levels_with_two_writings', 'tower_refusals', 'towers_A', 'towers_B',
                                       'node_lines', 'point_lines', 'unreduced_levels', 'cover_ties', 'jumps'))


def large_campaign():
    return [families.family(name, 1, n, n, kmax, 41)[0] for name, n, kmax in LARGE_ONLY]


def run_campaign(frozen, large=False):
    cnt = new_counters()
    failures = 0
    for cloud in (large_campaign() if large else campaign()):
        errors = compare_cloud(hgp11_ref, frozen, cloud, cnt)
        cnt['clouds'] += 1
        if errors:
            failures += 1
            if failures <= 10:
                print('ECART %s K=%d %r : %s' % (cloud.name, cloud.kmax, cloud.points, errors[0]))
    print('reference_diff_v10 ecarts=%d %s' % (failures, ' '.join('%s=%d' % (k, cnt[k]) for k in sorted(cnt))))
    if failures:
        return DISAGREEMENT
    exact = LARGE_EXACT if large else EXACT
    low = ['%s %d != %d' % (k, cnt[k], want) for k, want in sorted(exact.items()) if cnt[k] != want]
    if low or frozen.calls < 3 * len(THREADS):
        print('PLANCHER : %s' % ', '.join(low + ['appels du binaire %d' % frozen.calls]))
        return FLOOR
    print('reference_diff_v10%s_ok nuages=%d catalogues=%d tours=%d lignes=%d' % (
        '_large' if large else '', cnt['clouds'], cnt['catalogues'], cnt['towers_A'] + cnt['towers_B'],
        cnt['ball_lines'] + cnt['node_lines'] + cnt['point_lines']))
    return OK


def run_mutant(name, frozen):
    """Fixtures du mutant : dumps intacts conformes (temoin), puis dumps de la copie mutee differents du binaire."""
    mutant = ref_mutants.DUMP_MUTANTS[name]
    clouds = ref_mutants.clouds_of(mutant, families.fast_suite())
    for cloud in clouds:
        errors = compare_cloud(hgp11_ref, frozen, cloud, new_counters())
        if errors:
            print('temoin non conforme sur %s : %s' % (cloud.name, errors[0]))
            return DISAGREEMENT
    try:
        mutated = ref_mutants.load(name, os.path.join(HERE, 'hgp11_ref'))
    except ref_mutants.StalePatch as exc:
        print('PLANCHER : mutant %s inapplicable : %s' % (name, exc))
        return FLOOR
    killers, reduced_killers = [], []
    for cloud in clouds:
        for reduced, found in ((False, killers), (True, reduced_killers)):
            errors = compare_cloud(mutated, frozen, cloud, new_counters(), reduced)
            if errors:
                found.append('%s : %s' % (cloud.name, errors[0]))
    if not killers:
        print('mutant_survives %s' % name)
        return OK
    print('mutant %s tue par %d fixture(s) sur %d ; premiere : %s'
          % (name, len(killers), len(clouds), killers[0][:300]))
    if mutant['writing_only'] != (not reduced_killers):
        print('PLANCHER : mutant %s, ecriture seule attendue %s, ecarts apres reduction des niveaux : %d' % (
            name, mutant['writing_only'], len(reduced_killers)))
        return FLOOR
    print('mutant_killed %s%s' % (name, ' ecriture_seule' if mutant['writing_only'] else ''))
    return MUTANT_KILLED


def main(argv):
    folder = inject = None
    large = False
    for arg in argv:
        if arg.startswith('--frozen-dir=') and len(arg) > 13:
            folder = arg[13:]
        elif arg == '--large':
            large = True
        elif arg.startswith('--inject=') and arg[9:] in ref_mutants.DUMP_MUTANTS:
            inject = arg[9:]
        elif arg == '--list-mutants' and len(argv) == 1:
            for name in sorted(ref_mutants.DUMP_MUTANTS):
                print('%s %s' % (name, 'ecriture' if ref_mutants.DUMP_MUTANTS[name]['writing_only'] else 'objet'))
            return OK
        else:
            print('refus : argument %r' % arg)
            print(__doc__)
            return REFUSAL
    if folder is None or any(not os.access(os.path.join(folder, tool), os.X_OK) for tool in TOOLS):
        print('refus : binaire fige de la v10 absent (--frozen-dir=%s doit contenir %s)' % (folder, ' et '.join(TOOLS)))
        return REFUSAL
    tmp = tempfile.mkdtemp(prefix='hgp11_diff_v10_')
    try:
        frozen = Frozen(folder, tmp)
        if inject and large:
            print('refus : --inject et --large s\'excluent')
            return REFUSAL
        return run_mutant(inject, frozen) if inject else run_campaign(frozen, large)
    finally:
        for name in os.listdir(tmp):
            os.remove(os.path.join(tmp, name))
        os.rmdir(tmp)


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
