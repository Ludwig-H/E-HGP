"""G4-only paired FULL gate: frozen native v10 versus native v11 profiles18/21/24.

No installer, no product patch. Keeps source inventory, compiler commands, native
streams, both original dumps and both common canonical byte streams on failure.
Input coordinates stay within u18, even when v11 is compiled for21/24.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tarfile
import time

import full_v10_codec as codec

sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'bench'))
import catalogue_profiles as profiles

PIN = Path(__file__).with_name('v10_frozen_manifest.json')
NAMES = ('singleton','line024','square_plain','square_center','global_q3_nonfirst_shell',
         'two_components_same_plateau','line13_K12','regular_tetra','obtuse_prefix','extended_q4',
         'maximum_tetra','close_levels','random0','random1')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, value):
    temporary = path.with_suffix('.tmp')
    temporary.write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
    temporary.replace(path)


def extract(archive_path, destination, manifest):
    codec.need(manifest['source_commit'] == codec.COMMIT and archive_path.stat().st_size == manifest['archive_bytes']
               and sha(archive_path) == manifest['archive_sha256'], 'frozen source archive pin')
    destination.mkdir(parents=True,exist_ok=False)
    with tarfile.open(archive_path,'r:gz') as archive:
        members = archive.getmembers()
        codec.need([m.name for m in members] == [r['path'] for r in manifest['files']], 'frozen source inventory')
        for member,row in zip(members,manifest['files']):
            path = Path(member.name)
            codec.need(member.isfile() and not path.is_absolute() and '..' not in path.parts and
                       member.size == row['bytes'] and member.size < 1 << 20, 'frozen source member')
            raw = archive.extractfile(member).read()
            codec.need(hashlib.sha256(raw).hexdigest() == row['sha256'], 'frozen source member hash')
            target = destination/path; target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(raw)


def run(args):
    args.out.mkdir(parents=True,exist_ok=False); args.work.mkdir(parents=True,exist_ok=False)
    report = dict(schema='ehgp.v11.full_v10_diff.v1',source_v10=codec.COMMIT,complete=False,conforming=False,
                  comparison='common_canonical_bytes_full_only',commands=[],cases=[],errors=[])
    target = args.out/'summary.json'
    def save(): write(target,report)
    def process(name, argv, timeout=60, stdin=None):
        entry = dict(name=name,argv=list(map(str,argv)),status='running')
        report['commands'].append(entry); save(); start = time.monotonic()
        stdout, stderr = args.out/(name+'.stdout'), args.out/(name+'.stderr')
        try:
            with stdout.open('wb') as out,stderr.open('wb') as err:
                result = subprocess.run(entry['argv'],input=stdin,stdout=out,stderr=err,timeout=timeout,check=False)
            entry.update(status='ok' if result.returncode == 0 else 'failed',returncode=result.returncode)
        except (OSError,subprocess.SubprocessError) as error:
            entry.update(status='error',error=str(error))
        finally:
            entry['seconds'] = time.monotonic()-start
            for label,path in (('stdout',stdout),('stderr',stderr)):
                if path.exists(): entry[label] = dict(path=path.name,bytes=path.stat().st_size,sha256=sha(path))
            save()
        codec.need(entry['status'] == 'ok', 'process '+name+' '+entry['status'])
        return stdout.read_text()
    save()
    try:
        manifest = profiles.load(PIN); report['manifest_sha256'] = sha(PIN)
        extract(args.archive,args.work/'source',manifest); report['archive_sha256'] = sha(args.archive); save()
        source = args.work/'source'/'morsehgp3D_v10'; build = args.work/'v10_build'
        process('configure_v10',['cmake','-S',source,'-B',build,'-DCMAKE_BUILD_TYPE=Release',
                                 '-DCMAKE_CXX_COMPILER=g++','-DBUILD_TESTING=OFF'],timeout=45)
        process('build_v10',['cmake','--build',build,'--target','mhgp10_catalogue','mhgp10_tower','-j8'],timeout=150)
        report['v10_binaries'] = {name:sha(build/name) for name in ('mhgp10_catalogue','mhgp10_tower')}
        report['v10_cache_sha256'] = sha(build/'CMakeCache.txt')
        report['qualification_sha256'] = sha(args.qualification)
        executables = profiles.checked_builds(args,executable='mhgp11_tower_forest_probe')
        report['v11_binaries'] = executables; save()
        requests = {r['name']:r for r in codec.geometry.requests(18)}
        codec.need(set(NAMES) <= requests.keys(), 'fixture inventory')
        for case_index,name in enumerate(NAMES):
            req = requests[name]; kmax = req['kmax']; records = req['records']
            points = [tuple(r[:3]) for r in records]; sites = sorted(points,key=codec.morton)
            prefix = '%02d_%s' % (case_index,name); src = args.out/(prefix+'.u32le')
            src.write_bytes(b''.join(v.to_bytes(4,'little') for p in points for v in p))
            catpath, treepath = args.out/(prefix+'.catalogue.txt'),args.out/(prefix+'.tower.txt')
            for tool,dump in (('mhgp10_catalogue',catpath),('mhgp10_tower',treepath)):
                # c764 --no-points --dump dereferences absent point vectors. Keep attachments enabled,
                # parse their syntax and do not claim their agreement with v11's FULL-only API.
                process(prefix+'_'+tool,[build/tool,src,'--k='+str(kmax),'--threads=1','--dump='+str(dump)])
            balls = codec.catalogue(catpath.read_text(),sites)
            old = codec.common(sites,codec.old_orders(treepath.read_text(),balls,sites,kmax))
            (args.out/(prefix+'.v10.common')).write_bytes(old)
            record = dict(name=name,kmax=kmax,sites=len(sites),input_sha256=sha(src),
                          v10_sha256=hashlib.sha256(old).hexdigest(),v11=[])
            report['cases'].append(record); save()
            stdin = ('%d %d %d\n' % (kmax,1 << 28,len(records))+
                     ''.join(' '.join(map(str,p))+'\n' for p in records)).encode()
            wanted = [dict(support=b['support'],qmin=b['qmin'],inner=b['inner'],shell=b['shell'],
                           level=codec.encoded(b['level'])) for b in balls]
            for bits,exe in executables.items():
                text = process(prefix+'_v11_'+str(bits),[exe['path']],stdin=stdin)
                row = profiles.base.event_json(text)
                got = [dict(b,level=codec.encoded(codec.fraction(b['level'],16))) for b in row['balls']]
                codec.need(got == wanted, 'native catalogues disagree '+name)
                common = codec.common(sites,codec.new_orders(row,bits,sites,kmax))
                (args.out/(prefix+'.v11_'+str(bits)+'.common')).write_bytes(common)
                record['v11'].append(dict(bits=bits,sha256=hashlib.sha256(common).hexdigest(),equal=common == old)); save()
                codec.need(common == old, 'native FULL disagreement '+name+' bits'+str(bits))
        codec.need(len(report['cases']) == 14 and sum(len(r['v11']) for r in report['cases']) == 42,
                   'differential nonvacuity')
        report.update(complete=True,conforming=True)
    except (OSError,ValueError,KeyError,TypeError,IndexError,subprocess.SubprocessError,tarfile.TarError) as error:
        report['errors'].append(type(error).__name__+': '+str(error))
    finally:
        save()
    print(json.dumps(dict(conforming=report['conforming'],cases=len(report['cases']),errors=report['errors'])))
    return 0 if report['conforming'] else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('archive','builds','qualification','work','out'):
        parser.add_argument('--'+name,type=Path,required=True)
    raise SystemExit(run(parser.parse_args()))
