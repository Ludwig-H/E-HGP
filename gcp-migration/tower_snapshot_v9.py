#!/usr/bin/env python3
"""Paquet de session de la tour FULL v9, construit depuis un COMMIT git ; jamais d'appel GCP.

Port explicite de q34_spatial_snapshot_v8.py (SHA 73764da1d14c94ec...,
commit 70de84f2). La v8 lisait le worktree et une autorite native ; la v9
lit tout depuis les objets git du commit donne (git ls-tree + git cat-file,
equivalent de git show), jamais depuis un worktree sale :

- morsehgp3D_v9/CMakeLists.txt, cmake/, src/, bench/, tests/, oracle/
  (le CMake enregistre les portes : leurs sources doivent exister a la
  configuration) ; docs/, audits/ et receipts/ ne sont pas transportes ;
- le helper v7 epingle gcp-migration/full_probe_worker_v7.py ;
- les trois trames LiDAR sans sol a 1 mm, copiees sous data/scene_XX.u32le
  depuis morsehgp3D_v8/receipts/lidar_ground_20260921/... au meme commit,
  controlees contre leurs sha256, tailles et empreintes epinglees ;
- les quatre fichiers du protocole v9, pris au commit. S'ils n'y sont pas
  encore (ou different de ceux qui s'executent), le paquet est refuse, sauf
  --allow-uncommitted-protocol (preflight/selftest seulement) : la copie
  executee est alors transportee, data/provenance.json le dit, et le
  controleur refuse ce paquet pour une vraie session (--execute).

Ecrit snapshot.tar.gz (tar gzip deterministe : membres tries, mode 0444,
mtime 0), source_manifest.json (chemin -> sha256) et PACKAGE.json.
Le plan data/session_plan.json (schema mhgp9_tower_plan_v1) est le plan
par defaut ou un fichier --plan valide. Aucun GPU, aucune execution FULL.
"""
import argparse
import gzip
import hashlib
import io
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess
import tarfile
import tower_session_v9 as session
import tower_worker_v9 as worker

ROOT = Path(__file__).resolve().parent.parent
need = worker.need


def git(*args, stdin=None):
    env = dict(os.environ, GIT_OPTIONAL_LOCKS='0', GIT_NO_REPLACE_OBJECTS='1')
    # Lecture seule : rev-parse, ls-tree, cat-file. Aucune commande mutante.
    completed = subprocess.run(['git', '-C', str(ROOT), *args], input=stdin, capture_output=True, env=env, check=False)
    need(completed.returncode == 0, 'git ' + args[0] + ' failed: ' + completed.stderr.decode(errors='replace').strip())
    return completed.stdout


def resolve_commit(commit):
    need(type(commit) is str and re.fullmatch(r'[A-Za-z0-9._/@^~{}-]{1,200}', commit) and not commit.startswith('-'),
         'commit syntax')
    full = git('rev-parse', '--verify', '--quiet', commit + '^{commit}').decode().strip()
    tree = git('rev-parse', '--verify', '--quiet', full + '^{tree}').decode().strip()
    need(re.fullmatch('[0-9a-f]{40}|[0-9a-f]{64}', full) and re.fullmatch('[0-9a-f]{40}|[0-9a-f]{64}', tree),
         'commit/tree identity')
    return full, tree


def tree_entries(commit, paths):
    """Fichiers reguliers suivis sous `paths` au commit ; lien ou sous-module refuse."""
    out = {}
    for item in git('ls-tree', '-r', '-z', '--full-tree', commit, '--', *paths).split(b'\0'):
        if not item:
            continue
        meta, _, raw_path = item.partition(b'\t')
        mode, kind, oid = meta.decode('ascii').split(' ')
        name = raw_path.decode('utf-8')
        need(kind == 'blob' and mode in ('100644', '100755'), 'regular committed file required: ' + name)
        out[name] = oid
    return out


def read_blobs(oids):
    oids = sorted(set(oids))
    data = git('cat-file', '--batch', stdin=''.join(oid + '\n' for oid in oids).encode('ascii'))
    out, position = {}, 0
    for oid in oids:
        end = data.index(b'\n', position)
        got, kind, size = data[position:end].decode('ascii').split(' ')
        size, start = int(size), end + 1
        content = data[start:start + size]
        need(got == oid and kind == 'blob' and len(content) == size and data[start + size:start + size + 1] == b'\n',
             'git object framing')
        if len(oid) == 40:
            need(hashlib.sha1(b'blob ' + str(size).encode() + b'\0' + content).hexdigest() == oid, 'git blob identity')
        out[oid] = content
        position = start + size + 1
    need(position == len(data), 'git object stream length')
    return out


def default_plan():
    # Voies epinglees : defauts v9 de la chaine (tour statique sur W fils,
    # tous les leviers actifs), passes explicitement a la sonde.
    def case(scene, k, workers):
        return dict(scene=scene, file=worker.INPUTS[scene]['file'], n=worker.INPUTS[scene]['n'], k=k, s=8,
                    workers=workers, static_threads=workers if workers > 1 else 0,
                    levers={name: True for name in worker.LEVER_NAMES}, repeat=0)
    # Ordre voulu : pour chaque scene K5 puis K10 a 48 fils, puis la scene 00
    # a K5 avec 24 puis 1 fil (le cas W1 est le plus susceptible d'etre coupe).
    cases = [case(scene, k, 48) for scene in ('00', '01', '02') for k in (5, 10)]
    cases += [case('00', 5, 24), case('00', 5, 1)]
    return dict(schema=worker.PLAN_SCHEMA, cases=cases)


def collect(commit, plan_raw=None, allow_uncommitted_protocol=False):
    """Rend (fichiers nom -> octets, provenance) sans rien ecrire."""
    full, tree = resolve_commit(commit)
    sources = {name: oid for name, oid in tree_entries(full, [worker.CMAKE_LISTS, *(prefix.rstrip('/') for prefix in
                                                                                   worker.SOURCE_PREFIXES)]).items()
               if PurePosixPath(name).name != '.gitkeep'}
    helper = tree_entries(full, [worker.HELPER])
    inputs = tree_entries(full, [data['source'] for data in worker.INPUTS.values()])
    protocol = tree_entries(full, sorted(worker.PROTOCOL_NAMES))
    need(set(helper) == {worker.HELPER} and set(inputs) == {data['source'] for data in worker.INPUTS.values()},
         'pinned helper/input absent at commit')
    blobs = read_blobs([*sources.values(), *helper.values(), *inputs.values(), *protocol.values()])
    files = {name: blobs[oid] for name, oid in sources.items()}
    files[worker.HELPER] = blobs[helper[worker.HELPER]]
    for data in worker.INPUTS.values():
        files[data['file']] = blobs[inputs[data['source']]]
    executing = {name: (ROOT / name).read_bytes() for name in sorted(worker.PROTOCOL_NAMES)}
    committed = set(protocol) == set(worker.PROTOCOL_NAMES) and all(
        blobs[protocol[name]] == executing[name] for name in worker.PROTOCOL_NAMES)
    need(committed or allow_uncommitted_protocol,
         'v9 protocol files are not committed identically at ' + full + '; commit them first')
    files.update(executing)   # identiques aux objets du commit quand committed
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
    worker.validate_data(read)
    worker.validate_provenance(worker.strict_json(read(worker.PROVENANCE)), manifest)
    return manifest, cases


def write_archive(path, files):
    with Path(path).open('xb') as stream:
        with gzip.GzipFile(filename='', mode='wb', fileobj=stream, mtime=0) as zipped:
            with tarfile.open(fileobj=zipped, mode='w') as archive:
                for name in sorted(files):
                    raw = files[name]
                    member = tarfile.TarInfo(name)
                    member.size, member.mode, member.mtime = len(raw), 0o444, 0
                    member.uid = member.gid = 0
                    member.uname = member.gname = ''
                    archive.addfile(member, io.BytesIO(raw))


def build(commit, output, plan_path=None, allow_uncommitted_protocol=False):
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
        write_archive(archive_path, files)
        session.save(output / 'source_manifest.json', manifest)
        session.validate_snapshot(archive_path, manifest)
        record.update(status='prepared_not_executed', snapshot_sha256=worker.sha(archive_path),
                      manifest_sha256=worker.sha(output / 'source_manifest.json'),
                      controller_sha256=worker.sha(session.__file__), worker_sha256=worker.sha(worker.__file__),
                      inputs={data['file']: dict(commit_path=data['source'], sha256=data['sha256'], n=data['n'])
                              for data in worker.INPUTS.values()},
                      real_session_allowed=provenance['protocol_source'] == 'commit')
    except BaseException as cause:
        error = cause
        record['error'] = type(cause).__name__ + ': ' + str(cause)
    session.save(output / 'PACKAGE.json', record)
    if error is not None:
        raise error
    return record


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--commit', required=True, help='commit dont les objets git forment le paquet')
    parser.add_argument('--plan', type=Path, help='plan mhgp9_tower_plan_v1 ; defaut : plan par defaut')
    parser.add_argument('--output', type=Path, required=True, help='repertoire neuf')
    parser.add_argument('--allow-uncommitted-protocol', action='store_true',
                        help='preflight/selftest seulement ; le controleur refuse ce paquet avec --execute')
    args = parser.parse_args(argv)
    print(json.dumps(build(args.commit, args.output, args.plan, args.allow_uncommitted_protocol),
                     sort_keys=True, allow_nan=False))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
