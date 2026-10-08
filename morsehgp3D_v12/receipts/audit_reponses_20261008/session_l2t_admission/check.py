#!/usr/bin/env python3
"""MES-L2t : archives, sources et JSON seulement ; aucun moteur, réseau ou payload."""
from pathlib import Path
import argparse, csv, hashlib, importlib.util, io, json, re, subprocess, sys, tarfile, tempfile, types
sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent
P = 'morsehgp3D_v12/'
SOURCE = '8b9eab40a902e0322314ac059a3d0c24d76c46a9'
PUBLISHED = '94fa3a53b'
PUBLIC = P + 'receipts/g4_mesb2t_20261008/'

def need(value, reason):
    if not value: raise ValueError(reason)

def pin(raw): return dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())

def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module

def run(args):
    root = args.repo / P / 'receipts/audit_reponses_20261008'
    paths = dict(archive=root/'session_l1_recuperation/check.py',
                 source=root/'session_t1d_admission/check.py',
                 envelope=root/'session_l1r_contrelecture/reader.py',
                 metadata=root/'session_l2_contrelecture/capture.json')
    h, common = load('l2t_archive', paths['archive']), load('l2t_source', paths['source'])
    session = args.session
    receipt_raw = (session/'receipt.json').read_bytes()
    receipt = json.loads(receipt_raw)
    plan_raw = (session/'package/plan.json').read_bytes()
    plan = json.loads(plan_raw)
    archive = (session/'results/results.tar.gz').read_bytes()
    files = h.files(archive)
    h.manifest(files, 44)
    need(receipt['commit'] == SOURCE and receipt['results_sha256'] == pin(archive)['sha256']
         and receipt['results_bytes'] == len(archive), 'source/archive')
    need(receipt['status'] == 'completed' and receipt['worker_exit_code'] == 0
         and receipt['results_verified'] is True and not receipt['errors']
         and int((session/'DONE').read_text()) == 0, 'worker/DONE/results')
    need(receipt['targeted_shutdown_certified'] is True and receipt['stop_exit_code'] == 0
         and receipt['observed_before_stop']['status'] == 'RUNNING'
         and receipt['observed_after']['status'] == 'TERMINATED', 'certified shutdown')
    commands = list(csv.DictReader(io.StringIO(files['results/commands.tsv'].decode()), delimiter='\t'))
    need(commands == receipt['commands'] and len(commands) == 1
         and commands[0]['name'] == 'mes_b_l2' and commands[0]['exit_code'] == '0', 'commands')
    source, src = common.source(args.repo, session/'package/package.tar.gz', SOURCE)
    need(source['archive']['sha256'] == receipt['package_sha256']
         and pin(plan_raw)['sha256'] == receipt['plan_sha256'], 'package/plan')
    with tarfile.open(session/'package/package.tar.gz') as tar:
        for rel in ['microbancs/mes_b_scenes/pilote_b.py', 'microbancs/outils/banc_full.py']:
            raw = tar.extractfile(P+rel).read()
            need(raw == common.git(args.repo, SOURCE, P+rel), 'additional Git source')
            src[P+rel] = raw
    # Manifest no longer present at preflight.data_dir. Use the previously admitted
    # public L2 capture and require every declared file hash/size to be unchanged.
    meta = json.loads(paths['metadata'].read_text())['sessions']['L2']
    old_receipt_path = P+'receipts/g4_mesb2_20261008/receipt.json'
    old_raw = common.git(args.repo, PUBLISHED, old_receipt_path)
    old = json.loads(old_raw)
    declared = lambda r: {d['name']: {k:d[k] for k in ('size','sha256')} for d in r['data_files']}
    need(declared(receipt) == declared(old), 'L2 declared dataset differs')
    need(declared(receipt)['bundle_manifest.json']['sha256'] == meta['manifest_sha256'], 'metadata SHA')
    sites = meta['sites']
    base = 'results/cmd/000_mes_b_l2/files/b/'
    report_raw = files[base+'rapport_b.json']
    report = json.loads(report_raw)
    raws = {Path(name).name: raw for name, raw in files.items() if name.startswith(base+'brut/')}
    need(len(raws) == 8 and not files['results/cmd/000_mes_b_l2/stderr'].strip(), 'raw/command stderr')
    for name, raw in raws.items():
        need(common.git(args.repo, PUBLISHED, PUBLIC+'resultats/'+base.removeprefix('results/')+'brut/'+name) == raw,
             'published raw differs')
    public_report = json.loads(common.git(args.repo, PUBLISHED,
        PUBLIC+'resultats/'+base.removeprefix('results/')+'rapport_b.json'))
    need(all(public_report[k] == report[k] for k in ('cas','criteres','verdict','empreintes','provenance')),
         'published numerical report')
    for name, raw in {'rapport_b.json': report_raw, **{'brut/'+n:b for n,b in raws.items()}}.items():
        target = args.snapshot/name
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists(): need(target.read_bytes() == raw, 'snapshot changed')
        else: target.write_bytes(raw)
    # Exact port used for B1t: command index 0, overlap schema and the pinned pilot.
    text = paths['envelope'].read_text()
    changes = [("plan['commands'][1]['argv']", "plan['commands'][0]['argv']"),
       ("('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv')",
        "('fils','budget_octets','budget_appareil_octets','delai_global_s','series','argv','schema')"),
       ('457d0e6ff6f27daca0eb22f5852fe6b1e53fce1611c330fd3931a5b071c8c49d',
        pin(src[P+'microbancs/mes_b_scenes/pilote_b.py'])['sha256'])]
    for before, after in changes:
        need(text.count(before) == 1, 'port pattern')
        text = text.replace(before, after)
    reader = types.ModuleType('l2t_envelope')
    reader.__file__ = str(paths['envelope'])
    exec(compile(text, str(paths['envelope'])+' [L2t port]', 'exec'), reader.__dict__)
    with tempfile.TemporaryDirectory() as td:
        path = Path(td)/'lecteur_full.py'
        path.write_bytes(src[P+'microbancs/outils/lecteur_full.py'])
        lf = load('l2t_lf', path)
        def expected(case):
            return dict(voie=case['voie'], k=case['k'], fils=case['fils'], passes=case['expected'],
                empreinte=case['empreinte'], trames=[(case['etiquette'],case['sites'])],
                budget_appareil='separe', bits=21, schema='recouvert')
        def full(row, index, case):
            why = lf.check_full(row, index, expected(case))
            reader.need(not why, why)
            reader.need(16*case['sites'] <= row['pic_octets'] <= 160*(1<<30), 'host active budget')
            reader.need(row['epinglee_octets'] <= row['pic_octets'], 'pinned/host')
            reader.need(row['appareil_octets'] <= row['pic_appareil_octets'] <= 88*(1<<30), 'device budget')
            reader.need(all(pair[0] >= 16*case['sites'] for pair in row['memoire_octets'].values()), 'resident input')
            reader.need(row['hors_mur_ns']['empreinte'] == 0, 'digest outside plan')
        reader.full = full
        specs, options = reader.cohort(reader.decode(plan_raw), sites)
        need(len(specs) == 4 and all(c['k'] == 5 and c['voie'] == 'appareil'
             and c['expected'] == 1 and not c['empreinte'] for c in specs), 'cohort contract')
        need(report['parametres']['schema'] == 'recouvert', 'schema')
        reasons = dict(re.findall(r'^MHGP12_REASON\((\w+),\s*(\w+),', src[P+'src/core/reasons.def'].decode(), re.M))
        reviewed = reader.review(reader.decode(report_raw), raws, specs, options, meta['manifest_sha256'], reasons)
        need(reviewed['bruts_admis'], 'raw/report admission '+str(reviewed['conditions']))
        cases = []
        for entry, case, result in zip(report['cas'], specs, reviewed['cas']):
            tag = '%s_k%d_%s' % (case['nom'], case['k'], case['voie'])
            need(not raws[tag+'.err'].strip(), 'process stderr')
            second = lf.parse_output(entry['code'], raws[tag+'.jsonl'].decode('ascii'), expected(case))
            need(all(reader.same(entry[k], v) for k,v in second.items()), 'LF/envelope mismatch')
            cases.append({**{k:result[k] for k in ('nom','sites','k','voie','expected','etat','issue','raison','statistiques','passes')},
                'code':entry['code'], 'open_ns':result.get('open_ns'),
                'pic_nvidia_smi_mio':entry['pic_nvidia_smi_mio'], 'process_seconds':entry['secondes']})
    need(set(['CMAKE_BUILD_TYPE:STRING=Release','MHGP12_COORD_BITS:STRING=21',
              'MHGP12_ENABLE_CUDA:BOOL=ON']) <= set(report['provenance']['cmake']), 'CMake')
    native = subprocess.check_output(['git','-C',str(args.repo),'diff','--name-only','47feedc96',SOURCE,
        '--',P+'src',P+'bench/full_probe.cpp']).decode().splitlines()
    need(not native, 'R1 native changed')
    doc_raw = common.git(args.repo, PUBLISHED, PUBLIC+'README.md')
    with tempfile.TemporaryDirectory() as td:
        doc = Path(td)/PUBLIC/'README.md'; doc.parent.mkdir(parents=True); doc.write_bytes(doc_raw)
        for opts in [('--check',),(),('--reverse','--check'),('--reverse',)]:
            subprocess.run(['git','apply',*opts,str((HERE/'documentation.patch').resolve())], cwd=td, check=True, capture_output=True)
        need(doc.read_bytes() == doc_raw, 'documentation inverse')
    inventory = ''.join(pin(raw)['sha256']+'  '+name+'\n' for name,raw in sorted(raws.items()))
    capture = dict(source_git=SOURCE, source=source, published_commit=subprocess.check_output(['git','-C',str(args.repo),'rev-parse',PUBLISHED]).decode().strip(),
        source_equal_native_R1=True, metadata_directly_reread=False, declared_data_equal_L2=True,
        metadata_manifest_sha256=meta['manifest_sha256'], metadata_sites=sites,
        data_declared=declared(receipt), old_L2_receipt=pin(old_raw),
        closure={k:receipt[k] for k in ('status','worker_exit_code','results_verified','targeted_shutdown_certified',
             'stop_exit_code','stop_attempts','reserve_released','guest_guard_intact')},
        errors_count=0, done=0, stop_before='RUNNING', stop_after='TERMINATED', commands=commands,
        files={name:pin((session/name).read_bytes()) for name in ('receipt.json','DONE','preflight.json',
             'package/plan.json','package/package.tar.gz','results/results.tar.gz')},
        manifest=pin(h.role(files,'MANIFEST.sha256')), manifest_entries=44,
        helpers={key:pin(path.read_bytes()) for key,path in paths.items()},
        critical_sources={rel:pin(src[P+rel]) for rel in ('bench/full_probe.cpp','microbancs/mes_b_scenes/pilote_b.py',
             'microbancs/outils/lecteur_full.py','microbancs/outils/banc_full.py','src/core/reasons.def')},
        raw_inventory=dict(files=len(raws),sha256=pin(inventory.encode())['sha256']), report=pin(report_raw),
        ELF_initial=report['provenance']['sonde_sha256'], ELF_final_archived=False,
        published_readme=pin(doc_raw), documentation_patch=pin((HERE/'documentation.patch').read_bytes()))
    result = dict(processes=len(cases), full_passes=sum(len(c['passes']) for c in cases),
        warm_passes=sum(max(0,len(c['passes'])-1) for c in cases),
        successes=sum(c['etat']=='ok' for c in cases), refusals=sum(c['etat']=='refus' for c in cases),
        criteria=reviewed['criteres'], verdict=reviewed['verdict'], identities=reviewed['empreintes'],
        codes='declared by pinned pilot report; no independent per-process exit capture',
        cas=cases)
    return capture, result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('repo','session','snapshot'): parser.add_argument('--'+key,type=Path,required=True)
    parser.add_argument('--write',action='store_true')
    args = parser.parse_args()
    for key in ('repo','session','snapshot'): setattr(args,key,getattr(args,key).resolve())
    capture, result = run(args)
    for name,obj in [('capture.json',capture),('results.json',result)]:
        path = HERE/name
        if args.write: path.write_text(json.dumps(obj,ensure_ascii=False,separators=(',',':'))+'\n')
        else: need(json.loads(path.read_text()) == obj, name+' changed')
    print(json.dumps({k:result[k] for k in ('processes','full_passes','warm_passes','successes','refusals','criteria','verdict')}))

if __name__ == '__main__': main()
