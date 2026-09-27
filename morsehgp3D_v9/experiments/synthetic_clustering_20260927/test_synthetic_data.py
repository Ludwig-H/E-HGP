"""Bounded local generator gates; no geometry, clustering, fitting or cloud."""
from copy import deepcopy
from fractions import Fraction
import itertools
import json
import math
import unittest

import numpy as np

import synthetic_data as data


def spec(**changes):
    result = dict(n=127,groups=4,separation=4,seed=2026092711,family='spherical',noise_fraction=0)
    result.update(changes)
    return result


def ordered_component(points,labels,parameters,label):
    indices = np.asarray(parameters['component_draw_index'])
    take = np.flatnonzero(labels == label)
    return points[take[np.argsort(indices[take])]]


class SyntheticDataTests(unittest.TestCase):
    def test_valid_spec_and_immutability(self):
        for family,noise,g in itertools.product(data.FAMILIES,(0,0.1),(1,3,8,32,64)):
            current = spec(n=800,groups=g,family=family,noise_fraction=noise)
            before = deepcopy(current)
            points,labels,parameters = data.generate(current)
            self.assertEqual(current,before)
            self.assertEqual(points.dtype,np.dtype('float64')); self.assertEqual(labels.dtype,np.dtype('int64'))
            self.assertEqual(points.shape,(800,3)); self.assertEqual(labels.shape,(800,))
            self.assertTrue(points.flags.c_contiguous and labels.flags.c_contiguous)
            self.assertTrue(np.isfinite(points).all())
            self.assertEqual(set(labels),set(range(1,g+1)) | ({-1} if noise else set()))
            self.assertEqual(parameters['component_counts'],[int(np.sum(labels == j)) for j in range(1,g+1)])
            self.assertEqual(parameters['noise_count'],int(np.sum(labels == -1)))
            self.assertEqual(sum(parameters['component_counts'])+parameters['noise_count'],800)
            self.assertEqual(json.loads(json.dumps(parameters,allow_nan=False)),parameters)

    def test_invalid_parameters_and_empty_allocations(self):
        mutations = [('n',0),('n',-1),('n',1.5),('n',True),('n',2**64),
                     ('groups',0),('groups',65),('groups',False),('groups',4.0),
                     ('separation',0),('separation',-1),('separation',math.inf),('separation',math.nan),
                     ('separation','4'),('separation',True),('seed',-1),('seed',2**64),('seed',.5),('seed',False),
                     ('family','unknown'),('family',None),('noise_fraction',.2),('noise_fraction',-.1),
                     ('noise_fraction',math.nan),('noise_fraction',True),('noise_fraction','0.1')]
        for key,value in mutations:
            with self.subTest(key=key,value=value),self.assertRaises(ValueError):
                data.generate(spec(**{key:value}))
        for value in (None,{},dict(spec(),extra=1),{k:v for k,v in spec().items() if k != 'noise_fraction'}):
            with self.assertRaises(ValueError):
                data.generate(value)
        for current in (spec(n=2,groups=3),spec(n=4,groups=4,family='unbalanced'),
                        spec(n=1,groups=2,noise_fraction=.1)):
            with self.assertRaisesRegex(ValueError,'empty generating class'):
                data.generate(current)

    def test_hamilton_against_fraction_oracle(self):
        for n,g,family,noise in itertools.product((17,31,80,101),(2,3,5),('spherical','unbalanced'),(0,.1)):
            weights = [4 if j%2 == 0 else 1 for j in range(g)] if family == 'unbalanced' else [1]*g
            counts,noise_count,metadata = data.allocate_counts(n,weights,noise)
            probabilities = {j+1:Fraction(w,sum(weights))*(Fraction(9,10) if noise else 1)
                             for j,w in enumerate(weights)}
            if noise:
                probabilities[-1] = Fraction(1,10)
            quotas = {label:n*p for label,p in probabilities.items()}
            oracle = {label:q.numerator//q.denominator for label,q in quotas.items()}
            for label in sorted(quotas,key=lambda a:(-(quotas[a]-oracle[a]),a))[:n-sum(oracle.values())]:
                oracle[label] += 1
            self.assertEqual(counts,[oracle[j] for j in range(1,g+1)])
            self.assertEqual(noise_count,oracle.get(-1,0))
            self.assertTrue(all(abs(oracle[a]-q) < 1 for a,q in quotas.items()))
            self.assertFalse(metadata['size_monotonicity_claimed'])
        self.assertEqual(data.allocate_counts(5,[1],.1)[:2],([4],1))
        self.assertEqual(data.allocate_counts(5,[1,1],0)[:2],([3,2],0))

    def test_deterministic_farther_first_and_minimum_distance(self):
        for g in (1,2,3,4,8,16,32,64):
            unit = data.unit_layout(g)
            np.testing.assert_array_equal(unit,data.unit_layout(g))
            np.testing.assert_allclose(unit.mean(axis=0),0,atol=1e-14)
            if g > 1:
                self.assertAlmostEqual(min(math.dist(a,b) for i,a in enumerate(unit) for b in unit[:i]),1)
            _,_,parameters = data.generate(spec(n=400,groups=g,separation=3))
            if g == 1:
                self.assertIsNone(parameters['minimum_mean_distance_observed'])
                self.assertEqual(parameters['centers_affine_rank'],0)
            else:
                self.assertAlmostEqual(parameters['minimum_mean_distance_observed'],3)
                self.assertLessEqual(parameters['centers_affine_rank'],min(3,g-1))
            rotation = np.asarray(parameters['layout']['common_rotation'])
            np.testing.assert_allclose(rotation.T@rotation,np.eye(3),atol=2e-15)
            self.assertAlmostEqual(np.linalg.det(rotation),1)

    def test_repeatability_distinct_seed_and_global_rng_untouched(self):
        initial = np.random.get_state()
        x,y,p = data.generate(spec()); xx,yy,pp = data.generate(spec())
        np.testing.assert_array_equal(x,xx); np.testing.assert_array_equal(y,yy); self.assertEqual(p,pp)
        other = data.generate(spec(seed=2026092712))[0]
        self.assertFalse(np.array_equal(x,other))
        final = np.random.get_state()
        self.assertEqual(initial[0],final[0]); np.testing.assert_array_equal(initial[1],final[1])
        self.assertEqual(initial[2:],final[2:])

    def test_component_prefixes_after_shuffling_all_families(self):
        for family,noise in itertools.product(data.FAMILIES,(0,.1)):
            small = data.generate(spec(n=79,family=family,noise_fraction=noise))
            large = data.generate(spec(n=211,family=family,noise_fraction=noise))
            for label in set(small[1]):
                first = ordered_component(*small,label)
                second = ordered_component(*large,label)
                count = min(len(first),len(second))
                np.testing.assert_array_equal(first[:count],second[:count])
            self.assertEqual(small[2]['means'],large[2]['means'])
            self.assertEqual(small[2]['covariances'],large[2]['covariances'])
            self.assertEqual(small[2]['noise_bbox'],large[2]['noise_bbox'])
            before = {(r['purpose'],r['component_label']):r['initial_state'] for r in small[2]['seed_states']}
            after = {(r['purpose'],r['component_label']):r['initial_state'] for r in large[2]['seed_states']}
            self.assertEqual(before,after)

    def test_component_pairing_across_noise_fraction(self):
        for family in data.FAMILIES:
            pure = data.generate(spec(n=401,family=family,noise_fraction=0))
            noisy = data.generate(spec(n=401,family=family,noise_fraction=.1))
            self.assertEqual(pure[2]['noise_bbox'],noisy[2]['noise_bbox'])
            for label in range(1,5):
                first = ordered_component(*pure,label); second = ordered_component(*noisy,label)
                count = min(len(first),len(second))
                np.testing.assert_array_equal(first[:count],second[:count])

    def test_separation_pairing_and_shuffle_independent_positions(self):
        for family in data.FAMILIES:
            first = data.generate(spec(family=family,separation=1.5,noise_fraction=.1))
            second = data.generate(spec(family=family,separation=6,noise_fraction=.1))
            np.testing.assert_array_equal(first[1],second[1])
            self.assertEqual(first[2]['component_draw_index'],second[2]['component_draw_index'])
            self.assertEqual(first[2]['shuffle'],second[2]['shuffle'])
            self.assertEqual(first[2]['standardized_draws'],second[2]['standardized_draws'])
            self.assertEqual(first[2]['component_rotations'],second[2]['component_rotations'])
            for label in range(1,5):
                residual = ordered_component(*first,label)-first[2]['means'][label-1]
                other = ordered_component(*second,label)-second[2]['means'][label-1]
                np.testing.assert_allclose(residual,other,atol=3e-14,rtol=0)

    def test_covariances_ranks_and_model_weights(self):
        for family in data.FAMILIES:
            _,_,parameters = data.generate(spec(family=family,groups=6,n=600))
            self.assertEqual(parameters['true_rank'],3)
            self.assertEqual(parameters['component_covariance_ranks'],[3]*6)
            for j,covariance in enumerate(parameters['covariances']):
                expected = [.25,1,4] if family == 'anisotropic' else [(.5,1,2)[j%3]**2]*3 if family == 'heteroscedastic' else [.0025,.5025,.5025] if family == 'rings' else [1]*3
                np.testing.assert_allclose(np.linalg.eigvalsh(covariance),expected,atol=3e-15)
            if family == 'unbalanced':
                self.assertEqual(parameters['component_weights'],[4,1]*3)
                self.assertEqual(parameters['component_counts'],[160,40]*3)
            elif family == 'heteroscedastic':
                self.assertEqual(parameters['component_standard_deviations'],[[.5]*3,[1.]*3,[2.]*3]*2)
            elif family == 'rings':
                self.assertEqual(parameters['latent_manifold_dimension'],1)
                self.assertEqual(parameters['ring_radius'],1)
                self.assertEqual(parameters['ring_jitter_std'],.05)

    def test_noise_model_box_not_sample_bounding_box(self):
        points,labels,parameters = data.generate(spec(n=301,noise_fraction=.1))
        box = parameters['noise_bbox']; lower,upper = np.asarray(box['lower']),np.asarray(box['upper'])
        means = np.asarray(parameters['means'])
        np.testing.assert_allclose(lower,means.min(axis=0)-6,atol=2e-15)
        np.testing.assert_allclose(upper,means.max(axis=0)+6,atol=2e-15)
        noise = points[labels == -1]
        self.assertTrue(np.all(noise >= lower) and np.all(noise <= upper))
        self.assertTrue(np.all(means > lower) and np.all(means < upper))
        self.assertFalse(box['reject_in_hull_points'])
        self.assertFalse(np.array_equal(lower,points[labels > 0].min(axis=0)))

    def test_seed_state_replay_and_complete_permutation(self):
        points,labels,parameters = data.generate(spec(n=223,family='rings',noise_fraction=.1,seed=2**64-1))
        records = {(r['purpose'],r['component_label']):r for r in parameters['seed_states']}
        record = records['output_shuffle',0]
        bitgen = np.random.PCG64(); bitgen.state = deepcopy(record['initial_state'])
        rng = np.random.Generator(bitgen); permutation = rng.permutation(223)
        self.assertEqual(rng.bit_generator.state,record['final_state'])
        self.assertEqual(data._digest(permutation,'<i8'),parameters['shuffle']['permutation_sha256'])
        original_labels = np.concatenate([np.full(count,j+1,dtype=np.int64) for j,count in enumerate(parameters['component_counts'])]
                                        +[np.full(parameters['noise_count'],-1,dtype=np.int64)])
        np.testing.assert_array_equal(labels,original_labels[permutation])
        for label in set(labels):
            draw_ids = np.asarray(parameters['component_draw_index'])[labels == label]
            np.testing.assert_array_equal(np.sort(draw_ids),np.arange(len(draw_ids)))
        record = records['component_normal',1]
        bitgen.state = deepcopy(record['initial_state'])
        normals = rng.standard_normal((parameters['component_counts'][0],3))
        self.assertEqual(data._digest(normals,'<f8'),parameters['standardized_draws'][0]['normal_sha256'])
        self.assertEqual(rng.bit_generator.state,record['final_state'])


if __name__ == '__main__':
    unittest.main()
