#!/usr/bin/env python3
"""Lanceur de mutants de la v11 : chaque mutant est un correctif applique a une COPIE des sources.

Aucun mutant, crochet de test ni option d'injection ne vit dans le produit (docs/ARCHITECTURE.md, paragraphe 1,
regle 6). Un manifeste par module, tests/mutants/<module>.json :

    {
      "module": "core",
      "plancher": 6,                      nombre minimal de mutants juges : en dessous, la campagne est vide (code 3)
      "construction": ["core"],           optionnel : valeur de MHGP11_MODULES de la copie (defaut : le module)
      "mutants": [
        {
          "id": "priorite_plus_petit_k",                  unique dans le manifeste, [a-z0-9_]
          "fichier": "src/core/status.hpp",               relatif a la racine de la v11
          "cherche": "return order < o.order;",           texte exact, present UNE SEULE fois dans le fichier
          "remplace": "return order > o.order;",
          "porte": "mhgp11_core_unit_outcome",            porte CTest qui doit echouer sur la copie mutee
          "note": "le plus grand K d'abord",              optionnel
          "options": ["-DMHGP11_POISON=ON"],              optionnel : definitions CMake du mutant et de son temoin
          "aussi": [{"fichier": "...", "cherche": "...", "remplace": "..."}]
                                                          optionnel : autres remplacements du meme mutant, appliques
                                                          dans l'ordre, chaque motif present une seule fois a son tour
        },
        {
          "id": "...", "fichier": "...", "cherche": "...", "remplace": "...",
          "attendu": "construction",                      a la place de "porte" : la copie mutee ne doit plus se
          "jeton": "mhgp11_coord_bits_invalide"           construire, et la sortie de la construction porte ce jeton
                                                          (static_assert, #error)
        }
      ]
    }

Deroulement : (1) le manifeste est lu et juge contre l'arbre (schema, fichier present, motif present une seule fois) ;
(2) par jeu d'options, un temoin sans mutation est copie, configure, construit, et chaque porte citee doit y
passer ; (3) chaque mutant est copie, mute, configure avec -DMHGP11_MODULES (seuls le module, ses dependances et ses
portes sont construits), construit, puis sa porte est jouee et doit echouer. Les sources d'origine ne sont jamais
modifiees.

    python3 run_mutants.py --manifest <fichier.json> --source <racine v11> [--work <dossier>] [--jobs N]
            [--build-jobs M] [--only id,...] [--floor N] [--cmake <cmake>] [--ctest <ctest>] [--generator <nom>]
            [--cmake-arg=<definition>]... [--keep] [--check] [--list]

    --check   juge le manifeste contre l'arbre et s'arrete (aucune construction) ; ligne manifeste_ok
    --list    ecrit les identifiants et s'arrete
    --only    ne juge que ces mutants ; le plancher s'applique toujours, le baisser explicitement par --floor
    --jobs    mutants juges en parallele ; 0 (defaut) : la moitie des coeurs ; --build-jobs : coeurs par construction
    --keep    garde les copies (sinon le dossier de travail est efface a la fin)

Codes : 0 conforme (temoin vert, chaque mutant tue, plancher atteint) ; 1 desaccord (un mutant survit, ou le temoin
est rouge) ; 2 refus avant calcul (usage, manifeste illisible ou hors schema, dossier de travail etranger) ;
3 plancher ou invariant (moins de mutants que le plancher, motif absent ou multiple, mutant qui ne se construit pas
alors qu'une porte devait le tuer, porte absente). Ligne finale si conforme :
    mutants_ok module=<m> mutants=N tues=N dont_signal=A dont_delai=B dont_construction=C plancher=P
Les mutants tues par signal ou par delai sont comptes a part : ils sont tues, mais par un chemin moins precis.
Python 3.10 nu, aucun assert.
"""
import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys

COPIED = ('CMakeLists.txt', 'cmake', 'src', 'cli', 'tests', 'tools', 'reference', 'docs')
MARKER = '.mhgp11_mutants'
CONFIGURE_TIMEOUT = 900
BUILD_TIMEOUT = 3600
GATE_TIMEOUT = 7500
TOP_KEYS = {'module': str, 'plancher': int, 'construction': list, 'mutants': list}
MUTANT_KEYS = {'id': str, 'fichier': str, 'cherche': str, 'remplace': str, 'porte': str, 'note': str,
               'options': list, 'attendu': str, 'jeton': str, 'aussi': list}
EDIT_KEYS = ('fichier', 'cherche', 'remplace')
REQUIRED = ('id', 'fichier', 'cherche', 'remplace')


class Refusal(Exception):
    """Refus avec son code de sortie."""

    def __init__(self, code, message):
        Exception.__init__(self, message)
        self.code = code


def no_duplicate_keys(pairs):
    seen = {}
    for key, value in pairs:
        if key in seen:
            raise ValueError('cle JSON en double : %s' % key)
        seen[key] = value
    return seen


def load_manifest(path):
    """Manifeste juge contre son schema ; tout ecart est un refus (code 2)."""
    try:
        with open(path, encoding='utf-8') as handle:
            manifest = json.load(handle, object_pairs_hook=no_duplicate_keys)
    except (OSError, ValueError) as error:
        raise Refusal(2, 'manifeste illisible : %s' % error)
    if not isinstance(manifest, dict):
        raise Refusal(2, 'manifeste : un objet JSON est attendu')
    for key, value in manifest.items():
        if key not in TOP_KEYS or not isinstance(value, TOP_KEYS[key]) or isinstance(value, bool):
            raise Refusal(2, 'manifeste : cle inconnue ou de mauvais type : %s' % key)
    for key in ('module', 'plancher', 'mutants'):
        if key not in manifest:
            raise Refusal(2, 'manifeste : cle absente : %s' % key)
    if not re.fullmatch(r'[a-z0-9_]+', manifest['module']) or manifest['plancher'] < 1:
        raise Refusal(2, 'manifeste : module ou plancher invalide')
    construction = manifest.setdefault('construction', [manifest['module']])
    if not construction or not all(isinstance(unit, str) and re.fullmatch(r'[a-z0-9_]+', unit)
                                   for unit in construction):
        raise Refusal(2, 'manifeste : construction invalide')
    seen = set()
    for mutant in manifest['mutants']:
        if not isinstance(mutant, dict):
            raise Refusal(2, 'manifeste : un mutant est un objet JSON')
        for key, value in mutant.items():
            if key not in MUTANT_KEYS or not isinstance(value, MUTANT_KEYS[key]):
                raise Refusal(2, 'mutant %s : cle inconnue ou de mauvais type : %s' % (mutant.get('id'), key))
        for key in REQUIRED:
            if key not in mutant or (not mutant[key] and key != 'remplace'):
                raise Refusal(2, 'mutant %s : cle absente ou vide : %s' % (mutant.get('id'), key))
        name = mutant['id']
        if not re.fullmatch(r'[a-z0-9_]+', name) or name in seen:
            raise Refusal(2, 'mutant %s : identifiant invalide ou en double' % name)
        seen.add(name)
        for extra in mutant.setdefault('aussi', []):
            if not isinstance(extra, dict) or sorted(extra) != sorted(EDIT_KEYS) or not all(
                    isinstance(extra[key], str) for key in EDIT_KEYS) or not extra['fichier'] or not extra['cherche']:
                raise Refusal(2, 'mutant %s : aussi = liste d\'objets {fichier, cherche, remplace}' % name)
        for path, find, replace in edits_of(mutant):
            if find == replace:
                raise Refusal(2, 'mutant %s : cherche et remplace sont identiques' % name)
            parts = path.split('/')
            if os.path.isabs(path) or '..' in parts or parts[0] not in COPIED:
                raise Refusal(2, 'mutant %s : fichier hors des dossiers copies (%s)' % (name, ' '.join(COPIED)))
        options = mutant.setdefault('options', [])
        if not all(isinstance(option, str) and option.startswith('-D') for option in options):
            raise Refusal(2, 'mutant %s : options = liste de definitions -D...' % name)
        expected = mutant.setdefault('attendu', 'porte')
        if expected == 'porte':
            if not re.fullmatch(r'mhgp11_[a-z0-9_]+', mutant.get('porte', '')) or 'jeton' in mutant:
                raise Refusal(2, 'mutant %s : une porte mhgp11_... est attendue, sans jeton' % name)
        elif expected != 'construction' or not mutant.get('jeton') or 'porte' in mutant:
            raise Refusal(2, 'mutant %s : attendu = porte, ou construction avec un jeton et sans porte' % name)
    return manifest


def edits_of(mutant):
    """Remplacements d'un mutant, dans l'ordre : (fichier, cherche, remplace)."""
    first = [(mutant['fichier'], mutant['cherche'], mutant['remplace'])]
    return first + [(extra['fichier'], extra['cherche'], extra['remplace']) for extra in mutant.get('aussi', [])]


def read_text(path):
    with open(path, encoding='utf-8', newline='') as handle:
        return handle.read()


def mutated_files(root, mutant):
    """Texte mute de chaque fichier touche par le mutant. Chaque motif doit etre present une seule fois au moment ou
    il est applique ; sinon invariant viole (code 3)."""
    texts = {}
    for path, find, replace in edits_of(mutant):
        if path not in texts:
            try:
                texts[path] = read_text(os.path.join(root, path))
            except (OSError, ValueError) as error:
                raise Refusal(3, 'mutant %s : fichier illisible %s : %s' % (mutant['id'], path, error))
        count = texts[path].count(find)
        if count != 1:
            raise Refusal(3, 'mutant %s : motif present %d fois dans %s, une seule attendue'
                          % (mutant['id'], count, path))
        texts[path] = texts[path].replace(find, replace)
    return texts


def check_patterns(source, mutants):
    """Chaque mutant s'applique a l'arbre d'origine (rien n'est ecrit)."""
    for mutant in mutants:
        mutated_files(source, mutant)


def copy_tree(source, target):
    os.makedirs(target)
    ignore = shutil.ignore_patterns('__pycache__', '*.pyc')
    for name in COPIED:
        origin = os.path.join(source, name)
        if os.path.isdir(origin):
            shutil.copytree(origin, os.path.join(target, name), ignore=ignore)
        elif os.path.isfile(origin):
            shutil.copy2(origin, os.path.join(target, name))


def call(argv, timeout):
    """Lance une commande ; rend (code ou None si delai, sortie complete)."""
    env = dict(os.environ)
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    try:
        done = subprocess.run(argv, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace',
                              timeout=timeout, env=env)
    except subprocess.TimeoutExpired as expired:
        output = expired.stdout or ''
        return None, output if isinstance(output, str) else output.decode('utf-8', 'replace')
    except OSError as error:
        return 127, 'lancement impossible : %s' % error
    return done.returncode, done.stdout


def tail(text, lines=25):
    return '\n'.join('    | ' + line for line in text.strip().splitlines()[-lines:])


class Builder:
    """Configure, construit et joue les portes d'une copie."""

    def __init__(self, args, construction):
        self.args = args
        self.construction = construction

    def configure_and_build(self, folder, options):
        """Rend (etape en echec ou None, sortie)."""
        configure = [self.args.cmake, '-S', os.path.join(folder, 'src'), '-B', os.path.join(folder, 'build')]
        if self.args.generator:
            configure += ['-G', self.args.generator]
        configure += ['-DMHGP11_MODULES=' + ';'.join(self.construction)] + self.args.cmake_arg + list(options)
        code, output = call(configure, CONFIGURE_TIMEOUT)
        if code != 0:
            return 'configuration', output
        code, more = call([self.args.cmake, '--build', os.path.join(folder, 'build'), '--parallel',
                           str(self.args.build_jobs)], BUILD_TIMEOUT)
        if code != 0:
            return 'construction', output + more
        return None, output + more

    def gate(self, folder, name):
        """Joue une porte ; rend (verdict, sortie) : verdict parmi passe, echec, absente."""
        code, output = call([self.args.ctest, '--test-dir', os.path.join(folder, 'build'), '-R', '^%s$' % name,
                             '--output-on-failure', '--no-tests=error'], GATE_TIMEOUT)
        if 'No tests were found' in output:
            return 'absente', output
        return ('passe' if code == 0 else 'echec'), output


def kill_cause(output):
    """Chemin par lequel une porte en echec a tue le mutant (lu dans la sortie de run_expect.cmake et de CTest)."""
    if '***Timeout' in output:
        return 'delai'
    if 'run_expect_verdict arret_anormal' in output or '***Exception' in output:
        return 'signal'
    if 'run_expect_verdict ligne_absente' in output:
        return 'ligne'
    if 'run_expect_verdict code' in output:
        return 'code'
    return 'autre'


def judge(builder, source, work, mutant, keep):
    """Juge un mutant ; rend (verdict, detail, sortie) : verdict parmi TUE, SURVIT, INVALIDE."""
    folder = os.path.join(work, mutant['id'])
    try:
        return judge_copy(builder, source, folder, mutant)
    except OSError as error:
        return 'INVALIDE', 'copie impossible : %s' % error, ''
    finally:
        if not keep:
            shutil.rmtree(folder, ignore_errors=True)


def judge_copy(builder, source, folder, mutant):
    """Copie, mute, construit et juge ; rend (verdict, detail, sortie)."""
    copy_tree(source, os.path.join(folder, 'src'))
    try:
        texts = mutated_files(os.path.join(folder, 'src'), mutant)
    except Refusal as refusal:
        return 'INVALIDE', str(refusal), ''
    for path, text in texts.items():
        with open(os.path.join(folder, 'src', path), 'w', encoding='utf-8', newline='') as handle:
            handle.write(text)
    failed, output = builder.configure_and_build(folder, mutant['options'])
    if mutant['attendu'] == 'construction':
        if failed is None:
            return 'SURVIT', 'la copie mutee se construit, un refus etait attendu', ''
        if mutant['jeton'] not in output:
            return 'INVALIDE', '%s en echec sans le jeton %s' % (failed, mutant['jeton']), output
        return 'TUE', 'construction', ''
    if failed is not None:
        return 'INVALIDE', 'le mutant ne passe pas la %s : il doit etre tue par sa porte' % failed, output
    verdict, output = builder.gate(folder, mutant['porte'])
    if verdict == 'absente':
        return 'INVALIDE', 'porte %s absente de la copie' % mutant['porte'], output
    if verdict == 'passe':
        return 'SURVIT', 'la porte %s passe sur la copie mutee' % mutant['porte'], ''
    return 'TUE', kill_cause(output), ''


def witness(builder, source, work, label, options, gates):
    """Temoin sans mutation d'un jeu d'options : rend None, ou le texte du desaccord."""
    folder = os.path.join(work, 'temoin_%s' % label)
    try:
        copy_tree(source, os.path.join(folder, 'src'))
        failed, output = builder.configure_and_build(folder, options)
        if failed is not None:
            return 'temoin [%s] : %s en echec\n%s' % (' '.join(options), failed, tail(output))
        for name in gates:
            verdict, output = builder.gate(folder, name)
            if verdict != 'passe':
                return 'temoin [%s] : porte %s %s sans mutation\n%s' % (' '.join(options), name, verdict, tail(output))
        return None
    finally:
        if not builder.args.keep:
            shutil.rmtree(folder, ignore_errors=True)


def prepare_work(path):
    """Dossier de travail vide et marque ; un dossier non vide qui n'est pas le notre est refuse (code 2)."""
    if os.path.isdir(path) and os.listdir(path):
        if not os.path.isfile(os.path.join(path, MARKER)):
            raise Refusal(2, 'dossier de travail non vide et sans marque %s : %s' % (MARKER, path))
        shutil.rmtree(path)
    os.makedirs(path, exist_ok=True)
    with open(os.path.join(path, MARKER), 'w', encoding='utf-8') as handle:
        handle.write('dossier de travail de tests/mutants/run_mutants.py\n')


def parse_arguments(argv):
    parser = argparse.ArgumentParser(description='Lanceur de mutants par correctif de la v11.')
    parser.add_argument('--manifest', required=True)
    parser.add_argument('--source', required=True)
    parser.add_argument('--work', default='')
    parser.add_argument('--jobs', type=int, default=0)
    parser.add_argument('--build-jobs', type=int, default=2)
    parser.add_argument('--only', default='')
    parser.add_argument('--floor', type=int, default=None)
    parser.add_argument('--cmake', default='cmake')
    parser.add_argument('--ctest', default='ctest')
    parser.add_argument('--generator', default='')
    parser.add_argument('--cmake-arg', action='append', default=[])
    parser.add_argument('--keep', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--list', action='store_true')
    try:
        args = parser.parse_args(argv)
    except SystemExit:
        raise Refusal(2, 'usage')
    if args.jobs < 0 or args.build_jobs < 1 or (args.floor is not None and args.floor < 1):
        raise Refusal(2, 'usage : --jobs >= 0, --build-jobs >= 1, --floor >= 1')
    if args.jobs == 0:
        args.jobs = max(1, (os.cpu_count() or 1) // 2)
    return args


def run(argv):
    args = parse_arguments(argv)
    manifest = load_manifest(args.manifest)
    source = os.path.abspath(args.source)
    module, mutants = manifest['module'], manifest['mutants']
    if args.list:
        for mutant in mutants:
            print('%s %s %s' % (mutant['id'], mutant['fichier'], mutant['porte']))
        return 0
    if args.only:
        wanted = args.only.split(',')
        known = {mutant['id'] for mutant in mutants}
        if len(set(wanted)) != len(wanted) or not set(wanted) <= known:
            raise Refusal(2, 'usage : --only cite un mutant inconnu ou en double')
        mutants = [mutant for mutant in mutants if mutant['id'] in wanted]
    floor = manifest['plancher'] if args.floor is None else args.floor
    if len(mutants) < floor:
        raise Refusal(3, 'PLANCHER module=%s : %d mutant(s) a juger, au moins %d attendus'
                      % (module, len(mutants), floor))
    check_patterns(source, mutants)
    if args.check:
        print('manifeste_ok module=%s mutants=%d plancher=%d' % (module, len(mutants), floor))
        return 0

    work = os.path.abspath(args.work) if args.work else os.path.join(
        os.environ.get('TMPDIR', '/tmp'), 'mhgp11_mutants_%s_%d' % (module, os.getpid()))
    prepare_work(work)
    builder = Builder(args, manifest['construction'])
    try:
        groups = {}
        for mutant in mutants:
            groups.setdefault(tuple(mutant['options']), []).append(mutant)
        for label, (options, members) in enumerate(sorted(groups.items())):
            gates = sorted({mutant['porte'] for mutant in members if mutant['attendu'] == 'porte'})
            problem = witness(builder, source, work, str(label), options, gates)
            if problem is not None:
                print(problem)
                print('TEMOIN ROUGE module=%s : aucun mutant juge' % module)
                return 1
        with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
            verdicts = list(pool.map(lambda mutant: judge(builder, source, work, mutant, args.keep), mutants))
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)

    counts = {'TUE': 0, 'SURVIT': 0, 'INVALIDE': 0}
    causes = {'signal': 0, 'delai': 0, 'construction': 0}
    for mutant, (verdict, detail, output) in zip(mutants, verdicts):
        counts[verdict] += 1
        if verdict == 'TUE' and detail in causes:
            causes[detail] += 1
        print('%-40s %-8s %s' % (mutant['id'], verdict, detail))
        if output:
            print(tail(output))
    sys.stdout.flush()
    if counts['SURVIT']:
        print('SURVIVANTS module=%s : %d sur %d' % (module, counts['SURVIT'], len(mutants)))
        return 1
    if counts['INVALIDE']:
        print('INVALIDES module=%s : %d sur %d' % (module, counts['INVALIDE'], len(mutants)))
        return 3
    print('mutants_ok module=%s mutants=%d tues=%d dont_signal=%d dont_delai=%d dont_construction=%d plancher=%d'
          % (module, len(mutants), counts['TUE'], causes['signal'], causes['delai'], causes['construction'], floor))
    return 0


def main():
    try:
        return run(sys.argv[1:])
    except Refusal as refusal:
        print(str(refusal))
        return refusal.code


if __name__ == '__main__':
    sys.exit(main())
