#!/usr/bin/env python3
"""Actual worker refusal/receipt path with fake guards and tool observations.

Local temporary files only. No cloud, compiler, key or subprocess is used;
the fake support rejects any command if preflight unexpectedly gets past it.
"""
from contextlib import redirect_stdout
import importlib.util
import io
import json
from pathlib import Path
import sys
import tempfile
import time
from types import SimpleNamespace
from unittest.mock import patch

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('mocked_terminal_preflight',
    Path(__file__).with_name('terminal_batch_worker_v7.py'))
w = importlib.util.module_from_spec(spec)
spec.loader.exec_module(w)


def main():
    cases = []
    for absent in ('nvcc', 'g++', 'nvidia-smi', '/usr/bin/time', 'g++_binding'):
        with tempfile.TemporaryDirectory(prefix='ehgp-tool-preflight-mock-') as temporary:
            base = Path(temporary)
            root, output = base / 'source', base / 'output'
            root.mkdir()
            manifest, mark = base / 'source_manifest.json', base / 'guard_mark'
            manifest.write_text('{}\n')
            mark.write_text('mock=true\n')
            generation = '2026-09-11T00:00:00Z'
            paths = {'nvcc': '/mock/nvcc', 'g++': '/usr/bin/g++', 'nvidia-smi': '/mock/nvidia-smi'}
            if absent in paths:
                paths[absent] = None
            if absent == 'g++_binding':
                paths['g++'] = '/mock/different-g++'
            observation = w.discover_existing_tools(which=paths.get,
                is_file=lambda path: path == '/usr/bin/time' and absent != '/usr/bin/time',
                executable=lambda _path: False, resolve=lambda path: path)
            command_calls = []

            class Worker:
                def __init__(self, *_args):
                    self.commands = []

                def command(self, name, argv):
                    command_calls.append((name, argv))
                    raise RuntimeError('MOCK FORBIDS EVERY COMMAND')

            def save(path, value):
                with path.open('x') as stream:
                    json.dump(value, stream, sort_keys=True)
                    stream.write('\n')

            boot_epoch = time.time() - float(Path('/proc/uptime').read_text().split()[0])
            target = dict(project='mock-project', zone='mock-zone', instance='mock-instance')
            support = SimpleNamespace(save=save, fields=lambda text: dict(mock=text),
                scheduled_text=lambda: 'mock=true\n', metadata=lambda: dict(target, machine='g4-standard-48'),
                guard_values=lambda *_args: dict(work_deadline_epoch=time.time() + 600),
                epoch=lambda _text: boot_epoch, Worker=Worker, SessionDeadline=type('MockDeadline', (Exception,), {}))
            argv = ['mocked-worker', '--source-root=' + str(root), '--output=' + str(output),
                    '--source-manifest=' + str(manifest), '--source-manifest-sha256=' + w.sha(manifest),
                    '--guard-mark=' + str(mark), '--guard-mark-sha256=' + w.sha(mark),
                    '--generation=' + generation, '--session-deadline-epoch=' + str(int(time.time()) + 900)]
            for key, value in target.items():
                argv.append('--' + key + '=' + value)
            printed = io.StringIO()
            with patch.object(sys, 'argv', argv), patch.object(w, 'load_support', return_value=support), \
                    patch.object(w, 'source_map', return_value={}), \
                    patch.object(w.os, 'sched_getaffinity', return_value=set(range(48))), \
                    patch.object(w, 'discover_existing_tools', return_value=observation), redirect_stdout(printed):
                code = w.main()
            receipt = json.loads((output / 'receipt.json').read_text())
            w.need(code == 1 and receipt['status'] == 'failed' and not command_calls and receipt['commands'] == [],
                   'mock preflight must refuse before every command')
            w.need(receipt['tool_discovery'] == observation and receipt['sources_stable'] is True,
                   'actual main persists observation before rejection')
            expected_error = 'ValueError: existing tools required, no installation; missing: ' + absent
            if absent == 'g++_binding':
                expected_error = 'ValueError: strict adapter g++ binding: selected=/mock/different-g++, ' + \
                    'resolved=/mock/different-g++, required=/usr/bin/g++, error=None'
            w.need(receipt['error'] == expected_error, 'precise actual receipt error')
            w.need(json.loads(printed.getvalue())['status'] == 'failed', 'actual main failure stdout')
            cases.append(dict(absent=absent, exit_code=code, error=receipt['error'],
                              tool_discovery=receipt['tool_discovery'], commands=receipt['commands']))
    w.need(len(cases) == 5, 'mocked main refusal nonvacuity')
    print(json.dumps(dict(status='passed', cases=cases, mocked_worker_runs=5, GCP_used=False,
                         actual_commands=0, all_guards_and_metadata_mocked=True,
                         scope='diagnostic_receipt_only_no_lifecycle_qualification'), sort_keys=True))


if __name__ == '__main__':
    main()
