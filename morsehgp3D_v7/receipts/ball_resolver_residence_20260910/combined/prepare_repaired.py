#!/usr/bin/env python3
"""Keep original harness failure intact; only the new test allocator changes."""
from pathlib import Path
import shutil

BASE = Path('/workspaces/E-HGP/build/v7_combined_resolver_20260910')
ROOT = Path('/workspaces/E-HGP')
dest = BASE / 'combined_repaired'
shutil.copytree(BASE / 'combined', dest)
shutil.copy2(ROOT / 'morsehgp3D_v7/tests/facet_resolver_cache_gate.cpp', dest / 'tests/facet_resolver_cache_gate.cpp')
source = (BASE / 'record.py').read_text()
source = source.replace("BASE / ('run_' + MODE)", "BASE / ('run_cache_repaired_' + MODE)")
source = source.replace("tree = BASE / 'combined'", "tree = BASE / 'combined_repaired'")
source = source.replace("['full_ball_tower_gate', 'full_ball_work_gate', 'full_coverage_certificate_gate', 'facet_resolver_cache_gate']", "['facet_resolver_cache_gate']")
(BASE / 'record_cache_repaired.py').write_text(source)
