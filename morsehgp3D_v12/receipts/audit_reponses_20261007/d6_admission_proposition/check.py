#!/usr/bin/env python3
"""Rejeu JSON/fichiers publics : git epingle, patch temporaire, aucun build ni moteur HGP."""
import argparse
import contextlib
import copy
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile
import types

HERE = Path(__file__).resolve().parent
SOURCE = 'morsehgp3D_v12/microbancs/mes_d6_profils/pilote_d6.py'
BASE = 'morsehgp3D_v12/'
DIGEST = 'a' * 64


def load(data, name):
    module = types.ModuleType(name)
    exec(compile(data, name, 'exec'), module.__dict__)
    return module


def fields(cpp, begin, end):
    part = cpp[cpp.index(begin):cpp.index(end, cpp.index(begin))]
    return re.findall(r'\\"([a-z0-9_]+)\\"', part)


def proof(repo):
    pins = json.loads((HERE / 'sources.json').read_text())
    sources = {}
    for item in pins['inputs']:
        data = subprocess.check_output(['git', '-C', str(repo), 'show', pins['pin'] + ':' + item['path']])
        if len(data) != item['bytes'] or hashlib.sha256(data).hexdigest() != item['sha256']:
            raise ValueError('source differente : ' + item['path'])
        sources[item['path']] = data
    before = load(sources[SOURCE], 'd6_before')
    with tempfile.TemporaryDirectory(prefix='d6_admission_') as directory:
        root = Path(directory)
        target = root / SOURCE
        target.parent.mkdir(parents=True)
        target.write_bytes(sources[SOURCE])
        for extra in (['--check'], []):
            subprocess.run(['git', 'apply', *extra, str(HERE / 'proposition.patch')], cwd=root, check=True)
        proposed_bytes = target.read_bytes()
        if hashlib.sha256(proposed_bytes).hexdigest() != pins['proposed_sha256']:
            raise ValueError('patch different')
        after = load(proposed_bytes, 'd6_proposed')
        archived = load(sources[BASE + 'receipts/audit_d6_20261007/math/check.py'], 'archived_six')
        archived_before = archived.proof(before)
        # Les appels directs historiques ne portaient pas encore la configuration demandee.
        strict_summary = after.summarize
        expected2 = dict(passes=2, coord_bits=21, kmax=2, threads=1, leaf=24)
        after.summarize = lambda code, rows, phase, key, *args: strict_summary(
            code, rows, phase, key, *(args or [expected2]))
        archived_after = archived.proof(after)
        after.summarize = strict_summary
        archived_names = ('schema_success_two_passes', 'explicit_refusal_code_zero', 'bool_wall_and_bool_digest',
                          'dilation_changed_digest_same_counts', 'take_P5_two_passes_no_orders_invalid_json_invalid_export')
        if not all(archived_before[name]['controle_conforme'] for name in archived_names) or \
                archived_before['main_small_doubles'] != dict(code=0, prises=3, controles=[]) or \
                any(archived_after[name]['controle_conforme'] for name in archived_names) or \
                archived_after['main_small_doubles']['code'] != 1:
            raise ValueError('six observations historiques non reproduites')

        # Fabrique tiree des champs C++ epingles, pas des constantes du patch.
        cpp = sources[BASE + 'bench/catalogue_probe.cpp'].decode()
        ledger = fields(cpp, 'void ledger_json(', 'void diagnostics_json(')
        cat_diag = fields(cpp, 'void diagnostics_json(', '// Nuage synthetique')
        tower = sources[BASE + 'bench/tower_probe.cpp'].decode()
        gdiag = fields(tower, 'if (outcome.ok()) {', 'for (int k = 1;')
        export = sources[BASE + 'bench/tower_export.hpp'].decode()
        part = export.split('kCounterNames[kScalarCounters] = {', 1)[1].split('};', 1)[0]
        names = re.findall(r'"([a-z0-9_]+)"', part)
        if len(ledger) != 21 or len(cat_diag) != 19 or gdiag != ['diagnostics', 'count_ns', 'fill_ns',
                'tables_ns', 'resolve_ns', 'workspace_bytes', 'table_bytes', 'peak_bytes', 'order_ns'] or len(names) != 28:
            raise ValueError('schema C++ non reconnu')
        # Les premiers noms des printf sont les noms des sous-objets.
        ledger, cat_diag = ledger[1:], cat_diag[1:]

        def stream(phase, count=5, bits=21, kmax=2, threads=1, sites=3):
            rows = []
            for number in range(count):
                row = dict(phase=phase, status='ok', reason='none', coord_bits=bits, kmax=kmax,
                           threads=threads, sites=sites, wall_ns=10, **{'pass': number})
                if phase == 'catalogue':
                    row.update(leaf=24, balls=3, incidences=5, levels=2,
                               ledger=dict.fromkeys(ledger, 0), diagnostics=dict.fromkeys(cat_diag, 0))
                else:
                    row.update(order=0, diagnostics={**dict.fromkeys(gdiag[1:-1], 0), 'order_ns': [0] * kmax})
                rows.append(row)
                if phase == 'catalogue':
                    rows.append(dict(phase='digest', catalogue_sha256=DIGEST))
            if phase == 'tour_g':
                for k in range(1, min(kmax, sites) + 1):
                    rows.append(dict(phase='ordre', k=k, objet={**dict.fromkeys(names[:5], 0), 'births': sites if k == 1 else 1},
                                     travail={**dict.fromkeys(names[5:], 0), 'chaines': [0] * 16}))
                rows.append(dict(phase='digest', resolution_sha256=DIGEST))
            rows.append(dict(phase='exit', status='ok', reason='none', **({'order': 0} if phase == 'tour_g' else {})))
            return rows

        expected = dict(passes=5, coord_bits=21, kmax=2, threads=1, leaf=24)
        rows_g, rows_c = stream('tour_g'), stream('catalogue')
        cases = []

        def compare(name, rows, phase='tour_g', config=None, want=False, code=0):
            key = 'resolution_sha256' if phase == 'tour_g' else 'catalogue_sha256'
            old = before.summarize(code, rows, phase, key)
            new = after.summarize(code, rows, phase, key, config or expected)
            cases.append(dict(name=name, before=old['ok'], proposed=new['ok'], expected=want))
            if new['ok'] != want or (not want and new['chaud_ms'] is not None):
                raise ValueError('verdict incorrect : ' + name)

        compare('complete_G_P5_final_digest_and_exit', rows_g, want=True)
        compare('complete_catalogue_P5', rows_c, phase='catalogue', want=True)
        for bits in (24, 32):
            compare('complete_G_u%d' % bits, stream('tour_g', bits=bits), config={**expected, 'coord_bits': bits}, want=True)
        compare('complete_G_K5_sites3_three_orders', stream('tour_g', kmax=5),
                config={**expected, 'kmax': 5}, want=True)
        for field, value in [('pass', True), ('coord_bits', 24), ('coord_bits', True), ('kmax', 3),
                             ('threads', 2), ('threads', True), ('kmax', True), ('sites', 0), ('sites', True), ('wall_ns', True),
                             ('wall_ns', 0.5), ('wall_ns', -1), ('wall_ns', 1 << 64), ('order', True),
                             ('status', 'resource_exhausted'), ('reason', 'memory_budget')]:
            rows = copy.deepcopy(rows_g)
            rows[0][field] = value
            compare('G_stage_%s_%s' % (field, value), rows)
        for value in (False, 0.0, 1):
            rows = copy.deepcopy(rows_g); rows[-1]['order'] = value
            compare('G_exit_order_%s' % value, rows)
        for value in (True, 'a' * 16, 'G' * 64):
            rows = copy.deepcopy(rows_g); rows[-2]['resolution_sha256'] = value
            compare('G_digest_' + str(value), rows)
        for field in ('passes', 'coord_bits', 'kmax', 'threads', 'leaf'):
            compare('config_bool_' + field, rows_g, config={**expected, field: True})
        rows = copy.deepcopy(rows_g); rows[1]['pass'] = 0
        compare('G_duplicate_pass_index', rows)
        mutations = {
            'G_missing_exit': rows_g[:-1], 'G_duplicate_exit': rows_g + [rows_g[-1]],
            'G_no_orders': [r for r in rows_g if r['phase'] != 'ordre'],
            'G_duplicate_order': rows_g[:6] + [rows_g[5]] + rows_g[6:],
            'G_reversed_orders': rows_g[:5] + list(reversed(rows_g[5:7])) + rows_g[7:],
            'G_repeated_final_digest': rows_g[:-1] + [rows_g[-2], rows_g[-1]],
            'G_digest_after_each_pass': [r for row in rows_g[:5] for r in (row, rows_g[-2])] + rows_g[5:],
            'G_two_passes_for_P5': stream('tour_g', count=2),
            'G_six_passes_for_P5': stream('tour_g', count=6),
            'G_unknown_phase': rows_g[:-1] + [{'phase': 'unknown'}] + rows_g[-1:],
        }
        for name, rows in mutations.items():
            compare(name, rows)
        for selector, field, value in [(5, 'k', True), (5, 'k', 0)]:
            rows = copy.deepcopy(rows_g); rows[selector][field] = value
            compare('G_order_%s_%s' % (field, value), rows)
        rows = copy.deepcopy(rows_g); rows[5]['objet']['births'] = True
        compare('G_bool_object_counter', rows)
        rows = copy.deepcopy(rows_g); rows[5]['travail']['chaines'] = [0] * 15
        compare('G_short_histogram', rows)
        rows = copy.deepcopy(rows_g); rows[0]['diagnostics']['order_ns'] = [0]
        compare('G_short_order_diagnostics', rows)
        rows = copy.deepcopy(rows_g); rows[1]['sites'] = 4
        compare('G_sites_changed_between_passes', rows)
        for name, rows in [('cat_missing_digest', [r for i, r in enumerate(rows_c) if i != 1]),
                           ('cat_swapped_digest_and_stage', [rows_c[1], rows_c[0]] + rows_c[2:])]:
            compare(name, rows, phase='catalogue')
        rows = copy.deepcopy(rows_c); rows[3]['catalogue_sha256'] = 'b' * 64
        compare('cat_digest_changed_between_passes', rows, phase='catalogue')
        rows = copy.deepcopy(rows_c); rows[0]['balls'] = True
        compare('cat_bool_count', rows, phase='catalogue')
        rows = copy.deepcopy(rows_c); rows[0]['leaf'] = 32
        compare('cat_wrong_leaf', rows, phase='catalogue')
        compare('process_nonzero', rows_g, code=2)

        # JSON decoding: run/take remain real; executable doubles only print public strings.
        raw_cases = []
        encoded = '\n'.join(json.dumps(r) for r in rows_g).encode() + b'\n'
        streams = {'valid': encoded, 'invalid_line': b'NOT_JSON\n' + encoded,
                   'non_object': b'[]\n' + encoded, 'duplicate_key': encoded.replace(b'"pass": 0', b'"pass": 0, "pass": 0', 1),
                   'invalid_utf8': b'\xff\n' + encoded,
                   'NaN': encoded.replace(b'"wall_ns": 10', b'"wall_ns": NaN', 1)}
        for name, data in streams.items():
            command = [sys.executable, '-c', 'import sys; sys.stdout.buffer.write(' + repr(data) + ')']
            code, rows = after.run(command, 10, str(root / 'raw.jsonl'))
            okay = after.summarize(code, rows, 'tour_g', 'resolution_sha256', expected)['ok']
            raw_cases.append(dict(name=name, proposed=okay, expected=name == 'valid', raw_preserved=(root/'raw.jsonl').read_bytes()==data))
            if okay != (name == 'valid') or not raw_cases[-1]['raw_preserved']:
                raise ValueError('decodage incorrect : ' + name)

        def body(bits=21, frame='public_fixture'):
            header = b'MHGP12DP' + struct.pack('<6IQ', 1, 1, bits, 2, 0, 5, 3) + frame.encode().ljust(24, b'\0')
            sections = []
            # Valeurs publiques de format, PAS un catalogue dont la geometrie est certifiee.
            for tag, size, count in [('SITEXYZ', 12, 3), ('BALLS', 32, 3), ('POPOFF', 8, 4), ('POPVAL', 4, 5), ('NLEVELS', 8, 1)]:
                values = struct.pack('<Q', 2) if tag == 'NLEVELS' else b'\0' * (size * count)
                sections.append(tag.encode().ljust(8, b'\0') + struct.pack('<IIQ', size, 0, count) + values + b'\0' * (-len(values) % 8))
            return header + b''.join(sections)

        data = body()
        meta = dict(coord_bits=21, kmax=2, sites=3, frame='public_fixture', balls=3, incidences=5, levels=2,
                    sha256=hashlib.sha256(data).hexdigest(), bytes=len(data))
        dump = root / 'cat.bin'
        dump.write_bytes(data)
        reader = load(sources[BASE + 'tests/catalogue/catalogue_dump.py'], 'existing_format_reader')
        parsed = reader.read_dump(str(dump))
        if parsed['sites'] != 3 or parsed['bits'] != 21 or parsed['kmax'] != 2:
            raise ValueError('fabrique incompatible lecteur existant')
        body_cases = []
        def body_compare(name, data, expected=None, want=False):
            dump.write_bytes(data)
            old = before.body_sha(str(dump))
            new = after.body_sha(str(dump), meta if expected is None else expected)
            body_cases.append(dict(name=name, before=old is not None, proposed=new is not None, expected=want))
            if (new is not None) != want or (want and new != hashlib.sha256(data[64:]).hexdigest()):
                raise ValueError('export incorrect : ' + name)
        body_compare('MHGP12DP_v1_producer_layout', data, want=True)
        body_compare('old_64bytes_X', b'X' * 64)
        body_compare('valid_header_without_sections', data[:64])
        body_compare('truncated_payload', data[:-1])
        body_compare('extra_EOF_byte', data+b'X')
        for name, offset, new in [('magic',0,b'X'),('version',8,struct.pack('<I',2)),('kind',12,struct.pack('<I',4)),
                                 ('bits',16,struct.pack('<I',24)),('K',20,struct.pack('<I',3)),
                                 ('order',24,struct.pack('<I',1)),('sections',28,struct.pack('<I',4)),
                                 ('frame',40,b'X'),('tag',64,b'X'),('element_size',72,struct.pack('<I',8)),
                                 ('reserved',76,struct.pack('<I',1)),('sites_count',80,struct.pack('<Q',4)),
                                 ('padding',124,b'X')]:
            mutant=data[:offset]+new+data[offset+len(new):]
            body_compare(name,mutant,expected={**meta,'sha256':hashlib.sha256(mutant).hexdigest()})
        body_compare('wrong_full_hash', data, expected={**meta,'sha256': 'b'*64})
        body_compare('wrong_reported_bytes', data, expected={**meta,'bytes':len(data)+8})
        body_compare('wrong_json_ball_count', data, expected={**meta,'balls':4})

        # Vrai take : P5, export coherent, G digest final et exit avec order=0.
        bins=root/'faithful_bin'; bins.mkdir()
        faithful_cat=copy.deepcopy(rows_c)
        for row in faithful_cat:
            if row['phase']=='digest':row['catalogue_sha256']=meta['sha256']
        faithful_cat.insert(-1, dict(phase='export',cat_bin_sha256=meta['sha256'],bytes=len(data)))
        for name,payload in [('mhgp12_catalogue_probe',faithful_cat),('mhgp12_tower_probe',rows_g)]:
            script='#!'+sys.executable+'\nimport json,pathlib,sys\n'
            script+='for arg in sys.argv:\n if arg.startswith("--out="):\n  p=pathlib.Path(arg[6:]);p.mkdir(parents=True)\n  (p/"cat.bin").write_bytes('+repr(data)+')\n'
            script+='for row in '+repr(payload)+': print(json.dumps(row))\n'
            (bins/name).write_text(script);(bins/name).chmod(0o700)
        xyz,ids=root/'public.u32le',root/'public.ids.u32le'
        xyz.write_bytes(struct.pack('<9I',0,0,0,1,0,0,2,0,0));ids.write_bytes(struct.pack('<3I',0,1,2))
        args=types.SimpleNamespace(k=2,fils=1,passes=5,case_now='public_fixture',racine=str(root),delai=10)
        value=after.take({21:str(bins)},{1:(str(xyz),str(ids),2)},(21,1),args,str(root),0)
        faithful_take=dict(catalogue_ok=value['catalogue']['ok'],tour_g_ok=value['tour_g']['ok'],
                           controles=after.checks([value]),passes=len(value['tour_g']['passes_ms']),
                           body_sha256=value['catalogue']['corps_sha256'])
        if not faithful_take['catalogue_ok'] or not faithful_take['tour_g_ok'] or faithful_take['controles']:
            raise ValueError('vrai take nominal refuse')
        scaled=copy.deepcopy(value);scaled['facteur']=8;scaled['tour_g']['empreinte']='b'*64
        semantic_residual=dict(dilation_changed_digest_same_counts_admitted=not after.checks([value,scaled]),
                               geometry_tested=False)
        if not semantic_residual['dilation_changed_digest_same_counts_admitted']:
            raise ValueError('portee du correctif modifiee')
        return dict(pin=pins['pin'],proposed_sha256=pins['proposed_sha256'],archived_six_before=archived_before,
                    archived_six_proposed_with_expected=archived_after,producer_protocol=cases,strict_json=raw_cases,
                    structure_only=body_cases,faithful_take=faithful_take,semantic_residual=semantic_residual,
                    native_engine_runs=0,builds=0,cloud_calls=0)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo',type=Path,default=Path.cwd())
    parser.add_argument('--out',type=Path)
    args=parser.parse_args()
    result=proof(args.repo)
    encoded=json.dumps(result,indent=1,sort_keys=True)+'\n'
    if args.out:args.out.write_text(encoded)
    else:sys.stdout.write(encoded)
    return 0


if __name__=='__main__':
    sys.exit(main())
