#!/usr/bin/env python3
"""Validation locale du plan head par le controleur fige ; aucun cloud/natif."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import types

HERE = Path(__file__).resolve().parent
REPO = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path('/workspaces/E-HGP')


def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)


metadata = json.loads((HERE / 'head_plan_check.json').read_bytes())
need(hashlib.sha256((HERE / 'head_tests.cmake').read_bytes()).hexdigest() ==
     metadata['head_ctest_declaration_sha256'], 'declarations CMake capturees')
plan_bytes = (HERE / 'head_differential_plan.json').read_bytes()
need(hashlib.sha256(plan_bytes).hexdigest() == metadata['head_plan_sha256'], 'plan SHA')
source = subprocess.check_output(['git', '-C', str(REPO), 'show',
                                 metadata['controller_pin'] + ':gcp-migration/v11_session.py'])
need(hashlib.sha256(source).hexdigest() == metadata['controller_sha256'], 'controleur SHA')
module = types.ModuleType('audit_v11_session')
module.__file__ = str(REPO / 'gcp-migration/v11_session.py')
exec(compile(source, module.__file__, 'exec'), module.__dict__)
tracked = set(subprocess.check_output(['git', '-C', str(REPO), 'ls-tree', '-r', '--name-only',
                                      metadata['controller_pin']], text=True).splitlines())
checked = module.validate_plan(json.loads(plan_bytes), tracked, set())
need(checked['python_packages'] == 'pinned' and checked['default_build'] and
     checked['build_targets'] == ['mhgp11_cli'], 'construction/dependances')
need([c['argv'][c['argv'].index('-R') + 1] for c in checked['commands']] == metadata['commands'], 'selection')
need(all('-E' not in c['argv'] and '-LE' not in c['argv'] for c in checked['commands']), 'aucune exclusion')
need(len(checked['commands']) == 4 and sum(c['timeout_seconds'] for c in checked['commands']) == 16800,
     'quatre delais distincts')
print(json.dumps(metadata, sort_keys=True, indent=2))
