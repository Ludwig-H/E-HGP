"""Pure receipt-reader controls. No native product, compilation or cloud call."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('adaptive_failure_reader', HERE/'check.py')
c = importlib.util.module_from_spec(spec); spec.loader.exec_module(c)
positive = rejected = 0


def good(action):
    global positive
    action(); positive += 1


def bad(action):
    global rejected
    try: action()
    except (c.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError, OSError):
        rejected += 1
        return
    raise RuntimeError('corruption accepted')


def encode(value):
    return (json.dumps(value, sort_keys=True, allow_nan=False)+'\n').encode()


def mutate_json(data, path, transform):
    value = c.js(data[path]); transform(value); data[path] = encode(value)


def main():
    receipt, worker, data = c.old.read_capture(HERE)
    pins = c.source_contract()
    good(lambda: c.check())
    prefix = c.BASE+'mutants/'
    config = c.js(data[prefix+'result.json'])

    def mutant(change, data_change=lambda _d: None):
        value, items = copy.deepcopy(config), data.copy()
        change(value); items[prefix+'result.json'] = encode(value); data_change(items)
        return c.timeout_mutants(value, items)

    good(lambda: mutant(lambda _c: None))
    for key, value in [('status', 'ok'), ('conforming', True), ('reason', 'all passed'),
                       ('failures', [{'test': 'mhgp11_mutants_tower'}]), ('seconds', 0)]:
        bad(lambda key=key, value=value: mutant(lambda r: r.__setitem__(key, value)))
    for key, value in [('exit_code', 0), ('status', 'ok'), ('timed_out', False),
                       ('seconds', -1), ('timeout_seconds', 600)]:
        bad(lambda key=key, value=value: mutant(lambda r: r['steps'][-1].__setitem__(key, value)))
    bad(lambda: mutant(lambda r: r['tests'].__setitem__('passed', 21)))
    bad(lambda: mutant(lambda r: r['not_run'].clear()))
    bad(lambda: mutant(lambda r: r['passed_labels'].__setitem__('long', 7)))
    bad(lambda: mutant(lambda r: r['steps'].append(r['steps'][0])))
    for edit in (lambda r: r.append(r[0]), lambda r: r.pop(),
                 lambda r: r[0].__setitem__('disabled', True),
                 lambda r: r[0].__setitem__('labels', ['mutant'])):
        bad(lambda edit=edit: mutant(lambda _r: None,
            lambda d: mutate_json(d, prefix+'tests.json', edit)))
    original = data[prefix+'ctest.log']
    mutations = [original.replace(b'Passed', b'Failed', 1),
                 original.replace(b' 1/21', b' 2/21', 1),
                 original.replace(b'Test #453:', b'Test #454:', 1),
                 original.replace(b'      Start 453: mhgp11_mutants_core_manifest\n', b''),
                 original.replace(b'Start 473: mhgp11_mutants_tower', b'Start 473: mhgp11_mutants_num'),
                 original+b'21/21 Test #473: mhgp11_mutants_tower ... Passed 1.00 sec\n',
                 original.splitlines()[0]+b'\n']
    for blob in mutations:
        if blob == original: raise RuntimeError('test mutation did not apply')
        bad(lambda blob=blob: mutant(lambda _r: None,
            lambda d: d.__setitem__(prefix+'ctest.log', blob)))
    bad(lambda: mutant(lambda _r: None, lambda d: d.__setitem__(prefix+'junit.xml', b'<testsuite/>')))

    def matrix(path, edit):
        items = data.copy(); mutate_json(items, path, edit)
        return c.matrices(items, pins)

    good(lambda: c.matrices(data, pins))
    for key, value in [('complete', False), ('conforming', True), ('exit_code', 0),
                       ('signals', [15]), ('requested', ['gcc_release']*9)]:
        bad(lambda key=key, value=value: matrix(c.BASE+'summary.json', lambda r: r.__setitem__(key, value)))
    bad(lambda: matrix(c.BASE+'summary.json', lambda r: r['configurations'].pop()))
    bad(lambda: matrix(c.SUPP+'summary.json', lambda r: r.__setitem__('conforming', False)))
    bad(lambda: matrix(c.SUPP+'summary.json', lambda r: r['configurations'][0]['tests'].__setitem__('passed', 160)))
    proof = c.SUPP+'gcc_asan_ubsan18/build_provenance.json'
    bad(lambda: matrix(proof, lambda r: r.__setitem__('files',
        [v for v in r['files'] if v['path'] != 'mhgp11_tower_forest_probe'])))

    def wrong_bits(value):
        row = next(r for r in value['files'] if r['path'] == 'CMakeCache.txt')
        before = row['text']; row['text'] = before.replace('MHGP11_COORD_BITS:STRING=18', 'MHGP11_COORD_BITS:STRING=24')
        if row['text'] == before: raise RuntimeError('cache mutation did not apply')
        row['size'] = len(row['text'].encode()); row['sha256'] = c.sha(row['text'].encode())

    bad(lambda: matrix(proof, wrong_bits))
    good(lambda: c.commands(receipt, worker, data, pins))
    for path, blob in [('results/cmd/002_adaptive/stdout', b'all successful\n'),
                       ('results/cmd/002_adaptive/stderr', b'error'),
                       ('results/cmd/002_adaptive/files/adaptive.json.gz', b'unexpected')]:
        changed = data.copy(); changed[path] = blob
        bad(lambda changed=changed: c.commands(receipt, worker, changed, pins))
    for before, after in [(b'group_closed=1', b'group_closed=0'),
                          (b'exit_code=2', b'exit_code=0'),
                          (b'streams_truncated=0', b'streams_truncated=1')]:
        changed = data.copy(); key = 'results/cmd/002_adaptive/meta.txt'
        changed[key] = changed[key].replace(before, after)
        if changed[key] == data[key]: raise RuntimeError('meta mutation did not apply')
        bad(lambda changed=changed: c.commands(receipt, worker, changed, pins))
    changed = data.copy(); key = 'results/cmd/002_adaptive/argv.txt'
    changed[key] = changed[key].replace(b'--budget-seconds 700', b'--budget-seconds 701')
    bad(lambda: c.commands(receipt, worker, changed, pins))

    # Matched compact/raw mutations test semantic recovery guards, bypassing the
    # outer hash/copy check intentionally; all temporary raw files are deleted.
    with tempfile.TemporaryDirectory() as temp:
        folder = Path(temp)/'capture'; session = Path(temp)/'session'
        folder.mkdir(); session.mkdir(); (session/'host/logs').mkdir(parents=True)
        original_raw = c.js(Path(receipt['raw_receipt_local']).read_bytes())
        compact = c.js((HERE/'recovery.json').read_bytes())
        recovery_raw = c.js(Path(compact['raw_recovery_local']).read_bytes())
        diagnostic = c.js((HERE/'cleanup_failure.json').read_bytes())
        log_blob = Path(diagnostic['raw_log_local']).read_bytes()

        def recovered(edit=lambda _r, _o, _c: None, log=log_blob, missing=None):
            r, o, rec = copy.deepcopy(recovery_raw), copy.deepcopy(original_raw), copy.deepcopy(receipt)
            r['session'] = str(session); rec['raw_receipt_local'] = str(session/'receipt.json')
            edit(r, o, rec)
            (session/'receipt.json').write_bytes(encode(o))
            small = {k: r[k] for k in compact if k in r}
            if missing is not None: del r[missing]
            raw = encode(r); (session/'recovery.json').write_bytes(raw)
            small.update(raw_recovery_local=str(session/'recovery.json'), original_recovery_sha256=c.sha(raw))
            (folder/'recovery.json').write_bytes(encode(small))
            lp = session/'host/logs/removal.stderr'; lp.write_bytes(log)
            diag = dict(raw_log_local=str(lp), sha256=c.sha(log), bytes=len(log), contains_aborted=True)
            (folder/'cleanup_failure.json').write_bytes(encode(diag))
            return c.recovery(rec, folder)

        good(recovered)
        for key, value in [('oslogin_key_removed', False), ('targeted_shutdown_certified', False),
                           ('status', 'failed'), ('closing_generation', 'different'),
                           ('warnings', ['unclosed']), ('errors', ['failed'])]:
            bad(lambda key=key, value=value: recovered(lambda r, _o, _c: r.__setitem__(key, value)))
        bad(lambda: recovered(lambda r, _o, _c: r['target'].__setitem__('instance', 'other')))
        bad(lambda: recovered(lambda r, _o, _c: r['observed_before_stop'].__setitem__('status', 'RUNNING')))
        bad(lambda: recovered(lambda r, _o, _c: r['host_commands'][-1].__setitem__('exit_code', 1)))
        bad(lambda: recovered(lambda _r, _o, c: c.__setitem__('oslogin_key_removed', True)))
        bad(lambda: recovered(lambda _r, o, _c: o['host_commands'][-1].__setitem__('exit_code', 0)))
        bad(lambda: recovered(log=b'no diagnostic'))
        bad(lambda: recovered(missing='targeted_shutdown_certified'))
        bad(lambda: recovered(missing='oslogin_key_removed'))
    print(json.dumps(dict(positives=positive, corruptions_rejected=rejected, native_calls=0), sort_keys=True))


if __name__ == '__main__': main()
