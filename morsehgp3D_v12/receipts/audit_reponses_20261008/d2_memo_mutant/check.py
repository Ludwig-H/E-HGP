#!/usr/bin/env python3
"""Applique la proposition et le vrai lecteur de mutants ; aucune compilation."""
import argparse
from fractions import Fraction as F
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import types

HERE=Path(__file__).resolve().parent


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def replay(repo):
    pins=json.loads((HERE/'pins.json').read_text())
    with tempfile.TemporaryDirectory(prefix='date_mutant_') as name:
        root=Path(name)
        for item in pins['inputs']:
            raw=subprocess.check_output(['git','show',pins['pin']+':'+item['path']],cwd=repo)
            need(sha(raw)==item['sha256'],'source differs')
            p=root/item['path'];p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(raw)
        for extra in (['--check'],[]):
            subprocess.run(['git','apply',*extra,str(HERE/'proposition.patch')],cwd=root,check=True,
                           stdout=subprocess.PIPE,stderr=subprocess.PIPE)
        product=root/'morsehgp3D_v12'
        manifest=product/'tests/mutants/tower.json'
        need(sha(manifest.read_bytes())==pins['manifest_after_sha256'],'manifest differs')
        helper=product/'tests/mutants/run_mutants.py'
        module=types.ModuleType('date_mutant_runner');module.__file__=str(helper)
        exec(compile(helper.read_bytes(),str(helper),'exec'),module.__dict__)
        parsed=module.load_manifest(str(manifest))
        need(parsed['plancher']==8 and len(parsed['mutants'])==8,'floor/cohort')
        module.check_patterns(str(product),parsed['mutants'])
        mutant=parsed['mutants'][-1]
        need(mutant['porte']=='mhgp12_tower_unit_witness_memo','wrong gate')
        changed=module.mutated_files(str(product),mutant)
        need(list(changed)==['src/tower/resolve.cpp'] and len(module.edits_of(mutant))==4,'scope')
        body=changed['src/tower/resolve.cpp']
        need(body.count('if (chain == 1)')==2,'first-step branches')
        # Les fonctions control_rank/control_level restent rigoureusement identiques.
        original=(product/'src/tower/resolve.cpp').read_text()
        need(body.split('Result<u32> resolve_part',1)[0]==original.split('Result<u32> resolve_part',1)[0],
             'shared controls modified')
        unit=(product/'tests/tower/unit.cpp').read_text()
        need('for (int kmax : {2, 3})' in unit and
             'CHECK_EQ(early.target.outcome().reason, Reason::tower_invariant);' in unit,
             'witness no longer reaches expected refusal')
        mutated_sha=sha(body.encode())
    def verdict(levels,junction,mutant=False):
        if not all(b<a for a,b in zip(levels,levels[1:])):
            return False
        return levels[-1]<junction if mutant else levels[0]<junction
    cases=[]
    for label,levels,j in [('memo_K2_census',[F(9),F(1)],F(4)),
                           ('memo_K3_catalogue',[F(9),F(1)],F(4)),
                           ('valid_D2',[F(64),F(1)],F(1681,25)),
                           ('terminal_equal_junction',[F(9),F(4)],F(4)),
                           ('nondecreasing_step',[F(1),F(2)],F(4))]:
        cases.append(dict(case=label,original=verdict(levels,j),mutant=verdict(levels,j,True)))
    need([(x['original'],x['mutant']) for x in cases]==[(False,True),(False,True),(True,True),(False,False),(False,False)],
         'date model differs')
    return dict(patch_applies=True,real_manifest_reader_accepts=True,all_eight_patterns_apply=True,
                added_mutant=mutant['id'],gate=mutant['porte'],mutated_source_sha256=mutated_sha,
                shared_control_functions_unchanged=True,model=cases,native_played=False,
                expected_native_result='witness_memo fails by code; not yet executed')


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--repo-root',type=Path,default=HERE.parents[3]);p.add_argument('--check',action='store_true')
    args=p.parse_args();out=replay(args.repo_root)
    if args.check:
        need(out==json.loads((HERE/'results.json').read_text()),'results differ')
        print('d2_memo_mutant_proposition_ok: application et modele, aucun natif')
    else:print(json.dumps(out,indent=1,sort_keys=True))
