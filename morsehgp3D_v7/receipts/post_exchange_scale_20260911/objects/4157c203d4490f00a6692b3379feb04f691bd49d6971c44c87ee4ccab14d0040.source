#!/usr/bin/env python3
"""Run one FULL probe, retaining raw output; no operation or time quota.

The caller monitors progress and may interrupt the run. This is a measurement
recorder, not a completeness certificate or a qualification gate.
"""
import argparse
import hashlib
import json
import os
import re
from pathlib import Path
import signal
import subprocess
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + '\n')


class Once(argparse.Action):
    def __call__(self, parser, namespace, value, option_string=None):
        if getattr(namespace, self.dest, None) is not None:
            parser.error('duplicate option: ' + option_string)
        setattr(namespace, self.dest, value)


def positive_decimal(value):
    if not re.fullmatch(r'[0-9]+', value) or int(value) <= 0:
        raise argparse.ArgumentTypeError('expected a positive decimal integer')
    return int(value)


def cpu_list(value):
    if not re.fullmatch(r'[0-9]+(?:-[0-9]+)?(?:,[0-9]+(?:-[0-9]+)?)*', value):
        raise argparse.ArgumentTypeError('expected CPU IDs/ranges, e.g. 0,2-7')
    return value  # Kernel/taskset validates availability; never expand a huge range here.


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--binary', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--n', required=True, type=int)
    parser.add_argument('--s', required=True, type=int)
    parser.add_argument('--p', default='unlimited')
    affinity = parser.add_mutually_exclusive_group()
    affinity.add_argument('--cpu', type=int, default=6)
    affinity.add_argument('--cpu-list', type=cpu_list, action=Once)
    parser.add_argument('--threads', type=positive_decimal, action=Once)
    args = parser.parse_args()
    if args.threads is not None and args.threads > 1 and args.cpu_list is None:
        parser.error('--threads>1 requires an explicit --cpu-list (a singleton permits intentional oversubscription)')
    binary = args.binary.resolve()
    args.output.mkdir(parents=True, exist_ok=False)
    affinity_requested = args.cpu_list if args.cpu_list is not None else str(args.cpu)
    command = ['taskset', '-c', affinity_requested, '/usr/bin/time', '-v', str(binary),
               f'--n={args.n}', f'--s={args.s}', '--kmax=10', '--alias-policy=lazy',
               '--cache-entries=1000000', f'--meb-proposal-supports={args.p}']
    if args.threads is not None:
        command.append(f'--threads={args.threads}')
    record = dict(command=command, binary_sha256=digest(binary),
                  started_utc=time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
                  git_head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
                  status='running', public_status='not_claimed')
    if args.threads is not None or args.cpu_list is not None:
        record.update(pipeline_threads_requested=args.threads, cpu_list_requested=affinity_requested)
    save(args.output / 'run.json', record)
    started = time.monotonic()
    with (args.output / 'stdout.jsonl').open('wb') as out, (args.output / 'stderr.txt').open('wb') as err:
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=out,
                                   stderr=err, start_new_session=True)
        record['pid'] = process.pid
        save(args.output / 'run.json', record)
        print(json.dumps(dict(pid=process.pid, output=str(args.output))), flush=True)
        try:
            record['exit_code'] = process.wait()
            record['status'] = 'completed' if record['exit_code'] == 0 else 'failed'
        except KeyboardInterrupt:
            os.killpg(process.pid, signal.SIGTERM)
            record['exit_code'] = process.wait()
            record['status'] = 'interrupted'
        finally:
            record.update(elapsed_seconds=time.monotonic() - started,
                          binary_sha256_after=digest(binary),
                          stdout_sha256=digest(args.output / 'stdout.jsonl'),
                          stderr_sha256=digest(args.output / 'stderr.txt'))
            save(args.output / 'run.json', record)
    print(json.dumps(record), flush=True)
    return 0 if record['status'] == 'completed' else 1


if __name__ == '__main__':
    raise SystemExit(main())
