"""Read the closed R2 archive only: hashes before imports, no native replay or LIVE claim."""
from datetime import datetime
import hashlib
import importlib.util
import json
from math import isfinite
from pathlib import Path, PurePosixPath
import re
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent
CAPTURE = '/tmp/mhgp10-condensation-cover-r2-20260930.oaS7whUg'
BIN = '/tmp/mhgp10-condensation-cover-r2-binaries-20260930.1rEhq1OC'
COMMIT = '8bb4618e5c6c7c2bc8a5d7a1c2e189a4249d0047'
COMPILER = '/usr/bin/x86_64-linux-gnu-g++-13'
PYTHON = '/home/codespace/.python/current/bin/python3'
COMPILER_SHA = '1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769'
EXPECTED_PINS = {
    "check.py": "2bca422f92e6480d6b2ca074ca7d17f08d1d9cbb6f3587446a67c1d65933d7e9",
    "historical/internal_k3.json": "a2af5e645bba1eb775a5ed6d659ccc14d1ddd17246ce4f0973f7170a10a47499",
    "historical/internal_k3.u32le": "fefcf177572348fea81ca630ae728ebd66ba674591769724be3920decf997a0c",
    "historical/receipt.json": "745633cd5246625215654d9d065cda577e6c6b97b97e7dabcf2257f72b945ae1",
    "materialize.py": "ccc0a52e0ef49d437317e880344f0c79985c27c3d25529b81b89b8bb90ac6d7e",
    "materialized.hpp": "2d23aeb5f30afceae4347cfbadb4fb2c72be343f15f7e1ff545251ce4488af13",
    "probe.cpp": "ef5f45815f796daff2d3af3d1ba95be98bdf9789950afe3a5022051a46a7534b",
    "record.py": "91a55cf9c2c7d91cdf28f98ce586271e520d08dac5a767c8ab406db5f3aaf8ee",
    "reference/frontier_core.py": "86ba984ff7986bdd44a5d53e9b2c7d3f2c0eb847764938cd28259d18439850b7",
    "reference/hgp10_ref.py": "2cb84ad549b1f8982e71f7757794d97b5eadab323fd22d17461da18b76914104",
    "source/core/reasons.def": "0679dd4d0b4df946aa79568fd42cab744cea2c6c39bf9a3bd92a7698921a797b",
    "source/core/status.hpp": "f6983f95f60195eb5b5f73f73d9cc97b48d694d0877efd226072d6240cf84038",
    "source/core/types.hpp": "716da6aa079e46d1f9fde16e19a777228ec472bb1756ad0d7fede4655125da4c",
    "source/head/head.cpp": "f583da400d00571a547989a46b1690f2bb093e9e068a01897078363a92674578",
    "source/head/head.hpp": "f2710f309a201bc1094dcb49af7acc418ca33b1887f8373d8934f965d072c5e9",
    "source/points/dendrogram.cpp": "44711c93473cc31b675b5da29c07645d7c8a7cf686b540681a3da7e7b7c673d4",
    "source/points/dendrogram.hpp": "3162508b979572a6a4da0b3f0c445b09baa5e2fd77ba826a8d57b25711c42d16"
}
BINARY_PINS = {
    BIN+'/cover_normal': '21c99fe65e8a90ed857201604eec12587fb8c6010c9306fc155891c404c49bd4',
    BIN+'/cover_ubsan': '7ffa23850c8201b259d989bd54ea95e511745b5e14fe71e6c226985d8f87237c',
}
STATUS = 'HISTORICAL_COVER_SCORE_DIFFERENCE_CONFIRMED'


def require(ok, message):
    if not ok:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def no_constant(token):
    raise ValueError('nonfinite JSON: '+token)


def finite_float(token):
    value = float(token)
    require(isfinite(value), 'overflowing JSON float')
    return value


def unique_object(pairs):
    out = {}
    for key, value in pairs:
        require(key not in out, 'duplicate JSON key: '+key)
        out[key] = value
    return out


def parse(text):
    return json.loads(text, parse_constant=no_constant, parse_float=finite_float,
                      object_pairs_hook=unique_object)


def read_json(name):
    return parse((ROOT/name).read_text())


def verify_hashes():
    pins = {}
    for line in (ROOT/'SHA256SUMS').read_text().splitlines():
        require(re.fullmatch(r'[0-9a-f]{64}  [^\r\n]+', line) is not None, 'manifest format')
        digest, name = line.split('  ', 1)
        path = PurePosixPath(name)
        require(not path.is_absolute() and '..' not in path.parts and str(path)==name
                and name!='SHA256SUMS' and name not in pins, 'manifest identity')
        target = ROOT/name
        require(target.is_file() and not target.is_symlink()
                and target.resolve().is_relative_to(ROOT), 'manifest path')
        require(sha(target)==digest, 'hash mismatch: '+name)
        pins[name] = digest
    actual = {str(p.relative_to(ROOT)) for p in ROOT.rglob('*') if p.is_file()}
    require(actual==set(pins)|{'SHA256SUMS'} and len(pins)==44, 'exact archive inventory')
    require(all(pins.get(name)==digest for name,digest in EXPECTED_PINS.items()),
            'fixed 17 source/evidence pins')
    return pins


def utc(value):
    result = datetime.fromisoformat(value)
    require(result.tzinfo is not None, 'UTC time lacks timezone')
    require(result.utcoffset().total_seconds()==0, 'capture timezone not UTC')
    return result


def expected_commands():
    result = [('compiler', [COMPILER, '--version'])]
    for optimized in (False, True):
        suffix = 'optimized' if optimized else 'normal'
        result.append(('materialize_'+suffix, [PYTHON,'-B']+(['-O'] if optimized else [])
                       +[CAPTURE+'/materialize.py']))
    for mode in ('normal','ubsan'):
        flags = ['-O2'] if mode=='normal' else ['-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all']
        argv = [COMPILER,'-std=c++20']+flags+['-Wall','-Wextra','-Wpedantic','-Werror',
                '-I'+CAPTURE+'/source','-I'+CAPTURE,CAPTURE+'/probe.cpp',
                CAPTURE+'/source/head/head.cpp',CAPTURE+'/source/points/dendrogram.cpp',
                '-o',BIN+'/cover_'+mode]
        result.append(('compile_cover_'+mode,argv))
        result.append(('native_cover_'+mode,[BIN+'/cover_'+mode]))
        for optimized in (False, True):
            suffix = 'optimized' if optimized else 'normal'
            argv = [PYTHON,'-B']+(['-O'] if optimized else [])+[
                CAPTURE+'/check.py',CAPTURE+'/native_cover_'+mode+'.stdout']
            result.append(('judge_cover_'+mode+'_'+suffix,argv))
    return result


def verify_receipt(pins):
    r = read_json('receipt.json')
    require(r['schema']=='mhgp10_historical_cover_condensation_r2_v1'
            and r['status']==STATUS and r['errors']==[] and r['source_commit']==COMMIT,
            'capture schema/status/commit')
    require(r['sources_before']==r['sources_after']==EXPECTED_PINS, 'capture source pins')
    before, after = read_json('sources_before.json'), read_json('sources_after.json')
    require(before['sha256']==after['sha256']==EXPECTED_PINS and
            before['source_frozen_utc']==r['source_frozen_utc'], 'source snapshot identity')
    freeze, close = utc(r['source_frozen_utc']), utc(r['closed_observation_utc'])
    require(freeze<=utc(after['closed_observation_utc'])<=close, 'snapshot chronology')
    require(r['compiler']==COMPILER and
            r['compiler_sha256_before']==r['compiler_sha256_after']==COMPILER_SHA, 'compiler archive pin')
    require(r['binary_hashes']=={name:dict(before=digest,after=digest)
                                 for name,digest in BINARY_PINS.items()}, 'binary archive pins')
    require(r['native_invocations']==2 and r['oracle_invocations']==4 and
            type(r['native_generator_invocations']) is int and r['native_generator_invocations']==0
            and r['GCP_used'] is False and r['production_sources_modified'] is False, 'execution scope')
    require(r['source_scope']=='exact Git head blobs; historical native cover translation + independent Gamma3; no new generator'
            and r['historical_export_origin']=='morsehgp3D_v10/receipts/development_frontier_precision_20260930/qualification/native_normal/internal_k3.json',
            'historical source authority')
    expected = expected_commands()
    require(len(r['commands'])==len(expected)==11, 'command floor')
    previous = freeze
    for row,(name,argv) in zip(r['commands'],expected):
        require(row['name']==name and row['argv']==argv, 'command identity/argv: '+name)
        require(type(row['exit_code']) is int and row['exit_code']==0 and
                type(row['expected_exit_code']) is int and row['expected_exit_code']==0,
                'command rc: '+name)
        start,end = utc(row['started_utc']),utc(row['ended_utc'])
        require(previous<=start<=end<=close, 'command chronology: '+name)
        previous = end
        require(type(row['wall_seconds']) in (int,float) and isfinite(row['wall_seconds'])
                and row['wall_seconds']>=0, 'command duration: '+name)
        for stream in ('stdout','stderr'):
            path = name+'.'+stream
            require(row[stream+'_sha256']==pins[path], 'stream hash: '+path)
        require((ROOT/(name+'.stderr')).read_bytes()==b'', 'unexpected stderr: '+name)
        if name.startswith('compile_'):
            require((ROOT/(name+'.stdout')).read_bytes()==b'', 'compile diagnostics')
    require((ROOT/'compiler.stdout').read_text().splitlines()[0]==
            'x86_64-linux-gnu-g++-13 (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0', 'compiler stdout')
    return r


def verify_historical():
    h = read_json('historical/receipt.json')
    raw = read_json('historical/internal_k3.json')
    require(h['status']=='PASS' and h['GCP_used'] is False
            and h['profile']=='quantized_u18_input_only'
            and h['engine_binary_pin']=='post-build SHA only; full source closure is external',
            'historical scope')
    require(h['sources_before']==h['sources_after'], 'historical source snapshots')
    prefix = '/workspaces/E-HGP/build/v10-rank-search-dev-20260930.UYrPqDuW/'
    for name,digest in (
        ('final_source/reference/hgp10_ref.py',EXPECTED_PINS['reference/hgp10_ref.py']),
        ('native_foundations/frontier_core.py',EXPECTED_PINS['reference/frontier_core.py']),
        ('native_foundations/export_frontier_final','3b6820fb5d6fc07b0cccbe33b99f0e4e6a4aa2c8264a983571703bc833e43972')):
        require(h['sources_before'].get(prefix+name)==digest, 'historical reference/binary pin')
    rows = [row for row in h['clouds'] if row['case']=='internal_k3']
    require(len(rows)==1, 'historical cloud identity')
    row = rows[0]
    argv = [prefix+'native_foundations/export_frontier_final',
            prefix+'closed_results/native_normal/internal_k3.u32le','--k=3',
            '--out='+prefix+'closed_results/native_normal/internal_k3.json','--threads=1']
    require(row['export_argv']==argv and type(row['returncode']) is int and row['returncode']==0
            and row['stderr']=='' and row['K']==3 and row['n']==6
            and row['export_sha256']==EXPECTED_PINS['historical/internal_k3.json']
            and row['cloud_sha256']==EXPECTED_PINS['historical/internal_k3.u32le'], 'historical invocation pin')
    status = parse(row['stdout'])
    require(status['status']=='ok' and status['K']==3 and status['n_points']==status['n_sites']==6
            and status['orders_built']=='only_order' and status['nodes']==6
            and status['balls']==14 and status['levels']==10 and status['threads']==1,
            'historical invocation stdout')
    require(raw['engine_commit']=='d679ae29d-plus-rank-fix-final' and raw['K']==3
            and raw['coordinate_bits']==18 and raw['n_points']==raw['n_sites']==6
            and raw['orders_built']=='only_order', 'historical export metadata')
    return raw


def load(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def verify():
    pins = verify_hashes()  # All archived sources checked BEFORE any archived import.
    verify_receipt(pins)
    verify_historical()
    materialize = load('frozen_materialize_r2',ROOT/'materialize.py')
    proof = materialize.prove()
    require(proof['status']=='EXACT_HISTORICAL_COVER_MATERIALIZATION' and
            (proof['K'],proof['n'],proof['vertices'],proof['unions'],proof['cuts'],proof['first_cover_checks'])==
            (3,6,20,15,28,6) and proof['point_ids']==[3,4,5,1,0,2]
            and proof['point_alpha']==['13','13/2','13/2','13/2','25','13']
            and proof['late_point_id']==0 and proof['late_root_beta']=='169/9'
            and proof['late_point_entry']=='25' and proof['native_generator_invocations']==0,
            'materialization nonvacuity')
    require(proof['header']==(ROOT/'materialized.hpp').read_text(), 'materialized header identity')
    for suffix in ('normal','optimized'):
        require(read_json('materialize_'+suffix+'.stdout')==proof, 'archived materialization replay')
    require((ROOT/'native_cover_normal.stdout').read_bytes()==
            (ROOT/'native_cover_ubsan.stdout').read_bytes(), 'normal/UBSan archived native identity')
    judge = load('frozen_cover_judge_r2',ROOT/'check.py')
    for mode in ('normal','ubsan'):
        path = ROOT/('native_cover_'+mode+'.stdout')
        for line in path.read_text().splitlines():
            parse(line)  # Nonfinite/duplicate JSON prohibited before the pinned bounded judge.
        result = judge.judge(path)
        require(result['status']==STATUS and result['rows']==12 and
                result['exact_positive_controls']==8 and result['differing_rows']==4 and result['EOM_flips']==0
                and result['native_generator_invocations']==0, 'native/judge nonvacuity')
        for suffix in ('normal','optimized'):
            require(read_json('judge_cover_'+mode+'_'+suffix+'.stdout')==result,
                    'archived judgment replay')
    require(verify_hashes()==pins, 'archive changed while reading')
    return dict(status='ARCHIVE_PASS',scope='archived executions rejudged; no native, generator, compiler, or LIVE replay',
                manifest_entries=len(pins),source_pins=17,captured_commands=11,
                archived_native_invocations=2,archived_judge_invocations=4,
                native_rows_per_mode=12,positive_controls_per_mode=8,
                differing_rows_per_mode=4,EOM_flips_per_mode=0,native_generator_invocations=0,
                GCP_used=False)


if __name__=='__main__':
    require(len(sys.argv)==1, 'usage: verify.py (archive only)')
    print(json.dumps(verify(),sort_keys=True))

