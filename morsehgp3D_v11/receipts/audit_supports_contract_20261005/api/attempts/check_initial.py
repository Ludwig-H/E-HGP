#!/usr/bin/env python3
"""Bounded source review and Linux RLIMIT_FSIZE mechanism, no native HGP executed."""
import ast
import errno
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import sys
import tempfile

BASE = Path(__file__).resolve().parent
SRC = BASE / 'sources' / 'morsehgp3D_v11'
checks = 0

def need(value, label):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(label)

records = json.loads((BASE / 'SOURCES.json').read_text())
for record in records:
    b = (BASE / 'sources' / record['copy']).read_bytes()
    need(len(b) == record['bytes'], 'source size ' + record['copy'])
    need(hashlib.sha256(b).hexdigest() == record['sha256'], 'source hash ' + record['copy'])

cli = (SRC / 'cli/mhgp11.cpp').read_text()
api = (SRC / 'src/api/manifest.cpp').read_text()
header = (SRC / 'src/api/api.hpp').read_text()
directory = (SRC / 'src/io/directory.cpp').read_text()
contract = (BASE / 'sources/L0/morsehgp3D_v11/docs/SORTIES.md').read_text()
need('::signal(SIGPIPE, SIG_IGN);' in cli, 'SIGPIPE ignored')
need('SIGXFSZ' not in cli, 'captured CLI does not ignore SIGXFSZ')
need('published_complete' in contract, 'normative publication state')
need('published_complete' not in cli + api + header, 'publication state still absent in S5')
need('u64le(forest.order())' in api and 'out.u32le(node.birth_key)' in api,
     'S5 tree signature still legacy index based')
need('version 2' in contract and 'MHGP11GX' in contract, 'S0 signature v2')
need('api::tree_k_sha256(tower.order(product.k()))' in api, 'manifest uses requested order in captured source')
need('::stat(argv[i] + prefix.size(), &in)' in cli, 'stdout input symlink resolved')
need('(flags & O_ACCMODE) != O_RDONLY' in cli, 'readonly stdout rejected before execution')
need(directory.index('MHGP11_TRY(publish())') < directory.index('manifest_sha256_ = manifest'),
     'manifest digest remains assigned only after publish')
unit = (SRC / 'tests/api/session_test.cpp').read_text()
start = unit.index('MHGP11_TEST(tree_digest,')
end = unit.index('MHGP11_TEST(provenance,', start)
need('publish(' not in unit[start:end], 'tree digest unit group does not test published field')
# A helper present but never called is not an independent digest gate.
helpers = (SRC / 'tests/cli/cli_support.py').read_text()
need('def tree_digest_of(' in helpers, 'digest helper exists')
call_count = 0
for path in (SRC / 'tests/cli').glob('*.py'):
    for node in ast.walk(ast.parse(path.read_text())):
        if isinstance(node, ast.Call):
            name = node.func.id if isinstance(node.func, ast.Name) else node.func.attr if isinstance(node.func, ast.Attribute) else ''
            call_count += name == 'tree_digest_of'
need(call_count == 0, 'digest helper has no callers in captured CLI gates')

# Confirm the OS mechanism only. Child writes at most 64 bytes in a temporary file;
# RLIMIT_CORE prevents crash dumps, and parent always owns cleanup. No C++ executable.
child = '''import errno, os, resource, signal, sys
resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
resource.setrlimit(resource.RLIMIT_FSIZE, (64, 64))
signal.signal(signal.SIGXFSZ, signal.SIG_IGN if sys.argv[2] == "ignore" else signal.SIG_DFL)
fd = os.open(sys.argv[1], os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
try:
    first = os.write(fd, b"x" * 128)
    print("first=" + str(first), flush=True)
    os.write(fd, b"x")
except OSError as error:
    print("errno=" + str(error.errno), flush=True)
finally:
    os.close(fd)
'''
results = []
with tempfile.TemporaryDirectory(prefix='fsize-audit-') as folder:
    for mode in ('default', 'ignore'):
        path = Path(folder) / mode
        done = subprocess.run([sys.executable, '-S', '-B', '-c', child, str(path), mode],
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=5, check=False)
        need(path.stat().st_size == 64, 'bounded partial write ' + mode)
        need(b'first=64\n' in done.stdout, 'first write observed ' + mode)
        if mode == 'default':
            need(done.returncode == -signal.SIGXFSZ, 'default handling terminates by SIGXFSZ')
            need(b'errno=' not in done.stdout, 'no ordinary error recovery with default signal')
        else:
            need(done.returncode == 0, 'ignored signal enables ordinary error recovery')
            need(done.stdout == ('first=64\nerrno=' + str(errno.EFBIG) + '\n').encode(), 'ignored signal exposes EFBIG')
        results.append({'mode': mode, 'code': done.returncode, 'stdout': done.stdout.decode(),
                        'stderr': done.stderr.decode(), 'bytes': path.stat().st_size})
print(json.dumps({'status': 'conforme', 'checks': checks, 'source_files': len(records),
                  'native_executed': False, 'kernel_mechanism_only': results}, sort_keys=True))
