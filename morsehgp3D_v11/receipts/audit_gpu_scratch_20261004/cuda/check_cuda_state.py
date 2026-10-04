#!/usr/bin/env python3
"""Pinned source bindings and scalar state/warp models; no product/GPU calls."""
import hashlib
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
PIN = '22a6af6aa6c57302b3eac5adcb7f2b52c0f1e0b4'
checks = 0


def require(value, why):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(why)


def scratch_case(j, sizes, resolved):
    # Scalar transcription of ScratchSink counting/storage, using local-rank tokens.
    records = [None] * 128
    population = [None] * 1024
    balls = incidences = 0
    fits = True
    expected_records, expected_population = [], []
    for e, need in enumerate(sizes):
        require(2 <= need <= 32, 'valid model population per emission')
        record = (j, e)
        values = [(j, e, i % 32) for i in range(need)]
        expected_records.append(record)
        expected_population.extend(values)
        if fits and balls < 128 and incidences + need <= 1024:
            records[balls] = record
            population[incidences:incidences + need] = values
        else:
            fits = False
        balls += 1
        incidences += need
    stored = resolved and fits
    published_balls = balls if resolved else 0
    published_inc = incidences if resolved else 0
    require(stored == (resolved and balls <= 128 and incidences <= 1024), 'stored iff entire resolved sequence fits')
    if stored:
        require(records[:balls] == expected_records, 'stored records all written')
        require(population[:incidences] == expected_population, 'stored population all written')
    if not resolved:
        require(published_balls == published_inc == 0 and not stored, 'partial unresolved sequence discarded')
    return {'resolved':resolved, 'stored':stored, 'balls':published_balls, 'incidences':published_inc,
            'records':records, 'population':population, 'expected_records':expected_records,
            'expected_population':expected_population}


def warp_sum(values):
    require(len(values) == 32, 'all 32 lanes represented')
    current = list(values)
    for offset in (16, 8, 4, 2, 1):
        # NVIDIA shuffle-down: out-of-range source lane returns caller's own value.
        current = [v + current[i + offset if i + offset < 32 else i]
                   for i, v in enumerate(current)]
    return current[0], max(current)


def main():
    before = json.loads((BASE/'SOURCE_BEFORE.json').read_text())
    require(before['source_commit'] == PIN, 'source pin')
    sources = {}
    for name, record in before['files'].items():
        data = (BASE/name).read_bytes()
        require(hashlib.sha256(data).hexdigest() == record['sha256'], 'source SHA '+name)
        require(len(data) == record['bytes'], 'source length '+name)
        sources[name] = data.decode()
    hdr = sources['sources/src/catalogue/leaf_batch.hpp']
    cu = sources['sources/src/catalogue/leaf_batch_cuda.cu']
    host = sources['sources/src/catalogue/leaf_batch.cpp']
    pred = sources['sources/src/catalogue/leaf_device_predicates.hpp']
    require('kScratchRecords = 128, kScratchPopulation = 1024' in hdr, 'scratch capacities')
    require('fits && balls < kScratchRecords && incidences + need <= kScratchPopulation' in hdr, 'storage guard')
    require('stored[j] = s == leaf_device::kOk && sink.fits;' in cu, 'CUDA stored guard')
    require('stored[j] = resolved && sink.fits;' in host, 'host stored guard')
    require('s == leaf_device::kOk ? sink.balls : 0' in cu and 's == leaf_device::kOk ? sink.incidences : 0' in cu, 'unresolved counts zero')
    require('j >= count || !stored[j] || balls[j] == 0' in cu, 'copy domain')
    require('status[j] == leaf_device::kOk && balls[j] != 0 && !stored[j]' in cu, 'replay selection domain')
    require('constexpr int kThreads = 32;' in cu and '__shfl_down_sync(0xffffffffu, value, offset)' in cu, 'warp source anchors')
    require('if (s == leaf_device::kOk) counts_to_array(c, local);' in cu, 'ledger excludes unresolved')
    fill_body = cu.split('__global__ void fill_kernel(',1)[1].split('// Feuille resolue',1)[0]
    require('totals' not in fill_body and 'counts_to_array' not in fill_body, 'fill does not double ledger')
    require(cu.index('reservation.reserve(size, budget)') < cu.index('cudaMallocAsync('), 'reserve before CUDA allocation')
    require('~DeviceArray() { if (data != nullptr) cudaFreeAsync(data, 0); }' in cu, 'async free enqueued before reservation destruction')
    require('cudaMemPoolAttrReleaseThreshold, &keep' in cu and 'u64 keep = ~u64{0};' in cu, 'pool retention explicit')
    require('scratch_records.allocate(view.count * kScratchRecords, budget, device_bytes)' in cu and 'scratch_population.allocate(view.count * kScratchPopulation, budget, device_bytes)' in cu, 'scratch budgeted')
    require('if (!ok(cudaGetLastError()) || !ok(cudaDeviceSynchronize()))' in cu, 'count/write synchronization source anchors')
    require('if (fill_errors != 0) return fail(Reason::catalogue_invariant);' in cu, 'fill refusal checked')
    require('kMaxBatchJobs = u64{1} << 40' in pred and 'view.count > (u64{1} << 32)' in cu, 'limits source anchors')
    cases = [([],True), ([2],True), ([2]*128,True), ([2]*129,True),
             ([32]*32,True), ([32]*33,True), ([2]*3,False), ([2]*129,False)]
    cases += [([2 + (i*13+j)%31 for i in range(j)],j%5 != 0) for j in range(1,180)]
    leaves = [scratch_case(j, seq, resolved) for j,(seq,resolved) in enumerate(cases)]
    rb, pb, r_total, p_total = [], [], 0, 0
    for leaf in leaves:
        rb.append(r_total); pb.append(p_total)
        r_total += leaf['balls']; p_total += leaf['incidences']
    output_records, output_population = [None]*r_total, [None]*p_total
    copied, replayed, unresolved = [], [], []
    # Different count/fill launch order, with all outputs still indexed by original leaf j.
    order = sorted(range(len(leaves)), key=lambda j: -(j%32+1))
    for j in range(len(leaves)):
        leaf=leaves[j]
        if not leaf['resolved']:
            unresolved.append(j)
        elif leaf['stored'] and leaf['balls']:
            copied.append(j)
            output_records[rb[j]:rb[j]+leaf['balls']] = leaf['records'][:leaf['balls']]
            output_population[pb[j]:pb[j]+leaf['incidences']] = leaf['population'][:leaf['incidences']]
    for j in order:
        leaf=leaves[j]
        if leaf['resolved'] and leaf['balls'] and not leaf['stored']:
            replayed.append(j)
            output_records[rb[j]:rb[j]+leaf['balls']] = leaf['expected_records']
            output_population[pb[j]:pb[j]+leaf['incidences']] = leaf['expected_population']
    require(not(set(copied)&set(replayed)), 'copy and replay disjoint')
    require(set(copied)|set(replayed) == {j for j,l in enumerate(leaves) if l['resolved'] and l['balls']}, 'all resolved emitting leaves covered')
    require(not(set(unresolved)&(set(copied)|set(replayed))), 'unresolved partial slots ignored')
    require(output_records == [x for l in leaves if l['resolved'] for x in l['expected_records']], 'records unchanged by scratch/replay')
    require(output_population == [x for l in leaves if l['resolved'] for x in l['expected_population']], 'population unchanged by scratch/replay')
    require(None not in output_records and None not in output_population, 'no read from unwritten scalar slot')
    c=32*3*(32+496+4960+35960)
    for active in range(33):
        for field in range(15):
            vals=[(c-1 if field==0 else (lane*7919+field*104729)%(c)) if lane<active else 0 for lane in range(32)]
            actual,max_value=warp_sum(vals)
            require(actual==sum(vals), 'lane0 exact reduction incl partial block')
            require(max_value<=32*(c-1)<2**64, 'all scalar intermediates fit u64')
    require(c*2**40<2**62 and c*2**32<2**54, 'batch sum bounds')
    require(128*8+1024==2048, 'slot bytes')
    require(2048*2**40==2**51 and 2048*2**32==2**43, 'slot products fit u64')
    # Advisory domain edge only: no allocating 8TiB, no injected GPU failure.
    require((2**32 & (2**32-1))==0, 'unsigned error counter wraps at 2^32')
    print(json.dumps({'verdict':'source_and_scalar_review_favorable','source_commit':PIN,'checks':checks,
      'scratch_model_leaves':len(leaves),'stored_nonempty_leaves':len(copied),'replayed_leaves':len(replayed),
      'unresolved_leaves_ignored':len(unresolved),'records':r_total,'population':p_total,
      'warp_model_cases':33*15,'scratch_slot_bytes':2048,'advisory_error_counter_edge':{'errors':2**32,'unsigned32_result':0,'scratch_bytes_at_count':2**43,'G4_reproduction':False},
      'scope':'stdlib scalar state/warp model and frozen-source bindings, not C++/CUDA/CUB/runtime qualification'},sort_keys=True))


if __name__=='__main__':
    main()
