#!/usr/bin/env python3
"""Owned fixed-pivot navigators and inherited witness states; no fast bounds."""
import hashlib
import importlib.util
import json
from pathlib import Path

BASE = Path(__file__).resolve().parent
PREVIOUS = BASE.parent/'q34_global_contract_20260921/prefix_model.py'
spec = importlib.util.spec_from_file_location('previous_prefix_model', PREVIOUS)
old = importlib.util.module_from_spec(spec)
spec.loader.exec_module(old)
require = old.require
bump = old.bump


class Navigator:
    def __init__(self, n, pivot, kind):
        self.nodes, self.n, self.pivot, self.kind = old.tree(n), n, pivot, kind
        self.path, self.siblings, self.suffix = [], [], 0
        node = 0
        while True:
            self.path.append(node)
            lo, hi, left, right, _ = self.nodes[node]
            if left < 0:
                break
            if pivot < self.nodes[right][0]:
                self.siblings.append(right)
                node = left
            else:
                self.siblings.append(left)
                self.suffix = right
                node = right
        if kind == 'circular':
            self.order = list(range(pivot, n))+list(range(pivot))
        else:
            def visit(node):
                lo, hi, left, right, _ = self.nodes[node]
                if left < 0:
                    return [lo]
                children = [left, right]
                if lo <= pivot < hi and pivot >= self.nodes[right][0]:
                    children.reverse()
                return visit(children[0])+visit(children[1])
            self.order = visit(0)
        # Test scaffolding ONLY: the proposed navigator does not store this map.
        positions = {rank: pos for pos, rank in enumerate(self.order)}
        self.starts = [min(positions[r] for r in range(node[0], node[1])) for node in self.nodes]

    def initial(self):
        # cursor, phase (0 path/suffix, 1 siblings/prefix, 2 done), depth/slot
        return [self.suffix, 0, 0] if self.kind == 'circular' else [0, 0, 0]

    def position(self, state):
        cursor, phase, _ = state
        if phase == 2:
            return self.n
        first = self.nodes[cursor][0]
        if self.kind == 'circular':
            return first-self.pivot if phase == 0 else self.n-self.pivot+first
        return self.starts[cursor]

    def cut(self, state):
        cursor, phase, _ = state
        return self.kind == 'circular' and phase == 1 and self.nodes[cursor][1] > self.pivot

    def descend(self, state):
        cursor, phase, slot = state
        require(self.nodes[cursor][2] >= 0, 'cannot descend a leaf')
        if self.kind == 'pivot_path' and phase == 0:
            state[:] = [self.path[slot+1], 0, slot+1]
        else:
            state[0] = self.nodes[cursor][2]

    def consume(self, state, mutant=''):
        cursor, phase, slot = state
        if self.kind == 'circular':
            cursor = self.nodes[cursor][4]
            if cursor == len(self.nodes) and phase == 0:
                cursor, phase = 0, 1
            if phase == 1 and (cursor == len(self.nodes) or self.nodes[cursor][0] >= self.pivot):
                phase = 2
            state[:] = [cursor, phase, 0]
        else:
            if phase == 0:
                slot -= 1
                state[:] = [self.siblings[slot], 1, slot] if slot >= 0 else [0, 2, 0]
            else:
                cursor = self.nodes[cursor][4]
                if cursor == self.nodes[self.siblings[slot]][4] and mutant != 'forget_sibling_boundary':
                    slot -= 1
                    state[:] = [self.siblings[slot], 1, slot] if slot >= 0 else [0, 2, 0]
                elif cursor == len(self.nodes):
                    state[:] = [0, 2, 0]
                else:
                    state[0] = cursor


def structural(n, pivot, kind, policy, mutant=''):
    nav = Navigator(n, pivot, kind)
    state, output, cuts, steps = nav.initial(), [], 0, 0
    while state[1] != 2:
        steps += 1
        require(steps < 8*n+16, 'navigator failed to terminate')
        require(len(output) == nav.position(state), 'wrong consumed prefix position')
        cursor = state[0]
        lo, hi, left, _, _ = nav.nodes[cursor]
        if nav.cut(state) and mutant != 'test_before_cut':
            nav.descend(state)
            cuts += 1
        elif left >= 0 and (policy == 0 or (cursor+policy) % 3 == 0):
            nav.descend(state)
        else:
            # Collect the node's population in this fixed order, not ID order.
            output.extend(rank for rank in nav.order if lo <= rank < hi)
            nav.consume(state, mutant)
    require(output == nav.order, 'navigator omits or duplicates population')
    return cuts


def check_prefix(points, nav, task, k, work):
    a, b, lanes, _ = task
    for slot, (count, state, rejected) in enumerate(lanes):
        if rejected:
            continue
        prefix = nav.order[:nav.position(state)]
        require(0 <= count < k-1-slot, 'prefix threshold')
        for i in a:
            for j in b:
                actual = sum(old.witness(points[i], points[j], points[z], slot+3) for z in prefix)
                require(actual == count, 'nonuniform inherited prefix')
                bump(work, 'prefix_pair_checks')


def geometry(points, k, pivot, kind, quantum):
    nav = Navigator(len(points), pivot, kind)
    pending = [[a,b,[[0,nav.initial(),k-1-slot <= 0] for slot in (0,1)],turn]
               for a,b,_,turn in old.initial(len(points),k)['pending']]
    work, records, steps = {}, [], 0
    while pending:
        task = pending.pop()
        a,b,lanes,turn = task
        check_prefix(points,nav,task,k,work)
        active = [i for i in (turn,1-turn) if not lanes[i][2] and lanes[i][1][1] != 2]
        if not active:
            mask = sum(1 << (i+1) for i in (0,1) if not lanes[i][2])
            records.extend([i,j,mask] for i in a for j in b if mask)
            continue
        slot = active[0]
        count,state,_ = lanes[slot]
        task[3] = 1-slot
        cursor = state[0]
        first,last,left,_,_ = nav.nodes[cursor]
        if nav.cut(state):
            nav.descend(state)
            bump(work,'structural_cuts')
            pending.append(task)
        else:
            values = [old.witness(points[i],points[j],points[z],slot+3)
                      for i in a for j in b for z in range(first,last)]
            bump(work,'enumerated_predicates',len(values))
            if all(values) or not any(values):
                if all(values):
                    lanes[slot][0] += last-first
                    if lanes[slot][0] >= k-1-slot:
                        lanes[slot][2] = True
                nav.consume(state)
                pending.append(task)
            elif left >= 0:
                nav.descend(state)
                pending.append(task)
            else:
                require(len(a)*len(b)>1,'singleton must be decided')
                bump(work,'product_splits')
                bump(work,'split_after_wrap',any(x[1][1] == 1 and not x[2] for x in lanes))
                bump(work,'split_with_different_cursors',lanes[0][1] != lanes[1][1])
                bump(work,'split_with_eof',any(x[1][1] == 2 and not x[2] for x in lanes))
                factor = 0 if len(a) >= len(b) else 1
                ids = task[factor]
                for part in (ids[:len(ids)//2],ids[len(ids)//2:]):
                    child = [list(a),list(b),[[c,list(s),r] for c,s,r in lanes],task[3]]
                    child[factor] = part
                    pending.append(child)
        steps += 1
        if pending and steps % quantum == 0:
            pending = json.loads(json.dumps(pending))
            bump(work,'pauses')
    result = sorted(records)
    require(len(result) == len({(x[0],x[1]) for x in result}),'duplicate pair')
    require(result == old.oracle(points,k),'global witness oracle differs')
    work['steps'] = steps
    return work


def main():
    structural_count, cuts = 0, 0
    for n in range(1,49):
        for pivot in range(n):
            for kind in ('circular','pivot_path'):
                for policy in (0,1,2):
                    cuts += structural(n,pivot,kind,policy)
                    structural_count += 1
    killed = {}
    for mutant,kind in [('test_before_cut','circular'),('forget_sibling_boundary','pivot_path')]:
        for n in range(2,9):
            for pivot in range(n):
                try:
                    structural(n,pivot,kind,0 if kind == 'pivot_path' else 1,mutant)
                except RuntimeError as error:
                    killed[mutant] = dict(n=n,pivot=pivot,error=str(error))
                    break
            if mutant in killed:
                break
        require(mutant in killed,'surviving navigator mutant')
    cases = old.fixtures()
    names = ['line','triangle_contact','tetrahedron_contact','credit_then_ambiguity','u16','cube','random_3','random_6']
    totals, executions = {}, 0
    for name in names:
        points=cases[name]
        for pivot in sorted({0,1,len(points)//2,len(points)-1}):
            for kind in ('circular','pivot_path'):
                for k in (1,2,3,5,10):
                    baseline = None
                    for quantum in (1,7,1000000):
                        work=geometry(points,k,pivot,kind,quantum)
                        work.pop('pauses',None)
                        if baseline is None:
                            baseline=work
                            for key,value in work.items():
                                bump(totals,kind+'.'+key,value)
                        require(work==baseline,'pauses change work')
                        executions += 1
    for kind in ('circular','pivot_path'):
        for key in ('product_splits','split_after_wrap','split_with_different_cursors','split_with_eof'):
            require(totals[kind+'.'+key] > 0,'vacuous inherited branch')
    print(json.dumps(dict(status='passed',source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        reused_model_sha256=hashlib.sha256(PREVIOUS.read_bytes()).hexdigest(),structural_walks=structural_count,
        structural_cuts=cuts,geometry_executions=executions,work_once_per_configuration=totals,mutants=killed,
        scope='Exact navigator/prefix model with exhaustive Boolean certificates, not a product or speed test'),sort_keys=True))


if __name__=='__main__':
    main()
