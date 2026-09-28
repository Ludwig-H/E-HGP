"""Portes de la comparaison appariee : aucun tri possible, inversion visible.

Aucune porte ne repose sur `assert` : elles tiennent sous `python3 -O`.
"""

import json
import unittest

import compare


def need(condition, reason):
    if not condition:
        raise AssertionError(reason)


def rows(*items):
    """Construit l'index (methode, scene, graine) attendu par `compare`."""
    out = {}
    for method, scene, seed, ari, coverage in items:
        out[(method, scene, seed)] = dict(method=method, scene=scene, seed=str(seed), ari=str(ari),
                                          coverage=str(coverage), family='spherical', level='medium')
    return out


class Pairing(unittest.TestCase):
    def test_every_pair_is_published_including_the_losing_ones(self):
        """Le tri sur la paire favorable est structurellement impossible."""
        challenger = rows(('tour', 's1', 1, 0.9, 1.0), ('tour', 's2', 1, 0.8, 1.0),
                          ('tour_naive', 's1', 1, 0.1, 1.0), ('tour_naive', 's2', 1, 0.2, 1.0))
        reference = rows(('hdbscan_default', 's1', 1, 0.5, 0.9), ('hdbscan_default', 's2', 1, 0.5, 0.9),
                         ('hdbscan_oracle', 's1', 1, 0.95, 0.9), ('hdbscan_oracle', 's2', 1, 0.95, 0.9))
        table = compare.compare(challenger, reference)
        pairs = {(row['challenger'], row['reference']) for row in table}
        need(pairs == {('tour', 'hdbscan_default'), ('tour', 'hdbscan_oracle'),
                       ('tour_naive', 'hdbscan_default'), ('tour_naive', 'hdbscan_oracle')},
             'every challenger-reference pair must appear, got ' + str(sorted(pairs)))
        losing = [r for r in table if r['challenger'] == 'tour_naive' and r['reference'] == 'hdbscan_oracle']
        need(len(losing) == 1 and losing[0]['mean_delta'] < 0, 'a losing pair must still be published')

    def test_seed_inversion_is_detected_and_published(self):
        """Le constat majeur du 28 septembre : une inversion de graine se voit."""
        challenger = rows(('tour', 's1', 1, 0.90, 1.0), ('tour', 's1', 2, 0.10, 1.0))
        reference = rows(('hdbscan_default', 's1', 1, 0.50, 1.0), ('hdbscan_default', 's1', 2, 0.50, 1.0))
        row = compare.compare(challenger, reference)[0]
        need(row['seed_inversion'] is True, 'an inversion between two seeds must be flagged')
        per_seed = json.loads(row['per_seed_delta'])
        need(per_seed == {'1': 0.4, '2': -0.4}, 'each seed publishes its own delta, got ' + str(per_seed))
        need(abs(row['mean_delta']) < 1e-9, 'the mean alone would have hidden it')
        need(row['wins'] == 1 and row['losses'] == 1, 'wins and losses are counted per pair')

    def test_no_inversion_when_every_seed_agrees(self):
        challenger = rows(('tour', 's1', 1, 0.9, 1.0), ('tour', 's1', 2, 0.8, 1.0))
        reference = rows(('hdbscan_default', 's1', 1, 0.5, 1.0), ('hdbscan_default', 's1', 2, 0.5, 1.0))
        row = compare.compare(challenger, reference)[0]
        need(row['seed_inversion'] is False, 'no inversion when both seeds agree')
        need(row['worst_delta'] > 0, 'the worst case is published too')

    def test_pairing_is_on_scene_and_seed(self):
        """Comparer deux methodes sur des scenes differentes n'a aucun sens."""
        challenger = rows(('tour', 's1', 1, 0.9, 1.0), ('tour', 's2', 1, 0.9, 1.0))
        reference = rows(('hdbscan_default', 's1', 1, 0.5, 1.0), ('hdbscan_default', 's2', 9, 0.5, 1.0))
        row = compare.compare(challenger, reference)[0]
        need(row['pairs'] == 1, 'only the scene-seed pairs present on both sides are compared')

    def test_a_method_is_never_compared_to_itself(self):
        both = rows(('hdbscan_default', 's1', 1, 0.5, 1.0))
        other = rows(('hdbscan_default', 's1', 1, 0.5, 1.0), ('hdbscan_oracle', 's1', 1, 0.7, 1.0))
        table = compare.compare(both, other)
        need(all(r['challenger'] != r['reference'] for r in table), 'no self-comparison')
        need(len(table) == 1, 'exactly one real pair remains')

    def test_slices_split_without_losing_the_whole(self):
        challenger = rows(('tour', 's1', 1, 0.9, 1.0), ('tour', 's2', 1, 0.3, 1.0))
        reference = rows(('hdbscan_default', 's1', 1, 0.5, 1.0), ('hdbscan_default', 's2', 1, 0.5, 1.0))
        table = compare.compare(challenger, reference, slices=('all', 'level'))
        labels = {row['slice'] for row in table}
        need('all' in labels and 'level=medium' in labels, 'both the whole and the slice are published')

    def test_an_empty_or_unpaired_input_is_refused(self):
        with self.assertRaises(ValueError):
            compare.compare(rows(('tour', 's1', 1, 0.9, 1.0)), rows(('hdbscan_default', 's9', 1, 0.5, 1.0)))
        with self.assertRaises(ValueError):
            compare.compare({}, rows(('hdbscan_default', 's1', 1, 0.5, 1.0)))


if __name__ == '__main__':
    unittest.main()
