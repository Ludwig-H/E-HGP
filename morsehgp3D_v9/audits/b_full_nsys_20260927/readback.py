#!/usr/bin/env python3
"""Replay the closed r1 precondition failure, not a successful profile; no GCP."""
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path
import re
import tarfile
import capture as c
import worker as w

SCHEMA = 'mhgp9_full_nsys_readback_v1'
HERE = Path(__file__).resolve().parent
TEXT_FILES = ('receipt.json','guard_evidence.json','sources_before.json','sources_after.json')


def unique(pairs):
    result = {}
    for key,value in pairs:
        c.need(key not in result, 'duplicate JSON key')
        result[key] = value
    return result


def read(path):
    return json.loads(Path(path).read_bytes(),object_pairs_hook=unique)


def encoded(value):
    return (json.dumps(value,indent=2,sort_keys=True,allow_nan=False)+'\n').encode()


def safe_text(path):
    raw = Path(path).read_bytes()
    raw.decode('utf-8')
    c.need(b'PRIVATE KEY' not in raw and not re.search(rb'(?:ssh-ed25519|ssh-rsa|ecdsa-sha2-\S+) [A-Za-z0-9+/=]+',raw),
           'no key material in published text')
    return raw


def projection(after):
    # Same explicit projection used by the preceding S2 publication reader.
    return {key:after[key] for key in ('name','selfLink','status','lastStartTimestamp','lastStopTimestamp',
            'zone','machineType','labels','scheduling') if key in after}


def stop_check(native,host,verdict,after):
    generation = host.get('generation')
    c.need(host.get('targeted_shutdown_certified') is True and
           verdict.get('targeted_shutdown_certified') is True and verdict.get('generation') == generation,
           'certified same-generation stop')
    native.validate_target(after,'TERMINATED',generation)
    begin = datetime.fromisoformat(generation.replace('Z','+00:00'))
    end = datetime.fromisoformat(after['lastStopTimestamp'].replace('Z','+00:00'))
    c.need(begin.tzinfo is not None and end.tzinfo is not None and end >= begin, 'stop chronology')
    elapsed = (end-begin).total_seconds()
    c.need(verdict.get('vm_elapsed_seconds') == elapsed, 'stop duration binding')
    return dict(generation=generation,stopped=after['lastStopTimestamp'],vm_elapsed_seconds=elapsed,
                billed_cost_usd=None)


def check_command(directory,name,row,remember):
    command,intent = directory/(name+'.command.json'),directory/(name+'.intent.json')
    remember(command);remember(intent)
    c.need(read(command) == row and row.get('group_closed') is True and
           type(row.get('exit_code')) is int, 'closed command record')
    planned = read(intent)
    stable = ('name','argv','started_epoch','timeout_seconds')
    c.need(all(row.get(key) == planned.get(key) for key in stable), 'intent/command binding')
    start,end = row['started_epoch'],row['ended_epoch']
    c.need(end >= start, 'command chronology')
    for stream in ('stdout','stderr'):
        path = directory/(name+'.'+stream);remember(path)
        c.need(w.sha(path) == row[stream+'_sha256'], 'command stream hash')
    return start,end


def failed_worker(vm,host,verdict,manifest,before,after,target,worker_pin):
    c.need(vm.get('schema') == w.SCHEMA and vm.get('scope') == 'FULL_profile_diagnostic_only' and
           vm.get('target') == target and vm.get('generation') == host['generation'] and
           vm.get('engine_commit') == w.ENGINE_COMMIT and vm.get('worker_sha256') == worker_pin and
           vm.get('source_manifest_sha256') == c.MANIFEST_SHA and vm.get('native_validator_sha256') == w.PAYLOAD_PIN and
           vm.get('collector_sha256') == w.HELPER_PIN and vm.get('public_status') == 'not_claimed' and
           vm.get('contract_certified') is False and vm.get('useful_budget_seconds') == w.USEFUL_SECONDS,
           'same diagnostic worker identity and scope, including failure')
    c.need(host.get('status') == verdict.get('status') == 'worker_failed' and vm.get('status') == 'failed' and
           type(verdict.get('controller_exit')) is int and verdict['controller_exit'] == 1 and
           type(host.get('worker_exit_code')) is int and host['worker_exit_code'] == 1,'closed failure, never success')
    c.need(vm.get('sources_stable') is True and before == after == manifest, 'source closure')
    c.need(vm.get('commands') == [] and all(vm.get(key) is False for key in
           ('FULL_executed','CUDA_profile_executed','CUDA_installation_attempted','system_installation_attempted')),
           'failure before commands, execution or installation')
    c.need(vm.get('error') == 'ValueError: existing regular qualified artifact: '+str(w.BINARY) and
           not any(key in vm for key in ('artifacts_before','artifacts_after','libraries_before','libraries_after',
                                       'reports','activity','nsys','closure_error')), 'r1 artifact precondition failure')
    c.need(type(vm.get('elapsed_seconds')) in (int,float) and math.isfinite(vm['elapsed_seconds']) and
           0 <= vm['elapsed_seconds'] <= w.USEFUL_SECONDS, 'bounded worker duration')


def collect(session_dir):
    session_dir = session_dir.absolute();host = session_dir/'full_host';output = host/'received/output'
    public,private = {},{}
    def remember(path):
        path = Path(path)
        c.need(path.is_file() and not path.is_symlink(), 'regular private evidence: '+str(path))
        private[str(path)] = w.sha(path)
    def publish_text(path,name):
        remember(path);public[name] = safe_text(path)
    native,_ = c.runtime()
    provenance = c.package_check(native)
    launch = read(session_dir/'launch.json');verdict = read(session_dir/'verdict.json')
    receipt = read(host/'receipt.json');after = read(session_dir/'after_stop.json')
    for path in (session_dir/'launch.json',session_dir/'verdict.json',session_dir/'after_stop.json',
                 host/'receipt.json',host/'source_manifest.json',host/'snapshot.tar.gz',host/'worker.py',
                 c.HERE/'capture.py',c.HERE/'worker.py',HERE/'readback.py',HERE/'selftest_readback.py',c.PACKAGE/'PACKAGE.json',
                 c.PACKAGE/'snapshot.tar.gz',c.PACKAGE/'source_manifest.json'):
        remember(path)
    c.committed(launch['diagnostic_commit'])
    c.need(launch.get('worker_sha256') == w.sha(w.__file__) == receipt.get('worker_sha256') == w.sha(host/'worker.py') and
           launch.get('wrapper_sha256') == w.sha(c.__file__) and launch.get('helper_sha256') == c.HELPER_SHA and
           receipt.get('controller_sha256') == c.HELPER_SHA and launch.get('native_package_provenance') == provenance,
           'committed diagnostic and native package binding')
    c.need(receipt.get('target') == native.TARGET and receipt.get('snapshot_sha256') == w.sha(host/'snapshot.tar.gz') == c.SNAPSHOT_SHA and
           receipt.get('manifest_sha256') == w.sha(host/'source_manifest.json') == c.MANIFEST_SHA, 'transported original package')
    cost = stop_check(native,receipt,verdict,after)
    c.need(verdict.get('status') == receipt.get('status') and verdict.get('contract_certified') is False and
           verdict.get('scope') == 'FULL_profile_diagnostic_not_contract', 'closed host verdict')
    names = set()
    for row in receipt['commands']:
        name = row['name']
        c.need(re.fullmatch('[a-z0-9_]+',name) and name not in names,'unique host command')
        names.add(name);check_command(host,name,row,remember)
    c.need('guarded_stop' in names and receipt.get('capture_received') is True and
           receipt.get('capture_pack_exit_code') == 0, 'closed capture transport')
    remember(host/'capture.tar.gz')
    c.need(w.sha(host/'capture.tar.gz') == receipt.get('capture_sha256'),'capture archive hash')
    stop = next(row for row in receipt['commands'] if row['name'] == 'guarded_stop')
    c.need(stop['exit_code'] == 0 and stop['argv'] == [str(host/'stop_and_verify.sh'),'--yes',
           '--expected-last-start-timestamp',receipt['generation']], 'same-generation guarded stop command')
    manifest = read(host/'source_manifest.json')
    native.validate_snapshot(host/'snapshot.tar.gz',manifest)
    c.need({path.name for path in output.iterdir()} == set(TEXT_FILES), 'r1 output allowlist: no trace or other command')
    for name in TEXT_FILES:
        publish_text(output/name,'vm/'+name)
    with tarfile.open(host/'capture.tar.gz','r:gz') as archive:
        members = archive.getmembers()
        c.need(len(members) == 5 and {member.name for member in members} == {'output',*('output/'+name for name in TEXT_FILES)},
               'r1 received archive allowlist')
        for member in members:
            if member.name == 'output':c.need(member.isdir(),'archive output directory');continue
            c.need(member.isfile() and archive.extractfile(member).read() == (host/'received'/member.name).read_bytes(),
                   'received text bound to capture archive')
    vm = read(output/'receipt.json')
    failed_worker(vm,receipt,verdict,manifest,read(output/'sources_before.json'),read(output/'sources_after.json'),
                  native.TARGET,launch['worker_sha256'])
    mark = native.fields((host/'guardmarks/double_guard_verified').read_text())
    schedule = native.fields((host/'guest_schedule.stdout').read_text())
    remember(host/'guardmarks/double_guard_verified');remember(host/'guest_schedule.stdout')
    c.need(read(output/'guard_evidence.json') == dict(mark=mark,schedule=schedule),'same guard evidence')
    native.legacy.guard_deadline(mark,schedule,receipt['generation'],native.epoch(mark['date_utc']))
    summary = dict(schema=SCHEMA,status='failed',failure_replay=True,semantic_replay=False,scope='FULL_profile_diagnostic_not_contract',
        diagnostic_commit=launch['diagnostic_commit'],engine_commit=w.ENGINE_COMMIT,contract_certified=False,
        cost=cost,host_status=receipt['status'],worker_status=vm['status'],worker_error=vm['error'],
        worker_elapsed_seconds=vm['elapsed_seconds'],command_count=0,FULL_executed=False,CUDA_profile_executed=False,
        Nsight_downloaded=False,trace_produced=False,sources_stable=True,GPU_execution='not_executed')
    # Text-only publication: no raw GCE/OS Login, keys, input or binary report.
    public['after_stop.json'] = encoded(projection(after))
    public['launch.json'] = encoded(launch);public['verdict.json'] = encoded(verdict)
    public['SUMMARY.json'] = encoded(summary)
    public['PRIVATE_LINKS.json'] = encoded(dict(session=str(session_dir),pins=private))
    public['README.md'] = ('# Nsight FULL r1 — échec clos\n\n'
        'Binaire historique non disponible sous la forme attendue : aucun processus FULL, téléchargement Nsight ou profilage. '
        'Le contrôle ne distingue pas absence, accès refusé et type de fichier inadmissible ; message brut dans SUMMARY.json. '
        'Aucune trace ni mesure GPU acquise. Arrêt certifié de la même génération ; allocation '
        +str(cost['vm_elapsed_seconds'])+' s, sans estimation de prix.\n\n'
        'Lecture LIVE : `python3 -B morsehgp3D_v9/audits/b_full_nsys_20260927/readback.py --readback '
        'morsehgp3D_v9/receipts/full_nsys_20260927/r1`. Les preuves privées référencées doivent rester disponibles. '
        'Seuls les JSON VM, le lancement, le verdict et la projection d’arrêt sont publiés ; '
        'aucune clé, donnée KITTI, réponse GCE/OS Login brute ou rapport binaire.\n').encode()
    return summary,public


def publish(session_dir,destination):
    summary,files = collect(session_dir)
    destination.mkdir(parents=True,exist_ok=False)
    for name,raw in files.items():
        path = destination/name;path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('xb') as stream:stream.write(raw)
    inventory = {name:hashlib.sha256(raw).hexdigest() for name,raw in files.items()}
    (destination/'SHA256SUMS').write_text(''.join(pin+'  '+name+'\n' for name,pin in sorted(inventory.items())))
    return summary


def replay(directory):
    inventory = {}
    for line in (directory/'SHA256SUMS').read_text().splitlines():
        pin,name = line.split('  ',1)
        c.need(name not in inventory and re.fullmatch('[0-9a-f]{64}',pin),'publication inventory')
        inventory[name] = pin
    c.need(not any(path.is_symlink() for path in directory.rglob('*')), 'regular publication without symlinks')
    actual = {str(path.relative_to(directory)):w.sha(path) for path in directory.rglob('*')
              if path.is_file() and path.name != 'SHA256SUMS'}
    c.need(actual == inventory,'published file closure')
    links = read(directory/'PRIVATE_LINKS.json')
    c.need(all(Path(path).is_file() and not Path(path).is_symlink() and w.sha(path) == pin
               for path,pin in links['pins'].items()),'private LIVE evidence closure')
    summary,files = collect(Path(links['session']))
    c.need(all((directory/name).read_bytes() == raw for name,raw in files.items()) and set(files) == set(inventory),
           'publication reproduces from closed private evidence')
    return summary


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument('--session',type=Path);modes.add_argument('--readback',type=Path)
    parser.add_argument('--publish',type=Path)
    args = parser.parse_args()
    c.need(args.publish is None or args.session is not None,'publication needs private session')
    value = replay(args.readback) if args.readback else (
        publish(args.session,args.publish) if args.publish else collect(args.session)[0])
    print(json.dumps(value,sort_keys=True))
