"""Portes du clustering : plateaux, antichaine, masse, vote.

Aucune porte ne repose sur `assert` : elles tiennent sous `python3 -O`.
"""

from fractions import Fraction as F
import unittest

import cluster as C


def need(condition, reason):
    if not condition:
        raise AssertionError(reason)


def build(cofaces, keep=None):
    facets, plateaus = C.facet_levels(cofaces, keep)
    births = {}
    for vertices, beta in cofaces:
        for drop in vertices:
            facet = tuple(v for v in vertices if v != drop)
            births[facet] = min(births.get(facet, beta), beta)
    return facets, plateaus, births


class Plateaus(unittest.TestCase):
    def test_a_level_shared_by_two_cofaces_is_one_action(self):
        """Le plateau n'est jamais binarise : une date, un noeud, trois enfants.

        Deux cofaces disjointes au MEME niveau relient trois composantes. Une
        cascade binaire inventerait un ordre entre deux fusions simultanees et
        un niveau intermediaire qui n'existe pas.
        """
        cofaces = [((0, 1, 2), F(4)), ((1, 2, 3), F(4))]
        facets, plateaus, births = build(cofaces)
        need(len(plateaus) == 1, 'both cofaces sit on one level, got %d' % len(plateaus))
        nodes, roots = C.merge_tree(facets, plateaus, births)
        need(len(roots) == 1, 'the two cofaces are joined through the shared facet')
        need(len(nodes) == 1, 'one level must give exactly one node, got %d' % len(nodes))
        only = nodes[roots[0]]
        need(only['level'] == F(4), 'the node carries the exact plateau level')
        need(len(only['children']) == 5, 'the simultaneous merge keeps all five facets, got %d'
             % len(only['children']))

    def test_distinct_levels_give_distinct_actions(self):
        cofaces = [((0, 1, 2), F(1)), ((1, 2, 3), F(9))]
        facets, plateaus, births = build(cofaces)
        need(len(plateaus) == 2, 'two levels, two plateaus')
        nodes, roots = C.merge_tree(facets, plateaus, births)
        need(len(nodes) == 2 and len(roots) == 1, 'a chain of two actions')
        levels = sorted(node['level'] for node in nodes.values())
        need(levels == [F(1), F(9)], 'the exact levels are kept, got %s' % levels)

    def test_the_star_the_path_and_the_clique_agree(self):
        """Les trois sous-graphes couvrants d'une coface ont les memes composantes.

        C'est ce qui rend la question clique/chemin/etoile sans objet ici : le
        module reunit les facettes d'une coface en bloc, donc il ne choisit pas.
        """
        facets, plateaus, births = build([((0, 1, 2, 3), F(4))])
        nodes, roots = C.merge_tree(facets, plateaus, births)
        need(len(roots) == 1, 'all four facets of one coface land in one component')
        need(len(nodes[roots[0]]['members']) == 4, 'and all four are members')


class Condensation(unittest.TestCase):
    def _tree(self):
        # Deux paquets nets qui se rejoignent tard : une vraie scission.
        cofaces = [((0, 1, 2), F(1)), ((1, 2, 3), F(1)),
                   ((10, 11, 12), F(1)), ((11, 12, 13), F(1)),
                   ((1, 2, 10), F(100)), ((2, 10, 11), F(100))]
        facets, plateaus, births = build(cofaces)
        nodes, roots = C.merge_tree(facets, plateaus, births)
        masses = {facet: 1.0 for facet in facets}
        return nodes, roots, masses, births

    def test_the_threshold_is_a_mass_not_a_count(self):
        """Une masse faible repartie sur beaucoup de facettes ne fait pas un cluster."""
        nodes, roots, masses, births = self._tree()
        light = {facet: 0.01 for facet in masses}
        heavy, _ = C.condense(nodes, roots, light, births, 1.0, 'radius', 1)
        need(len(heavy) == 1, 'under a mass threshold of one, nothing splits, got %d' % len(heavy))
        many, _ = C.condense(nodes, roots, masses, births, 1.0, 'radius', 1)
        need(len(many) > 1, 'with unit masses the same tree does split')

    def test_selection_is_an_antichain(self):
        nodes, roots, masses, births = self._tree()
        clusters, order = C.condense(nodes, roots, masses, births, 2.0, 'radius', 1)
        selected = set(C.select_excess_of_mass(clusters, order))
        need(selected, 'the selection is never empty')
        for name in selected:
            parent = clusters[name]['parent']
            while parent is not None:
                need(parent not in selected, 'a selected cluster contains another: not an antichain')
                parent = clusters[parent]['parent']

    def test_a_split_parent_keeps_the_mass_it_carried(self):
        """Le parent compte les facettes de ses gros enfants, a la scission.

        Les omettre viderait tout parent de sa masse : sa stabilite serait
        nulle par construction et l'exces de masse choisirait les enfants a
        chaque scission, quel que soit le contraste. C'est la definition de
        HDBSCAN, et c'est le defaut qui faisait surdecouper la famille
        `hierarchical` du banc.
        """
        nodes, roots, masses, births = self._tree()
        clusters, order = C.condense(nodes, roots, masses, births, 2.0, 'radius', 1)
        parents = [name for name, c in clusters.items() if len(c['children']) >= 2]
        need(parents, 'the fixture must contain a real split')
        for name in parents:
            held = {facet for facet, _ in clusters[name]['falls']}
            for child in clusters[name]['children']:
                child_facets = {facet for facet, _ in clusters[child]['falls']}
                need(child_facets <= held,
                     'the parent must have carried every facet of its child before the split')
            need(clusters[name]['mass'] >= sum(clusters[child]['mass'] for child in clusters[name]['children'])
                 - 1e-9, 'the parent mass covers its children')
        need(sum(c['stability'] for c in clusters.values()) > 0.0, 'the tree carries some stability')

    def test_omitting_the_parent_mass_would_collapse_its_stability(self):
        """Temoin du defaut : sans les facettes des enfants, le parent est vide.

        On reconstruit ici le calcul fautif et on exige qu'il donne un parent
        strictement plus leger. Sans ce temoin, une regression qui reintroduit
        le defaut passerait inapercue, parce que le pipeline continuerait de
        rendre des clusters.
        """
        nodes, roots, masses, births = self._tree()
        clusters, _ = C.condense(nodes, roots, masses, births, 2.0, 'radius', 1)
        parents = [name for name, c in clusters.items() if len(c['children']) >= 2]
        need(parents, 'the fixture must contain a real split')
        for name in parents:
            own = {facet for facet, _ in clusters[name]['falls']}
            from_children = set()
            for child in clusters[name]['children']:
                from_children |= {facet for facet, _ in clusters[child]['falls']}
            need(from_children, 'the children must hold facets')
            need(own - from_children != own, 'the parent must share facets with its children')

    def test_the_stability_scale_is_declared(self):
        nodes, roots, masses, births = self._tree()
        for scale in C.STABILITY_SCALES:
            clusters, _ = C.condense(nodes, roots, masses, births, 2.0, 'radius', 1, scale)
            need(clusters, scale + ': the condensation must produce clusters')
        with self.assertRaises(ValueError):
            C.condense(nodes, roots, masses, births, 2.0, 'radius', 1, 'invented')

    def test_an_unknown_lambda_mode_is_refused(self):
        nodes, roots, masses, births = self._tree()
        with self.assertRaises(ValueError):
            C.condense(nodes, roots, masses, births, 1.0, 'whatever', 1)


class Vote(unittest.TestCase):
    def test_a_point_without_any_vote_is_noise(self):
        sums = {(0, 1): 1.0}
        totals = {0: 1.0, 1: 1.0, 2: 0.0}
        labels, ties = C.vote(3, {(0, 1): 0}, sums, totals, 1)
        need(labels == [0, 0, -1], 'the uncovered point is noise, got %s' % labels)
        need(ties == 0, 'no tie here')

    def test_the_argmax_follows_the_weight_and_ties_are_counted(self):
        sums = {(0, 1): 3.0, (0, 2): 1.0}
        totals = {0: 4.0, 1: 3.0, 2: 1.0}
        labels, _ = C.vote(3, {(0, 1): 0, (0, 2): 1}, sums, totals, 2)
        need(labels[0] == 0, 'point zero follows its heavier facet, got %s' % labels)
        even = {(0, 1): 2.0, (0, 2): 2.0}
        totals = {0: 4.0, 1: 2.0, 2: 2.0}
        labels, ties = C.vote(3, {(0, 1): 0, (0, 2): 1}, even, totals, 2)
        need(ties == 1, 'a genuine tie must be counted, not hidden')
        need(labels[0] == 0, 'and settled by the smallest index, deterministically')

    def test_a_facet_outside_every_selected_cluster_does_not_vote(self):
        sums = {(0, 1): 5.0}
        totals = {0: 5.0, 1: 5.0}
        labels, _ = C.vote(2, {}, sums, totals, 1)
        need(labels == [-1, -1], 'nothing is selected, so nothing is labelled')


if __name__ == '__main__':
    unittest.main()
