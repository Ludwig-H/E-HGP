#!/usr/bin/env python3
"""Worker invite de la tour FULL v9 (reference_cpu), inerte sans --execute.

Port explicite de q34_spatial_worker_v8.py (SHA 39596c7607370e27...,
commit 70de84f2, lui-meme port de cpu_probe_worker_v8.py) : memes primitives
de garde et de commande, reprises du worker v7 epingle par SHA256
(gcp-migration/full_probe_worker_v7.py, da967163...). Son producteur FULL v7, sa recette de compilation et son
lecteur ne sont jamais appeles ; le fichier v7 n'est pas modifie.

Ce qui change pour la v9 :
- la construction passe par le CMake de morsehgp3D_v9 (cible unique
  mhgp9_tower_probe), sans modifier le CMakeLists ni affaiblir -Werror ;
- les entrees sont les trois trames LiDAR sans sol a 1 mm, fichiers entiers
  epingles (sha256, taille, empreinte FNV-1a de la sonde) ;
- chaque cas du plan a un plafond propre ; un cas qui l'atteint est tue et
  consigne, les suivants sont sautes quand le budget utile est epuise ;
- chaque cas epingle tous les leviers de la chaine (`levers`, noms exacts :
  meme objet, travail different), passes a la sonde par `--lever=NOM=0|1` et
  relus dans sa sortie ;
- une sortie de sonde refusee par le validateur est un defaut deterministe
  de protocole : les cas suivants sont sautes au lieu de repeter le calcul
  (session G4 R2 du 23 septembre 2026, treize cas refuses pour un champ).

Aucune installation de paquet, aucun reboot, aucune mutation cloud, aucun
CUDA, aucun ELF local transporte. public_status reste not_claimed : une
mesure, meme complete, ne certifie ni la tour ni le contrat 1 s / 100 ms.
"""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path, PurePosixPath
import re
import shlex
import shutil
import signal
import struct
import sys
import time
import types

TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
HELPER = 'gcp-migration/full_probe_worker_v7.py'
HELPER_SHA = 'da967163bdb7247bc6aad4df0c294cda1071076a0127cd5bd9f59bc0e4788439'
PLAN = 'data/session_plan.json'
PROVENANCE = 'data/provenance.json'
PLAN_SCHEMA = 'mhgp9_tower_plan_v6'
PROVENANCE_SCHEMA = 'mhgp9_tower_provenance_v1'
PROBE_SCHEMA = 'mhgp9_tower_probe_v21'
PROTOCOL_NAMES = frozenset('gcp-migration/tower_' + name + '_v9.py' for name in
                           ('worker', 'session', 'snapshot', 'selftest'))
SOURCE_ROOT = 'morsehgp3D_v9'
CMAKE_LISTS = SOURCE_ROOT + '/CMakeLists.txt'
PROBE_SOURCE = SOURCE_ROOT + '/bench/tower_probe.cpp'
CHAIN_SOURCE = SOURCE_ROOT + '/src/chain/tower_chain.cpp'
# Le CMake v9 enregistre aussi les portes (tests/, oracle/) : leurs sources
# doivent exister a la configuration, meme si seule la sonde est construite.
SOURCE_PREFIXES = tuple(SOURCE_ROOT + '/' + name + '/' for name in ('cmake', 'src', 'bench', 'tests', 'oracle'))
REQUIRED_SOURCES = frozenset({CMAKE_LISTS, SOURCE_ROOT + '/cmake/run_expect.cmake', PROBE_SOURCE,
                              CHAIN_SOURCE, SOURCE_ROOT + '/src/chain/tower_chain.hpp'})
PROBE_TARGET = 'mhgp9_tower_probe'
INPUT_ROOT = 'morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6'
# Trames entieres (jamais un prefixe). fnv = empreinte FNV-1a 64 imprimee par
# la sonde (n puis x, y, z en u64 LE) ; celle de la scene 00 est recoupee
# avec la sortie C++ de mhgp9_tower_probe a d2700314 (5c785760053d17ce).
INPUTS = {
    '00': dict(file='data/scene_00.u32le', source=INPUT_ROOT + '/scene_00_grid/full.u32le', n=39885,
               sha256='0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf', fnv='5c785760053d17ce'),
    '01': dict(file='data/scene_01.u32le', source=INPUT_ROOT + '/scene_01_grid/full.u32le', n=35551,
               sha256='ba15adc6907d58e50bf28bca92305210c1efdde6efdf46c782aa1eec2318036f', fnv='4210173194931f58'),
    '02': dict(file='data/scene_02.u32le', source=INPUT_ROOT + '/scene_02_grid/full.u32le', n=45845,
               sha256='a4bbc86d00f92627b869fdc34aa260353bf1b821eff7c992ad93beb2a13308af', fnv='1c41bd0d1d689300'),
}
TIME = '/usr/bin/time'
BOOST_HEADER = '/usr/include/boost/multiprecision/cpp_int.hpp'
BOOST_ROOT = '/usr'
BUILD_PARALLEL = '48'
# Garde fixe : maxRunDuration 3600 s (valide sur la cible, jamais modifie).
# Arret invite 40 min et non 30 comme en v8 : avec 30 min, l'arret invite
# tombe a ~1800 s du depart et la marge de fermeture de 300 s ne laisse que
# ~1300-1400 s apres transfert, donc 1500 s utiles seraient une promesse
# fausse. 40 min = 2400 s couvrent 1500 s utiles + 300 s de fermeture + les
# ~150-300 s de certification/transfert ; et 40*60 + 300 (reserve GCE) +
# 120 (tolerance systemd) + 480 (armement) = 3300 <= 3600, l'inegalite
# imposee par start_and_verify.sh (et guest*60 + 900 <= 3600 exigee par la
# primitive v7 guard_values) tient avec 300 s de marge.
MAX_RUN_SECONDS = '3600'
GUEST_SHUTDOWN_MINUTES = '40'
USEFUL_BUDGET_SECONDS = 1500   # defaut et plafond dur du worker
CASE_CAP_SECONDS = 600         # plafond par cas : un K10 qui explose laisse vivre les autres cas
MIN_CASE_START_SECONDS = 15    # aucun cas n'est lance avec moins de temps utile restant
PROBE_STATUSES = ('complete_relative', 'unsupported_degeneracy', 'invalid_input',
                  'resource_exhausted', 'invariant_violated')
OUTCOMES = ('complete_relative', 'explicit_refusal', 'killed_case_cap', 'killed_budget',
            'skipped_budget', 'probe_failed', 'skipped_protocol_defect')
CASE_KEYS = frozenset({'scene', 'file', 'n', 'k', 's', 'workers', 'static_threads', 'levers', 'repeat'})
LEVER_NAMES = ('atlas_saturate_deep', 'q3_leaf_census', 'q34_dead_lanes', 'q34_witness_cache', 'q34_dead_core',
               'tower_meb_proposal', 'q34_jobs_by_mass', 'q34_fine_jobs', 'tower_overlap_static',
               'q2_jobs_by_mass', 'q34_batch_filter', 'q34_gpu_filter', 'q34_batch_certificates',
               'q34_gpu_certificates', 'q34_batch_q3', 'q34_gpu_q3', 'q34_batch_q4')
# v17 (S2) : filtre q3/q4 par lots, puis sur GPU. Le build G4 active CUDA
# (nvcc et nvidia-smi existants, aucune installation) ; l'appareil attendu :
DEVICE_NAME = 'NVIDIA RTX PRO 6000 Blackwell Server Edition'
CUDA_PATHS = ('/usr/local/cuda/bin/nvcc', '/usr/local/cuda-12.9/bin/nvcc')
BATCH_KEYS = frozenset({'used', 'backend', 'front_ms', 'filter_ms', 'edges_ms', 'device_ms', 'rectangles', 'survivors',
                        'certificate_backend', 'certificate_ms', 'certificate_device_ms', 'deferred', 'judged_edges',
                        'rebuilt_covers', 'certificate_warps', 'filter_kernel_ms', 'filter_transfer_ms',
                        'certificate_kernel_ms', 'certificate_transfer_ms', 'lanes_backend', 'lanes_ms',
                        'lanes_device_ms', 'lanes_kernel_ms', 'lanes_transfer_ms', 'lanes_wait_ms', 'tail_ms',
                        'lanes_asked', 'lanes_decided', 'lanes_deferred', 'lanes_records', 'lanes_judged',
                        'lanes_warps'})
# v20 (S4a) : voie q3 des survivants certifies par lots, sans atlas (CPU ou
# GPU). Chronos et comptes de l'appel, registre declare lanes_* du mode.
LANES_TIMES = ('lanes_ms', 'lanes_device_ms', 'lanes_kernel_ms', 'lanes_transfer_ms', 'lanes_wait_ms', 'tail_ms')
LANES_COUNTS = ('lanes_asked', 'lanes_decided', 'lanes_deferred', 'lanes_records', 'lanes_judged', 'lanes_warps')
LANES_LEDGER = ('lanes_edges', 'lanes_cover_sites', 'lanes_cover_node_visits', 'lanes_seed_tests',
                'lanes_acute_sites', 'lanes_owner_rejections', 'lanes_seeds', 'lanes_census_point_tests',
                'lanes_census_inside_sites', 'lanes_census_shell_sites', 'lanes_census_outside_sites',
                'lanes_depth_rejections', 'lanes_emitted', 'lanes_shell_ids', 'lanes_q3_edges', 'lanes_census_seeds')
# v21 (S4b) : registre declare des voies q4 de l'appel (lentilles, groupes).
LANES4_LEDGER = tuple('lanes4_' + name for name in (
    'edges seeds certified certified_chunk1 survivors pass_chunks pass_site_tests buffered_events max_buffered '
    'live_buckets filter_steps bucket_events candidates foreign_candidates groups compare_steps '
    'depth_rejected_groups positivity_tests groups_without_valid emitted emitting_seeds multi_emission_seeds '
    'max_emissions_per_seed shell_ids max_group constant_shell_sites list_steps group_steps').split())
# v18 (S3) : certificats de voie morte par lots (CPU ou GPU). Leur travail ne
# depend que des leviers q34_dead_lanes/q34_dead_core : deux cas du meme
# (fichier, K, s) avec ces leviers egaux doivent avoir ces compteurs egaux.
CERTIFICATE_WORK_KEYS = ('expanded_pairs', 'witness_rejected_pairs', 'cover_builds', 'cover_sites',
                         'cover_node_visits', 'q3_edges', 'q4_edges', 'both_edges', 'dead_loads', 'dead_form_sites',
                         'dead_cells', 'dead_outside_cells', 'dead_deep_cells', 'dead_failed_cells',
                         'dead_uniform_tests', 'dead_point_tests', 'dead_q3_proved', 'dead_q3_open',
                         'dead_q4_proved', 'dead_q4_open', 'core_builds', 'core_sites', 'core_closed_edges',
                         'dead_core_loads', 'dead_core_form_sites', 'dead_core_cells', 'dead_core_uniform_tests',
                         'dead_core_point_tests', 'dead_core_q3_proved', 'dead_core_q3_open', 'dead_core_q4_proved',
                         'dead_core_q4_open', 'core_cover_node_visits', 'core_cover_bound_tests',
                         'core_cover_point_tests', 'dead_core_outside_cells', 'dead_core_deep_cells',
                         'dead_core_failed_cells')
TOP_KEYS = frozenset({'schema', 'status', 'reason', 'input', 'options', 'times_ms', 'chain_cpu_s', 'generator',
                      'ledger', 'catalogue', 'q34_occupancy', 'q34_batch', 'tower_phases_ms', 'tower_work', 'orders',
                      'tower_digest', 'catalogue_digest', 'presentation_digest', 'peak_rss_kb'})
INPUT_KEYS = frozenset({'format', 'grid', 'sites', 'hash'})
OPTION_KEYS = frozenset({'K', 'K_effective', 's', 'workers', 'tower_static_threads', 'run_tower', 'certificate_capacity',
                         'certificate_judge', 'lanes_capacity', 'lanes_judge', 'lanes_events', 'levers'})
# v18 : ardoise par warp des certificats GPU (defaut de gpu/filter_runner.hpp).
# Une trame plus petite ne peut rien mettre en attente ; le preflight de mise
# en attente rejoue le preflight GPU avec une ardoise de 64 sites.
DEFAULT_CERTIFICATE_CAPACITY = 1 << 16
DEFERRAL_CAPACITY = 64
# v20 : ardoise (sites) des voies q3 GPU, defaut de gpu/filter_runner.hpp ; le
# preflight de mise en attente la reduit a 24 sites (moins que l'ardoise des
# certificats, sinon toute arete decidee par S3 tiendrait dans celle des voies).
DEFAULT_LANES_CAPACITY = 1 << 16
LANES_DEFERRAL_CAPACITY = 24
# v18 (auditeur C, audits/c_catalogue_digest_20260923) : condenses FULL (egaux
# a R12) et du catalogue des trois trames a s = 8, identiques sur le moteur,
# le lot CPU et les certificats S3 CPU. Tout cas de ces (scene, K, s), GPU ou
# non, doit les reproduire : une erreur commune aux deux jumeaux se voit.
PINNED_DIGESTS = {('00', 5, 8): ('67450c64611075b1', '5ad1fe09354411ba'),
                  ('00', 10, 8): ('ac108f7f71096c3f', 'a6e959d227f3dafa'),
                  ('01', 5, 8): ('dbf799c8ed83f53f', 'a4a5149c15b4122e'),
                  ('01', 10, 8): ('9ddbf7430c9086cc', 'c5cddc5b0baefcf1'),
                  ('02', 5, 8): ('8240af3d4dce3d45', '143a367b4f27ef02'),
                  ('02', 10, 8): ('ba973af0c8da95bd', '5c8cc01b1e45b461')}
TIME_KEYS = frozenset({'read', 'prepare', 'gen_index', 'q2', 'q34', 'merge', 'tower_index', 'census', 'tower',
                       'chain_total', 'digest', 'catalogue_digest'})
ORDER_KEYS = frozenset({'K', 'nodes', 'births', 'merges', 'parents', 'contributions'})
# tower_work : compteurs entiers, sauf ces deux champs types du noyau MEB
# (libelle de comptabilite epingle, histogramme des tailles de supports).
MEB_ACCOUNTING = 'anchor_meb_first_maximal_pair_then_double_welzl_proposal_exact_boundary_canonical_v3'
MEB_REFERENCE_ACCOUNTING = 'anchor_meb_first_maximal_pair_then_lexicographic_supports_extremes_first_v2'
MEB_PROPOSAL_KEYS = ('meb_proposals', 'meb_verified_proposals', 'meb_boundary_canonicalizations',
                     'meb_proposal_fallbacks')
TOWER_WORK_TYPED = frozenset({'meb_accounting', 'meb_supports_by_size'})
# Schema v5 EXACT des sections de travail (contre-audit B du protocole v4) :
# champ manquant, inconnu ou histogramme de mauvaise longueur = refus.
GENERATOR_KEYS = frozenset(('q2_front_rectangles q2_candidate_pairs q2_accepted_pairs q34_expanded_pairs '
                            'q34_cover_builds q3_emitted q4_emitted').split())
LEDGER_KEYS = frozenset((
    'expanded_pairs cover_builds cover_sites cover_node_visits q3_edges q4_edges both_edges '
    'witness_input_pair_mass witness_rejected_rectangles witness_rejected_pairs q3_seeds q3_ball_builds '
    'q3_depth_rejections q3_census_bounds q3_census_point_tests q3_atlas_edges q3_atlas_locations '
    'q3_atlas_rejections q3_atlas_outside_domain atlas_cells atlas_leaf_cells atlas_deep_cells '
    'atlas_outside_cells atlas_splits atlas_node_visits atlas_block_bounds atlas_point_tests atlas_ids_copied '
    'q4_seeds q4_live_leaves q4_whole_atlas_skips q4_sweep_events q3_leaf_censuses q3_leaf_point_tests '
    'q3_leaf_rejections q3_lower_bound_fallbacks dead_loads dead_form_sites dead_cells dead_outside_cells '
    'dead_deep_cells dead_failed_cells dead_uniform_tests dead_point_tests dead_q3_proved dead_q3_open '
    'dead_q4_proved dead_q4_open witness_cache_queries witness_cache_node_tests '
    'witness_cache_rejected_pairs core_builds core_sites core_closed_edges dead_core_loads dead_core_form_sites '
    'dead_core_cells dead_core_uniform_tests dead_core_point_tests dead_core_q3_proved dead_core_q3_open '
    'dead_core_q4_proved dead_core_q4_open core_cover_node_visits core_cover_bound_tests core_cover_point_tests '
    'dead_core_outside_cells dead_core_deep_cells dead_core_failed_cells q34_input_rectangles witness_rect_queries '
    'witness_rect_node_visits witness_pair_queries witness_pair_node_visits q3_edge_queries q3_seed_node_visits '
    'q3_seed_point_tests q3_seed_bound_tests q4_geometry_preparations q4_domain_node_visits '
    'q4_cover_decomposition_node_visits q4_seed_node_visits q4_seed_cell_queries q4_sweep_active_sites '
    'lanes_edges lanes_cover_sites lanes_cover_node_visits lanes_seed_tests lanes_acute_sites '
    'lanes_owner_rejections lanes_seeds lanes_census_point_tests lanes_census_inside_sites '
    'lanes_census_shell_sites lanes_census_outside_sites lanes_depth_rejections lanes_emitted '
    'lanes_shell_ids lanes_q3_edges lanes_census_seeds').split()) | frozenset(LANES4_LEDGER)
CATALOGUE_LISTS = dict(by_qmin=3, by_shell=17)
CATALOGUE_KEYS = frozenset(('q2_presentations q3_presentations q4_presentations unique_keys balls '
                            'extra_shell_balls shell_over_12 max_shell max_interior census_nodes census_leaf_tests '
                            'bytes by_qmin by_shell euler').split())
# Invariant d'Euler du catalogue (v13) : condition NECESSAIRE de completude.
EULER_KEYS = frozenset({'status', 'checkable_max_k', 'by_k'})
EULER_STATUSES = ('holds', 'fails', 'not_checkable')
# Occupation mesuree des ouvriers q3/q4 et chronos par phase de la tour (v13).
OCCUPANCY_COUNTS = ('started_workers', 'jobs', 'tasks_published', 'tasks_consumed', 'task_waits')
OCCUPANCY_TIMES = ('wall_max_ms', 'wall_min_ms', 'cpu_sum_s', 'wait_sum_s', 'job_sum_s', 'max_job_ms')
TOWER_PHASES = ('validate', 'static', 'lots', 'populations', 'images', 'bank', 'encode')
TOWER_PHASES_BY_K = ('static_by_k', 'lots_by_k', 'images_by_k', 'encode_by_k', 'order_by_k')
STATIC_PATH_PHASES = ('static', 'lots', 'populations', 'images')
# Arrondi des chronos imprimes a 0,001 ms ; marge relative des sommes CPU.
PHASE_TOLERANCE_MS = 0.01
TOWER_WORK_KEYS = frozenset(('records extra_records representatives anchor_hits key_lookups intruder_queries '
                             'intruder_nodes meb_calls meb_power_tests births merges contributions grouped_lots '
                             'resolver_cache_hits meb_accounting meb_pair_distances meb_materializations '
                             'meb_supports_by_size meb_proposals meb_verified_proposals '
                             'meb_boundary_canonicalizations meb_proposal_fallbacks').split())
MEB_SIZES_LENGTH = 4
# Tolerance du rapprochement chrono interne / mur externe du cas (horloges
# monotones de la meme machine ; granularite, pas une marge de contrat).
EXTERNAL_WALL_TOLERANCE_SECONDS = 0.05
# Preflight natif (lecon de R2) : la vraie sonde sur un petit nuage
# deterministe, jugee par validate_probe, avant tout cas LiDAR.
PREFLIGHT_FILE = 'preflight.u32le'
PREFLIGHT_SITES = 1500
STAGE_TIME_KEYS = ('prepare', 'gen_index', 'q2', 'q34', 'merge', 'tower_index', 'census', 'tower')
SCOPE = 'FULL_tower_chain_relative_to_cross_checked_catalogue'


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def save(path, value):
    with Path(path).open('x') as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write('\n')


def canonical_json(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), allow_nan=False).encode('ascii') + b'\n'


def strict_json(raw):
    def pairs(items):
        out = {}
        for key, value in items:
            need(key not in out, 'duplicate JSON key')
            out[key] = value
        return out

    def invalid(_):
        raise ValueError('nonfinite JSON number')

    def finite(text):
        value = float(text)
        need(math.isfinite(value), 'nonfinite JSON number')
        return value
    return json.loads(raw, object_pairs_hook=pairs, parse_constant=invalid, parse_float=finite)


def safe_name(name):
    if type(name) is not str or not name:
        return False
    path = PurePosixPath(name)
    if str(path) != name or path.is_absolute() or any(part in ('.', '..') for part in path.parts):
        return False
    return (name == CMAKE_LISTS or name.startswith(SOURCE_PREFIXES) or name == HELPER or name in PROTOCOL_NAMES or
            name in (PLAN, PROVENANCE) or name in {data['file'] for data in INPUTS.values()})


def validate_manifest(manifest):
    need(type(manifest) is dict and manifest, 'payload manifest object')
    for name, pin in manifest.items():
        need(safe_name(name) and type(pin) is str and re.fullmatch('[0-9a-f]{64}', pin), 'unsafe payload path/hash')
    need(REQUIRED_SOURCES | PROTOCOL_NAMES | {HELPER, PLAN, PROVENANCE} |
         {data['file'] for data in INPUTS.values()} <= set(manifest), 'required payload absent')
    need(manifest[HELPER] == HELPER_SHA, 'legacy helper pin')
    need(all(manifest[data['file']] == data['sha256'] for data in INPUTS.values()), 'pinned LiDAR frame hash')


def validate_sources(read_bytes):
    """Controle statique bon marche : cible et interface CLI attendues au commit."""
    cmake = read_bytes(CMAKE_LISTS)
    probe = read_bytes(PROBE_SOURCE)
    need(b'mhgp9_product_executable(' + PROBE_TARGET.encode() + b' bench/tower_probe.cpp)' in cmake,
         'CMake target mhgp9_tower_probe absent')
    need(all(token in probe for token in (PROBE_SCHEMA.encode(), b'"--s="', b'"--static="', b'"--grid="', b'"--catalogue-digest"',
                                          b'"--lever="', *(b'"' + name.encode() + b'"' for name in LEVER_NAMES))),
         'tower probe schema/CLI differs from the v9 protocol')


def _integer(value, low, high):
    return type(value) is int and low <= value <= high


def _levers(value):
    # Le noyau diametral n'existe que sous le certificat de voie morte ; le
    # filtre GPU n'existe que sur le chemin par lots ; les certificats par lots
    # exigent le chemin par lots et le certificat, leur GPU les exige.
    return (type(value) is dict and set(value) == set(LEVER_NAMES) and
            all(type(item) is bool for item in value.values()) and
            (value['q34_dead_lanes'] or not value['q34_dead_core']) and
            (value['q34_batch_filter'] or not value['q34_gpu_filter']) and
            ((value['q34_batch_filter'] and value['q34_dead_lanes']) or not value['q34_batch_certificates']) and
            (value['q34_batch_certificates'] or not value['q34_gpu_certificates']) and
            (value['q34_batch_certificates'] or not value['q34_batch_q3']) and
            (value['q34_batch_q3'] or not value['q34_gpu_q3']) and
            (value['q34_batch_q3'] or not value['q34_batch_q4']))


def uses_device(levers):
    return levers['q34_gpu_filter'] or levers['q34_gpu_certificates'] or levers['q34_gpu_q3']


def plan_uses_gpu(cases):
    return any(uses_device(case['levers']) for case in cases)


def engine_levers(levers):
    """The same levers on the engine path (no batch call, no device)."""
    return dict(levers, q34_batch_filter=False, q34_gpu_filter=False, q34_batch_certificates=False,
                q34_gpu_certificates=False, q34_batch_q3=False, q34_gpu_q3=False, q34_batch_q4=False)


def lever_arguments(case):
    return ['--lever=' + name + '=' + ('1' if case['levers'][name] else '0') for name in LEVER_NAMES]


def validate_plan(plan, manifest):
    need(type(plan) is dict and set(plan) == {'schema', 'cases'} and plan['schema'] == PLAN_SCHEMA and
         type(plan['cases']) is list and 0 < len(plan['cases']) <= 64, 'tower plan schema')
    seen = set()
    for case in plan['cases']:
        need(type(case) is dict and set(case) == CASE_KEYS, 'tower case fields')
        need(type(case['scene']) is str and case['scene'] in INPUTS and
             case['file'] == INPUTS[case['scene']]['file'] and case['file'] in manifest, 'tower scene/file identity')
        need(case['n'] == INPUTS[case['scene']]['n'] and type(case['n']) is int, 'whole-frame size; prefixes forbidden')
        need(type(case['k']) is int and case['k'] in (5, 10) and type(case['s']) is int and case['s'] in (8, 10, 12) and
             _integer(case['workers'], 1, 48) and _integer(case['static_threads'], 0, 48) and
             _levers(case['levers']) and
             _integer(case['repeat'], 0, (1 << 32) - 1), 'tower case domain')
        identity = tuple(case[key] for key in ('scene', 'k', 's', 'workers', 'static_threads', 'repeat')) + tuple(
            case['levers'][name] for name in LEVER_NAMES)
        need(identity not in seen, 'duplicate case needs an explicit distinct repetition')
        seen.add(identity)
    # The preflight runs the levers of the first case: pin them all ON, so
    # that every lever a later case may enable has been exercised first.
    need(all(plan['cases'][0]['levers'][name] for name in LEVER_NAMES),
         'the first case sets the preflight levers and must pin every lever ON')
    # v17: every (frame, K, s) run on the batch path has an engine-path twin,
    # so that the cross-case object comparison judges the batch/GPU tower.
    # v18: with the same certificate levers, so that its certificate work is
    # compared too.
    twin = lambda c: (c['scene'], c['k'], c['s'], c['levers']['q34_dead_lanes'], c['levers']['q34_dead_core'])
    engine = {twin(c) for c in plan['cases'] if not c['levers']['q34_batch_filter']}
    need(all(twin(c) in engine for c in plan['cases'] if c['levers']['q34_batch_filter']),
         'a batch/GPU case without an engine-path twin on the same frame, K, s and certificate levers')
    return plan['cases']


_FNV_PRIME = 1099511628211
_FNV_MASK = (1 << 64) - 1
_FNV_ZERO5 = pow(_FNV_PRIME, 5, 1 << 64)


def input_fnv(raw):
    """FNV-1a 64 de mhgp9_tower_probe : n puis chaque coordonnee en u64 LE."""
    need(len(raw) % 12 == 0, 'u32le incomplete site record')
    h = 14695981039346656037
    for byte in (len(raw) // 12).to_bytes(8, 'little'):
        h = ((h ^ byte) * _FNV_PRIME) & _FNV_MASK
    for (value,) in struct.iter_unpack('<I', raw):
        need(value < (1 << 18), 'coordinate outside [0, 2^18)')
        h = ((h ^ (value & 255)) * _FNV_PRIME) & _FNV_MASK
        h = ((h ^ ((value >> 8) & 255)) * _FNV_PRIME) & _FNV_MASK
        h = ((h ^ (value >> 16)) * _FNV_PRIME) & _FNV_MASK
        h = (h * _FNV_ZERO5) & _FNV_MASK   # cinq octets nuls de poids fort
    return '%016x' % h


def validate_data(read_bytes):
    for data in INPUTS.values():
        raw = read_bytes(data['file'])
        need(hashlib.sha256(raw).hexdigest() == data['sha256'] and len(raw) == 12 * data['n'] and
             input_fnv(raw) == data['fnv'], 'pinned whole LiDAR frame differs')


def validate_provenance(value, manifest):
    need(type(value) is dict and set(value) == {'schema', 'commit', 'tree', 'protocol_source', 'inputs', 'helper'} and
         value['schema'] == PROVENANCE_SCHEMA, 'provenance schema')
    need(all(type(value[key]) is str and re.fullmatch('[0-9a-f]{40}|[0-9a-f]{64}', value[key])
             for key in ('commit', 'tree')), 'provenance commit/tree')
    need(value['protocol_source'] in ('commit', 'worktree_uncommitted'), 'protocol source')
    need(value['inputs'] == {data['file']: dict(commit_path=data['source'], sha256=data['sha256'])
                             for data in INPUTS.values()}, 'provenance inputs')
    need(value['helper'] == dict(path=HELPER, sha256=HELPER_SHA) and manifest.get(HELPER) == HELPER_SHA,
         'provenance helper')
    return value


def validate_runtime(manifest):
    # L'executable est televerse comme remote/worker.py, hors de source/ :
    # epingler ce code reel en plus de sa copie archivee.
    need(sha(__file__) == manifest.get('gcp-migration/tower_worker_v9.py'),
         'executing worker differs from transported worker source')


def source_map(root, manifest):
    validate_manifest(manifest)
    out = {}
    for name, pin in manifest.items():
        path = root / name
        need(path.is_file() and not path.is_symlink() and path.resolve().is_relative_to(root), 'payload file type')
        out[name] = sha(path)
        need(out[name] == pin, 'payload changed: ' + name)
    return out


def load_helper(root):
    path = root / HELPER
    raw = path.read_bytes()
    need(hashlib.sha256(raw).hexdigest() == HELPER_SHA, 'worker helper changed')
    module = types.ModuleType('mhgp9_pinned_worker_primitives')
    module.__file__ = str(path)
    exec(compile(raw, str(path), 'exec'), module.__dict__)
    return module


def available_cpus():
    return sorted(os.sched_getaffinity(0))


def boot_epoch():
    return time.time() - float(Path('/proc/uptime').read_text().split()[0])


def probe_command(build, root, case):
    return [str(build / PROBE_TARGET), str(root / case['file']), str(case['k']), str(case['workers']),
            '--s=' + str(case['s']), '--static=' + str(case['static_threads']), '--grid=1mm',
            '--catalogue-digest', *lever_arguments(case)]


def expected_probe_tail(case, capacity=0, judge=False, lanes_capacity=0):
    # v20: `judge` judges every batch call of the case (certificates, and the
    # q3 lanes when that lever is on).
    return [str(case['k']), str(case['workers']), '--s=' + str(case['s']),
            '--static=' + str(case['static_threads']), '--grid=1mm', '--catalogue-digest',
            *(['--certificate-capacity=' + str(capacity)] if capacity else []),
            *(['--certificate-judge'] if judge else []),
            *(['--lanes-capacity=' + str(lanes_capacity)] if lanes_capacity else []),
            *(['--lanes-judge'] if judge and case['levers']['q34_batch_q3'] else []), *lever_arguments(case)]


def deferral_capacities(levers):
    """v20: reduced slabs of the deferral preflight (0 = not reduced): the
    device certificates and the device q3 lanes, each when its lever is on."""
    return (DEFERRAL_CAPACITY if levers['q34_gpu_certificates'] else 0,
            LANES_DEFERRAL_CAPACITY if levers['q34_gpu_q3'] else 0)


def judged_preflight(levers):
    """v18: the preflights judge the certificate call edge by edge (CPU
    reference); v20: and the q3 lanes call (the engine's q3 lane)."""
    return levers['q34_batch_certificates']


def _count(value):
    return type(value) is int and 0 <= value < (1 << 64)


def _number(value):
    return type(value) in (int, float) and math.isfinite(value) and value >= 0


def _tower_work(value):
    if type(value) is not dict or set(value) != TOWER_WORK_KEYS:
        return False
    for name, item in value.items():
        if name == 'meb_accounting':
            if item not in (MEB_ACCOUNTING, MEB_REFERENCE_ACCOUNTING):
                return False
        elif name == 'meb_supports_by_size':
            if type(item) is not list or len(item) != MEB_SIZES_LENGTH or not all(_count(x) for x in item):
                return False
        elif not _count(item):
            return False
    return True


def _counters(value, keys):
    return type(value) is dict and set(value) == keys and all(_count(item) for item in value.values())


def _catalogue(value):
    if type(value) is not dict or set(value) != CATALOGUE_KEYS:
        return False
    for name, item in value.items():
        if name == 'euler':
            if (type(item) is not dict or set(item) != EULER_KEYS or item['status'] not in EULER_STATUSES or
                    not _count(item['checkable_max_k']) or type(item['by_k']) is not list or
                    not all(type(x) is int for x in item['by_k'])):
                return False
        elif name in CATALOGUE_LISTS:
            if type(item) is not list or len(item) != CATALOGUE_LISTS[name] or not all(_count(x) for x in item):
                return False
        elif not _count(item):
            return False
    return True


def validate_euler(value, case):
    """Euler v13 : ordres verifiables min(K-2, n) (rien sous K3), sommes publiees
    pour K = 1..K ; une tour complete exige `holds` (toutes egales a 1).

    Un refus explicite anterieur a l'etape Euler du recensement (generateur,
    fusion, recensement, coquille > 12, ressources) publie `not_checkable` et
    une borne 0 : c'est un refus valide, jamais un defaut de protocole. `fails`
    n'existe qu'avec son propre refus."""
    euler = value['catalogue']['euler']
    checkable = min(case['k'] - 2, case['n']) if case['k'] >= 3 else 0
    need(len(euler['by_k']) == case['k'], 'euler length')
    reached = euler['checkable_max_k'] == checkable and (checkable > 0 or euler['status'] == 'not_checkable')
    if value['status'] == 'complete_relative':
        need(reached and euler['status'] == ('holds' if checkable else 'not_checkable') and
             euler['by_k'][:checkable] == [1] * checkable, 'euler invariant of a complete catalogue')
    elif value['reason'] == 'chain_catalogue_euler_violated' or euler['status'] == 'fails':
        # Lien dans les deux sens : ce refus porte une somme fausse atteinte.
        need(reached and euler['status'] == 'fails' and value['status'] == 'invariant_violated' and
             value['reason'] == 'chain_catalogue_euler_violated' and euler['by_k'][:checkable] != [1] * checkable,
             'euler failure without its refusal')
    elif euler['status'] == 'holds':
        need(reached and checkable > 0 and euler['by_k'][:checkable] == [1] * checkable,
             'euler holds of a later refusal')
    else:
        need(euler['checkable_max_k'] in (0, checkable), 'euler bound of an early refusal')


def validate_batch(value, case, capacity=0, judge=False, lanes_capacity=0):
    """Section q34_batch (v18) : phases du chemin par lots, a zero sur le chemin moteur."""
    batch, levers = value['q34_batch'], case['levers']
    times = ('front_ms', 'filter_ms', 'edges_ms', 'device_ms', 'certificate_ms', 'certificate_device_ms',
             'filter_kernel_ms', 'filter_transfer_ms', 'certificate_kernel_ms', 'certificate_transfer_ms')
    need(type(batch) is dict and set(batch) == BATCH_KEYS and type(batch['used']) is bool and
         type(batch['backend']) is str and type(batch['certificate_backend']) is str and
         type(batch['lanes_backend']) is str and
         all(_number(batch[key]) for key in times + LANES_TIMES) and _count(batch['rectangles']) and
         _count(batch['survivors']) and _count(batch['deferred']) and _count(batch['judged_edges']) and
         _count(batch['rebuilt_covers']) and _count(batch['certificate_warps']) and
         all(_count(batch[key]) for key in LANES_COUNTS), 'probe q34_batch fields')
    if value['status'] != 'complete_relative':
        return
    ledger = value['ledger']
    validate_lanes(value, case, lanes_capacity, judge)
    if not levers['q34_batch_filter']:
        need(batch['used'] is False and batch['backend'] == '' and batch['certificate_backend'] == '' and
             all(batch[key] == 0 for key in times) and batch['rectangles'] == 0 and batch['survivors'] == 0 and
             all(batch[key] == 0 for key in ('deferred', 'judged_edges', 'rebuilt_covers', 'certificate_warps')),
             'q34_batch filled on the engine path')
        return
    certificates, gpu_certificates = levers['q34_batch_certificates'], levers['q34_gpu_certificates']
    if certificates:
        # A device pass only when there is something to decide (an empty
        # call launches no kernel, auditor B), and then at least one edge
        # decided on the device: a call deferring everything is no S3 run.
        ran = gpu_certificates and batch['survivors'] > 0
        # v19 (B): the device interval split into kernel and transfers.
        need(((batch['certificate_kernel_ms'] > 0 and batch['certificate_kernel_ms'] + batch['certificate_transfer_ms'] <=
               batch['certificate_device_ms'] + 0.05) if ran else
              batch['certificate_kernel_ms'] == 0 and batch['certificate_transfer_ms'] == 0),
             'q34_batch certificate kernel/transfer split')
        need(batch['certificate_backend'] == (DEVICE_NAME if gpu_certificates else 'cpu') and
             (batch['certificate_device_ms'] > 0) == ran and (batch['certificate_warps'] > 0) == ran and
             batch['certificate_device_ms'] <= batch['certificate_ms'] + 0.05 and
             batch['deferred'] <= batch['survivors'] and (not ran or batch['deferred'] < batch['survivors']) and
             (gpu_certificates or batch['deferred'] == 0) and
             # v20: a q3 lane deferred by the lanes call rebuilds its cover
             # a second time in the tail (review of 23 September, night).
             batch['rebuilt_covers'] <= ledger['cover_builds'] + batch['lanes_deferred'] and
             batch['judged_edges'] == (batch['survivors'] - batch['deferred'] if judge else 0),
             'q34_batch certificate backend/device time/deferred/judge')
        # A correct device defers only an edge whose core or cover exceeds its
        # slab: never on a frame smaller than the default slab; always some,
        # never all, on the reduced-slab preflight (review of 23 September).
        if gpu_certificates and capacity:
            need(0 < batch['deferred'] < batch['survivors'], 'reduced-slab certificates deferred none or all edges')
        elif gpu_certificates and case['n'] < DEFAULT_CERTIFICATE_CAPACITY:
            need(batch['deferred'] == 0, 'device certificates deferred edges of a frame below the slab')
    else:
        need(batch['certificate_backend'] == '' and batch['certificate_ms'] == 0 and
             batch['certificate_device_ms'] == 0 and batch['certificate_kernel_ms'] == 0 and
             batch['certificate_transfer_ms'] == 0 and
             all(batch[key] == 0 for key in ('deferred', 'judged_edges', 'rebuilt_covers', 'certificate_warps')),
             'q34_batch certificates without the lever')
    gpu = levers['q34_gpu_filter']
    need(batch['used'] is True and batch['backend'] == (DEVICE_NAME if gpu else 'cpu') and
         (batch['device_ms'] > 0) == gpu, 'q34_batch backend/device time')
    need(((batch['filter_kernel_ms'] > 0 and batch['filter_kernel_ms'] + batch['filter_transfer_ms'] <=
           batch['device_ms'] + 0.05) if gpu else
          batch['filter_kernel_ms'] == 0 and batch['filter_transfer_ms'] == 0), 'q34_batch filter kernel/transfer split')
    # v20: the lanes call runs before the workers on the CPU, beside them on
    # the device; the wait for it and the tail follow the workers.
    serial = batch['front_ms'] + batch['filter_ms'] + batch['certificate_ms'] + batch['edges_ms'] + \
        batch['lanes_wait_ms'] + batch['tail_ms'] + (0 if levers['q34_gpu_q3'] else batch['lanes_ms'])
    need(batch['rectangles'] == ledger['q34_input_rectangles'] and
         batch['survivors'] == ledger['expanded_pairs'] - ledger['witness_rejected_pairs'] and
         serial <= value['times_ms']['q34'] + 0.05 and
         batch['device_ms'] <= batch['filter_ms'] + 0.05, 'q34_batch rectangles/survivors/phase times')


def validate_lanes(value, case, lanes_capacity=0, judge=False):
    """v20 (S4a) : appel des voies q3 et registre declare lanes_* du mode."""
    batch, levers, ledger = value['q34_batch'], case['levers'], value['ledger']
    if not levers['q34_batch_q4'] or value['options']['K'] < 3:
        need(all(ledger[key] == 0 for key in LANES4_LEDGER), 'q4 lanes filled without the lever')
    if not levers['q34_batch_q3']:
        need(batch['lanes_backend'] == '' and all(batch[key] == 0 for key in LANES_TIMES + LANES_COUNTS) and
             all(ledger[key] == 0 for key in LANES_LEDGER), 'q3 lanes filled without the lever')
        return
    q4 = levers['q34_batch_q4'] and value['options']['K'] >= 3
    gpu = levers['q34_gpu_q3']
    ran = gpu and batch['lanes_asked'] > 0
    open_edges = (ledger['q3_edges'] + ledger['q4_edges'] - ledger['both_edges']) if q4 else ledger['q3_edges']
    need(batch['lanes_backend'] == (DEVICE_NAME if gpu else 'cpu') and
         batch['lanes_decided'] + batch['lanes_deferred'] == batch['lanes_asked'] and
         # Every lane left open by a DECIDED certificate is asked (q3, and q4
         # under S4b); each certificate-deferred survivor adds at most one
         # open edge (review of 23 September).
         batch['lanes_asked'] <= open_edges <= batch['lanes_asked'] + batch['deferred'] and
         batch['lanes_decided'] == ledger['lanes_edges'] and
         batch['lanes_records'] == ledger['lanes_emitted'] + ledger['lanes4_emitted'] and
         batch['lanes_judged'] == (batch['lanes_decided'] if judge else 0) and
         (batch['lanes_device_ms'] > 0) == ran and (batch['lanes_warps'] > 0) == ran and
         (not ran or batch['lanes_decided'] > 0) and
         ((batch['lanes_kernel_ms'] > 0 and batch['lanes_kernel_ms'] + batch['lanes_transfer_ms'] <=
           batch['lanes_device_ms'] + 0.05 and batch['lanes_device_ms'] <= batch['lanes_ms'] + 0.05) if ran else
          batch['lanes_kernel_ms'] == 0 and batch['lanes_transfer_ms'] == 0 and batch['lanes_device_ms'] == 0),
         'q3 lanes backend/counts/device time/judge')
    # The reduced-slab preflight defers some but never all q3 lanes. Below
    # the default site slab a correct call may still defer an edge beyond its
    # record slab or the record arena (auditor's exact 4 097-cluster fixture,
    # gate mhgp9_gpu_lanes_port): a deferral is judged by the tail and the
    # digests, never refused by the reader.
    if lanes_capacity:
        need(0 < batch['lanes_deferred'] < batch['lanes_asked'], 'reduced-slab q3 lanes deferred none or all edges')
    need(ledger['lanes_census_seeds'] == ledger['lanes_depth_rejections'] + ledger['lanes_emitted'] and
         ledger['lanes_census_seeds'] <= ledger['lanes_seeds'] and ledger['lanes_q3_edges'] <= ledger['lanes_edges'] and
         (q4 or ledger['lanes_q3_edges'] == ledger['lanes_edges']) and
         ledger['lanes_acute_sites'] == ledger['lanes_owner_rejections'] + ledger['lanes_seeds'] and
         ledger['lanes_seed_tests'] == ledger['lanes_cover_sites'] and
         ledger['lanes_cover_sites'] >= 2 * ledger['lanes_edges'] and
         ledger['lanes_cover_node_visits'] >= ledger['lanes_edges'] and
         ledger['lanes_census_point_tests'] == ledger['lanes_census_inside_sites'] +
         ledger['lanes_census_shell_sites'] + ledger['lanes_census_outside_sites'] and
         ledger['lanes_shell_ids'] <= ledger['lanes_census_shell_sites'] and
         3 * ledger['lanes_emitted'] <= ledger['lanes_shell_ids'] and
         ledger['lanes_emitted'] <= value['generator']['q3_emitted'], 'q3 lanes ledger identity')
    if q4:
        need(ledger['lanes4_seeds'] == ledger['lanes4_certified'] + ledger['lanes4_survivors'] and
             ledger['lanes4_certified_chunk1'] <= ledger['lanes4_certified'] and
             ledger['lanes4_groups'] == ledger['lanes4_depth_rejected_groups'] +
             ledger['lanes4_groups_without_valid'] + ledger['lanes4_emitted'] and
             ledger['lanes4_multi_emission_seeds'] <= ledger['lanes4_emitting_seeds'] <= ledger['lanes4_emitted'] and
             ledger['lanes4_emitting_seeds'] <= ledger['lanes4_survivors'] and
             ledger['lanes4_seeds'] <= ledger['lanes_seeds'] and ledger['lanes4_edges'] <= ledger['lanes_edges'] and
             ledger['lanes4_edges'] <= ledger['q4_edges'] and
             ledger['lanes4_emitted'] <= value['generator']['q4_emitted'] and
             4 * ledger['lanes4_emitted'] <= ledger['lanes4_shell_ids'] and
             ledger['lanes4_max_buffered'] <= ledger['lanes4_buffered_events'] and
             ledger['lanes4_pass_chunks'] >= ledger['lanes4_seeds'] and
             # every live bucket builds its list, every group locates and compares
             ledger['lanes4_list_steps'] >= ledger['lanes4_live_buckets'] and
             ledger['lanes4_group_steps'] >= 2 * ledger['lanes4_compare_steps'] and
             ledger['lanes4_compare_steps'] >= ledger['lanes4_groups'], 'q4 lanes ledger identity')


def validate_occupancy(value, case):
    """Occupation q3/q4 mesuree : murs des ouvriers dans celui de l'etape q34,
    CPU et attente bornes par fils x mur, taches consommees = publiees."""
    o = value['q34_occupancy']
    need(type(o) is dict and set(o) == set(OCCUPANCY_COUNTS + OCCUPANCY_TIMES) and
         all(_count(o[key]) for key in OCCUPANCY_COUNTS) and all(_number(o[key]) for key in OCCUPANCY_TIMES),
         'q34 occupancy fields')
    if value['status'] != 'complete_relative':
        return
    started = o['started_workers']
    need(1 <= started <= case['workers'] and o['tasks_consumed'] == o['tasks_published'] and
         o['wall_min_ms'] <= o['wall_max_ms'] <= value['times_ms']['q34'] + PHASE_TOLERANCE_MS, 'q34 occupancy walls')
    budget = started * o['wall_max_ms'] / 1000.0
    need(o['cpu_sum_s'] <= budget * 1.01 + 0.01 and o['wait_sum_s'] <= budget + 0.01, 'q34 occupancy cpu/wait')
    # v14 : mur dans les jobs du front (somme et plus long job), sous-chronos
    # des boucles des fils.
    need(o['max_job_ms'] <= o['wall_max_ms'] + PHASE_TOLERANCE_MS and o['job_sum_s'] <= budget + 0.01 and
         (o['jobs'] == 0 or o['max_job_ms'] > 0 or o['job_sum_s'] == 0), 'q34 occupancy jobs')


def validate_tower_phases(value, case):
    """Chronos de phase de la tour : sous-chronos du mur de la tour, par K
    bornes par leur etape parallele, voie statique ou sequentielle exclusive."""
    phases = value['tower_phases_ms']
    need(type(phases) is dict and set(phases) == set(TOWER_PHASES + TOWER_PHASES_BY_K) and
         all(_number(phases[key]) for key in TOWER_PHASES) and
         all(type(phases[key]) is list and len(phases[key]) == case['k'] and all(_number(x) for x in phases[key])
             for key in TOWER_PHASES_BY_K), 'tower phase fields')
    if value['status'] != 'complete_relative':
        return
    total = sum(phases[key] for key in TOWER_PHASES) + sum(phases['order_by_k'])
    need(total <= value['times_ms']['tower'] + PHASE_TOLERANCE_MS * (len(TOWER_PHASES) + case['k']),
         'tower phases exceed the tower time')
    static_path = case['static_threads'] > 1 and min(case['k'], case['n']) > 1
    if static_path:
        # v15 : avec tower_overlap_static, la phase A d'un ordre peut recouvrir
        # la phase 0 ; `lots` est le reste apres la phase 0, et chaque
        # lots_by_k est borne par la fenetre phase 0 + reste.
        overlap = case['levers'].get('tower_overlap_static', False)
        lots_window = phases['lots'] + (phases['static'] if overlap else 0.0)
        need(not any(phases['order_by_k']) and phases['static_by_k'][0] == 0 and
             abs(phases['static'] - sum(phases['static_by_k'])) <= PHASE_TOLERANCE_MS * case['k'] and
             all(x <= lots_window + PHASE_TOLERANCE_MS for x in phases['lots_by_k']) and
             all(x <= phases[step] + PHASE_TOLERANCE_MS
                 for step, key in (('images', 'images_by_k'), ('encode', 'encode_by_k'))
                 for x in phases[key]), 'tower static path phases')
        if overlap:
            # Dependance par ordre (contrelecture B) : la phase A de K suit la
            # phase 0 de K et, par K decroissant, celles des ordres superieurs ;
            # populations, images, banque et encodage suivent tous les lots.
            # L'ordre 1 n'attend aucune phase 0 : il tourne pendant elles
            # (contrelecture A/B) ; seul le maximum des deux entre dans la chaine.
            after = sum(phases[key] for key in ('populations', 'images', 'bank', 'encode'))
            for k in range(1, case['k'] + 1):
                before = (max(phases['static'], phases['lots_by_k'][0]) if k == 1 else
                          sum(phases['static_by_k'][k - 1:]) + phases['lots_by_k'][k - 1])
                chain = phases['validate'] + before + after
                need(chain <= value['times_ms']['tower'] + PHASE_TOLERANCE_MS * (case['k'] + 7),
                     'tower phase A of order %d before its phase 0' % k)
    else:
        need(not any(phases[key] for key in STATIC_PATH_PHASES) and
             not any(any(phases[key]) for key in ('static_by_k', 'lots_by_k', 'images_by_k')),
             'tower sequential path phases')


def validate_ledger_identities(value, levers):
    """Identites exactes d'une tour complete entre generateur, registre et catalogue."""
    ledger, generator, catalogue = value['ledger'], value['generator'], value['catalogue']
    need(ledger['expanded_pairs'] == ledger['cover_builds'] + ledger['core_closed_edges'] +
         ledger['witness_rejected_pairs'] and
         generator['q34_expanded_pairs'] == ledger['expanded_pairs'] and
         generator['q34_cover_builds'] == ledger['cover_builds'], 'ledger pair/cover identity')
    need(generator['q2_accepted_pairs'] == catalogue['q2_presentations'] and
         generator['q3_emitted'] == catalogue['q3_presentations'] and
         generator['q4_emitted'] == catalogue['q4_presentations'], 'generator/catalogue presentation identity')
    need(catalogue['balls'] == catalogue['unique_keys'] == sum(catalogue['by_qmin']) == sum(catalogue['by_shell']) and
         catalogue['q2_presentations'] + catalogue['q3_presentations'] + catalogue['q4_presentations'] >=
         catalogue['unique_keys'], 'catalogue identity')
    # La chaine refuse toute coquille de plus de 12 sites avant complete_relative.
    need(catalogue['shell_over_12'] == 0 and catalogue['max_shell'] <= 12 and
         all(count == 0 for count in catalogue['by_shell'][13:]), 'complete catalogue with a shell above 12')
    # Parcours de l'index global deja comptes par le generateur (arbre plein
    # de 2n-1 noeuds : au plus 2n-1 visites par appel) et leurs identites.
    nodes = 2 * value['input']['sites'] - 1
    need(ledger['witness_rect_queries'] == ledger['q34_input_rectangles'] and
         ledger['witness_pair_queries'] + ledger['witness_cache_rejected_pairs'] == ledger['expanded_pairs'] and
         ledger['q3_seed_node_visits'] == ledger['q3_seed_point_tests'] + ledger['q3_seed_bound_tests'] and
         ledger['q3_edge_queries'] <= ledger['q3_edges'] and
         ledger['q4_geometry_preparations'] <= ledger['q4_edges'] and
         ledger['q4_seed_cell_queries'] <= ledger['q4_edges'] and
         ledger['witness_rect_node_visits'] <= nodes * ledger['witness_rect_queries'] and
         ledger['witness_pair_node_visits'] <= nodes * ledger['witness_pair_queries'] and
         ledger['q3_seed_node_visits'] <= nodes * ledger['q3_edge_queries'] and
         all(ledger[name] <= nodes * ledger['q4_geometry_preparations']
             for name in ('q4_domain_node_visits', 'q4_cover_decomposition_node_visits')) and
         ledger['q4_seed_node_visits'] <= nodes * ledger['q4_seed_cell_queries'],
         'hidden q3/q4 index traversals identity/bounds')
    # Une arete aux deux voies est dans q3, dans q4 et a un cover.
    need(ledger['both_edges'] <= min(ledger['q3_edges'], ledger['q4_edges']) and
         ledger['q3_edges'] + ledger['q4_edges'] - ledger['both_edges'] <= ledger['cover_builds'],
         'edge lane identity')
    dead = ('dead_loads', 'dead_form_sites', 'dead_cells', 'dead_outside_cells', 'dead_deep_cells', 'dead_failed_cells',
            'dead_uniform_tests', 'dead_point_tests', 'dead_q3_proved', 'dead_q3_open', 'dead_q4_proved', 'dead_q4_open')
    if levers['q34_dead_lanes']:
        need(ledger['dead_loads'] == ledger['cover_builds'] and
             ledger['dead_form_sites'] == ledger['cover_sites'] - 2 * ledger['dead_loads'] and
             ledger['dead_q3_open'] == ledger['q3_edges'] and ledger['dead_q4_open'] == ledger['q4_edges'],
             'dead-lane ledger identity')
        # Une voie est prouvee ou ouverte au plus une fois par cover ; chaque
        # cover porte au moins une voie ; classes de cellules disjointes.
        need(all(ledger['dead_q%d_proved' % q] + ledger['dead_q%d_open' % q] <= ledger['cover_builds'] for q in (3, 4)) and
             ledger['q3_edges'] + ledger['q4_edges'] - ledger['both_edges'] + ledger['dead_q3_proved'] +
             ledger['dead_q4_proved'] >= ledger['cover_builds'] and
             ledger['dead_outside_cells'] + ledger['dead_deep_cells'] + ledger['dead_failed_cells'] <=
             ledger['dead_cells'], 'dead-lane lane/cell bounds')
    else:
        need(all(ledger[name] == 0 for name in dead), 'dead-lane counters while the lever is off')
    core = ('core_builds', 'core_sites', 'core_closed_edges', 'dead_core_loads', 'dead_core_form_sites',
            'dead_core_cells', 'dead_core_uniform_tests', 'dead_core_point_tests', 'dead_core_q3_proved',
            'dead_core_q3_open', 'dead_core_q4_proved', 'dead_core_q4_open', 'core_cover_node_visits',
            'core_cover_bound_tests', 'core_cover_point_tests', 'dead_core_outside_cells', 'dead_core_deep_cells',
            'dead_core_failed_cells')
    if levers['q34_dead_core']:
        need(ledger['core_builds'] == ledger['cover_builds'] + ledger['core_closed_edges'] and
             ledger['dead_core_loads'] == ledger['core_builds'] and
             ledger['dead_core_form_sites'] == ledger['core_sites'] - 2 * ledger['core_builds'] and
             ledger['dead_core_q3_open'] == ledger['dead_q3_proved'] + ledger['dead_q3_open'] and
             ledger['dead_core_q4_open'] == ledger['dead_q4_proved'] + ledger['dead_q4_open'],
             'dead-lane core ledger identity')
        need(all(ledger['dead_core_q%d_proved' % q] + ledger['dead_core_q%d_open' % q] <= ledger['core_builds']
                 for q in (3, 4)) and
             ledger['core_closed_edges'] <= ledger['dead_core_q3_proved'] + ledger['dead_core_q4_proved'] and
             ledger['cover_builds'] <= ledger['dead_core_q3_open'] + ledger['dead_core_q4_open'] and
             ledger['core_cover_node_visits'] == ledger['core_cover_bound_tests'] + ledger['core_cover_point_tests'] and
             ledger['core_builds'] <= ledger['core_cover_node_visits'] and
             ledger['dead_core_outside_cells'] + ledger['dead_core_deep_cells'] + ledger['dead_core_failed_cells'] <=
             ledger['dead_core_cells'], 'dead-lane core lane/cell/cover bounds')
    else:
        need(all(ledger[name] == 0 for name in core), 'dead-lane core counters while the lever is off')
    cache = ('witness_cache_queries', 'witness_cache_node_tests', 'witness_cache_rejected_pairs')
    if levers['q34_batch_filter']:
        # The batch path searches every expanded pair once, without the cache.
        need(all(ledger[name] == 0 for name in cache) and ledger['witness_pair_queries'] == ledger['expanded_pairs'],
             'batch path: no cache, one search per expanded pair')
    elif levers['q34_witness_cache']:
        # Un rejet par le cache suppose une requete et au moins un noeud teste.
        need(ledger['witness_cache_rejected_pairs'] <= ledger['witness_rejected_pairs'] and
             ledger['witness_cache_rejected_pairs'] <= ledger['witness_cache_queries'] <= ledger['expanded_pairs'] and
             (ledger['witness_cache_rejected_pairs'] == 0 or ledger['witness_cache_node_tests'] > 0),
             'witness cache identity')
    else:
        need(all(ledger[name] == 0 for name in cache), 'witness cache counters while the lever is off')
    if not levers['q3_leaf_census']:
        need(ledger['q3_leaf_censuses'] == 0 and ledger['q3_leaf_point_tests'] == 0, 'leaf census while the lever is off')
    tower = value['tower_work']
    if levers['tower_meb_proposal']:
        # Every proposal is verified, canonicalized from a verified one, or
        # falls back to the reference enumeration; one proposal per MEB at most.
        need(tower['meb_accounting'] == MEB_ACCOUNTING and tower['meb_proposals'] <= tower['meb_calls'] and
             tower['meb_verified_proposals'] + tower['meb_proposal_fallbacks'] == tower['meb_proposals'] and
             tower['meb_boundary_canonicalizations'] <= tower['meb_verified_proposals'],
             'tower MEB proposal identity')
    else:
        need(tower['meb_accounting'] == MEB_REFERENCE_ACCOUNTING and
             all(tower[name] == 0 for name in MEB_PROPOSAL_KEYS), 'tower MEB proposal counters while the lever is off')


def validate_preflight_work(value, levers):
    """Le preflight doit exercer chaque levier actif (jamais une sonde vide)."""
    ledger, generator = value['ledger'], value['generator']
    need(value['catalogue']['balls'] > 0 and generator['q3_emitted'] > 0 and generator['q4_emitted'] > 0 and
         ledger['cover_builds'] > 0, 'preflight did no geometric work')
    # v20: under q34_batch_q3 the engine's atlas q3 lane never runs (the
    # lanes call and its tail use no atlas): no leaf census is owed.
    need((not levers['q3_leaf_census'] or levers['q34_batch_q3'] or ledger['q3_leaf_censuses'] > 0) and
         # v21: under q34_batch_q4 no atlas is built (the engine twin proves it).
         (not levers['atlas_saturate_deep'] or levers['q34_batch_q4'] or ledger['atlas_deep_cells'] > 0) and
         (not levers['q34_dead_lanes'] or ledger['dead_q3_proved'] + ledger['dead_q4_proved'] > 0) and
         (not levers['q34_witness_cache'] or levers['q34_batch_filter'] or
          ledger['witness_cache_rejected_pairs'] > 0) and
         (not levers['q34_batch_filter'] or value['q34_batch']['survivors'] > 0) and
         (not levers['q34_batch_certificates'] or value['q34_batch']['certificate_ms'] > 0) and
         (not levers['q34_batch_q3'] or (value['q34_batch']['lanes_decided'] > 0 and
                                         value['q34_batch']['lanes_records'] > 0)) and
         (not levers['q34_batch_q4'] or ledger['lanes4_emitted'] > 0) and
         (not levers['q34_dead_core'] or ledger['core_closed_edges'] > 0) and
         (not levers['tower_meb_proposal'] or (value['tower_work']['meb_proposals'] > 0 and
                                               value['tower_work']['meb_verified_proposals'] > 0)),
         'preflight did not exercise an active lever')


def preflight_cloud():
    """Nuage u18 deterministe (LCG 64 bits, trois grappes) du preflight natif."""
    state, out = 3, bytearray()
    for i in range(PREFLIGHT_SITES):
        centre = (i % 3) * 40000 + 20000
        coords = []
        for _ in range(3):
            state = (state * 6364136223846793005 + 1442695040888963407) % (1 << 64)
            coords.append(centre + (state >> 40) % 9000)
        out += struct.pack('<3I', *coords)
    return bytes(out)


def preflight_case(cases, raw):
    """Cas du preflight : les voies du premier cas du plan, K5, deux fils."""
    return dict(cases[0], scene='preflight', file=PREFLIGHT_FILE, n=len(raw) // 12, k=5, workers=2, static_threads=2,
                repeat=0)


def preflight_inputs(raw):
    return {'preflight': dict(n=len(raw) // 12, fnv=input_fnv(raw))}


def validate_probe(value, case, exit_code, inputs=None, capacity=0, judge=False, lanes_capacity=0):
    """Lit la sortie de la sonde ; rend complete_relative ou explicit_refusal.

    Un statut non complet n'est accepte que s'il est explicite (code 3) ; une
    sortie partielle, un code inattendu ou un domaine different est refuse.
    `inputs` remplace INPUTS pour la porte locale sonde reelle / validateur."""
    need(type(value) is dict and set(value) == TOP_KEYS, 'probe JSON fields')
    need(value['schema'] == PROBE_SCHEMA and value['status'] in PROBE_STATUSES and type(value['reason']) is str,
         'probe schema/status')
    need(_counters(value['ledger'], LEDGER_KEYS), 'probe ledger')
    data = (INPUTS if inputs is None else inputs)[case['scene']]
    source = value['input']
    need(type(source) is dict and set(source) == INPUT_KEYS and source['format'] == 'u32le' and
         source['grid'] == '1mm' and source['sites'] == case['n'] == data['n'] and source['hash'] == data['fnv'],
         'probe input identity')
    options = value['options']
    need(type(options) is dict and set(options) == OPTION_KEYS and options['K'] == case['k'] and
         options['s'] == case['s'] and options['workers'] == case['workers'] and
         options['tower_static_threads'] == case['static_threads'] and options['run_tower'] is True and
         options['certificate_capacity'] == capacity and options['certificate_judge'] is judge and
         options['lanes_capacity'] == lanes_capacity and options['lanes_events'] == 0 and
         options['lanes_judge'] is (judge and case['levers']['q34_batch_q3']) and
         _levers(options['levers']) and options['levers'] == case['levers'] and
         type(options['K_effective']) is int, 'probe options')
    times = value['times_ms']
    need(type(times) is dict and set(times) == TIME_KEYS and all(_number(item) for item in times.values()) and
         _number(value['chain_cpu_s']), 'probe times')
    # Les etapes sont des sous-chronos du total de chaine (la lecture est
    # mesuree a part) : leur somme ne peut pas le depasser, a l'arrondi pres.
    need(sum(times[key] for key in STAGE_TIME_KEYS) <= times['chain_total'] + 0.01 * len(STAGE_TIME_KEYS),
         'probe stage times exceed the chain total')
    need(_counters(value['generator'], GENERATOR_KEYS), 'probe counters generator')
    need(_tower_work(value['tower_work']), 'probe counters tower_work')
    need(_catalogue(value['catalogue']), 'probe catalogue')
    validate_euler(value, case)
    validate_occupancy(value, case)
    validate_batch(value, case, capacity, judge, lanes_capacity)
    validate_tower_phases(value, case)
    orders = value['orders']
    need(type(orders) is list and all(type(order) is dict and set(order) == ORDER_KEYS and
                                      all(_count(item) for item in order.values()) for order in orders), 'probe orders')
    need(type(value['tower_digest']) is str and re.fullmatch('[0-9a-f]{16}', value['tower_digest']) and
         type(value['catalogue_digest']) is str and re.fullmatch('[0-9a-f]{16}', value['catalogue_digest']) and
         type(value['presentation_digest']) is str and re.fullmatch('[0-9a-f]{16}', value['presentation_digest']) and
         type(value['peak_rss_kb']) is int and value['peak_rss_kb'] >= -1, 'probe digest/RSS')
    effective = min(case['k'], case['n'])
    if value['status'] == 'complete_relative':
        need(exit_code == 0 and options['K_effective'] == effective and
             [order['K'] for order in orders] == list(range(1, effective + 1)), 'complete tower: code 0, orders 1..K')
        validate_ledger_identities(value, case['levers'])
        pin = PINNED_DIGESTS.get((case['scene'], case['k'], case['s'])) if inputs is None else None
        need(pin is None or (value['tower_digest'], value['catalogue_digest']) == pin,
             'tower or catalogue digest differs from the pinned CPU value')
        return 'complete_relative'
    need(exit_code == 3 and options['K_effective'] in (0, effective), 'explicit refusal must exit with code 3')
    return 'explicit_refusal'


def validate_external_wall(value, elapsed_seconds):
    """La lecture, le chrono interne de chaine et le condense de verification
    mesure apres lui (times_ms.read/chain_total/digest, sequentiels dans la
    sonde) sont bornes ensemble par le mur externe du cas (GNU time enveloppe
    tout l'executable). La lecture n'entre pas pour autant dans le contrat."""
    times = value['times_ms']
    need(_number(elapsed_seconds) and
         (times['read'] + times['chain_total'] + times['digest'] + times['catalogue_digest']) / 1000.0 <=
         elapsed_seconds + EXTERNAL_WALL_TOLERANCE_SECONDS,
         'read, chain total and digest exceed the external wall time of the case')


def validate_gnu_time(text, exit_code):
    need('Command being timed: "' in text and 'Elapsed (wall clock) time' in text, 'GNU time -v report')
    rss = re.findall(r'^\s*Maximum resident set size \(kbytes\): (\d+)$', text, re.M)
    status = re.findall(r'^\s*Exit status: (\d+)$', text, re.M)
    need(len(rss) == 1 and status == [str(exit_code)], 'GNU time exit status/RSS')
    return int(rss[0])


def logical_result(value):
    """Projection comparee entre nombres d'ouvriers : l'objet, pas les couts."""
    # v21 (review S4b): every presentation (key, arity, support) and their
    # counts too, which the tower and catalogue digests do not see.
    generator, catalogue = value['generator'], value['catalogue']
    return dict(hash=value['input']['hash'], sites=value['input']['sites'],
                K_effective=value['options']['K_effective'], unique_keys=catalogue['unique_keys'],
                balls=catalogue['balls'], euler=catalogue['euler'], orders=value['orders'],
                tower_digest=value['tower_digest'], catalogue_digest=value['catalogue_digest'],
                presentation_digest=value['presentation_digest'],
                presentations=(generator['q2_accepted_pairs'], generator['q3_emitted'], generator['q4_emitted'],
                               catalogue['q2_presentations'], catalogue['q3_presentations'],
                               catalogue['q4_presentations']))


def certificate_work(value):
    """v18 : travail des certificats et du cover, egal entre deux cas aux memes
    leviers q34_dead_lanes/q34_dead_core (moteur, lots CPU, GPU, tout W)."""
    return {name: value['ledger'][name] for name in CERTIFICATE_WORK_KEYS}


def compare_cases(cases, outcomes, values):
    groups, out = {}, []
    for index, (case, entry) in enumerate(zip(cases, outcomes)):
        if entry.get('outcome') != 'complete_relative':
            continue
        key = (case['file'], case['k'], case['s'])
        if key in groups:
            reference = groups[key]
            same = logical_result(values[reference]) == logical_result(values[index])
            levers = [cases[i]['levers'] for i in (reference, index)]
            if all(levers[0][name] == levers[1][name] for name in ('q34_dead_lanes', 'q34_dead_core')):
                same = same and certificate_work(values[reference]) == certificate_work(values[index])
            out.append(dict(reference=reference, other=index, equal=same))
        else:
            groups[key] = index
    return out


def configure_command(tools, root, build):
    """Strict configure (no -Wno-error), CUDA on with the existing nvcc."""
    return [tools['cmake'], '-S', str(root / SOURCE_ROOT), '-B', str(build), '-DCMAKE_BUILD_TYPE=Release',
            '-DBOOST_ROOT=' + BOOST_ROOT, '-DCMAKE_CXX_COMPILER=' + tools['g++'], '-DMHGP9_ENABLE_CUDA=ON',
            '-DCMAKE_CUDA_COMPILER=' + tools['nvcc']]


def unpaired_batch_cases(cases, outcomes):
    """Complete batch/GPU cases without a complete engine-path twin (same file,
    K, s): their tower was never compared on LiDAR (auditor A, S2 partial)."""
    engine = {(case['file'], case['k'], case['s']) for case, entry in zip(cases, outcomes)
              if entry.get('outcome') == 'complete_relative' and not case['levers']['q34_batch_filter']}
    return [index for index, (case, entry) in enumerate(zip(cases, outcomes))
            if entry.get('outcome') == 'complete_relative' and case['levers']['q34_batch_filter'] and
            (case['file'], case['k'], case['s']) not in engine]


def device_ran(case, value):
    """A q3/q4 batch phase observed on the device (auditor B): the S2 filter
    with a device time, or S3 certificates with a device time and warps. The
    levers alone are not enough (an empty call launches no kernel)."""
    batch = value['q34_batch']
    return ((case['levers']['q34_gpu_filter'] and batch['device_ms'] > 0) or
            (case['levers']['q34_gpu_certificates'] and batch['certificate_device_ms'] > 0 and
             batch['certificate_warps'] > 0) or
            (case['levers']['q34_gpu_q3'] and batch['lanes_device_ms'] > 0 and batch['lanes_warps'] > 0))


def gpu_completed_cases(cases, outcomes, values):
    """Complete LiDAR towers with a q3/q4 phase observed on the device: the
    only ground for GPU_executed (a device preflight alone is not, auditor B)."""
    return [index for index, (case, entry) in enumerate(zip(cases, outcomes))
            if entry.get('outcome') == 'complete_relative' and device_ran(case, values[index])]


def compiled_dependencies(build, root, before):
    consumed, relative_seen = {}, set()
    depfiles = sorted(build.glob('CMakeFiles/*.dir/**/*.o.d'))
    need(depfiles, 'no compiler depfile under the CMake build')
    for dep in depfiles:
        text = dep.read_text().replace('\\\n', ' ')
        need(':' in text, 'malformed depfile')
        for name in shlex.split(text.split(':', 1)[1]):
            path = Path(name) if Path(name).is_absolute() else build / name
            path = path.resolve()
            if path.is_relative_to(root):
                relative = str(path.relative_to(root))
                need(relative in before and sha(path) == before[relative], 'unmanifested compilation dependency')
                relative_seen.add(relative)
            consumed[str(path)] = sha(path)
    need({PROBE_SOURCE, CHAIN_SOURCE} <= relative_seen, 'depfiles lack the probe/chain sources')
    return consumed


class BuildFailed(Exception):
    pass


class PreflightFailed(Exception):
    pass


def execute(args):
    root, output = args.source_root.resolve(), args.output.absolute()
    # Un plafond par cas superieur au budget est licite : le budget gouverne.
    need(1 <= args.useful_budget_seconds <= USEFUL_BUDGET_SECONDS and
         1 <= args.case_cap_seconds <= int(MAX_RUN_SECONDS) and args.closing_margin_seconds >= 300, 'cost/closing budget')
    need(not output.exists() and not output.is_symlink() and not output.resolve().is_relative_to(root), 'fresh output')
    output.mkdir(mode=0o700)
    result = dict(status='failed', backend='reference_cpu', scope=SCOPE, public_status='not_claimed',
                  GPU_attempted=False, GPU_preflight_executed=False, GPU_completed_cases=[], GPU_executed=False,
                  FULL_executed=False, contract_certified=False,
                  targeted_GCP_stop_required_by_ROOT=True, worker_argv=list(args.argv), worker_sha256=sha(__file__),
                  useful_budget_seconds=args.useful_budget_seconds, case_cap_seconds=args.case_cap_seconds)
    worker, manifest, before, consumed = None, None, None, {}
    tools, binary, binary_sha = {}, None, None
    began = time.time()

    def interrupted(signum, _frame):
        raise InterruptedError('guest signal ' + str(signum))
    previous = {signum: signal.signal(signum, interrupted) for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)}
    try:
        helper = load_helper(root)
        need(sha(args.guard_mark) == args.guard_mark_sha256, 'guard pin')
        mark, schedule = helper.fields(args.guard_mark.read_text()), helper.fields(helper.scheduled_text())
        target = {key: getattr(args, key) for key in TARGET}
        need(target == TARGET and mark.get('max_run_seconds') == MAX_RUN_SECONDS and
             mark.get('guest_shutdown_minutes') == GUEST_SHUTDOWN_MINUTES, 'fixed target and dual-guard duration')
        guards = helper.guard_values(mark, schedule, target, args.generation, args.session_deadline_epoch,
                                     args.closing_margin_seconds, time.time())
        observed = helper.metadata()
        need(all(observed[key] == value for key, value in TARGET.items()) and
             observed['machine'] == 'g4-standard-48', 'guest target/type')
        need(abs(boot_epoch() - helper.epoch(args.generation)) <= 300, 'guest boot/generation')
        cpus = available_cpus()
        need(len(cpus) == 48, '48 available vCPUs required')
        useful_deadline = min(began + args.useful_budget_seconds, guards['work_deadline_epoch'])
        worker = helper.Worker(output, useful_deadline, schedule)
        result.update(target=target, generation=args.generation, available_cpus=cpus, guards=guards,
                      useful_deadline_epoch=useful_deadline)
        save(output / 'guard_evidence.json', dict(mark=mark, schedule=schedule, metadata=observed))
        need(sha(args.source_manifest) == args.source_manifest_sha256, 'manifest pin')
        manifest = strict_json(args.source_manifest.read_bytes())
        validate_runtime(manifest)
        result['source_manifest_sha256'] = args.source_manifest_sha256
        before = source_map(root, manifest)
        save(output / 'sources_before.json', before)
        read = lambda name: (root / name).read_bytes()
        validate_sources(read)
        cases = validate_plan(strict_json(read(PLAN)), manifest)
        validate_data(read)
        result['provenance'] = validate_provenance(strict_json(read(PROVENANCE)), manifest)
        result.update(plan_schema=PLAN_SCHEMA, cases=cases, case_outcomes=[], completed_case_indices=[],
                      cross_worker_comparisons=[])
        # v17: the G4 build always enables CUDA (the plan's GPU cases need
        # it; CPU cases are unchanged). Existing tools only.
        result['backend'] = 'cuda_g4' if plan_uses_gpu(cases) else 'reference_cpu'
        nvcc = next((p for p in CUDA_PATHS if Path(p).is_file() and os.access(p, os.X_OK)), None)
        tools = {'g++': shutil.which('g++'), 'cmake': shutil.which('cmake'), 'nvcc': nvcc,
                 'nvidia-smi': shutil.which('nvidia-smi')}
        need(all(tools.values()) and Path(TIME).is_file() and Path(BOOST_HEADER).is_file(),
             'g++, cmake, nvcc, nvidia-smi, GNU time and system Boost required; no automatic installation')
        result['CUDA_installation_attempted'] = False
        result['tool_paths'] = dict(tools, time=TIME, boost_header=BOOST_HEADER)
        result['tool_sha256'] = {name: sha(path) for name, path in tools.items()}
        for name, command in [('compiler', [tools['g++'], '--version']), ('cmake_version', [tools['cmake'], '--version']),
                              ('nvcc_version', [tools['nvcc'], '--version']),
                              ('gpu_inventory', [tools['nvidia-smi'],
                                                 '--query-gpu=name,driver_version,memory.total,compute_cap',
                                                 '--format=csv,noheader']),
                              ('time_version', [TIME, '--version']), ('lscpu', ['lscpu']), ('nproc', ['nproc'])]:
            need(worker.command(name, command)['exit_code'] == 0, name)
        for filename in ('/etc/os-release', '/proc/meminfo'):
            with (output / (Path(filename).name + '.txt')).open('x') as stream:
                stream.write(Path(filename).read_text())
        build = output / 'build'
        # Build strict tel quel : -DCMAKE_CXX_FLAGS=-Wno-error serait place
        # AVANT le -Werror de add_compile_options (CMAKE_CXX_FLAGS precede les
        # options de compilation), donc sans effet ; le CMake ne le permet
        # pas et le worker ne le passe pas. Un echec est consigne, sans relance.
        need(DEVICE_NAME in (output / 'gpu_inventory.stdout').read_text(), 'G4 GPU inventory')
        configure = worker.command('configure', configure_command(tools, root, build))
        if configure['exit_code'] != 0:
            raise BuildFailed('cmake configure failed; logs in configure.stdout/stderr')
        compiled = worker.command('build', [tools['cmake'], '--build', str(build), '--target', PROBE_TARGET,
                                            '--parallel', BUILD_PARALLEL])
        if compiled['exit_code'] != 0:
            raise BuildFailed('strict build of mhgp9_tower_probe failed; logs in build.stdout/stderr')
        binary = build / PROBE_TARGET
        need(binary.is_file() and not binary.is_symlink(), 'probe binary absent after build')
        binary_sha = sha(binary)
        consumed = compiled_dependencies(build, root, before)
        save(output / 'compiled_dependencies.json', consumed)
        result.update(binary_sha256=binary_sha,
                      compiled_dependency_manifest_sha256=sha(output / 'compiled_dependencies.json'))
        # Preflight natif avant tout cas LiDAR : la sonde reelle, les voies du
        # premier cas, jugee par le meme validate_probe. Echec = aucun cas.
        pre_raw = preflight_cloud()
        with (output / PREFLIGHT_FILE).open('xb') as stream:
            stream.write(pre_raw)
        pre_case = preflight_case(cases, pre_raw)
        if uses_device(pre_case['levers']):
            result['GPU_attempted'] = True
        pre_judge = judged_preflight(pre_case['levers'])
        pre_row = worker.command('preflight', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                               *expected_probe_tail(pre_case, judge=pre_judge)])
        try:
            need(pre_row['exit_code'] == 0, 'preflight probe exit code')
            pre_value = strict_json((output / 'preflight.stdout').read_bytes())
            need(validate_probe(pre_value, pre_case, 0, inputs=preflight_inputs(pre_raw), judge=pre_judge) ==
                 'complete_relative', 'preflight probe not complete')
            validate_external_wall(pre_value, pre_row['elapsed_seconds'])
            validate_preflight_work(pre_value, pre_case['levers'])
            validate_gnu_time((output / 'preflight.stderr').read_text(errors='replace'), 0)
        except (ValueError, KeyError, TypeError, UnicodeError) as error:
            raise PreflightFailed(type(error).__name__ + ': ' + str(error)) from error
        result['preflight'] = dict(sites=pre_case['n'], tower_digest=pre_value['tower_digest'])
        if pre_case['levers']['q34_batch_filter']:
            # Engine-path twin of the preflight: the batch (and device) tower
            # must equal the engine tower before any LiDAR case runs.
            engine_case = dict(pre_case, levers=engine_levers(pre_case['levers']))
            engine_row = worker.command('preflight_engine', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                                             *expected_probe_tail(engine_case)])
            try:
                need(engine_row['exit_code'] == 0, 'engine preflight exit code')
                engine_value = strict_json((output / 'preflight_engine.stdout').read_bytes())
                need(validate_probe(engine_value, engine_case, 0, inputs=preflight_inputs(pre_raw)) ==
                     'complete_relative', 'engine preflight not complete')
                validate_gnu_time((output / 'preflight_engine.stderr').read_text(errors='replace'), 0)
                need(engine_value['tower_digest'] == pre_value['tower_digest'] and
                     logical_result(engine_value) == logical_result(pre_value) and
                     certificate_work(engine_value) == certificate_work(pre_value),
                     'batch/GPU preflight tower or certificate work differs from the engine path')
                # v20: the engine twin proves the work of the levers the batch
                # arm legitimately bypasses (leaf census, witness cache).
                validate_preflight_work(engine_value, engine_case['levers'])
            except (ValueError, KeyError, TypeError, UnicodeError) as error:
                raise PreflightFailed(type(error).__name__ + ': ' + str(error)) from error
            result['preflight']['engine_tower_digest'] = engine_value['tower_digest']
        if pre_case['levers']['q34_gpu_certificates'] or pre_case['levers']['q34_gpu_q3']:
            # v18: the device deferral path on real hardware, before any LiDAR
            # case: a 64-site slab must defer some edges to the engine path and
            # still give the engine tower, catalogue and certificate work.
            # v20: and a 24-site q3 lanes slab some q3 lanes to the CPU tail.
            capacity, lanes_capacity = deferral_capacities(pre_case['levers'])
            deferral_row = worker.command('preflight_deferral', [TIME, '-v', str(binary), str(output / PREFLIGHT_FILE),
                                                                 *expected_probe_tail(pre_case, capacity, judge=True,
                                                                                      lanes_capacity=lanes_capacity)])
            try:
                need(deferral_row['exit_code'] == 0, 'deferral preflight exit code')
                deferral_value = strict_json((output / 'preflight_deferral.stdout').read_bytes())
                need(validate_probe(deferral_value, pre_case, 0, inputs=preflight_inputs(pre_raw),
                                    capacity=capacity, judge=True, lanes_capacity=lanes_capacity) ==
                     'complete_relative', 'deferral preflight not complete')
                validate_gnu_time((output / 'preflight_deferral.stderr').read_text(errors='replace'), 0)
                need(logical_result(deferral_value) == logical_result(pre_value) and
                     certificate_work(deferral_value) == certificate_work(pre_value),
                     'reduced-slab preflight differs from the default-slab preflight')
            except (ValueError, KeyError, TypeError, UnicodeError) as error:
                raise PreflightFailed(type(error).__name__ + ': ' + str(error)) from error
            result['preflight']['deferred'] = deferral_value['q34_batch']['deferred']
            result['preflight']['lanes_deferred'] = deferral_value['q34_batch']['lanes_deferred']
        if uses_device(pre_case['levers']):
            # Validated device pass on the synthetic preflight only (auditor B):
            # GPU_executed waits for a complete LiDAR tower on the device.
            result['GPU_preflight_executed'] = True

        def left():
            try:
                return worker.remaining()
            except helper.SessionDeadline:
                return 0.0
        outcomes, values, exhausted, defect = result['case_outcomes'], {}, False, False
        for index, case in enumerate(cases):
            suffix = str(index)
            if defect:
                outcomes.append(dict(index=index, outcome='skipped_protocol_defect'))
                continue
            if exhausted or left() < MIN_CASE_START_SECONDS:
                exhausted = True
                outcomes.append(dict(index=index, outcome='skipped_budget'))
                continue
            need(worker.command('uptime_before_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
            started = time.time()
            capped = started + args.case_cap_seconds < useful_deadline
            case_worker = helper.Worker(output, min(started + args.case_cap_seconds, useful_deadline), schedule)
            name = 'probe_' + suffix
            result['FULL_executed'] = True
            if uses_device(case['levers']):
                result['GPU_attempted'] = True
            killed, row = False, None
            try:
                row = case_worker.command(name, [TIME, '-v', *probe_command(build, root, case)])
            except helper.SessionDeadline:
                killed = True
            finally:
                worker.commands.extend(case_worker.commands)
            if killed:
                if not case_worker.commands:
                    exhausted = True
                    outcomes.append(dict(index=index, outcome='skipped_budget'))
                    continue
                row = case_worker.commands[-1]
                # Fail closed: a killed case whose process group is not
                # certified closed ends the campaign (nothing may still run).
                need(row.get('group_closed') is True, 'killed case process group not certified closed')
                outcomes.append(dict(index=index, outcome='killed_case_cap' if capped else 'killed_budget',
                                     exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds']))
                exhausted = not capped
            else:
                entry = dict(index=index, exit_code=row['exit_code'], elapsed_seconds=row['elapsed_seconds'])
                try:
                    need(row['exit_code'] in (0, 3), 'probe exit code outside {0, 3}')
                    value = strict_json((output / (name + '.stdout')).read_bytes())
                    outcome = validate_probe(value, case, row['exit_code'])
                    validate_external_wall(value, row['elapsed_seconds'])
                    rss = validate_gnu_time((output / (name + '.stderr')).read_text(errors='replace'), row['exit_code'])
                    values[index] = value
                    entry.update(outcome=outcome, probe_status=value['status'], tower_digest=value['tower_digest'],
                                 gnu_time_max_rss_kb=rss, chain_total_ms=value['times_ms']['chain_total'])
                except (ValueError, KeyError, TypeError, UnicodeError) as error:
                    entry.update(outcome='probe_failed', reason=type(error).__name__ + ': ' + str(error))
                    defect = True
                outcomes.append(entry)
                need(sha(binary) == binary_sha, 'binary changed')
            save(output / (name + '.summary.json'), dict(case=case, input_file_sha256=manifest[case['file']],
                                                         **outcomes[-1]))
            if not exhausted and left() >= 5:
                need(worker.command('uptime_after_' + suffix, ['uptime'])['exit_code'] == 0, 'uptime')
        need(result['FULL_executed'], 'no case started within the useful budget')
        result['completed_case_indices'] = [e['index'] for e in outcomes if e['outcome'] == 'complete_relative']
        result['cross_worker_comparisons'] = compare_cases(cases, outcomes, values)
        # A batch/GPU tower counts as verified on LiDAR only with its twin.
        result['unpaired_batch_cases'] = unpaired_batch_cases(cases, outcomes)
        result['GPU_completed_cases'] = gpu_completed_cases(cases, outcomes, values)
        result['GPU_executed'] = bool(result['GPU_completed_cases'])
        if any(e['outcome'] == 'probe_failed' for e in outcomes):
            result['status'] = 'probe_failed'
        elif not all(item['equal'] for item in result['cross_worker_comparisons']):
            result['status'] = 'cross_worker_mismatch'
        elif len(result['completed_case_indices']) == len(cases):
            result['status'] = 'completed'
        else:
            result['status'] = 'partial'
    except BuildFailed as error:
        result.update(status='build_failed', error=str(error))
    except PreflightFailed as error:
        result.update(status='preflight_failed', error=str(error))
    except BaseException as error:
        result.update(status='failed', error=type(error).__name__ + ': ' + str(error))
    finally:
        result['commands'] = worker.commands if worker else []
        result['elapsed_before_closure_seconds'] = time.time() - began
        try:
            if before is not None:
                after = source_map(root, manifest)
                save(output / 'sources_after.json', after)
                result['sources_stable'] = before == after
            if binary_sha is not None:
                result['binary_stable'] = sha(binary) == binary_sha
                need(result['binary_stable'], 'binary changed at closure')
            for name, pin in result.get('tool_sha256', {}).items():
                need(sha(tools[name]) == pin, 'tool changed: ' + name)
            for name, pin in consumed.items():
                need(sha(name) == pin, 'compiled dependency changed at closure')
            result['compiled_dependencies_stable'] = bool(consumed)
        except Exception as error:
            result.update(status='failed', closure_error=type(error).__name__ + ': ' + str(error))
        save(output / 'receipt.json', result)
        print(json.dumps(dict(status=result['status'], targeted_GCP_stop_required_by_ROOT=True)), flush=True)
        for signum, handler in previous.items():
            signal.signal(signum, handler)
    return 0 if result['status'] in ('completed', 'partial') else 1


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('source-root', 'source-manifest', 'guard-mark', 'output'):
        parser.add_argument('--' + key, type=Path)
    for key in ('source-manifest-sha256', 'guard-mark-sha256', 'project', 'zone', 'instance', 'generation'):
        parser.add_argument('--' + key)
    parser.add_argument('--session-deadline-epoch', type=int)
    parser.add_argument('--closing-margin-seconds', type=int, default=300)
    parser.add_argument('--useful-budget-seconds', type=int, default=USEFUL_BUDGET_SECONDS)
    parser.add_argument('--case-cap-seconds', type=int, default=CASE_CAP_SECONDS)
    parser.add_argument('--execute', action='store_true')
    args = parser.parse_args(argv)
    if not args.execute:
        print(json.dumps(dict(status='inert', target=TARGET, backend='reference_cpu', scope=SCOPE,
                              useful_budget_seconds=USEFUL_BUDGET_SECONDS, case_cap_seconds=CASE_CAP_SECONDS,
                              GPU_executed=False, FULL_executed=False), sort_keys=True))
        return 0
    need(all(getattr(args, key.replace('-', '_')) is not None for key in (
        'source-root', 'source-manifest', 'guard-mark', 'output', 'source-manifest-sha256',
        'guard-mark-sha256', 'project', 'zone', 'instance', 'generation', 'session-deadline-epoch')), 'missing arguments')
    args.argv = ['worker.py', *argv]
    return execute(args)


if __name__ == '__main__':
    raise SystemExit(main())
