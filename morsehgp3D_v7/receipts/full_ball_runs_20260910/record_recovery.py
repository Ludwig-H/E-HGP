#!/usr/bin/env python3
"""Read/verify exactly the refused SPOT target; never start or stop other VMs."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

OUT = Path(__file__).resolve().parent / 'recovery_r2'
OUT.mkdir(exist_ok=False)
ROOT = Path('/workspaces/E-HGP')
TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b', instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
GCLOUD = '/home/codespace/google-cloud-sdk/bin/gcloud'
env = dict(os.environ, GCP_PROJECT_ID=TARGET['project'], GCP_ZONE=TARGET['zone'], GCP_INSTANCE_NAME=TARGET['instance'])
env['PATH'] = str(Path(GCLOUD).parent) + os.pathsep + env['PATH']
rows = []


def command(name, argv):
    row = dict(name=name, argv=argv, started_epoch=time.time())
    result = subprocess.run(argv, cwd=ROOT, env=env, capture_output=True, timeout=300)
    (OUT / (name + '.stdout')).write_bytes(result.stdout)
    (OUT / (name + '.stderr')).write_bytes(result.stderr)
    row.update(exit_code=result.returncode, ended_epoch=time.time(),
               stdout_sha256=hashlib.sha256(result.stdout).hexdigest(), stderr_sha256=hashlib.sha256(result.stderr).hexdigest())
    rows.append(row)
    if result.returncode:
        raise RuntimeError(name + ': nonzero exit')
    return result.stdout


receipt = dict(status='failed', target=TARGET, gpu_benchmark_executed=False,
               controller_original_status='shutdown_uncertified', controller_original_preserved=True)
try:
    flags = ['--project=' + TARGET['project'], '--zone=' + TARGET['zone'],
             '--format=json(name,status,lastStartTimestamp,lastStopTimestamp,labels,machineType,scheduling)']
    before = json.loads(command('describe_before', [GCLOUD, 'compute', 'instances', 'describe', TARGET['instance'], *flags]))
    generation = '2026-09-06T06:19:11.593-07:00'
    if before['status'] != 'TERMINATED' or before['lastStartTimestamp'] != generation or before['labels']['project'] != 'e-hgp':
        raise RuntimeError('refused start target changed; never adopt another generation')
    command('guarded_stop', ['./gcp-migration/stop_and_verify.sh', '--yes', '--expected-last-start-timestamp', generation])
    after = json.loads(command('describe_after', [GCLOUD, 'compute', 'instances', 'describe', TARGET['instance'], *flags]))
    if after['status'] != 'TERMINATED' or after['lastStartTimestamp'] != generation:
        raise RuntimeError('target not certified stopped')
    command('active_inventory', [GCLOUD, 'compute', 'instances', 'list', '--project=' + TARGET['project'],
        '--filter=status!=TERMINATED', '--format=json(name,zone,status,labels,machineType,guestAccelerators,scheduling.provisioningModel)'])
    receipt.update(status='refused_start_target_shutdown_certified', targeted_shutdown_certified=True,
                   generation=generation, no_new_generation_observed=True, other_instances_modified=False)
except BaseException as error:
    receipt['error'] = type(error).__name__ + ': ' + str(error)
finally:
    receipt['commands'] = rows
    (OUT / 'receipt.json').write_text(json.dumps(receipt, indent=2, sort_keys=True) + '\n')
    print(json.dumps({k:v for k,v in receipt.items() if k != 'commands'}), flush=True)
raise SystemExit(0 if receipt.get('targeted_shutdown_certified') else 1)
