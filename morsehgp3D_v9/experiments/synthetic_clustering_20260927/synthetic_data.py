#!/usr/bin/env python3
"""Original, model-declared synthetic 3D inputs; no fitting or quantization.

generate(spec) -> float64[n,3], int64[n], JSON-ready model parameters.
Six required keys: n, groups, separation, seed, family, noise_fraction.
Every component has a count-independent random stream. After undoing the
shuffle by (label, component_draw_index), repeated sizes share the same
component prefix, up to the smaller allocated count. Hamilton allocation
is NOT generally monotone in n, so whole-scene inclusion is not promised.
No historical/noncommercial clustering implementation is imported/copied.
"""
from __future__ import annotations

from copy import deepcopy
from fractions import Fraction
import hashlib
import itertools
import math
from numbers import Integral, Real

import numpy as np

SCHEMA = 'mhgp9_declared_synthetic_model_v1'
FAMILIES = ('spherical', 'anisotropic', 'unbalanced', 'heteroscedastic', 'rings')
MAX_GROUPS = 64
SPEC_KEYS = {'n', 'groups', 'separation', 'seed', 'family', 'noise_fraction'}
STREAMS = dict(layout_rotation=10, component_rotation=20, component_normal=30,
               component_angle=40, uniform_noise=50, output_shuffle=60)


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def _integer(value, name, lower, upper=None):
    need(isinstance(value, Integral) and not isinstance(value, (bool, np.bool_)), name+': integer required')
    value = int(value)
    need(value >= lower and (upper is None or value <= upper), name+': outside supported domain')
    return value


def _real(value, name):
    need(isinstance(value, Real) and not isinstance(value, (bool, np.bool_)), name+': real number required')
    try:
        result = float(value)
    except (ValueError, OverflowError) as exc:
        raise ValueError(name+': unrepresentable binary64 value') from exc
    need(math.isfinite(result), name+': finite value required')
    return result


def validate_spec(spec):
    need(isinstance(spec, dict) and set(spec) == SPEC_KEYS, 'exact six-key synthetic specification required')
    n = _integer(spec['n'], 'n', 1, np.iinfo(np.intp).max//24)
    groups = _integer(spec['groups'], 'groups', 1, MAX_GROUPS)
    seed = _integer(spec['seed'], 'seed', 0, 2**64-1)
    separation = _real(spec['separation'], 'separation')
    noise = _real(spec['noise_fraction'], 'noise_fraction')
    need(separation > 0, 'separation must be positive')
    need(type(spec['family']) is str and spec['family'] in FAMILIES, 'unsupported family')
    need(noise in (0.0, 0.1), 'noise_fraction must be 0 or 0.1')
    return dict(n=n, groups=groups, separation=separation, seed=seed,
                family=spec['family'], noise_fraction=noise)


def allocate_counts(n, weights, noise_fraction):
    """One exact Hamilton allocation over all components and optional noise.

    Noise's ideal mass is precisely 1/10, not its binary64 approximation.
    Remainders are compared as integers; ties use label ID (-1 before 1..G).
    Zero allocated generating classes are refused, never deleted or merged.
    """
    n = _integer(n, 'n', 1)
    weights = [_integer(w, 'component weight', 1) for w in weights]
    need(weights and len(weights) <= MAX_GROUPS, 'nonempty supported component weights')
    noise = _real(noise_fraction, 'noise_fraction')
    need(noise in (0.0, 0.1), 'noise_fraction must be 0 or 0.1')
    total = sum(weights)
    numerators = [9*w for w in weights] if noise else weights
    ids = list(range(1, len(weights)+1))
    denominator = 10*total if noise else total
    if noise:
        ids.append(-1); numerators.append(total)
    counts = [n*value//denominator for value in numerators]
    remainders = [n*value % denominator for value in numerators]
    order = sorted(range(len(ids)), key=lambda i: (-remainders[i], ids[i]))
    for i in order[:n-sum(counts)]:
        counts[i] += 1
    component_counts = counts[:len(weights)]
    need(all(value > 0 for value in component_counts), 'Hamilton allocation leaves an empty generating class')
    noise_count = counts[-1] if noise else 0
    need(sum(component_counts)+noise_count == n, 'allocation conserves total n')
    return component_counts, noise_count, dict(
        method='Hamilton_largest_remainders_single_global_allocation',
        tie_rule='ascending_label_ID_noise_minus1_before_positive_components',
        label_ids=ids, probability_numerators=numerators, probability_denominator=denominator,
        floors=[n*value//denominator for value in numerators], remainders=remainders,
        allocated_counts=counts, size_monotonicity_claimed=False)


def _grid_layout(groups):
    grid = list(itertools.product((-3, -1, 1, 3), repeat=3))
    selected = [grid[0]]
    while len(selected) < groups:
        remaining = [point for point in grid if point not in selected]
        distance = lambda point: min(sum((a-b)**2 for a,b in zip(point, prior)) for prior in selected)
        selected.append(max(remaining, key=distance))  # lexicographic tie, exact integers
    minimum = min((sum((a-b)**2 for a,b in zip(x,y))
                   for i,x in enumerate(selected) for y in selected[:i]), default=None)
    centered = np.asarray(selected, dtype=np.float64)
    centered -= centered.mean(axis=0)
    if minimum is not None:
        centered /= math.sqrt(minimum)
    return centered, selected, minimum


def unit_layout(groups):
    """Center zero; minimum pair distance one for G>=2, singleton at zero."""
    return _grid_layout(_integer(groups, 'groups', 1, MAX_GROUPS))[0]


def _exact_affine_rank(points):
    rows = [[Fraction(a-b) for a,b in zip(point, points[0])] for point in points[1:]]
    rank = 0
    for column in range(3):
        pivot = next((i for i in range(rank, len(rows)) if rows[i][column]), None)
        if pivot is None:
            continue
        rows[rank],rows[pivot] = rows[pivot],rows[rank]
        divisor = rows[rank][column]
        rows[rank] = [value/divisor for value in rows[rank]]
        for i in range(rank+1,len(rows)):
            coefficient = rows[i][column]
            rows[i] = [a-coefficient*b for a,b in zip(rows[i],rows[rank])]
        rank += 1
    return rank


class _Streams:
    def __init__(self, seed, groups):
        self.seed, self.groups = seed, groups
        self.records = []

    def open(self, purpose, component=0):
        entropy = [self.seed & (2**32-1), self.seed >> 32, self.groups, STREAMS[purpose], component]
        sequence = np.random.SeedSequence(entropy)
        rng = np.random.Generator(np.random.PCG64(sequence))
        record = dict(purpose=purpose, component_label=component, entropy=entropy,
                      spawn_key=list(sequence.spawn_key), pool_size=sequence.pool_size,
                      initial_state=deepcopy(rng.bit_generator.state))
        self.records.append(record)
        return rng, record

    @staticmethod
    def close(rng, record):
        record['final_state'] = deepcopy(rng.bit_generator.state)

    def rotation(self, purpose, component=0):
        rng, record = self.open(purpose, component)
        matrix, triangular = np.linalg.qr(rng.standard_normal((3,3)))
        matrix = matrix @ np.diag(np.where(np.diag(triangular) < 0, -1.0, 1.0))
        if np.linalg.det(matrix) < 0:
            matrix[:,-1] *= -1
        self.close(rng, record)
        return matrix


def _rows_transform(rows, transform):
    # Fixed scalar expression per row avoids n-dependent BLAS kernels.
    return (rows[:,0,None]*transform[:,0] + rows[:,1,None]*transform[:,1]) + rows[:,2,None]*transform[:,2]


def _digest(array, dtype):
    return hashlib.sha256(np.asarray(array, dtype=dtype).tobytes(order='C')).hexdigest()


def generate(spec):
    """Return all n shuffled original points and declared model provenance.

    Separation is minimum center distance, not a Mahalanobis separation or
    a guarantee of separated density modes. For rings it is center spacing
    relative to radius one, not a convexity or separability certificate.
    Noise is uniform on a MODEL box, includes possible in-hull points, and
    is not derived by thresholding/filtering any realized points.
    """
    spec = validate_spec(spec)
    n,g,seed,family = (spec[key] for key in ('n','groups','seed','family'))
    weights = [4 if i % 2 == 0 else 1 for i in range(g)] if family == 'unbalanced' else [1]*g
    counts,noise_count,allocation = allocate_counts(n,weights,spec['noise_fraction'])
    streams = _Streams(seed,g)
    rotation = streams.rotation('layout_rotation')
    unit,grid,minimum_integer = _grid_layout(g)
    centers = _rows_transform(unit,rotation)
    with np.errstate(over='raise', invalid='raise', under='ignore'):
        try:
            means = spec['separation']*centers
        except FloatingPointError as exc:
            raise ValueError('model centers overflow binary64') from exc
    need(np.isfinite(means).all(), 'finite model centers required')
    rotations,transforms,covariances,standard_deviations,extents = [],[],[],[],[]
    for j in range(g):
        component_rotation = streams.rotation('component_rotation',j+1)
        std = [2.0,1.0,0.5] if family == 'anisotropic' else [(.5,1.,2.)[j % 3]]*3 if family == 'heteroscedastic' else [1.0]*3
        transform = component_rotation @ np.diag(std)
        if family == 'rings':
            covariance = .5*(component_rotation[:,:2] @ component_rotation[:,:2].T)+.05**2*np.eye(3)
            extent = np.sqrt(np.sum(component_rotation[:,:2]**2,axis=1)) + 6*.05
        else:
            covariance = transform @ transform.T
            extent = 6*np.sqrt(np.diag(covariance))
        rotations.append(component_rotation); transforms.append(transform)
        covariances.append(covariance); standard_deviations.append(std); extents.append(extent)
    with np.errstate(over='raise',invalid='raise'):
        try:
            lower = np.min(means-np.asarray(extents),axis=0)
            upper = np.max(means+np.asarray(extents),axis=0)
            width = upper-lower
        except FloatingPointError as exc:
            raise ValueError('model noise box overflows binary64') from exc
    need(np.isfinite(lower).all() and np.isfinite(upper).all() and np.isfinite(width).all() and np.all(width > 0),
         'finite positive model noise box required')
    pieces,labels,draw_indices,draw_hashes = [],[],[],[]
    for j,count in enumerate(counts):
        rng,record = streams.open('component_normal',j+1)
        normals = rng.standard_normal((count,3)); streams.close(rng,record)
        draw_record = dict(component_label=j+1,normal_sha256=_digest(normals,'<f8'))
        if family == 'rings':
            rng,record = streams.open('component_angle',j+1)
            angles = 2*math.pi*rng.random(count); streams.close(rng,record)
            # Scalar libm calls make the prefix independent of vector tail length.
            cosine = np.fromiter((math.cos(float(x)) for x in angles),dtype=np.float64,count=count)
            sine = np.fromiter((math.sin(float(x)) for x in angles),dtype=np.float64,count=count)
            offsets = cosine[:,None]*rotations[j][:,0]+sine[:,None]*rotations[j][:,1]+.05*normals
            draw_record['angle_sha256'] = _digest(angles,'<f8')
        else:
            offsets = _rows_transform(normals,transforms[j])
        with np.errstate(over='raise',invalid='raise'):
            try:
                component = means[j]+offsets
            except FloatingPointError as exc:
                raise ValueError('realized coordinates overflow binary64') from exc
        pieces.append(component); labels.append(np.full(count,j+1,dtype=np.int64))
        draw_indices.append(np.arange(count,dtype=np.int64)); draw_hashes.append(draw_record)
    rng,record = streams.open('uniform_noise')
    uniform = rng.random((noise_count,3)); streams.close(rng,record)
    pieces.append(lower+uniform*width); labels.append(np.full(noise_count,-1,dtype=np.int64))
    draw_indices.append(np.arange(noise_count,dtype=np.int64))
    points = np.concatenate(pieces); truth = np.concatenate(labels); draw_index = np.concatenate(draw_indices)
    rng,record = streams.open('output_shuffle')
    permutation = rng.permutation(n); streams.close(rng,record)
    points = np.ascontiguousarray(points[permutation],dtype=np.float64)
    truth = np.ascontiguousarray(truth[permutation],dtype=np.int64)
    need(points.shape == (n,3) and truth.shape == (n,) and np.isfinite(points).all(), 'finite complete output')
    observed_distance = min((math.dist(a,b) for i,a in enumerate(means) for b in means[:i]),default=None)
    need(g == 1 or observed_distance > 0, 'positive center separation lost in binary64')
    parameters = dict(schema=SCHEMA,**spec,dimension=3,means=means.tolist(),centers=means.tolist(),
        means_scope='declared distribution expectations, not fitted empirical means',
        covariances=np.asarray(covariances).tolist(),covariance_scope='population distribution covariance',
        component_counts=counts,true_counts=counts,noise_count=noise_count,injected_noise_count=noise_count,
        realized_noise_fraction=noise_count/n,component_weights=weights,
        component_priors_conditional=[w/sum(weights) for w in weights],
        component_priors_unconditional=[(1-spec['noise_fraction'])*w/sum(weights) for w in weights],
        realized_component_fractions=[count/n for count in counts],allocation=allocation,
        true_rank=3,component_covariance_ranks=[3]*g,
        centers_affine_rank=_exact_affine_rank(grid),
        latent_manifold_dimension=1 if family == 'rings' else 3,
        rank_scope='positive Gaussian variance/jitter gives full ambient distribution rank; not empirical finite-sample rank',
        component_standard_deviations=None if family == 'rings' else standard_deviations,
        ring_radius=1.0 if family == 'rings' else None,ring_jitter_std=.05 if family == 'rings' else None,
        layout=dict(rule='fixed_4cubed_grid_farthest_first_integer_distances_lexicographic_ties',
                    integer_grid_points=[list(point) for point in grid],minimum_integer_squared_distance=minimum_integer,
                    rotated_unit_centers=centers.tolist(),common_rotation=rotation.tolist()),
        component_rotations=np.asarray(rotations).tolist(),minimum_mean_distance_observed=observed_distance,
        separation_interpretation=('center distance relative to ring radius 1; not surface gap/separability/nonconvexity guarantee'
            if family == 'rings' else 'minimum Euclidean center distance in reference std=1 units; not Mahalanobis/density-mode separation'),
        noise_bbox=dict(lower=lower.tolist(),upper=upper.tolist(),
            model_rule='union AABB of center +/- (projected ring radius + six jitter std)' if family == 'rings'
                       else 'union AABB of center +/- six marginal population standard deviations',
            independent_of_n_and_realized_draws=True,uniform_entire_box=True,reject_in_hull_points=False,
            caution='injected noise may lie inside component hulls or overlap components; Gaussian tails are unbounded'),
        truth_labels='generating classes 1..G; injected uniform noise -1, no geometric relabeling',
        component_draw_index=draw_index[permutation].tolist(),
        shuffle=dict(independent_stream=True,uses_coordinates_or_truth=False,
                     permutation_sha256=_digest(permutation,'<i8'),policy='one full permutation of all n generated rows; no rejection/filtering'),
        generator='numpy.PCG64 with explicit SeedSequence entropy',numpy_version=np.__version__,seed_states=streams.records,
        standardized_draws=draw_hashes,uniform_noise_unit_draws_sha256=_digest(uniform,'<f8'),
        pairing='same component draw prefixes for fixed seed/G/family/separation, up to min allocated counts; undo shuffle by label and component_draw_index',
        whole_scene_growth_inclusion_claimed=False,across_groups_pairing_claimed=False,
        quantized=False,clustering_used=False,truth_used_for_parameter_choice=False)
    return points,truth,parameters
