#!/usr/bin/env python3
"""Temoins du plan D6 : build/take simules, preparation et jugement reels. Aucun moteur."""
import contextlib
import difflib
import hashlib
import io
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import types

PIN = '9b2747eff364d56215b589c782b1a4e51d59a576'
SOURCE = 'morsehgp3D_v12/microbancs/mes_d6_profils/pilote_d6.py'
HERE = Path(__file__).resolve().parent


def proposal(source):
    before = "    os.makedirs(os.path.join(args.sortie, 'brut'), exist_ok=True)"
    after = """    if len(cases) != len(set(cases)) or len(profiles) != len(set(profiles)) or args.jobs < 1 or args.delai < 1:
        return 2
""" + before
    if source.count(before) != 1:
        raise ValueError('point de remplacement ambigu')
    source = source.replace(before, after)
    before = """        except OSError:
            return 2
        combos ="""
    after = """        except (OSError, ValueError, OverflowError):
            return 2
        # La comparaison D6 exige sa reference (21, x1) pour chaque trame.
        if inputs[1][0] is None or inputs[1][2] >= 1 << 21:
            return 2
        combos ="""
    if source.count(before) != 1:
        raise ValueError('point de remplacement ambigu')
    return source.replace(before, after)


def witness(source, spec):
    module = types.ModuleType('d6_test')
    exec(compile(source, SOURCE, 'exec'), module.__dict__)
    tags = []
    module.build = lambda src, root, bits, jobs: root

    def fake_take(binaries, inputs, combo, args, raw_dir, round_index):
        bits, factor = combo
        tag = '%s_p%d_x%d_t%d' % (args.case_now, bits, factor, round_index)
        tags.append(tag)
        # Ces chiffres et empreintes sont inventes ; on teste le plan, pas summarize.
        stage = dict(code=0, ok=True, chaud_ms=1.0, empreinte='a' * 64,
                     corps_sha256='b' * 64, boules=1, incidences=2, objet={})
        return dict(cas=args.case_now, profil=bits, facteur=factor, tour=round_index,
                    catalogue=dict(stage), tour_g=dict(stage))

    module.take = fake_take
    with tempfile.TemporaryDirectory(prefix='audit-d6-plan-') as temp:
        root = Path(temp)
        (root / 'lidar_test.u32le').write_bytes(struct.pack('<6I', 0, 0, 0, spec['largest'], 0, 0))
        (root / 'lidar_test.ids.u32le').write_bytes(struct.pack('<2I', 0, 1))
        argv = ['pilote_d6.py', '--src', str(root), '--racine', str(root / 'build'),
                '--donnees', str(root), '--sortie', str(root / 'out'), '--cas', spec.get('cases', 'test'),
                '--profils', spec.get('profiles', '21,24'), '--passes', '2', '--tours', '1']
        try:
            with contextlib.redirect_stdout(io.StringIO()):
                code = module.main(argv)
        except Exception as error:
            code = type(error).__name__
        path = root / 'out/mes_d6.json'
        report = json.loads(path.read_text()) if path.exists() else {}
        entries = report.get('prises', [])
        return dict(code=code, takes=len(entries), duplicate_tags=len(tags) - len(set(tags)),
                    baseline_takes=sum(e['profil'] == 21 and e['facteur'] == 1 for e in entries),
                    issues=report.get('controles'),
                    ratios=[r['catalogue']['rapport'] for r in report.get('rapports', [])])


def main():
    source = subprocess.check_output(['git', 'show', PIN + ':' + SOURCE], text=True)
    changed = proposal(source)
    cases = [dict(name='reference_present', largest=1),
             dict(name='u21_absent_u24_present', largest=1 << 21),
             dict(name='all_combinations_absent', largest=1 << 21, profiles='21'),
             dict(name='duplicate_profile', largest=1, profiles='21,21,24'),
             dict(name='duplicate_case', largest=1, cases='test,test')]
    results = [dict(case=spec['name'], before=witness(source, spec), after=witness(changed, spec)) for spec in cases]
    expected = [0, 2, 2, 2, 2]
    if [r['after']['code'] for r in results] != expected:
        raise RuntimeError('proposition non conforme')
    patch = ''.join(difflib.unified_diff(source.splitlines(True), changed.splitlines(True),
                                       fromfile='a/' + SOURCE, tofile='b/' + SOURCE))
    (HERE / 'proposition.patch').write_text(patch)
    print(json.dumps(dict(pin=PIN, source_sha256=hashlib.sha256(source.encode()).hexdigest(),
                          proposed_sha256=hashlib.sha256(changed.encode()).hexdigest(),
                          scope='main instrumente : build et take simules ; aucun calcul HGP',
                          cases=results), indent=2, sort_keys=True))


if __name__ == '__main__':
    main()
