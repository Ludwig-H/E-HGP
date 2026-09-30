"""Bounded native check of the NEW band candidate, outside sealed A0..A6.

The supplied foundations are explicit, source-pinned external dependencies.
Never discovers an arbitrary binary by default. No benchmark scores, GCP or
performance qualification; six clouds with five to seven sites only.
"""
import argparse
from fractions import Fraction as F
import hashlib
import importlib.util
from itertools import combinations
import json
from pathlib import Path
import sys


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    mod = importlib.util.module_from_spec(spec)
    sys.modules[name] = mod
    spec.loader.exec_module(mod)
    return mod


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def first_gamma_join(R, points, k, left, right):
    # Independent graph oracle: all K-subsets + all (K+1)-unions, exact MEB.
    vertices = {v: R.meb(points, v)[0] for v in combinations(range(len(points)), k)}
    edges = {e: R.meb(points, e)[0] for e in combinations(range(len(points)), k + 1)}
    for beta in sorted(set(vertices.values()) | set(edges.values())):
        parent = {v: v for v, birth in vertices.items() if birth <= beta}

        def find(v):
            while parent[v] != v:
                v = parent[v]
            return v

        for edge, birth in edges.items():
            if birth > beta:
                continue
            faces = [tuple(x for x in edge if x != omit) for omit in edge]
            root = find(faces[0])
            for face in faces[1:]:
                parent[find(face)] = root
        if left in parent and right in parent and find(left) == find(right):
            return beta
    raise RuntimeError('Gamma oracle did not join the designated vertices')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--foundations', type=Path, required=True)
    ap.add_argument('--exporter', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=False)
    root = Path(__file__).resolve().parents[2]
    sources = [Path(__file__).resolve(), root / 'bench/frontier/cover_band.py',
               root / 'reference/hgp10_ref.py', args.foundations / 'frontier_core.py',
               args.foundations / 'arms.py', args.exporter]
    before = {str(p): sha(p) for p in sources}
    sys.path.insert(0, str(args.foundations))
    import frontier_core as fc
    import arms
    band = load('cover_band_native', sources[1])
    R = load('band_graph_reference', sources[2])
    rows = []
    checks = 0
    for scale in (1024, 2048):
        for jitter in (0, 1):
            S = scale
            raw = [(0, 0, 0), (-4*S, 5*S-jitter, 0), (-4*S, -5*S+jitter, 0),
                   (4*S, 0, 5*S), (4*S, 0, -5*S)]
            points = [tuple(c + 5*S for c in p) for p in raw]
            rows.append(('near_tie_%d_%d' % (S, jitter), points, 3, None))
    rows.extend([
        ('internal_k3', [(15, 4, 0), (5, 4, 0), (7, 8, 0), (7, 0, 0), (1, 4, 0), (0, 4, 1)], 3, F(25)),
        ('internal_k5', [(325, 325, 650), (520, 325, 65), (200, 325, 25), (325, 416, 13),
                         (325, 130, 65), (442, 481, 65), (250, 225, 25)], 5, F(105625))])
    results = []
    for tag, points, k, expected_internal in rows:
        cloud, export = args.out / (tag + '.u32le'), args.out / (tag + '.json')
        fc.write_cloud(points, cloud)
        code, summary, stdout, stderr, argv = fc.run_exporter(str(args.exporter), str(cloud), str(export),
                                                           k, threads=1, timeout=30)
        require(code == 0, 'native export failed: ' + tag + ': ' + stderr)
        e = fc.load_export(export)
        ctx = arms.ArmContext(e)
        zero = band.cover_band_lca(ctx, F(0))
        old = arms.arm_unique_else_lca(ctx)
        require(zero.dates == tuple(old.dates) and zero.nodes == tuple(old.nodes), 'eta0 differs from A5')
        checks += ctx.n
        variants = []
        for eta in (F(0), F(1, 64), F(1, 32), F(1, 8), F(1, 4), F(1)):
            value = band.cover_band_lca(ctx, eta)
            att = fc.Attachments(ctx.forest, value.dates, value.nodes)
            require(tuple(att.nodes) == value.nodes and tuple(att.dates) == value.dates, 'non-neutral attachment')
            cuts = sorted({F(0)} | set(ctx.forest.levels) | set(value.dates))
            previous = None
            for beta in cuts:
                current = att.blocks(beta)
                require(sorted(x for block in current for x in block) == list(range(ctx.n)), 'lost/duplicate point')
                if previous is not None:
                    require(fc.nested(previous, current), 'non-laminar attachment')
                previous = current
                checks += 1
            variants.append((value, att))
        common_cuts = sorted({F(0)} | set(ctx.forest.levels) |
                             {t for value, _att in variants for t in value.dates})
        for (_small, a), (_large, b) in zip(variants, variants[1:]):
            for beta in common_cuts:
                require(fc.nested(b.blocks(beta), a.blocks(beta)), 'larger eta did not refine smaller eta')
                checks += 1
        x = ctx.point_id.index(0)
        value, att = variants[2]  # eta=1/32 was chosen before this native run.
        if expected_internal is None:
            expected = first_gamma_join(R, points, 3, (0, 1, 2), (0, 3, 4))
            require(value.dates[x] == expected, 'near-tie did not wait for Gamma join')
            a = ctx.point_id.index(1)
            require(att.merge_height(x, a) == expected, 'wrong projected near-tie height')
            require(ctx.forest.level(value.nodes[x]) == expected, 'wrong near-tie owner birth')
        else:
            expected = expected_internal
            require(value.dates[x] == expected, 'internal-cover point delayed or lost')
            require(ctx.forest.birth[value.nodes[x]] == fc.NONE, 'internal-cover fixture attached to leaf')
        checks += 2
        results.append({'case': tag, 'K': k, 'n': ctx.n, 'export_argv': argv, 'returncode': code,
                        'stdout': stdout, 'stderr': stderr, 'export_sha256': sha(export),
                        'cloud_sha256': sha(cloud), 'eta': '1/32', 'point0_date': str(value.dates[x]),
                        'point0_expected': str(expected), 'examined': value.examined, 'selected': value.selected})
    after = {str(p): sha(p) for p in sources}
    require(before == after, 'source or binary changed during native test')
    require(len(results) == 6 and checks >= 250, 'test floor not met')
    report = {'status': 'PASS', 'checks': checks, 'clouds': results, 'sources_before': before,
              'sources_after': after, 'optimize_flag': sys.flags.optimize,
              'profile': 'quantized_u18_input_only', 'scope': 'bounded native structural regression only',
              'engine_binary_pin': 'post-build SHA only; full source closure is external', 'GCP_used': False}
    (args.out / 'receipt.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    print(json.dumps({'status': report['status'], 'checks': checks, 'clouds': len(results)}, sort_keys=True))
    return 0


if __name__ == '__main__':
    sys.exit(main())
