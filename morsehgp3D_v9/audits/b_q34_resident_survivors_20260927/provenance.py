#!/usr/bin/env python3
"""Source-only provenance and unchanged geometric kernels, not a GPU test."""
import difflib
import hashlib
import json
from pathlib import Path
import sys
HERE=Path(__file__).resolve().parent
OLD=HERE.parent/'b_q34_filtered_resident_20260927'
PINS={
    'device.hpp':'59747b0ff7289b0cda8532a5a6186626fd55aefa6711b3f207d2f5ed3c8edb9e',
    'device_cpu.cpp':'b1bdace2429499df54202d517c1e85cdb54398d5805c16e3f7410e5d3b6b62d9',
    'device_cuda.cu':'1a1e8bdbe2976b5377ed5222e06d49484b570f399cb6f8af4a53c50764758737',
    'device_stub.cpp':'4b442c75b7818bf32b00f2d4cf81bd8f705edaab3a6075a3fbff0ef98c297c56',
    'host.hpp':'7c3b20dd88d80b5b1722201c8755f576c6004e424a2e15373aeeb2831945f3f0',
    'CMakeLists.txt':'04fb5d0c8a99674b00e7e810aa787c3568d4f615b42a140213d4c95e442acb79'}
def sha(path): return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def need(value,why):
    if not value: raise ValueError(why)
def kernel(text,name):
    begin=text.index('__global__ void '+name+'(');opening=text.index('{',begin);depth=1;end=opening+1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[begin:end]
def check():
    need(all(sha(OLD/name)==pin for name,pin in PINS.items()),'frozen af369c44 source changed')
    old,new=(p.read_text() for p in (OLD/'device_cuda.cu',HERE/'device_cuda.cu'))
    kernels={}
    for name in ('filter_kernel','scatter_kernel','rectangles_kernel'):
        before,after=kernel(old,name),kernel(new,name);need(before==after,'geometric/scan scatter kernel changed: '+name)
        kernels[name]=hashlib.sha256(before.encode()).hexdigest()
    return dict(status='passed',baseline_commit='af369c44efa75236fa98e25e8f1bc4708b128fc4',
        old_source_sha256=PINS,new_source_sha256={name:sha(HERE/name) for name in PINS},
        unchanged_kernel_sha256=kernels,CUDA_executed=False,GCP_used=False)
def diff_text():
    check();out=''
    for name in PINS:
        out+=''.join(difflib.unified_diff((OLD/name).read_text().splitlines(True),(HERE/name).read_text().splitlines(True),
                                        fromfile='frozen_af369c44/'+name,tofile='output_only_port/'+name))
    return out
if __name__=='__main__':
    if len(sys.argv)==2 and sys.argv[1]=='--diff': print(diff_text(),end='')
    else:
        need(len(sys.argv)==1,'usage');print(json.dumps(check(),sort_keys=True))
