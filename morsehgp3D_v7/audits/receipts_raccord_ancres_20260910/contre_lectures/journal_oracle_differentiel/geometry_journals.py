"""Journaux derives d'une geometrie REELLE via les modeles de l'auditeur
(lecture seule, sys.path) : plateau_model.Model (Gram rationnel exhaustif,
juge Gamma) et coverage_contribution_model.produce (producteur a contributions
datees). Les journaux `gap` (D_B) et `whole` (S_B complet) de chaque ordre sont
traduits au format du pont, avec :
  - identites produit simulees (ids denses dans l'ordre des actions, continuations
    sautees), K1 : naissances reordonnees dans l'ordre du domaine ;
  - include_interior normalise a (I non vide) : le produit refuse un drapeau
    d'interieur sur une population sans interieur (le modele le laisse a True).
Les attentes par coupe (toutes les MEB du nuage, intermediaires, 0, max+1 ;
ouverte et fermee) sont : les couvertures des composantes Gamma (multi-ensemble)
et la lecture par token du modele (read_cut), remappee en ids produit.
"""

import sys
from collections import defaultdict
from fractions import Fraction

AUDITOR_DIR = '/workspaces/E-HGP/morsehgp3D_v7/audits/receipts_plateaux_full_20260906'
if AUDITOR_DIR not in sys.path:
    sys.path.insert(0, AUDITOR_DIR)

import coverage_contribution_model as ccm   # noqa: E402  (lecture seule)
import plateau_model as pm                  # noqa: E402

CLOUDS = {
    'abcz': [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0)],
    'square': [(0, 0, 0), (2, 0, 0), (2, 2, 0), (0, 2, 0)],
    'right_triangle': [(0, 0, 0), (4, 0, 0), (2, 2, 0)],
    'external_bridge': [(0, 0, 0), (4, 0, 0), (2, 2, 0), (2, 3, 0)],
    'abczxy': [(1, 8, 0), (5, 10, 0), (9, 8, 0), (5, 0, 0), (10, 6, 0), (9, 1, 0)],
    'window_shell7': [(10, 5, 0), (0, 5, 0), (5, 10, 0), (5, 0, 0), (8, 9, 0), (2, 1, 0), (9, 8, 0)],
    'tetra_origin': [(2, 2, 2), (2, 0, 0), (0, 2, 0), (0, 0, 2), (0, 0, 0)],
}


class Refusal(Exception):
    pass


def refuse(condition, reason):
    if not condition:
        raise Refusal(reason)


def frac_repr(value):
    return [str(value.numerator), str(value.denominator)]


def translate(model, state, k, whole):
    """Etat final du producteur de l'auditeur (ordre k) -> journal du pont.

    Rend (journal, token_to_id)."""
    stream = state.whole if whole else state.gap
    populations = []
    key_to_pop = {}

    def population(key):
        if key not in key_to_pop:
            ball = model.balls[key]
            key_to_pop[key] = len(populations)
            populations.append(dict(interior=sorted(ball['interior']), shell=sorted(ball['shell'])))
        return key_to_pop[key]

    def ref(contribution):
        key = contribution.ball
        interior = model.balls[key]['interior']
        return dict(population=population(key), shell_mask=contribution.shell_mask,
                    include_interior=bool(contribution.include_interior and interior))

    radii = sorted({node.radius for node in state.nodes} | {c.radius for c in stream})
    token_to_id = {}
    batches = []
    next_id = 0
    for radius in radii:
        new_tokens = [t for t, node in enumerate(state.nodes) if node.radius == radius]
        by_token = defaultdict(list)
        for c in stream:
            if c.radius == radius:
                by_token[c.token].append(c)
        if k == 1 and radius == 0:
            # naissances des singletons : ordre du domaine impose par le produit
            def point_of(token):
                rows = by_token[token]
                refuse(len(rows) == 1, 'k1_singleton_birth_has_one_reference')
                ball = model.balls[rows[0].ball]
                pts = sorted(ball['interior'] | ball['shell'])
                refuse(len(pts) == 1, 'k1_singleton_population')
                return pts[0]
            new_tokens.sort(key=point_of)
        actions = []
        for t in new_tokens:
            node = state.nodes[t]
            parents = sorted(token_to_id[p] for p in node.parents)
            refuse(len(parents) != 1, 'topology_node_is_birth_or_multifusion')
            actions.append(dict(parents=parents, contributions=[ref(c) for c in by_token.pop(t, [])]))
            token_to_id[t] = next_id
            next_id += 1
        for t in sorted(by_token):
            refuse(t in token_to_id and state.nodes[t].radius < radius, 'continuation_targets_older_token')
            actions.append(dict(parents=[token_to_id[t]], contributions=[ref(c) for c in by_token[t]]))
        refuse(bool(actions), 'nonempty_batch')
        batches.append(dict(level=frac_repr(radius), actions=actions))
    domain = sorted(model.ids)
    return dict(order=k, domain=domain, populations=populations, batches=batches), token_to_id


def cut_grid(model):
    levels = sorted(set(model.level.values()))
    cuts = set(levels)
    for i in range(1, len(levels)):
        cuts.add((levels[i - 1] + levels[i]) / 2)
    cuts.add(Fraction(0))
    cuts.add(levels[-1] + 1)
    return sorted(cuts)


def expectations(model, state, k, token_to_id, cuts, whole):
    """Par coupe : couvertures Gamma (multi-ensemble) et lecture du modele par id."""
    stream = state.whole if whole else state.gap
    out = []
    for cut in cuts:
        for closed in (False, True):
            gamma = model.gamma(k, cut, closed)
            gamma_cover = sorted(sorted(pm.cover(component)) for component in gamma)
            read = ccm.read_cut(model, state.nodes, stream, cut, closed)
            by_id = {str(token_to_id[token]): sorted(points) for token, points in read.items()}
            out.append(dict(level=frac_repr(cut), closed=closed, gamma=gamma_cover, model_read=by_id))
    return out


def geometry_corpus():
    """Liste de (journal, attentes) pour chaque nuage, ordre et variante gap/whole."""
    corpus = []
    for name, points in sorted(CLOUDS.items()):
        model = pm.Model(points)
        kmax = len(points)
        final, _, _ = ccm.produce(model, kmax)
        cuts = cut_grid(model)
        for k in range(1, kmax + 1):
            state = final[k - 1]
            for whole in (False, True):
                journal, token_to_id = translate(model, state, k, whole)
                journal['name'] = 'geometry_%s_K%d_%s' % (name, k, 'whole' if whole else 'gap')
                journal['cloud'] = name
                journal['points'] = [list(p) for p in points]
                # toutes les coupes en extra ; le pont ajoute lui-meme les niveaux des lots
                journal['cuts'] = [frac_repr(c) for c in cuts]
                expected = expectations(model, state, k, token_to_id, cuts, whole)
                corpus.append((journal, dict(cuts=expected, token_to_id={str(t): i for t, i in token_to_id.items()},
                                             nodes=len(state.nodes))))
    return corpus
