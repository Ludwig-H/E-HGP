#!/usr/bin/env python3
"""Source-pinned scalar review: no C++/CUDA execution or hardware claim."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
checks = 0


def require(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


def size_order(sizes):
    # Literal scalar transcription of size_order; only certified m=1..32.
    start = [0] * 34
    for m in sizes:
        if not 1 <= m <= 32:
            raise ValueError('outside certified producer domain')
        start[32 - m + 1] += 1
    for b in range(1, 34):
        start[b] += start[b - 1]
    order = [None] * len(sizes)
    for j, m in enumerate(sizes):
        order[start[32 - m]] = j
        start[32 - m] += 1
    return order


def main():
    before = json.loads((BASE / 'SOURCE_BEFORE.json').read_text())
    require(before['source_commit'] == '00800dd88d5d1368b1987e399a05e87a17f208ae', 'pin')
    sources = {}
    for path, record in before['files'].items():
        data = (BASE / path).read_bytes()
        require(len(data) == record['bytes'], 'source size: ' + path)
        require(hashlib.sha256(data).hexdigest() == record['sha256'], 'source SHA: ' + path)
        sources[path] = data.decode()
    pred = sources['sources/src/catalogue/leaf_device_predicates.hpp']
    cuda = sources['sources/src/catalogue/leaf_batch_cuda.cu']
    host = sources['sources/src/catalogue/leaf_batch.cpp']
    probe = sources['sources/bench/full_probe.cpp']
    single = sources['sources/src/catalogue/single_pass.cpp']
    require('kPrefixBound = 32 + 496 + 4960 + 35960' in pred, 'prefix formula')
    require('kCountBound = u64{kMaxSites} * 3 * kPrefixBound' in pred, 'count formula')
    require('kMaxBatchJobs = u64{1} << 40' in pred, 'job limit')
    require('view.count > view.cloud_sites' not in host + cuda, 'obsolete count/n limit')
    require('view.count > (u64{1} << 32)' in cuda, 'CUDA index guard')
    require(cuda.count('const u64 j = v.order[thread];') == 2, 'both kernels preserve job identities')
    require(cuda.index('reservation.reserve(size, budget)') < cuda.index('cudaMalloc(&data, size)'), 'reserve before allocation')
    require('~DeviceArray() { if (data != nullptr) cudaFree(data); }' in cuda, 'free before member reservation destruction')
    require('order.allocate(view.count, budget)' in cuda and 'threads.allocate(view.count, budget, device_bytes)' in cuda, 'both order arrays budgeted')
    require(cuda.index('t.prefetch_ns = join_prefetch();') < cuda.index('Buffer<u32> order;'), 'join before executor work')
    require('if (p.thread.joinable()) p.thread.join();' in cuda and 'std::thread([&p]' in cuda, 'static prefetch lifetime')
    require(probe.index('Stopwatch full_clock, index_clock;') < probe.index('if (params.cuda_leaves) prefetch_device_context();') < probe.index('auto index = build_index'), 'prefetch within FULL before index')
    require(single.index('if (params.cuda_leaves) prefetch_cuda_context();') < single.index('MHGP11_TRY(prepare_frontier'), 'prefetch before frontier')
    prefix = 32 + 496 + 4960 + 35960
    bound = 32 * 3 * prefix
    require(prefix == 41448, 'P')
    require(bound == 3979008 and bound < 2**22, 'C')
    require(bound * 2**40 < 2**62, 'CPU field sums')
    require(bound * 2**32 < 2**54, 'CUDA field sums')
    require((2**32 - 1).bit_length() == 32, 'largest admitted original job index fits u32')
    require((2**32 + 127) // 128 == 2**25, 'grid fits u32; hardware limit not tested')
    require(4 * 2**32 < 2**64 and 2 * 2**40 < 2**64, 'order/host factors do not overflow u64')
    require(80 <= 2**40 and 80 <= 2**32 and 80 > 9, 'previous causal 80/9 frontier no longer rejected by count domain')
    cases = [[], [1], [32], [1,32], [32,1], [4,4,1,32,4,32], list(range(1,33)), list(range(32,0,-1)), [9]*80]
    cases += [[1+(i*17+j*13)%32 for i in range(j)] for j in range(1,33)]
    for sizes in cases:
        order = size_order(sizes)
        require(order == sorted(range(len(sizes)), key=lambda j: -sizes[j]), 'stable descending size sort')
        require(sorted(order) == list(range(len(sizes))), 'permutation')
        # Count/fill visit reordered threads but read/write original-job slots.
        counts = [(j*7)%11 for j in range(len(sizes))]
        got = [None] * len(sizes)
        for j in order:
            got[j] = counts[j]
        require(got == counts, 'original-job count slots')
        starts, at = [], 0
        for n in counts:
            starts.append(at)
            at += n
        records = [None] * at
        for j in order:
            records[starts[j]:starts[j]+counts[j]] = [(j,q) for q in range(counts[j])]
        require(records == [(j,q) for j,n in enumerate(counts) for q in range(n)], 'original-job fill order')
    print(json.dumps({'verdict':'source_and_scalar_model_conforme','source_commit':before['source_commit'],'checks':checks,'order_cases':len(cases),'bounds':{'P':prefix,'C':bound,'CPU_jobs_max':2**40,'CUDA_jobs_max':2**32,'CPU_field_upper':bound*2**40,'CUDA_field_upper':bound*2**32,'CUDA_grid_at_max':2**25},'scope':'stdlib scalar order/arithmetic plus source bindings; no native/CUDA/GCP; no runtime qualification'},sort_keys=True))


if __name__ == '__main__':
    main()
