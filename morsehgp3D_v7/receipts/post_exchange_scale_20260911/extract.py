#!/usr/bin/env python3
"""Reconstruct retained logical text files into a new directory, never execute."""
import hashlib
import json
from pathlib import Path
import sys

BASE=Path(__file__).resolve().parent


def main():
    if len(sys.argv)!=2:raise RuntimeError('NEW_DIRECTORY_required')
    out=Path(sys.argv[1]).resolve()
    if out.exists():raise RuntimeError('new_destination_required')
    values={}
    for logical,row in json.loads((BASE/'storage_map.json').read_text()).items():
        target,stored=Path(logical),Path(row['storage'])
        if any(p.is_absolute() or '..' in p.parts for p in (target,stored)):raise RuntimeError('unsafe_path')
        value=(BASE/stored).read_bytes()
        if len(value)!=row['size'] or hashlib.sha256(value).hexdigest()!=row['sha256'] or value.startswith(b'\x7fELF'):
            raise RuntimeError('original_bytes_corrupted')
        values[target]=value
    out.mkdir()
    for relative,value in values.items():
        path=out/relative;path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('xb') as stream:stream.write(value)
    print(json.dumps(dict(status='extracted_not_executed',logical_files=len(values),ELF_included=False)))


if __name__=='__main__':main()
