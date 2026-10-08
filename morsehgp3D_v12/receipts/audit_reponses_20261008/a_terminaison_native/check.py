#!/usr/bin/env python3
"""Read hashes, Git, ar/map and closed logs only; never execute a native binary."""
import argparse,hashlib,json,os,re,subprocess,sys,types
from pathlib import Path
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent

def need(ok,why):
    if not ok:raise ValueError(why)
def sha(b):return hashlib.sha256(b).hexdigest()
def tree_digest(src):
    h=hashlib.sha256()
    for name in ('CMakeLists.txt','cmake','src','cli','bench','tests','tools','reference','docs'):
        o=src/name;paths=[o] if o.is_file() else []
        for folder,dirs,files in os.walk(o):
            dirs[:]=sorted(x for x in dirs if x!='__pycache__')
            paths.extend(Path(folder)/x for x in sorted(files) if not x.endswith('.pyc'))
        for f in sorted(paths):h.update(str(f.relative_to(src)).encode()+b'\0');h.update(sha(f.read_bytes()).encode()+b'\n')
    return h.hexdigest()
def archive(raw):
    need(raw.startswith(b'!<arch>\n'),'ar signature');pos=8;names=b'';out=[]
    while pos<len(raw):
        head=raw[pos:pos+60];need(len(head)==60 and head[58:]==b'`\n','ar member')
        size=int(head[48:58]);name=head[:16].rstrip();body=raw[pos+60:pos+60+size]
        need(len(body)==size,'ar length')
        if name==b'//':names=body
        elif name not in (b'/',b'/SYM64/'):
            if name.startswith(b'/'):
                off=int(name[1:]);end=names.find(b'/\n',off);need(end>=off,'ar name');name=names[off:end]
            else:name=name.rstrip(b'/')
            out.append((name.decode(),sha(body)))
        pos+=60+size+(size%2)
    need(pos==len(raw),'ar EOF');return out

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--repo',type=Path,required=True);ap.add_argument('--artifacts',type=Path,required=True);a=ap.parse_args()
    c=json.loads((HERE/'capture.json').read_text());raws={}
    for p,v in c['artifacts'].items():
        b=(a.artifacts/p).read_bytes();need(len(b)==v['bytes'] and sha(b)==v['sha256'],'artifact '+p);raws[p]=b
    git=lambda p:subprocess.check_output(['git','-C',str(a.repo),'show',c['commit']+':morsehgp3D_v12/'+p])
    names=subprocess.check_output(['git','-C',str(a.repo),'ls-tree','-r','--name-only',c['commit'],'--',*['morsehgp3D_v12/'+x for x in c['source_roots']]]).decode().splitlines()
    inventory=[]
    for name in names:
        rel=name.removeprefix('morsehgp3D_v12/');b=git(rel);inventory.append(sha(b)+'  '+rel+'\n')
        for root in ['w_term72/morsehgp3D_v12','mut_term/temoin_0/src']:
            need((a.artifacts/root/rel).read_bytes()==b,'Git source '+root+'/'+rel)
    need(len(names)==c['source_count'] and sha(''.join(sorted(inventory)).encode())==c['source_inventory_sha256'],'source inventory')
    for p,h in c['source_pins'].items():need(sha(git(p))==h,'pin '+p)
    report=json.loads(raws['mut_term.json']);need(report==c['mutants_report'],'mutant report')
    witness=a.artifacts/'mut_term/temoin_0/src'
    need(tree_digest(witness)==report['sources_sha256'],'witness source digest')
    manifest=git('tests/mutants/tower.json');need(sha(manifest)==report['manifeste_sha256'],'manifest hash')
    mutant=next(x for x in json.loads(manifest)['mutants'] if x['id']=='terminaison_relecture_native')
    altered={}
    for e in [mutant]+mutant.get('aussi',[]):
        p=e['fichier'];b=altered.get(p,(witness/p).read_text());need(b.count(e['cherche'])==1,'mutation cardinality')
        altered[p]=b.replace(e['cherche'],e['remplace'])
    variant=a.artifacts/'mut_term/terminaison_relecture_native/src'
    wf={str(p.relative_to(witness)):p for p in witness.rglob('*') if p.is_file() and '__pycache__' not in p.parts}
    vf={str(p.relative_to(variant)):p for p in variant.rglob('*') if p.is_file() and '__pycache__' not in p.parts}
    need(set(wf)==set(vf),'mutant file set')
    for p,f in wf.items():need(vf[p].read_bytes()==(altered[p].encode() if p in altered else f.read_bytes()),'unrelated mutation '+p)
    good='terminaison premier_evenement_A=retour attente_A=0 retours_A=1 retours_B=1 in_flight=0'
    bad='terminaison premier_evenement_A=attente attente_A=1 retours_A=1 retours_B=1 in_flight=0'
    for p in ['build_term72/Testing/Temporary/LastTest.log','mut_term/temoin_0/build/Testing/Temporary/LastTest.log']:
        text=raws[p].decode();need(good in text and 'test terminaison controles=11 echecs=0 plancher=11' in text and 'run_expect_verdict conforme' in text,'positive primary')
    text=raws['mut_term/terminaison_relecture_native/build/Testing/Temporary/LastTest.log'].decode()
    need(bad in text and 'code de sortie 1, attendu 0' in text and 'waits_a.load() == 0 && returns_a.load() == 1 && returns_b.load() == 1' in text,'causal assertion')
    text=raws['mut_term_native_sortie.log'].decode()
    need(text.count(bad)==3 and text.count('code=1')==3 and text.count('ECHECS 1')==3,'three mutant outputs')
    ctest=raws['ctest_term72_cible.log'].decode()
    need(len(re.findall(r'\d+/12 Test #\d+:.*Passed',ctest))==12 and '0 tests failed out of 12' in ctest,'targeted CTest')
    # Use the delivered pure-Python map reader on existing metadata, no CTest or native invocation.
    mod=types.ModuleType('pinned_map');exec(compile(git('tests/tower/region_map_check.py'),'region_map_check.py','exec'),mod.__dict__)
    need(mod.check(a.artifacts/'build_term72/mhgp12_tower_region.map',a.artifacts/'build_term72/libmhgp12.a')==[],'map linkage')
    for prefix,tsan in [('build_term72',False),('build_term72_tsan',True)]:
        target=raws[prefix+'/CMakeFiles/mhgp12_tower_region.dir/flags.make'].decode();prod=raws[prefix+'/CMakeFiles/mhgp12.dir/flags.make'].decode()
        need('-DMHGP12_REGION_HOOKS' in target and 'MHGP12_REGION_HOOKS' not in prod,'target-only macro')
        need(('-fsanitize=thread' in target)==tsan and ('-fsanitize=thread' in prod)==tsan,'sanitizer flags')
        link=raws[prefix+'/CMakeFiles/mhgp12_tower_region.dir/link.txt'].decode()
        need('mhgp12_tower_region.dir/src/tower/pipeline_run.cpp.o' in link and ('-fsanitize=thread' in link)==tsan,'target link')
    need(b'__tsan_init' in raws['build_term72_tsan/mhgp12_tower_region'] and b'libtsan.so' in raws['build_term72_tsan/mhgp12_tower_region'],'instrumented ELF evidence')
    for i in range(1,6):
        text=raws[f'tsan_region_{i}.log'].decode();need(good in text and 'mhgp12_test_ok tests=1 controles=11' in text and 'WARNING' not in text,'TSan output')
    # The baseline object files no longer accompany the two primary hash lists: compare their declarations,
    # then independently rehash every current object and every archive member. Do not claim a rebuilt baseline.
    before=raws['objets_base72/sha.txt'];after=raws['objets_base72/sha_patch.txt'];need(before==after,'base/current lists')
    rows=[line.split(None,1) for line in before.decode().splitlines()];need(len(rows)==51,'object count')
    expected=[]
    for digest,path in rows:
        rel=path.removeprefix('./');b=(a.artifacts/'build_term72/CMakeFiles/mhgp12.dir'/rel).read_bytes()
        need(sha(b)==digest,'object '+rel);expected.append((Path(rel).name,digest))
    need(sorted(archive(raws['build_term72/libmhgp12.a']))==sorted(expected),'archive member bytes')
    for p,b in raws.items():need((a.artifacts/p).read_bytes()==b,'artifact changed')
    result={'commit':c['commit'],'source_files_exact':len(names),'native_mutation_exact':True,'positive_controls':11,'targeted_ctest_passed':12,'native_mutant_primary':'assertion waits_a==0, code1, threads joined','mutant_extra_logs':3,'declared_object_hashes_equal':51,'current_objects_and_archive_rehashed':51,'map_check':'conforme','tsan_logs_positive':5,'tsan_build_instrumented':True,'tsan_external_codes_and_run_hash_binding':False,'native_executions_by_audit':0}
    print(json.dumps(result,sort_keys=True,indent=2))
if __name__=='__main__':main()
