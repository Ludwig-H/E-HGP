"""Pure controls of the failure reader; no native executable or cloud call."""
import copy
import importlib.util
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('adaptive2_reader', HERE/'check.py')
c = importlib.util.module_from_spec(spec); spec.loader.exec_module(c)
positives = rejected = 0


def good(action):
    global positives
    action(); positives += 1


def bad(action):
    global rejected
    try: action()
    except (c.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError, OSError):
        rejected += 1
        return
    raise RuntimeError('corruption accepted')


def encode(value): return json.dumps(value, sort_keys=True, allow_nan=False).encode()


def main():
    receipt, worker, data = c.old.read_capture(HERE); pins = c.source_contract()
    good(c.check)
    declarations = {r['name']: r for r in pins['matrix']['configurations']}
    configs = {r['name']: r for r in c.js(data[c.BASE+'summary.json'])['configurations']}

    def config(name, edit=lambda _r: None, alter=lambda _d, _p: None):
        value, items = copy.deepcopy(configs[name]), data.copy()
        edit(value); prefix = c.BASE+name+'/'
        items[prefix+'result.json'] = encode(value); alter(items, prefix)
        return c.build_failure(value, items, declarations[name])

    good(lambda: config('gcc_release'))
    good(lambda: config('mutants'))
    for key, value in [('status', 'ok'), ('conforming', True), ('failures', []), ('not_run', [{'test': 'missing'}])]:
        bad(lambda key=key, value=value: config('gcc_release', lambda r: r.__setitem__(key, value)))
    for step, key, value in [(1, 'exit_code', 0), (3, 'exit_code', 0), (1, 'status', 'ok'),
                             (1, 'timed_out', True)]:
        bad(lambda step=step, key=key, value=value: config('gcc_release',
            lambda r: r['steps'][step].__setitem__(key, value)))
    bad(lambda: config('gcc_release', lambda r: r['tests'].__setitem__('failed', 0)))
    bad(lambda: config('gcc_release', lambda r: r['failures'].append(r['failures'][0])))

    def replacement(path, before, after):
        def change(items, prefix):
            blob = items[prefix+path]; changed = blob.replace(before, after)
            if blob == changed: raise RuntimeError('mutation did not apply')
            items[prefix+path] = changed
        return change

    for before, after in [(b'memo_fault.cpp:31:5: error:', b'other.cpp:31:5: error:'),
                          (b'[-Werror=misleading-indentation]', b'[-Werror=other]'),
                          (b'const auto preserved=times;', b'const auto other=times;')]:
        bad(lambda before=before, after=after: config('gcc_release',
            alter=replacement('build.log', before, after)))
    bad(lambda: config('gcc_release', alter=lambda d, p: d.__setitem__(p+'build.log', d[p+'build.log']+b'error: other\n')))
    for before, after in [(b'run_expect_verdict lancement_impossible', b'run_expect_verdict code'),
                          (b'code 127 du shell', b'code 1 du shell'),
                          (b'not found', b'not found\nmhgp11_test_ok')]:
        bad(lambda before=before, after=after: config('gcc_release',
            alter=replacement('junit.xml', before, after)))
    for before, after in [(b'TEMOIN ROUGE module=tower : aucun mutant juge', b'TUE all'),
                          (b'temoin [] : construction en echec', b'temoin vert'),
                          (b'run_expect_verdict code', b'run_expect_verdict conforme')]:
        bad(lambda before=before, after=after: config('mutants',
            alter=replacement('junit.xml', before, after)))

    def matrix(path, edit):
        changed = data.copy(); value = c.js(changed[path]); edit(value); changed[path] = encode(value)
        return c.matrices(changed, pins)

    good(lambda: c.matrices(data, pins))
    for key, value in [('conforming', True), ('exit_code', 0), ('complete', False), ('signals', [15])]:
        bad(lambda key=key, value=value: matrix(c.BASE+'summary.json', lambda r: r.__setitem__(key, value)))
    bad(lambda: matrix(c.BASE+'summary.json', lambda r: r['configurations'].pop()))
    bad(lambda: matrix(c.SUPP+'summary.json', lambda r: r.__setitem__('conforming', True)))
    bad(lambda: matrix(c.SUPP+'summary.json', lambda r: r['configurations'][0]['tests'].__setitem__('passed', 178)))
    good(lambda: c.commands(receipt, worker, data, pins))
    for path, value in [('results/cmd/002_adaptive/stdout', b'conforme\n'),
                        ('results/cmd/002_adaptive/stderr', b'error'),
                        ('results/cmd/002_adaptive/files/adaptive.json.gz', b'unexpected')]:
        changed = data.copy(); changed[path] = value
        bad(lambda changed=changed: c.commands(receipt, worker, changed, pins))
    for key, value in [('commands_ok', '3'), ('status', 'completed'), ('interrupted', '1')]:
        changed_worker = dict(worker, **{key: value})
        bad(lambda changed_worker=changed_worker: c.commands(receipt, changed_worker, data, pins))
    for before, after in [(b'group_closed=1', b'group_closed=0'),
                          (b'residual_group_killed=0', b'residual_group_killed=1'),
                          (b'streams_truncated=0', b'streams_truncated=1')]:
        changed = data.copy(); p = 'results/cmd/002_adaptive/meta.txt'
        changed[p] = changed[p].replace(before, after)
        if changed[p] == data[p]: raise RuntimeError('meta mutation did not apply')
        bad(lambda changed=changed: c.commands(receipt, worker, changed, pins))
    print(json.dumps(dict(positives=positives, corruptions_rejected=rejected, native_calls=0), sort_keys=True))


if __name__ == '__main__': main()
