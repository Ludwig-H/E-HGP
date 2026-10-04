#!/usr/bin/env python3
"""Scalar disjoint-write and lifetime review; no C++/CUDA execution."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
CHECKS = 0


def need(ok, why):
    global CHECKS
    CHECKS += 1
    if not ok:
        raise ValueError(why)


meta = json.loads((ROOT / 'BEFORE.json').read_text())
sources = {}
for row in meta['files']:
    raw = (ROOT / row['copy']).read_bytes()
    need(hashlib.sha256(raw).hexdigest() == row['sha256'], 'source hash')
    need(len(raw) == row['bytes'], 'source size')
    sources[row['path'].split('morsehgp3D_v11/', 1)[1]] = raw.decode()
batch = sources['src/catalogue/single_pass_batch.cpp']
single = sources['src/catalogue/single_pass.cpp']
queue = sources['src/catalogue/leaf_queue.hpp']
pool = sources['src/sched/pool.cpp']
cuda = sources['src/catalogue/leaf_batch_cuda.cu']
for fragment in (
    'gathering.offsets[2 * i] = job_count;',
    'gathering.offsets[2 * i + 1] = site_count;',
    'checked_add(job_count, queues[i]->jobs())',
    'checked_add(site_count, queues[i]->sites())',
    'self.jobs.subspan(job_at, q->jobs())',
    'self.sites.subspan(site_at, q->sites())',
    'pool.parallel_for(queues.size(), 1, &gathering, Gather::body)',
    'MHGP11_TRY(budget.admit(bytes));',
    'MHGP11_TRY(jobs.allocate(job_count, budget));',
    'MHGP11_TRY(sites.allocate(site_count, budget));',
):
    need(fragment in batch, 'gather binding ' + fragment)
need('checked_add(job.begin, site_offset)' in queue, 'local to global list offset')
for fragment in (
    'kChunk = 16384;',
    'chunk < self.record_chunks',
    'self.records[self.ball_at + i]',
    'out = self.batch.records[i];',
    'checked_add(out.population_begin, self.population_at)',
    '(chunk - self.record_chunks) * kChunk',
    'self.population.data() + self.population_at + first',
    'pool.parallel_for(chunks, 1, &copy, BatchCopy::body)',
    'batch.fallback.compact(records.subspan(ball_at + n, fb)',
    'population.subspan(population_at + p, fp)',
    'population_at + p);',
):
    need(fragment in single, 'batch copy binding ' + fragment)
need(single.index('pool.parallel_for(frontier.size(), 1, &run, SingleRun<Front>::generate_body)')
     < single.index('batch_stage<Capacity>'), 'front joined before gathering')
need(single.index('pool.parallel_for(frontier.size(), 1, &run, SingleRun<Front>::compact_body)')
     < single.index('batch_compact(batch, pool'), 'task copy joined before batch copy')
need('done_.wait(lock, [&] { return job.remaining == 0; });' in pool,
     'all workers acknowledged before callback context destroyed')
need('current_ = nullptr;' in pool and 'active_.test_and_set' in pool,
     'synchronous single active invocation')
need('outcome = merge(outcome, invoke(' in pool,
     'failed task outcomes retained (effects private until successful assembly)')
for fragment in ('if (bytes == 0) return {};', '(bytes + 4095) / 4096',
                 'self.base[page * 4096] = 0;', 'host.allocate(count, budget)',
                 'touch_pages(pool, reinterpret_cast<u8*>(host.data()), count * sizeof(T))',
                 'cudaMemcpy(host.data(), device, count * sizeof(T), cudaMemcpyDeviceToHost)'):
    need(fragment in cuda, 'first-touch binding ' + fragment)
need(cuda.index('MHGP11_TRY(touch_pages(pool') < cuda.index('if (count != 0 && !ok(cudaMemcpy(host.data()'),
     'all CPU touches finish before device-to-host copy')


def orders(n):
    direct = list(range(n))
    return [direct, direct[::-1], direct[::2] + direct[1::2], direct[1::2] + direct[::2]]


# Independent specification: concatenate immutable queues by ordinal, never by claim order.
# Model values are synthetic transport payloads; they do not certify geometric balls.
gather_cases = []
for sizes in [[], [0], [0, 1, 0], [1, 0, 3, 2], [2, 2, 2, 2],
              [i % 4 for i in range(37)], [i % 2 for i in range(1024)]]:
    queues = []
    for q, jobs in enumerate(sizes):
        rows = [tuple(600 + q * 100 + j * 33 + r for r in range(1 + (q + j) % 32))
                for j in range(jobs)]
        queues.append(rows)
    expected_sites = [s for rows in queues for row in rows for s in row]
    expected_jobs = []
    pos = 0
    for q, rows in enumerate(queues):
        for j, row in enumerate(rows):
            expected_jobs.append((q, j, pos, len(row)))
            pos += len(row)
    offsets = []
    nj = ns = 0
    for rows in queues:
        offsets.append((nj, ns))
        nj += len(rows)
        ns += sum(map(len, rows))
    for claim_order in orders(len(queues)):
        got_jobs, got_sites = [None] * nj, [None] * ns
        writes_j, writes_s = [0] * nj, [0] * ns
        for q in claim_order:
            j0, s0 = offsets[q]
            at = 0
            for j, row in enumerate(queues[q]):
                got_jobs[j0 + j] = (q, j, s0 + at, len(row))
                writes_j[j0 + j] += 1
                for k, value in enumerate(row):
                    got_sites[s0 + at + k] = value
                    writes_s[s0 + at + k] += 1
                at += len(row)
        need(got_jobs == expected_jobs and got_sites == expected_sites, 'ordinal concatenation')
        need(all(x == 1 for x in writes_j + writes_s), 'gather writes disjoint and exhaustive')
        need(all(got_sites[begin:begin + m] == queues[q][j]
                 for q, j, begin, m in got_jobs), 'each job points to its own immutable list')
    gather_cases.append({'queues': len(queues), 'jobs': nj, 'site_entries': ns})


# Independent specification for batch + fallback: concatenate record streams,
# move only each record population offset, preserve all other fields and populations.
K = 16384
copy_cases = []
for n in [0, 1, K - 1, K, K + 1, 2 * K + 1]:
    lengths = [2 + i % 5 for i in range(n)]
    records, pop = [], []
    for i, length in enumerate(lengths):
        records.append(('support-' + str(i), 'exact-level-' + str(i), len(pop)))
        pop.extend((i, k) for k in range(length))
    fallback_records = [('fallback-q2', 'exact-fallback', 0)]
    fallback_pop = [('fallback', 0), ('fallback', 1)]
    ball_at, population_at = 3, 5
    expected_r = [('pre', k, 0) for k in range(ball_at)]
    expected_r += [(support, level, population_at + at) for support, level, at in records]
    expected_r += [(support, level, population_at + len(pop) + at)
                   for support, level, at in fallback_records]
    expected_p = [('pre', k) for k in range(population_at)] + pop + fallback_pop
    rchunks = (n + K - 1) // K
    chunks = rchunks + (len(pop) + K - 1) // K
    for claims in orders(chunks):
        got_r = expected_r[:ball_at] + [None] * (n + len(fallback_records))
        got_p = expected_p[:population_at] + [None] * (len(pop) + len(fallback_pop))
        writes_r, writes_p = [0] * n, [0] * len(pop)
        for chunk in claims:
            if chunk < rchunks:
                first, last = chunk * K, min((chunk + 1) * K, n)
                for i in range(first, last):
                    support, level, at = records[i]
                    got_r[ball_at + i] = (support, level, at + population_at)
                    writes_r[i] += 1
            else:
                first = (chunk - rchunks) * K
                last = min(first + K, len(pop))
                got_p[population_at + first:population_at + last] = pop[first:last]
                for i in range(first, last):
                    writes_p[i] += 1
        # Synchronous return: fallback occupies exactly the suffix, never a batch interval.
        got_r[ball_at + n:] = [(s, l, population_at + len(pop) + at)
                              for s, l, at in fallback_records]
        got_p[population_at + len(pop):] = fallback_pop
        need(got_r == expected_r and got_p == expected_p, 'batch/fallback exact specification')
        need(all(x == 1 for x in writes_r + writes_p), 'batch writes disjoint and exhaustive')
        need(records == [(s, l, at - population_at)
                         for s, l, at in expected_r[ball_at:ball_at + n]], 'source untouched')
    copy_cases.append({'records': n, 'population': len(pop), 'chunks': chunks})


# First-touch writes one distinct byte in every 4 KiB interval. Payload is then
# wholly overwritten by the synchronized fetch; no initialized-byte assumption.
touch_cases = []
for size in [0, 1, 7, 4095, 4096, 4097, 8192, 8193, 65537]:
    pages = (size + 4095) // 4096
    addresses = [page * 4096 for page in range(pages)]
    need(all(0 <= a < size for a in addresses), 'touch inside allocated range')
    need(len(set(addresses)) == pages, 'touches do not overlap')
    need(pages == (0 if size == 0 else 1 + (size - 1) // 4096), 'all intervals touched')
    touch_cases.append({'bytes': size, 'pages': pages})

# Universal download arithmetic at admitted CUDA count2^32 and per-leaf<2^22;
# no large allocation. New LeafRecord8 and population1 bound bytes well below u64.
C = 3979008
for count in [0, 1, 80, 2**32 - 1, 2**32]:
    for width in [1, 8]:
        size = count * C * width
        need(size + 4095 < 2**64, 'ceil and touch arithmetic under u64')
        need(size // 4096 * 4096 < 2**64, 'page offset fits u64')
# FixedPages<SiteIdx> maximum plus Buffer<SiteIdx> maximum cannot wrap on sum.
need(2 * ((2**64 - 1) // 4) < 2**64, 'p+fallback population sum bounded by admitted buffers')
need(2 * ((2**64 - 1) // 8) + K < 2**64, 'record/chunk arithmetic with record type >=8 bytes')

print(json.dumps({'verdict': 'pass', 'checks': CHECKS, 'pin': meta['pin'],
                  'gather': gather_cases, 'copy': copy_cases, 'first_touch': touch_cases,
                  'scope': 'synthetic transport, disjoint writes, scalar capacities and source bindings; not geometry, native race or CUDA timing',
                  'native_runs': 0, 'gcp_actions': 0}, sort_keys=True, indent=2))
