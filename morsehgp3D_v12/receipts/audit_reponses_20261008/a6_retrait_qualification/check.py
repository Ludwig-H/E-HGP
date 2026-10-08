#!/usr/bin/env python3
"""Relecture d'archives de RESULTATS, sources Git et JSON ; aucun moteur/controleur."""
import csv
import hashlib
import io
import json
from pathlib import Path
import re
import subprocess
import sys
import tarfile
import tempfile

HERE = Path(__file__).resolve().parent
PREFIX = 'morsehgp3D_v12/'


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(b):
    return hashlib.sha256(b).hexdigest()


def git(repo, *args):
    return subprocess.check_output(['git', '-C', str(repo), *args])


def archive(blob):
    with tarfile.open(fileobj=io.BytesIO(blob)) as t:
        members = [m for m in t if m.isfile()]
        need(len(members) == len({m.name for m in members}), 'duplicate archive member')
        data = {m.name: t.extractfile(m).read() for m in members}
    manifest = data['results/MANIFEST.sha256']
    expected = {}
    for line in manifest.decode().splitlines():
        h, name = line.split(None, 1)
        name = 'results/' + name.strip().removeprefix('./')
        need(name not in expected, 'duplicate manifest entry')
        expected[name] = h
    need(set(data)-{'results/MANIFEST.sha256'} == set(expected), 'manifest inventory')
    need(all(sha(data[n]) == h for n, h in expected.items()), 'manifest hash')
    commands = list(csv.DictReader(io.StringIO(data['results/commands.tsv'].decode()), delimiter='\t'))
    return data, dict(bytes=len(blob), sha256=sha(blob), manifest_members=len(expected),
                      manifest_sha256=sha(manifest), commands=commands)


def source_package(repo, package, pin):
    paths = [PREFIX+x for x in ['src', 'bench', 'tests', 'cmake', 'CMakeLists.txt']]
    expected = git(repo, 'ls-tree', '-r', '--name-only', pin, '--', *paths).decode().splitlines()
    inventory = []
    with tarfile.open(package) as t:
        entries = {m.name:m for m in t if m.isfile()}
        for name in expected:
            b = t.extractfile(entries[name]).read()
            need(b == git(repo, 'show', pin+':'+name), 'package source '+name)
            inventory.append([name, sha(b)])
    return dict(files=len(inventory), inventory_sha256=sha(json.dumps(inventory).encode()))


def main(repo, session):
    cap = json.loads((HERE/'capture.json').read_text())
    for name, h in cap['primaries'].items():
        need(sha((session/name).read_bytes()) == h, 'primary '+name)
    after, out = archive((session/'results/results.tar.gz').read_bytes())
    first, prior = archive(after['results/cmd/000_archive_t2da6/files/t2da6_archive/results.tar.gz'])
    recovered, recovery = archive(after['results/cmd/001_archive_t2da6r/files/t2da6r_archive/results.tar.gz'])
    need(sha(recovered['results/cmd/000_archive_t2da6/files/t2da6_archive/results.tar.gz']) == prior['sha256'], 'same recovered first')
    receipt = json.loads((session/'receipt.json').read_text())
    keys = ['commit', 'worker_exit_code', 'status', 'results_verified', 'targeted_shutdown_certified',
            'stop_exit_code', 'stop_attempts', 'guest_guard_intact', 'reserve_released']
    closure = {k:receipt[k] for k in keys}
    closure['done'] = int((session/'DONE').read_text())
    closure['observed_after'] = receipt['observed_after']['status']
    need(closure['commit'] == cap['source_6497'] and closure['done'] == closure['worker_exit_code'] == closure['stop_exit_code'] == 0, 'closure codes')
    need(closure['status'] == 'completed' and closure['observed_after'] == 'TERMINATED' and closure['targeted_shutdown_certified'] is True, 'closure status')
    need(not receipt['errors'] and all(c['exit_code'] == '0' and c['status'] == 'ok' for c in out['commands']), 'campaign commands')
    declared = [{k:d[k] for k in ('name','size','sha256')} for d in receipt['data_files']]
    need(sorted(declared,key=lambda x:x['name']) == sorted(cap['reader_metadata']['data_declared'],key=lambda x:x['name']), 'same declared inputs')
    plan = json.loads((session/'package/plan.json').read_text())
    argv = plan['commands'][3]['argv']
    need(plan['commands'][3]['name'] == 't2da6_pilote' and '--essai' not in argv, 'plan command')
    for option, value in cap['reader_metadata']['configuration'].items():
        need(argv[argv.index(option)+1] == value, 'plan '+option)
    sources = source_package(repo, session/'package/package.tar.gz', cap['source_6497'])
    scopes = [PREFIX+x for x in ['src/tower','tests/tower','tests/mutants/tower.json']]
    need(not git(repo,'diff','--name-only',cap['base'],cap['withdrawal'],'--',*scopes), 'withdrawal differs')
    patches = {}
    with tempfile.TemporaryDirectory(prefix='a6-review-') as tmp:
        root=Path(tmp)
        for label, name, patch in [
            ('judge','microbancs/mes_t2d_a6/pilote_t2d_a6.py','a6_pilote_admission'),
            ('bridge','src/tower/forest_kernel.cpp','a6_indices_concurrence')]:
            target=root/PREFIX/name;target.parent.mkdir(parents=True,exist_ok=True)
            target.write_bytes(git(repo,'show','30a69104a:'+PREFIX+name))
            diff=git(repo,'show','ce81936fc:'+PREFIX+'receipts/audit_reponses_20261008/'+patch+'/proposition.patch')
            subprocess.run(['git','apply','--check','-'],input=diff,cwd=root,check=True)
            subprocess.run(['git','apply','-'],input=diff,cwd=root,check=True)
            delivered=git(repo,'show',cap['source_6497']+':'+PREFIX+name)
            if label=='judge':need(target.read_bytes()==delivered,'judge exact postimage')
            else:
                clean=lambda b:re.sub(rb'//[^\n]*',b'',b).split()
                need(clean(target.read_bytes())==clean(delivered),'bridge executable body')
            patches[label]=dict(proposal_sha256=sha(diff),delivered_sha256=sha(delivered),
                                exact_bytes=target.read_bytes()==delivered)
        measures={}
        for label, data, pin in [('first',first,'30a69104a'),('bridge',after,cap['source_6497'])]:
            folder=root/label;folder.mkdir()
            for name,b in data.items():
                if '/files/t2da6/' not in name:continue
                rel=name.split('/files/t2da6/',1)[1]
                if rel!='rapport_t2d_a6.json' and not (rel.startswith('journaux/') and rel.endswith('.jsonl')):continue
                need(not Path(rel).is_absolute() and '..' not in Path(rel).parts,'journal path')
                p=folder/rel;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(b)
            target=root/(label+'.json')
            subprocess.check_output([sys.executable,str(HERE/'admit.py'),'--repo',str(repo),'--report',str(folder/'rapport_t2d_a6.json'),'--out',str(target)])
            r=json.loads(target.read_text())
            need(not r['worker_judgment_differences'] and r['judgment']['verdict']=='rejete','worker judgment')
            measures[label]={'source':pin,'report':r['report'],'processes':r['processes'],'full_passes':r['full_passes'],
                'decisive_warm':r['decisive_warm'],'verdict':r['judgment']['verdict'],'refus':r['judgment']['refus'],
                'identity':r['judgment']['identite_ok'],'statistics':r['judgment']['cas']['statistiques'],
                'ng_process_medians_ns':{f:{a:q['median_process_medians_ns'] for a,q in x.items()} for f,x in r['ng'].items()},
                'large_summary':r['large_summary'],'elf_initial':r['elf_initial'],'elf_final':r['elf_final'],
                'raw_inventory_sha256':sha(json.dumps(r['journal_pins'],sort_keys=True).encode())}
        gate=repo/PREFIX/'receipts/audit_reponses_20261008/a6_pilote_admission/check.py'
        synth=json.loads(subprocess.check_output([sys.executable,str(gate),str(repo)]))
        need(synth['essai']['proposed_table_verdict']=='essai' and synth['official_selftests']==6,'synthetic gates')
    gates={}
    for label,data,fast,lidar in [('first',first,'000_socle_ctest','002_lidar_ctest'),('bridge',after,'002_socle_ctest','005_lidar_ctest')]:
        text=data['results/cmd/'+fast+'/stdout'].decode()
        gates[label]={'passed':len(re.findall(r'^\s*\d+/734\b.*\bPassed\b',text,re.M)),
                      'skipped':len(re.findall(r'^\s*\d+/734\b.*\bSkipped\b',text,re.M)),
                      'lidar_7':b'100% tests passed, 0 tests failed out of 7' in data['results/cmd/'+lidar+'/stdout']}
        need(gates[label]==dict(passed=734,skipped=0,lidar_7=True),'gates totals')
    return dict(closure=closure,sources=sources,patches=patches,archives={'first':prior,'recovery':recovery,'bridge':out},
                measurements=measures,gates=gates,judge_checks=synth,withdrawal_scopes_equal=True,native_calls=0)


if __name__=='__main__':
    result=main(Path(sys.argv[1]),Path(sys.argv[2]))
    if '--check' in sys.argv:need(result==json.loads((HERE/'results.json').read_text()),'stored result')
    print(json.dumps(result,ensure_ascii=False,sort_keys=True,indent=2))
