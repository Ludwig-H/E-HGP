#!/usr/bin/env python3
"""Exact EU target binding for the unchanged, pinned FULL session controller.

Same CLI, guard scripts, generation checks and shutdown funnel. This module
never invokes a cloud command itself. The controller's receipt identifies the
controller, not this wrapper: retain the separately pinned wrapper binding.
"""
import hashlib
import importlib.util
from pathlib import Path
import sys

CONTROLLER_SHA256 = '177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8'
ORIGINAL_TARGET = dict(project='devpod-gpu-exploration', zone='us-central1-b',
                       instance='ehgp-v7-4fa0e0789a7d5bb06b787d35')
TARGET = dict(project='devpod-gpu-exploration', zone='europe-west4-a', instance='ehgp-blackwell-spot')


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def load_controller():
    path = Path(__file__).with_name('full_probe_session_v7.py')
    if path.is_symlink() or not path.is_file() or sha(path) != CONTROLLER_SHA256:
        raise ValueError('reviewed original controller pin')
    spec = importlib.util.spec_from_file_location('mhgp7_exact_eu_controller', path)
    if spec is None or spec.loader is None:
        raise ValueError('reviewed controller import')
    controller = importlib.util.module_from_spec(spec)
    sys.dont_write_bytecode = True
    spec.loader.exec_module(controller)
    if sha(path) != CONTROLLER_SHA256 or controller.TARGET != ORIGINAL_TARGET:
        raise ValueError('original controller changed during import')
    # The only controller global replaced. All functions, scripts and CLI stay
    # the original objects; every target-sensitive guard reads this same dict.
    controller.TARGET = dict(TARGET)
    return controller


def binding():
    return dict(schema='ehgp.v7.exact_session_wrapper.v1', wrapper_sha256=sha(__file__),
                controller_sha256=CONTROLLER_SHA256, original_target=dict(ORIGINAL_TARGET),
                target=dict(TARGET), changed_controller_globals=['TARGET'],
                controller_receipt_identifies_wrapper=False, no_cloud_command_in_wrapper=True)


def main():
    return load_controller().main()


if __name__ == '__main__':
    raise SystemExit(main())
