"""Portes du banc : determinisme, verite de reference, plan, quantification.

Aucune porte ne repose sur `assert` : elles tiennent sous `python3 -O`.

  python3 -m unittest discover -s . -p 'test_*.py'
"""

import unittest

import numpy as np

import baselines
import bench_datasets as data
import plan as bench_plan


def need(condition, reason):
    if not condition:
        raise AssertionError(reason)


CENTRE = dict(family='spherical', n=600, groups=4, level='medium', noise_fraction=0.0, seed=7)


class Datasets(unittest.TestCase):
    def test_same_spec_same_bytes(self):
        first = data.generate(dict(CENTRE))
        second = data.generate(dict(CENTRE))
        need(first[2]['digest'] == second[2]['digest'], 'a spec must give the same points twice')
        need(np.array_equal(first[0], second[0]) and np.array_equal(first[1], second[1]), 'same arrays')
        other = data.generate(dict(CENTRE, seed=8))
        need(other[2]['digest'] != first[2]['digest'], 'another seed must give other points')

    def test_every_family_and_level(self):
        for family in data.FAMILIES:
            for level in data.LEVELS:
                points, labels, meta = data.generate(dict(CENTRE, family=family, level=level))
                need(len(points) == len(labels) == CENTRE['n'], family + ': n points and n labels')
                need(np.isfinite(points).all(), family + ': finite coordinates')
                need(meta['separation'] == data.SEPARATION[family][level], family + ': published separation')
                bridge = family == 'bridge'
                need(meta['groups_present'] == CENTRE['groups'], family + ': every group present')
                need((meta['noise_points'] > 0) == bridge, family + ': noise only where the family makes it')

    def test_noise_is_labelled_and_counted(self):
        points, labels, meta = data.generate(dict(CENTRE, noise_fraction=0.25))
        need(meta['noise_points'] == int((labels < 0).sum()) > 0, 'noise is labelled -1 and counted')
        need(abs(meta['noise_points'] - 0.25 * CENTRE['n']) <= 1, 'the noise fraction is honoured')

    def test_sizes_are_balanced_or_unbalanced(self):
        _, balanced, _ = data.generate(dict(CENTRE, family='spherical'))
        counts = np.bincount(balanced[balanced >= 0])
        need(counts.max() - counts.min() <= 1, 'the spherical family has equal groups')
        _, skewed, _ = data.generate(dict(CENTRE, family='unbalanced'))
        counts = np.bincount(skewed[skewed >= 0])
        need(counts.max() >= 4 * counts.min(), 'the unbalanced family is really unbalanced')

    def test_spec_is_closed(self):
        for bad in (dict(CENTRE, family='unknown'), dict(CENTRE, level='trivial'), dict(CENTRE, groups=1),
                    dict(CENTRE, n=4), dict(CENTRE, noise_fraction=1.0), dict(CENTRE, extra=1)):
            with self.assertRaises(ValueError):
                data.generate(bad)
        with self.assertRaises(ValueError):
            data.generate({key: value for key, value in CENTRE.items() if key != 'seed'})

    def test_every_family_declares_its_intrinsic_dimension(self):
        """L'exposant z du poids se lit sur la famille, jamais sur l'ambiant."""
        need(set(data.INTRINSIC_DIMENSION) == set(data.FAMILIES), 'every family declares a dimension')
        need(set(data.INTRINSIC_DIMENSION.values()) == {1, 2, 3}, 'the bench spans dimensions one to three')
        need(data.INTRINSIC_DIMENSION['shells'] == 2, 'a hollow sphere is a surface')
        need(data.INTRINSIC_DIMENSION['filaments'] == 1, 'a filament is a curve')

    def test_the_calibration_table_is_complete_and_ordered(self):
        """La difficulte publiee decroit strictement d'easy a extreme."""
        need(set(data.CALIBRATION) == set(data.FAMILIES), 'every family is calibrated')
        for family in data.FAMILIES:
            published = [data.CALIBRATION[family][level] for level in data.LEVELS]
            need(published == sorted(published, reverse=True), family + ': easy to extreme must decrease')
            need(published[0] - published[-1] >= 0.40,
                 family + ': easy and extreme must be far apart (got %.2f)' % (published[0] - published[-1]))

    def test_quantisation_fits_the_engine(self):
        points, _, _ = data.generate(dict(CENTRE, n=2000))
        scale = (points.max() - points.min()) / 40000.0
        grid, step = data.quantize(points, millimetre=scale)
        need(grid.dtype == np.uint32 and grid.shape == points.shape, 'u32 grid of the same shape')
        need(step == scale, 'the step is published')
        with self.assertRaises(ValueError):
            data.quantize(points, millimetre=scale * 1e6)  # everything on one cell: duplicates


class Plan(unittest.TestCase):
    def test_plan_is_stable_and_covers_the_axes(self):
        first, second = bench_plan.specifications(), bench_plan.specifications()
        need(first == second, 'the plan is deterministic')
        summary = bench_plan.summary(first)
        need(set(summary['families']) == set(data.FAMILIES), 'every family is planned')
        need(set(summary['levels']) == set(data.LEVELS), 'every difficulty is planned')
        need(summary['sizes'] == list(bench_plan.SIZES), 'every size is planned')
        need(summary['groups'] == list(bench_plan.GROUPS), 'every group count is planned')
        need(summary['runs'] == summary['scenes'] * bench_plan.SEEDS, 'every scene has its seeds')

    def test_light_plan_drops_the_heavy_sizes(self):
        light = bench_plan.summary(bench_plan.specifications(heavy=False))
        need(32000 not in light['sizes'], 'the light plan has no 32000-point scene')


class Baselines(unittest.TestCase):
    TOLERANCE = 0.18

    def test_levels_match_the_published_calibration(self):
        """Chaque famille retombe sur sa difficulte publiee, sans inversion.

        C'est la porte qui donne un sens aux quatre noms : un niveau est
        `hard` parce que la reference y tombe a 0,4 au point de calibration,
        et la table du module doit rester vraie. Trois graines sur les cinq
        publiees suffisent a detecter une derive ; la tolerance couvre la
        variation de graine, pas un changement de famille.
        """
        point = data.CALIBRATION_POINT
        for family in data.FAMILIES:
            measured = []
            for level in data.LEVELS:
                scores = []
                for seed in point['seeds'][:3]:
                    spec = dict(family=family, n=point['n'], groups=point['groups'], level=level,
                                noise_fraction=point['noise_fraction'], seed=seed)
                    points, truth, _ = data.generate(spec)
                    scores.append(baselines.hdbscan_default(points, truth)['ari'])
                measured.append(sum(scores) / len(scores))
            for level, value in zip(data.LEVELS, measured):
                published = data.CALIBRATION[family][level]
                need(abs(value - published) <= self.TOLERANCE,
                     '%s/%s: measured %.2f, published %.2f' % (family, level, value, published))
            for earlier, later in zip(measured, measured[1:]):
                need(later <= earlier + 0.05,
                     '%s: difficulty must not invert (%.2f then %.2f)' % (family, earlier, later))

    def test_easy_is_solved_and_extreme_resists_everywhere(self):
        """Le banc discrimine dans TOUTES les familles, pas seulement la premiere."""
        point = data.CALIBRATION_POINT
        for family in data.FAMILIES:
            floor = data.CALIBRATION[family]['extreme'] + self.TOLERANCE
            easy = data.generate(dict(family=family, n=point['n'], groups=point['groups'], level='easy',
                                      noise_fraction=0.0, seed=point['seeds'][0]))
            solved = baselines.hdbscan_default(easy[0], easy[1])['ari']
            need(solved > 0.85, family + ': the easy level must be solved (got %.3f)' % solved)
            worst = data.generate(dict(family=family, n=point['n'], groups=point['groups'], level='extreme',
                                       noise_fraction=0.0, seed=point['seeds'][0]))
            resists = baselines.hdbscan_default(worst[0], worst[1])['ari']
            need(resists < floor, family + ': the extreme level must resist (got %.3f)' % resists)
            need(solved - resists > 0.35, family + ': easy and extreme must be far apart')

    def test_oracle_is_at_least_the_default(self):
        points, truth, _ = data.generate(dict(CENTRE, level='hard'))
        default = baselines.hdbscan_default(points, truth)
        oracle = baselines.hdbscan_oracle(points, truth)
        need(oracle['ari'] >= default['ari'] - 1e-12, 'the oracle is the best HDBSCAN, never worse')
        need(oracle['parameter'] in baselines.ORACLE_GRID, 'the oracle publishes the size it chose')

    def test_scores_are_consistent(self):
        points, truth, _ = data.generate(dict(CENTRE))
        perfect = baselines.scores(truth, truth)
        need(abs(perfect['ari'] - 1.0) < 1e-12 and perfect['coverage'] == 1.0, 'truth against itself scores one')
        rejected = np.full(len(truth), -1)
        need(baselines.scores(truth, rejected)['coverage'] == 0.0, 'rejecting everything gives zero coverage')


if __name__ == '__main__':
    unittest.main()
