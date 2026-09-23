#!/usr/bin/env python3
"""Produce a pinned, audit-only shadow of the dead-core form prefix."""
import argparse
import hashlib
import io
import json
import subprocess
import tarfile
from pathlib import Path

SOURCE_COMMIT = 'c265a5dae4dd92059fc78acc0a1d7f52de9c1435'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(text, old, new):
    if text.count(old) != 1:
        raise RuntimeError(f'patch anchor count {text.count(old)}: {old!r}')
    return text.replace(old, new)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--repo', type=Path, default=Path('/workspaces/E-HGP'))
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    repo, out = args.repo, args.out
    if out.exists():
        raise RuntimeError(f'refusing existing output: {out}')
    out.mkdir(parents=True)
    source = out / 'morsehgp3D_v9'
    archive = subprocess.run(['git', '-C', str(repo), 'archive', SOURCE_COMMIT,
                              'morsehgp3D_v9/CMakeLists.txt', 'morsehgp3D_v9/cmake',
                              'morsehgp3D_v9/src', 'morsehgp3D_v9/bench',
                              'morsehgp3D_v9/tests'], check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        for member in tar.getmembers():
            parts = Path(member.name).parts
            if not parts or parts[0] != 'morsehgp3D_v9' or '..' in parts:
                raise RuntimeError(f'unsafe tar member: {member.name}')
        tar.extractall(out, filter='data')
    hashes = {}

    def patch(rel, edits):
        path = source / rel
        before = path.read_bytes()
        text = before.decode()
        for old, new in edits:
            text = replace_once(text, old, new)
        path.write_text(text)
        hashes[rel] = {'source_sha256': sha(before), 'injected_sha256': sha(path.read_bytes())}

    patch('src/gen/lanes/q34_dead_lanes.hpp', [
        ('  [[nodiscard]] std::size_t retained_bytes() const;\n',
         '  [[nodiscard]] std::size_t retained_bytes() const;\n'
         '  // AUDIT ONLY: smallest visited prefix and depth-2 traversal state.\n'
         '  [[nodiscard]] std::size_t audit_prefix() const { return audit_prefix_; }\n'
         '  [[nodiscard]] std::uint32_t audit_depth2_scanned() const { return audit_depth2_scanned_; }\n'
         '  [[nodiscard]] std::uint32_t audit_depth2_full() const { return audit_depth2_full_; }\n'
         '  [[nodiscard]] std::uint32_t audit_depth2_descended() const { return audit_depth2_descended_; }\n'),
        ('  std::vector<std::vector<std::uint32_t>> levels_;  // frontier by depth\n',
         '  std::vector<std::vector<std::uint32_t>> levels_;  // frontier by depth\n'
         '  std::size_t audit_prefix_{};\n'
         '  std::uint32_t audit_depth2_scanned_{}, audit_depth2_full_{}, audit_depth2_descended_{};\n'),
    ])
    patch('src/gen/lanes/q34_dead_lanes.cpp', [
        ('#include "lanes/q34_dead_lanes.hpp"\n',
         '#include "lanes/q34_dead_lanes.hpp"\n\n#include <algorithm>\n'),
        ('  loaded_ = false;\n  const auto& index = *cover.index();\n',
         '  loaded_ = false;\n'
         '  audit_prefix_ = 0;\n'
         '  audit_depth2_scanned_ = audit_depth2_full_ = audit_depth2_descended_ = 0;\n'
         '  const auto& index = *cover.index();\n'),
        ('    if (inside >= target) { counter_add(work.deep_cells); return lanes; }\n'
         '    auto& next = levels_[depth];\n',
         '    if (inside >= target) { counter_add(work.deep_cells); return lanes; }\n'
         '    if (depth == 2) ++audit_depth2_scanned_;\n'
         '    auto& next = levels_[depth];\n'),
        ('    for (const auto id : frontier) {\n      ++tests;\n'
         '      const auto& f = forms_[id];\n',
         '    for (const auto id : frontier) {\n      ++tests;\n'
         '      audit_prefix_ = std::max(audit_prefix_, static_cast<std::size_t>(id) + 1);\n'
         '      const auto& f = forms_[id];\n'),
        ('        if (++inside >= target) {\n'
         '          counter_add(work.uniform_tests, tests);\n',
         '        if (++inside >= target) {\n'
         '          if (depth == 2 && tests == frontier.size()) ++audit_depth2_full_;\n'
         '          counter_add(work.uniform_tests, tests);\n'),
        ('    counter_add(work.uniform_tests, tests);\n    // q4 may already hold here',
         '    if (depth == 2) ++audit_depth2_full_;\n'
         '    counter_add(work.uniform_tests, tests);\n    // q4 may already hold here'),
        ('  const i64 x = c.left + (c.right - c.left) / 2, y = c.bottom + (c.top - c.bottom) / 2;\n',
         '  if (depth == 2) ++audit_depth2_descended_;\n'
         '  const i64 x = c.left + (c.right - c.left) / 2, y = c.bottom + (c.top - c.bottom) / 2;\n'),
    ])
    patch('src/gen/pipeline/wspd_q34.cpp', [
        ('#include "pipeline/wspd_q34.hpp"\n',
         '#include "pipeline/wspd_q34.hpp"\n#include "pipeline/lazy_prefix_trace_audit.hpp"\n'),
        ('      dead_.load(*core, work.dead_core);\n'
         '      mask = static_cast<std::uint8_t>(mask & ~dead_.prove(k_, mask, work.dead_core));\n',
         '      dead_.load(*core, work.dead_core);\n'
         '      const auto audit_before = mask;\n'
         '      mask = static_cast<std::uint8_t>(mask & ~dead_.prove(k_, mask, work.dead_core));\n'
         '      lazy_prefix_trace_audit::record(a, b, core->site_count(), dead_.audit_prefix(),\n'
         '          audit_before, mask, dead_.audit_depth2_scanned(), dead_.audit_depth2_full(),\n'
         '          dead_.audit_depth2_descended(), index_->cloud().points().size());\n'),
    ])
    header = Path(__file__).with_name('lazy_prefix_trace_audit.hpp')
    target = source / 'src/gen/pipeline/lazy_prefix_trace_audit.hpp'
    target.write_bytes(header.read_bytes())
    manifest = {'source_commit': SOURCE_COMMIT, 'git_archive_sha256': sha(archive),
                'files': hashes, 'trace_header_sha256': sha(header.read_bytes()),
                'source_dir': str(source), 'schema': 'mhgp9_lazy_prefix_shadow_patch_v1'}
    (out / 'PATCH_MANIFEST.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    print(json.dumps(manifest, sort_keys=True))


if __name__ == '__main__':
    main()
