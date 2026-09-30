"""Read-only short SiteTree R2 gate replay; stdout receipt, no engine build."""
import hashlib
import json
from pathlib import Path
import subprocess
import time

ROOT = Path('/tmp/mhgp10-r2/sitetree')
TARGETS = {
    'binary': ROOT / 'build/mhgp10_site_tree_far_center',
    'library': ROOT / 'build/libmhgp10_core.a',
    'source': ROOT / 'src/morsehgp3D_v10/src/cloud/site_tree.cpp',
    'header': ROOT / 'src/morsehgp3D_v10/src/cloud/site_tree.hpp',
    'gate_source': ROOT / 'src/morsehgp3D_v10/tests/regression/site_tree_far_center.cpp',
    'cmake_cache': ROOT / 'build/CMakeCache.txt',
    'core_flags': ROOT / 'build/CMakeFiles/mhgp10_core.dir/flags.make',
    'gate_flags': ROOT / 'build/CMakeFiles/mhgp10_site_tree_far_center.dir/flags.make',
    'link': ROOT / 'build/CMakeFiles/mhgp10_site_tree_far_center.dir/link.txt',
}

def hashes():
    return {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in TARGETS.items()}

before = hashes()
argv = [str(TARGETS['binary'])]
start = time.monotonic()
result = subprocess.run(argv, capture_output=True, text=True, timeout=10)
wall = time.monotonic()-start
after = hashes()
print(json.dumps({'scope': 'existing R2 SiteTree gate only; four cfenv rounding modes; no engine build; GCP0',
                  'argv': argv, 'returncode': result.returncode, 'wall_seconds': wall,
                  'stdout': result.stdout, 'stderr': result.stderr,
                  'hashes_before': before, 'hashes_after': after,
                  'hashes_stable': before == after, 'targets': {k: str(v) for k, v in TARGETS.items()},
                  'core_flags': TARGETS['core_flags'].read_text(),
                  'gate_flags': TARGETS['gate_flags'].read_text(),
                  'link_command': TARGETS['link'].read_text(),
                  'qualified_fast_path_rounding': 'FE_TONEAREST',
                  'qualified_directed_path': 'exact fallback under FE_UPWARD/FE_DOWNWARD/FE_TOWARDZERO',
                  'MXCSR_only_rounding': 'excluded from header contract; no such replay here',
                  'FTZ_DAZ': 'no direct gate in inspected R2 sources; not newly measured',
                  'public_center_validation': 'caller representation precondition remains; filtered=false is not refusal',
                  'whole_pipeline_FENV_qualification': False}, indent=2))
