#!/usr/bin/env python3
"""Paquet de session de la sonde S1 GPU v9, construit depuis un COMMIT git ; jamais d'appel GCP.

Memes regles que tower_snapshot_v9.py, dont les lectures git (rev-parse,
ls-tree, cat-file), le controle d'identite des objets et l'archive
deterministe sont reutilises tels quels : sources morsehgp3D_v9 (CMake,
cmake/, src/, bench/, tests/, oracle/), helper v7 epingle, trois trames LiDAR
sans sol epinglees, protocole GPU (et les modules v9 qu'il importe) pris au
commit, plan data/session_plan.json (schema mhgp9_gpu_filter_plan_v2, v1 historique lu) et
provenance. Un protocole non committe n'est transporte qu'avec
--allow-uncommitted-protocol (preflight/selftest) et le controleur refuse
alors une vraie session.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath

import gpu_filter_worker_v9 as worker
import tower_snapshot_v9 as git_snapshot

ROOT = git_snapshot.ROOT
need = worker.need


def default_plan():
    def case(scene, k):
        return dict(scene=scene, file=worker.INPUTS[scene]['file'], n=worker.INPUTS[scene]['n'], k=k, s=8,
                    workers=48, repeats=3, repeat=0, tile_cache=False)
    # 08/000000 (scene 00) a K5 d'abord : c'est le cas du seuil S1.
    return dict(schema=worker.PLAN_SCHEMA, cases=[case(scene, k) for scene in ('00', '01', '02') for k in (5, 10)])


def collect(commit, plan_raw=None, allow_uncommitted_protocol=False):
    full, tree = git_snapshot.resolve_commit(commit)
    sources = {name: oid for name, oid in git_snapshot.tree_entries(
        full, [worker.CMAKE_LISTS, *(prefix.rstrip('/') for prefix in worker.SOURCE_PREFIXES)]).items()
        if PurePosixPath(name).name != '.gitkeep'}
    helper = git_snapshot.tree_entries(full, [worker.HELPER])
    inputs = git_snapshot.tree_entries(full, [data['source'] for data in worker.INPUTS.values()])
    protocol = git_snapshot.tree_entries(full, sorted(worker.PROTOCOL_NAMES))
    need(set(helper) == {worker.HELPER} and set(inputs) == {data['source'] for data in worker.INPUTS.values()},
         'pinned helper/input absent at commit')
    blobs = git_snapshot.read_blobs([*sources.values(), *helper.values(), *inputs.values(), *protocol.values()])
    files = {name: blobs[oid] for name, oid in sources.items()}
    files[worker.HELPER] = blobs[helper[worker.HELPER]]
    for data in worker.INPUTS.values():
        files[data['file']] = blobs[inputs[data['source']]]
    executing = {name: (ROOT / name).read_bytes() for name in sorted(worker.PROTOCOL_NAMES)}
    committed = set(protocol) == set(worker.PROTOCOL_NAMES) and all(
        blobs[protocol[name]] == executing[name] for name in worker.PROTOCOL_NAMES)
    need(committed or allow_uncommitted_protocol,
         'GPU protocol files are not committed identically at ' + full + '; commit them first')
    files.update(executing)
    plan = default_plan() if plan_raw is None else worker.strict_json(plan_raw)
    files[worker.PLAN] = worker.canonical_json(plan) if plan_raw is None else plan_raw
    provenance = dict(schema=worker.PROVENANCE_SCHEMA, commit=full, tree=tree,
                      protocol_source='commit' if committed else 'worktree_uncommitted',
                      inputs={data['file']: dict(commit_path=data['source'], sha256=data['sha256'])
                              for data in worker.INPUTS.values()},
                      helper=dict(path=worker.HELPER, sha256=worker.HELPER_SHA))
    files[worker.PROVENANCE] = worker.canonical_json(provenance)
    return files, provenance


def validate_files(files):
    manifest = {name: hashlib.sha256(raw).hexdigest() for name, raw in files.items()}
    worker.validate_manifest(manifest)
    read = files.__getitem__
    worker.validate_sources(read)
    cases = worker.validate_plan(worker.strict_json(read(worker.PLAN)), manifest)
    worker.validate_sources(read, tile_cache=any(case.get('tile_cache', False) for case in cases))
    worker.base.validate_data(read)
    worker.base.validate_provenance(worker.strict_json(read(worker.PROVENANCE)), manifest)
    return manifest, cases


def build(commit, output, plan_path=None, allow_uncommitted_protocol=False):
    import gpu_filter_session_v9 as session
    output = Path(output).absolute()
    need(not output.exists() and not output.is_symlink(), 'fresh output directory')
    plan_raw = Path(plan_path).read_bytes() if plan_path is not None else None
    files, provenance = collect(commit, plan_raw, allow_uncommitted_protocol)
    manifest, cases = validate_files(files)
    output.mkdir(parents=True, mode=0o700)
    record = dict(status='failed', public_status='not_claimed', GCP_used=False, GPU_executed=False,
                  FULL_executed=False, commit=provenance['commit'], tree=provenance['tree'],
                  protocol_source=provenance['protocol_source'], cases=cases,
                  source_files=sum(name.startswith(worker.SOURCE_ROOT + '/') for name in files))
    error = None
    try:
        archive_path = output / 'snapshot.tar.gz'
        git_snapshot.write_archive(archive_path, files)
        worker.save(output / 'source_manifest.json', manifest)
        session.validate_snapshot(archive_path, manifest)
        record.update(status='prepared_not_executed', snapshot_sha256=worker.sha(archive_path),
                      manifest_sha256=worker.sha(output / 'source_manifest.json'),
                      controller_sha256=worker.sha(session.__file__), worker_sha256=worker.sha(worker.__file__),
                      base_worker_sha256=worker.sha(worker.base.__file__),
                      inputs={data['file']: dict(commit_path=data['source'], sha256=data['sha256'], n=data['n'])
                              for data in worker.INPUTS.values()},
                      real_session_allowed=provenance['protocol_source'] == 'commit')
    except BaseException as cause:
        error = cause
        record['error'] = type(cause).__name__ + ': ' + str(cause)
    worker.save(output / 'PACKAGE.json', record)
    if error is not None:
        raise error
    return record


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--commit', required=True, help='commit dont les objets git forment le paquet')
    parser.add_argument('--plan', type=Path, help='plan mhgp9_gpu_filter_plan_v2 (v1 historique lu) ; defaut : sans cache')
    parser.add_argument('--output', type=Path, required=True, help='repertoire neuf')
    parser.add_argument('--allow-uncommitted-protocol', action='store_true',
                        help='preflight/selftest seulement ; le controleur refuse ce paquet avec --execute')
    args = parser.parse_args(argv)
    print(json.dumps(build(args.commit, args.output, args.plan, args.allow_uncommitted_protocol),
                     sort_keys=True, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
