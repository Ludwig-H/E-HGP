#!/usr/bin/env python3
"""Contre-lecture G-c patch 1 : JSON seulement et un nuage exact de huit sites.

Usage: python3 -B check.py RACINE_V12 > results.json
Reconstruction: extraire main6f362 puis appliquer active.patch depuis la racine du depot.
Aucun binaire moteur, jeu prive ou chrono natif. Les fausses sondes restent des fixtures de juge.
"""
import ast
import contextlib
import hashlib
import importlib.util
import io
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
from types import SimpleNamespace

sys.dont_write_bytecode = True
HERE = Path(__file__).resolve().parent


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def function(source, name):
    tree = ast.parse(source)
    return ast.dump(next(node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name == name))


def compact_cpp(source):
    return re.sub(r'\s+', '', re.sub(r'//[^\n]*', '', source))


def main():
    source = Path(sys.argv[1]).resolve()
    capture = json.loads((HERE / 'capture.json').read_text())
    for name, record in capture['files'].items():
        require(hashlib.sha256((source / name).read_bytes()).hexdigest() == record['active_sha256'],
                'source differente de la capture : ' + name)
    # Le diff permet de reconstruire le parent sans exiger le petit depot Git du prototype.
    patch = (HERE / 'active.patch').read_text()
    require(hashlib.sha256(patch.encode()).hexdigest() == capture['active_patch_sha256'], 'patch modifie')
    old_files = {}
    current = None
    chunks = {}
    for line in patch.splitlines(True):
        if line.startswith('diff --git '):
            current = line.split(' b/', 1)[1].strip().removeprefix('morsehgp3D_v12/')
            chunks[current] = []
        if current is not None:
            chunks[current].append(line)
    # git apply --reverse sur une copie temporaire: pas de modification du prototype.
    import tempfile
    with tempfile.TemporaryDirectory(prefix='audit-gc-object-') as temp:
        root = Path(temp)
        for name in chunks:
            dest = root / 'morsehgp3D_v12' / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            dest.write_bytes((source / name).read_bytes())
        done = subprocess.run(['git', 'apply', '--reverse', '--unsafe-paths', str(HERE / 'active.patch')],
                              cwd=root, capture_output=True)
        require(done.returncode == 0, 'reconstruction du parent impossible')
        for name in chunks:
            old_files[name] = (root / 'morsehgp3D_v12' / name).read_text()
            require(hashlib.sha256(old_files[name].encode()).hexdigest() == capture['files'][name]['base_sha256'],
                    'parent different : ' + name)
    judge_path = 'tests/tower/g_determinism.py'
    active_judge = (source / judge_path).read_text()
    kept = ['natural', 'decimal', 'run', 'invariants']
    admission_same = all(function(active_judge, name) == function(old_files[judge_path], name) for name in kept)
    require(admission_same, 'admission ou invariants modifies')
    # A defaut object_only=false, le code execute pour res.bin est identique au parent.
    export = (source / 'bench/tower_export.hpp').read_text()
    reconstructed = export.replace('bool header = true,\n                  bool object_only = false)',
                                   'bool header = true)')
    reconstructed = reconstructed.replace(
        '    if (object_only) MHGP12_TRY(column(out, "COBJ", k, std::span<const u64>(v.data(), kObjectCounters)));\n'
        '    else MHGP12_TRY(column(out, "CNTR", k, std::span<const u64>(v.data(), v.size())));',
        '    MHGP12_TRY(column(out, "CNTR", k, std::span<const u64>(v.data(), v.size())));')
    reconstructed = reconstructed.replace('sink, false, true)', 'sink, false)')
    res_code_same = compact_cpp(reconstructed) == compact_cpp(old_files['bench/tower_export.hpp'])
    require(res_code_same, 'autre changement de code dans exportateur')
    require('kObjectCounters = 5;' in export and '"TMSK", k, o.trace_masks()' in export and
            '"TARG", k, o.targets()' in export, 'frontiere de empreinte inattendue')

    judge = load('gc_active_judge', source / judge_path)
    fake = load('gc_fake_probe', source / 'tests/tower/g_fausse_sonde.py')
    old_subprocess_run = judge.subprocess.run
    previous_mode = os.environ.get('MHGP12_FAUSSE_SONDE')
    results = []
    modes = [('', 0), ('k1_seul', 2), ('ordre_double', 2), ('empreinte_courte', 2), ('sans_sortie', 2),
             ('travail_8', 1), ('objet_8', 1), ('digest_suffix_8', 1), ('negative_work', 2),
             ('bool_work', 2), ('exit_order_false', 0)]
    try:
        for mode, expected in modes:
            def fake_run(command, **kwargs):
                os.environ['MHGP12_FAUSSE_SONDE'] = mode
                text = io.StringIO()
                with contextlib.redirect_stdout(text):
                    fake.main(command[1:])
                rows = [json.loads(line) for line in text.getvalue().splitlines()]
                if mode == 'digest_suffix_8' and '--threads=8' in command:
                    rows[-2]['resolution_sha256'] = 'ab' * 31 + 'cd'
                elif mode == 'negative_work':
                    rows[1]['travail']['census_sites'] = -1
                elif mode == 'bool_work':
                    rows[1]['travail']['census_sites'] = False
                elif mode == 'exit_order_false':
                    rows[-1]['order'] = False
                return SimpleNamespace(returncode=0, stdout=('\n'.join(map(json.dumps, rows))+'\n').encode(), stderr=b'')
            judge.subprocess.run = fake_run
            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                code = judge.main(['g_determinism.py', sys.executable, 'audit_json', '--uniform=10,1,18',
                                   '--k=5', '--threads=1,8'])
            require(code == expected, 'code inattendu : ' + mode)
            results.append(dict(case=mode or 'controle', code=code, expected=expected,
                                output=output.getvalue().strip()))
    finally:
        judge.subprocess.run = old_subprocess_run
        if previous_mode is None:
            os.environ.pop('MHGP12_FAUSSE_SONDE', None)
        else:
            os.environ['MHGP12_FAUSSE_SONDE'] = previous_mode

    # Modele independant de la mise en octets: montre ce que le filtre CNTR -> COBJ exclut ou conserve.
    def column(tag, width, values):
        name = (tag + '_04').encode().ljust(8, b'\0')
        body = b''.join(value.to_bytes(width, 'little') for value in values)
        return name + struct.pack('<IIQ', width, 0, len(values)) + body + b'\0' * (-len(body) % 8)
    counters = [3, 1, 0, 0, 2] + [0] * 39
    def encoded(work=0, target=1, mask=3, objects=3, object_only=True):
        counts = list(counters)
        counts[0], counts[18] = objects, work
        fields = [('BKEY', 4, [0,1,2]), ('BRNK', 4, [0,1,2]), ('CBAL', 4, [4]), ('CRNK', 4, [5]),
                  ('CFLG', 1, [0]), ('COFF', 8, [0,2]), ('TMSK', 8, [mask,5]), ('TARG', 4, [target,2])]
        fields += [('COBJ', 8, counts[:5])] if object_only else [('CNTR', 8, counts)]
        return b''.join(column(*field) for field in fields)
    model = dict(work_excluded=encoded(work=0)==encoded(work=9),
                 work_retained_in_export=encoded(work=0,object_only=False)!=encoded(work=9,object_only=False),
                 target_retained=encoded(target=1)!=encoded(target=2),
                 mask_retained=encoded(mask=3)!=encoded(mask=6),
                 object_counter_retained=encoded(objects=3)!=encoded(objects=4))
    require(all(model.values()), 'modele de filtrage incorrect')

    # Temoin exact trouve dans la famille publique generic_11_11: aucun nuage prive, aucun natif.
    sys.path.insert(0, str(source / 'reference'))
    from hgp12_ref.constructive import Reference
    points = [(112,245,101),(664,38,692),(78,572,78),(41,278,136),
              (264,343,681),(457,904,832),(42,397,56),(338,811,241)]
    refs = [Reference(points,4,resolution=policy) for policy in ('v12_indices','v12_proches')]
    part, k, parent = (0,4,5,7), 4, 45
    ball = refs[0].balls[parent]
    require(ball.lo <= k <= ball.hi and refs[0].cell(ball,k)[0] != 'birth', 'pas une cellule de fenetre')
    chosen = tuple(x for x in part if x not in ball.inner)
    require(set(part) == set(ball.inner) | set(chosen) and set(chosen) <= set(ball.shell), 'trace hors cellule')
    mask = sum(1 << ball.shell.index(x) for x in chosen)
    require(refs[0]._separable(ball)(mask), 'trace non stricte')
    targets = [ref.resolve_v12(part,k) for ref in refs]
    require(targets == [('naissance',21),('naissance',29)], 'temoin modifie')
    orders = [ref.order(k) for ref in refs]
    same_full = all(getattr(orders[0],field) == getattr(orders[1],field)
                    for field in ('nodes','lower','core','cover','cuts'))
    require(same_full, 'ordres FULL differents')
    target_details = []
    for ref, target in zip(refs, targets):
        b = ref.balls[target[1]]
        target_details.append(dict(policy=ref.resolution, target=list(target), level=str(b.level),
                                   center=list(map(str,b.center))))
    result = dict(native_executed=False, sources_verified=len(capture['files']),
                  source_checks=dict(admission_ast_unchanged=admission_same, res_bin_default_code_unchanged=res_code_same),
                  judge_cases=results, binary_model=model,
                  policy_witness=dict(fixture='generic_11_11', points=points, K=4, k=k,
                                      parent_ball=parent, parent_level=str(ball.level), trace_internal=part,
                                      trace_positions=[refs[0].sites[refs[0].site_of[x]] for x in part],
                                      targets=target_details, full_order_equal=same_full))
    print(json.dumps(result,indent=2,sort_keys=True))


if __name__ == '__main__':
    main()
