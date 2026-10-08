#!/usr/bin/env python3
"""Porte du pilote MES-C sur une sonde simulee, sans moteur ni donnees reelles. Python 3.10 nu, aucun assert (tient
sous -O).

  droites        Session simulee dont le mur vaut a + b n par groupe (premier tour plus lent de 5 ms, froid) : le
                 pilote retrouve a et b sur le tour chaud pour le groupe reel
                 et pour une famille synthetique, sans jamais les melanger ; C1 et C2 jugent la droite reelle de
                 cpu:5:48 aux seuils ecrits (2 ms, 3,727 us par site) ;
  difficiles     un refus wide_leaf sur la quasi-sphere a K5 rend C3 non tenu et reste un resultat publie (pas un
                 controle manquant) ; sans refus, C3 est tenu ;
  empreintes     une empreinte qui change d'un tour a l'autre (deux tours) fait manquer un controle ;
  usage          archive refusee (membre a chemin) : code 2 avant toute construction.
Codes : 0 conforme ; 1 ecart.
"""
import io
import json
import os
import stat
import sys
import tarfile
import tempfile
from contextlib import redirect_stderr, redirect_stdout

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pilote_c  # noqa: E402

LINES = {'reel': (1_000_000, 2_000), 'uniform': (5_000_000, 9_000), 'lattice': (1_000_000, 50_000),
         'sphere': (1_000_000, 50_000), 'line': (1_000_000, 1_000)}

FAKE = r'''#!/usr/bin/env python3
import json, os, sys
MODE = %r
LINES = %r
GROUPS = %r
frames = [a.split('=', 1)[1].split(',') for a in sys.argv[1:] if a.startswith('--trame=')]
args = dict(a.split('=', 1) for a in sys.argv[1:] if '=' in a and not a.startswith('--trame='))
passes, k, fils = int(args['--passes']), int(args['--k']), int(args['--threads'])
device = '--device' in sys.argv
if device:
    print(json.dumps(dict(phase='open', status='ok', reason='none', wall_ns=7, budget_appareil='separe')))
if MODE == 'refus_sphere' and len(frames) == 1 and GROUPS[frames[0][2]] == 'sphere' and k == 5:
    print(json.dumps(dict(phase='exit', status='unsupported_degeneracy', reason='wide_leaf')))
    sys.exit(2)
for i in range(passes):
    xyz, _ids, label = frames[i %% len(frames)]
    sites = os.path.getsize(xyz) // 12
    a, b = LINES[GROUPS[label]]
    wall = a + b * sites + (5_000_000 if i < len(frames) else 0)  # premier tour plus lent (froid)
    sha = ('%%02x' %% (i // len(frames) if MODE == 'empreinte_instable' else 7)) * 32
    row = dict(phase='full', trame=label, voie='device' if device else 'cpu', status='ok', coord_bits=21, kmax=k,
               threads=fils, sites=sites, wall_ns=wall, etapes_ns=dict(P=1, C=1, G=1, raccord=1, TMVR=4, T=1, M=1,
               V=1, R=1), c_ns=dict(parcours=1, feuilles=1, emission=1, fin_etage=1, transferts=0, publication=0),
               g_ns=dict(tables=0, resolution=1), hors_mur_ns=dict(validation=1, empreinte=1), pic_octets=9,
               cpu_ns=wall, rss_max_octets=9, appareil_octets=5 if device else 0, epinglee_octets=1 if device else 0,
               pic_appareil_octets=6 if device else 0,
               memoire_octets=dict(P=[1, 2], C=[3, 9], G=[4, 5], raccord=[4, 4], TMVR=[6, 7]), full_sha256=sha)
    row['pass'] = i
    print(json.dumps(row))
    print(json.dumps({'phase': 'liberation', 'pass': i, 'liberation_ns': 1}))
print(json.dumps(dict(phase='exit', status='ok', reason='none')))
'''

CLOUDS = [('objet_a', 'reel', 100), ('objet_b', 'reel', 1000), ('objet_c', 'reel', 3000), ('uni_1', 'uniform', 100),
          ('uni_2', 'uniform', 1000), ('lat_1', 'lattice', 300), ('sph_1', 'sphere', 1000), ('lin_1', 'line', 100)]


def make_archive(folder, bad=False):
    src = os.path.join(folder, 'paquet')
    os.makedirs(src, exist_ok=True)
    cases = []
    for name, group, n in CLOUDS:
        with open(os.path.join(src, name + '.u32le'), 'wb') as out:
            out.write(b'\0' * (12 * n))
        with open(os.path.join(src, name + '.ids.u32le'), 'wb') as out:
            out.write(b'\0' * (4 * n))
        case = dict(name=name, count=n, coordinates=name + '.u32le', point_ids=name + '.ids.u32le')
        if group == 'reel':
            case['subfamily'] = 'objet_reel'
        else:
            case.update(subfamily='synthetique', provenance=dict(family=group))
        cases.append(case)
    with open(os.path.join(src, 'bundle_manifest.json'), 'w', encoding='utf-8') as out:
        json.dump(dict(cases=cases), out)
    path = os.path.join(folder, 'small.tar')
    with tarfile.open(path, 'w') as tar:
        for member in sorted(os.listdir(src)):
            tar.add(os.path.join(src, member), arcname=('sous/' if bad and member == 'bundle_manifest.json' else '')
                    + member)
    return path


def campaign(folder, mode, bad=False):
    archive = make_archive(folder, bad)
    groups = {'c%03d' % i: g for i, (_n, g, _s) in enumerate(CLOUDS)}
    probe = os.path.join(folder, 'sonde_%s.py' % mode)
    with open(probe, 'w', encoding='utf-8') as out:
        out.write(FAKE % (mode, LINES, groups))
    os.chmod(probe, os.stat(probe).st_mode | stat.S_IXUSR)
    out_dir = os.path.join(folder, 'sortie_' + mode)
    argv = ['pilote_c.py', '--essai', '--sonde', probe, '--archive', archive, '--deballage',
            os.path.join(folder, 'deb_' + mode), '--sortie', out_dir, '--k', '5', '--fils', '48', '--tours', '2',
            '--fils-difficiles', '48', '--delai-global', '300', '--delai-cas', '60']
    with redirect_stdout(io.StringIO()), redirect_stderr(io.StringIO()):
        code = pilote_c.main(argv)
    try:
        with open(os.path.join(out_dir, 'rapport_c.json'), encoding='utf-8') as handle:
            return code, json.load(handle)
    except (OSError, ValueError):
        return code, None


def near(x, y):
    return abs(x - y) <= 1e-6 * max(1.0, abs(y))


def main():
    errors = []
    with tempfile.TemporaryDirectory() as folder:
        code, report = campaign(folder, 'ok')
        lines = (report or {}).get('configurations', {}).get('cpu:5:48', {}).get('droites', {})
        reel, uni = lines.get('reel'), lines.get('uniform')
        if code != 0 or report is None or not reel or not uni or not near(reel['fixe_ns'], 1e6) or \
                not near(reel['par_site_ns'], 2000) or not near(uni['fixe_ns'], 5e6) or \
                not near(uni['par_site_ns'], 9000) or reel['nuages'] != 3 or set(lines) != {'reel', 'uniform'}:
            errors.append('droites : code %s, droites %s' % (code, lines))
        crit = (report or {}).get('criteres', {})
        if [crit.get(c, {}).get('etat') for c in ('C1', 'C2', 'C3')] != ['tenu', 'tenu', 'tenu'] or \
                (report or {}).get('controles'):
            errors.append('criteres : %s, controles %s' % (crit, (report or {}).get('controles')))
        code, report = campaign(folder, 'refus_sphere')
        crit = (report or {}).get('criteres', {})
        sphere = [h for h in (report or {}).get('difficiles', []) if h['nom'] == 'sph_1']
        if code != 0 or crit.get('C3', {}).get('etat') != 'non tenu' or not sphere or sphere[0]['etat'] != 'refus' \
                or (report or {}).get('controles'):
            errors.append('difficiles : C3 %s, sphere %s, controles %s' % (crit.get('C3'), sphere,
                                                                            (report or {}).get('controles')))
        code, report = campaign(folder, 'empreinte_instable')
        if code != 0 or not any('empreintes FUL1' in c for c in (report or {}).get('controles', [])):
            errors.append('empreintes : controles %s' % (report or {}).get('controles'))
        code, report = campaign(folder, 'usage', bad=True)
        if code != 2:
            errors.append('usage : archive a chemin admise (code %s)' % code)
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print('test_pilote_c_ok droites=1 criteres=1 difficiles=1 empreintes=1 usage=1')
    return 0


if __name__ == '__main__':
    sys.exit(main())
