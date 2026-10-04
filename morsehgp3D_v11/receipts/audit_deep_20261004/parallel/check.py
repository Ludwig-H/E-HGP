#!/usr/bin/env python3
"""Bounded review of scratch ownership and pipeline ordering; not a native test."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
checks = 0


def require(value, message):
    global checks
    checks += 1
    if not value:
        raise RuntimeError(message)


def source(rel):
    return (HERE / "sources" / rel).read_text()


def main():
    records = json.loads((HERE / "SOURCES.json").read_text())
    for entry in records["sources"]:
        rel = entry["path"].removeprefix("morsehgp3D_v11/")
        require(hashlib.sha256((HERE / "sources" / rel).read_bytes()).hexdigest() == entry["sha256"], rel)

    clauses = {
        "src/sched/pool.cpp": [
            "job.next.compare_exchange_weak(begin, end, std::memory_order_relaxed)",
            "outcome = merge(outcome, invoke(job.body, job.context, begin, end, worker))",
            "done_.wait(lock, [&] { return job.remaining == 0; })",
            "active_.test_and_set(std::memory_order_acquire)",
        ],
        "src/tower/forest_parallel.hpp": [
            "if (p.concurrent_orders) return std::min(pool->size(), p.descent_lanes)",
            "std::min({pool->size(), p.descent_lanes, p.regular_batch_capacity})",
        ],
        "src/tower/forest_parallel.cpp": [
            "std::min(lanes_, count_) < pool_->size() ? slot : worker",
            "std::min<u64>(lanes_, total) < pool_->size() ? slot : worker",
            "(next_lane_ + slot) % lanes_",
        ],
        "src/tower/forest_pipeline.cpp": [
            "CensusWorkspace* scratch = parallel.census_slot(task)",
            "if (!one.ok()) { outcome = merge(outcome, one); continue; }",
            ".store(outcome.ok() ? u8{1} : u8{2}, std::memory_order_release)",
            "return tasks - (2 * kmax - 1)",
            "pool.parallel_for(tasks, 1, &pipeline, Pipeline::body)",
        ],
        "src/tower/forest_vertical.cpp": [
            "closed = state.closed.load(std::memory_order_acquire)",
            "nodes = state.nodes.load(std::memory_order_acquire)",
            "sweep.advance(level, work, low.nodes)",
        ],
    }
    for rel, terms in clauses.items():
        text = source(rel)
        for term in terms:
            require(term in text, f"source changed: {rel}: {term}")

    # Algebraic proof: j=min(L,count); if j<W, private scratch index=slot<j;
    # otherwise index=worker<W and j>=W implies L,count>=W. Simultaneous
    # tasks never share a worker; slots and cyclic logical lanes are injective.
    sizes = (1, 2, 4, 8, 24, 48, 256)
    capacities = (1, 2, 4, 8, 64, 4096)
    cases = 0
    for workers in sizes:
        for lanes in sizes:
            for capacity in capacities:
                reserved = min(workers, lanes, capacity)
                for count in sorted({1, capacity, max(1, capacity - 1), min(capacity, lanes)}):
                    jobs = min(lanes, count)
                    used = set(range(jobs if jobs < workers else workers))
                    require(all(0 <= i < reserved for i in used), "batch scratch index")
                    for rotation in (0, lanes - 1):
                        logical = [(rotation + slot) % lanes for slot in range(jobs)]
                        require(len(set(logical)) == jobs, "memo lane alias")
                    cases += 1
            reserved = min(workers, lanes)
            for count in (1, lanes, lanes + 1, 4096):
                jobs = min(lanes, count)
                used = set(range(jobs if jobs < workers else workers))
                require(all(0 <= i < reserved for i in used), "concurrent-order scratch index")
                cases += 1

    # Dependencies: resolve -> publication -> followed sweep. Every awaited
    # producer has a smaller task index. The Pool keeps claiming tasks after
    # an Outcome failure; resolve publishes each claimed block even on failure.
    pipeline_cases = 0
    for workers in sizes:
        for lanes_limit in sizes:
            for k in range(2, 33):
                for reuse in (False, True):
                    tasks = min(workers, lanes_limit) if reuse else workers
                    enabled = workers >= 2 * k and tasks >= 2 * k
                    if not enabled:
                        continue
                    resolvers = tasks - (2 * k - 1)
                    publishers = range(resolvers, resolvers + k)
                    followers = range(resolvers + k, tasks)
                    require(resolvers >= 1 and tasks <= workers, "pipeline admission")
                    require(not reuse or tasks <= min(workers, lanes_limit), "task-owned scratch")
                    require(all(r < p for r in range(resolvers) for p in publishers), "publish dependency")
                    require(all(p < f for p in publishers for f in followers), "follow dependency")
                    pipeline_cases += 1

    # Source-confirmed abandonment hole. A terminal abandoned view satisfies
    # readiness with closed=kNone. A guard *inside* the false-ready loop cannot
    # guard the dependent access after a wake-up. This model does not observe
    # a native race or an erroneous successful FULL result.
    none = 2**32 - 1
    for level in (0, 1, 2, 31, 255, 4096, 2**24, none - 2):
        before = dict(done=False, closed=level, abandoned=False)
        after = dict(done=False, closed=none, abandoned=True)
        ready = lambda v: v["done"] or level < v["closed"]
        require(not ready(before) and ready(after), "abandon wakes ready predicate")
        require(after["abandoned"], "guard needed after loop")
    print(json.dumps({"checks": checks, "scratch_cases": cases,
                      "pipeline_cases": pipeline_cases, "native_execution": False}, sort_keys=True))


if __name__ == "__main__":
    main()
