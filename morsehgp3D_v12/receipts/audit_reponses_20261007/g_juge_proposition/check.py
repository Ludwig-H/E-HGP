#!/usr/bin/env python3
"""Correctif du juge G au pin 99fa : sept temoins precedents, schema producteur et gardes ciblees.

JSON uniquement. Python normal/-O ; aucun moteur, build, nuage ou GPU.
"""
import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
PRIOR = ROOT/'morsehgp3D_v12/receipts/audit_t2g_prepublication_20261007'
RELATIVE = 'morsehgp3D_v12/tests/tower/g_determinism.py'


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    meta = json.loads((HERE/'sources.json').read_text())
    dependencies = [HERE/'check.py', HERE/'sources.json', HERE/'proposition.patch', PRIOR/'check.py']
    hashes = {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in dependencies}
    source = {p:subprocess.check_output(['git','show',meta['pin']+':'+p],cwd=ROOT)
              for p in meta['sources_sha256']}
    need({p:sha(b) for p,b in source.items()} == meta['sources_sha256'], 'source hors pin')
    need(sha((HERE/'proposition.patch').read_bytes()) == meta['patch_sha256'], 'patch different')
    spec = importlib.util.spec_from_file_location('previous_g_witnesses', PRIOR/'check.py')
    prior = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(prior)
    # Les noms des compteurs viennent bien du schema C++ publie, pas d'un second format invente.
    header = source['morsehgp3D_v12/bench/tower_export.hpp'].decode()
    names = re.findall(r'"([a-z_0-9]+)"', header.split('kCounterNames[kScalarCounters] = {',1)[1].split('};',1)[0])
    need(tuple(names) == prior.OBJECT+prior.WORK, 'schema C++ et fabrique different')
    good = [prior.order(k,10) for k in range(1,6)]
    cases = [('temoin_cinq_ordres',prior.payload(good),0,0),
             ('seul_k1',prior.payload(good[:1]),0,2),
             ('k3_absent',prior.payload([r for r in good if r['k'] != 3]),0,2),
             ('k2_duplique_k3_absent',prior.payload([good[0],good[1],good[1],good[3],good[4]]),0,2),
             ('digest_non_hexadecimal',prior.payload(good,digest='x'*64),0,2),
             ('digest_liste',prior.payload(good,digest=[]),0,2)]
    cmake = source['morsehgp3D_v12/tests/tower/tests.cmake'].decode()
    pinned = re.search(r'set\(tower_scale_8000 "([^\"]+)"\)',cmake).group(1)
    totals = dict(re.findall(r'(naissances|cellules|representants|cibles_cellule)=(\d+)',pinned))
    prefix = re.search(r'empreinte=([0-9a-f]+)',pinned).group(1)
    truncated = prior.order(1,8000)
    truncated['objet'].update(births=int(totals['naissances']),cells=int(totals['cellules']),
                              representatives=int(totals['representants']))
    truncated['travail']['cell_stops'] = int(totals['cibles_cellule'])
    cases.append(('k1_tronque_totaux_wrapper',prior.payload([truncated],sites=8000,
                  digest=prefix+'0'*(64-len(prefix))),0,2))
    normal = prior.payload(good)
    normal[0]['diagnostics'] = dict.fromkeys(('count_ns','fill_ns','tables_ns','resolve_ns',
                                             'workspace_bytes','table_bytes','peak_bytes'),0)
    normal[0]['diagnostics']['order_ns'] = [0]*5
    cases.append(('format_complet_tower_probe',normal,0,0))
    cases.append(('sites_dedup_deux_sur_dix',prior.payload([prior.order(k,2) for k in (1,2)],sites=2),0,0))
    cases.append(('fils_dupliques',prior.payload(good),0,2))
    cases.append(('fils_texte_ignore',prior.payload(good),0,2))
    typed = prior.payload(good)
    typed[1]['objet']['births'] = True
    cases.append(('compteur_booleen',typed,0,2))
    wrong_k = prior.payload(good)
    wrong_k[0]['kmax'] = 4
    cases.append(('k_stage_different',wrong_k,0,2))
    cases.append(('tour_g_absent',prior.payload(good)[1:],0,2))
    cases.append(('digest_suffixe_different',prior.payload(good),1,1))
    results = []
    python = [sys.executable,'-B','-S']+(['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-g-proposition-') as tmp:
        folder = Path(tmp)
        before, proposed = folder/'before.py', folder/RELATIVE
        before.write_bytes(source[RELATIVE])
        proposed.parent.mkdir(parents=True)
        proposed.write_bytes(source[RELATIVE])
        subprocess.run(['git','apply','--check',str(HERE/'proposition.patch')],cwd=folder,check=True)
        subprocess.run(['git','apply',str(HERE/'proposition.patch')],cwd=folder,check=True)
        need(sha(proposed.read_bytes()) == meta['proposed_judge_sha256'],'corps propose incorrect')
        stub = folder/'sonde_json.py'
        code = '#!'+sys.executable+'\nimport json,pathlib,sys\n'
        code += "data=json.loads(pathlib.Path(__file__).with_suffix('.json').read_text())\n"
        code += "threads=int(next(s[10:] for s in sys.argv if s.startswith('--threads=')))\n"
        code += "for row in data['rows']:\n if row.get('phase')=='tour_g':\n  row['threads']=threads\n  row['pass']=row.pop('pass_index')\n if data['vary'] and threads!=1 and row.get('phase')=='digest': row['resolution_sha256']='0'*63+'1'\n print(json.dumps(row))\n"
        stub.write_text(code)
        stub.chmod(0o700)
        for name, rows, old_code, new_code in cases:
            wrapper = name == 'k1_tronque_totaux_wrapper'
            n, threads = (8000,'1,8') if wrapper else (10,'1,48')
            if name == 'fils_dupliques': threads = '1,1'
            if name == 'fils_texte_ignore': threads = '1,x,48'
            case = 'synth_u8000_k5' if wrapper else name
            args = [str(stub),case,'--uniform=%d,20261007,18'%n,'--k=5','--threads='+threads]
            stub.with_suffix('.json').write_text(json.dumps(dict(rows=rows,vary=name=='digest_suffixe_different')))
            row = dict(cas=name,ordres=[r['k'] for r in rows if r['phase']=='ordre'],
                       command_arguments=['<sonde_json.py>']+args[1:])
            for version, judge, expected in [('before',before,old_code),('proposed',proposed,new_code)]:
                proc = subprocess.run(python+[str(judge)]+args,capture_output=True,text=True,timeout=10)
                need(proc.returncode == expected,(name,version,proc.returncode,proc.stdout,proc.stderr))
                row[version] = dict(code=proc.returncode,stdout=proc.stdout.strip(),stderr=proc.stderr)
            if wrapper:
                need(row['before']['stdout'] == 'g_determinism_ok cas=synth_u8000_k5 fils=1,8 '+pinned,
                     'ligne wrapper historique differente')
            results.append(row)
    need(hashes == {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in dependencies},'dependance modifiee')
    print(json.dumps(dict(sources=meta,dependencies_sha256=hashes,results=results,
                         native_execution=False,scope='schema JSON et validation du juge; aucun temps HGP'),
                     ensure_ascii=False,sort_keys=True,indent=2))


if __name__ == '__main__':
    main()
