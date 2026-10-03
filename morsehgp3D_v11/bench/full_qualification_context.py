"""Read a closed earlier worker archive, then verify a newly qualified comparator build."""
import hashlib
import os
from pathlib import Path, PurePosixPath
import tarfile

import full_campaign as full

need, base, profiles = full.need, full.base, full.profiles
SCHEMA = 'ehgp.v11.full_prior_qualification.v1'


def capture(archive, expected_sha256, out):
    need(base.digest(archive) == expected_sha256, 'prior qualification archive hash')
    with tarfile.open(archive, 'r:gz') as package:
        members = package.getmembers()
        names = [m.name.rstrip('/') for m in members]
        need(len(names) == len(set(names)) and sum(m.size for m in members) <= 512 * 1024**2,
             'prior archive inventory/size')
        need(all(not PurePosixPath(m.name).is_absolute() and '..' not in PurePosixPath(m.name).parts and
                 (m.isfile() or m.isdir()) for m in members), 'prior archive paths/types')
        files = {m.name.removeprefix('./'): m for m in members if m.isfile()}
        manifest_name = 'results/MANIFEST.sha256'
        manifest = package.extractfile(files[manifest_name]).read().decode()
        expected = {}
        for line in manifest.splitlines():
            digest, name = line.split('  ', 1)
            name = 'results/' + name.removeprefix('./')
            need(name not in expected and full.reuse.sha(digest), 'prior manifest line')
            expected[name] = digest
        need(set(expected) == set(files) - {manifest_name}, 'prior archive exhaustive manifest')
        for name, digest in expected.items():
            h = hashlib.sha256()
            with package.extractfile(files[name]) as stream:
                for chunk in iter(lambda: stream.read(1 << 20), b''):
                    h.update(chunk)
            need(h.hexdigest() == digest, 'prior result payload hash: ' + name)
        worker = package.extractfile(files['results/worker.txt']).read().decode()
        entries = dict(line.split('=', 1) for line in worker.splitlines() if '=' in line)
        need(entries['source'] == os.environ['V11_SOURCE_PIN'] and entries['source'].startswith('commit:') and
             entries['status'] == 'completed' and entries['commands_ok'] == entries['commands_total'] and
             entries['interrupted'] == '0' and entries['overflow_unresolved'] == '0', 'prior worker failed/source changed')
        out.mkdir(parents=True, exist_ok=True)
        copied = {}
        selected = ['results/worker.txt', manifest_name,
                    'results/cmd/000_matrice/files/matrix/summary.json',
                    'results/cmd/000_matrice/files/matrix/bits21/build_provenance.json',
                    'results/cmd/001_asan18/files/matrix/summary.json']
        for ordinal, name in enumerate(selected):
            content = package.extractfile(files[name]).read()
            path = out / ('prior_%d_%s' % (ordinal, Path(name).name))
            path.write_bytes(content)
            copied[name] = dict(path=str(path), sha256=base.digest(path), bytes=len(content))
    qualification = profiles.load(Path(copied[selected[2]]['path']))
    need(qualification['complete'] is True and qualification['conforming'] is True and
         type(qualification['exit_code']) is int and qualification['exit_code'] == 0 and
         not qualification.get('signals'), 'prior complete matrix failed')
    configs = qualification['configurations']
    names = [r['name'] for r in configs]
    need(len(set(names)) == len(names) and all(any(r['name'] == name and r['status'] == 'ok' for r in configs)
         for name in profiles.PROFILES.values()), 'prior three-profile qualification inventory')
    supplement = profiles.checked_supplement(Path(copied[selected[4]]['path']))
    report = dict(schema=SCHEMA, source=entries['source'], archive_path=str(archive), archive_sha256=expected_sha256,
                  manifest_sha256=hashlib.sha256(manifest.encode()).hexdigest(), copied=copied,
                  qualification_path=copied[selected[2]]['path'], supplement_path=copied[selected[4]]['path'],
                  bits21_provenance_path=copied[selected[3]]['path'], supplement_sha256=supplement,
                  scope='earlier closed worker payload verified; VM closure certified separately by controller receipt',
                  qualified_binary_scope='old compilation only; new current binary requires its own targeted gates')
    base.save(out / 'prior_context.json', report)
    return report


def checked_current(args):
    prior = profiles.load(args.prior_context)
    need(prior['schema'] == SCHEMA and prior['source'] == os.environ['V11_SOURCE_PIN'], 'prior source context')
    for row in prior['copied'].values():
        need(base.digest(Path(row['path'])) == row['sha256'], 'prior context payload drift')
    summary = profiles.load(args.qualification)
    need(summary['complete'] is True and summary['conforming'] is True and summary['exit_code'] == 0 and
         not summary.get('signals') and summary['requested'] == ['bits21'] and
         len(summary['configurations']) == 1 and summary['configurations'][0]['name'] == 'bits21' and
         summary['configurations'][0]['status'] == 'ok', 'new bits21 targeted gates incomplete/nonconforming')
    tests = summary['configurations'][0]['tests']
    need(type(tests['selected']) is int and tests['selected'] >= 30 and tests['passed'] == tests['selected'] and
         tests['failed'] == tests['not_run'] == 0, 'targeted gates actually executed')
    inventory_path = args.qualification.parent / 'bits21/tests.json'
    inventory = profiles.load(inventory_path)
    names = [r['name'] for r in inventory]
    need(len(names) == len(set(names)) == tests['selected'] and all(name in names for name in
         ('mhgp11_tower_population_concurrent_lemma', 'mhgp11_tower_population_concurrent_equivalence',
          'mhgp11_tower_population_concurrent_refusals', 'mhgp11_tower_full_bench_io',
          'mhgp11_tower_population_contract_contexts', 'mhgp11_tower_population_contract_each_step',
          'mhgp11_tower_population_contract_owned_admission', 'mhgp11_tower_population_contract_ledger',
          'mhgp11_catalogue_sort_fenv_key_modes', 'mhgp11_catalogue_sort_fenv_permutations',
          'mhgp11_catalogue_sort_fault_allocations')),
         'targeted native population/concurrent and IO gates missing')
    path = args.qualification.parent / 'bits21/build_provenance.json'
    provenance = profiles.load(path)
    need(provenance['complete'] is True and not provenance['errors'], 'new bits21 provenance incomplete')
    rows = {r['path']: r for r in provenance['files']}
    need(len(rows) == len(provenance['files']), 'new binary provenance inventory')
    exe = args.builds / 'bits21/build/mhgp11_full_bench'
    binary, cache = rows[exe.name], rows['CMakeCache.txt']
    need(base.digest(exe) == binary['sha256'] and exe.stat().st_size == binary['size'], 'new bits21 binary hash')
    need(hashlib.sha256(cache['text'].encode()).hexdigest() == cache['sha256'], 'new cache provenance')
    old = profiles.load(Path(prior['bits21_provenance_path']))
    first_summary = profiles.load(Path(prior['qualification_path']))
    need(summary['host']['gxx'] == first_summary['host']['gxx'] and
         summary['host']['cmake'] == first_summary['host']['cmake'], 'compiler/CMake version changed between sessions')
    old_cache = next(r['text'] for r in old['files'] if r['path'] == 'CMakeCache.txt')
    # Absolute source/build paths differ. Compiler path, flags, profile and target remain exact.
    keys = ('CMAKE_CXX_COMPILER', 'CMAKE_CXX_FLAGS', 'CMAKE_CXX_FLAGS_RELEASE', 'MHGP11_MARCH', 'MHGP11_COORD_BITS')
    def entries(text):
        return dict((line.split('=', 1)[0].split(':', 1)[0], line.split('=', 1)[1]) for line in text.splitlines()
                    if ':' in line and '=' in line and not line.startswith(('#', '//')))
    before, current = entries(old_cache), entries(cache['text'])
    need(all(current[key] == before[key] for key in keys) and current['MHGP11_COORD_BITS'] == '21',
         'new bits21 profile/compiler flags differ from first qualification')
    return dict(configuration='bits21', coord_bits=21, path=str(exe), sha256=binary['sha256'], bytes=binary['size'],
                cache_sha256=cache['sha256'], provenance_sha256=base.digest(path),
                targeted_qualification_sha256=base.digest(args.qualification),
                targeted_inventory_sha256=base.digest(inventory_path)), prior
