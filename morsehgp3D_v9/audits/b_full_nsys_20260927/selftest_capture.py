#!/usr/bin/env python3
"""Small offline tests; never start a VM or invoke a controller."""
from unittest.mock import patch
import json
import types

import capture as c


def main():
    positive = rejected = 0

    def refuse(call):
        nonlocal rejected
        try:
            call()
        except ValueError:
            rejected += 1
        else:
            raise RuntimeError('mutation accepted')

    def git(argv, **_kwargs):
        c.need(argv[:4] == ['git', '-C', str(c.ROOT), '--no-replace-objects'] and
               argv[4] == 'show', 'read-only Git recipe')
        relative = argv[5].split(':', 1)[1]
        return types.SimpleNamespace(stdout=(c.ROOT/relative).read_bytes())

    with patch.object(c.subprocess, 'run', side_effect=git):
        c.committed('1'*40)
        positive += 1
        for bad in ('main', '1'*39, '../main', None):
            refuse(lambda: c.committed(bad))
    with patch.object(c.subprocess, 'run', return_value=types.SimpleNamespace(stdout=b'changed')):
        refuse(lambda: c.committed('1'*40))

    # The real immutable helpers and FULL package can be checked offline.
    native, wait = c.runtime()
    c.need(wait.__module__ == 'session', 'pinned cooperative wait')
    c.package_check(native)
    positive += 1
    with patch.object(c, 'SNAPSHOT_SHA', '0'*64):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'MANIFEST_SHA', '0'*64):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'NATIVE_COMMIT', '0'*40):
        refuse(lambda: c.package_check(native))
    with patch.object(c, 'PINS', {'gcp-migration/full_probe_session_v7.py': '0'*64}):
        refuse(c.runtime)
    c.need(positive == 2 and rejected == 9, 'nonvacuous offline test count')
    print(json.dumps(dict(status='passed', positive=positive, rejected=rejected,
                         GCP_used=False, GPU_executed=False)))


if __name__ == '__main__':
    main()
