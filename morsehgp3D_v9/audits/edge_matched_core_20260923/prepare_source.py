#!/usr/bin/env python3
"""Archive pinned product source into /tmp and add audit-only core tracing."""
import argparse
import hashlib
import io
import json
import shutil
import subprocess
import tarfile
from pathlib import Path

SOURCE_COMMIT = 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435'
REL = 'src/gen/pipeline/wspd_q34.cpp'
BEFORE_INCLUDE = '#include "pipeline/wspd_q34.hpp"\n'
BEFORE_LOAD = '      dead_.load(*core, work.dead_core);\n'
AFTER_LOAD = (BEFORE_LOAD + '      edge_trace_audit::record_core(a, b, core->site_count(), mask,\n'
              '                                   index_->cloud().points().size());\n')
BEFORE_PROVE = '      mask = static_cast<std::uint8_t>(mask & ~dead_.prove(k_, mask, work.dead_core));\n'
AFTER_PROVE = (BEFORE_PROVE +
               '      edge_trace_audit::record_core(a, b, core->site_count(),\n'
               '                                   audit_mask_before_core, mask,\n'
               '                                   index_->cloud().points().size());\n')


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--after-core', action='store_true',
                    help='also record the mask after the exact dead-core proof')
    args = ap.parse_args()
    if args.out.exists():
        raise SystemExit(f'refusing existing output: {args.out}')
    args.out.mkdir(parents=True)
    command = ['git', '-C', str(args.repo), 'archive', SOURCE_COMMIT,
               'morsehgp3D_v9/CMakeLists.txt', 'morsehgp3D_v9/cmake',
               'morsehgp3D_v9/src', 'morsehgp3D_v9/bench', 'morsehgp3D_v9/tests']
    archive = subprocess.run(command, check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar.getmembers():
            path = Path(member.name)
            if path.is_absolute() or '..' in path.parts or path.parts[0] != 'morsehgp3D_v9':
                raise SystemExit(f'unsafe source member: {member.name}')
        tar.extractall(args.out, filter='data')
    source = args.out / 'morsehgp3D_v9'
    path = source / REL
    original = path.read_bytes()
    text = original.decode()
    if text.count(BEFORE_INCLUDE) != 1 or text.count(BEFORE_LOAD) != 1 or \
       (args.after_core and text.count(BEFORE_PROVE) != 1):
        raise SystemExit('pinned patch anchors changed')
    text = text.replace(BEFORE_INCLUDE, BEFORE_INCLUDE + '#include "pipeline/edge_trace_audit.hpp"\n')
    if args.after_core:
        text = text.replace(BEFORE_LOAD, '      const auto audit_mask_before_core = mask;\n' + BEFORE_LOAD)
        text = text.replace(BEFORE_PROVE, AFTER_PROVE)
    else:
        text = text.replace(BEFORE_LOAD, AFTER_LOAD)
    path.write_text(text)
    header = Path(__file__).with_name('edge_trace_after_audit.hpp' if args.after_core
                                     else 'edge_trace_audit.hpp')
    shutil.copyfile(header, source / 'src/gen/pipeline/edge_trace_audit.hpp')
    manifest = {'source_commit': SOURCE_COMMIT, 'source_file': REL,
                'source_sha256': sha(original), 'injected_sha256': sha(path.read_bytes()),
                'trace_header_sha256': sha(header.read_bytes()),
                'git_archive_sha256': sha(archive), 'source_dir': str(source),
                'trace_mode': 'before_after_core' if args.after_core else 'before_core'}
    (args.out / 'PATCH_MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps(manifest, sort_keys=True))


if __name__ == '__main__':
    main()
