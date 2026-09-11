#!/usr/bin/env python3
"""Explicit reversible extraction to a new directory; never execute a capture."""
import hashlib
import json
from pathlib import Path
import sys

BASE=Path(__file__).resolve().parent


def main():
    if len(sys.argv)!=2:
        raise RuntimeError('usage: extract.py NEW_DIRECTORY')
    out=Path(sys.argv[1]).resolve()
    if out.exists():
        raise RuntimeError('new_directory_required')
    mapping=json.loads((BASE/'storage_map.json').read_text())
    contents={}
    for logical,row in mapping.items():
        relative=Path(logical)
        storage=Path(row['storage'])
        if relative.is_absolute() or '..' in relative.parts or storage.is_absolute() or '..' in storage.parts:
            raise RuntimeError('unsafe_path')
        data=(BASE/storage).read_bytes()
        if len(data)!=row['size'] or hashlib.sha256(data).hexdigest()!=row['sha256'] or data.startswith(b'\x7fELF'):
            raise RuntimeError('capture_corrupted:'+logical)
        contents[relative]=data
    out.mkdir()
    for relative,data in contents.items():
        path=out/relative
        path.parent.mkdir(parents=True,exist_ok=True)
        with path.open('xb') as stream:
            stream.write(data)
    print(json.dumps(dict(status='extracted_not_executed',logical_files=len(contents),ELF_included=False)))


if __name__=='__main__':
    main()
