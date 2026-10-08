#!/usr/bin/env python3
"""Compare les sources du paquet MES-C au Git ; aucun contrôleur ni donnée de scène."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile


def need(ok, message):
    if not ok:
        raise ValueError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, required=True)
    ap.add_argument('--package', type=Path, required=True)
    ap.add_argument('--plan', type=Path, required=True)
    ap.add_argument('--capture', type=Path, required=True)
    a = ap.parse_args()
    cap = json.loads(a.capture.read_text())
    raw = a.package.read_bytes()
    need(sha(raw)==cap['package_sha256'] and len(raw)==cap['package_bytes'], 'paquet différent')
    need(sha(a.plan.read_bytes())==cap['plan_sha256'], 'plan différent')
    roots, entries = cap['scopes'], {}
    tree = subprocess.check_output(['git','-C',str(a.repo),'ls-tree','-r','-z',cap['source_git'],'--',*roots])
    for entry in tree.split(b'\0'):
        if not entry:
            continue
        meta, name = entry.split(b'\t',1)
        _mode, kind, oid = meta.decode().split()
        need(kind=='blob', 'type Git')
        entries[name.decode()] = oid
    batch = subprocess.run(['git','-C',str(a.repo),'cat-file','--batch'],
                           input=''.join(v+'\n' for v in entries.values()).encode(),
                           capture_output=True, check=True).stdout
    blobs, offset = {}, 0
    for name, oid in entries.items():
        end = batch.index(b'\n',offset)
        head = batch[offset:end].decode().split()
        need(head[:2]==[oid,'blob'], 'lecture Git')
        size = int(head[2]); offset = end+1
        blobs[name] = batch[offset:offset+size]; offset += size+1
    seen = {}
    with tarfile.open(a.package) as archive:
        for member in archive:
            if not any(member.name.startswith(x) if x.endswith('/') else member.name==x for x in roots):
                continue
            if member.isdir():
                continue
            need(member.isfile() and member.name not in seen, 'membre dupliqué/non régulier')
            body = archive.extractfile(member).read()
            need(body==blobs.get(member.name), 'source différente')
            seen[member.name] = sha(body)
    need(set(seen)==set(entries) and len(seen)==cap['files_exact'], 'périmètre différent')
    inventory = ''.join(f'{v}  {k}\n' for k,v in sorted(seen.items()))
    need(sha(inventory.encode())==cap['source_inventory_sha256'], 'inventaire différent')
    need(a.package.read_bytes()==raw and sha(a.plan.read_bytes())==cap['plan_sha256'], 'mutation')
    print(json.dumps(dict(files_exact=len(seen), source_git=cap['source_git'], native_execution=False,
                          source_inventory_sha256=sha(inventory.encode())), sort_keys=True))


if __name__=='__main__':
    main()
