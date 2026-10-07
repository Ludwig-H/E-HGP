#!/usr/bin/env python3
"""One G1 translation unit, existing pinned v11 library, causal synthetic admission checks only."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[4]
PIN = 'f601b36ace16bcc8f7ac9bc532e45ab5079f9667'
LIBRARY_SHA = '050532a95c322cc2d064eef5bfcba74b3920227f98dc21d157a0d675732cf7ca'
OLD = ROOT / 'morsehgp3D_v12/receipts/audit_t2_20261007/mesures/g1_probe.py'
TU = ROOT / 'morsehgp3D_v12/microbancs/mes_g1_saut/mes_g1.cpp'
FLAGS = ['-std=c++20','-O2','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror',
         '-DMHGP11_COORD_BITS=21','-pthread']


def sha(data):
    return hashlib.sha256(data).hexdigest()


def need(ok, why):
    if not ok:
        raise RuntimeError(why)


def source_hash(path):
    raw = (ROOT/path).read_bytes()
    reference = subprocess.check_output(['git','show',PIN+':'+path],cwd=ROOT)
    need(raw == reference, 'source differs from pin: '+path)
    return sha(raw)


def dependencies(text):
    return sorted({str(Path(p).resolve().relative_to(ROOT))
                   for p in text.replace('\\\n','').split(':',1)[1].split()})


def build(library, directory):
    need(sha(library.read_bytes()) == LIBRARY_SHA, 'wrong v11 static library')
    binary, meta = directory/'g1', directory/'build.json'
    if meta.exists():
        doc = json.loads(meta.read_text())
        need(doc['pin'] == PIN and sha(binary.read_bytes()) == doc['binary_sha256'], 'cached binary identity')
        need(doc['source_sha256'] == {p:source_hash(p) for p in doc['source_sha256']}, 'cached source identity')
        return binary, doc
    directory.mkdir(parents=True,exist_ok=True)
    command = ['c++',*FLAGS,'-I',str(ROOT/'morsehgp3D_v11/src')]
    dep = subprocess.check_output([*command,'-MM',str(TU)],text=True)
    before = {p:source_hash(p) for p in dependencies(dep)}
    proc = subprocess.run([*command,'-MMD','-MF',str(directory/'deps.d'),str(TU),str(library),'-o',str(binary)],
                          capture_output=True,text=True,timeout=120)
    need(proc.returncode == 0, 'compilation: '+proc.stderr)
    compiled = dependencies((directory/'deps.d').read_text())
    need(set(compiled) == set(before), 'compiled dependency closure differs')
    need(before == {p:source_hash(p) for p in before}, 'sources changed while building')
    need(sha(library.read_bytes()) == LIBRARY_SHA, 'library changed while building')
    doc = dict(pin=PIN,library_sha256=LIBRARY_SHA,source_sha256=before,binary_sha256=sha(binary.read_bytes()),
               flags=FLAGS,compiler=subprocess.check_output(['c++','--version'],text=True).splitlines()[0],
               translation_units=1,archive_rebuilt=False,compile_returncode=proc.returncode)
    meta.write_text(json.dumps(doc,sort_keys=True,indent=2)+'\n')
    return binary, doc


def clean(value):
    if isinstance(value,dict):
        return {k:clean(v) for k,v in value.items() if k != 'secondes'}
    if isinstance(value,list):
        return [clean(v) for v in value]
    return value


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--library',type=Path,required=True)
    parser.add_argument('--build',type=Path,required=True)
    parser.add_argument('--baseline',type=Path,help='optional original binary, checked against the historical receipt')
    args = parser.parse_args()
    self_sha = sha(Path(__file__).read_bytes())
    old_sha = source_hash(str(OLD.relative_to(ROOT)))
    spec = importlib.util.spec_from_file_location('old_g1_probe',OLD)
    old = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(old)
    binary, meta = build(args.library,args.build)
    rows, baseline_rows = [], []
    baseline_meta = json.loads((OLD.parent/'g1_build.json').read_text())
    if args.baseline is not None:
        need(sha(args.baseline.read_bytes()) == baseline_meta['binary_sha256'],'historical binary identity')
        need(baseline_meta['library_sha256'] == LIBRARY_SHA,'historical library identity')
    with tempfile.TemporaryDirectory(prefix='ehgp-g1-resume-input-') as temp:
        folder = Path(temp)
        cat = old.catalogue()
        (folder/'cat.bin').write_bytes(cat)
        # The independent exact transition reader certifies the tiny catalogue itself.
        reader = ROOT/'morsehgp3D_v12/reference/transition_catalogue.py'
        reader_sha = source_hash(str(reader.relative_to(ROOT)))
        validation = subprocess.run([sys.executable,'-B','-S',*(['-O'] if sys.flags.optimize else []),str(reader),
                                     str(folder/'cat.bin'),str(folder/'cat.bin'),'--convention-candidat','v11'],
                                    capture_output=True,text=True,timeout=10)
        need(validation.returncode == 0,'exact catalogue rejected: '+validation.stderr)
        for name,route,extra,want in [('valid',2,[],0),('forged_catalogue_route',1,[],3),
                                     ('selected_order_absent',2,['--ordres','9'],2),
                                     ('mixed_valid_and_absent_order',2,['--ordres','2,9'],2),
                                     ('route3_forged_for_saturated_census',3,[],3)]:
            payload = old.order(route)
            (folder/'ordre_2.bin').write_bytes(payload)
            proc = subprocess.run([str(binary),str(folder),*extra],capture_output=True,text=True,timeout=10)
            lines = [json.loads(line) for line in proc.stdout.splitlines()]
            need(proc.returncode == want,name+' unexpected exit: '+str(proc.returncode)+' '+proc.stderr)
            if want == 0:
                summary = next(line for line in lines if line.get('phase') == 'bilan')
                need(summary['code'] == 0 and summary['route2']['parties'] == 1 and
                     summary['route2']['ensembles']['voisins']['certifiees'] == 1,'positive control')
            else:
                need(not any(line.get('phase') == 'bilan' and line.get('code') == 0 for line in lines),
                     name+' published a successful balance')
            rows.append(dict(name=name,exit_code=proc.returncode,stdout=clean(lines),stderr=proc.stderr,
                             order_sha256=sha(payload)))
            if args.baseline is not None and name in ('valid','forged_catalogue_route','selected_order_absent'):
                previous = subprocess.run([str(args.baseline),str(folder),*extra],capture_output=True,text=True,timeout=10)
                previous_lines = [json.loads(line) for line in previous.stdout.splitlines()]
                summary = next(line for line in previous_lines if line.get('phase') == 'bilan')
                need(previous.returncode == 0 and summary['code'] == 0,'historical baseline no longer reproduced')
                baseline_rows.append(dict(name=name,exit_code=previous.returncode,stdout=clean(previous_lines),
                                          stderr=previous.stderr,order_sha256=sha(payload)))
        gate = subprocess.run([str(binary),'--porte'],capture_output=True,text=True,timeout=15)
        gate_lines = [json.loads(line) for line in gate.stdout.splitlines()]
        expected = dict(porte='mes_g1',mutant_cote_nul=False,mutant_garde='aucun',temoins=4,admission=6,ecarts=0)
        need(gate.returncode == 0 and expected in gate_lines,'bounded native gate')
    need(meta['source_sha256'] == {p:source_hash(p) for p in meta['source_sha256']},'closing source hashes')
    need(sha(binary.read_bytes()) == meta['binary_sha256'],'closing binary hash')
    need(sha(args.library.read_bytes()) == LIBRARY_SHA,'closing library hash')
    if args.baseline is not None:
        need(sha(args.baseline.read_bytes()) == baseline_meta['binary_sha256'],'closing historical binary hash')
    need(sha(Path(__file__).read_bytes()) == self_sha,'script changed')
    result = dict(schema='ehgp.v12.audit_reprise.g1.v1',pin=PIN,build=meta,results=rows,
                  native_gate=clean(gate_lines),native_gate_exit_code=gate.returncode,
                  historical_script_sha256=old_sha,script_sha256=self_sha,catalogue_sha256=sha(cat),
                  transition_reader_sha256=reader_sha,transition_reader_exit_code=validation.returncode,
                  source_binary_library_hashes_closed=True,gcp_used=False,private_data_read=False,
                  baseline_results=baseline_rows,
                  baseline_binary_sha256=baseline_meta['binary_sha256'] if args.baseline is not None else None,
                  historical_lidar_dumps_replayed=False,saut_timing_arm_replayed=False)
    print(json.dumps(result,sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
