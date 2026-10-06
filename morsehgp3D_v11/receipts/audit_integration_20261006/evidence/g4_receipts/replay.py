#!/usr/bin/env python3
"""Closed local receipts Kruskal/J2memo; stdlib metadata read only, no native/cloud actions."""
import argparse,hashlib,io,json,re,subprocess,tarfile,shlex
from collections import Counter
from datetime import datetime,timezone
from pathlib import Path
RESULT = re.compile(r'^\s*\d+/\d+ Test\s+#\d+:\s+(\S+)\s+\.+\s*(Passed|\*\*\*[^\n]+?)\s+(\d+(?:\.\d+)?) sec\s*$', re.M)

def need(ok, detail):
    if not ok:
        raise RuntimeError(detail)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def sha_file(path):
    out = hashlib.sha256()
    with path.open('rb') as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b''):
            out.update(block)
    return out.hexdigest()

def read_member(archive, path, maximum=16 * 1024 * 1024):
    member = archive.getmember(path)
    need(member.isfile() and 0 <= member.size <= maximum, 'membre non borne : ' + path)
    return archive.extractfile(member).read()

def kv(data):
    return dict(line.split('=', 1) for line in data.decode().splitlines() if '=' in line)

def verdicts(data):
    entries = RESULT.findall(data.decode())
    out = {name: dict(test=name, state=state, seconds=seconds) for name, state, seconds in entries}
    need(len(entries) == len(out), 'verdicts termines dupliques')
    return out

def source_tree(package, repo, pin):
    """Une archive Git selective ; egalite exacte de tous les fichiers utiles du paquet."""
    prefixes = ('morsehgp3D_v11/src/', 'morsehgp3D_v11/include/', 'morsehgp3D_v11/tests/',
                'morsehgp3D_v11/cmake/', 'morsehgp3D_v11/bench/', 'morsehgp3D_v11/reference/',
                'morsehgp3D_v11/tools/', 'morsehgp3D_v11/cli/', 'morsehgp3D_v11/python/')
    # Le controleur et les gardes s'executent localement ; seul le worker est livre au paquet.
    exact = {'morsehgp3D_v11/CMakeLists.txt', 'gcp-migration/v11_worker.sh'}
    tracked = subprocess.check_output(['git', '-C', str(repo), 'ls-tree', '-r', '--name-only', pin],
                                      text=True).splitlines()
    selected = {name for name in tracked if name in exact or name.startswith(prefixes)}
    # Un repertoire absent (include/, python/, etc.) n'est pas transmis comme pathspec inexistant.
    paths = sorted(exact & selected) + [prefix.rstrip('/') for prefix in prefixes
                                      if any(name.startswith(prefix) for name in selected)]
    git_tar = subprocess.check_output(['git', '-C', str(repo), 'archive', '--format=tar', pin, '--', *paths])
    manifest = {}
    with tarfile.open(fileobj=io.BytesIO(git_tar), mode='r:') as expected, tarfile.open(package, 'r:gz') as actual:
        wanted = {entry.name for entry in expected.getmembers() if entry.isfile()}
        need(wanted == selected, 'source Git selective incomplete ou non reguliere')
        # L'archive inclut egalement ses entrees de repertoires.
        actual_files = {entry.name for entry in actual.getmembers() if entry.isfile() and
                        (entry.name in exact or entry.name.startswith(prefixes))}
        need(actual_files == wanted, 'sources utiles ajoutees/manquantes dans le paquet')
        for name in sorted(wanted):
            original = read_member(expected, name)
            delivered = read_member(actual, name)
            need(original == delivered, 'source du paquet/Git divergent : ' + name)
            manifest[name] = sha(original)
    manifest_bytes = ''.join(digest + '  ' + name + '\n' for name, digest in sorted(manifest.items())).encode()
    return dict(method='git_archive_selectif_egalite_exacte', source_commit=pin,
                files_verified=len(manifest), manifest_sha256=sha(manifest_bytes),
                paths=paths, extra_or_missing_files=False,
                includes_docs_or_receipts=False)


CANONICAL = {
    5: {'lidar_ng00': '3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe',
        'lidar_ng01': '5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091',
        'lidar_ng02': '78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207'},
    10: {'lidar_ng00': '61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295',
         'lidar_ng01': '838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de',
         'lidar_ng02': '81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e'}}
SESSIONS = [
    ('kruskal', 'v11.20261006.claudesupkr', 'supports_kruskal', '07428324eaa5cbf26446362bceadd77722c1a5b7', 15, 'ctest.txt'),
    ('j2memo', 'v11.20261006.claudej2memo', 'j2memo_rangs_locaux', '34a8a561d0c6c7f346608c49ac9113e3830b216f', 14, 'supports_v2_ctest.txt')]


def git_file(repo, pin, path):
    return subprocess.check_output(['git', '-C', str(repo), 'show', pin + ':' + path])


def gpu_report(raw, expected_k):
    doc = json.loads(raw)
    need(doc['verdict'] == 'conforme' and doc['refusals'] == [], 'GPU refusals/verdict')
    need(doc['identity'] == CANONICAL[expected_k], 'six canonical reference hashes')
    need(doc['modes'] == {'cpu': '16379', 'gpu': '81915'} and int(doc['workers']) == 48, 'GPU modes/workers')
    need((doc['kmax'], doc['leaf'], doc['warm_passes'], doc['reps']) ==
         ((5, 16, 12, 5) if expected_k == 5 else (10, 24, 8, 3)), 'GPU case specification')
    counter = Counter((r['frame'], r['mode']) for r in doc['cold'])
    warm_counter = Counter((r['frame'], r['mode']) for r in doc['warm'])
    expected = {(frame, mode) for frame in CANONICAL[expected_k] for mode in doc['modes']}
    need(set(counter) == set(warm_counter) == expected and all(n == doc['reps'] for n in counter.values())
         and all(n == 1 for n in warm_counter.values()), 'GPU inventory')
    for row in doc['cold'] + doc['warm']:
        need(row['code'] == 0 and row['summary']['status'] == 'ok' and not row['stderr'], 'GPU process status')
        need(row['dump_sha256'] == CANONICAL[expected_k][row['frame']], 'retained canonical hash')
    for row in doc['warm']:
        passes = row['passes']
        need([p['pass'] for p in passes] == list(range(1, doc['warm_passes'] + 1)) and
             all(p['status'] == 'ok' for p in passes), 'warm pass inventory/status')
    ledger_bytes = json.dumps(doc['ledger'], sort_keys=True, separators=(',', ':')).encode()
    return dict(k=expected_k, leaf=doc['leaf'], workers=48, modes=doc['modes'], report_sha256=sha(raw),
        verdict=doc['verdict'], refusals=0, cold_processes=len(doc['cold']), warm_processes=len(doc['warm']),
        constructed_full_passes=len(doc['cold']) + len(doc['warm'])*doc['warm_passes'],
        retained_canonical_hashes=len(doc['cold']) + len(doc['warm']),
        intermediate_passes_without_dump_or_ledger=len(doc['warm'])*(doc['warm_passes']-1),
        bench_sha256=doc['bench_sha256'], canonical=doc['identity'], ledger_reference_sha256=sha(ledger_bytes),
        ledger_equality='Compared by pinned gpu_ab judge; refusals empty. Individual native work lines not retained.',
        scope=doc['scope'])


def read_session(repo, session_root, publication, spec):
    label, name, folder, pin, expected_tests, ctest_name = spec
    session = session_root / name
    receipt_raw = (session / 'receipt.json').read_bytes()
    receipt = json.loads(receipt_raw)
    need((session / 'DONE').read_text().strip() == '0' and receipt['status'] == 'completed' and receipt['worker_exit_code'] == 0,
         'session not completed')
    need(receipt['source_kind'] == 'commit' and receipt['evidence_grade'] == 'pushed_commit' and
         receipt['commit'] == pin and receipt['worker_source'] == 'commit:' + pin, 'source identity')
    need(receipt['closure'] == 'stopped' and receipt['targeted_shutdown_certified'] and receipt['start_certified'] and
         receipt['observed_after']['status'] == 'TERMINATED' and receipt['observed_after']['name'] == receipt['target']['instance'] and
         receipt['generation'] == receipt['closing_generation'] == receipt['observed_after']['lastStartTimestamp'], 'certified targeted closure')
    need(receipt['private_key_deleted'] and receipt['oslogin_key_removed'] and receipt['reserve_released'], 'closure cleanup')
    need(receipt['results_verified'] and not receipt['results_skipped_members'] and not receipt['overflow']['evicted'] and
         not receipt['overflow']['truncated_streams'] and receipt['data_verified_remote'], 'incomplete results/data')
    archive_path = session / 'results/results.tar.gz'
    package = session / 'package/package.tar.gz'
    need(sha_file(archive_path) == receipt['results_sha256'] and sha_file(package) == receipt['package_sha256'], 'archive/package hash')
    plan_raw = (session / 'package/plan.json').read_bytes()
    need(sha(plan_raw) == receipt['plan_sha256'], 'plan hash')
    plan = json.loads(plan_raw)
    base = 'morsehgp3D_v11/receipts/developpement_20261006/' + folder + '/'
    published = {}
    for line in git_file(repo, publication, base + 'SHA256SUMS').decode().splitlines():
        digest, filename = line.split(None, 1)
        filename = filename.strip()
        raw = git_file(repo, publication, base + filename)
        need(sha(raw) == digest, 'published checksum ' + filename)
        published[filename] = raw
    need(published['receipt.json'] == receipt_raw and published['plan.json'] == plan_raw and
         published['launch.json'] == (session/'launch.json').read_bytes(), 'published/session pieces differ')
    proof = source_tree(package, repo, pin)
    commands = []
    reports = []
    tests = []
    with tarfile.open(archive_path, 'r:gz') as archive:
        files = {m.name for m in archive.getmembers() if m.isfile()}
        listed = set()
        manifest_raw = read_member(archive, 'results/MANIFEST.sha256')
        for line in manifest_raw.decode().splitlines():
            digest, path = line.split(None, 1)
            path = 'results/' + path.strip().removeprefix('./')
            need(path not in listed and sha(read_member(archive, path)) == digest, 'result manifest hash')
            listed.add(path)
        need(files == listed | {'results/MANIFEST.sha256'}, 'archive inventory not fully covered')
        configure_argv = shlex.split(read_member(archive, 'results/build/configure/argv.txt').decode())
        options = [word for word in configure_argv if word.startswith('-D')]
        need('-DCMAKE_BUILD_TYPE=Release' in options and not any('MHGP11_COORD_BITS' in w or 'SANITIZE=ON' in w or 'TSAN=ON' in w for w in options),
             'profile/configure options')
        cmake_source = git_file(repo, pin, 'morsehgp3D_v11/CMakeLists.txt').decode()
        need('set(MHGP11_COORD_BITS 21 CACHE STRING' in cmake_source, 'default coordinate profile')
        for index, cmd in enumerate(plan['commands']):
            prefix = 'results/cmd/%03d_%s/' % (index, cmd['name'])
            metadata = kv(read_member(archive, prefix+'meta.txt'))
            need(metadata['status'] == 'ok' and metadata['exit_code'] == '0', 'command failure')
            entry = dict(name=cmd['name'], status='ok', exit_code=0, metadata_sha256=sha(read_member(archive,prefix+'meta.txt')))
            if cmd['argv'][0] == 'ctest':
                need('-E' in cmd['argv'] and cmd['argv'][cmd['argv'].index('-E')+1] == '_opt$', 'normal-only selection')
                raw = read_member(archive, prefix+'stdout')
                completed = verdicts(raw)
                starts = re.findall(r'^\s*Start\s+\d+:\s+(\S+)\s*$', raw.decode(), re.M)
                total = re.search(r'100% tests passed, 0 tests failed out of (\d+)', raw.decode())
                need(len(starts) == len(set(starts)) == expected_tests and set(starts) == set(completed) and
                     total and int(total.group(1)) == expected_tests and all(v['state']=='Passed' for v in completed.values()), 'CTest inventory/verdicts')
                excerpt = verdicts(published[ctest_name])
                need(excerpt == completed, 'published CTest excerpt differs')
                tests = sorted(completed)
                need(not any(n.endswith('_opt') for n in tests), 'unexpected optimized Python gate')
                entry.update(selected=expected_tests, passed=expected_tests, failed=0, missing=0, tests=tests,
                             stdout_sha256=sha(raw), published_excerpt_sha256=sha(published[ctest_name]))
            else:
                k = 5 if cmd['name'] == 'gpu_k5' else 10
                path = prefix + 'files/' + cmd['name'] + '/gpu_ab_report.json'
                raw = read_member(archive,path)
                need(raw == published['gpu_ab_report_k%d.json'%k], 'published GPU report differs')
                report = gpu_report(raw,k)
                reports.append(report)
                entry.update(kind='gpu_ab', report_sha256=sha(raw))
            commands.append(entry)
        result_manifest = dict(files=len(listed), sha256=sha(manifest_raw), exact_inventory=True)
    source_compare = {}
    if label == 'kruskal':
        need('mhgp11_cli_supports_spanning_reader' in tests, 'spanning reader not played')
    else:
        need('mhgp11_cli_supports_spanning_reader' not in tests, 'old role selection unexpectedly qualifies new reader')
        need(len(reports)==2 and reports[0]['bench_sha256']==reports[1]['bench_sha256'], 'GPU build reuse')
        judge = git_file(repo,pin,'morsehgp3D_v11/bench/gpu_ab.py').decode()
        anchors = {
            'catalogue_work_extraction': "work = got.get('domain', {}).get('catalogue_work')",
            'catalogue_work_equality': "work is not None and work == report['ledger'].get(frame)",
            'canonical_dump_equality': 'digest is not None and digest == reference',
            'warm_pass_count': "[p.get('pass') for p in passes_seen] == list(range(1, passes + 1))",
            'last_warm_scope': 'les passes 1..P-1 ne serialisent rien',
            'dump_removed_after_hash': 'dump.unlink()'}
        need(all(value in judge for value in anchors.values()), 'GPU judge evidence/scope anchors')
        source_compare.update(judge_sha256=sha(judge.encode()), judge_path='morsehgp3D_v11/bench/gpu_ab.py',
            evidence_lines={label: (max if label == 'dump_removed_after_hash' else min)(
                i for i, line in enumerate(judge.splitlines(), 1) if anchor in line)
                for label, anchor in anchors.items()})
    return dict(label=label, session=name, source_pin=pin, status='completed', done=0, worker_exit_code=0,
        targeted_stop_certified=True, closing_generation=receipt['closing_generation'],
        stop_utc=datetime.fromisoformat(receipt['observed_after']['lastStopTimestamp']).astimezone(timezone.utc).isoformat(),
        receipt_sha256=sha(receipt_raw), archive_sha256=receipt['results_sha256'], package_sha256=receipt['package_sha256'],
        plan_sha256=receipt['plan_sha256'], source_equality=proof, published_files_verified=len(published),
        result_manifest=result_manifest, configure_options=options, profile='Release u21',
        numeric_profile_evidence='Pinned CMake default21, no configure override; normal support oracle/reader gates passed.',
        commands=commands, gpu_reports=reports, source_compare=source_compare,
        exclusions=['Python _opt', 'mutants', 'product ASan/UBSan', 'product TSan', 'profiles u18/u24'],
        supports_scope='Kruskal SPv2 final source' if label=='kruskal' else 'SPv2 role filter only; not final Kruskal selection')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path('/workspaces/E-HGP'))
    parser.add_argument('--sessions',type=Path,default=Path('/workspaces/.ehgp-sessions'))
    parser.add_argument('--publication',default='d8c015dbd5059fc06d18c0991923b51c26b18561')
    parser.add_argument('--capture',action='store_true')
    args=parser.parse_args()
    sessions=[read_session(args.repo,args.sessions,args.publication,spec) for spec in SESSIONS]
    gpu=sessions[1]['gpu_reports']
    totals={key:sum(row[key] for row in gpu) for key in ('constructed_full_passes','retained_canonical_hashes','intermediate_passes_without_dump_or_ledger')}
    totals['lidar_processes']=sum(row['cold_processes']+row['warm_processes'] for row in gpu)
    result=dict(schema='audit_g4_kruskal_j2memo_metadata_v1',publication_commit=args.publication,sessions=sessions,j2memo_totals=totals,
        native_runs=0,cloud_actions=0,limits=['Canonical hashes, no raw FULL artifacts retained or reread.',
        'Complete catalog work equality is the pinned judge result, not individual native work lines archived.',
        'Final Kruskal15 applies only to074/u21/normal Python; J2memo14 was the earlier role filter.',
        'No100ms FULL or points/flat output contract acquired; executor gain is scoped to the declared LiDAR calls.',
        'Receipt qualification is not transferred to later chrono instrumentation or current origin/main source changes.'])
    text=json.dumps(result,sort_keys=True,indent=2)+'\n'
    target=Path(__file__).with_name('summary.json')
    if args.capture:target.write_text(text)
    else:need(target.read_text()==text,'summary changed')
    print(text,end='')
