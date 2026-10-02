"""LIVE startup-only reader: preserve controller failure; verify later read-only closure observations."""
from datetime import datetime
import json
from pathlib import Path
from urllib.parse import urlparse

SOURCE = '25792084eb4e672c5222d62f5b2ae87bd2ee4948'
FIELDS = ('schema', 'source_kind', 'evidence_grade', 'commit', 'worker_source', 'worker_plan_sha256',
          'plan_sha256', 'package_sha256', 'status', 'closure', 'generation', 'pre_start_generation',
          'start_certified', 'start_may_have_been_requested', 'gcp_mutation_phase_entered',
          'targeted_shutdown_certified', 'worker_launched', 'worker_exit_code', 'worker_outcome',
          'results_verified', 'private_key_deleted', 'oslogin_key_removed', 'reserve_released')


def encoded(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True).encode()


def views(raw, external, api):
    need = api.need
    need(raw['schema'] == 'ehgp.v11.session_receipt.v1' and raw['source_kind'] == 'commit' and
         raw['evidence_grade'] == 'pushed_commit' and raw['commit'] == SOURCE and
         raw['worker_source'] == 'commit:' + SOURCE,
         'stockout_source')
    for key in ('plan_sha256', 'worker_plan_sha256', 'package_sha256'):
        need(type(raw[key]) is str and api.HEX.fullmatch(raw[key]), 'stockout_hash_forme')
    need(raw['status'] == 'shutdown_uncertified' and raw['closure'] == 'generation_unknown' and
         raw['generation'] is None and raw['worker_exit_code'] is raw['worker_outcome'] is None,
         'stockout_statut_controleur')
    for key in ('start_certified', 'targeted_shutdown_certified', 'worker_launched', 'results_verified'):
        need(raw[key] is False, 'stockout_fausse_execution')
    for key in ('start_may_have_been_requested', 'gcp_mutation_phase_entered', 'private_key_deleted',
                'oslogin_key_removed', 'reserve_released'):
        need(raw[key] is True, 'stockout_nettoyage_demande')
    starts = [c for c in raw['host_commands'] if c['name'] == 'guarded_start']
    need(len(starts) == 1 and type(starts[0]['exit_code']) is int and starts[0]['exit_code'] != 0,
         'stockout_lancement_refuse')
    need(external['schema'] == 'ehgp.v11.external_closure.v1' and
         external['session_status_unchanged'] == raw['status'] and
         external['conclusion'] == 'target_terminated_no_new_generation_after_failed_start' and
         type(external['controller_recovery_code']) is int and external['controller_recovery_code'] == 74,
         'stockout_fermeture_externe')
    reads = external['reads']
    need(type(reads) is list and len(reads) == 2, 'stockout_lectures')
    target = raw['target']
    observation, operations = reads[0]['response'], reads[1]['response']
    need(reads[0]['argv'][1:5] == ['compute', 'instances', 'describe', target['instance']] and
         '--project=' + target['project'] in reads[0]['argv'] and
         '--zone=' + target['zone'] in reads[0]['argv'] and
         reads[1]['argv'][1:4] == ['compute', 'operations', 'list'] and
         '--project=' + target['project'] in reads[1]['argv'] and
         '--zones=' + target['zone'] in reads[1]['argv'] and
         [a for a in reads[1]['argv'] if a.startswith('--filter')] ==
         ['--filter=targetLink~' + target['instance'] + ' AND operationType=start'] and
         not any(a.startswith('--limit') for a in reads[1]['argv']), 'stockout_commandes_cible')
    need(raw['observed_after']['status'] == observation['status'] == 'TERMINATED' and
         raw['observed_after']['name'] == observation['name'] == target['instance'] and
         raw['observed_after']['lastStartTimestamp'] == observation['lastStartTimestamp'] ==
         raw['pre_start_generation'], 'stockout_ancienne_generation')
    need(type(operations) is list and operations and
         len({o['name'] for o in operations}) == len(operations), 'stockout_operations_inventaire')
    suffix = '/projects/%s/zones/%s/instances/%s' % (target['project'], target['zone'], target['instance'])
    for operation in operations:
        need(operation['operationType'] == 'start' and
             urlparse(operation['targetLink']).path.endswith(suffix), 'stockout_operation_cible')
    pending = sum(o['status'] != 'DONE' for o in operations)
    latest = max(operations, key=lambda o: datetime.fromisoformat(o['insertTime']))
    codes = [e['code'] for e in latest['error']['errors']]
    need(pending == 0 and latest['status'] == 'DONE' and
         codes == ['ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS'], 'stockout_pas_de_start_en_cours')
    parse = datetime.fromisoformat
    need(parse(raw['pre_start_generation']) < parse(latest['insertTime']) <= parse(latest['endTime']) <=
         parse(reads[0]['observed_at']) <= parse(reads[1]['observed_at']) and
         parse(observation['lastStopTimestamp']) <= parse(reads[0]['observed_at']), 'stockout_dates')
    safe = dict(schema=external['schema'], session_status_unchanged=external['session_status_unchanged'],
                conclusion=external['conclusion'], controller_recovery_code=external['controller_recovery_code'],
                observed_at=reads[0]['observed_at'], operations_observed_at=reads[1]['observed_at'],
                observation={k: observation[k] for k in ('status', 'lastStartTimestamp', 'lastStopTimestamp')},
                operations_count=len(operations), pending_start_count=pending,
                latest_start={k: latest[k] for k in ('operationType', 'status', 'insertTime', 'endTime')},
                latest_error_codes=codes, target_sha256=api.sha(encoded(target)),
                observation_sha256=api.sha(encoded(observation)), latest_start_sha256=api.sha(encoded(latest)))
    return {k: raw[k] for k in FIELDS}, safe


def check(folder, api):
    need = api.need
    need(folder.name == 'meb2' and {p.name for p in folder.iterdir()} == {'receipt.json', 'external_closure.json'},
         'stockout_capture_sans_archive')
    compact = api.js((folder / 'receipt.json').read_bytes())
    raw_path, closure_path = Path(compact['raw_receipt_local']), Path(compact['external_closure_local'])
    need(raw_path.name == 'receipt.json' and closure_path == raw_path.parent / 'external_closure.json' and
         raw_path.parent.name == 'v11.20261002.meb2', 'stockout_origine_locale')
    raw_bytes, external_bytes = raw_path.read_bytes(), closure_path.read_bytes()
    need(api.sha(raw_bytes) == compact['original_receipt_sha256'] and
         api.sha(external_bytes) == compact['original_external_closure_sha256'], 'stockout_hash_originaux')
    need((raw_path.parent / 'DONE').read_text().strip() == '74' and
         not (raw_path.parent / 'results/results.tar.gz').exists(), 'stockout_aucun_worker')
    receipt, closure = views(api.js(raw_bytes), api.js(external_bytes), api)
    pins = {k: compact[k] for k in ('raw_receipt_local', 'external_closure_local',
                                   'original_receipt_sha256', 'original_external_closure_sha256')}
    need(encoded(compact) == encoded(dict(receipt, **pins)) and
         encoded(api.js((folder / 'external_closure.json').read_bytes())) == encoded(closure),
         'stockout_compacts_differents')
    print('meb2 coherence=ok campagne=ECHEC_DEMARRAGE commit=' + SOURCE)
    print('  worker=non_lance essais=0 non_joues=18 controleur=shutdown_uncertified '
          'observation_externe=TERMINATED_ancienne_generation start=DONE_stockout pending=0')
    return True
