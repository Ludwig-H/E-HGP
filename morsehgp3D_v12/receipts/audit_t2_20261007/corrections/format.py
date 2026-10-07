#!/usr/bin/env python3
"""Vrai Reader MHGP12DP, petits fichiers, aucune lecture de la plage invalide."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
PIN='274592a30f6961cb7702125dcd2f031ff22b7df2'
RELS=['morsehgp3D_v12/microbancs/mes_m3_m4_tour/common/format.hpp',
      'morsehgp3D_v12/microbancs/mes_m3_m4_tour/vidage/vidage_v11.cpp',
      'morsehgp3D_v12/docs/CONTRAT_CATALOGUE.md']
def need(ok,why):
    if not ok:raise RuntimeError(why)
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def check_sources():
    result={}
    for rel in RELS:
        data=(ROOT/rel).read_bytes()
        need(data==subprocess.check_output(['git','-C',str(ROOT),'show',PIN+':'+rel]),'pin mismatch: '+rel)
        result[rel]=sha(ROOT/rel)
    return result

def main():
    before=check_sources(); own={n:sha(HERE/n) for n in ['reader.cpp','format.py']}
    header=struct.pack('<8s6IQ24s',b'MHGP12DP',1,1,21,1,0,1,1,b'audit')
    need(len(header)==64,'header shape')
    section=lambda size,count:struct.pack('<8sIIQ',b'POPVAL',size,0,count)
    cases={
      'empty_valid':(header+section(4,0),'admitted 0'),
      'one_valid':(header+section(4,1)+struct.pack('<II',0,0),'admitted 1'),
      'ordinary_truncation':(header+section(4,1),'refused'),
      'multiplication_overflow':(header+section(12,2**63),'refused'),
      'truncated_padding':(header+section(4,1)+struct.pack('<I',0),'refused'),
      'truncated_section':(header+b'0'*10,'refused'),
      'trailing_bytes':(header+section(4,0)+b'0'*8,'refused'),
      'addition_overflow':(header+section(4,2**62-1),'refused'),
    }
    compiler=subprocess.check_output(['g++','--version'],text=True).splitlines()[0]
    with tempfile.TemporaryDirectory(prefix='ehgp-u32-format-') as tmp:
        d=Path(tmp);binary=d/'reader';dep=d/'reader.d'
        command=['g++','-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror',
                 '-I'+str((ROOT/RELS[0]).parent),'-MMD','-MF',str(dep),str(HERE/'reader.cpp'),'-o',str(binary)]
        subprocess.run(command,check=True,capture_output=True,text=True,timeout=45)
        deps=dep.read_text().replace('\\\n','').split(':',1)[1].split()
        need(set(map(lambda s:str(Path(s).resolve()),deps))=={str(HERE/'reader.cpp'),str(ROOT/RELS[0])},'unclosed dependency')
        results={}
        for name,(data,expected) in cases.items():
            f=d/(name+'.bin');f.write_bytes(data)
            run=subprocess.run([str(binary),str(f)],capture_output=True,text=True,timeout=5)
            need(run.returncode==0 and run.stdout.strip()==expected and not run.stderr,'unexpected result '+name)
            results[name]={'file_bytes':len(data),'response':run.stdout.strip(),'probe_exit':run.returncode}
        binary_hash=sha(binary)
    need(check_sources()==before and own=={n:sha(HERE/n) for n in own},'changed source/witness')
    print(json.dumps({'pin':PIN,'schema':'audit.t2.corrections.format.v1','sources_sha256':before,
        'witness_sha256':own,'compiler':compiler,'binary_sha256':binary_hash,'cases':results,
        'payload_read':False,'huge_allocation':False,'gcp_used':False,'gpu_used':False,
        'before_after_unchanged':True,'scope':'generic Reader admission, not full catalogue or M4 adoption'},indent=2,sort_keys=True))
if __name__=='__main__':main()
