"""Record the completed direct compilation and its project dependencies."""
import hashlib
import json
from pathlib import Path

root = Path('/workspaces/E-HGP')
out = root / 'build/v7_wspd_probe_20260906'
old = json.loads((root / 'morsehgp3D_v7/receipts/full_probe_no_quotas_20260906/build_r1/sources_before.json').read_text())['files']
changes = {
    'morsehgp3D_v7/src/pipeline/generate.hpp': '345129a775d430a40e151d3b1adb5cd9efeaf77a6ffb6713bd081c74d40bdd9c',
    'morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp': 'f32f66894ddc5b638602fd0b45068750cbb1d30ba82c45e651999b6d50699b6a',
}
sources = {}
for name in (out / 'full_probe.d').read_text().replace('\\\n', ' ').split(':', 1)[1].split():
    path = (root / name).resolve()
    relative = str(path.relative_to(root))
    raw = path.read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != changes.get(relative, old.get(relative)):
        raise ValueError('unexpected source change: ' + relative)
    target = out / 'source_snapshot' / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    with target.open('xb') as stream:
        stream.write(raw)
    sources[relative] = digest
record = dict(status='completed', exit_code=0, public_status='not_claimed',
    compilation_observed_by_ROOT=True, dependencies=len(sources), project_sources=sources,
    binary_sha256=hashlib.sha256((out / 'full_probe').read_bytes()).hexdigest(),
    command='c++ -O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -MMD -MF build/v7_wspd_probe_20260906/full_probe.d morsehgp3D_v7/bench/full_gabriel_lazy_probe.cpp -o build/v7_wspd_probe_20260906/full_probe',
    changes_from_published_probe='terminal q2 count reuse; trailing blank removed; help text only',
    authority='direct_compilation_and_project_dependency_snapshot_not_hermetic_toolchain')
with (out / 'build.json').open('x') as stream:
    json.dump(record, stream, indent=2, sort_keys=True)
    stream.write('\n')
print(json.dumps(dict(status='recorded', dependencies=len(sources), binary=record['binary_sha256'])))
