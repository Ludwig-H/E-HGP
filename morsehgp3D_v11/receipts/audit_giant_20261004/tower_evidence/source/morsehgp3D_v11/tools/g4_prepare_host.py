#!/usr/bin/env python3
"""Install missing C++ build tools only inside a guarded G4 session; no product build or benchmark."""
import argparse
import datetime
import json
from pathlib import Path
import shutil
import subprocess
import time
import urllib.error
import urllib.request


def need(condition, message):
    if not condition:
        raise ValueError(message)


class NoRedirect(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, _request, _fp, _code, _message, _headers, _new_url):
        raise urllib.error.URLError('metadata redirect refused')


def metadata(key):
    request = urllib.request.Request('http://169.254.169.254/computeMetadata/v1/'+key,
                                     headers={'Metadata-Flavor': 'Google'})
    opener = urllib.request.build_opener(urllib.request.ProxyHandler({}), NoRedirect())
    with opener.open(request, timeout=5) as response:
        need(response.headers.get('Metadata-Flavor') == 'Google', 'GCE metadata response required')
        return response.read(4096).decode().strip()


def inventory():
    result = {}
    for command in ('g++', 'cmake', 'ctest', 'make', '/usr/bin/time'):
        path = shutil.which(command)
        value = dict(path=path)
        if path:
            run = subprocess.run([path, '--version'], capture_output=True, text=True, timeout=10, check=False)
            value.update(exit_code=run.returncode, version=(run.stdout+run.stderr)[:2000])
        result[command] = value
    return result


def required_packages(facts):
    mapping = {'g++': 'g++', 'cmake': 'cmake', 'ctest': 'cmake', 'make': 'make', '/usr/bin/time': 'time'}
    return sorted({package for tool, package in mapping.items() if not facts[tool]['path']})


def run(args):
    args.out.mkdir(parents=True, exist_ok=True)
    report = dict(schema='ehgp.v11.host_tools.v1', status='preflight', commands=[], product_executed=False)
    path = args.out/'host_tools.json'

    def save():
        temporary = path.with_suffix('.tmp'); temporary.write_text(json.dumps(report, indent=2)+'\n')
        temporary.replace(path)

    def command(argv, limit):
        row = dict(argv=argv, timeout_seconds=limit, state='intent', started_utc=datetime.datetime.now(
            datetime.timezone.utc).isoformat()); report['commands'].append(row); save()
        ordinal = len(report['commands'])-1
        with (args.out/('%02d.stdout'%ordinal)).open('wb') as out, (args.out/('%02d.stderr'%ordinal)).open('wb') as err:
            try:
                completed = subprocess.run(argv, stdout=out, stderr=err, timeout=limit, check=False)
            except subprocess.TimeoutExpired:
                row.update(state='timed_out', descendant_shutdown='delegated_to_guarded_worker_and_target_stop; '
                           'not_certified_by_this_script'); save()
                raise
        row.update(state='returned', exit_code=completed.returncode); save()
        need(completed.returncode == 0, 'toolchain command failed')

    save()
    try:
        identity = {key: metadata(key) for key in ('project/project-id', 'instance/name', 'instance/zone',
                                                  'instance/machine-type')}
        report['identity'] = identity; save()
        need(identity['project/project-id'] == 'devpod-gpu-exploration' and identity['instance/name'] == args.instance
             and identity['instance/zone'].rsplit('/',1)[-1] == args.zone
             and identity['instance/machine-type'].rsplit('/',1)[-1] == 'g4-standard-48', 'exact G4 target required')
        guest = subprocess.run(['sudo','-n','cat','/run/systemd/shutdown/scheduled'], capture_output=True,
                               text=True, timeout=15, check=False)
        need(guest.returncode == 0, 'guest cutoff unreadable')
        fields = dict(line.split('=',1) for line in guest.stdout.splitlines() if '=' in line)
        remaining = int(fields.get('USEC','0'))/1e6-time.time()
        need(fields.get('MODE') == 'poweroff' and 840 < remaining <= 2700, 'active bounded guest cutoff required')
        report['guest_cutoff'] = fields
        report['before'] = inventory(); packages = required_packages(report['before']); report['packages'] = packages
        save()
        if packages:
            need(args.install_missing, 'missing tools; explicit guarded plan installation required')
            command(['sudo','-n','env','DEBIAN_FRONTEND=noninteractive','apt-get',
                     '-o','DPkg::Lock::Timeout=30','update'], 180)
            command(['sudo','-n','env','DEBIAN_FRONTEND=noninteractive','apt-get',
                     '-o','DPkg::Lock::Timeout=30','install','--no-install-recommends','-y',*packages], 360)
        report['after'] = inventory()
        need(not required_packages(report['after']) and all(row.get('exit_code') == 0
             for row in report['after'].values()), 'tools still missing or version command failed')
        report['status'] = 'ready'; save(); return 0
    except (OSError, ValueError, subprocess.SubprocessError) as error:
        report.update(status='failed', error=type(error).__name__+': '+str(error)); save(); return 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--zone', required=True)
    parser.add_argument('--instance', required=True)
    parser.add_argument('--install-missing', action='store_true')
    return run(parser.parse_args())


if __name__ == '__main__':
    raise SystemExit(main())
