#!/usr/bin/env python3
"""Porte des outils de preparation des donnees (bench/data/) : constats CST-0216, CST-0217, CST-0218.

Petits fichiers synthetiques seulement (aucune donnee reelle, aucun reseau), ecrits ici en bibliotheque standard ; les
vrais outils sont lances en sous-processus : replay_all.sh (bash), verify_inputs.py (python -S), crop_scenes.py
(python -I, numpy requis par l'outil, pas par cette porte).
  - CST-0216 : le pilote trouve ses outils a cote de lui depuis un autre repertoire courant ; `outils` (rejeu a blanc)
    rend 0 sur la disposition versionnee, 1 si un outil est modifie, absent ou non epingle ; ROOT absent ou dans
    l'arbre v12, etape inconnue : 2 ; `verify` rejoue le vrai verificateur sur un dossier synthetique ;
  - CST-0217 : manifeste vide, empreinte nulle, absente ou mal formee, multiplicite sans empreinte, nom de chemin,
    compte nul, schema inconnu : code 2 et aucun fichier lu ; empreinte fausse, fichier absent ou tronque : code 1 ;
  - CST-0218 : colonne de quatre sites au centre, taille visee 2 : les quatre sont gardes (count 4, rayon 0) ; le
    temoin de l'auditeur (colonne seule) ne produit aucune decoupe, puisque le carre couvre toute la scene ; decoupes
    emboitees ; aucune entree perimee laissee pour une decoupe sautee ; decoupes admises par verify_inputs.
Usage : python3 -S -O bench/donnees_test.py [--python PYTHON_AVEC_NUMPY]
Codes : 0 conforme, 1 ecart (detail en JSON), 2 usage. Aucune garde par assert (tient sous -O).
"""
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ICI = Path(__file__).resolve().parent
DATA = ICI / 'data'
SCHEMA = 'mhgp12.benchmark_inputs.v1'


class Ecart(Exception):
    pass


def exiger(condition, message):
    if not condition:
        raise Ecart(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(argv, cwd=None, env=None):
    return subprocess.run([str(a) for a in argv], cwd=cwd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True, timeout=120)


def ecrire_cas(dossier, nom, points, ids=None):
    """Cas au format v11 (x, y, z u32 entrelaces ; ids u32), ordre lexicographique, translation au minimum."""
    points = sorted(points)
    ids = list(range(len(points))) if ids is None else ids
    lo = [min(p[a] for p in points) for a in range(3)]
    pts = [tuple(p[a] - lo[a] for a in range(3)) for p in points]
    xyz = b''.join(struct.pack('<3I', *p) for p in pts)
    idb = b''.join(struct.pack('<I', i) for i in ids)
    (dossier / (nom + '.u32le')).write_bytes(xyz)
    (dossier / (nom + '.ids.u32le')).write_bytes(idb)
    ext = [max(p[a] for p in pts) for a in range(3)]
    return {'name': nom, 'coordinates': nom + '.u32le', 'point_ids': nom + '.ids.u32le', 'count': len(pts),
            'duplicate_sites': 0, 'sha256': sha(xyz), 'ids_sha256': sha(idb), 'extent_mm': ext,
            'bits_needed': max(e.bit_length() for e in ext)}


def manifeste(chemin, cas, **extra):
    valeur = dict({'schema': SCHEMA, 'cases': cas}, **extra)
    chemin.write_text(json.dumps(valeur))
    return chemin


def verifier(python, chemin, *options):
    r = run([python, '-S', DATA / 'verify_inputs.py', chemin] + list(options))
    return r.returncode, r.stdout


def porte_verificateur(python, tmp):
    dossier = tmp / 'verif'
    dossier.mkdir()
    base = ecrire_cas(dossier, 'colonne', [(0, 0, z) for z in range(4)])
    m = dossier / 'manifest.json'
    lignes = []

    def cas(nom, contenu, attendu, mot=None):
        if isinstance(contenu, list):
            manifeste(m, contenu)
        else:
            m.write_text(json.dumps(contenu))
        code, sortie = verifier(python, m)
        exiger(code == attendu, '%s : code %d, attendu %d (%s)' % (nom, code, attendu, sortie.strip()[-200:]))
        if mot is not None:
            exiger(mot in sortie, '%s : sortie sans %r (%s)' % (nom, mot, sortie.strip()[-200:]))
        if attendu == 2:
            exiger('aucun fichier lu' in sortie or 'illisible' in sortie or 'python3 -S' in sortie,
                   '%s : refus sans mention (%s)' % (nom, sortie.strip()[-200:]))
        lignes.append({'cas': nom, 'code': code})

    cas('nominal', [base], 0, '0 ecarts')
    cas('manifeste_vide (auditeur)', [], 2, 'REFUS')
    cas('empreinte_nulle (auditeur)', [dict(base, sha256=None)], 2, 'REFUS')
    cas('empreinte_fausse (auditeur)', [dict(base, sha256='0' * 64)], 1, 'EMPREINTE')
    cas('empreinte_absente', [{k: v for k, v in base.items() if k != 'ids_sha256'}], 2, 'REFUS')
    cas('empreinte_majuscules', [dict(base, sha256=base['sha256'].upper())], 2, 'REFUS')
    cas('empreinte_courte', [dict(base, sha256=base['sha256'][:63])], 2, 'REFUS')
    cas('nom_chemin', [dict(base, coordinates='../colonne.u32le')], 2, 'REFUS')
    cas('compte_nul', [dict(base, count=0)], 2, 'REFUS')
    cas('compte_booleen', [dict(base, count=True)], 2, 'REFUS')
    cas('multiplicite_sans_empreinte', [dict(base, mult='colonne.ids.u32le')], 2, 'REFUS')
    cas('distincte_sans_multiplicites', [dict(base, distinct={k: base[k] for k in (
        'coordinates', 'point_ids', 'count', 'sha256', 'ids_sha256')})], 2, 'REFUS')
    cas('paquet_inconnu', [dict(base, bundled='brut')], 2, 'REFUS')
    cas('meme_nom_deux_empreintes', [base, dict(base, name='autre', sha256='1' * 64)], 2, 'REFUS')
    cas('schema_inconnu', {'schema': 'autre', 'cases': [base]}, 2, 'REFUS')
    cas('cases_absente', {'schema': SCHEMA}, 2, 'REFUS')
    cas('crops_non_liste', {'schema': SCHEMA, 'cases': [base], 'crops': {}}, 2, 'REFUS')
    cas('objet_non_dict', [base, 7], 2, 'REFUS')
    (dossier / 'colonne.ids.u32le').write_bytes(b'\0' * 4)
    cas('fichier_tronque', [base], 1, 'TAILLE')
    (dossier / 'colonne.ids.u32le').unlink()
    cas('fichier_absent', [base], 1, 'ABSENT')
    r = run([python, '-S', DATA / 'verify_inputs.py'])
    exiger(r.returncode == 2, 'sans argument : code %d' % r.returncode)
    r = run([python, '-S', DATA / 'verify_inputs.py', m, '--autre'])
    exiger(r.returncode == 2, 'option inconnue : code %d' % r.returncode)
    lignes.append({'cas': 'usage', 'code': 2})
    return lignes


def faux_arbre(tmp):
    """Copie de bench/data dans un arbre morsehgp3D_v12 jetable (aucun ecrit dans la copie de travail)."""
    racine = tmp / 'arbre' / 'morsehgp3D_v12' / 'bench' / 'data'
    shutil.copytree(DATA, racine, ignore=shutil.ignore_patterns('__pycache__'))
    return racine


def porte_pilote(python, tmp):
    lignes = []
    ailleurs = tmp / 'ailleurs'
    ailleurs.mkdir()
    env = {k: v for k, v in os.environ.items() if k not in ('ROOT', 'PY')}
    # Disposition versionnee, depuis un autre repertoire courant.
    r = run(['bash', DATA / 'replay_all.sh', 'outils'], cwd=ailleurs, env=env)
    exiger(r.returncode == 0 and 'outils conformes' in r.stderr, 'outils (copie de travail) : %d %s'
           % (r.returncode, r.stderr[-300:]))
    lignes.append({'cas': 'outils_depuis_un_autre_repertoire', 'code': r.returncode})
    for nom, alterer, attendu in (
            ('outil_modifie', lambda d: (d / 'verify_inputs.py').write_text(
                (d / 'verify_inputs.py').read_text() + '\n# modifie\n'), 1),
            ('outil_absent', lambda d: (d / 'crop_scenes.py').unlink(), 1),
            ('script_non_epingle', lambda d: (d / 'v12data' / 'intrus.py').write_text('x = 1\n'), 1),
            ('epingle_absente', lambda d: (d / 'SHA256SUMS.txt').unlink(), 1)):
        copie = faux_arbre(tmp / nom)
        alterer(copie)
        r = run(['bash', copie / 'replay_all.sh', 'outils'], cwd=ailleurs, env=env)
        exiger(r.returncode == attendu, '%s : code %d, attendu %d (%s)' % (nom, r.returncode, attendu,
                                                                        r.stderr[-300:]))
        lignes.append({'cas': nom, 'code': r.returncode})
    copie = faux_arbre(tmp / 'usage')
    for nom, etapes, extra, attendu in (
            ('etape_inconnue', ['inconnue'], {}, 2),
            ('root_absent', ['verify'], {}, 2),
            ('root_dans_v12', ['verify'], {'ROOT': str(copie.parent.parent / 'donnees')}, 2)):
        r = run(['bash', copie / 'replay_all.sh'] + etapes, cwd=ailleurs, env=dict(env, **extra))
        exiger(r.returncode == attendu, '%s : code %d (%s)' % (nom, r.returncode, r.stderr[-300:]))
        lignes.append({'cas': nom, 'code': r.returncode})
    exiger(not (copie.parent.parent / 'donnees').exists(), 'ROOT dans l\'arbre v12 : dossier cree malgre le refus')
    # Etape verify : le vrai verificateur, avec --measure, sur un dossier de donnees synthetique.
    racine = tmp / 'donnees'
    for jeu, fichier in (('synth', 'manifest.json'), ('semantickitti', 'manifest_v12set.json')):
        dossier = racine / 'data' / jeu
        dossier.mkdir(parents=True)
        manifeste(dossier / fichier, [ecrire_cas(dossier, jeu + '_a', [(1, 2, 3), (4, 0, 9), (7, 7, 0)])])
    r = run(['bash', DATA / 'replay_all.sh', 'verify'], cwd=ailleurs, env=dict(env, ROOT=str(racine), PY=python))
    exiger(r.returncode == 0 and r.stdout.count(' 0 ecarts') == 2, 'verify : %d %s %s' % (
        r.returncode, r.stdout[-300:], r.stderr[-300:]))
    lignes.append({'cas': 'verify_synthetique', 'code': r.returncode})
    manifeste(racine / 'data' / 'synth' / 'manifest.json', [])
    r = run(['bash', DATA / 'replay_all.sh', 'verify'], cwd=ailleurs, env=dict(env, ROOT=str(racine), PY=python))
    exiger(r.returncode == 2, 'verify sur un manifeste vide : code %d' % r.returncode)
    lignes.append({'cas': 'verify_manifeste_vide', 'code': r.returncode})
    shutil.rmtree(racine / 'data')
    r = run(['bash', DATA / 'replay_all.sh', 'verify'], cwd=ailleurs, env=dict(env, ROOT=str(racine), PY=python))
    exiger(r.returncode == 2, 'verify sans aucun manifeste : code %d (jamais un vert vide)' % r.returncode)
    lignes.append({'cas': 'verify_sans_donnees', 'code': r.returncode})
    return lignes


def lire_u32(chemin):
    data = chemin.read_bytes()
    return list(struct.unpack('<%dI' % (len(data) // 4), data))


def decouper(python, m, *tailles):
    r = run([python, '-I', '-B', DATA / 'crop_scenes.py', '--manifest', m, '--sizes'] + [str(t) for t in tailles])
    exiger(r.returncode == 0, 'crop_scenes : code %d (%s)' % (r.returncode, r.stderr[-400:]))
    return json.loads(m.read_text()).get('crops', [])


def porte_decoupes(python, tmp):
    lignes = []
    # Temoin de l'auditeur : la colonne seule. Aucune selection horizontale ne peut en garder deux sites : le carre
    # de rayon 0 les contient tous, il couvre la scene, la decoupe est sautee.
    d1 = tmp / 'colonne_seule'
    d1.mkdir()
    m1 = manifeste(d1 / 'manifest.json', [ecrire_cas(d1, 'colonne', [(0, 0, z) for z in range(4)])])
    exiger(decouper(python, m1, 2) == [], 'colonne seule : une decoupe tronquee publiee')
    lignes.append({'cas': 'temoin_auditeur_colonne_seule', 'decoupes': 0})
    # Colonne de quatre sites au centre de la boite xy, et huit sites plus loin.
    d2 = tmp / 'colonne_centre'
    d2.mkdir()
    loin = [(0, 0, 0), (10, 0, 1), (0, 10, 2), (10, 10, 3), (5, 0, 4), (0, 5, 5), (10, 5, 6), (5, 10, 7)]
    cas = ecrire_cas(d2, 'scene', [(5, 5, z) for z in range(4)] + [(6, 5, 9)] + loin)
    m2 = manifeste(d2 / 'manifest.json', [cas])
    crops = {c['name']: c for c in decouper(python, m2, 2, 5, 9)}
    c2 = crops.get('scene_c2')
    exiger(c2 is not None and c2['count'] == 4 and c2['crop']['target_sites'] == 2 and
           c2['crop']['radius_chebyshev_mm'] == 0, 'colonne : %r' % c2)
    xyz = lire_u32(d2 / c2['coordinates'])
    exiger(sorted(zip(xyz[0::3], xyz[1::3], xyz[2::3])) == [(0, 0, z) for z in range(4)],
           'colonne : sites gardes %r' % xyz)
    ids = lire_u32(d2 / c2['point_ids'])
    exiger(len(ids) == 4, 'colonne : identifiants %r' % ids)
    c5 = crops.get('scene_c5')
    exiger(c5 is not None and c5['count'] == 5 and c5['crop']['radius_chebyshev_mm'] == 1, 'rayon 1 : %r' % c5)
    exiger('scene_c9' not in crops, 'carre de rayon 5 = toute la scene : decoupe non sautee')
    ids5 = set(lire_u32(d2 / c5['point_ids']))
    exiger(set(ids) <= ids5, 'decoupes non emboitees')
    lignes.append({'cas': 'colonne_au_centre', 'taille_visee': 2, 'sites_gardes': c2['count'],
                   'rayon_mm': c2['crop']['radius_chebyshev_mm']})
    lignes.append({'cas': 'emboitement_et_carre_couvrant', 'c5': c5['count'], 'c9': 'sautee'})
    code, sortie = verifier(python, m2)
    exiger(code == 0, 'decoupes refusees par verify_inputs : %s' % sortie[-300:])
    lignes.append({'cas': 'decoupes_admises_par_verify_inputs', 'code': code})
    # Entree perimee (rejeu anterieur) pour une decoupe desormais sautee : retiree du manifeste.
    valeur = json.loads(m2.read_text())
    valeur['crops'].append(dict(c2, name='scene_c9'))
    m2.write_text(json.dumps(valeur))
    crops = {c['name'] for c in decouper(python, m2, 2, 5, 9)}
    exiger('scene_c9' not in crops and {'scene_c2', 'scene_c5'} <= crops, 'entree perimee laissee : %r' % crops)
    lignes.append({'cas': 'entree_perimee_retiree', 'decoupes': sorted(crops)})
    return lignes


def main(argv):
    python = sys.executable
    if len(argv) == 3 and argv[1] == '--python':
        python = argv[2]
    elif len(argv) != 1:
        print(__doc__)
        return 2
    resultats, ecarts = [], []
    with tempfile.TemporaryDirectory(prefix='mhgp12-donnees-') as tmp:
        tmp = Path(tmp)
        for nom, porte in (('verificateur', porte_verificateur), ('pilote', porte_pilote),
                           ('decoupes', porte_decoupes)):
            dossier = tmp / nom
            dossier.mkdir()
            try:
                resultats += porte(python, dossier)
            except Ecart as e:
                ecarts.append('%s : %s' % (nom, e))
    if list(DATA.rglob('__pycache__')):
        ecarts.append('cache de bytecode ecrit dans bench/data : le dossier ne serait plus egal a son epingle')
    for r in resultats:
        print(json.dumps(r, ensure_ascii=False, sort_keys=True))
    print(json.dumps({'porte': 'donnees_v12', 'cas': len(resultats), 'ecarts': ecarts, 'optimise': sys.flags.optimize,
                      'python': sys.version.split()[0]}, ensure_ascii=False))
    return 0 if not ecarts else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
