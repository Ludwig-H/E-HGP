#!/usr/bin/env python3
"""Guarded host tooling selftest: all metadata and subprocess calls are mocked."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
from unittest import mock


SOURCE = Path(__file__).with_name('g4_prepare_host.py')
spec = importlib.util.spec_from_file_location('host_tools_under_test', SOURCE)
host = importlib.util.module_from_spec(spec)
spec.loader.exec_module(host)
IDENTITY = {'project/project-id': 'devpod-gpu-exploration',
            'instance/name': 'ehgp-v7-3b1d496aed430749ea7e049f',
            'instance/zone': 'projects/123/zones/us-central1-c',
            'instance/machine-type': 'projects/123/machineTypes/g4-standard-48'}
TOOLS = ('g++', 'cmake', 'ctest', 'make', '/usr/bin/time')
checks = 0


def need(condition, message):
    global checks
    if not condition:
        raise ValueError(message)
    checks += 1


def facts(missing=(), failed=()):
    return {key: dict(path=None) if key in missing else
            dict(path='/mock/'+key, exit_code=int(key in failed), version='mock version') for key in TOOLS}


def metadata_guards():
    class Response:
        headers = {'Metadata-Flavor': 'Google'}
        def __enter__(self):
            return self
        def __exit__(self, *_args):
            return False
        def read(self, size):
            need(size == 4096, 'bounded metadata read')
            return b'expected\n'
    response = Response()
    def open_request(request, timeout):
        need(request.full_url == 'http://169.254.169.254/computeMetadata/v1/instance/name', 'exact metadata endpoint')
        need(request.get_header('Metadata-flavor') == 'Google' and timeout == 5, 'metadata request guard')
        return response
    def opener(*handlers):
        need(len(handlers) == 2 and isinstance(handlers[0], host.urllib.request.ProxyHandler)
             and handlers[0].proxies == {}, 'environment proxies disabled')
        need(isinstance(handlers[1], host.NoRedirect), 'redirect guard installed')
        return SimpleNamespace(open=open_request)
    with mock.patch.dict('os.environ', {'HTTP_PROXY': 'http://unusable.invalid:1'}), \
         mock.patch.object(host.urllib.request, 'build_opener', side_effect=opener):
        need(host.metadata('instance/name') == 'expected', 'metadata value')
        response.headers = {}
        try:
            host.metadata('instance/name')
        except ValueError:
            need(True, 'missing metadata flavour refused')
        else:
            raise ValueError('missing metadata flavour accepted')
    for target in ('http://example.invalid/', 'http://169.254.169.254/elsewhere'):
        try:
            host.NoRedirect().redirect_request(None, None, 302, '', {}, target)
        except host.urllib.error.URLError:
            need(True, 'redirect refused')
        else:
            raise ValueError('metadata redirect accepted')


def scenario(*, identity=None, remaining=1000, mode='poweroff', guest_code=0,
             missing=(), after_missing=(), after_failed=(), install=True, apt_code=0,
             apt_timeout=False, metadata_error=False):
    calls, inventory_calls = [], []
    def metadata(key):
        if metadata_error:
            raise OSError('mock metadata inaccessible')
        return (identity or IDENTITY)[key]
    def inventory():
        inventory_calls.append(None)
        return facts(missing) if len(inventory_calls) == 1 else facts(after_missing, after_failed)
    def command(argv, **kwargs):
        calls.append(argv)
        if argv[:3] == ['sudo', '-n', 'cat']:
            return SimpleNamespace(returncode=guest_code,
                                   stdout='USEC=%d\nMODE=%s\n' % ((10000+remaining)*1000000, mode))
        need(argv[:4] == ['sudo', '-n', 'env', 'DEBIAN_FRONTEND=noninteractive'], 'unexpected command')
        need('apt-get' in argv and kwargs.get('timeout') in (180, 360), 'apt command bounds')
        if apt_timeout:
            raise subprocess.TimeoutExpired(argv, kwargs['timeout'])
        return SimpleNamespace(returncode=apt_code)
    with tempfile.TemporaryDirectory(prefix='mhgp11-host-tools-test-') as directory:
        args = SimpleNamespace(out=Path(directory), zone='us-central1-c', instance=IDENTITY['instance/name'],
                               install_missing=install)
        with mock.patch.object(host, 'metadata', side_effect=metadata), \
             mock.patch.object(host, 'inventory', side_effect=inventory), \
             mock.patch.object(host.subprocess, 'run', side_effect=command), \
             mock.patch.object(host.time, 'time', return_value=10000):
            code = host.run(args)
        report = json.loads((args.out/'host_tools.json').read_text())
        need(report['product_executed'] is False, 'product execution announced')
        need(code == int(report['status'] != 'ready'), 'exit/status disagreement')
        return report, calls, inventory_calls


def main():
    metadata_guards()
    need(host.required_packages(facts()) == [], 'already provisioned')
    need(host.required_packages(facts(('cmake', 'ctest'))) == ['cmake'], 'cmake package deduplication')
    need(host.required_packages(facts(TOOLS)) == ['cmake', 'g++', 'make', 'time'], 'minimal package set')
    for key in IDENTITY:
        wrong = copy.deepcopy(IDENTITY); wrong[key] = 'wrong'
        report, calls, inventories = scenario(identity=wrong)
        need(report['status'] == 'failed' and not calls and not inventories, 'identity before all guest commands')
    for values in (dict(remaining=840), dict(remaining=2701), dict(mode='reboot'), dict(guest_code=1)):
        report, calls, inventories = scenario(**values)
        need(report['status'] == 'failed' and len(calls) == 1 and not inventories, 'guest cutoff before inventory/apt')
    report, calls, _ = scenario(metadata_error=True)
    need(report['status'] == 'failed' and not calls, 'metadata failure closed')
    for remaining in (841, 2700):
        report, calls, _ = scenario(remaining=remaining)
        need(report['status'] == 'ready' and len(calls) == 1 and report['packages'] == [], 'valid cutoff no apt')
    report, calls, _ = scenario(missing=('g++',), install=False)
    need(report['status'] == 'failed' and len(calls) == 1, 'installation opt-in required')
    report, calls, _ = scenario(missing=('g++', 'ctest'))
    need(report['status'] == 'ready' and len(calls) == 3, 'guarded update and install')
    need(calls[-1][-2:] == ['cmake', 'g++'] and '--no-install-recommends' in calls[-1], 'only missing packages')
    need(all(row['state'] == 'returned' and row['exit_code'] == 0 for row in report['commands']), 'completed commands retained')
    for values in (dict(apt_code=7), dict(apt_timeout=True)):
        report, calls, _ = scenario(missing=('g++',), **values)
        need(report['status'] == 'failed' and len(calls) == 2 and len(report['commands']) == 1, 'failed update not followed by install')
        if values.get('apt_timeout'):
            row = report['commands'][0]
            need(row['state'] == 'timed_out' and 'exit_code' not in row and
                 row['descendant_shutdown'] == 'delegated_to_guarded_worker_and_target_stop; not_certified_by_this_script',
                 'timeout does not certify apt descendants stopped')
    for values in (dict(after_missing=('g++',)), dict(after_failed=('ctest',))):
        report, _, _ = scenario(missing=('g++',), **values)
        need(report['status'] == 'failed', 'post-install inventory required')
    # No process creation is allowed accidentally if the tested implementation changes its launch primitive.
    need(host.subprocess.run is subprocess.run, 'mock restoration')
    print('g4_host_tools_verdict conforme checks%d native0 cloud0' % checks)


if __name__ == '__main__':
    with mock.patch.object(host.subprocess, 'Popen', side_effect=RuntimeError('unmocked process forbidden')):
        main()
