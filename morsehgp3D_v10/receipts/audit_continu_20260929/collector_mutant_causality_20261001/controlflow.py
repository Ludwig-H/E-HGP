"""One bounded replay of the real collector main; build, judges and open are RAM substitutes."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import sys
from unittest.mock import patch

CASES = {'judge_1': 1, 'signal_11': -11, 'timeout': 'delai_1500s', 'unknown_id': 0}


def run(case):
    spec = importlib.util.spec_from_file_location('collector_snapshot', Path(__file__).parent /
                                                  'sources/mutants_entrees_cli.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # The real MA1 replacement, with one match, supplies the virtual source fixture.
    fixture = next(row for row in module.MUTANTS if row[0] == 'MA1')[3][0][1]
    rows, stdout = io.StringIO(), io.StringIO()
    count = [0]
    selected = 'NO_SUCH_ID' if case == 'unknown_id' else 'MA1'

    def virtual_open(path, mode='r', **kwargs):
        return rows if mode == 'a' else io.StringIO(fixture if mode == 'r' else '')

    def virtual_judges(src, build, with_fast, env):
        count[0] += 1
        names = ['unit', 'fault', 'produit', 'temoin', 'fils'] + (['fast'] if with_fast else [])
        values = {name: (0, '', 0) for name in names}
        if count[0] == 2 and selected == 'MA1':
            values['produit'] = (CASES[case], '', 0)
        return values

    with patch.object(module, 'open', virtual_open, create=True), \
            patch.object(module, 'build_copy', return_value=(0, '', 0)), \
            patch.object(module, 'judges', side_effect=virtual_judges), \
            patch.object(sys, 'argv', ['collector', '/tmp/virtual-src', '/tmp/virtual-build',
                                      'virtual.jsonl', selected]), contextlib.redirect_stdout(stdout):
        result = module.main()
    return {'case': case, 'optimized': bool(sys.flags.optimize), 'returncode': result,
            'judges_calls': count[0], 'events': [json.loads(line) for line in rows.getvalue().splitlines()],
            'collector_stdout': stdout.getvalue(), 'native_build_calls': 0, 'native_judge_calls': 0,
            'collector_filesystem_writes': 0}


if __name__ == '__main__':
    if len(sys.argv) != 2 or sys.argv[1] not in CASES:
        raise SystemExit(2)
    print(json.dumps(run(sys.argv[1]), sort_keys=True))
