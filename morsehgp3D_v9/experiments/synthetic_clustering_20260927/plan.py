"""Fixed synthetic design, declared before any new clustering labels."""

QUALITY_SEEDS = (2026092801, 2026092802)
GROWTH_SEED = 2026092891


def _case(phase, axis, family, n, groups, separation, noise, seed, replicate):
    tag = lambda value: str(value).replace('.', 'p')
    return dict(id=f'{phase}_{family}_n{n}_g{groups}_d{tag(separation)}_noise{tag(noise)}_s{replicate}',
                phase=phase, axis=axis, replicate=replicate,
                spec=dict(n=n, groups=groups, separation=separation, noise_fraction=noise,
                          family=family, seed=seed))


def cases():
    scenarios = [('size', 'spherical', n, 8, 4, 0) for n in (400, 800, 1600)]
    scenarios += [('groups', 'spherical', 800, g, 4, 0) for g in (2, 4, 16, 32)]
    scenarios += [('difficulty', 'spherical', 800, 8, d, 0) for d in (2, 8)]
    scenarios += [('shape_density', family, 800, 8, 4, 0)
                  for family in ('anisotropic', 'unbalanced', 'heteroscedastic')]
    scenarios += [('nonconvex', 'rings', 800, 8, d, 0) for d in (1.5, 3, 6)]
    scenarios += [('noise', family, 800, 8, d, 0.1) for family, d in (('spherical', 4), ('rings', 3))]
    rows = [_case('quality', *scenario, seed, replicate)
            for replicate, seed in enumerate(QUALITY_SEEDS, 1) for scenario in scenarios]
    rows += [_case('growth', 'size', family, n, 8, d, 0, GROWTH_SEED, 1)
             for family, d in (('spherical', 4), ('anisotropic', 4), ('rings', 3))
             for n in (8000, 16000, 32000)]
    return rows


PLAN = dict(schema='mhgp9_synthetic_clustering_plan_v1', cases=cases(),
    quality=dict(cases=34, scenarios=17, seeds=list(QUALITY_SEEDS), k=[5],
        min_cluster_size=[20, 50], exp_z=[1, 2],
        primary=dict(k=5, min_cluster_size=20, exp_z=1),
        methods=['hgp_exclusive_point_routing', 'hgp_weighted_full_vote',
                 'hgp_first_coverage', 'hdbscan_common', 'hdbscan_standard'],
        standard_exp_z=[1], expected_rows=612, expected_rows_per_case=18),
    growth=dict(cases=9, n=[8000, 16000, 32000], seed=GROWTH_SEED,
        status='inputs_prepared_only_until_explicit_closed_execution',
        repeats_required_for_timing=3,
        primary_work='geometric_candidates_facets_incidences_source_nodes_then_projection',
        scaling_interpretation='measured_regime_only_never_a_global_complexity_bound'),
    root_policy='excluded_for_common_EOM_no_noise_filling',
    statistical_scope='predeclared_new_synthetic_draws_frozen_methods_two_replicates_not_universal_dominance',
    coordinate_profile='common_isotropic_u18_per_family_G_delta_noise_seed_not_metres_or_SemanticKITTI',
    pairing='component_streams_independent_of_n; common_grid_across_sizes; no_subsampling_or_merging',
    failure_policy='preserve_failed_units_and_complete_prescribed_inventory_no_favourable_case_filter')
