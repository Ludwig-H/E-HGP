#!/usr/bin/env python3
"""Close normal/-O readers and analysis without rerunning qualified C++ probes."""
from pathlib import Path
import sys
from campaign import BASE, execute, require, sha, write


def inventory():
    return {str(p.relative_to(BASE)):sha(p) for p in sorted(BASE.rglob('*'))
            if p.is_file() and not any(part in ('.build','.inputs','__pycache__') for part in p.parts)
            and p.name != 'VALIDATION.json'}


def main():
    path=BASE/'VALIDATION.json'
    require(not path.exists(),'Validation receipt already exists')
    before=inventory()
    records=[]
    output=dict(status='started',files=before,records=records)
    write(path,output)
    pairs=[]
    for name in ('qualification_r1','refined_r1','lidar_r1','lidar_anchors_r1'):
        args=[str(BASE/'read.py'),str(BASE/'receipts'/name),'--compact']
        pairs.append([args,args])
    args=[str(BASE/'analyze.py'),str(BASE/'receipts/lidar_r1'),str(BASE/'receipts/lidar_anchors_r1')]
    pairs.append([args,args])
    for normal, optimized in pairs:
        reports=[]
        for options,args in (([],normal),(['-O'],optimized)):
            result=execute([sys.executable,'-B',*options,*args])
            records.append(result)
            write(path,output)
            require(result['returncode']==0 and not result['stderr'],'Reader failed; captured in VALIDATION.json')
            reports.append(result['stdout'])
        require(reports[0]==reports[1],'Normal/-O mismatch')
        print('PASS reader',normal[0],normal[1:],flush=True)
    require(reports[0] == (BASE/'LIDAR_SUMMARY.json').read_text(), 'Stored analysis differs')
    require(before==inventory(),'Files changed during final validation')
    output.update(status='passed',normal_optimized_identical=True)
    write(path,output)


if __name__=='__main__':
    main()
