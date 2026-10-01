"""OPEN recorder; never execute a closed packet. Native binaries go outside the archive."""
import datetime
import hashlib
import json
import pathlib
import subprocess
import sys
import time

PACKET = pathlib.Path(__file__).resolve().parent
SOURCE = pathlib.Path('/workspaces/E-HGP/build/v10-integration-r2/src/morsehgp3D_v10/src/core')
PINS = {
    'cli_output.hpp': '763e2ee3bb74ab4b40042eb0efcdc7f47f62e7dd53a6dcda610076925792e20e',
    'status.hpp': 'f6983f95f60195eb5b5f73f73d9cc97b48d694d0877efd226072d6240cf84038',
    'types.hpp': '716da6aa079e46d1f9fde16e19a777228ec472bb1756ad0d7fede4655125da4c',
    'reasons.def': 'bf30b339ed2b26cd97e07c06c644683e80f641ec9c1eeb77c8135e668afb3409',
}


def now():
    return datetime.datetime.now(datetime.timezone.utc).isoformat()


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


def run(argv, cwd):
    started, t = now(), time.monotonic()
    try:
        r = subprocess.run(argv, cwd=cwd, capture_output=True, text=True, timeout=10)
        return dict(argv=argv, cwd=str(cwd), started=started, ended=now(), seconds=time.monotonic()-t,
                    exit=r.returncode, stdout=r.stdout, stderr=r.stderr, timeout=False)
    except subprocess.TimeoutExpired as e:
        def text(v):
            return v.decode(errors='replace') if isinstance(v, bytes) else v or ''
        return dict(argv=argv, cwd=str(cwd), started=started, ended=now(), seconds=time.monotonic()-t,
                    exit=None, stdout=text(e.stdout), stderr=text(e.stderr), timeout=True)


def main():
    if len(sys.argv) != 2 or (PACKET/'MANIFEST.sha256').exists():
        raise SystemExit('REFUS closed packet or runtime argument')
    work = pathlib.Path(sys.argv[1]).resolve()
    if not work.is_dir() or list(work.iterdir()):
        raise SystemExit('REFUS runtime must be existing empty private directory')
    before = {f: sha(SOURCE/f) for f in PINS}
    snapshots = {f: sha(PACKET/'baseline/core'/f) for f in PINS}
    if before != PINS or snapshots != PINS:
        raise SystemExit('REFUS source pins')
    receipt = dict(schema=1, scope='native_header_only_mp10_causal_not_historical_code3', started=now(),
                   source_root=str(SOURCE), source_before=before, snapshot_sources=snapshots,
                   runtime=str(work), compiler=run(['/usr/bin/g++', '--version'], work), builds=[], cases=[])
    if receipt['compiler']['exit'] != 0:
        raise SystemExit('REFUS compiler unavailable')
    receipt['compiler_path'] = str(pathlib.Path('/usr/bin/g++').resolve())
    receipt['compiler_sha256'] = sha(pathlib.Path(receipt['compiler_path']))
    for variant in ('baseline', 'mutant'):
        binary = work/variant
        args = ['/usr/bin/g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-DMP10_EXPECT_MUTANT='+('1' if variant == 'mutant' else '0'),
                '-I'+str(PACKET/variant), '-I'+str(PACKET/'baseline'), str(PACKET/'probe.cpp'),
                '-Wl,--wrap=open', '-o', str(binary)]
        build = run(args, work)
        receipt['builds'].append(dict(variant=variant, command=build))
        if build['exit'] != 0:
            break
        build['binary_sha256'] = sha(binary)
        for mode in ('plain', 'inject'):
            case_dir = work/(variant+'_'+mode)
            case_dir.mkdir()
            call = run([str(binary), mode, str(case_dir)], work)
            receipt['cases'].append(dict(id=variant+'_'+mode, binary_sha256=sha(binary), command=call))
            if call['exit'] != 0:
                break
        if any(c['command']['exit'] != 0 for c in receipt['cases']):
            break
    receipt['source_after'] = {f: sha(SOURCE/f) for f in PINS}
    receipt['snapshot_after'] = {f: sha(PACKET/'baseline/core'/f) for f in PINS}
    receipt['ended'] = now()
    print(json.dumps(receipt, indent=2, ensure_ascii=False))
    return 0 if receipt['source_after'] == before and len(receipt['cases']) == 4 and all(
        c['command']['exit'] == 0 for c in receipt['cases']) else 1


if __name__ == '__main__':
    sys.exit(main())
