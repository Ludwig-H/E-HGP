#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--repo', type=Path, required=True)
    args = parser.parse_args()
    cap = json.loads((HERE / 'capture.json').read_text())
    def git_object(commit, path):
        return subprocess.run(['git', 'show', commit + ':' + path], cwd=args.repo,
                              capture_output=True, check=True).stdout
    old = git_object(cap['historical_receipt_commit'], cap['historical_capture_path'])
    if hashlib.sha256(old).hexdigest() != cap['historical_capture_sha256']:
        raise ValueError('capture historique differente')
    pilot = git_object(cap['pilot_commit'], cap['pilot_path'])
    if hashlib.sha256(pilot).hexdigest() != cap['pilot_sha256'] or json.loads(old)['pilot_sha256'] != cap['pilot_sha256']:
        raise ValueError('pilote different')
    print(json.dumps(cap['result'], indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
