#!/usr/bin/env python3
"""Independently describe one already-closed fixed G4 generation; read-only cloud call."""
import argparse
import json
import os
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gcp-migration'))
import tower_session_v9 as session


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', type=Path, required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    receipt = json.loads((args.host / 'receipt.json').read_text())
    session.need(receipt['targeted_shutdown_certified'] is True and receipt['target'] == session.TARGET,
                 'already-closed fixed target required')
    session.need(not args.output_dir.exists() and not args.output_dir.is_symlink(), 'fresh observation directory')
    args.output_dir.mkdir(mode=0o700)
    args.output_dir.chmod(0o700)
    commands = session.Commands(args.output_dir, os.environ.copy())
    rc, raw, _ = commands.run('after_stop', ['/home/codespace/google-cloud-sdk/bin/gcloud',
        'compute', 'instances', 'describe', session.TARGET['instance'],
        '--project=' + session.TARGET['project'], '--zone=' + session.TARGET['zone'], '--format=json'])
    session.need(rc == 0, 'GCE observation failed')
    value = json.loads(raw)
    session.validate_target(value, 'TERMINATED', receipt['generation'])
    session.need(session.epoch(value['lastStopTimestamp']) >= session.epoch(receipt['generation']), 'stop chronology')
    session.save(args.output_dir / 'after_stop.json', value)
    print(json.dumps(dict(status='TERMINATED', generation=receipt['generation'],
                         last_stop=value['lastStopTimestamp'], state_sha256=session.sha(args.output_dir / 'after_stop.json'))))


if __name__ == '__main__':
    main()
