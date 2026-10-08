#!/usr/bin/env python3
"""Compare le périmètre source K aux blobs Git ; aucun moteur ou accès distant."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(body):
    return hashlib.sha256(body).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True)
    args = parser.parse_args()
    here = Path(__file__).parent
    cap = json.loads((here / 'capture.json').read_bytes())
    for name, digest in cap['receipt_artifacts_sha256'].items():
        need(sha((here / name).read_bytes()) == digest, 'reçu modifié : ' + name)
    need(sha(args.package.read_bytes()) == cap['package_sha256'], 'paquet différent')
    roots = cap['roots']

    def selected(path):
        return any(path == root or path.startswith(root + '/') for root in roots)

    package = {}
    with tarfile.open(args.package, 'r:gz') as tar:
        for member in tar:
            if not selected(member.name):
                continue
            need(member.isfile() and member.name not in package, 'membre source non ordinaire ou répété')
            package[member.name] = tar.extractfile(member).read()
    tree = subprocess.check_output(['git', '-C', str(args.repo), 'ls-tree', '-rz', '--full-tree',
                                    cap['commit'], '--'] + roots)
    blobs = {}
    for row in tree.split(b'\0'):
        if not row:
            continue
        attrs, name = row.split(b'\t', 1)
        mode, kind, oid = attrs.split()
        need(kind == b'blob' and mode in (b'100644', b'100755'), 'entrée Git non ordinaire')
        blobs[name.decode()] = oid.decode()
    need(set(package) == set(blobs), 'fichier manquant ou supplémentaire')
    names = sorted(blobs)
    result = subprocess.check_output(['git', '-C', str(args.repo), 'cat-file', '--batch'],
                                     input=('\n'.join(blobs[name] for name in names) + '\n').encode())
    at = 0
    for name in names:
        end = result.index(b'\n', at)
        oid, kind, size = result[at:end].split()
        need(oid.decode() == blobs[name] and kind == b'blob', 'réponse Git différente')
        size = int(size)
        at = end + 1
        need(package[name] == result[at:at + size], 'octets source différents : ' + name)
        at += size
        need(result[at:at + 1] == b'\n', 'fin de blob')
        at += 1
    need(at == len(result), 'réponse Git supplémentaire')
    manifest = ''.join(sha(package[name]) + '  ' + name + '\n' for name in names).encode()
    counts = {root: sum(name == root or name.startswith(root + '/') for name in names) for root in roots}
    need(counts == cap['scope_files'] and len(names) == cap['files']
         and sum(map(len, package.values())) == cap['bytes'], 'taille du périmètre')
    need(sha(manifest) == cap['source_manifest_sha256'], 'manifeste source différent')
    need(sha(args.package.read_bytes()) == cap['package_sha256'], 'paquet modifié pendant lecture')
    print(json.dumps(dict(ok=True, files=len(names), bytes=cap['bytes'], scope_files=counts,
                         source_manifest_sha256=sha(manifest), byte_identical=True,
                         native_executed=False, binary_identity_proven=False), sort_keys=True))


if __name__ == '__main__':
    main()
