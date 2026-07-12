import os
from pathlib import Path
import subprocess
import sys


def test_repository_root_shim_keeps_atlas_dependencies_lazy():
    repository_root = Path(__file__).resolve().parents[2]
    script = r'''
import builtins
import sys

original_import = builtins.__import__

def guarded_import(name, *args, **kwargs):
    if name == "torch" or name.startswith("torch."):
        raise AssertionError("PowerCover import attempted to load optional torch")
    return original_import(name, *args, **kwargs)

builtins.__import__ = guarded_import
import perg_hgp
assert perg_hgp.PowerCover3D is not None
assert perg_hgp.PowerCoverConfig is not None
assert "torch" not in sys.modules
'''
    environment = os.environ.copy()
    environment["PYTHONPATH"] = str(repository_root)
    completed = subprocess.run(
        [sys.executable, "-c", script],
        cwd=repository_root,
        env=environment,
        text=True,
        capture_output=True,
        check=False,
    )
    assert completed.returncode == 0, completed.stderr
