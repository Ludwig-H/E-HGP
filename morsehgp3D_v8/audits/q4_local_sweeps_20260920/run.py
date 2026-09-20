#!/usr/bin/env python3
"""Measured local sweeps; these roots do not include the positive-support cascade."""
import gzip
import json
from pathlib import Path
import sys
from evidence import BASE,execute,pins,sha,write
from inputs import cases,save


def main():
    binary,folder=Path(sys.argv[1]).resolve(),Path(sys.argv[2]).resolve()
    mode=int(sys.argv[3])
    if mode not in (0,1):raise RuntimeError('Only count or sweep measurement mode')
    if folder.exists():raise RuntimeError('Capture already exists')
    folder.mkdir()
    manifest=dict(schema='audit_local_sweeps_capture_v1',status='started',sources=pins(),
                  binary=str(binary),binary_sha256=sha(binary),mode=mode,records=[],
                  scope='local covered roots only; no positivity/support selection, FULL or global edge generation')
    write(folder/'MANIFEST.json',manifest)
    for name,points,provenance,_ in cases():
        file,digest=save(name,points)
        for domain in (0,1):
            command=[str(binary),str(file),'10','7','4096',str(domain),str(mode)]
            result=execute(command)
            record=dict(case=name,n=len(points),provenance=provenance,input_sha256=digest,domain=domain,**result)
            path=folder/f'{len(manifest["records"]):03d}.json.gz'
            path.write_bytes(gzip.compress(json.dumps(record,sort_keys=True).encode(),mtime=0))
            manifest['records'].append(dict(path=path.name,sha256=sha(path)))
            write(folder/'MANIFEST.json',manifest)
            if result['returncode']!=0:raise RuntimeError('Measurement failed; capture preserved')
            print(f'{name} domain={domain} mode={mode} completed',flush=True)
    if manifest['sources']!=pins() or manifest['binary_sha256']!=sha(binary):raise RuntimeError('Measurement sources changed')
    manifest['status']='completed'
    write(folder/'MANIFEST.json',manifest)
    write(folder/'COMPLETION.json',dict(status='completed',manifest_sha256=sha(folder/'MANIFEST.json'),records=len(manifest['records'])))


if __name__=='__main__':main()
