"""Calcule valeurs.json depuis les journaux de l'audit."""
import json
import os
import re
import sys

B = '/tmp/v11-audit/l08_tests_portes/'
heure = sys.argv[1] if len(sys.argv) > 1 else '@@HEURE_FIN@@'


def fr(n):
    return '{:,}'.format(int(n)).replace(',', ' ')


vals = {
    'LIDAR_EXTENDED': '227 sur 1 306 696, coquille maximale 5',
    'EULER_LIDAR': ('**$E_K = 1$** sur les trois trames, pour K = 1 à 5 à kcat = 7 (2 565 656, 2 104 698 et 2 675 990 '
                    'boules ; 320, 204 et 865 coquilles étendues) et pour **K = 1 à 10** à kcat = 12 (8 314 472, '
                    '6 492 748 et 8 025 829 boules ; 529, 341 et 1 559 coquilles étendues)'),
    'DET_L00_CAT': 'identiques à 1, 2, 4 fils (203 Mo)', 'DET_L00_CAT_P': 'identique',
    'DET_L00_TOW': 'identiques à 1, 2, 4 fils', 'DET_L00_TOW_P': 'identique',
    'TSAN_RESULT': ('`mhgp10_unit` (dont le stress du pool à 4 et 8 fils) et `mhgp10_tower` (2 000 points uniformes, '
                    'K = 5, 4 fils : catalogue, tour FULL, attaches) construits avec `-DMHGP10_TSAN=ON` et lancés par '
                    '`setarch -R` : codes 0, aucun rapport. La suite complète n\'a pas été jouée sous TSan.'),
    'TSAN_SHORT': 'joué sur `mhgp10_unit` et une tour de 2 000 points seulement',
    'HEURE_FIN': heure,
}
# ---- ASan
s = open(B + 'logs/ctest-asan.log').read()
t = re.search(r'(\d+)% tests passed, (\d+) tests failed out of (\d+)', s)
tt = re.search(r'Total Test time \(real\) = +([\d.]+) sec', s)
san = len(re.findall(r'ERROR: AddressSanitizer|runtime error:|LeakSanitizer', s))
if t and tt:
    ok = int(t.group(3)) - int(t.group(2))
    vals['ASAN_RESULT'] = '**%d / %s**, code 0, %d rapport de sanitizer' % (ok, t.group(3), san)
    vals['ASAN_TIME'] = '%s s (charge 15 à 36)' % fr(float(tt.group(1)))
# ---- oracles K = 7, 8, 10
lines = open(B + 'exp/oracle_k10.out').read().splitlines()
cat = [l for l in lines if l.startswith('catalogue fini')]
tour = [l for l in lines if l.startswith('tour nuage')]
if cat:
    d = eval(cat[-1][cat[-1].index('{'):cat[-1].rindex('}') + 1])
    vals['K10_CAT'] = '%d (K = 8 : %s boules jugées ; K = 10 : %s)' % (d['catalogue_K8'] + d['catalogue_K10'], fr(d['catalogue_boules_K8']), fr(d['catalogue_boules_K10']))
    vals['K10_CAT_ECARTS'] = '**%s**' % re.search(r'ecarts (\d+)', cat[-1]).group(1)
if tour:
    d = eval(tour[-1][tour[-1].index('{'):tour[-1].rindex('}') + 1])
    e = re.search(r'ecarts (\d+)', tour[-1]).group(1)
    vals['K10_TOUR'] = '%d (K = 7 : %d nuages, %s coupes ; K = 10 : %d nuages, %s coupes)' % (
        d.get('tour_K7', 0) + d.get('tour_K10', 0), d.get('tour_K7', 0), fr(d.get('tour_coupes_K7', 0)), d.get('tour_K10', 0), fr(d.get('tour_coupes_K10', 0)))
    vals['K10_TOUR_ECARTS'] = '**%s**' % e
# ---- determinisme K = 10 (catalogue : determinisme.py ; tour avec attaches : determinisme_tour.py)
rows = ''
p = B + 'exp/determinisme_lidar00_k10.json'
if os.path.exists(p) and os.path.getsize(p) > 50:
    s = open(p).read()
    j = json.loads(s[:s.rindex('}') + 1])
    rows += '| `lidar00`, K = 10 | catalogue (%s Mo) | %s | %s |\n' % (
        fr(j['catalogue_octets'] // 1000000), 'identiques à 1 et 4 fils' if j['catalogue_identique_selon_fils'] else '**différents**',
        'identique' if j['catalogue_permute_identique'] else '**différent**')
p = B + 'exp/determinisme_tour_lidar00_k10.json'
if os.path.exists(p) and os.path.getsize(p) > 50:
    s = open(p).read()
    j = json.loads(s[:s.rindex('}') + 1])
    rows += '| `lidar00`, K = 10 | tour FULL avec attaches (%s Mo) | %s | %s |\n' % (
        fr(j['tour_octets'] // 1000000), 'identiques à 1 et 4 fils' if j['tour_identique_selon_fils'] else '**différents**',
        'identique' if j['tour_permutee_identique'] else '**différent**')
    c = j['compteurs_a_1_fil']
    vals['LIDAR_K10_REPLIS'] = ('`meb_fallbacks` %d ; `level_exact` %d ; `jump_exact` %d' % (
        c['meb_fallbacks'], sum(c['replis'][x]['level_exact'] for x in c['replis']), sum(c['replis'][x]['jump_exact'] for x in c['replis'])))
if rows:
    vals['DET_K10_ROWS'] = rows
# ---- mutants
def mutant(name):
    d = B + 'mut/' + name + '/'
    gates = oracles = None
    killers = []
    for f in ('ctest.log', 'ctest_hors_oracles.log'):
        if os.path.exists(d + f):
            s = open(d + f).read()
            killers += re.findall(r'\d+ - (\S+) \((\w+)\)', s)
            passed = len(re.findall(r'Test +#\d+: \S+ \.+ +Passed', s))
            failed = len(re.findall(r'Test +#\d+: \S+ \.+\*\*\*', s))
            gates = (passed, failed)
    orc = d + 'oracles_rapides.json'
    j = None
    if os.path.exists(orc) and os.path.getsize(orc) > 20:
        s = open(orc).read()
        j = json.loads(s[:s.rindex('}') + 1])
    return gates, sorted({k[0] for k in killers}), j


M = {}
for name in sorted(os.listdir(B + 'mut')):
    if name.startswith('M') and os.path.isdir(B + 'mut/' + name):
        M[name[:3]] = mutant(name)
json.dump({k: (v[0], v[1], v[2]) for k, v in M.items()}, open(B + 'rapport/mutants_brut.json', 'w'), indent=1)
for k, (gates, killers, j) in M.items():
    if k == 'M01':
        continue
    if gates:
        hors = [x for x in killers if 'oracle' not in x]
        vals[k + '_GATES'] = 'vertes' if not hors and gates[0] >= 11 else ('**rouge** : ' + ', '.join('`%s`' % x for x in hors) if hors else 'inachevé')
    if j:
        cat, tow = j['catalogue'], j['tour']
        bits = []
        if cat['ecarts'] or cat['translation'] != 'conforme':
            bits.append('catalogue **rouge** (%d écarts sur %d dumps rejugés%s)' % (len(cat['ecarts']), cat['rejuges'], '' if cat['translation'] == 'conforme' else ' ; translation'))
        if tow['ecarts']:
            bits.append('tour **rouge** (%d écarts sur %d dumps rejugés)' % (len(tow['ecarts']), tow['rejuges']))
        if not bits:
            ident = cat['dumps_identiques'] + tow['dumps_identiques']
            tot = cat['entrees'] + tow['entrees']
            bits.append('vertes (%d dumps sur %d identiques à HEAD%s)' % (ident, tot, '' if ident == tot else ' ; %d rejugés, 0 écart' % (cat['rejuges'] + tow['rejuges'])))
        vals[k + '_ORACLES'] = ' ; '.join(bits)
    if gates and j:
        dead = vals[k + '_GATES'] != 'vertes' or 'rouge' in vals[k + '_ORACLES']
        vals[k + '_VERDICT'] = 'tué' if dead else '**survit**'
vals['M09_EFFET'] = ('uniforme 8 000, K = 5 : 11 boules perdues sur 599 711, tour publiée `ok` avec 274 143 nœuds à l\'ordre 5 '
                     'au lieu de 274 144 ; `lidar00` : 1 boule perdue, statut `ok` ; Euler à kcat = 7 inchangé ; seule '
                     'la restriction cat(5) contre cat(7) le voit à l\'échelle (11 boules q2, p = 4)')
vals['M10_EFFET'] = ('uniforme 8 000 : 90 boules perdues à K = 5 ; Euler à kcat = 7 : 20, −2, 8, −1, −9 au lieu de 1 ; tour : '
                     '`invariant_violated / census_mismatch` (code 3) à 8 000 points et sur `lidar00`')
m8 = M.get('M08')
if m8 and m8[0]:
    killed = [x for x in m8[1] if 'oracle' not in x]
    if killed:
        vals['M08_RESULT'] = ('le correctif retiré (mutant M08, § 5.5), la porte tombe sur cette machine (%s) : la scène régénérée '
                              'porte donc encore la collision ici. Rien ne le garantit sur une autre version de numpy ou une '
                              'autre machine, et la porte ne le dirait pas.' % ', '.join('`%s`' % x for x in killed))
    elif m8[0][0] >= 11:
        vals['M08_RESULT'] = ('le correctif retiré (mutant M08, § 5.5), les 11 portes hors oracles restent **vertes** : la scène '
                              'régénérée sur cette machine ne porte plus la collision qu\'elle devait garder, et la porte '
                              'de régression est verte par vacuité.')
vals['R2_PORTE'] = ('J\'ai aussi rejoué la porte de tour de l\'arbre R2 sur son propre binaire (`journaux/juge_r2_porte_complete.txt`) : '
                    'code 0 en 37 s (le juge de HEAD, plus faible, en demande 216, faute de cache des miniboules), '
                    '`tower_oracle_checks 73 fails 0 cuts 23444`, 12 574 bijections, 3 978 verticales jugées dont 2 905 sans '
                    'point entré, 1 512 mutants de dump tués sur 1 512, à 1, 2 et 4 fils. Et les dumps par défaut de '
                    '`mhgp10_tower` sont identiques octet pour octet entre HEAD et R2 sur les 73 entrées de la porte '
                    '(`scripts/head_egale_r2.py`). **Sur ces 73 entrées, la forêt, les attaches `core` et les verticales '
                    'publiées par HEAD sont donc celles que le juge à témoins accepte** : le défaut est dans la porte du '
                    'dépôt, pas dans la tour.')
surv = ['M01'] + [k for k in sorted(M) if k != 'M01' and vals.get(k + '_VERDICT') == '**survit**']
dead = [k for k in sorted(M) if vals.get(k + '_VERDICT') == 'tué']
noms = {'M01': 'attache `core` stricte', 'M02': 'rang de l\'attache `cover`', 'M03': 'image verticale des naissances',
        'M04': 'refus de K ≥ 10', 'M06': 'exposant z ignoré', 'M08': 'collision de niveaux non corrigée',
        'M09': 'boule rare perdue en dernière couche', 'M10': 'boule rare perdue en toute couche',
        'M05': 'seuil de masse strict', 'M07': 'admission q = 4 décalée'}
vals['MUTANTS_SYNTHESE'] = (
    'Sur dix mutants, **%d survivent aux 13 portes** : %s. %d sont tués : %s. Lecture : '
    '(i) la convention d\'attache à l\'égalité, les verticales, l\'ordre 10 et l\'exposant z n\'ont aucune porte ; '
    '(ii) les deux oracles ne voient rien de ce qui dépend de la magnitude (M09 et M10 y rendent des dumps identiques à '
    'HEAD) ; (iii) les portes de régression à 2 000 et 8 000 points tuent ce qui déclenche un garde-fou interne '
    '(M07 : `root_count` ; M10 : `census_mismatch`), et M09, qui publie `ok` partout, n\'est tué que par ricochet : '
    '4 contrôles sur 136 de `mhgp10_regression_batch_equivalence`, à K = 1, parce que cette porte compare le catalogue '
    'd\'ordre K à la restriction d\'un catalogue d\'ordre 9 — c\'est, au niveau des étiquettes et à 2 000 points, '
    'l\'invariant de restriction qu\'il faut promouvoir en porte d\'échelle ; (iv) la tête n\'a qu\'un '
    'seul juge, scikit-learn sur le témoin (M05), et il ne voit pas z. M06 change 32 723 étiquettes sur 39 885 et le '
    'nombre d\'amas (66 contre 48) sur `lidar00` à K = 5, mcs = 200, z = 3, entrée `cover`.' % (
        len(surv), ', '.join('%s (%s)' % (k, noms[k]) for k in surv), len(dead),
        ', '.join('%s (%s)' % (k, noms[k]) for k in dead)))
json.dump(vals, open(B + 'rapport/valeurs.json', 'w'), indent=1, ensure_ascii=False)
print(json.dumps({k: v for k, v in vals.items() if k.startswith('M') or k.startswith('K10') or k.startswith('ASAN') or k.startswith('DET_K10')}, indent=1, ensure_ascii=False))
