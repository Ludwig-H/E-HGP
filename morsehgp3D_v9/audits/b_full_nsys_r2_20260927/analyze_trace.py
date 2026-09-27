#!/usr/bin/env python3
"""Summarize a closed Nsight SQLite export, read-only; no FULL time inference."""
import argparse
import hashlib
import heapq
import json
from pathlib import Path
import re
import sqlite3


TABLES = {
    'kernel': 'CUPTI_ACTIVITY_KIND_KERNEL',
    'memcpy': 'CUPTI_ACTIVITY_KIND_MEMCPY',
    'memset': 'CUPTI_ACTIVITY_KIND_MEMSET',
}
S2 = re.compile(r'\b(rectangle|tile_mass|representative|pair|flag|scatter)_kernel\b')
S4 = re.compile(r'\blanes_(plan|fill|task|replay|answer|gather)_kernel\b')


def need(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def family(name):
    if re.search(r'\bcertificate_kernel\b', name):
        return 'certificates'
    if S4.search(name):
        return 'S4_lanes'
    if S2.search(name):
        return 'S2_filter'
    return 'CUB_unassigned' if 'cub::' in name else 'other_kernel'


class Intervals:
    """Accept nondecreasing starts; retain the maximum end of the whole prefix."""
    def __init__(self):
        self.count = self.total = self.union = self.bytes = 0
        self.first = self.end = self.previous_start = None
        self.gaps = []

    def add(self, start, end, size=0):
        need(all(type(v) is int for v in (start, end, size)), 'noninteger activity')
        need(end >= start and size >= 0, 'invalid interval/bytes')
        need(self.previous_start is None or start >= self.previous_start, 'unsorted intervals')
        if self.first is None:
            self.first = start
        if self.end is not None and start > self.end:
            heapq.heappush(self.gaps, (start-self.end, self.end, start))
            if len(self.gaps) > 10:
                heapq.heappop(self.gaps)
        self.union += max(0, end-max(start, self.end if self.end is not None else start))
        self.end = max(end, self.end if self.end is not None else end)
        self.previous_start = start
        self.count += 1
        self.total += end-start
        self.bytes += size

    def result(self):
        span = 0 if self.first is None else self.end-self.first
        return dict(count=self.count, summed_ns=self.total, union_ns=self.union,
                    span_ns=span, internal_gaps_ns=span-self.union, bytes=self.bytes,
                    first_ns=self.first, last_ns=self.end,
                    largest_internal_gaps=[dict(duration_ns=d, start_ns=s, end_ns=e)
                                           for d, s, e in sorted(self.gaps, reverse=True)])


def summarize(connection):
    tables = {row[0] for row in connection.execute("SELECT name FROM sqlite_master WHERE type='table'")}
    parts, warnings, counts = [], [], {}
    for kind, table in TABLES.items():
        if table not in tables:
            warnings.append(table + ' absent: empty lazy export or missing collection; not inferred complete')
            continue
        columns = {row[1] for row in connection.execute("PRAGMA table_info('" + table + "')")}
        need({'deviceId', 'start', 'end'} <= columns, 'missing activity columns: ' + table)
        join, name, size = '', "'" + kind + "'", '0'
        if kind == 'kernel':
            column = next((c for c in ('demangledName', 'shortName') if c in columns), None)
            if column and 'StringIds' in tables:
                join = ' LEFT JOIN StringIds s ON s.id=a.' + column
                name = "COALESCE(s.value, '<unresolved>')"
            else:
                name = "'<unresolved>'"
                warnings.append('kernel names unavailable; family attribution incomplete')
        else:
            need('bytes' in columns, 'missing transfer bytes: ' + table)
            size = 'a.bytes'
            if kind == 'memcpy':
                need('copyKind' in columns, 'missing copyKind')
                name = "printf('copyKind=%d',a.copyKind)"
                if 'ENUM_CUDA_MEMCPY_OPER' in tables:
                    join = ' LEFT JOIN ENUM_CUDA_MEMCPY_OPER c ON c.id=a.copyKind'
                    name = "COALESCE(c.label,printf('copyKind=%d',a.copyKind))"
        parts.append("SELECT a.deviceId,a.start,a.end,'" + kind + "'," + name + ',' + size +
                     ' FROM ' + table + ' a' + join)
        counts[table] = connection.execute('SELECT COUNT(*) FROM ' + table).fetchone()[0]
    need(parts, 'no CUDA activity tables')
    devices, unresolved = {}, 0
    query = ' UNION ALL '.join(parts) + ' ORDER BY 1,2,3'
    for device, start, end, kind, name, size in connection.execute(query):
        need(type(device) is int and device >= 0 and isinstance(name, str), 'invalid device/name')
        state = devices.setdefault(device, dict(all=Intervals(), kinds={}, families={}, operations={}))
        state['all'].add(start, end, size)
        state['kinds'].setdefault(kind, Intervals()).add(start, end, size)
        state['operations'].setdefault((kind, name), Intervals()).add(start, end, size)
        if kind == 'kernel':
            state['families'].setdefault(family(name), Intervals()).add(start, end)
            unresolved += name == '<unresolved>'
    if unresolved:
        warnings.append(str(unresolved) + ' unresolved kernel names')
    need(sum(s['all'].count for s in devices.values()) == sum(counts.values()), 'activity join multiplied rows')
    if not devices:
        warnings.append('no CUDA activity rows; no GPU timing conclusion')
    output = {}
    for device, state in sorted(devices.items()):
        output[str(device)] = dict(all_activity=state['all'].result(),
            kinds={k: v.result() for k, v in sorted(state['kinds'].items())},
            kernel_families={k: v.result() for k, v in sorted(state['families'].items())},
            operations=[dict(kind=k, name=n, **v.result()) for (k, n), v in
                        sorted(state['operations'].items(), key=lambda pair: (-pair[1].total, pair[0]))])
    return dict(schema='mhgp9_nsys_sqlite_activity_v1', scope='observed_cuda_activity_only',
                completeness='not_certified_by_this_reader', table_counts=counts, devices=output,
                warnings=warnings, notes=[
                    'Times are integer nanoseconds; sums may overlap. Only all_activity.union_ns merges every kind.',
                    'Kind/family gaps mean absence of that category, not GPU idleness.',
                    'CUB scans occur in S2 and S4; CUB_unassigned is not automatically charged to either.',
                    'Gaps cover only first-to-last observed activity, not process leading/trailing time.',
                    'Without NVTX phase markers and CPU sampling, gaps do not identify CPU FULL phases.',
                    'No FULL duration is inferred by subtracting GPU activity from process time.',
                    'Digests are outside the chain stopwatch but remain in the profiled process.',
                    'Cold/warm passes and frames are not segmented without independently established boundaries.',
                    'Observed CUDA activity is not a measurement of SM occupancy or global GPU idleness.',
                    'Dropped-event diagnostics are not inspected here; check profiler warnings separately.',
                    'Family names are a source-name classification, not a new correctness or performance certificate.'])


def analyze(path):
    path = path.resolve(strict=True)
    need(path.is_file(), 'SQLite input must be a file')
    wal = Path(str(path) + '-wal')
    need(not wal.exists() or wal.stat().st_size == 0, 'closed checkpointed export required; nonempty WAL')
    before = digest(path)
    # immutable prevents SQLite from creating sidecars; the closed input is
    # checked again below, not silently treated as a live export.
    connection = sqlite3.connect(path.as_uri() + '?mode=ro&immutable=1', uri=True)
    try:
        connection.execute('PRAGMA query_only=ON')
        result = summarize(connection)
    finally:
        connection.close()
    need(digest(path) == before, 'SQLite changed during analysis')
    need(not wal.exists() or wal.stat().st_size == 0, 'WAL appeared during analysis')
    return dict(result, sqlite_sha256=before, sqlite_bytes=path.stat().st_size)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('sqlite', type=Path)
    parser.add_argument('--output', type=Path, help='new JSON file; existing paths are refused')
    args = parser.parse_args(argv)
    report = json.dumps(analyze(args.sqlite), indent=2, sort_keys=True, allow_nan=False) + '\n'
    if args.output is None:
        print(report, end='')
    else:
        with args.output.open('x', encoding='utf-8') as stream:
            stream.write(report)


if __name__ == '__main__':
    main()
