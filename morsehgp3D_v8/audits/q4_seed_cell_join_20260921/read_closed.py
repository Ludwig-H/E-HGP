#!/usr/bin/env python3
"""Verify immutable receipt dependencies even after the live product advances.

Every dependency must match its original SHA256 either at its live path or in
the explicit content-addressed snapshot. Nothing is inferred from newer code.
Then run the original frozen reader with its live-only guard disabled; inputs,
commands, record hashes, outputs and the small oracle remain fully checked.
"""
import argparse
import json
from pathlib import Path
from campaign import BASE, ROOT, load, require, sha
from read import read as original_read


def read(folder):
    manifest=load(folder/'MANIFEST.json')
    for name, expected in {**manifest['sources'],**manifest['binaries']}.items():
        live=ROOT/name
        archive=BASE/'snapshot/by_sha'/expected
        require((live.is_file() and sha(live)==expected) or
                (archive.is_file() and sha(archive)==expected), 'Frozen dependency unavailable: '+name)
    return original_read(folder,check_live=False)


if __name__=='__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('folder',type=Path)
    parser.add_argument('--compact',action='store_true')
    args=parser.parse_args()
    result=read(args.folder.resolve())
    if args.compact: result.pop('rows')
    print(json.dumps(result,sort_keys=True))
