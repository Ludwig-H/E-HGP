#!/usr/bin/env python3
"""Portable read-only CMake/source/work checks, effective under Python -O."""
import hashlib
import json
from pathlib import Path
import sys
import xml.etree.ElementTree as ET

BASE=Path(__file__).resolve().parent
PINS={'src/forest/full_ball_tower.hpp':'6763a877f43d79a45532bce4426feca645b6ee97f18c9a4ee1fea4c47cd408a5',
      'tests/full_ball_tower_gate.cpp':'c4f39462487610e32fd5b23210c268a4f7300490aff511252a3cf859afe9cb88',
      'bench/full_ball_tower_probe.cpp':'e96f8d36a523c6cfb64848ae5ee86933fef2279050691e6b4f774a430a0f2f46'}
BINARY='ade5dbdd7b35fb1f66a8e9f7ea6f33c315458b1ce12897396686ce7782cd9512'


def need(ok,why):
    if not ok:raise RuntimeError(why)


def sha(data):return hashlib.sha256(data).hexdigest()


def path(name):
    p=Path(name)
    need(not p.is_absolute() and '..' not in p.parts and p.as_posix()==name,'relative_path')
    need((BASE/p).resolve().is_relative_to(BASE),'inside_packet')
    return BASE/p


def main():
    need(len(sys.argv)==1,'no_arguments')
    manifest=json.loads((BASE/'MANIFEST.json').read_text())
    found={p.relative_to(BASE).as_posix() for p in BASE.rglob('*') if p.is_file() and p.name!='MANIFEST.json'}
    need(found==set(manifest['files']),'exact_inventory')
    for name,pin in manifest['files'].items():
        p=path(name);data=p.read_bytes()
        need(not p.is_symlink() and sha(data)==pin and not data.startswith(b'\x7fELF'),'hash_no_ELF')
    need(manifest['GCP_used'] is False and manifest['cuda_executed'] is False and
         manifest['public_status']=='not_claimed' and manifest['vendor_included'] is False,'scope')
    mapping=json.loads((BASE/'source_map.json').read_text())
    data={}
    for logical,row in mapping.items():
        path(logical)
        value=path(row['storage']).read_bytes()
        need(len(value)==row['size'] and sha(value)==row['sha256'],'original_bytes:'+logical)
        data[logical]=value
    def obj(name):return json.loads(data[name])
    receipt=obj('receipt.json')
    need(receipt['status']=='passed' and receipt['commands']==21 and receipt['selected_CTests']==24 and
         receipt['sources']==173 and receipt['used_dependencies']==1048 and receipt['sources_and_dependencies_stable'],
         'closed_capture_floors')
    before,after=obj('sources_before.json'),obj('sources_after.json')
    need(before==after and len(before)==173,'source_before_after')
    for name,pin in before.items():need(sha(data['source/'+name])==pin,'own_source_snapshot')
    for name,pin in PINS.items():
        need(sha(data['source/morsehgp3D_v7/'+name])==pin,'exact_active_clean_file')
    dependencies=obj('dependencies_before.json')
    need(dependencies==obj('dependencies_after.json') and len(dependencies)==1048,'used_dependencies_stable')
    for name,pin in dependencies.items():
        prefix='/workspaces/E-HGP/'
        if name.startswith(prefix):
            relative=name[len(prefix):]
            if relative in before:need(before[relative]==pin,'compiled_own_dependency_bound_to_snapshot')
    binaries=obj('binaries_before.json')
    need(binaries==obj('binaries_after.json')==receipt['binary_sha256'] and len(binaries)==10,'ten_binary_pins_stable')
    need(binaries['mhgp7_full_ball_tower_probe']==BINARY,'qualified_probe_ELF_pin_not_included')
    commands=obj('commands.json');need(len(commands)==21,'all_commands')
    prior=0
    for row in commands:
        need(row['exit_code']==row['expected_exit']==0 and prior<=row['started_ns']<=row['ended_ns'],'command_success')
        prior=row['ended_ns']
        for suffix in ('stdout','stderr'):
            need(sha(data[row['name']+'.'+suffix])==row[suffix+'_sha256'],'command_stream_pin')
    byname={row['name']:row for row in commands}
    need('-DMHGP7_ENABLE_CUDA=OFF' in byname['configure']['argv'],'CUDA_disabled')
    build=byname['build']['argv'];need(build[build.index('--parallel')+1]=='1','sequential_build')
    selected=obj('selected_compile_commands.json');need(len(selected)==10,'selected_compile_commands')
    need(all('-O3' in row['command'] and '-DNDEBUG' in row['command'] and '-std=c++20' in row['command'] for row in selected),
         'Release_flags')
    tests=obj('selected_tests.json');need(len(tests)==len(set(tests))==24,'24_distinct_CTests')
    xml=ET.fromstring(data['ctest.junit.xml'])
    need(xml.attrib['tests']=='24' and xml.attrib['failures']=='0' and xml.attrib['skipped']=='0','JUnit_success')
    cases=xml.findall('testcase');need({r.attrib['name'] for r in cases}==set(tests),'JUnit_selected_identity')
    need(all(r.attrib['status']=='run' and r.find('failure') is None for r in cases),'all_tests_ran')
    for name in ('mhgp7_full_ball_static_cpu1','mhgp7_full_ball_static_cpu4'):
        output=next(r.find('system-out').text for r in cases if r.attrib['name']==name)
        rows=[json.loads(line) for line in output.splitlines() if line.startswith('{')]
        need(len(rows)==1 and rows[0]['clouds']==34 and rows[0]['orders']==150 and rows[0]['vertical_checks']==87230,
             'active_static_geometry_floors')
        need('post_seed_queries=20 post_seed_hits=4 post_seed_terminals=4' in output,'active_hit_floor')
    a,b=obj('probe_200_static1.stdout'),obj('probe_O2_reference.stdout')
    times={'index_s','generate_s','sort_s','prefilter_s','census_s','tower_s','digest_s','total_s'}
    need({k:v for k,v in a.items() if k not in times}=={k:v for k,v in b.items() if k not in times},'O3_O2_all_non_time_fields')
    q,h,t=(sum(r[k] for r in a['static_orders']) for k in ('post_seed_queries','post_seed_hits','post_seed_terminals'))
    need((q,h,t,a['resolver_meb_calls'])==(17419,2945,2945,48618),'probe_work_crosscheck')
    need(a['resolver_meb_calls']==a['anchor_hits']+a['intruder_queries'] and a['contract_qualified'] is False,'actual_work_scope')
    for name in ('worker_normal','worker_optimized'):
        row=obj(name+'.stdout')
        need(row['status']=='passed' and row['checks']==389 and row['CUDA_executed'] is False and
             row['GCP_used'] is False and row['subprocess_invoked'] is False,'worker_format_not_GPU')
    print(json.dumps(dict(status='verified_active_post_exchange_CMake',commands=21,CTests=24,
        own_sources=173,used_dependency_pins=1048,source_header_sha256=PINS['src/forest/full_ball_tower.hpp'],
        probe_binary_sha256=BINARY,compiler_executed_here=False,GCP_used=False,public_status='not_claimed',
        latency_contract_qualified=False)))


if __name__=='__main__':main()
