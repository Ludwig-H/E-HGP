#!/usr/bin/env python3
"""Porte du lecteur de transition du catalogue (transition_catalogue.py, docs/CONTRAT_CATALOGUE.md, paragraphes 6.1
et 8 bis) : temoins et cas de fixtures/transition_catalogue.json, vidages ecrits par transition_temoins.py depuis le
catalogue de l'oracle borne (hgp12_ref, etage B), mutants du lecteur.

Usage :
  test_transition_catalogue.py               suite : attendus graves de chaque temoin contre l'oracle (nombre de
                                             boules et de niveaux, ordre de Morton des sites, ordre publie dans
                                             chaque convention, S* qui changent, centres, niveaux, q et populations
                                             des boules nommees), puis chaque cas juge dans le processus (code,
                                             categories exactes des desaccords, ligne de conformite, motif du
                                             refus), et trois cas rejoues par la ligne de commande du lecteur
                                             (codes 0, 1 et 2) ; ligne transition_temoins_ok ...
  test_transition_catalogue.py --cas=NOM     un cas par la ligne de commande du lecteur : rend le code du lecteur
                                             (0, 1 ou 2) si tout l'attendu du cas tient, 3 sinon
  test_transition_catalogue.py --inject=NOM  mutant du lecteur (copie modifiee dans un dossier temporaire, aucun
                                             crochet dans les sources) : 4 et mutant_killed NOM s'il est tue par
                                             l'un de ses cas avec le code declare (tue par sa cause) ; un mutant
                                             declare equivalent doit laisser le code de TOUS les cas inchange (0 et
                                             mutant_survives NOM) ; 3 sinon
  test_transition_catalogue.py --list-cases | --list-mutants
Codes : 0 conforme ; 2 usage ; 3 attendu grave viole ou mutant non tue ; 4 mutant tue. Python 3.10 nu, aucun assert :
memes codes et memes lignes sous python3 -O (la jumelle -O des portes, et le lecteur lance qui en herite).
"""
import importlib.util
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)
import transition_temoins as temoins  # noqa: E402

READER = os.path.join(HERE, 'transition_catalogue.py')
OK, USAGE, VIOLATION, KILLED = 0, 2, 3, 4
CLI_CASES = ('transl', 'boule_manquante', 'refus_magie')


class Violation(Exception):
    pass


def _m(edits, cases, code, why, equivalent=False):
    return dict(edits=edits, cases=cases, code=code, why=why, equivalent=equivalent)


S8, S12, S16, S20 = ' ' * 8, ' ' * 12, ' ' * 16, ' ' * 20
# Mutants : (texte present UNE fois dans transition_catalogue.py, remplacement), cas qui doivent le tuer, code que
# rend le lecteur mute sur l'un d'eux (tue par sa cause). Un mutant equivalent doit survivre a tous les cas.
MUTANTS = {
    'sans_absence': _m(
        [(S16 + "ecarts.add('absente', 'reference:%d' % r[0], 'boule de reference sans image dans le candidat')",
          S16 + 'pass'),
         (S20 + "ecarts.add('absente', 'reference:%d' % ball[0], 'niveau absent du candidat')", S20 + 'pass')],
        ['boule_manquante', 'niveau_manquant'], 0, 'bijection : boule ou niveau de reference sans image ignores'),
    'sans_en_trop': _m(
        [(S16 + "ecarts.add('en_trop', 'candidat:%d' % c[0], 'boule candidate sans antecedent dans la reference')",
          S16 + 'pass'),
         (S20 + "ecarts.add('en_trop', 'candidat:%d' % ball[0], 'niveau absent de la reference')", S20 + 'pass')],
        ['boule_de_trop'], 0, 'bijection : boule candidate sans antecedent ignoree'),
    'sans_doublon': _m(
        [(S16 + "ecarts.add('doublon', 'reference:%d' % r[0], 'meme boule que reference:%d' % seen[r[1]])",
          S16 + 'pass'),
         (S16 + "ecarts.add('doublon', 'candidat:%d' % extra[0], 'meme boule que candidat:%d' % matches[0][0])",
          S16 + 'pass')],
        ['boule_en_double'], 0, 'bijection : cle repetee ignoree'),
    'sans_interieur_croise': _m(
        [(S8 + 'if r[5] != self._population(c[5]):', S8 + 'if False:')],
        ['interieur_omis'], 0, 'I non compares entre les vidages'),
    'sans_coquille_croisee': _m(
        [(S8 + 'if r[6] != self._population(c[6]):', S8 + 'if False:')],
        ['coquille_omise'], 0, 'U non comparees entre les vidages'),
    'sans_rangs_croises': _m(
        [(S8 + 'if r[2] != c[2]:', S8 + 'if False:')],
        [], None, 'garde defensive : bijection et rangs recalcules l\'impliquent', equivalent=True),
    'sans_cardinal_support': _m(
        [(S12 + 'if r[3] != c[3]:', S12 + 'if False:')],
        [], None, 'garde defensive : qmin et U egales l\'impliquent', equivalent=True),
    'sans_sites': _m(
        [(S8 + 'if left != right:', S8 + 'if False:')],
        ['site_en_trop'], 0, 'ensembles de positions des sites non compares'),
    'sans_support_minimal': _m(
        [(S12 + 'if not minimal:', S12 + 'if False:'),
         (S16 + "ecarts.add('support_non_minimal', where, 'S* %s affinement dependant' % list(sstar))", S16 + 'pass')],
        ['sstar_non_minimal_partout'], 0, 'S* non minimal (ou degenere) accepte'),
    'sans_interieur_strict': _m(
        [(S20 + "ecarts.add('interieur_non_strict', where, 'site %d de I hors de la boule ouverte' % i)",
          S20 + 'pass')],
        ['interieur_non_strict_partout'], 0, 'sites de I non juges strictement interieurs'),
    'sans_coquille_sphere': _m(
        [(S20 + "ecarts.add('coquille_hors_sphere', where, 'site %d de U hors de la sphere' % i)", S20 + 'pass')],
        ['coquille_hors_sphere_partout'], 0, 'sites de U non juges sur la sphere'),
    'sans_support_dans_coquille': _m(
        [(S16 + "ecarts.add('support_hors_coquille', where, 'S* %s non inclus dans U' % list(sstar))", S16 + 'pass')],
        ['support_hors_coquille_partout'], 0, 'S* hors de U accepte'),
    'sans_admission': _m(
        [(S12 + 'if p + q > kmax + 1:', S12 + 'if False:')],
        ['admission_partout'], 0, 'boule hors de Cat_K (p + q > K + 1) acceptee'),
    'sans_rangs': _m(
        [(S12 + 'if rank != dense:', S12 + 'if False:')],
        ['rangs_faux_partout'], 0, 'rang publie non compare au rang dense recalcule'),
    'sans_niveaux_croissants': _m(
        [(S16 + 'if level is not None and compare_levels(ball_level, level) < 0:', S16 + 'if False:')],
        ['niveaux_decroissants_partout'], 0, 'niveaux decroissants dans l\'ordre publie acceptes'),
    'sans_niveaux': _m(
        [(S8 + 'if cat.nlevels != dense + 1:', S8 + 'if False:')],
        ['niveaux_faux_partout'], 0, 'NLEVELS non controle'),
    'sans_ordre': _m(
        [(S12 + 'if previous_key is not None and order_key < previous_key:', S12 + 'if False:')],
        ['ordre_v11_sous_v12'], 0, 'ordre publie a niveau egal non recalcule'),
    'sans_qmin': _m(
        [(S12 + 'if first_support(sphere, shell, size, positions) is not None:', S12 + 'if False:')],
        ['qmin_non_minimal_partout'], 0, 'support de cardinal < q dans U ignore'),
    'sans_convention': _m(
        [(S8 + 'if set(first) != set(sstar):', S8 + 'if False:')],
        ['sstar_non_canonique', 'sstar_morton_sous_v12', 'sstar_morton_sous_v12_q3', 'sstar_morton_sous_v12_q4',
         'sstar_v12_sous_v11'], 0, 'S* non premier de sa convention accepte'),
    'convention_v12_par_indices': _m(
        [(S8 + "first = by_index if convention == 'v11' else by_position", S8 + 'first = by_index'),
         (S12 + "order_key = sstar + (NONE,) * (4 - q) if convention == 'v11' else spos + (PAD_V12,) * (4 - q)",
          S12 + 'order_key = sstar + (NONE,) * (4 - q)')],
        ['carre_tourne', 'cercle_q3', 'sphere_q4'], 1, 'convention v12 departagee par les SiteIdx (rang de Morton)'),
    'sites_par_indice': _m(
        [(S8 + 'self.same_sites = reference.positions == candidate.positions', S8 + 'self.same_sites = True')],
        ['carre_tourne_sites_inverses'], 1, 'sites identifies par leur indice, non par leur position'),
    'boules_par_support': _m(
        [(S12 + 'group.append((index, key, rank, q, spos, inner, shell))',
          S12 + 'group.append((index, spos, rank, q, spos, inner, shell))')],
        ['carre_tourne', 'cercle_q3', 'sphere_q4'], 1, 'boules identifiees par S*, non par centre et rayon carre'),
    'sans_positions_distinctes': _m(
        [(S8 + 'if len(set(self.positions)) != count:', S8 + 'if False:')],
        ['refus_positions_en_double'], 1, 'positions en double admises : l\'identite par position est perdue'),
    'sans_domaine_des_indices': _m(
        [(S12 + 'if sstar[-1] >= n or any(sstar[i] >= sstar[i + 1] for i in range(q - 1)):',
          S12 + 'if any(sstar[i] >= sstar[i + 1] for i in range(q - 1)):'),
         (S16 + 'if part and (part[-1] >= n or any(part[i] >= part[i + 1] for i in range(len(part) - 1))):',
          S16 + 'if part and any(part[i] >= part[i + 1] for i in range(len(part) - 1)):')],
        ['refus_index_hors_domaine'], 3, 'indices de sites hors domaine admis : le lecteur sort par une exception'),
    'sans_decalages': _m(
        [(S12 + 'if offset < previous or offset - previous != p + m:', S12 + 'if False:')],
        ['refus_decalage'], 1, 'decalages non compares a p + m : population tronquee jugee comme une autre'),
}


def load_reader(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def mutated_reader(name, directory):
    """Copie du lecteur modifiee par le mutant name, chargee sous un nom propre."""
    with open(READER, encoding='utf-8') as handle:
        text = handle.read()
    for old, new in MUTANTS[name]['edits']:
        if text.count(old) != 1:
            raise Violation('mutant %s perime : motif present %d fois : %r' % (name, text.count(old), old.strip()))
        text = text.replace(old, new)
    path = os.path.join(directory, 'transition_catalogue_mutant.py')
    with open(path, 'w', encoding='utf-8') as out:
        out.write(text)
    return load_reader(path, 'transition_catalogue_mutant_' + name)


def judge_in_process(reader, paths, case):
    """(code, rapport, message) du lecteur charge, comme sa ligne de commande les rendrait."""
    try:
        code, report = reader.judge_files(paths[0], paths[1], case.get('convention_reference', 'v11'),
                                          case.get('convention_candidat', 'v12'))
        return code, report, ''
    except reader.Refus as refusal:
        return 2, None, str(refusal)
    except reader.Invariant as violation:
        return 3, None, str(violation)
    except Exception as error:  # le lecteur rend 3 sur toute exception inattendue
        return 3, None, '%s : %s' % (type(error).__name__, error)


def expectation(case, code, first_line, categories, message):
    """Ecart a l'attendu du cas, ou None."""
    if code != case['code']:
        return 'code %d, attendu %d (%s)' % (code, case['code'], message or first_line)
    if code == 0 and first_line != case['line']:
        return 'ligne %r, attendue %r' % (first_line, case['line'])
    if code == 1 and categories != case['categories']:
        return 'categories %r, attendues %r' % (categories, case['categories'])
    if code == 2 and case['refus'] not in message:
        return 'refus %r sans le motif %r' % (message, case['refus'])
    return None


def categories_of(line):
    if not line.startswith('transition_catalogue_desaccord '):
        return {}
    field = line.split(' categories=', 1)[1] if ' categories=' in line else ''
    return {name: int(count) for name, count in (item.split(':') for item in field.split(',') if item)}


def run_cli(case, paths):
    command = [sys.executable, READER, paths[0], paths[1],
               '--convention-reference', case.get('convention_reference', 'v11'),
               '--convention-candidat', case.get('convention_candidat', 'v12')]
    done = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True,
                          check=False)
    lines = done.stdout.splitlines()
    return done.returncode, (lines[0] if lines else ''), done.stdout, done.stderr


def check_witnesses(fixture):
    """Attendus graves de chaque temoin contre l'oracle (generateur). Rend le nombre de boules des temoins."""
    total = 0
    for name, witness in sorted(fixture['witnesses'].items()):
        got, want = temoins.describe(witness), witness['expected']
        facts = [('boules', got['boules'], want['boules']), ('niveaux', got['niveaux'], want['niveaux']),
                 ('sites_morton', ''.join(got['sites_morton']), want['sites_morton']),
                 ('ordre_v11', got['ordre']['v11'], want['ordre_v11']),
                 ('ordre de l\'oracle', got['ordre_oracle'], want['ordre_v11']),
                 ('ordre_v12', got['ordre']['v12'], want['ordre_v12']),
                 ('sstar_v12_differents', {b: s for b, s in got['sstar']['v12'].items() if s != b},
                  want['sstar_v12_differents']),
                 ('v11 = nom', all(s == b for b, s in got['sstar']['v11'].items()), True)]
        for ball, detail in want['detail'].items():
            facts.append(('detail ' + ball, got['boules_detail'].get(ball), detail))
        for label, value, expected in facts:
            if value != expected:
                raise Violation('temoin %s, %s : %r, attendu %r' % (name, label, value, expected))
        total += got['boules']
    return total


def run_suite(fixture):
    balls = check_witnesses(fixture)
    names = [case['name'] for case in fixture['cases']]
    if len(set(names)) != len(names) or not set(CLI_CASES) <= set(names):
        raise Violation('cas en double, ou cas rejoues par la ligne de commande absents : %r' % (CLI_CASES,))
    reader = load_reader(READER, 'transition_catalogue_porte')
    counts = {0: 0, 1: 0, 2: 0}
    changed = 0
    work = tempfile.mkdtemp(prefix='mhgp12_transition_')
    try:
        for case in fixture['cases']:
            directory = os.path.join(work, case['name'])
            os.makedirs(directory)
            paths = temoins.case_files(fixture, case, directory)
            code, report, message = judge_in_process(reader, paths, case)
            first = reader.summary(code, report)[0] if report is not None else ''
            wrong = expectation(case, code, first, report['ecarts'] if report else {}, message)
            if wrong is not None:
                raise Violation('cas %s : %s' % (case['name'], wrong))
            counts[code] += 1
            if code == 0:
                changed += report['croise']['sstar_differents']
            if case['name'] in CLI_CASES:
                cli_code, cli_first, _out, err = run_cli(case, paths)
                wrong = expectation(case, cli_code, cli_first, categories_of(cli_first), err)
                if wrong is not None:
                    raise Violation('cas %s par la ligne de commande : %s' % (case['name'], wrong))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    print('transition_temoins_ok temoins=%d cas=%d conformes=%d desaccords=%d refus=%d boules=%d sstar_changes=%d'
          % (len(fixture['witnesses']), len(fixture['cases']), counts[0], counts[1], counts[2], balls, changed))
    return OK


def run_case(fixture, name):
    case = next((c for c in fixture['cases'] if c['name'] == name), None)
    if case is None:
        print('cas inconnu : %s' % name, file=sys.stderr)
        return USAGE
    work = tempfile.mkdtemp(prefix='mhgp12_transition_')
    try:
        paths = temoins.case_files(fixture, case, work)
        code, first, out, err = run_cli(case, paths)
    finally:
        shutil.rmtree(work, ignore_errors=True)
    sys.stdout.write(out)
    sys.stderr.write(err)
    wrong = expectation(case, code, first, categories_of(first), err)
    if wrong is not None:
        print('transition_cas_viole %s : %s' % (name, wrong))
        return VIOLATION
    return code


def run_mutant(fixture, name):
    """Un mutant reel est tue par CHACUN de ses cas, avec le code declare ; un mutant equivalent laisse le code de
    tous les cas inchange."""
    mutant = MUTANTS[name]
    work = tempfile.mkdtemp(prefix='mhgp12_transition_mutant_')
    try:
        reader = mutated_reader(name, work)
        cases = [c for c in fixture['cases'] if mutant['equivalent'] or c['name'] in mutant['cases']]
        if not mutant['equivalent'] and len(cases) != len(mutant['cases']):
            raise Violation('mutant %s : cas inconnus dans %r' % (name, mutant['cases']))
        survivors = []
        for case in cases:
            directory = os.path.join(work, case['name'])
            os.makedirs(directory)
            paths = temoins.case_files(fixture, case, directory)
            code, _report, _message = judge_in_process(reader, paths, case)
            if mutant['equivalent'] and code != case['code']:
                raise Violation('mutant %s declare equivalent, mais le cas %s rend %d au lieu de %d'
                                % (name, case['name'], code, case['code']))
            if not mutant['equivalent'] and not (code == mutant['code'] and code != case['code']):
                survivors.append('%s (code %d)' % (case['name'], code))
    finally:
        shutil.rmtree(work, ignore_errors=True)
    if mutant['equivalent']:
        print('mutant_survives %s' % name)
        print('garde equivalente : %s ; codes inchanges sur les %d cas' % (mutant['why'], len(cases)))
        return OK
    if survivors:
        print('mutant_survives %s' % name)
        print('non tue par : %s' % ', '.join(survivors))
        return VIOLATION
    print('mutant_killed %s' % name)
    print('tue par %s, code %d chacun (%s)' % (', '.join(mutant['cases']), mutant['code'], mutant['why']))
    return KILLED


def main(argv):
    try:
        fixture = temoins.load_fixture()
    except (OSError, ValueError) as error:
        print('transition_refus : %s' % error, file=sys.stderr)
        return USAGE
    args = argv[1:]
    try:
        if not args:
            return run_suite(fixture)
        if args == ['--list-cases']:
            for case in fixture['cases']:
                print('%s %d' % (case['name'], case['code']))
            return OK
        if args == ['--list-mutants']:
            for name in sorted(MUTANTS):
                print('%s %s' % (name, 'equivalent' if MUTANTS[name]['equivalent'] else 'reel'))
            return OK
        if len(args) == 1 and args[0].startswith('--cas='):
            return run_case(fixture, args[0].split('=', 1)[1])
        if len(args) == 1 and args[0].startswith('--inject=') and args[0].split('=', 1)[1] in MUTANTS:
            return run_mutant(fixture, args[0].split('=', 1)[1])
    except Violation as violation:
        print('transition_viole : %s' % violation)
        return VIOLATION
    print('usage : test_transition_catalogue.py [--cas=NOM | --inject=MUTANT | --list-cases | --list-mutants]',
          file=sys.stderr)
    return USAGE


if __name__ == '__main__':
    sys.exit(main(sys.argv))
