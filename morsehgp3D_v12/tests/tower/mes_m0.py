#!/usr/bin/env python3
"""MES-M0 de la tour (contrat, paragraphe 9.1) sur les graines de la v11 : vidages FUL1 de la v12 identiques a ceux de
la v11 gelee.

Les cas sont les neuf de docs/MESURE.md, paragraphe 4 (ng00, ng01, ng02 a K5 et K10 ; uniformes de 8 000, 16 000 et
32 000 sites a K5). Pour chacun, mhgp12_tower_dumps lit les vidages MHGP12DP de la v11 (outil mhgp12_vidage des
microbancs) et joue T, M, V, R et l'export, dans les deux modes de cibles (graines de la v11 ; regle d'arret de la v12
derivee des parties de descente) :
  - profil 21 : SHA-256 des octets egal a l'empreinte de MESURE.md ;
  - profils 24 et 32 : le vidage est ecrit, relu par le lecteur strict adapte (full_reader.py) ; empreinte SEMANTIQUE
    egale a celle des vidages de la v11 (gravee ci-dessous, calculee par ce lecteur sur les vidages de la v11 reproduits
    a l'octet, et recoupee par le lecteur de la v11 lui-meme) ;
  - determinisme : a 1 fil et a 8 fils (tranches de contraction de 97 evenements), memes empreintes et memes compteurs
    de l'objet et du travail ;
  - avec --juge-emst : JUG-EMST (juges/emst) sur l'ordre un du vidage ecrit (code 0 exige).

    python3 mes_m0.py <mhgp12_tower_dumps> <bits> <donnees> <vidages> [--cas c1,c2,...] [--modes m1,m2]
                      [--juge-emst <binaire>]
    python3 mes_m0.py --chaine <mhgp12_tower_chain> <bits> <donnees> [--cas c1,c2,...]

Mode --chaine (sortie de T2) : la chaine du produit, catalogue de T1 et etage G compris (mhgp12_tower_chain), sans
vidage de la v11 : empreinte SEMANTIQUE egale a celle des vidages de la v11 a tout profil (les octets peuvent differer :
formes des niveaux et des centres suivant le departage de S* par positions, temoin WIT-FORME-NIVEAU), 1 fil contre
8 fils.

<donnees> : les trames (lidar_ng0*.u32le, uniform_u18_n*.u32le et leurs .ids.u32le), ou - pour MHGP12_DATA_DIR ;
<vidages> : un dossier par cas
(ng00_k5, ..., u32000_k5) contenant cat.bin et ordre_k.bin. Codes : 0 conforme ; 1 ecart ; 2 usage ; 3 refus d'un
outil ou entree absente. Python 3.10 nu, aucun assert. Les donnees SemanticKITTI ne sont jamais versees.
"""
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import full_reader  # noqa: E402

OK, DISAGREEMENT, USAGE, REFUSED = 0, 1, 2, 3
# cas : (trame, K, sites, SHA-256 des octets (MESURE.md, paragraphe 4), empreinte semantique des vidages de la v11)
CASES = {
    'u8000_k5': ('uniform_u18_n8000', 5, 8000,
                 'f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf',
                 'a45a9c64ed8a39f5fae3b6065e9c9b3785d83e8129972590521b49ed8346bdb0'),
    'u16000_k5': ('uniform_u18_n16000', 5, 16000,
                  '141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889',
                  '04a7b6bd55766feb459beae93c43759f6583ba5a95b910ba03e0e53d33650e61'),
    'u32000_k5': ('uniform_u18_n32000', 5, 32000,
                  'a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b',
                  '5a3b2d363c007c0711b92dc2dbde8dbb72cee0dfb58dd7e9b8253001c5250353'),
    'ng00_k5': ('lidar_ng00', 5, 39885, '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
                'fcca94600524d0dc931d430f590ebb7177593f266f0af1a0a96013e4e55dd276'),
    'ng01_k5': ('lidar_ng01', 5, 35551, '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
                '07277866f20347eff9b4f37fccf10a598e7ed6ef94a2ec99c28c6be6befcad21'),
    'ng02_k5': ('lidar_ng02', 5, 45845, '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207',
                '9741adbfb76ff77296a1fd42df7ff5eed0d9674f1a7259d600086852521df868'),
    'ng00_k10': ('lidar_ng00', 10, 39885, '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
                 'cda4390176faed3fbca64fd3723643e12a1d44ce1b704e78834d6fd492053958'),
    'ng01_k10': ('lidar_ng01', 10, 35551, '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
                 '7e568d4eb91b3cfb2714a2ba29ee2ae990f4e1cc0fd3be7763a08df8fedb4456'),
    'ng02_k10': ('lidar_ng02', 10, 45845, '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e',
                 'dd01755285a44f3f9904e7435d226f7c8a4d6023d557948718b2edf8ba7dd230'),
}
MODES = ('graines', 'v12')


def play(tool, data, dumps, case, mode, threads, extra):
    frame = CASES[case][0]
    command = [tool, os.path.join(data, frame + '.u32le'), os.path.join(data, frame + '.ids.u32le'),
               os.path.join(dumps, case), '--cibles', mode, '--fils', str(threads)] + extra
    done = subprocess.run(command, capture_output=True, text=True, check=False)
    try:
        result = json.loads(done.stdout.strip().splitlines()[-1])
    except (ValueError, IndexError):
        result = dict(refus='sortie illisible : %s' % done.stderr.strip()[:200])
    return done.returncode, result


def logical(result):
    """Empreinte et compteurs de l'objet et du travail (sans les durees)."""
    return (result.get('sha256'), result.get('octets'),
            [dict((key, value) for key, value in order.items() if key != 'ns_min') for order in result.get('ordres', [])])


def check_case(args, case, mode, scratch):
    """Ecarts d'un cas et d'un mode : empreinte (octets ou semantique), determinisme, JUG-EMST."""
    tool, bits, data, dumps, judge = args
    frame, kmax, sites, raw, semantic = CASES[case]
    out = os.path.join(scratch, '%s_%s' % (case, mode))
    code, first = play(tool, data, dumps, case, mode, 1, ['--sortie', out])
    if code != 0:
        return ['%s %s : outil code %d, %s' % (case, mode, code, first.get('refus', ''))], REFUSED
    gaps = []
    path = os.path.join(out, 'tour.ful1')
    if bits == 21 and first['sha256'] != raw:
        gaps.append('%s %s : octets %s, attendu %s' % (case, mode, first['sha256'], raw))
    if bits != 21:
        try:
            decoded = full_reader.inspect(path, bits, kmax, sites)
        except ValueError as error:
            return ['%s %s : lecteur strict : %s' % (case, mode, error)], DISAGREEMENT
        if decoded['sha256'] != semantic:
            gaps.append('%s %s : empreinte semantique %s, attendu %s' % (case, mode, decoded['sha256'], semantic))
    code8, eight = play(tool, data, dumps, case, mode, 8, ['--tranche', '97'])
    if code8 != 0 or logical(eight) != logical(first):
        gaps.append('%s %s : 8 fils differents de 1 fil' % (case, mode))
    if judge is not None and mode == 'graines':
        done = subprocess.run([judge, os.path.join(data, frame + '.u32le'), '--ids',
                               os.path.join(data, frame + '.ids.u32le'), '--vidage', path, '--bits', str(bits)],
                              capture_output=True, text=True, check=False)
        if done.returncode != 0:
            gaps.append('%s : JUG-EMST code %d' % (case, done.returncode))
    shutil.rmtree(out, ignore_errors=True)
    return gaps, DISAGREEMENT


def chain_case(tool, bits, data, case, scratch):
    """Mode --chaine : ecarts d'un cas (empreinte semantique, determinisme)."""
    frame, kmax, sites, _raw, semantic = CASES[case]
    out = os.path.join(scratch, case)
    base = [tool, os.path.join(data, frame + '.u32le'), os.path.join(data, frame + '.ids.u32le'), str(kmax)]
    runs = []
    for extra in (['--fils', '1', '--sortie', out], ['--fils', '8', '--tranche', '97']):
        done = subprocess.run(base + extra, capture_output=True, text=True, check=False)
        try:
            runs.append((done.returncode, json.loads(done.stdout.strip().splitlines()[-1])))
        except (ValueError, IndexError):
            runs.append((done.returncode, dict(refus='sortie illisible')))
    (code, first), (code8, eight) = runs
    if code != 0:
        return ['%s chaine : code %d %s' % (case, code, first.get('refus', ''))], REFUSED
    gaps = []
    try:
        decoded = full_reader.inspect(os.path.join(out, 'tour.ful1'), bits, kmax, sites)
        if decoded['sha256'] != semantic:
            gaps.append('%s chaine : empreinte semantique %s, attendu %s' % (case, decoded['sha256'], semantic))
    except ValueError as error:
        gaps.append('%s chaine : lecteur strict : %s' % (case, error))
    if code8 != 0 or logical(eight) != logical(first):
        gaps.append('%s chaine : 8 fils differents de 1 fil' % case)
    shutil.rmtree(out, ignore_errors=True)
    return gaps, DISAGREEMENT


def main_chain(args):
    if len(args) < 3 or args[1] not in ('21', '24', '32'):
        print(__doc__.split('\n\n')[2], file=sys.stderr)
        return USAGE
    tool, bits, data, cases = args[0], int(args[1]), args[2], list(CASES)
    if data == '-':
        data = os.environ.get('MHGP12_DATA_DIR', '')
    if len(args) == 5 and args[3] == '--cas':
        cases = args[4].split(',')
    elif len(args) != 3:
        print('usage : mes_m0.py --chaine <outil> <bits> <donnees> [--cas c1,c2,...]', file=sys.stderr)
        return USAGE
    if any(case not in CASES for case in cases):
        print('cas inconnu dans %r' % cases, file=sys.stderr)
        return USAGE
    scratch = tempfile.mkdtemp(prefix='mhgp12_chain_m0_')
    failures, worst = [], OK
    try:
        for case in cases:
            gaps, code = chain_case(tool, bits, data, case, scratch)
            failures += gaps
            worst = max(worst, code if gaps else OK)
            print('%s chaine %s' % (case, 'conforme' if not gaps else 'ECART'))
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    if failures:
        for gap in failures:
            print(gap, file=sys.stderr)
        return worst
    print('mes_m0_chaine_ok profil=%d cas=%d' % (bits, len(cases)))
    return OK


def main(argv):
    args = argv[1:]
    if args and args[0] == '--chaine':
        return main_chain(args[1:])
    if len(args) < 4 or args[1] not in ('21', '24', '32'):
        print(__doc__.split('\n\n')[2], file=sys.stderr)
        return USAGE
    tool, bits, data, dumps = args[0], int(args[1]), args[2], args[3]
    if data == '-':  # porte lidar : les trames sont dans MHGP12_DATA_DIR
        data = os.environ.get('MHGP12_DATA_DIR', '')
    cases, judge, modes, rest = list(CASES), None, list(MODES), args[4:]
    while rest:
        if rest[0] == '--cas' and len(rest) > 1:
            cases = rest[1].split(',')
        elif rest[0] == '--modes' and len(rest) > 1 and all(m in MODES for m in rest[1].split(',')):
            modes = rest[1].split(',')
        elif rest[0] == '--juge-emst' and len(rest) > 1:
            judge = rest[1]
        else:
            print('option inconnue : %r' % rest[0], file=sys.stderr)
            return USAGE
        rest = rest[2:]
    if any(case not in CASES for case in cases):
        print('cas inconnu dans %r' % cases, file=sys.stderr)
        return USAGE
    for case in cases:
        if not os.path.isfile(os.path.join(dumps, case, 'cat.bin')):
            print('vidages absents : %s' % os.path.join(dumps, case), file=sys.stderr)
            return REFUSED
    scratch = tempfile.mkdtemp(prefix='mhgp12_mes_m0_')
    failures, worst = [], OK
    try:
        for case in cases:
            for mode in modes:
                gaps, code = check_case((tool, bits, data, dumps, judge), case, mode, scratch)
                failures += gaps
                worst = max(worst, code if gaps else OK)
                print('%s %s %s' % (case, mode, 'conforme' if not gaps else 'ECART'))
    finally:
        shutil.rmtree(scratch, ignore_errors=True)
    if failures:
        for gap in failures:
            print(gap, file=sys.stderr)
        return worst
    print('mes_m0_ok profil=%d cas=%d modes=%d juge_emst=%s' % (bits, len(cases), len(modes),
                                                                  'oui' if judge else 'non'))
    return OK


if __name__ == '__main__':
    sys.exit(main(sys.argv))
