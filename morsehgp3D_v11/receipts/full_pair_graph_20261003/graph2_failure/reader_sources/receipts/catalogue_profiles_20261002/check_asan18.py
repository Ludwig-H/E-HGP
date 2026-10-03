"""LIVE reader for the separate num-only ASan/UBSan B18 campaign; no performance claim.

Exit0 means coherent evidence, including a failed campaign. Original local receipt remains mandatory.
One worker command, one matrix configuration; no catalogue executable is expected in this build.
"""
import importlib.util
from pathlib import Path, PurePosixPath
import sys
import tarfile

HERE = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('previous_catalogue_reader', HERE.parent / 'catalogue_20261002/check.py')
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)
need, js, sha, fields, BASE, HEX = old.need, old.js, old.sha, old.fields, old.BASE, old.HEX
NAME = 'gcc_asan_ubsan18'
PREFIX = BASE + NAME + '/'
REQUIRED = {'mhgp11_num_unit_power_paths', 'mhgp11_num_fraction', 'mhgp11_num_fraction_opt'}
CACHE = dict(MHGP11_COORD_BITS='18', MHGP11_SANITIZE='ON', MHGP11_TSAN='OFF', MHGP11_POISON='OFF',
             MHGP11_MODULES='num')
BINARIES = {'libmhgp11.a', 'mhgp11_num_probe', 'mhgp11_num_unit'}
TARGETS = {'mhgp11', 'mhgp11_num_probe', 'mhgp11_num_unit'}
BUILD_FILES = {'CMakeCache.txt'} | {'CMakeFiles/%s.dir/%s' % (target, filename)
                                  for target in TARGETS for filename in ('flags.make', 'link.txt')}


def provenance(config, data):
    path = PREFIX + 'build_provenance.json'
    if path not in data:
        need(config['status'] != 'ok', 'provenance_absente')
        return
    value = js(data[path])
    need(value['schema'] == 'ehgp.v11.build_provenance.v1' and
         value['complete'] is (not bool(value['errors'])), 'provenance_statut')
    rows = value['files']
    names = [row['path'] for row in rows]
    need(len(names) == len(set(names)), 'provenance_doublon')
    for row in rows:
        relative = PurePosixPath(row['path'])
        need(not relative.is_absolute() and '..' not in relative.parts and HEX.fullmatch(row['sha256']) and
             type(row['size']) is int and row['size'] >= 0, 'provenance_champs')
        if 'text' in row:
            text = row['text'].encode('utf-8')
            need(len(text) == row['size'] and sha(text) == row['sha256'], 'provenance_texte')
    by_name = {row['path']: row for row in rows}
    if config['status'] == 'ok':
        need(value['complete'] is True and BINARIES | BUILD_FILES <= set(names), 'provenance_incomplete')
    if 'CMakeCache.txt' in by_name:
        cache = by_name['CMakeCache.txt']['text'].splitlines()
        for key, expected in CACHE.items():
            found = [line.split('=', 1)[1] for line in cache if line.startswith(key + ':')]
            need(found == [expected], 'cache_' + key)
    for target in TARGETS:
        path = 'CMakeFiles/%s.dir/flags.make' % target
        if path in by_name:
            need('-fsanitize=address,undefined' in by_name[path]['text'], 'instrumentation_absente')


def required_tests(config, data):
    if 'tests' not in config:
        need(config['status'] != 'ok', 'tests_absents')
        return 0
    root = old.foundation.ET.fromstring(data[PREFIX + 'junit.xml'])
    cases = {case.get('name'): case for case in root.iter('testcase')}
    if config['status'] == 'ok':
        need(REQUIRED <= set(cases), 'portes_requises_absentes')
    passed, fractions = 0, []
    for name in REQUIRED & set(cases):
        case = cases[name]
        if case.get('status') != 'run' or case.find('failure') is not None or case.find('skipped') is not None:
            continue
        lines = (case.findtext('system-out') or '').splitlines()
        need('run_expect_verdict conforme' in lines, 'verdict_porte_absent')
        if name == 'mhgp11_num_unit_power_paths':
            need('test power_paths controles=207 echecs=0 plancher=205' in lines and
                 'mhgp11_test_ok tests=1 controles=207' in lines, 'power_paths_plancher')
        else:
            values = [js(line) for line in lines if line.startswith('{')]
            need(len(values) == 1 and values[0]['bits'] == 18 and values[0]['checks'] == 7526 and
                 values[0]['geometry'] == 504 and values[0]['degeneracies'] == 50 and values[0]['integers'] == 160
                 and HEX.fullmatch(values[0]['input_sha256']), 'Fraction_b18_plancher')
            fractions.append(values[0])
        passed += 1
    need(len(fractions) < 2 or fractions[0] == fractions[1], 'Fraction_normal_opt_different')
    if config['status'] == 'ok':
        need(passed == 3, 'portes_requises_non_passees')
    return passed


def judge(data):
    summary = js(data[BASE + 'summary.json'])
    configs = summary['configurations']
    need(summary['schema'] == 'ehgp.v11.g4_matrix_summary.v1' and summary['complete'] is True and
         summary['requested'] == [NAME] and len(configs) == 1 and configs[0]['name'] == NAME, 'inventaire_matrice')
    config = configs[0]
    need(summary['statuses'] == {NAME: config['status']}, 'statut_resume')
    need({p[len(BASE):].split('/')[0] for p in data if p.startswith(BASE) and '/' in p[len(BASE):]} == {NAME},
         'dossiers_configuration')
    if 'tests' not in config and config['status'] in old.EARLY | {'build_failed'}:
        need(js(data[PREFIX + 'result.json']) == config and config['conforming'] is False, 'echec_precoce')
        counts = (0, 0, 0, 0)
    else:
        counts = old.foundation.judge_config(config, data)
    provenance(config, data)
    passed = required_tests(config, data)
    state = config['status']
    code = 1 if summary.get('signals') or state not in ('ok', 'vacuous', 'incomplete', 'floor_violated') else (
        0 if state == 'ok' else 3)
    need(type(summary['exit_code']) is int and summary['exit_code'] == code and
         summary['conforming'] is (code == 0), 'verdict_matrice')
    return counts, passed, code


def check(folder):
    receipt, worker, data = old.read_capture(folder)
    need(all(receipt[key] is True for key in ('private_key_deleted', 'oslogin_key_removed', 'reserve_released')),
         'nettoyage_session')
    need((folder / 'matrix.json').read_bytes() == data[BASE + 'summary.json'], 'copie_matrice')
    need({p.split('/')[2] for p in data if p.startswith('results/cmd/')} == {'000_matrice'}, 'commande_unique')
    counts, passed, code = judge(data)
    uploaded = receipt.get('data_files', [])
    if any(row['name'] == 'manifest.json' for row in uploaded):
        old.inputs(folder, receipt)
    else:
        need(not (folder / 'inputs.json').exists(), 'entrees_non_upload')
    meta = fields(data['results/cmd/000_matrice/meta.txt'])
    need(meta.get('group_closed') == '1' and int(meta['exit_code']) == code and
         meta['status'] == ('ok' if code == 0 else 'failed'), 'commande_matrice')
    good = code == 0
    need(worker['commands_total'] == '1' and worker['commands_ok'] == ('1' if good else '0') and
         worker['status'] == ('completed' if good else 'failed') and
         receipt['status'] == ('completed' if good else 'failed_remote') and
         receipt['worker_exit_code'] == (0 if good else 1), 'verdict_session')
    print('%s coherence=ok campagne=%s commit=%s B18 ASan/UBSan portes=%d/%d requises=%d/3' %
          (folder.name, 'CONFORME' if good else 'ECHEC', receipt['commit'], counts[1], counts[0], passed))
    return not good


def main():
    folders = [Path(p) for p in sys.argv[1:]] or [HERE / 'asan18']
    failed = sum(check(folder) for folder in folders)
    print('captures_coherentes=%d campagnes_echouees=%d' % (len(folders), failed))


if __name__ == '__main__':
    try:
        main()
    except (old.foundation.Refusal, OSError, ValueError, KeyError, TypeError, IndexError,
            old.foundation.ET.ParseError, tarfile.TarError) as error:
        print('REFUS ' + (str(error) if isinstance(error, old.foundation.Refusal) else type(error).__name__))
        sys.exit(1)
