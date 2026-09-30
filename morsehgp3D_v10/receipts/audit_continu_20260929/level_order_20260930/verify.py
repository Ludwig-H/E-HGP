"""Hash before import, then replay closed comparator evidence read-only."""
import hashlib
import importlib.util
import json
from math import isfinite
from pathlib import Path
import shlex
import sys

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parent


def require(value,message):
    if not value:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    def unique(pairs):
        result = {}
        for key,value in pairs:
            require(key not in result,'duplicate JSON key')
            result[key] = value
        return result
    def constant(value):
        raise ValueError('nonfinite JSON constant')
    def finite(value):
        number = float(value)
        require(isfinite(number),'nonfinite JSON float')
        return number
    return json.loads(path.read_text(),object_pairs_hook=unique,parse_constant=constant,parse_float=finite)


def manifest(folder):
    names = set()
    for line in (folder/'SHA256SUMS').read_text().splitlines():
        digest,name = line.split('  ',1)
        path = folder/name
        require(len(digest)==64 and all(c in '0123456789abcdef' for c in digest),'SHA schema')
        require(not Path(name).is_absolute() and '..' not in Path(name).parts and
                name not in names and path.is_file() and not path.is_symlink(),'manifest path')
        require(sha(path)==digest,'SHA mismatch: '+name)
        names.add(name)
    require(names=={str(path.relative_to(folder)) for path in folder.rglob('*')
                    if path.is_file() and path!=folder/'SHA256SUMS'},'manifest incomplete')
    return len(names)


def module(name,path):
    spec = importlib.util.spec_from_file_location(name,path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def pins(record):
    required = {'inputs.py','judge.py','record.py','level_comparator.cpp','PROTOCOL.txt',
                'normal','ubsan','float_mutant','high_mutant'}
    names = [Path(path).name for path in record['before']]
    require(type(record['before']) is dict and record['before']==record['after'] and
            set(names)==required and len(names)==len(required),'required source/binary pins')


def independent():
    folder = ROOT/'independent'
    record = read(folder/'execution.json')
    require(record['status']=='PASS' and record['GCP_used'] is False and
            record['production_sources_modified'] is False and record['requests']==2444,'native scope')
    pins(record)
    prototype = read(ROOT/'prototype'/'receipt.json')
    binary_shas = {Path(path).name:digest for path,digest in
                   prototype['provenance']['external_binary_hashes_after_relocation'].items()}
    for original,digest in record['before'].items():
        name = Path(original).name
        if name in binary_shas:
            require(digest==binary_shas[name],'binary relocation differs')
        else:
            path = ROOT/'prototype'/name if name in ('level_comparator.cpp','PROTOCOL.txt') else folder/name
            require(sha(path)==digest,'source pin differs')
    inputs = module('closed_level_inputs',folder/'inputs.py')
    judge = module('closed_level_judge',folder/'judge.py')
    requests = read(folder/'requests.json')
    require(requests==inputs.panel(),'requests not regenerated')
    require((folder/'requests.stdin').read_text()=='\n'.join(map(inputs.line,requests))+'\n',
            'serialized requests differ')
    commands = {row['name']:row for row in record['commands']}
    require(len(commands)==len(record['commands'])==12,'native/judge command coverage')
    expected = set()
    failures = {'float_mutant':'wrong exact comparison at 3','high_mutant':'lost high product word at 2178'}
    for case in ('normal','ubsan','float_mutant','high_mutant'):
        expected.add(case)
        require(commands[case]['exit_code']==0 and commands[case]['expected_exit_code']==0 and
                (folder/(case+'.stderr')).read_text()=='','native exit/diagnostic')
        answers = [judge.load(line) for line in (folder/(case+'.stdout')).read_text().splitlines()]
        try:
            actual = judge.judge(requests,answers)
            rc = 0
        except ValueError as error:
            actual,rc = dict(status='FAIL',error=str(error)),1
        if case in failures:
            require(rc==1 and actual['error']==failures[case],'mutant causal failure')
        else:
            require(rc==0 and actual['counts']['geometric_comparisons']==729 and
                    actual['counts']['domain_refusals']==6 and actual['counts']['raw_overflow_refusals']==14,
                    'positive panel')
        for mode in ('normal','optimized'):
            name = 'judge_'+case+'_'+mode
            expected.add(name)
            require(commands[name]['exit_code']==commands[name]['expected_exit_code']==rc and
                    read(folder/(name+'.stdout'))==actual and (folder/(name+'.stderr')).read_text()=='',
                    'archived judgment differs')
    require(set(commands)==expected,'paired commands missing')
    require((folder/'normal.stdout').read_bytes()==(folder/'ubsan.stdout').read_bytes(),'normal/UBSan differ')
    # Cheap causal structural controls; no native execution is repeated.
    empty = dict(record,before={},after={})
    try:
        pins(empty)
    except ValueError:
        pass
    else:
        raise ValueError('empty pins survived')
    return dict(unique_requests=2444,C=2172,M=272,geometric_levels=27,geometric_pairs=729,
                native_invocations=4,archived_judgments=8,domain_refusals=6,raw_overflow_refusals=14,
                mutants=failures)


def source_port():
    folder = ROOT/'independent'/'source_port'
    record = read(folder/'receipt.json')
    required = {'morsehgp3D_v10/src/'+path for path in
                ('arith/geometry.cpp','arith/geometry.hpp','arith/wide.hpp','catalogue/generator.cpp',
                 'core/types.hpp','sched/sort.hpp','tower/tower.cpp','tower/tower.hpp')}
    require(record['status']=='PASS' and record['native_executions']==0 and record['GCP_used'] is False and
            record['before']==record['after'] and set(record['before'])==required and
            record['source_commit']=='bbc21eef7223565fc8ba682199046c0c137cf87d' and
            record['worktree_paths_same_as_commit'] is True,'source audit closure')
    for path,digest in record['before'].items():
        require(sha(folder/'sources'/path)==digest,'source blob pin')
    argv = record['command']['argv']
    source = Path(argv[-3])
    require(argv[0]==record['compiler']=='/usr/bin/x86_64-linux-gnu-g++-13' and
            record['compiler_sha256']=='1353e9bdd29a7295c7226bf6c63abccce056d8cac31f112e5cdbecc3f28c2769' and
            source.name=='generic_product.cpp' and argv[-4]=='-c' and argv[-2]=='-o' and
            Path(argv[-1])==source.with_suffix('.o') and argv[7]==
            '-I'+str(source.parent/'sources'/'morsehgp3D_v10'/'src') and
            argv[1:7]==['-std=c++20','-O2','-Wall','-Wextra','-Wpedantic','-Werror'] and
            len(argv)==12,'generic compile command binding')
    diagnostic = (folder/'generic_product.compile.stderr').read_text()
    require(sha(folder/'generic_product.cpp')==record['source_sha256'] and
            record['command']['exit_code']==record['command']['expected_exit_code']==1 and
            'Wide<L> : 1 <= L <= 8 mots' in diagnostic and 'Wide<9>' in diagnostic and
            '(9 <= 8)' in diagnostic and str(source) in diagnostic and
            (folder/'generic_product.compile.stdout').read_text()=='','generic width diagnostic')
    require(not (folder/'generic_product.o').exists(),'unexpected executable/object')
    return dict(source_files=8,expected_compile_exit=1,native_executions=0,
                meaning='generic Wide9 intentionally refused; current u18 engine not failing')


def lemmas():
    folder = ROOT/'independent'
    record = read(folder/'lemmas_execution.json')
    require(record['status']=='PASS' and record['before']==record['after'] and
            set(record['before'])=={'order_lemmas.py','record_lemmas.py'} and
            record['native_executions']==0 and record['GCP_used'] is False,'lemma closure')
    for path,digest in record['before'].items():
        require(sha(folder/path)==digest,'lemma source pin')
    actual = module('closed_level_order_lemmas',folder/'order_lemmas.py').run()
    require(actual['status']=='PASS' and actual['intervals']==2051 and
            actual['interval_fixtures']==33 and actual['certified_boundaries']==1805,'lemma panel')
    require(len(record['commands'])==2 and all(call['exit_code']==0 for call in record['commands']),
            'lemma paired commands')
    for mode in ('normal','optimized'):
        require(read(folder/('lemmas_'+mode+'.stdout'))==actual and
                (folder/('lemmas_'+mode+'.stderr')).read_text()=='','lemma archive differs')
    return actual


def smoke():
    folder = ROOT/'prototype'
    record = read(folder/'receipt.json')
    require(record['gcp_used'] is False and record['engine_modified'] is False and record['full'] is False and
            record['execution']['own_native_invocations']==1,'prototype scope')
    require(record['provenance']['before_smoke']==record['provenance']['after_smoke'],'smoke closure')
    builds = read(folder/'compilations.json')
    modes = {'normal','ubsan','float_mutant','high_mutant','smoke'}
    require(len(builds)==5 and {row['mode'] for row in builds}==modes,'compilation mode coverage')
    base = ['timeout','45s','g++','-std=c++20']
    for build in builds:
        mode = build['mode']
        flags = ['-O1','-g'] if mode in ('ubsan','smoke') else ['-O2']
        flags += ['-Wall','-Wextra','-Wpedantic','-Werror']
        if mode in ('ubsan','smoke'):
            flags += ['-fsanitize=undefined','-fno-sanitize-recover=all']
        if mode in ('float_mutant','high_mutant'):
            flags += ['-DMUTANT_FLOAT' if mode=='float_mutant' else '-DMUTANT_IGNORE_HIGH']
        target = 'smoke_ubsan' if mode=='smoke' else mode
        source = 'smoke.cpp' if mode=='smoke' else 'level_comparator.cpp'
        require(shlex.split(build['command'])==base+flags+[source,'-o',target],
                'compilation command differs')
        completion = build.get('completion',build)
        require(completion['exit_code']==0 and build['output']=='' and completion['output']=='',
                'compilation exit/diagnostic')
    sources = record['provenance']['frozen_sources']
    require(set(sources)=={'level_comparator.cpp','smoke.cpp','PROTOCOL.txt'},'smoke source set')
    for name,digest in sources.items():
        require(sha(folder/name)==digest and record['provenance']['before_smoke'][name]==digest,
                'smoke source pin')
    execution = read(folder/'smoke_run.json')['execution']
    require(execution['exit_code']==0 and execution['argv']==['timeout','5s','./smoke_ubsan'] and
            execution['stdout']==(folder/'smoke.stdout').read_text() and
            execution['stderr']==(folder/'smoke.stderr').read_text(),'smoke command/output binding')
    actual = read(folder/'smoke.stdout')
    require(actual['checks']==27 and actual['failures']==0 and
            (folder/'smoke.stderr').read_text()=='','smoke result')
    return dict(native_invocations=1,checks=27)


if __name__=='__main__':
    inner = sys.argv[1:]==['--inner-only']
    require(inner or not sys.argv[1:],'unsupported arguments')
    # All packet files are hashed BEFORE any archived Python module executes.
    manifests = {name:manifest(ROOT/name) for name in ('prototype','independent')}
    if not inner:
        manifests['root'] = manifest(ROOT)
    assembly = read(ROOT/'assembly.json')
    for folder,entries in assembly['copied_sha256'].items():
        for path,digest in entries.items():
            require(sha(ROOT/folder/path)==digest,'assembly changed bytes')
    print(json.dumps(dict(status='PASS',manifests=manifests,independent=independent(),
                         source_port=source_port(),lemmas=lemmas(),smoke=smoke(),GCP_used=False,
                         scope='closed audit only; no native level generator/sort/FULL qualification'),sort_keys=True))
