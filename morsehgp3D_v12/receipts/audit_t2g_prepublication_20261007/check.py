#!/usr/bin/env python3
"""Contre-epreuves de g_determinism : minuscules doubles stdout JSON, aucune execution native.

La derniere sortie egale aussi la ligne attendue du wrapper CTest synth_u8000_k5.
Les nombres reproduits sont ceux de cette ligne, jamais des mesures nouvelles.
"""
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
OBJECT = ('births', 'cells', 'inert_cells', 'extended_cells', 'representatives')
WORK = ('probes', 'first_probe_hits', 'probe_hits_after_steps', 'route_t1', 'route_cert_table',
        'route_cert_census', 'route_fallback_table', 'route_fallback_census', 'fallback_no_proposal',
        'fallback_not_in_part', 'fallback_certificate', 'census_saturated', 'census_complete',
        'census_sites', 'census_sites_max', 'census_nodes', 'jumps_catalogue', 'jumps_census',
        'inert_steps', 'cell_stops', 'birth_stops', 'controls', 'max_chain')


def need(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def order(k, sites):
    obj = dict.fromkeys(OBJECT, 0)
    obj['births'] = sites if k == 1 else 1
    work = dict.fromkeys(WORK, 0)
    work['chaines'] = [0]*16
    return dict(phase='ordre', k=k, objet=obj, travail=work)


def payload(rows, sites=10, digest='0'*64):
    return [dict(phase='tour_g', pass_index=0, status='ok', reason='none', order=0, coord_bits=21,
                 kmax=5, threads=1, sites=sites, wall_ns=0), *copy.deepcopy(rows),
            dict(phase='digest', resolution_sha256=digest)]


def main():
    paths = [HERE/'check.py', HERE/'sources.json', HERE/'g_determinism.snapshot.py', HERE/'tests.cmake.snapshot']
    hashes = {p.name: sha(p.read_bytes()) for p in paths}
    meta = json.loads((HERE/'sources.json').read_text())
    need(hashes['g_determinism.snapshot.py'] ==
         meta['files_sha256']['morsehgp3D_v12/tests/tower/g_determinism.py'], 'snapshot incorrect')
    need(hashes['tests.cmake.snapshot'] == meta['files_sha256']['morsehgp3D_v12/tests/tower/tests.cmake'],
         'wrapper different')
    good = [order(k, 10) for k in range(1, 6)]
    cases = [('temoin_schema_complet', payload(good)), ('seul_k1', payload(good[:1])),
             ('k3_absent', payload([row for row in good if row['k'] != 3])),
             ('k2_duplique_k3_absent', payload([good[0], good[1], good[1], good[3], good[4]])),
             ('digest_non_hexadecimal', payload(good, digest='x'*64)),
             ('digest_liste', payload(good, digest=[]))]
    cmake = (HERE/'tests.cmake.snapshot').read_text()
    pinned = re.search(r'set\(tower_scale_8000 "([^\"]+)"\)', cmake).group(1)
    totals = dict(re.findall(r'(naissances|cellules|representants|cibles_cellule)=(\d+)', pinned))
    prefix = re.search(r'empreinte=([0-9a-f]+)', pinned).group(1)
    truncated = order(1, 8000)
    truncated['objet'].update(births=int(totals['naissances']), cells=int(totals['cellules']),
                              representatives=int(totals['representants']))
    truncated['travail']['cell_stops'] = int(totals['cibles_cellule'])
    cases.append(('k1_tronque_totaux_wrapper', payload([truncated], sites=8000, digest=prefix+'0'*(64-len(prefix)))))
    observations = []
    python = [sys.executable, '-B', '-S'] + (['-O'] if sys.flags.optimize else [])
    with tempfile.TemporaryDirectory(prefix='audit-g-json-') as tmp:
        folder = Path(tmp)
        probe = folder/'sonde_json.py'
        # Ce double ne calcule rien : il rend le JSON fourni en reprenant seulement le nombre de fils demande.
        stub = '#!'+sys.executable+'\nimport json,pathlib,sys\n'
        stub += "rows=json.loads(pathlib.Path(__file__).with_suffix('.json').read_text())\n"
        stub += "threads=int(next(s[10:] for s in sys.argv if s.startswith('--threads=')))\n"
        stub += "for row in rows:\n if row.get('phase')=='tour_g':\n  row['threads']=threads\n  row['pass']=row.pop('pass_index')\n print(json.dumps(row))\n"
        probe.write_text(stub)
        probe.chmod(0o700)
        for name, rows in cases:
            probe.with_suffix('.json').write_text(json.dumps(rows))
            last = name == 'k1_tronque_totaux_wrapper'
            case = 'synth_u8000_k5' if last else name
            n, threads = (8000, '1,8') if last else (10, '1,48')
            arguments = [str(HERE/'g_determinism.snapshot.py'), str(probe), case,
                         '--uniform=%d,20261007,18' % n, '--k=5', '--threads='+threads]
            process = subprocess.run(python+arguments, capture_output=True, text=True, timeout=10)
            need(process.returncode == 0 and process.stdout.startswith('g_determinism_ok '),
                 (name, process.returncode, process.stdout, process.stderr))
            observed = dict(cas=name, code=process.returncode, stdout=process.stdout.strip(),
                            stderr=process.stderr, ordres=[r['k'] for r in rows if r['phase']=='ordre'],
                            command=['python3', '-B', '-S', 'g_determinism.snapshot.py', '<sonde_json.py>']+arguments[2:],
                            json_input_sha256=sha(json.dumps(rows).encode()))
            if last:
                expected = 'g_determinism_ok cas=synth_u8000_k5 fils=1,8 '+pinned
                need(process.stdout.strip() == expected, ('wrapper mismatch', process.stdout, expected))
                observed.update(wrapper_exact_line=expected, wrapper_line_match=True,
                                births_k1=truncated['objet']['births'], sites_announced=8000)
            observations.append(observed)
    need(hashes == {p.name: sha(p.read_bytes()) for p in paths}, 'source modifiee')
    print(json.dumps(dict(source_sha256=hashes, observations=observations, stub_sha256=sha(stub.encode()),
                         native_execution=False, scope='validation du juge seulement; aucune prise moteur historique'),
                     ensure_ascii=False, sort_keys=True, indent=2))


if __name__ == '__main__':
    main()
