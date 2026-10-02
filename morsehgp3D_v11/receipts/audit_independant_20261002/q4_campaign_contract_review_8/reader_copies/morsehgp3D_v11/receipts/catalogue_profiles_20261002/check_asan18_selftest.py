"""Synthetic single-configuration receipt tests. No native/cloud execution or historical receipt mutation."""
import copy
import json
import xml.etree.ElementTree as ET

import check_asan18 as reader


def encode(value):
    return json.dumps(value, sort_keys=True).encode()


def fixture(failed=False, omitted=None):
    names = sorted(reader.REQUIRED - ({omitted} if omitted else set())) + ['synthetic_control']
    tests = [dict(name=name, disabled=False, labels=['unit', 'fast']) for name in names]
    count = len(tests)
    root = ET.Element('testsuite', tests=str(count), failures=str(int(failed)), disabled='0', skipped='0')
    for name in names:
        bad = failed and name == 'synthetic_control'
        case = ET.SubElement(root, 'testcase', name=name, status='fail' if bad else 'run')
        if bad:
            ET.SubElement(case, 'failure').text = 'synthetic failure'
        if name == 'mhgp11_num_unit_power_paths':
            output = 'test power_paths controles=207 echecs=0 plancher=205\nmhgp11_test_ok tests=1 controles=207\n'
        elif name.startswith('mhgp11_num_fraction'):
            output = json.dumps(dict(bits=18, checks=7526, geometry=504, degeneracies=50, integers=160,
                                     input_sha256='a' * 64)) + '\n'
        else:
            output = ''
        ET.SubElement(case, 'system-out').text = output + 'run_expect_verdict conforme\n'
    config = dict(name=reader.NAME, status='failed' if failed else 'ok', conforming=not failed,
                  tests=dict(selected=count, passed=count - int(failed), failed=int(failed), not_run=0,
                             ctest_total=count, ctest_failed=int(failed)),
                  passed_labels=dict(unit=count - int(failed), fast=count - int(failed)), steps=[])
    for name in ('configure', 'build', 'list', 'test'):
        bad = failed and name == 'test'
        config['steps'].append(dict(name=name, status='failed' if bad else 'ok', exit_code=int(bad)))
    summary = dict(schema='ehgp.v11.g4_matrix_summary.v1', complete=True, requested=[reader.NAME],
                   configurations=[config], statuses={reader.NAME: config['status']}, signals=[],
                   exit_code=int(failed), conforming=not failed)
    files = []
    for name in sorted(reader.BINARIES | reader.BUILD_FILES):
        if name == 'CMakeCache.txt':
            text = ''.join('%s:STRING=%s\n' % (key, value) for key, value in reader.CACHE.items())
        elif name.endswith('flags.make'):
            text = 'CXX_FLAGS = -fsanitize=address,undefined\n'
        elif name.endswith('link.txt'):
            text = 'synthetic link command\n'
        else:
            text = None
        payload = (text or 'synthetic binary').encode()
        row = dict(path=name, sha256=reader.sha(payload), size=len(payload))
        if text is not None:
            row['text'] = text
        files.append(row)
    provenance = dict(schema='ehgp.v11.build_provenance.v1', complete=True, errors=[], files=files)
    return {reader.BASE + 'summary.json': encode(summary), reader.PREFIX + 'result.json': encode(config),
            reader.PREFIX + 'tests.json': encode(tests), reader.PREFIX + 'junit.xml': ET.tostring(root),
            reader.PREFIX + 'build_provenance.json': encode(provenance)}


def mutate_json(data, path, operation):
    value = json.loads(data[path])
    operation(value)
    data[path] = encode(value)


def cache_value(data, key, value):
    def change(provenance):
        row = next(r for r in provenance['files'] if r['path'] == 'CMakeCache.txt')
        row['text'] = row['text'].replace(key + ':STRING=' + reader.CACHE[key], key + ':STRING=' + value)
        row.update(size=len(row['text'].encode()), sha256=reader.sha(row['text'].encode()))
    mutate_json(data, reader.PREFIX + 'build_provenance.json', change)


def output_value(data, name, old, new):
    root = ET.fromstring(data[reader.PREFIX + 'junit.xml'])
    case = next(c for c in root if c.get('name') == name)
    output = case.find('system-out')
    output.text = output.text.replace(old, new)
    data[reader.PREFIX + 'junit.xml'] = ET.tostring(root)


def main():
    positive, failed = fixture(), fixture(True)
    if reader.judge(positive) != ((4, 4, 0, 0), 3, 0) or reader.judge(failed) != ((4, 3, 1, 0), 3, 1):
        raise ValueError('positive/failed campaign witness')
    cases = []

    def corrupted(name, change):
        data = copy.deepcopy(positive)
        change(data)
        cases.append((name, data))

    for key, value in [('MHGP11_COORD_BITS', '24'), ('MHGP11_SANITIZE', 'OFF'), ('MHGP11_TSAN', 'ON'),
                       ('MHGP11_POISON', 'ON'), ('MHGP11_MODULES', 'all')]:
        corrupted(key, lambda data, key=key, value=value: cache_value(data, key, value))
    for name in sorted(reader.REQUIRED):
        cases.append(('missing_' + name, fixture(omitted=name)))
    corrupted('missing_unit_executable', lambda data: mutate_json(data, reader.PREFIX + 'build_provenance.json',
              lambda p: p.update(files=[r for r in p['files'] if r['path'] != 'mhgp11_num_unit'])))
    corrupted('missing_flags', lambda data: mutate_json(data, reader.PREFIX + 'build_provenance.json',
              lambda p: p.update(files=[r for r in p['files'] if not r['path'].endswith('flags.make')])))
    corrupted('wrong_provenance_hash', lambda data: mutate_json(data, reader.PREFIX + 'build_provenance.json',
              lambda p: next(r for r in p['files'] if 'text' in r).update(sha256='0' * 64)))
    corrupted('power_path_count', lambda data: output_value(data, 'mhgp11_num_unit_power_paths', '207', '206'))
    corrupted('Fraction_profile', lambda data: output_value(data, 'mhgp11_num_fraction', '"bits": 18', '"bits": 24'))
    corrupted('Fraction_count', lambda data: output_value(data, 'mhgp11_num_fraction_opt', '7526', '7525'))
    corrupted('Fraction_input_disagreement', lambda data: output_value(data, 'mhgp11_num_fraction_opt', 'a' * 64, 'b' * 64))
    corrupted('extra_configuration', lambda data: mutate_json(data, reader.BASE + 'summary.json',
              lambda s: s['configurations'].append(s['configurations'][0])))
    data = copy.deepcopy(failed)
    mutate_json(data, reader.BASE + 'summary.json', lambda s: s.update(exit_code=0, conforming=True))
    cases.append(('failed_green', data))
    corrupted('unexpected_configuration_directory', lambda data: data.update({reader.BASE + 'other/result.json': b'{}'}))
    for name, data in cases:
        try:
            reader.judge(data)
        except (reader.old.foundation.Refusal, ValueError, KeyError, TypeError, IndexError):
            pass
        else:
            raise ValueError(name + ' accepted')
    if len(cases) != 18:
        raise ValueError('corruption floor')
    print(json.dumps(dict(synthetic_positive_fixtures=2, corruptions_refused=len(cases), native_calls=0,
                         cases=[name for name, _ in cases]), sort_keys=True))


if __name__ == '__main__':
    main()
