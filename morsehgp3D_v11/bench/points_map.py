#!/usr/bin/env python3
"""Etiquettes MAP (maximum a posteriori aux vrais parametres) des scenes synthetiques du generateur epingle (E1, v11).

    python3 bench/points_map.py --self-test        # porte de rejeu, normalisation des densites ; code 0 ou 3
    python3 bench/points_map.py --spec '{"family": "shells", "groups": 8, "level": "hard", "n": 8000,
                                         "noise_fraction": 0.05, "seed": 1}'   # niveau de Bayes d'une scene

Cadre : phase=exploration_v11_hors_registre, backend=cpu_reference, public_status=not_claimed. Specification :
build/v11-points-select/juge/SPEC.md, par. 3.1 (MAP : niveau de Bayes, seconde verite, regret) et par. 3.2.

PROVENANCE (port explicite, aucun import du code prive build/v11-points-select/) :
  - build/v11-points-select/mesure/carte_map.py
      sha256 4e85730afce7b75bc47c96fe7c2b7c484ed412fcd1b1f0dcb6c9346bd6fafa20
    rejeu du generateur avec capture des parametres, modele de densite par classe, MAP en log ; inchange sur le
    fond. Ajouts : le generateur est charge par chemin depuis le recu et son sha256 est verifie avant execution ; la
    restriction aux sites gardes compare aussi la grille entiere (pas seulement les etiquettes et le nombre).
  - build/v11-points-select/mesure/controle_map.py (controle de normalisation par Monte-Carlo d'importance et
    porte de rejeu sur les huit familles), repris dans self_test.

Generateur epingle, lu comme une donnee : morsehgp3D_v11/receipts/full_points_20261003/experiment/vendor_scenes.py,
sha256 61ea9abc511726c2a9ff1e066f7603d6bef195a7c5356a2d2048c28229608f00 (identique octet pour octet a
`git show origin/main:` de ce chemin au commit 8f68622b2, et a la copie build/v11-points-select/mesure/
vendor_scenes_v10_pin.py du prototype). Il est lu en octets, son empreinte comparee a l'epingle, puis compile et
execute depuis ces memes octets (aucun bytecode ecrit dans le recu) ; toute autre empreinte est refusee.

Porte de rejeu : `replay` refait les memes appels au generateur aleatoire dans le meme ordre en capturant les
parametres tires (centres, rotations, echelles, rayons, directions, boite du bruit) et REFUSE si l'empreinte sha256
des points rejoues differe de celle de generate(spec) sur la meme machine. L'empreinte flottante depend du BLAS
(80 scenes sur 128 differaient entre la VM et le codespace, rapport `mesure` par. 3.1) : le MAP se calcule donc une
fois, a la generation, sur les memes flottants, et la VM consomme des fichiers geles (points_scenes_freeze.py).

Modele de densite par classe (etiquette c de la verite ; bruit = -1), exact pour chaque famille :
  gaussiennes (spherical, anisotropic, heteroscedastic, unbalanced) : N(mu_j, T_j T_j^T), T_j = R_j diag(echelle) ;
  hierarchical : melange des trois sous-amas du groupe j (poids = effectifs), N(mu_jc, inner^2 I) ;
  shells : x = mu + u s, u uniforme sur la sphere, s ~ N(R_j, 0,15^2) ; densite
           [phi((rho - R)/sig) + phi((-rho - R)/sig)] / (sig 4 pi rho^2), rho = |x - mu| (le pli s < 0 compris) ;
  filaments : segment uniforme de longueur 3 (+/- 1,5 autour de mu_j) convolue par N(0, 0,18^2 I) ;
  bridge : groupes N(mu_j, I) ; classe bruit = ponts (segments mu_j -> mu_j+1 convolues par N(0, 0,25^2 I)) +
           bruit uniforme ;
  bruit uniforme : 1 / volume de la boite englobante des groupes elargie de 10 % (celle du generateur).
A priori : effectifs exacts du generateur (n_c / n), le plan etant a composition fixe (modele marginal declare ;
M4 n'y vaut qu'approximativement). MAP = argmax_c n_c p_c(x), en log, sur les coordonnees flottantes avant
quantification, puis restreint aux sites gardes par quantize18 (doublons retires), comme les methodes. Le MAP n'est
jamais un plafond (fixture F7 de points_flat_metrics) ni un critere de reglage.
"""
import argparse
import hashlib
import json
import math
import os
import sys
import types

import numpy as np
from scipy.special import log_ndtr, logsumexp

HERE = os.path.dirname(os.path.abspath(__file__))
GENERATOR_PATH = os.path.normpath(os.path.join(HERE, '..', 'receipts', 'full_points_20261003', 'experiment',
                                               'vendor_scenes.py'))
GENERATOR_SHA256 = '61ea9abc511726c2a9ff1e066f7603d6bef195a7c5356a2d2048c28229608f00'
_VS = None


def need(ok, why):
    if not ok:
        raise ValueError(why)


def generator():
    """Module du generateur epingle, execute depuis les octets dont l'empreinte a ete verifiee."""
    global _VS
    if _VS is None:
        with open(GENERATOR_PATH, 'rb') as stream:
            source = stream.read()
        got = hashlib.sha256(source).hexdigest()
        need(got == GENERATOR_SHA256, 'generateur : empreinte %s, epingle %s' % (got, GENERATOR_SHA256))
        module = types.ModuleType('vendor_scenes_pin')
        module.__file__ = GENERATOR_PATH
        exec(compile(source, GENERATOR_PATH, 'exec'), module.__dict__)
        _VS = module
    return _VS


def _gaussian(family, groups, sizes, delta, rng):
    vs = generator()
    middles = delta * vs.centres(groups)
    points, labels, comps = [], [], []
    for j in range(groups):
        if family == 'anisotropic':
            scale = np.diag([2.0, 1.0, 0.35])
        elif family == 'heteroscedastic':
            scale = np.diag([(0.4, 1.0, 2.5)[j % 3]] * 3)
        else:
            scale = np.eye(3)
        transform = vs._rotation(rng) @ scale
        points.append(middles[j] + rng.standard_normal((sizes[j], 3)) @ transform.T)
        labels.append(np.full(sizes[j], j, dtype=np.int64))
        comps.append(dict(kind='gauss', label=j, weight=int(sizes[j]), mu=middles[j].copy(),
                          cov=transform @ transform.T))
    return np.vstack(points), np.concatenate(labels), comps


def _shells(groups, sizes, delta, rng):
    vs = generator()
    radii = np.array([vs.shell_radius(size) for size in sizes], dtype=np.float64)
    middles = (2.0 * float(radii.max()) + delta) * vs.centres(groups)
    points, labels, comps = [], [], []
    for j in range(groups):
        direction = rng.standard_normal((sizes[j], 3))
        direction /= np.linalg.norm(direction, axis=1, keepdims=True)
        thickness = vs.SHELL_THICKNESS * rng.standard_normal((sizes[j], 1))
        points.append(middles[j] + direction * (radii[j] + thickness))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
        comps.append(dict(kind='shell', label=j, weight=int(sizes[j]), mu=middles[j].copy(), radius=float(radii[j]),
                          sigma=vs.SHELL_THICKNESS))
    return np.vstack(points), np.concatenate(labels), comps


def _bridge(groups, sizes, delta, rng, bridge_total):
    vs = generator()
    middles = delta * vs.centres(groups)
    points, labels, comps = [], [], []
    for j in range(groups):
        points.append(middles[j] + rng.standard_normal((sizes[j], 3)))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
        comps.append(dict(kind='gauss', label=j, weight=int(sizes[j]), mu=middles[j].copy(), cov=np.eye(3)))
    pairs = max(1, groups // 2)
    per_pair = bridge_total // pairs
    for index, j in enumerate(range(0, groups - 1, 2)):
        share = per_pair + (bridge_total - per_pair * pairs if index == 0 else 0)
        t = rng.random((share, 1))
        line = middles[j] + t * (middles[j + 1] - middles[j])
        points.append(line + 0.25 * rng.standard_normal((share, 3)))
        labels.append(np.full(share, -1, dtype=np.int64))
        comps.append(dict(kind='segment', label=-1, weight=int(share), a=middles[j].copy(),
                          b=middles[j + 1].copy(), sigma=0.25))
    return np.vstack(points), np.concatenate(labels), comps


def _hierarchical(groups, sizes, delta, rng):
    vs = generator()
    middles = vs.HIERARCHICAL_GROUP_GAP * vs.centres(groups)
    inner = 1.0 / math.sqrt(1.0 + (delta ** 2) / 9.0)
    points, labels, comps = [], [], []
    for j in range(groups):
        share = vs._sizes(int(sizes[j]), 3, 'equal', rng)
        local = vs._rotation(rng) @ ((delta * inner) * vs.centres(3)).T
        for c in range(3):
            points.append(middles[j] + local[:, c] + inner * rng.standard_normal((share[c], 3)))
            labels.append(np.full(share[c], j, dtype=np.int64))
            comps.append(dict(kind='gauss', label=j, weight=int(share[c]), mu=middles[j] + local[:, c],
                              cov=(inner ** 2) * np.eye(3)))
    return np.vstack(points), np.concatenate(labels), comps


def _filaments(groups, sizes, delta, rng):
    vs = generator()
    middles = delta * vs.centres(groups)
    points, labels, comps = [], [], []
    for j in range(groups):
        direction = vs._rotation(rng)[:, 0]
        t = (rng.random((sizes[j], 1)) - 0.5) * vs.FILAMENT_LENGTH
        points.append(middles[j] + t * direction + 0.18 * rng.standard_normal((sizes[j], 3)))
        labels.append(np.full(sizes[j], j, dtype=np.int64))
        half = 0.5 * vs.FILAMENT_LENGTH * direction
        comps.append(dict(kind='segment', label=j, weight=int(sizes[j]), a=middles[j] - half, b=middles[j] + half,
                          sigma=0.18))
    return np.vstack(points), np.concatenate(labels), comps


def replay(spec):
    """Meme sortie que generate(spec) plus la liste des composantes du modele. Porte de rejeu : refus (ValueError)
    si l'empreinte sha256 des points rejoues differe de celle du generateur epingle sur cette machine."""
    vs = generator()
    spec = vs.validate_spec(spec)
    family, groups, n = spec['family'], spec['groups'], spec['n']
    delta = vs.SEPARATION[family][spec['level']]
    rng = np.random.default_rng(spec['seed'])
    noise_count = int(round(spec['noise_fraction'] * n))
    bridge_total = vs.bridge_budget(n - noise_count, groups) if family == 'bridge' else 0
    sizes = vs._sizes(n - noise_count - bridge_total, groups, family, rng)
    if family in ('spherical', 'anisotropic', 'heteroscedastic', 'unbalanced'):
        points, labels, comps = _gaussian(family, groups, sizes, delta, rng)
    elif family == 'shells':
        points, labels, comps = _shells(groups, sizes, delta, rng)
    elif family == 'bridge':
        points, labels, comps = _bridge(groups, sizes, delta, rng, bridge_total)
    elif family == 'hierarchical':
        points, labels, comps = _hierarchical(groups, sizes, delta, rng)
    else:
        points, labels, comps = _filaments(groups, sizes, delta, rng)
    if noise_count:
        low, high = points.min(axis=0), points.max(axis=0)
        margin = 0.1 * (high - low)
        box_low, box_high = low - margin, high + margin
        points = np.vstack([points, rng.uniform(box_low, box_high, size=(noise_count, 3))])
        labels = np.concatenate([labels, np.full(noise_count, -1, dtype=np.int64)])
        comps.append(dict(kind='uniform', label=-1, weight=noise_count, low=box_low, high=box_high))
    order = rng.permutation(len(points))
    points, labels = points[order], labels[order]
    digest = hashlib.sha256(np.ascontiguousarray(points, dtype='<f8').tobytes()).hexdigest()
    ref_points, ref_labels, meta = vs.generate(spec)
    need(digest == meta['digest'], 'porte de rejeu : empreinte rejouee %s, generateur %s, spec %s'
         % (digest, meta['digest'], json.dumps(spec, sort_keys=True)))
    need(np.array_equal(labels, ref_labels), 'porte de rejeu : etiquettes differentes')
    return points, labels, comps, meta


def _log_gauss(x, mu, cov):
    d = x - mu
    chol = np.linalg.cholesky(cov)
    z = np.linalg.solve(chol, d.T)
    logdet = 2.0 * np.log(np.diag(chol)).sum()
    return -0.5 * (z * z).sum(axis=0) - 0.5 * (3 * math.log(2 * math.pi) + logdet)


def _log_segment(x, a, b, sigma):
    """log de (1/L) int_0^L N(x; a + s e, sigma^2 I) ds
    = log phi_2(d_perp) + log[(Phi((L - s)/sig) - Phi(-s/sig))/L]."""
    e = b - a
    length = float(np.linalg.norm(e))
    e = e / length
    d = x - a
    s = d @ e
    perp2 = np.maximum((d * d).sum(axis=1) - s * s, 0.0)
    log_phi2 = -perp2 / (2 * sigma ** 2) - math.log(2 * math.pi * sigma ** 2)
    u, lo = (length - s) / sigma, -s / sigma  # u > lo ; log(Phi(u) - Phi(lo)) sans annulation
    upper = lo > 0  # les deux dans la queue droite : Phi(u) - Phi(lo) = Phi(-lo) - Phi(-u)
    big = np.where(upper, log_ndtr(-lo), log_ndtr(u))
    small = np.where(upper, log_ndtr(-u), log_ndtr(lo))
    diff = big + np.log1p(-np.exp(np.minimum(small - big, 0.0)))
    return log_phi2 + diff - math.log(length)


def _log_shell(x, mu, radius, sigma):
    rho = np.linalg.norm(x - mu, axis=1)
    rho = np.maximum(rho, 1e-300)
    a = -0.5 * ((rho - radius) / sigma) ** 2
    b = -0.5 * ((-rho - radius) / sigma) ** 2
    return np.logaddexp(a, b) - 0.5 * math.log(2 * math.pi) - math.log(sigma) - math.log(4 * math.pi) - 2 * np.log(rho)


def log_scores(points, comps):
    """Matrice (n, classes) de log(n_c p_c(x)) ; classes = [-1 (bruit)] + groupes tries."""
    labels = sorted(set(c['label'] for c in comps))
    cols = dict((lab, []) for lab in labels)
    for c in comps:
        w = math.log(c['weight'])
        if c['kind'] == 'gauss':
            lp = _log_gauss(points, c['mu'], c['cov'])
        elif c['kind'] == 'segment':
            lp = _log_segment(points, c['a'], c['b'], c['sigma'])
        elif c['kind'] == 'shell':
            lp = _log_shell(points, c['mu'], c['radius'], c['sigma'])
        else:
            vol = float(np.prod(c['high'] - c['low']))
            inside = np.all((points >= c['low']) & (points <= c['high']), axis=1)
            lp = np.where(inside, -math.log(vol), -np.inf)
        cols[c['label']].append(w + lp)
    mat = np.stack([logsumexp(np.stack(cols[lab]), axis=0) for lab in labels], axis=1)
    return labels, mat


def map_labels(points, comps):
    """Etiquettes MAP (-1 = classe bruit) et marge log entre la meilleure et la deuxieme classe."""
    labels, mat = log_scores(points, comps)
    pick = np.argmax(mat, axis=1)
    out = np.array(labels, dtype=np.int64)[pick]
    srt = np.sort(mat, axis=1)
    margin = srt[:, -1] - srt[:, -2] if mat.shape[1] > 1 else np.full(len(points), np.inf)
    return out, margin


def scene(spec):
    """Scene gelable : grille quantize18 (sites distincts, ordre du generateur), verite, MAP sur les sites gardes
    (memes flottants), marge log du MAP, indices gardes, points flottants gardes, metadonnees du generateur."""
    vs = generator()
    points, labels, comps, meta = replay(spec)
    grid, kept_labels, dropped, step = vs.quantize18(points, labels)
    # indices gardes par quantize18 (premiere occurrence de chaque case, ordre d'origine), meme calcul
    low = points.min(axis=0)
    span = float((points.max(axis=0) - low).max())
    hq = span / float((1 << 18) - 1) if span > 0 else 1.0
    g = np.clip(np.floor((points - low) / hq + 0.5).astype(np.int64), 0, (1 << 18) - 1)
    _, first = np.unique(g, axis=0, return_index=True)
    keep = np.sort(first)
    need(len(keep) == len(grid) and hq == step, 'indices gardes incoherents (nombre ou pas)')
    need(np.array_equal(g[keep].astype(np.uint32), grid) and np.array_equal(labels[keep], kept_labels),
         'indices gardes incoherents (grille ou etiquettes)')
    mp, margin = map_labels(points[keep], comps)
    return dict(grid=grid, truth=kept_labels, map=mp, margin=margin, dropped=int(dropped), step=float(step),
                keep=keep, points=points[keep], meta=meta, comps=comps)


def _integral(comp, rng, draws=200000):
    """Integrale de la densite d'une composante par echantillonnage d'importance sur une gaussienne large."""
    if comp['kind'] == 'gauss':
        centre, scale = comp['mu'], 4.0 * math.sqrt(float(np.max(np.linalg.eigvalsh(comp['cov']))))
    elif comp['kind'] == 'segment':
        centre = 0.5 * (comp['a'] + comp['b'])
        scale = 0.5 * float(np.linalg.norm(comp['b'] - comp['a'])) + 4 * comp['sigma']
    else:
        centre, scale = comp['mu'], comp['radius'] + 6 * comp['sigma']
    x = centre + scale * rng.standard_normal((draws, 3))
    log_q = -0.5 * (((x - centre) / scale) ** 2).sum(axis=1) - 1.5 * math.log(2 * math.pi) - 3 * math.log(scale)
    _, mat = log_scores(x, [dict(comp, weight=1)])
    w = np.exp(mat[:, 0] - log_q)
    return float(w.mean()), float(w.std() / math.sqrt(draws))


def self_test(verbose=False):
    """Porte de rejeu sur les huit familles (deux niveaux), normalisation des densites, coherence de la
    quantification avec bench/points_campaign.synthetic_scene. Rend la liste des ecarts (vide si conforme)."""
    vs = generator()
    fails = []
    rng = np.random.default_rng(20261004)
    for family in vs.FAMILIES:
        for level in ('medium', 'hard'):
            spec = dict(family=family, level=level, groups=3, n=2000, noise_fraction=0.05, seed=7)
            try:
                _, _, comps, meta = replay(spec)
            except ValueError as error:
                fails.append('rejeu %s %s : %s' % (family, level, error))
                continue
            worst = 0.0
            for comp in comps:
                if comp['kind'] in ('gauss', 'segment', 'shell'):
                    m, se = _integral(comp, rng)
                    worst = max(worst, abs(m - 1.0))
                    if abs(m - 1.0) > max(5 * se, 0.01):
                        fails.append('integrale %s %s %s : %.4f +/- %.4f' % (family, level, comp['kind'], m, se))
            if verbose:
                print('  %-16s %-7s rejeu ok %s, %d composantes, ecart d integrale max %.4f'
                      % (family, level, meta['digest'][:12], len(comps), worst))
    spec = dict(family='bridge', level='medium', groups=8, n=8000, noise_fraction=0.05, seed=9342)
    sc = scene(spec)
    grid, labels, dropped, _ = vs.quantize18(*vs.generate(spec)[:2])
    if not (np.array_equal(grid, sc['grid']) and np.array_equal(labels, sc['truth'])):
        fails.append('quantification differente du banc v11')
    # mutant : un rejeu decale d'une graine doit etre refuse par la porte (empreinte)
    bad = dict(spec, seed=9343)
    pts, _, _ = vs.generate(bad)
    ref = vs.generate(spec)[2]['digest']
    if hashlib.sha256(np.ascontiguousarray(pts, dtype='<f8').tobytes()).hexdigest() == ref:
        fails.append('mutant de graine non distingue par l empreinte')
    return fails


def main():
    parser = argparse.ArgumentParser(description='Etiquettes MAP des scenes synthetiques E1')
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--spec', help='spec JSON complete du generateur')
    parser.add_argument('--verbose', action='store_true')
    args = parser.parse_args()
    if args.self_test:
        try:
            fails = self_test(verbose=args.verbose)
        except Exception as error:  # noqa: BLE001 - une exception de la porte est un echec, code 3
            fails = ['exception : %r' % (error,)]
        for line in fails:
            print('ECHEC', line)
        print('points_map self-test', 'conforme' if not fails else 'NON CONFORME (%d)' % len(fails))
        return 0 if not fails else 3
    if args.spec:
        sys.path.insert(0, HERE)
        import points_flat_metrics as fm
        try:
            sc = scene(json.loads(args.spec))
        except ValueError as error:
            print('refus :', error)
            return 2
        miou, _, _ = fm.miou_hungarian(sc['truth'], sc['map'])
        print(json.dumps(dict(sites=int(len(sc['grid'])), dropped=sc['dropped'], bayes_level=miou,
                              stratum=fm.bayes_stratum(miou), min_margin=float(sc['margin'].min())), sort_keys=True))
        return 0
    parser.print_help()
    return 2


if __name__ == '__main__':
    sys.exit(main())
