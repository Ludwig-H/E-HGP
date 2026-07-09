# Top-level package redirection to enable seamless local imports from repository root
import os
import sys
import importlib.util

# Resolve the absolute path to the real inner package's __init__.py
_inner_pkg_path = os.path.join(os.path.dirname(__file__), 'perg_hgp', '__init__.py')

spec = importlib.util.spec_from_file_location("perg_hgp.inner", _inner_pkg_path)
inner_module = importlib.util.module_from_spec(spec)
sys.modules["perg_hgp.inner"] = inner_module
spec.loader.exec_module(inner_module)

# Expose all symbols from the real inner package
globals().update({k: v for k, v in inner_module.__dict__.items() if not k.startswith('_')})
