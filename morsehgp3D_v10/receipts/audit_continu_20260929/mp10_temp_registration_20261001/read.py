"""Read-only archive judge; hash/inventory before parsing receipts. Never invokes native code."""
import hashlib
import json
import pathlib
import re
import sys

FILES = {
    'README.txt', 'probe.cpp', 'capture.py', 'read.py', 'receipt.json',
    'baseline/core/cli_output.hpp', 'baseline/core/status.hpp', 'baseline/core/types.hpp',
    'baseline/core/reasons.def', 'mutant/core/cli_output.hpp',
}
PINS = {
    'cli_output.hpp': '763e2ee3bb74ab4b40042eb0efcdc7f47f62e7dd53a6dcda610076925792e20e',
    'status.hpp': 'f6983f95f60195eb5b5f73f73d9cc97b48d694d0877efd226072d6240cf84038',
    'types.hpp': '716da6aa079e46d1f9fde16e19a777228ec472bb1756ad0d7fede4655125da4c',
    'reasons.def': 'bf30b339ed2b26cd97e07c06c644683e80f641ec9c1eeb77c8135e668afb3409',
}
OLD = "      e.temp = std::move(t);  // noexcept : aucune operation qui leve entre la creation et l'enregistrement\n"
NEW = '      const std::string copy = t + "";\n      e.temp = copy;\n'
IDS = ['baseline_plain', 'baseline_inject', 'mutant_plain', 'mutant_inject']


def need(ok, why):
    if not ok:
        raise ValueError(why)


def sha(blob):
    return hashlib.sha256(blob).hexdigest()


def unique(pairs):
    out = {}
    for k, v in pairs:
        need(k not in out, 'duplicate JSON key')
        out[k] = v
    return out


def main():
    need(len(sys.argv) == 3, 'argv root external_manifest_sha')
    root, pin = pathlib.Path(sys.argv[1]), sys.argv[2]
    need(re.fullmatch('[a-f0-9]{64}', pin) is not None, 'external manifest SHA')
    manifest = (root/'MANIFEST.sha256').read_bytes()
    need(sha(manifest) == pin, 'external manifest mismatch')
    need((root/'MANIFEST_SHA256').read_text() == pin+'\n', 'manifest anchor')
    actual = set()
    for path in root.rglob('*'):
        need(not path.is_symlink(), 'archive symlink')
        if path.is_file():
            actual.add(path.relative_to(root).as_posix())
        else:
            need(path.is_dir(), 'archive special file')
    need(actual == FILES | {'MANIFEST.sha256', 'MANIFEST_SHA256'}, 'closed file inventory')
    listed = {}
    for line in manifest.decode().splitlines():
        m = re.fullmatch('([a-f0-9]{64})  (.+)', line)
        need(m is not None and m[2] in FILES and m[2] not in listed, 'manifest record')
        listed[m[2]] = m[1]
    need(set(listed) == FILES, 'manifest inventory')
    for name, expected in listed.items():
        need(sha((root/name).read_bytes()) == expected, 'content SHA '+name)
    # Only after the entire exact archive is hashed may JSON be parsed.
    d = json.loads((root/'receipt.json').read_text(), object_pairs_hook=unique)
    need(d['schema'] == 1 and d['scope'] == 'native_header_only_mp10_causal_not_historical_code3', 'scope')
    need(all(d[k] == PINS for k in ['source_before', 'source_after', 'snapshot_sources', 'snapshot_after']), 'source pins')
    for name, expected in PINS.items():
        need(sha((root/'baseline/core'/name).read_bytes()) == expected, 'snapshot '+name)
    base = (root/'baseline/core/cli_output.hpp').read_text()
    need(base.count(OLD) == 1 and (root/'mutant/core/cli_output.hpp').read_text() == base.replace(OLD, NEW), 'exact MP10')
    def good(c):
        need(type(c['exit']) is int and c['exit'] == 0 and c['timeout'] is False, 'argv/exit/timeout')
        need(c['stderr'] == '' and 0 <= c['seconds'] <= 10.1, 'stderr/elapsed')
    good(d['compiler'])
    need(d['compiler']['argv'] == ['/usr/bin/g++', '--version'] and
         d['compiler']['stdout'].startswith('g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0\n'), 'GNU13.3')
    need(re.fullmatch('[a-f0-9]{64}', d['compiler_sha256']) is not None, 'compiler SHA')
    need([b['variant'] for b in d['builds']] == ['baseline', 'mutant'], 'build inventory')
    buildpins = {}
    origin = None
    for b in d['builds']:
        c, variant = b['command'], b['variant']
        good(c)
        need(c['stdout'] == '', 'build stdout')
        args = c['argv']
        need(len(args) == 14 and args[10].endswith('/probe.cpp'), 'compile argv shape')
        current_origin = str(pathlib.Path(args[10]).parent)
        origin = origin or current_origin
        need(origin == current_origin, 'compile source origin')
        expected = ['/usr/bin/g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                    '-DMP10_EXPECT_MUTANT='+('1' if variant == 'mutant' else '0'),
                    '-I'+origin+'/'+variant, '-I'+origin+'/baseline', origin+'/probe.cpp',
                    '-Wl,--wrap=open', '-o', d['runtime']+'/'+variant]
        need(args == expected and c['cwd'] == d['runtime'], 'compile argv')
        need(re.fullmatch('[a-f0-9]{64}', c['binary_sha256']) is not None, 'binary SHA')
        buildpins[variant] = c['binary_sha256']
    need([c['id'] for c in d['cases']] == IDS, 'case inventory')
    for case in d['cases']:
        variant, mode = case['id'].split('_')
        c = case['command']
        good(c)
        need(c['argv'] == [d['runtime']+'/'+variant, mode, d['runtime']+'/'+case['id']] and
             c['cwd'] == d['runtime'] and case['binary_sha256'] == buildpins[variant], 'case argv/pin')
        result = json.loads(c['stdout'], object_pairs_hook=unique)
        leak = int(variant == 'mutant' and mode == 'inject')
        expected = dict(variant=variant, case=mode, declaration_ok=1, reserve_returned=1, outer_exception=0,
                        reason='memory_budget' if leak else 'none', creations=1, injections=leak,
                        pending_before_disarm=int(variant == 'baseline' and mode == 'inject'),
                        fd_delta=leak, temp_delta=leak, created_fd_live=leak, created_temp_live=leak,
                        destination_intact=1, cleanup_fd_ok=1, cleanup_temp_ok=1,
                        post_cleanup_fd_delta=0, post_cleanup_temp_delta=0, expected_observation=1)
        need(result == expected and all(type(result[k]) is type(v) for k, v in expected.items()), 'causal observation')
    need(all(sha((root/name).read_bytes()) == expected for name, expected in listed.items()) and
         (root/'MANIFEST.sha256').read_bytes() == manifest, 'archive changed during read')
    print('MP10_ARCHIVE_OK cases=4 baseline_leaks=0 mutant_injected_fd_temp=1/1 cleanup=0/0 native_replayed=0')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (ValueError, KeyError, OSError, TypeError, IndexError) as e:
        print('REFUS '+str(e))
        sys.exit(2)
