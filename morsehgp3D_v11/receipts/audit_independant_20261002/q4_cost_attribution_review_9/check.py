"""Frozen declared costs and work; no native execution or timing inference."""
from pathlib import Path
from fractions import Fraction
import json

r = Path(__file__).parent
base = r/'source/morsehgp3D_v11/receipts'
new = json.loads((base/'catalogue_q4_20261002/q4levels1/profiles.json').read_text())
old = json.loads((base/'catalogue_profiles_20261002/profiles1/profiles.json').read_text())
receipt = json.loads((base/'catalogue_q4_20261002/q4levels1/receipt.json').read_text())
checks = 0


def require(ok, message):
    global checks
    checks += 1
    if not ok:
        raise RuntimeError(message)


def key(row):
    return row['case'], row['coord_bits'], row['kmax'], row['repetition']


require(receipt['commit'] == 'ffc2ff95f0ae7296bdc522df81df34c58c3fdf47', 'executed source')
require(receipt['status'] == 'failed_remote', 'failed campaign preserved')
require(len(new['runs']) == 33 and len(new['not_run']) == 3, 'all requested units accounted')
require(new['complete'] is True and new['conforming'] is False, 'closed is not successful')
old_rows = {key(row): row for row in old['runs'] if row['status'] == 'ok'}
new_rows = [row for row in new['runs'] if row['status'] == 'ok']
require(len(new_rows) == len(old_rows) == 15, 'fifteen comparable successes')
metrics = []
for row in new_rows:
    previous = old_rows[key(row)]
    event, before = row['events'][1], previous['events'][1]
    require(row['kmax'] == 5 and row['whole_input'] is True, 'whole declared K5 input')
    cloud = row['events'][0]
    require(cloud['points'] == cloud['sites'] == row['count'], 'unique unit-weight sites in these completed inputs')
    require(row['canonical_sha256'] == previous['canonical_sha256'] and
            row['canonical_bytes'] == previous['canonical_bytes'], 'same declared raw serialization')
    require(row['semantic']['sha256'] == previous['semantic']['sha256'], 'same declared normalized geometry')
    require(event['logical'] == before['logical'], 'same nine geometric counters')
    for field in ('balls', 'levels', 'incidences', 'peak_reserved_bytes', 'reserved_after_bytes'):
        require(event[field] == before[field], 'same ' + field)
    C, L4 = event['work']['q4_candidates'], event['work']['q4_levels']
    require(0 <= L4 <= C < 1 << 64, 'native counter domain')
    require(L4 == row['qmin_counts']['4'], 'materializations equal canonical qmin4 balls on success')
    require(sum(row['qmin_counts'].values()) == event['balls'], 'qmin distribution covers output')
    n, N, L, P = row['count'], event['balls'], event['levels'], event['incidences']
    bits = row['coord_bits']
    emission, level = {18:(96,48), 21:(104,64), 24:(112,72)}[bits]
    U, E, F = 28*n+8, emission*N+4*P, 40*N+level*L+4*P+8
    require(U+E+F == event['peak_reserved_bytes'], 'old assembly phase equation still matches new peak')
    require(U+F == event['reserved_after_bytes'], 'old retained output equation still matches')
    saving = Fraction(C-L4, C)
    observed = Fraction(before['wall_ns'], event['wall_ns'])
    require(C > 0 and saving > Fraction(99,100), 'large reduction in level call count')
    metrics.append({'case': row['case'], 'bits': bits, 'q4_candidates_per_pass': C,
                    'q4_levels_per_pass': L4, 'avoided_level_calls_two_passes': 2*(C-L4),
                    'avoided_percent': round(float(100*saving), 6),
                    'old_api_seconds': before['wall_ns']/1e9, 'new_api_seconds': event['wall_ns']/1e9,
                    'observed_old_over_new': round(float(observed), 9),
                    'peak_reserved_bytes': U+E+F})
for name in {row['case'] for row in new_rows}:
    group = [x for x in metrics if x['case'] == name]
    require({x['bits'] for x in group} == {18,21,24}, 'all compiled profiles completed for case')
    require(len({(x['q4_candidates_per_pass'], x['q4_levels_per_pass']) for x in group}) == 1, 'same candidate work in all profiles')
require(len([x for x in new['runs'] if x['status'] != 'ok']) == 18, 'eighteen attempts remain failed')

# A conditional cost decomposition cannot identify a measured timing fraction.
# Different unchanged-cost shares all coexist with the same operation-count ratio.
s = Fraction(metrics[0]['q4_levels_per_pass'], metrics[0]['q4_candidates_per_pass'])
hypothetical = []
for fraction in (Fraction(1,20), Fraction(1,2), Fraction(9,10)):
    speed = 1/(1-fraction+fraction*s)
    require(1 < speed < 1/(1-fraction), 'conditional speed limit, no attribution from operation counts')
    hypothetical.append({'assumed_old_level_time_share': str(fraction), 'ideal_speed_if_rest_fixed': str(speed)})

print(json.dumps({'status':'PASS', 'checks':checks, 'metrics':metrics,
                  'hypothetical_only':hypothetical,
                  'scope':'closed declared costs/work and exact Buffer equations; not canonical payload rehash, native execution, repeated gain or phase timing'},
                 sort_keys=True, indent=2))
