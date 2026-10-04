#!/usr/bin/env python3
"""Bounded source-pinned control model; no compiler, cloud or native execution."""
import json
from pathlib import Path
import re

ROOT = Path(__file__).parent
SRC = ROOT / 'source/morsehgp3D_v11/src/tower'
checks = 0


def need(condition, message):
    global checks
    checks += 1
    if not condition:
        raise ValueError(message)


header = (SRC / 'forest_internal.hpp').read_text()
vertical = (SRC / 'forest_vertical.cpp').read_text()
pipeline = (SRC / 'forest_pipeline.cpp').read_text()
seeds = (SRC / 'regular_vertical_seeds.hpp').read_text()
match = re.search(r'follow_lower_ready\(LevelRank level, bool done, u32 closed\) noexcept \{ return (.*?); \}', header)
need(match is not None, 'current predicate source extracted')
expression = match[1].replace('idx(level)', 'level').replace('||', ' or ')
need(expression == 'done  or  level < closed', 'exact current predicate, no invented rule')
predicate = compile(expression, '<source follow_lower_ready>', 'eval')
finish = header[header.index('void finish(u32 count, bool complete)'):header.index('// Blocs de cellules')]
need(finish.index('(complete ? done : abandoned).store') < finish.index('closed.store(kNone'),
     'abandoned publication precedes sentinel release')
start = vertical.index('while (!follow_lower_ready(level, low.done, low.closed))')
stop = vertical.index('MHGP11_TRY(visit(i, work));', start)
source_loop = vertical[start:stop]
need(source_loop.count('if (low.abandoned) return {};') == 1, 'single abort check inside wait body')
need(source_loop.index('if (low.abandoned)') < source_loop.index('low.block();'), 'abort checked before refresh')
need(source_loop.index('low.block();') < source_loop.index('sweep.advance'), 'refresh then dependent read')
need('birth_image(i, scratch, work)' in source_loop, 'cache dependent read follows the loop')
need('const NodeIdx seed = seeds_[upper_birth.birth_key];' in seeds and 'seeds_[idx(ball)] = seed;' in seeds,
     'read and write of plain seed buffer pinned')
need('if (!one.ok()) { outcome = merge(outcome, one); continue; }' in pipeline,
     'other resolutions may continue after refusal')

NONE = (1 << 32) - 1


def ready(level, done, closed):
    return eval(predicate, {'__builtins__': {}}, {'level': level, 'done': done, 'closed': closed})


def current_wait(level, before, after):
    """Exact relevant branch order in follow; one unblock/refresh."""
    done, closed, abandoned = before
    if not ready(level, done, closed):
        if abandoned:
            return 'abandon'
        done, closed, abandoned = after
        if not ready(level, done, closed):
            return 'wait'
    return 'dependent_read'


def guarded_wait(level, before, after):
    """Suggested guard after the readiness loop, before any dependent read."""
    done, closed, abandoned = before
    if not ready(level, done, closed):
        if abandoned:
            return 'abandon'
        done, closed, abandoned = after
        if not ready(level, done, closed):
            return 'wait'
    return 'abandon' if abandoned else 'dependent_read'


bad = []
for level in (0, 1, 2, 3, 7, 13, 255, 256, NONE - 1):
    before = (False, level, False)  # all merges at level are not yet closed
    after = (False, NONE, True)    # finish(count,false), fully acquired
    need(not ready(level, before[0], before[1]), 'reader really blocked before refusal')
    need(ready(level, after[0], after[1]), 'sentinel currently passes predicate despite abandonment')
    got = current_wait(level, before, after)
    need(got == 'dependent_read', 'current loop skips abort after wakeup')
    need(guarded_wait(level, before, after) == 'abandon', 'suggested guard stops dependent read')
    bad.append({'level': level, 'before': before, 'after': after, 'current': got, 'guarded': 'abandon'})
    # Successful completion remains usable; ordinary published plateaus unchanged.
    for normal in ((True, NONE, False), (False, level + 1, False), (False, level, False)):
        need(current_wait(level, before, normal) == guarded_wait(level, before, normal),
             'guard does not change normal publication decisions')

report = dict(checks=checks, verdict='confirmed_control_protocol_hole', counterexamples=bad,
              scope='source exact bounded sequential wait/refresh branch; not native FULL or TSan execution',
              effect='dependent lower read is permitted after known refusal; successful wrong FULL not established',
              race_scope='seed readiness no longer follows from lower closed rank; a native overlapping-write race has not been reproduced')
print(json.dumps(report, indent=2))
