#!/usr/bin/env python3
"""P1 (validated hints, sequential commit) on abstract phase-A histories.

Scratch, not a receipt.  Transliteration of order_lots / order_block_lean /
order_lot / order_new_node / order_root of full_ball_tower.hpp @ f44a8db03
(same text as 22eb467cb), with:

  A. an INTERLEAVED committer/helper simulation (random scheduler, helper
     ahead or behind, relaxed-style stale reads of `compressed`), in which the
     committer uses a published hint iff hint < prior_count of its lot;
  B. ADVERSARIAL hints: any node of the final successor chain of the anchor
     (stale, fresh or "future" as a load-buffering execution could produce);
  C. MUTANTS / broken protocols: no prior test, hint used as root without
     chasing, uninitialised slot read as 0 (pre-sized array + relaxed, no
     acquire), stale ring slot (hint of another facet), and garbage < prior.

Every run is compared with the plain reference on next, birth_ball, runs,
anchors, the whole flat draft, the nine counters and the live count.
"""
import random
import sys

ABSENT = None


class Fail(Exception):
    pass


def require(cond, reason):
    if not cond:
        raise Fail(reason)


def instance(rng, k1, big=False):
    n0 = rng.randint(1, 7 if not big else 20) if k1 else 0
    M = rng.randint(1, 40 if not big else 160)
    runs, r = [], 0
    for _ in range(M):
        if rng.random() < rng.choice([0.2, 0.5, 0.8]):
            r += 1
        runs.append(r)
    runs = [x - runs[0] for x in runs]
    facets, contrib = [], []
    for j in range(M):
        earlier = [t for t in range(j) if runs[t] < runs[j]]
        fs = []
        c = rng.choice([0, 0, 1, 1, 2, 2, 3, 4])
        for _ in range(c):
            if k1 and (not earlier or rng.random() < 0.5):
                fs.append(("site", rng.randrange(n0)))
            elif earlier:
                # bias to old targets as well (long chains) and duplicates
                t = rng.choice(earlier) if rng.random() < 0.6 else earlier[rng.randrange(max(1, len(earlier) // 4))]
                fs.append(("pos", t))
        facets.append(fs)
        contrib.append(True if not fs else rng.random() < 0.3)
    return dict(n0=n0, runs=runs, facets=facets, contrib=contrib)


class Order:
    """Committer state + write log (global logical clock)."""

    def __init__(self, inst):
        self.inst = inst
        self.nxt, self.comp, self.birth_ball, self.node_run = [], [], [], []
        self.anchors = [ABSENT] * len(inst["runs"])
        self.flat = dict(level=[], batch_begin=[0], parent_begin=[0], parent=[], contribution_begin=[0], contribution=[])
        self.st = dict(anchor_blocks=0, representatives=0, births=0, merges=0, contributions=0,
                       singleton_lots=0, grouped_lots=0, lot_dsu_slots=0, inert_blocks=0)
        self.lot_run = 0
        self.clock = 0
        self.comp_hist = []      # per node: [(time, value)]  (every value ever stored)
        self.init_time = []
        self.anchor_time = [None] * len(inst["runs"])
        self.inv_viol = 0
        self.hops = 0

    def tick(self):
        self.clock += 1
        return self.clock

    def succ_chain(self, p):
        out = [p]
        while self.nxt[p] is not ABSENT:
            p = self.nxt[p]
            out.append(p)
        return out

    def write_comp(self, p, v, check=True):
        # invariant (1): the stored value is on p's CURRENT successor chain
        if check and v not in self.succ_chain(p):
            self.inv_viol += 1
        self.comp[p] = v
        self.comp_hist[p].append((self.tick(), v))

    def new_node(self, parents, birth):
        nid = len(self.nxt)
        self.nxt.append(ABSENT)
        self.comp.append(nid)
        t = self.tick()
        self.comp_hist.append([(t, nid)])
        self.init_time.append(t)
        for p in parents:
            require(self.nxt[p] is ABSENT, "next_written_twice")  # write-once of next
            self.nxt[p] = nid
            self.write_comp(p, nid)
        self.birth_ball.append(birth)
        self.node_run.append(self.lot_run)
        self.st["births" if not parents else "merges"] += 1
        return nid

    def root(self, tok, prior, no_chase=False, skip_prior=False):
        if not skip_prior:
            require(tok < prior, "anchor_not_prior")
        require(tok < len(self.comp), "out_of_bounds")
        if no_chase:
            r = tok
        else:
            r = tok
            while self.comp[r] != r:
                r = self.comp[r]
                self.hops += 1
            while self.comp[tok] != tok:
                nx = self.comp[tok]
                if nx != r:
                    self.write_comp(tok, r)
                else:
                    self.write_comp(tok, r)
                tok = nx
        if not skip_prior:
            require(r < prior and self.nxt[r] is ABSENT, "root_not_prior")
        return r


def commit_steps(o, hint_of=None, mode="prior", facet_log=None):
    """Generator: the committer (today's loop).  Yields between memory events.
    hint_of(c) -> hint or None, read at the facet's consumption.
    mode: 'prior' (P1 rule), 'noprior' (mutant: accept any hint, bypassing
    the < prior test AND order_root's own require), 'nochase' (mutant: hint
    used as root without chasing), 'plain' (no hints)."""
    inst = o.inst
    n0, runs, facets, contrib = inst["n0"], inst["runs"], inst["facets"], inst["contrib"]
    M = len(runs)
    st, flat = o.st, o.flat

    def open_batch(lv):
        flat["level"].append(lv)
        flat["batch_begin"].append(flat["batch_begin"][-1])

    def add_action(parents, contributions):
        flat["parent"].extend(parents)
        flat["parent_begin"].append(len(flat["parent"]))
        flat["contribution"].extend(contributions)
        flat["contribution_begin"].append(len(flat["contribution"]))
        flat["batch_begin"][-1] += 1

    if n0:
        o.lot_run = 0
        open_batch(0)
        for j in range(n0):
            add_action([], [("site", j)])
            st["contributions"] += 1
            o.new_node([], ABSENT)
    o.capacity = n0 + M            # reserve_lean (1228-1229)
    yield ("reserved",)
    ordinal = 0
    begin = 0
    while begin < M:
        end = begin + 1
        while end < M and runs[end] == runs[begin]:
            end += 1
        prior = len(o.nxt)
        o.lot_run = runs[begin] + 1
        blocks = []
        for j in range(begin, end):
            st["anchor_blocks"] += 1
            roots = []
            for (kind, t) in facets[j]:
                st["representatives"] += 1
                c = ordinal
                ordinal += 1
                if kind == "site":
                    anchor = t
                else:
                    require(runs[t] < runs[j], "static_target_not_strict")
                    require(o.anchors[t] is not ABSENT, "closed_anchor_missing")
                    anchor = o.anchors[t]
                tc = o.tick()
                h = hint_of(c) if (hint_of and mode != "plain") else None
                used = False
                if h is not None:
                    if mode == "prior":
                        if h < prior:
                            r = o.root(h, prior)
                            used = True
                        else:
                            r = o.root(anchor, prior)
                    elif mode == "noprior":
                        r = o.root(h, prior, skip_prior=True)
                        used = True
                    elif mode == "nochase":
                        r = o.root(h, prior, no_chase=True, skip_prior=True) if h < prior else o.root(anchor, prior)
                        used = True
                    elif mode == "prior_mono":  # extra cheap check: anchor <= hint < prior
                        if anchor <= h < prior:
                            r = o.root(h, prior)
                            used = True
                        else:
                            r = o.root(anchor, prior)
                    else:
                        raise ValueError(mode)
                else:
                    r = o.root(anchor, prior)
                if facet_log is not None:
                    facet_log.append(dict(c=c, tc=tc, prior=prior, anchor=anchor, hint=h, used=used, root=r))
                roots.append(r)
                yield ("facet", c)
            blocks.append((j, sorted(set(roots)), contrib[j]))
        lv = runs[begin] + 1
        if len(blocks) == 1:
            st["singleton_lots"] += 1
            j, roots, cflag = blocks[0]
            if cflag:
                st["contributions"] += 1
            if len(roots) == 1:
                target = roots[0]
            else:
                if not roots:
                    require(cflag, "distinct_births")
                target = o.new_node(roots, j if not roots else ABSENT)
                yield ("node",)
            if len(roots) != 1 or cflag:
                open_batch(lv)
                add_action(roots, [("ball", j)] if cflag else [])
            else:
                st["inert_blocks"] += 1
            require(o.anchors[j] is ABSENT, "anchor_duplicate")
            o.anchors[j] = target
            o.anchor_time[j] = o.tick()
            yield ("anchor", j)
        else:
            st["grouped_lots"] += 1
            st["lot_dsu_slots"] += len(blocks)
            dsu = list(range(len(blocks)))

            def find(a):
                while dsu[a] != a:
                    dsu[a] = dsu[dsu[a]]
                    a = dsu[a]
                return a
            owners = sorted((p, b) for b in range(len(blocks)) for p in blocks[b][1])
            for i in range(1, len(owners)):
                if owners[i - 1][0] == owners[i][0]:
                    a, b = find(owners[i - 1][1]), find(owners[i][1])
                    dsu[max(a, b)] = min(a, b)
            groups = [[] for _ in blocks]
            for b in range(len(blocks)):
                groups[find(b)].append(b)
            opened = False
            targets = [ABSENT] * len(blocks)
            for g in groups:
                if not g:
                    continue
                parents, contributions = [], []
                for b in g:
                    parents.extend(blocks[b][1])
                    if blocks[b][2]:
                        contributions.append(("ball", blocks[b][0]))
                        st["contributions"] += 1
                parents = sorted(set(parents))
                if len(parents) == 1:
                    target = parents[0]
                else:
                    if not parents:
                        require(len(g) == 1 and len(contributions) == 1, "distinct_births")
                    target = o.new_node(parents, blocks[g[0]][0] if not parents else ABSENT)
                    yield ("node",)
                for b in g:
                    targets[b] = target
                if len(parents) != 1 or contributions:
                    if not opened:
                        open_batch(lv)
                        opened = True
                    add_action(parents, contributions)
                else:
                    st["inert_blocks"] += len(g)
            for b in range(len(blocks)):
                j = blocks[b][0]
                require(o.anchors[j] is ABSENT, "anchor_duplicate")
                o.anchors[j] = targets[b]
                o.anchor_time[j] = o.tick()
                yield ("anchor", j)
        begin = end
    o.live = sum(1 for x in o.nxt if x is ABSENT)
    require(len(o.nxt) <= o.capacity, "capacity_exceeded")  # no reallocation after reserve_lean


def run_plain(inst):
    o = Order(inst)
    log = []
    for _ in commit_steps(o, None, "plain", log):
        pass
    return o, log


def result(o):
    return dict(nxt=o.nxt, birth_ball=o.birth_ball, runs=o.node_run, anchors=o.anchors,
                flat=o.flat, stats=o.st, live=o.live)


def facet_targets(inst):
    out = []
    for j, fs in enumerate(inst["facets"]):
        for (kind, t) in fs:
            out.append((kind, t))
    return out


def interleaved(inst, rng, nosync_p=0.0, ring=0):
    """Committer + one helper under a random scheduler.
    Helper: for each facet ordinal c in order, read anchors[tau] (absent ->
    no hint; it may retry later), chase `compressed` with stale reads (any
    value ever stored in the slot up to now: relaxed, per-slot history), and
    publish hint[c].  With nosync_p > 0, a read of a slot whose initialisation
    the helper did not synchronise with returns 0 (pre-sized zero array read
    with relaxed loads and no acquire): the unsafe protocol.  ring > 0: the
    committer reads a ring of `ring` slots without an ordinal tag."""
    o = Order(inst)
    targets = facet_targets(inst)
    F = len(targets)
    hints = {}          # c -> (time, hint)
    ring_slots = {}     # slot -> hint (untagged)
    log = []
    stats = dict(published=0, used=0, rejected=0, sc_hint_ge_prior=0, non_ancestor_published=0)

    def hint_of(c):
        if ring:
            v = ring_slots.get(c % ring)
            return v
        e = hints.get(c)
        return None if e is None else e[1]

    committer = commit_steps(o, hint_of, "prior", log)
    speed = rng.choice([0.2, 0.5, 0.8, 0.95])  # probability of scheduling the helper
    reserved = False
    last_sync = 0

    def helper_gen():
        nonlocal last_sync
        c = 0
        while c < F:
            kind, t = targets[c]
            if kind == "site":
                a = t
            else:
                a = o.anchors[t]
                if a is None:
                    # not yet committed: skip this facet (or retry once)
                    if rng.random() < 0.5:
                        yield
                        continue
                    c += 1
                    continue
            # acquire point: models an acquire load of the committed count
            if rng.random() < 0.3:
                last_sync = o.clock
            x = a
            steps = 0
            while True:
                if x >= len(o.comp):
                    break  # never follow beyond the committed size
                hist = o.comp_hist[x]
                if nosync_p and o.init_time[x] > last_sync and rng.random() < nosync_p:
                    v = 0  # initialisation not visible: the zero of a pre-sized array
                else:
                    v = rng.choice(hist)[1] if rng.random() < 0.5 else hist[-1][1]
                yield
                steps += 1
                if v == x:
                    break
                x = v
            if ring:
                ring_slots[c % ring] = x
            else:
                hints[c] = (o.clock, x)
            stats["published"] += 1
            c += 1
            yield

    helper = helper_gen()
    done_c = done_h = False
    try:
        while not done_c:
            if not done_h and reserved and rng.random() < speed:
                try:
                    next(helper)
                except StopIteration:
                    done_h = True
            else:
                try:
                    ev = next(committer)
                    if ev[0] == "reserved":
                        reserved = True  # the helper starts only after reserve_lean
                except StopIteration:
                    done_c = True
    except Fail as f:
        return None, str(f), stats, o, log
    # chain property of the published hints (against the final history)
    for e in log:
        if e["hint"] is not None:
            chain = o.succ_chain(e["anchor"])
            if e["hint"] not in chain:
                stats["non_ancestor_published"] += 1
            if e["hint"] >= e["prior"]:
                stats["sc_hint_ge_prior"] += 1
            if e["used"]:
                stats["used"] += 1
            else:
                stats["rejected"] += 1
    return result(o), None, stats, o, log


def adversarial(inst, plain_o, plain_log, rng, mode):
    """Hints from the FINAL successor chain of each anchor (any node, including
    ones created in the facet's own lot or later: the values a load-buffering
    execution or a lagging helper could deliver), or broken hints."""
    hint_map = {}
    F = len(plain_log)
    for e in plain_log:
        chain = plain_o.succ_chain(e["anchor"])
        if mode in ("chain", "chain_noprior", "chain_nochase", "chain_mono"):
            hint_map[e["c"]] = rng.choice(chain)
        elif mode == "zero":
            hint_map[e["c"]] = 0 if rng.random() < 0.3 else rng.choice(chain)
        elif mode == "garbage_lt_prior":
            hint_map[e["c"]] = rng.randrange(e["prior"]) if (e["prior"] and rng.random() < 0.3) else rng.choice(chain)
        elif mode == "other_facet":
            other = plain_log[rng.randrange(F)]
            hint_map[e["c"]] = rng.choice(plain_o.succ_chain(other["anchor"])) if rng.random() < 0.3 else rng.choice(chain)
    cm = {"chain": "prior", "chain_noprior": "noprior", "chain_nochase": "nochase", "chain_mono": "prior_mono",
          "zero": "prior", "garbage_lt_prior": "prior", "other_facet": "prior"}[mode]
    o = Order(inst)
    log = []
    try:
        for _ in commit_steps(o, lambda c: hint_map.get(c), cm, log):
            pass
    except Fail as f:
        return None, str(f), o, log
    return result(o), None, o, log


def main():
    count = int(sys.argv[1]) if len(sys.argv) > 1 else 3000
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 20260926
    rng = random.Random(seed)
    agg = dict(instances=0, plain_fail=0, k1=0, k=0, grouped=0, facets=0, max_hops_path=0,
               inv_viol_plain=0, inv_viol_hinted=0,
               il_ok=0, il_mismatch=0, il_fail=0, il_used=0, il_rejected=0, il_published=0,
               il_sc_hint_ge_prior=0, il_non_ancestor=0,
               il_nosync_mismatch_or_fail=0, il_nosync_runs=0, il_nosync_non_ancestor=0,
               il_ring_mismatch_or_fail=0, il_ring_runs=0,
               adv_chain_ok=0, adv_chain_bad=0, adv_hint_ge_prior=0, adv_hint_used=0,
               adv_mono_ok=0, adv_mono_bad=0)
    mut = {m: dict(runs=0, same=0, fail=0, mismatch=0, reasons={}) for m in
           ("chain_noprior", "chain_nochase", "zero", "garbage_lt_prior", "other_facet")}
    capacity_max_ratio = 0.0
    for i in range(count):
        k1 = i % 2 == 0
        inst = instance(rng, k1, big=(i % 3 == 0))
        try:
            po, plog = run_plain(inst)
        except Fail:
            agg["plain_fail"] += 1
            continue
        ref = result(po)
        agg["instances"] += 1
        agg["k1" if k1 else "k"] += 1
        agg["grouped"] += po.st["grouped_lots"] > 0
        agg["facets"] += len(plog)
        agg["inv_viol_plain"] += po.inv_viol
        capacity_max_ratio = max(capacity_max_ratio, len(po.nxt) / max(1, po.capacity))
        # A. interleaved, safe protocol (stale reads, init visible)
        for rep in range(3):
            res, err, stt, oo, _ = interleaved(inst, rng)
            agg["inv_viol_hinted"] += oo.inv_viol
            if err:
                agg["il_fail"] += 1
                print("IL FAIL", i, err)
                continue
            if res == ref:
                agg["il_ok"] += 1
            else:
                agg["il_mismatch"] += 1
                print("IL MISMATCH", i)
            agg["il_used"] += stt["used"]
            agg["il_rejected"] += stt["rejected"]
            agg["il_published"] += stt["published"]
            agg["il_sc_hint_ge_prior"] += stt["sc_hint_ge_prior"]
            agg["il_non_ancestor"] += stt["non_ancestor_published"]
        # A'. unsafe: no synchronisation with node initialisation (zero-filled slots)
        res, err, stt, _, _ = interleaved(inst, rng, nosync_p=0.7)
        agg["il_nosync_runs"] += 1
        agg["il_nosync_non_ancestor"] += stt["non_ancestor_published"]
        if err or res != ref:
            agg["il_nosync_mismatch_or_fail"] += 1
        # A''. unsafe: untagged ring of hints
        res, err, stt, _, _ = interleaved(inst, rng, ring=4)
        agg["il_ring_runs"] += 1
        if err or res != ref:
            agg["il_ring_mismatch_or_fail"] += 1
        # B. adversarial chain hints with the prior test (and with anchor <= hint too)
        res, err, ao, alog = adversarial(inst, po, plog, rng, "chain")
        if err is None and res == ref:
            agg["adv_chain_ok"] += 1
        else:
            agg["adv_chain_bad"] += 1
            print("ADV BAD", i, err)
        agg["adv_hint_ge_prior"] += sum(1 for e in alog if e["hint"] is not None and e["hint"] >= e["prior"])
        agg["adv_hint_used"] += sum(1 for e in alog if e["used"])
        agg["inv_viol_hinted"] += ao.inv_viol
        res, err, _, _ = adversarial(inst, po, plog, rng, "chain_mono")
        agg["adv_mono_ok" if (err is None and res == ref) else "adv_mono_bad"] += 1
        # C. mutants / broken hints
        for m in mut:
            res, err, _, _ = adversarial(inst, po, plog, rng, m)
            mut[m]["runs"] += 1
            if err is not None:
                mut[m]["fail"] += 1
                mut[m]["reasons"][err] = mut[m]["reasons"].get(err, 0) + 1
            elif res == ref:
                mut[m]["same"] += 1
            else:
                mut[m]["mismatch"] += 1
    print("seed", seed, "count", count)
    for k, v in agg.items():
        print(f"  {k} = {v}")
    print(f"  max nodes/capacity = {capacity_max_ratio:.3f}")
    for m, v in mut.items():
        print(f"  mutant {m}: {v}")
    ok = (agg["il_mismatch"] == 0 and agg["il_fail"] == 0 and agg["adv_chain_bad"] == 0 and agg["adv_mono_bad"] == 0
          and agg["inv_viol_plain"] == 0 and agg["inv_viol_hinted"] == 0 and agg["il_sc_hint_ge_prior"] == 0
          and agg["il_non_ancestor"] == 0)
    print("VERDICT", "ok" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
