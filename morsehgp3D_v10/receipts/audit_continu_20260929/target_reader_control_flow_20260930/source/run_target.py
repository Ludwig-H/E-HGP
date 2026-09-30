#!/usr/bin/env python3
"""Juge des fixtures cibles : chaque regle candidate contre chaque cible de chaque fixture, variantes comprises.

Usage :
  python3 -B run_target.py FIXTURE_OU_DOSSIER [...] [--out RAPPORT.json] [--table TABLE.txt]
                           [--regles r1,r2,...] [--gamma auto|non|force] [--gamma-budget S] [--explorer]

Une fixture est un fichier JSON (un objet, une liste d'objets, ou {"fixtures": [...]}) ou un module Python qui
definit FIXTURES (liste), FIXTURE (objet) ou fixtures() ; un dossier est parcouru (fichiers *.json et *.py,
tries). Format : voir README.md (name, K, points nommes, intent, target, tolerance, variants).

Pour chaque fixture et chaque variante (la base d'abord) : export natif FULL_K, recoupe Gamma de reference si le
cout le permet, compatibilite de chaque cible avec FULL (chaque bloc attendu doit etre contenu dans un amas discret
propre a chaque rayon de l'intervalle), puis verdict de chaque regle sur chaque cible : passe, ou premier intervalle
viole, rayon exact du contre-exemple, blocs observes et attendus.

Codes : 0 le juge a tourne (verdicts passe/echec dans le rapport) ; 2 refus (fixture invalide, export refuse) ;
3 invariant viole (oracle Gamma, laminarite, exclusivite ou recoupe en desaccord). Rapport JSON et table texte
ecrits dans tous les cas. Exact partout (Fraction, QS) ; comportement identique sous python3 -O.
"""
import argparse
import hashlib
import importlib.util
import json
import os
import sys
import time

sys.dont_write_bytecode = True
ICI = os.path.dirname(os.path.abspath(__file__))
if ICI not in sys.path:
    sys.path.insert(0, ICI)
import regles as RG  # noqa: E402
from regles import Rayon, RegleErreur, Incoherence, exiger  # noqa: E402

ALIAS = {
    'nom': 'name', 'intention': 'intent', 'cibles': 'target', 'cible': 'target', 'targets': 'target',
    'variantes': 'variants', 'libres': 'free', 'blocs': 'blocks', 'bornes': 'bounds', 'intervalle': 'interval',
    'deplacements': 'displacements', 'justification': 'why', 'pourquoi': 'why',
}


def _cles(d):
    out = {}
    for k, v in d.items():
        k2 = ALIAS.get(k, k)
        if k2 in out:
            raise RegleErreur('cle en double (alias) : %s' % k)
        out[k2] = v
    return out


# ------------------------------------------------------------------ chargement des fixtures

def charger(chemin):
    """Liste de (fixture brute, source) depuis un fichier ou un dossier."""
    if os.path.isdir(chemin):
        out = []
        for f in sorted(os.listdir(chemin)):
            if f.endswith('.json') or (f.endswith('.py') and not f.startswith('_')):
                out += charger(os.path.join(chemin, f))
        return out
    exiger(os.path.exists(chemin), 'fixture introuvable : %s' % chemin)
    if chemin.endswith('.json'):
        with open(chemin) as f:
            data = json.load(f)
        if isinstance(data, dict) and 'fixtures' in data:
            data = data['fixtures']
        if isinstance(data, dict):
            data = [data]
        exiger(isinstance(data, list), '%s : objet ou liste attendu' % chemin)
        return [(d, chemin) for d in data]
    if chemin.endswith('.py'):
        nom = 'fixture_cible_%s' % hashlib.sha256(os.path.abspath(chemin).encode()).hexdigest()[:12]
        spec = importlib.util.spec_from_file_location(nom, chemin)
        mod = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mod)
        if hasattr(mod, 'FIXTURES'):
            data = list(mod.FIXTURES)
        elif hasattr(mod, 'FIXTURE'):
            data = [mod.FIXTURE]
        elif hasattr(mod, 'fixtures'):
            data = list(mod.fixtures())
        else:
            return []
        return [(d, chemin) for d in data]
    raise RegleErreur('extension non reconnue : %s' % chemin)


def lire_intervalle(spec):
    """Intervalle exact : {'r': [a, b]} (rayons rationnels) ou {'r2': [a, b]} (rayons carres), 'bounds' parmi
    '[]' (defaut), '[)', '(]', '()' ; b peut valoir 'inf'."""
    spec = _cles(spec)
    exiger(('r' in spec) != ('r2' in spec), 'intervalle : donner r ou r2 (exactement un)')
    key = 'r' if 'r' in spec else 'r2'
    ab = spec[key]
    exiger(isinstance(ab, (list, tuple)) and len(ab) == 2, 'intervalle : deux bornes attendues')
    mk = Rayon.rationnel if key == 'r' else Rayon.niveau
    a = mk(ab[0])
    b = None if (isinstance(ab[1], str) and ab[1].strip() in ('inf', '+inf', 'infini')) else mk(ab[1])
    bounds = spec.get('bounds', '[]')
    exiger(bounds in ('[]', '[)', '(]', '()'), 'bornes invalides : %r' % bounds)
    fg, fd = bounds[0] == '[', bounds[1] == ']'
    if b is not None:
        c = a.cmp(b)
        exiger(c < 0 or (c == 0 and fg and fd), 'intervalle vide ou renverse')
    else:
        fd = False
    texte = '%s%s, %s%s' % (bounds[0], _txt(key, ab[0]), 'inf' if b is None else _txt(key, ab[1]), bounds[1])
    return {'a': a, 'b': b, 'ferme_gauche': fg, 'ferme_droite': fd, 'texte': texte}


def _txt(key, v):
    return '%s=%s' % (key, v)


def lire_cible(c, noms, tol_fix, libres_fix):
    if isinstance(c, (list, tuple)):
        exiger(len(c) in (2, 3), 'cible en liste : [intervalle, blocs] ou [intervalle, blocs, tolerance]')
        iv = c[0] if isinstance(c[0], dict) else {'r': list(c[0])}
        c = {'interval': iv, 'blocks': c[1], 'tolerance': c[2] if len(c) == 3 else []}
    exiger(isinstance(c, dict), 'cible : objet attendu')
    c = _cles(c)
    iv = c.get('interval')
    if iv is None:
        iv = {k: c[k] for k in ('r', 'r2', 'bounds', 'bornes') if k in c}
    inter = lire_intervalle(iv)
    blocks = c.get('blocks')
    exiger(isinstance(blocks, list), 'cible : liste de blocs attendue')
    vus = set()
    blocs = []
    for b in blocks:
        exiger(isinstance(b, list) and b, 'bloc vide ou invalide')
        for x in b:
            exiger(x in noms, 'bloc : point inconnu %r' % (x,))
            exiger(x not in vus, 'point %r dans deux blocs attendus' % (x,))
            vus.add(x)
        blocs.append(list(b))
    tol = set(tol_fix) | set(c.get('tolerance', []))
    lib = set(libres_fix) | set(c.get('free', []))
    for x in tol | lib:
        exiger(x in noms, 'tolerance/libres : point inconnu %r' % (x,))
    return {'intervalle': inter, 'blocs': blocs, 'tolerance': sorted(tol, key=noms.index),
            'libres': sorted(lib, key=noms.index), 'pourquoi': c.get('why', '')}


def normaliser(brut, source):
    """Fixture normalisee : name, K, points [(nom, xyz)], intent, target, variants [(nom, points, target)]."""
    exiger(isinstance(brut, dict), '%s : fixture objet attendu' % source)
    d = _cles(brut)
    name = d.get('name')
    exiger(isinstance(name, str) and name, '%s : name manquant' % source)
    K = d.get('K')
    exiger(isinstance(K, int) and not isinstance(K, bool), '%s : K entier attendu' % name)
    pts = RG.normaliser_points(d.get('points', {}))
    noms = [nm for nm, _ in pts]
    intent = d.get('intent', '')
    exiger(isinstance(intent, str) and intent.strip(), '%s : intent (texte francais) obligatoire' % name)
    tol_fix = list(d.get('tolerance', []))
    lib_fix = list(d.get('free', []))
    cibles_brutes = d.get('target')
    exiger(isinstance(cibles_brutes, list) and cibles_brutes, '%s : target (liste non vide) obligatoire' % name)
    cibles = [lire_cible(c, noms, tol_fix, lib_fix) for c in cibles_brutes]
    variantes = [{'nom': 'base', 'points': pts, 'cibles': cibles, 'deplacements': {}}]
    for v in d.get('variants', []):
        v = _cles(v)
        vn = v.get('name')
        exiger(isinstance(vn, str) and vn and vn != 'base', '%s : variante sans nom (ou nommee base)' % name)
        if 'points' in v:
            vp = RG.normaliser_points(v['points'])
            exiger([nm for nm, _ in vp] == noms, '%s/%s : memes noms, meme ordre exiges' % (name, vn))
            dep = {}
        else:
            dep = v.get('displacements', {})
            exiger(isinstance(dep, dict) and dep, '%s/%s : displacements (objet non vide) attendu' % (name, vn))
            coords = dict(pts)
            for x, dxyz in dep.items():
                exiger(x in coords, '%s/%s : point inconnu %r' % (name, vn, x))
                exiger(isinstance(dxyz, (list, tuple)) and len(dxyz) == 3 and
                       all(isinstance(c, int) and not isinstance(c, bool) for c in dxyz),
                       '%s/%s : deplacement entier (dx, dy, dz) attendu' % (name, vn))
                coords[x] = tuple(c + e for c, e in zip(coords[x], dxyz))
            vp = RG.normaliser_points([(nm, coords[nm]) for nm in noms])
        vc = cibles
        if 'target' in v:
            vc = [lire_cible(c, noms, tol_fix, lib_fix) for c in v['target']]
        variantes.append({'nom': vn, 'points': vp, 'cibles': vc, 'deplacements': dep})
    exiger(len({v['nom'] for v in variantes}) == len(variantes), '%s : noms de variantes dupliques' % name)
    return {'name': name, 'K': K, 'noms': noms, 'intent': intent, 'variantes': variantes, 'source': source}


# ------------------------------------------------------------------ jugement d'une partition

def conforme(part, cible, sc):
    """Partition (identifiants) contre blocs attendus. Regles : (1) un bloc observe (hors points libres) de deux
    points ou plus ne contient que des points d'un meme bloc attendu ; (2) les points non toleres d'un bloc attendu
    sont dans un meme bloc observe ; un point tolere est dans ce bloc ou seul (hors libres). Les points absents de
    tout bloc attendu doivent etre seuls (hors libres)."""
    lab = {}
    for j, b in enumerate(cible['blocs']):
        for x in b:
            lab[sc.index[x]] = j
    tol = {sc.index[x] for x in cible['tolerance']}
    lib = {sc.index[x] for x in cible['libres']}
    where = {}
    for i, blk in enumerate(part):
        for x in blk:
            where[x] = i
    for blk in part:
        core = [x for x in blk if x not in lib]
        if len(core) >= 2:
            labs = {lab.get(x) for x in core}
            if None in labs:
                seuls = [sc.noms[x] for x in core if lab.get(x) is None]
                return False, 'points attendus seuls mais regroupes : %s dans %s' % (seuls, [sc.noms[x] for x in core])
            if len(labs) > 1:
                return False, 'blocs attendus differents reunis : %s' % [sc.noms[x] for x in core]
    for j, b in enumerate(cible['blocs']):
        ids = [sc.index[x] for x in b]
        nt = [x for x in ids if x not in tol and x not in lib]
        if not nt:
            continue
        homes = {where[x] for x in nt}
        if len(homes) > 1:
            return False, 'bloc attendu %s scinde' % b
        home = homes.pop()
        for t in ids:
            if t in tol and t not in lib and where[t] != home:
                others = [y for y in part[where[t]] if y not in lib and y != t]
                if others:
                    return False, 'point tolere %s regroupe hors de son bloc' % sc.noms[t]
    return True, ''


def points_de_controle(h, inter):
    """Rayons ou evaluer la partition pour couvrir tout l'intervalle : la borne gauche (coupe fermee : etat sur
    [a, prochain evenement), qui couvre aussi ]a, ...) puis chaque rayon de changement dans ]a, b] (ou ]a, b[)."""
    a, b, fd = inter['a'], inter['b'], inter['ferme_droite']
    pts = [a]
    for e in h.rayons_de_changement():
        if e.cmp(a) <= 0:
            continue
        if b is not None:
            c = e.cmp(b)
            if c > 0 or (c == 0 and not fd):
                break
        pts.append(e)
    return pts


def juger(h, cible, sc):
    """Verdict d'une regle sur une cible : passe, premier viol, partitions observees (distinctes) sur l'intervalle."""
    obs = []
    viol = None
    for r in points_de_controle(h, cible['intervalle']):
        part = h.blocs_ids(r)
        if not obs or obs[-1][1] != part:
            obs.append((r, part))
        ok, raison = conforme(part, cible, sc)
        if not ok and viol is None:
            viol = {'rayon': r.json(), 'observes': sc.nommer(part), 'raison': raison}
    return {'passe': viol is None, 'viol': viol,
            'observes': [{'depuis': r.json(), 'blocs': sc.nommer(p), 'texte': format_blocs(sc.nommer(p))}
                         for r, p in obs[:8]], 'partitions_distinctes': len(obs)}


# ------------------------------------------------------------------ compatibilite d'une cible avec FULL

def _sdr(cands):
    """Representants distincts (couplage biparti par chemins augmentants) ; rend le couplage ou None."""
    match = {}

    def essayer(j, vus):
        for v in cands[j]:
            if v in vus:
                continue
            vus.add(v)
            if v not in match or essayer(match[v], vus):
                match[v] = j
                return True
        return False
    for j in range(len(cands)):
        if not essayer(j, set()):
            return None
    return {j: v for v, j in match.items()}


def compat_full(sc, cible):
    """Chaque bloc attendu (points non toleres, non libres, au moins deux) doit etre contenu dans l'amas discret
    X n delta_r(C) d'une composante vivante C, des composantes distinctes pour des blocs distincts, a chaque rayon
    de l'intervalle (theoreme T2 : c'est necessaire pour toute regle couvrante). Controle aux rayons ou la
    couverture change : borne gauche, fusions FULL et niveaux des temoins des points concernes."""
    inter = cible['intervalle']
    a, b, fd = inter['a'], inter['b'], inter['ferme_droite']
    exiger(a.b2 is not None, 'compatibilite : borne gauche rationnelle au carre requise')
    tol = {sc.index[x] for x in cible['tolerance']}
    lib = {sc.index[x] for x in cible['libres']}
    blocs = []
    for bl in cible['blocs']:
        nt = [sc.index[x] for x in bl if sc.index[x] not in tol and sc.index[x] not in lib]
        if len(nt) >= 2:
            blocs.append((bl, nt))
    if not blocs:
        return {'compatible': True, 'controles': 0}
    pts = sorted({p for _bl, nt in blocs for p in nt})
    # lemme L3 (memo d'ancrage) : Cov_x(r) = { anc_r(v) : v feuille couvrante de x, c(v) <= r }
    feuilles = {p: sc.covers[sc.site[p]].leaves for p in pts}
    lv = set(sc.fusions_full())
    for p in pts:
        lv |= {c for c, _v in feuilles[p]}
    radii = [a.b2]
    for lw in sorted(lv):
        if lw <= a.b2:
            continue
        if b is not None and (lw > b.b2 or (lw == b.b2 and not fd)):
            break
        radii.append(lw)
    f = sc.foret
    n_ctrl = 0
    for beta in radii:
        t = f.threshold(beta, True)
        cov = {p: {f.ancestor_lv(v, t) for c, v in feuilles[p] if c <= beta} for p in pts}
        cands = []
        for bl, nt in blocs:
            c = set.intersection(*(cov[p] for p in nt))
            if not c:
                return {'compatible': False, 'controles': n_ctrl, 'rayon': Rayon(b2=beta).json(),
                        'raison': 'aucune composante vivante ne couvre tout le bloc %s' % bl}
            cands.append(sorted(c))
        if _sdr(cands) is None:
            return {'compatible': False, 'controles': n_ctrl, 'rayon': Rayon(b2=beta).json(),
                    'raison': 'blocs forces dans une meme composante : %s' % [bl for bl, _nt in blocs]}
        n_ctrl += 1
    return {'compatible': True, 'controles': n_ctrl}


# ------------------------------------------------------------------ affichage

def format_blocs(part):
    """'ABC | DEF' (singletons omis) ; noms de plus d'un caractere : '{a,b} | {c,d}'."""
    grands = [b for b in part if len(b) >= 2]
    if not grands:
        return '(singletons)'
    court = all(len(x) == 1 for b in grands for x in b)
    return ' | '.join(''.join(b) if court else '{%s}' % ','.join(b) for b in grands)


def table_texte(rapport, regles):
    fx = [f for f in rapport['fixtures'] if f.get('statut') == 'juge']
    lignes = []
    lignes.append('Table regle x fixture (OK : toutes les variantes passent toutes les cibles ; k/m : variantes '
                  'reussies ; ! : cible incompatible avec FULL dans une variante)')
    codes = ['F%02d' % (i + 1) for i in range(len(fx))]
    w = max([len(r) for r in regles] + [6])
    lignes.append('%-*s | %s' % (w, 'regle', ' | '.join('%-5s' % c for c in codes)))
    lignes.append('-' * (w + 3 + 8 * len(codes)))
    for r in regles:
        cells = []
        for f in fx:
            vs = f['variantes']
            ok = sum(1 for v in vs if v['regles'].get(r, {}).get('passe'))
            cells.append('%-5s' % ('OK' if ok == len(vs) else '%d/%d' % (ok, len(vs))))
        lignes.append('%-*s | %s' % (w, r, ' | '.join(cells)))
    lignes.append('')
    for c, f in zip(codes, fx):
        bang = '' if all(ci['compatible'] for v in f['variantes'] for ci in v['compat_full']) else ' !'
        lignes.append('%s = %s (K=%d, n=%d, %d variante(s))%s'
                      % (c, f['name'], f['K'], f['n'], len(f['variantes']), bang))
    lignes.append('')
    lignes.append('Detail : partition observee sur chaque cible (texte : blocs non singletons).')
    for f in fx:
        for v in f['variantes']:
            lignes.append('== %s / %s (gamma : %s)' % (f['name'], v['nom'], v['gamma'].get('statut')))
            for k, ci in enumerate(v['cibles']):
                lignes.append('   cible %d %s attendu %s%s' % (k, ci['intervalle'], ci['attendu'],
                                                               '' if v['compat_full'][k]['compatible']
                                                               else '  [INCOMPATIBLE AVEC FULL : %s]'
                                                               % v['compat_full'][k]['raison']))
            for r in regles:
                res = v['regles'].get(r)
                if res is None:
                    continue
                txt = []
                for ci in res['cibles']:
                    obs = ' -> '.join('%s@%.6g' % (o['texte'], o['depuis']['r']) for o in ci['observes'])
                    txt.append(('ok ' if ci['passe'] else 'KO ') + obs)
                lignes.append('   %-*s %s' % (w, r, ' || '.join(txt)))
    for f in rapport['fixtures']:
        if f.get('statut') != 'juge':
            lignes.append('REFUS %s : %s' % (f.get('name', f.get('source')), f.get('erreur')))
    return '\n'.join(lignes) + '\n'


# ------------------------------------------------------------------ execution

def juger_fixture(fx, regles, gamma, budget, cnt):
    res = {'name': fx['name'], 'K': fx['K'], 'n': len(fx['noms']), 'source': fx['source'], 'intent': fx['intent'],
           'points': fx['noms'], 'variantes': [], 'statut': 'juge'}
    for var in fx['variantes']:
        t0 = time.time()
        sc = RG.Scene(var['points'], fx['K'], nom='%s/%s' % (fx['name'], var['nom']), gamma=gamma,
                      gamma_budget=budget)
        vres = {'nom': var['nom'], 'deplacements': var['deplacements'],
                'coordonnees': {nm: list(xyz) for nm, xyz in var['points']},
                'gamma': sc.gamma, 'full': sc.resume_full(), 'cibles': [], 'compat_full': [], 'regles': {}}
        for ci in var['cibles']:
            vres['cibles'].append({'intervalle': ci['intervalle']['texte'], 'attendu': format_blocs(ci['blocs']),
                                   'blocs': ci['blocs'], 'tolerance': ci['tolerance'], 'libres': ci['libres'],
                                   'pourquoi': ci['pourquoi']})
            comp = compat_full(sc, ci)
            cnt['compat'] += comp['controles']
            vres['compat_full'].append(comp)
        for r in regles:
            h = sc.regle(r)
            verdicts = [juger(h, ci, sc) for ci in var['cibles']]
            cnt['jugements'] += len(verdicts)
            premier = next(({'cible': k, 'intervalle': var['cibles'][k]['intervalle']['texte'],
                             'attendu': format_blocs(var['cibles'][k]['blocs']), **vd['viol']}
                            for k, vd in enumerate(verdicts) if not vd['passe']), None)
            vres['regles'][r] = {'passe': all(vd['passe'] for vd in verdicts), 'premier_viol': premier,
                                 'cibles': verdicts,
                                 'dates': {sc.noms[p]: [float(h.dates[p]), h.dates[p].texte()] for p in range(sc.n)}}
        vres['controles'] = sc.controles
        vres['recoupes'] = sc.recoupes
        vres['secondes'] = round(time.time() - t0, 3)
        res['variantes'].append(vres)
    res['passe'] = {r: all(v['regles'][r]['passe'] for v in res['variantes']) for r in regles}
    return res


def semantique(rapport):
    """Partie du rapport independante des durees, chemins et caches (comparaison normal / -O)."""
    out = []
    for f in rapport['fixtures']:
        if f.get('statut') != 'juge':
            out.append({'refus': f.get('erreur')})
            continue
        vs = []
        for v in f['variantes']:
            g = {k: v['gamma'].get(k) for k in ('statut', 'niveaux', 'controles')}
            vs.append({'nom': v['nom'], 'gamma': g, 'compat': v['compat_full'],
                       'regles': {r: {'passe': x['passe'], 'premier_viol': x['premier_viol'],
                                      'observes': [c['observes'] for c in x['cibles']]}
                                  for r, x in v['regles'].items()},
                       'recoupes': v['recoupes'],
                       'controles': v['controles']})
        out.append({'name': f['name'], 'variantes': vs})
    return out


def main(argv):
    ap = argparse.ArgumentParser(description='Juge des fixtures cibles (toutes les regles, toutes les variantes).')
    ap.add_argument('chemins', nargs='+')
    ap.add_argument('--out', help='rapport JSON (defaut : SCRATCH/rapports/rapport_<horodatage>.json)')
    ap.add_argument('--table', help='table texte (defaut : a cote du rapport, .txt)')
    ap.add_argument('--regles', help='liste de regles separees par des virgules (defaut : toutes)')
    ap.add_argument('--gamma', choices=('auto', 'non', 'force'), default='auto')
    ap.add_argument('--gamma-budget', type=float, default=RG.GAMMA_BUDGET)
    ap.add_argument('--explorer', action='store_true', help='afficher FULL et les evenements de chaque regle')
    ap.add_argument('--quiet', action='store_true')
    a = ap.parse_args(argv)
    regles = RG.NOMS_REGLES if not a.regles else [r.strip() for r in a.regles.split(',') if r.strip()]
    for r in regles:
        if r not in RG.NOMS_REGLES:
            print('refus : regle inconnue %s' % r, file=sys.stderr)
            return 2
    t0 = time.time()
    avant = RG.empreintes()
    rapport = {'cadre': RG.CADRE, 'argv': ['run_target.py'] + list(argv), 'optimize': sys.flags.optimize,
               'regles': [{'nom': n, 'famille': f, 'definition': d} for n, f, d in RG.REGLES if n in regles],
               'fixtures': []}
    cnt = {'jugements': 0, 'compat': 0}
    code = 0
    bruts = []
    for c in a.chemins:
        try:
            bruts += charger(c)
        except (RegleErreur, ValueError, SyntaxError, OSError) as err:
            rapport['fixtures'].append({'source': c, 'statut': 'refus', 'erreur': str(err)})
            code = 2
    fichiers = sorted({src for _b, src in bruts})
    rapport['fichiers'] = {f: RG.sha_fichier(f) for f in fichiers}
    for brut, src in bruts:
        try:
            fx = normaliser(brut, src)
        except RegleErreur as err:
            rapport['fixtures'].append({'source': src, 'name': brut.get('name', brut.get('nom'))
                                        if isinstance(brut, dict) else None, 'statut': 'refus', 'erreur': str(err)})
            code = max(code, 2)
            continue
        if a.explorer:
            explorer(fx, regles, a.gamma, a.gamma_budget)
            continue
        try:
            res = juger_fixture(fx, regles, a.gamma, a.gamma_budget, cnt)
        except RegleErreur as err:
            res = {'source': src, 'name': fx['name'], 'statut': 'refus', 'erreur': str(err)}
            code = max(code, 2)
        except Incoherence as err:
            res = {'source': src, 'name': fx['name'], 'statut': 'incoherence', 'erreur': str(err)}
            code = 3
        rapport['fixtures'].append(res)
        if not a.quiet and res.get('statut') == 'juge':
            ok = [r for r in regles if res['passe'][r]]
            print('%-40s passent : %s' % (fx['name'], ', '.join(ok) if ok else '(aucune)'), flush=True)
        elif not a.quiet:
            print('%-40s %s : %s' % (fx['name'], res['statut'], res['erreur']), flush=True)
    if a.explorer:
        return code
    apres = RG.empreintes()
    rapport['sources_avant'] = avant
    rapport['sources_apres'] = apres
    if avant != apres:
        rapport['avertissement'] = 'une source a change pendant le jugement'
        code = max(code, 3)
    rapport['compteurs'] = cnt
    rapport['secondes'] = round(time.time() - t0, 2)
    sem = semantique(rapport)
    rapport['empreinte_semantique'] = hashlib.sha256(json.dumps(sem, sort_keys=True).encode()).hexdigest()
    rapport['code'] = code
    out = a.out
    if not out:
        d = os.path.join(RG.SCRATCH, 'rapports')
        os.makedirs(d, exist_ok=True)
        out = os.path.join(d, 'rapport_%s_%d.json' % (time.strftime('%Y%m%dT%H%M%S'), os.getpid()))
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    with open(out, 'w') as f:
        json.dump(rapport, f, indent=1, ensure_ascii=False, sort_keys=True)
        f.write('\n')
    table = table_texte(rapport, regles)
    tpath = a.table or (os.path.splitext(out)[0] + '.txt')
    with open(tpath, 'w') as f:
        f.write(table)
    if not a.quiet:
        print(table)
    print(json.dumps({'code': code, 'rapport': out, 'table': tpath, 'fixtures': len(rapport['fixtures']),
                      'empreinte_semantique': rapport['empreinte_semantique'], 'secondes': rapport['secondes']}))
    return code


def explorer(fx, regles, gamma, budget):
    """Affichage lisible : noeuds de FULL_K (amas discrets) et suite des partitions de chaque regle."""
    for var in fx['variantes']:
        sc = RG.Scene(var['points'], fx['K'], nom='%s/%s' % (fx['name'], var['nom']), gamma=gamma,
                      gamma_budget=budget)
        print('== %s / %s : K=%d, n=%d, gamma %s' % (fx['name'], var['nom'], fx['K'], sc.n, sc.gamma.get('statut')))
        print('FULL_%d : %d noeuds' % (fx['K'], len(sc.foret)))
        for nd in sc.noeuds_full():
            amas = ''.join(nd['amas']) if all(len(x) == 1 for x in nd['amas']) else nd['amas']
            print('  r=%-14.6f r2=%-24s %-9s n%-4d enfants=%s amas=%s'
                  % (nd['r'], nd['r2'], nd['type'], nd['noeud'], nd['enfants'], amas))
        for nm in sc.noms:
            print('  point %-6s alpha=%.6f (r2=%s)  d_K=%.6f (r2=%s)'
                  % (nm, float(sc.alpha2(nm)) ** 0.5, sc.alpha2(nm), float(sc.dK2(nm)) ** 0.5, sc.dK2(nm)))
        for r in regles:
            h = sc.regle(r)
            evs = h.evenements()
            print('  %s' % r)
            for ev in evs:
                print('     r=%-14.6f %-30s %s' % (ev['rayon']['r'], ev['rayon']['exact'], format_blocs(ev['blocs'])))


if __name__ == '__main__':
    try:
        sys.exit(main(sys.argv[1:]))
    except RegleErreur as err:
        print('refus : %s' % err, file=sys.stderr)
        sys.exit(2)
    except Incoherence as err:
        print('incoherence : %s' % err, file=sys.stderr)
        sys.exit(3)
