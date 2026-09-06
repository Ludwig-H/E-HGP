#!/usr/bin/env python3
"""Same pinned SAN ELF, fresh logs after a preserved ptrace-only failure."""
import importlib.util
import json
from pathlib import Path
import sys

HERE = Path(__file__).resolve().parent
PREVIOUS = HERE.parent
spec = importlib.util.spec_from_file_location('diameter_capture', PREVIOUS / 'record.py')
c = importlib.util.module_from_spec(spec)
spec.loader.exec_module(c)
c.BASE = HERE


def main():
    result = dict(schema='mhgp7-local-plateau-diameter-san-resume-v1', status='failed',
                  public_status='not_claimed', source_delta=False, recompilation=False)
    try:
        c.require(c.sha(PREVIOUS / 'record.py') == 'b9e31056fb60f8a67f83fe4fab6671cbea25c5d88d4a12b0710bf9d35a928db5', 'capture_pin')
        c.require(c.sha(PREVIOUS / 'prepare.receipt.json') == 'e9fb6d2a0dbd14fb79eae5145b074c23d43519313cc6f8e64c10932e4ea43afd', 'prepare_pin')
        result['prepare_receipt_sha256'] = c.sha(PREVIOUS / 'prepare.receipt.json')
        result['failed_san_receipt_sha256'] = c.sha(PREVIOUS / 'san.receipt.json')
        binary = PREVIOUS / 'bin/san'
        result['binary_sha256'] = '45a7cab777e785c5537aeac07947b6c8128a210ab928622ae26e81d4d122bd72'
        c.require(c.sha(binary) == result['binary_sha256'], 'binary_pin')
        expected = json.loads((PREVIOUS / 'san.sources_before.json').read_text())
        c.require(c.dependencies(PREVIOUS / 'san.d') == expected, 'source_pin')
        c.save(HERE / 'sources_before.json', expected)
        c.command('san_selftest', [str(binary), '--selftest'])
        c.command('san_unknown', [str(binary), '--unknown'], 2)
        c.require((HERE / 'san_selftest.stdout').read_bytes() == (PREVIOUS / 'O2_selftest.stdout').read_bytes(), 'O2_SAN_literal_equality')
        after = c.dependencies(PREVIOUS / 'san.d')
        c.save(HERE / 'sources_after.json', after)
        c.require(after == expected and c.sha(binary) == result['binary_sha256'], 'sources_binary_stable')
        result['status'] = 'completed'
    except BaseException as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        result.update(commands=len(c.ROWS), cpp_closed=all(row['closed'] for row in c.ROWS))
        c.save(HERE / 'receipt.json', result)
        print(json.dumps(result, sort_keys=True), flush=True)
    return 0 if result['status'] == 'completed' else 1


if __name__ == '__main__':
    sys.exit(main())
