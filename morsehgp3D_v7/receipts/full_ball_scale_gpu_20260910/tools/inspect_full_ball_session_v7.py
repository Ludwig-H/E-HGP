#!/usr/bin/env python3
"""Read progress from an already guarded FULL session; no start/stop/benchmark.

Only the exact target and SSH identity recorded by the guarded controller are
accepted. This diagnostic does not acquire a VM and does not discharge ROOT's
obligation to stop and certify the session it already owns.
"""
import argparse
from datetime import datetime
import hashlib
import json
import math
from pathlib import Path, PurePosixPath
import re
import shlex
import stat
import subprocess
import time

TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
              instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
GCLOUD = '/home/codespace/google-cloud-sdk/bin/gcloud'
READ = '''import json,sys
from pathlib import Path
p=Path(sys.argv[1])
if p.is_symlink() or not p.is_dir(): raise ValueError("output directory unavailable")
out={"files":{},"stages":{},"summaries":{},"summary_read_errors":{}}
for f in sorted(p.iterdir()):
 if not f.is_file() or f.is_symlink(): continue
 if f.name.endswith((".stdout",".stderr",".summary.json")):
  out["files"][f.name]=f.stat().st_size
 if f.name.endswith(".stderr"):
  with f.open("rb") as s:
   s.seek(max(0,f.stat().st_size-8192)); tail=s.read().decode("utf-8",errors="replace")
  out["stages"][f.name]=[line for line in tail.splitlines() if line.startswith(("stage_","exception=","{\\"status\\""))][-4:]
 if f.name.endswith(".summary.json") and f.stat().st_size<32768:
  try: out["summaries"][f.name]=json.loads(f.read_text())
  except (OSError,ValueError) as e: out["summary_read_errors"][f.name]=type(e).__name__
print(json.dumps(out,sort_keys=True))
'''


def need(ok, reason):
    if not ok:
        raise ValueError(reason)


def timestamp(value):
    need(type(value) is str and re.fullmatch(
        r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\d(?:\.\d{1,6})?(?:Z|[+-]\d\d:\d\d)', value),
        'RFC3339 timestamp')
    return datetime.fromisoformat(value.replace('Z', '+00:00')).timestamp()


def validate_controller_records(session, session_mode, handoff, intent):
    """Pure validation: no filesystem lookup, subprocess or GCP request."""
    need(isinstance(session, Path) and session.is_absolute() and '..' not in session.parts and
         len(session.parts) > 1, 'absolute session path')
    need(type(session_mode) is int and stat.S_ISDIR(session_mode) and
         stat.S_IMODE(session_mode) == 0o700, 'private session')
    need(type(handoff) is dict and all(handoff.get(k) == v for k, v in TARGET.items()),
         'exact controller target')
    need(handoff.get('schema') == 'e-hgp.start-handoff.v3' and
         handoff.get('status') == 'targeted_running', 'guarded running handoff')
    generation = handoff.get('last_start_timestamp')
    generation_epoch = timestamp(generation)
    need(type(intent) is dict, 'worker intent object')
    argv = intent.get('argv')
    need(type(argv) is list and len(argv) == 13 and all(type(value) is str for value in argv),
         'recorded SSH argv shape')
    need(argv[:7] == [GCLOUD, 'compute', 'ssh', TARGET['instance'],
                     '--project=' + TARGET['project'], '--zone=' + TARGET['zone'], '--quiet'],
         'recorded SSH target')
    need(argv[7] == '--ssh-key-file=' + str(session / 'session_key') and
         argv[8].startswith('--ssh-key-expiration=') and argv[9:12] ==
         ['--ssh-flag=-n', '--ssh-flag=-o BatchMode=yes', '--ssh-flag=-o ConnectTimeout=15'] and
         argv[-1].startswith('--command=exec '), 'recorded guarded SSH identity')
    expiration = argv[8].partition('=')[2]
    need(timestamp(expiration) > generation_epoch, 'SSH expiration after generation')
    remote = shlex.split(argv[-1][len('--command=exec '):])
    if remote and remote[-1] == '--bootstrap':
        remote = remote[:-1]
    keys = {'--source-root', '--source-manifest', '--source-manifest-sha256', '--guard-mark',
            '--guard-mark-sha256', '--generation', '--session-deadline-epoch',
            '--closing-margin-seconds', '--output', '--project', '--zone', '--instance'}
    need(len(remote) == 2 + 2 * len(keys) and remote[0] == 'python3', 'recorded worker argv shape')
    options = dict(zip(remote[2::2], remote[3::2]))
    need(set(options) == keys, 'recorded worker options')
    output = options['--output']
    need(re.fullmatch(r'/tmp/ehgp-full-v7-[0-9a-f]{16}\.[A-Za-z0-9]{10}/output', output) is not None,
         'recorded output directory')
    directory = PurePosixPath(output).parent
    need(remote[1] == str(directory / 'worker.py') and all(options[option] == str(directory / name)
         for option, name in (('--source-root', 'source'), ('--source-manifest', 'source_manifest.json'),
                              ('--guard-mark', 'double_guard_verified'))), 'same remote session paths')
    need(all(options['--' + key] == value for key, value in TARGET.items()), 'recorded worker target')
    need(options['--generation'] == generation, 'recorded worker generation')
    need(all(re.fullmatch(r'[0-9a-f]{64}', options[key]) is not None for key in
             ('--source-manifest-sha256', '--guard-mark-sha256')), 'recorded worker pins')
    deadline = float(options['--session-deadline-epoch'])
    need(math.isfinite(deadline) and deadline > generation_epoch and
         options['--closing-margin-seconds'] == '300', 'recorded worker deadline')
    return dict(argv=argv.copy(), output=output, generation=generation, ssh_expiration=expiration)


def validate_running_generation(state, generation):
    """Pure check of the read-only describe response against the handoff."""
    timestamp(generation)
    need(type(state) is dict and state.get('status') == 'RUNNING' and
         type(state.get('labels')) is dict and state['labels'].get('project') == 'e-hgp' and
         state.get('lastStartTimestamp') == generation, 'same running guarded generation')


def summary_lines(receipt, destination):
    """Bound stdout; the unabridged JSON observation stays in destination."""
    progress = receipt['progress']
    need(type(progress) is dict and all(type(progress.get(key)) is dict for key in
         ('files', 'stages', 'summaries')), 'progress object')
    lines = [f"Read-only {TARGET['instance']} generation={receipt['generation']}",
             f"Files={len(progress['files'])} summaries={len(progress['summaries'])} "
             f"summary_read_errors={len(progress.get('summary_read_errors', {}))}"]
    active = [(name, stages) for name, stages in sorted(progress['stages'].items())
              if type(stages) is list and stages]
    for name, stages in active[-3:]:
        # json.dumps escapes terminal control characters from observed logs.
        lines.append(json.dumps(name)[:96] + ': ' + json.dumps(stages[-1], ensure_ascii=True)[:240])
    lines.append('Complete receipt: ' + str(destination))
    return lines


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session-dir', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    session = args.session_dir.resolve()
    destination = args.output.absolute()
    need(not destination.exists() and not destination.is_symlink() and destination.parent.is_dir(),
         'fresh local receipt path')
    session_mode = session.stat().st_mode
    host = session / 'full_host'
    handoff = json.loads((host / 'handoff.json').read_text())
    intent_path = host / 'worker.intent.json'
    intent = json.loads(intent_path.read_text())
    plan = validate_controller_records(session, session_mode, handoff, intent)
    argv = plan['argv']
    describe = subprocess.run([GCLOUD, 'compute', 'instances', 'describe', TARGET['instance'],
        '--project=' + TARGET['project'], '--zone=' + TARGET['zone'],
        '--format=json(status,lastStartTimestamp,labels)'], capture_output=True, text=True, timeout=45, check=True)
    state = json.loads(describe.stdout)
    validate_running_generation(state, plan['generation'])
    command = argv[:-1] + ['--command=' + shlex.join(['python3', '-c', READ, plan['output']])]
    started = time.time()
    observed = subprocess.run(command, capture_output=True, text=True, timeout=45, check=True)
    receipt = dict(target=TARGET, generation=state['lastStartTimestamp'], started_epoch=started,
        ended_epoch=time.time(), read_only=True, ownership_unchanged=True,
        controller_worker_intent_sha256=hashlib.sha256(intent_path.read_bytes()).hexdigest(),
        progress=json.loads(observed.stdout))
    with destination.open('x') as stream:
        json.dump(receipt, stream, indent=2, sort_keys=True)
        stream.write('\n')
    print('\n'.join(summary_lines(receipt, destination)))


if __name__ == '__main__':
    main()
