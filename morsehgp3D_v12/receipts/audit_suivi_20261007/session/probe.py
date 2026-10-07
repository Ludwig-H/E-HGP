#!/usr/bin/env python3
"""Small host checks of the pinned M6 source. Never opens a CUDA context."""
import hashlib
import json
import pathlib
import re
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[3]
SOURCE = ROOT / 'bench/mes_m6_session_cost.cu'


def checked_run(args):
    result = subprocess.run(args, text=True, capture_output=True)
    if result.returncode:
        raise RuntimeError(json.dumps(dict(args=args, exit=result.returncode, stderr=result.stderr)))
    return result


def main():
    text = SOURCE.read_text()
    quantile_code = text[text.index('struct Quantiles {'):text.index('void emit(')]
    host = '''#include <algorithm>
#include <vector>
#include <cstdio>
''' + quantile_code + '''
int main() {
  const auto even = quantiles({1,2,3,4,5,6,7,8,9,10});
  const auto odd = quantiles({1,2,3,4,5,6,7,8,9});
  const auto mixed = quantiles({100,1,1,1,1,1,1,1,1,1});
  const auto warm = quantiles({1,1,1,1,1,1,1,1,1,1});
  printf("{\\\"even_p50\\\":%.1f,\\\"odd_p50\\\":%.1f,\\\"injected_cold_max\\\":%.1f,\\\"warm_only_max\\\":%.1f}\\n",
      even.p50, odd.p50, mixed.max, warm.max);
}
'''
    with tempfile.TemporaryDirectory(prefix='mhgp12-m6-audit-') as tmp:
        path = pathlib.Path(tmp)
        (path / 'quantiles.cpp').write_text(host)
        checked_run(['g++', '-std=c++20', '-O2', '-Wall', '-Wextra', '-Werror',
                     str(path / 'quantiles.cpp'), '-o', str(path / 'quantiles')])
        quantiles = json.loads(checked_run([str(path / 'quantiles')]).stdout)
        if quantiles['even_p50'] != 5 or quantiles['odd_p50'] != 5:
            raise RuntimeError('Pinned quantile witness changed')
        nvcc = '/usr/local/cuda-12.9/bin/nvcc'
        compilation = checked_run([nvcc, '-std=c++20', '-O3', '-arch=sm_120',
            '-Xcompiler=-Wall,-Wextra,-Werror', str(SOURCE), '-o', str(path / 'm6')])
        cli = []
        for arg in ['--reps=9', '--reps=100001', '--reps=', '--sync=invalid', '--reps='+'9'*42]:
            result = subprocess.run([str(path / 'm6'), arg], text=True, capture_output=True)
            if result.returncode != 2 or result.stdout:
                raise RuntimeError('CLI invalid input accepted')
            cli.append(dict(argument=arg, exit=result.returncode, empty_stdout=True))
    expected = 5.5
    report = dict(source_sha256=hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        scope='host_only_invalid_cli_before_run_no_gpu_workload', quantiles=quantiles,
        expected_even_median=expected, throughput_bias_example=expected/quantiles['even_p50']-1,
        injected_cold_note='Synthetic timings, not measured GPU durations',
        nvcc_compile_exit=compilation.returncode, nvcc_arch='sm_120', cli=cli,
        check_last_error_calls=len(re.findall(r'check\(cudaGetLastError\(', text)),
        kernel_launch_sites=len(re.findall(r'<<<', text)))
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
