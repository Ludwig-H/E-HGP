"""Pure reader tests on the pinned LIVE capture; no product, native executable or cloud invocation."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import re
import shutil
import tempfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('region_reader_under_test', HERE/'check.py')
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)
positive = negative = 0


def yes(condition, label):
    global positive
    if not condition:
        raise RuntimeError('positive_' + label)
    positive += 1


def reject(call, label):
    global negative
    try:
        call()
    except c.ERRORS:
        negative += 1
    else:
        raise RuntimeError('accepted_' + label)


def encoded(value):
    return json.dumps(value, separators=(',', ':')).encode()


def main():
    source = HERE/'region1'
    receipt, worker, data = c.old.read_capture(source)
    pins = c.contract()
    counts, builds, code = c.matrices(data, pins)
    yes(code == 0 and sum(v[1] for v in counts.values()) == 1398, 'matrix')
    mapped = {c.BASE+p[len(c.q4.SUPPLEMENT):]: v for p, v in data.items() if p.startswith(c.q4.SUPPLEMENT)}
    extra, code = c.supplement(mapped, pins)
    yes(code == 0 and extra == (73, 73, 0, 0), 'asan18')
    manifest, manifest_hash = c.old.inputs(source, receipt)
    report = c.js((source/'profiles.json').read_bytes())
    hashes = {n: c.sha(data[c.BASE+n+'/build_provenance.json']) for n in c.profiles.PROFILES.values()}
    args = (manifest, manifest_hash, c.sha(data[c.BASE+'summary.json']), builds, hashes,
            c.sha(data[c.q4.SUPPLEMENT+'summary.json']))
    result = c.report_judge(report, *args)
    yes(result['conforming'] is False and result['attempted'] == 29 and result['equal'] == 4, 'partial')

    def changed(label, edit):
        value = copy.deepcopy(report)
        edit(value)
        reject(lambda: c.report_judge(value, *args), label)

    changed('complete', lambda v: v.update(complete=True))
    changed('conforming', lambda v: v.update(conforming=True))
    changed('duplicate', lambda v: v['runs'].insert(0, copy.deepcopy(v['runs'][0])))
    changed('missing_middle', lambda v: v['runs'].pop(2))
    changed('lost_old_omission', lambda v: v['not_run'].pop(0))
    changed('foreign_omission', lambda v: v['not_run'][0].update(coord_bits=18))
    changed('timeout_greened', lambda v: v['runs'][1].update(status='ok'))
    changed('false_comparison', lambda v: v['comparisons'][0].update(status='incomplete'))
    changed('different_digest', lambda v: v['runs'][0]['semantic'].update(sha256='0'*64))
    changed('q4_count', lambda v: v['runs'][0]['qmin_counts'].update({'4': 0}))
    changed('q4_comparison', lambda v: v['q4_comparisons'][0].update(status='incomplete'))
    changed('stderr_success', lambda v: v['runs'][0].update(stderr='diagnostic'))
    changed('nonfinite', lambda v: v['runs'][0].update(process_wall_seconds=float('nan')))
    changed('raw_hash', lambda v: v['runs'][0].update(canonical_sha256='short'))
    changed('supplement_hash', lambda v: v.update(supplement_sha256='0'*64))
    changed('binary_hash', lambda v: v['builds'][0].update(sha256='0'*64))
    changed('argv_profile', lambda v: v['runs'][0]['argv'].__setitem__(5, '32'))
    changed('wrong_bits', lambda v: v['runs'][0].update(coord_bits=24))
    changed('population_count', lambda v: v['runs'][0]['semantic'].update(incidences=0))

    for key, amount in [('region_pair_tests', -1), ('region_line_tests', 2**64),
                        ('region_pair_rejects', 2**63), ('region_line_rejects', 2**63)]:
        def edit(value, key=key, amount=amount):
            row = value['runs'][0]
            row['events'][1]['logical'][key] = amount
            row['stdout'] = ''.join(json.dumps(e)+'\n' for e in row['events'])
        changed(key, edit)

    divergent = copy.deepcopy(report)
    divergent['runs'][0]['semantic']['sha256'] = '0'*64
    divergent['comparisons'] = c.profiles.compare(divergent['runs'])
    result = c.report_judge(divergent, *args)
    yes(result['different'] == 1 and result['conforming'] is False, 'divergence_preserved')

    summary = c.js(data[c.BASE+'summary.json'])
    config = next(x for x in summary['configurations'] if x['name'] == 'gcc_release')
    path = c.BASE+'gcc_release/tests.json'
    selected = c.js(data[path])
    selected[0]['name'] += '_unregistered'
    bad = dict(data, **{path: encoded(selected)})
    reject(lambda: c.selection(config, bad, pins), 'selection_pin')
    path = c.BASE+'gcc_release/junit.xml'
    root = c.old.foundation.ET.fromstring(data[path])
    node = next(root.iter('testcase'))
    c.old.foundation.ET.SubElement(node, 'skipped')
    bad = dict(data, **{path: c.old.foundation.ET.tostring(root)})
    reject(lambda: c.matrices(bad, pins), 'skipped_green')
    bad = dict(data, **{c.BASE+'summary.json': encoded(dict(summary, exit_code=1))})
    reject(lambda: c.matrices(bad, pins), 'matrix_exit')
    bad = dict(data)
    prefix = c.BASE+'mutants/'
    regex = r'(region_pair_contact_lost\s+TUE\s+)code'
    for name in ('LastTest.log', 'junit.xml'):
        raw, replaced = re.subn(regex, r'\1signal', bad[prefix+name].decode())
        if replaced != 1:
            raise RuntimeError('fixture_mutant')
        bad[prefix+name] = raw.encode()
    mutants = next(x for x in summary['configurations'] if x['name'] == 'mutants')
    reject(lambda: c.mutants(mutants, bad, pins), 'signal_is_not_causal_kill')

    meta = c.fields(data[c.q4.BENCH+'meta.txt'])
    c.command_metadata(meta, 820)
    yes(meta['status'] == 'timeout', 'external_timeout')
    for key, value in [('exit_code', '0'), ('wall_seconds', 'NaN'), ('group_closed', '0'),
                       ('streams_truncated', '1'), ('status', 'invented'), ('wall_seconds', '819')]:
        reject(lambda k=key, v=value: c.command_metadata(dict(meta, **{k: v}), 820), 'command_'+key)

    with tempfile.TemporaryDirectory(prefix='mhgp11-region-reader-') as directory:
        folder = Path(directory)/'region1'
        shutil.copytree(source, folder)
        with contextlib.redirect_stdout(io.StringIO()):
            yes(c.check(folder) is True, 'live_failed_capture')
        for key, value in [('closure', 'running'), ('reserve_released', False), ('commit', '0'*40)]:
            compact = copy.deepcopy(receipt)
            compact[key] = value
            (folder/'receipt.json').write_bytes(encoded(compact))
            reject(lambda: c.check(folder), 'compact_'+key)
        (folder/'receipt.json').write_bytes((source/'receipt.json').read_bytes())
        (folder/'results.tar.gz').write_bytes((source/'results.tar.gz').read_bytes()[:-1])
        reject(lambda: c.check(folder), 'archive_truncated')
    print(json.dumps(dict(positives=positive, corruptions=negative, native=0, cloud=0,
                         verdict='conforme'), sort_keys=True))


if __name__ == '__main__':
    main()
