"""Independent analytical cross-check of four-site inverse-beta evidence.

Prints a receipt; does not write files or invoke native code. The 44 checks
use elementary diameter formulas, not the imported Fraction MEB routines.
The separate Gamma replay shares the original frozen Fraction reference.
"""
from fractions import Fraction as Q
from pathlib import Path
import hashlib
import json
import math
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / 'source_snapshot'
EXPECTED_ROWS = 'd9a5e8a52c7f21cf29acfbedeb4c8ab54f216e58697bf19c2243ec42b73fc85f'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def source_hashes():
    paths = [Path(__file__).resolve(), SOURCE / 'check_shell.py',
             SOURCE / 'frozen_reference.py', ROOT / 'original_packet_SHA256SUMS']
    return {str(p.relative_to(ROOT)): digest(p) for p in paths}

def original_hashes():
    if len(sys.argv) == 1:
        return None
    if len(sys.argv) != 2:
        raise RuntimeError('usage: review.py [ORIGINAL_PACKET]')
    directory = Path(sys.argv[1])
    result = {}
    for line in (ROOT / 'original_packet_SHA256SUMS').read_text().splitlines():
        expected, name = line.split(maxsplit=1)
        got = digest(directory / name)
        if got != expected:
            raise RuntimeError('original packet hash mismatch: ' + name)
        result[name] = got
    return result

start = time.monotonic()
before = source_hashes()
original_before = original_hashes()
argv = [sys.executable, '-B'] + (['-O'] if sys.flags.optimize else [])
argv += [str(SOURCE / 'check_shell.py'), str(SOURCE / 'frozen_reference.py')]
replay = subprocess.run(argv, capture_output=True, text=True, timeout=10)
if replay.returncode != 0 or replay.stderr:
    raise RuntimeError('Fraction replay failed: ' + repr((replay.returncode, replay.stderr)))
data = json.loads(replay.stdout)
row_hash = hashlib.sha256(json.dumps(data['rows'], sort_keys=True, separators=(',', ':')).encode()).hexdigest()
if row_hash != EXPECTED_ROWS:
    raise RuntimeError('semantic rows differ from closed original captures')

rows = []
for row in data['rows']:
    M = row['M']
    old, new = row['original'], row['perturbed']
    a, b = old['at_local_beta'], new['at_local_beta']
    beta_old = [Q(4*M*M), Q(25*M*M, 4), Q(2500*M*M),
                Q(41*M*M, 4), Q(2504*M*M), Q(10025*M*M, 4)]
    beta_new = [Q((4*M-1)**2+2, 4), Q((5*M-1)**2+2, 4),
                Q((100*M-1)**2+2, 4)]
    W_old = sum((1/x for x in beta_old), Q())
    W_new = sum((1/x for x in beta_new), Q())
    atoms = a['atoms'] + b['atoms']
    checks = {
        'six_original_diameter_betas': sorted(Q(x['beta']) for x in a['atoms']) == sorted(beta_old),
        'three_perturbed_diameter_betas': sorted(Q(x['beta']) for x in b['atoms']) == sorted(beta_new),
        'original_C_weight_total': Q(a['total_weights'][0]) == W_old,
        'perturbed_C_weight_total': Q(b['total_weights'][0]) == W_new,
        'original_unique_local_CA_cover': a['gamma_covers'] == [[0, 1]],
        'perturbed_unique_local_CA_cover': b['gamma_covers'] == [[0, 1]],
        'positive_inverse_beta_strong_K2_atoms': all(x['I'] == [] and x['qmin'] == 2 and
            Q(x['beta']) > 0 and Q(x['weight']) == 1/Q(x['beta']) for x in atoms),
        'K2_alive_population': all(len(x['U']) >= 2 for x in atoms),
        'original_CA_projection_date': Q(old['CA_first_beta']) == Q(41*M*M, 4),
        'perturbed_CA_projection_date': Q(new['CA_first_beta']) == beta_new[0],
        'u18_coordinate_domain': all(0 <= v < 2**18 for part in (old, new) for point in part['points'] for v in point),
    }
    if not all(checks.values()):
        raise RuntimeError('analytical check failed at M=' + str(M) + ': ' + repr(checks))
    margins = [(2/Q(old['local_beta'])-W_old)/W_old,
               (2/Q(new['local_beta'])-W_new)/W_new]
    rows.append({'M': M, 'checks': checks,
                 'majority_margin_ratio_before': str(margins[0]),
                 'majority_margin_ratio_after': str(margins[1]),
                 'approx_margin_ratios': [float(x) for x in margins],
                 'cross_diameter_powers_after': [3-9*M, 3-104*M, 3-105*M],
                 'max_coordinate': max(v for part in (old, new) for point in part['points'] for v in point),
                 'FULL_ABC_fusion_beta': row['FULL_ABC_fusion_beta_before_and_after'],
                 'FULL_fusion_p_q_before': row['FULL_fusion_p_q_before'],
                 'FULL_fusion_p_q_after': row['FULL_fusion_p_q_after'],
                 'radius_before_after': [math.sqrt(float(Q(old['CA_first_beta']))),
                                        math.sqrt(float(Q(new['CA_first_beta'])))]})

after = source_hashes()
original_after = original_hashes()
if before != after or original_before != original_after:
    raise RuntimeError('sources or original packet changed during review')
print(json.dumps({'status': 'COUNTER_REVIEW_PASS', 'optimized': sys.flags.optimize,
                  'scope': '44 elementary exact analytical checks plus frozen Fraction Gamma replay',
                  'native_calls': 0, 'GCP_used': False, 'engine_modified': False,
                  'analytical_checks': sum(len(x['checks']) for x in rows),
                  'fraction_pair_nesting_checks': sum(x[k]['nesting_pair_checks'] for x in data['rows']
                                                     for k in ('original', 'perturbed')),
                  'semantic_rows_sha256': row_hash, 'rows_equal_to_original_capture': True,
                  'sources_before': before, 'sources_after': after,
                  'original_packet_before': original_before, 'original_packet_after': original_after,
                  'replay': {'argv': argv, 'returncode': replay.returncode, 'stderr': replay.stderr,
                             'status': data['status'], 'source_hashes_before': data['hashes_before'],
                             'source_hashes_after': data['hashes_after']},
                  'rows': rows, 'wall_seconds': time.monotonic()-start}, indent=2))
