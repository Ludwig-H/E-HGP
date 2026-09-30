"""One five-site native export, exact fixed-K band and independent 1D Gamma checks.

Uses the explicitly supplied existing build; never builds, edits, or discovers an
engine. Core .o.d dependencies, objects, archive, compiler, sources and inherited
build receipt are closed before/after. No performance/statistical qualification.
"""
import argparse
from fractions import Fraction as F
import hashlib
from itertools import combinations
import json
from pathlib import Path
import subprocess
import sys


def need(ok, message):
    if not ok:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


POINTS = (0, 1, 2, 6, 9)


def rho2(face):
    return F((max(POINTS[x] for x in face) - min(POINTS[x] for x in face)) ** 2, 4)


def gamma(k, beta):
    # ALL K-subsets and ALL Johnson-adjacent unions. Exact 1D MEB = half span.
    vertices = [f for f in combinations(range(5), k) if rho2(f) <= beta]
    parent = {f: f for f in vertices}

    def find(f):
        while parent[f] != f:
            f = parent[f]
        return f

    for union in combinations(range(5), k + 1):
        if rho2(union) <= beta:
            faces = [tuple(x for x in union if x != omitted) for omitted in union]
            anchor = find(faces[0])
            for face in faces[1:]:
                parent[find(face)] = anchor
    blocks = {}
    for face in vertices:
        blocks.setdefault(find(face), []).append(face)
    return tuple(sorted(tuple(sorted(b)) for b in blocks.values()))


def canonical_partition(blocks, ids):
    return sorted(sorted(ids[x] for x in block) for block in blocks)


def closure(args, root):
    paths = {Path(__file__).resolve(), args.exporter.resolve(), args.library.resolve(),
             args.build_receipt.resolve(), Path(sys.executable).resolve(),
             Path('/usr/bin/c++').resolve()}
    paths.update(p.resolve() for p in (root / 'source_snapshot').iterdir() if p.is_file())
    source = json.loads(args.build_receipt.read_text())
    need(source['source_before'] == source['source_after'], 'inherited build was not source-closed')
    need(sha(args.exporter) == source['engine_binary_sha256'], 'unexpected exporter')
    for relative, expected in source['source_after'].items():
        p = args.source_tree / relative
        need(sha(p) == expected, 'inherited source changed: ' + relative)
        paths.add(p.resolve())
    core = args.library.parent / 'CMakeFiles/mhgp10_core.dir'
    deps = sorted(core.rglob('*.o.d'))
    need(len(deps) >= 8, 'missing archive dependency records')
    for dep in deps:
        paths.add(dep.resolve())
        paths.add(Path(str(dep)[:-2]).resolve())
        text = dep.read_text().replace('\\\n', ' ')
        for token in text.split(':', 1)[1].split():
            paths.add(Path(token).resolve())
    for relative in ('CMakeCache.txt', 'CMakeFiles/mhgp10_core.dir/flags.make',
                     'CMakeFiles/mhgp10_core.dir/link.txt'):
        paths.add((args.library.parent / relative).resolve())
    return {str(p): sha(p) for p in sorted(paths)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--exporter', type=Path, required=True)
    parser.add_argument('--library', type=Path, required=True)
    parser.add_argument('--source-tree', type=Path, required=True)
    parser.add_argument('--build-receipt', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parent
    args.out.mkdir(parents=True, exist_ok=False)
    before = closure(args, root)
    sys.path.insert(0, str(root / 'source_snapshot'))
    import frontier_core as fc
    import arms
    import cover_band as band
    cloud, export = args.out / 'line5.u32le', args.out / 'line5.json'
    fc.write_cloud([(x, 0, 0) for x in POINTS], cloud)
    code, summary, stdout, stderr, argv = fc.run_exporter(
        str(args.exporter), str(cloud), str(export), 3, threads=1, all_orders=True, timeout=10)
    need(code == 0, 'export failed: ' + stderr)
    e = fc.load_export(export)
    need(e.point_id == list(range(5)) and e.meta['coordinate_bits'] == 18, 'unexpected sites/profile')
    rows, attachments = {}, {}
    cuts = {F(0), F(9)}
    for k in (2, 3):
        cuts.update(rho2(f) for j in (k, k + 1) for f in combinations(range(5), j))
    checks = 0
    for k in (2, 3):
        ctx = arms.ArmContext(e, k)
        value = band.cover_band_lca(ctx, F(1, 8))
        att = fc.Attachments(ctx.forest, value.dates, value.nodes)
        need(tuple(att.dates) == value.dates and tuple(att.nodes) == value.nodes, 'non-neutral attachment')
        attachments[k] = att
        rows[k] = {'dates': [str(t) for t in value.dates], 'nodes': list(value.nodes),
                   'blocks_beta9': canonical_partition(att.blocks(F(9)), e.point_id),
                   'first_cover': [str(ctx.cover_level(s)) for s in range(5)],
                   'per_site_selected': list(value.per_site_selected),
                   'selected_witnesses': [], 'qmin_recomputed': ctx.qmin_checked,
                   'forest': [{'node':v,'beta':str(ctx.forest.level(v)),
                               'parent':ctx.forest.parent[v], 'birth':ctx.forest.birth[v],
                               'children':ctx.forest.children[v]} for v in range(len(ctx.forest))]}
        for s, witnesses in enumerate(ctx.witnesses()):
            threshold = F(81, 64) * ctx.cover_level(s)
            rows[k]['selected_witnesses'].append(
                [[b, str(beta), node] for b, beta, node in witnesses if beta <= threshold])
        previous = None
        for beta in sorted(cuts | set(value.dates)):
            blocks = gamma(k, beta)
            face_block = {f: j for j, block in enumerate(blocks) for f in block}
            signatures = []
            for v in ctx.forest.components(beta):
                signature = set()
                for leaf in ctx.forest.births_under(v):
                    b = e.balls[ctx.forest.birth[leaf]]
                    need(b.population == k, 'unexpected 1D birth population')
                    f = tuple(sorted(e.point_id[s] for s in b.I + b.U))
                    need(f in face_block, 'native birth vertex absent from Gamma')
                    signature.add(face_block[f])
                need(len(signature) == 1, 'native node merges wrong Gamma components')
                signatures.extend(signature)
            need(sorted(signatures) == list(range(len(blocks))), 'native/Gamma component mismatch')
            current = att.blocks(beta)
            if previous is not None:
                need(fc.nested(previous, current), 'fixed-K band not laminar')
            previous = current
            checks += 1
    need(rows[2]['blocks_beta9'] == [[0, 1, 2], [3, 4]], 'wrong K2 partition')
    need(rows[3]['blocks_beta9'] == [[0, 1, 2, 3], [4]], 'wrong K3 partition')
    need(not fc.nested(attachments[3].blocks(F(9)), attachments[2].blocks(F(9))),
         'cross-K counterexample disappeared')
    # The active K3 component at beta9 maps geometrically to the LEFT K2 component.
    k2, k3 = e.order(2), e.order(3)
    c3 = k3.forest.components(F(9))
    need(len(c3) == 1, 'unexpected active K3 components')
    image2 = k2.forest.ancestor(k3.lower[c3[0]], F(9))
    need(image2 == k2.forest.ancestor(rows[2]['nodes'][0], F(9)), 'wrong vertical image')
    need(image2 != k2.forest.ancestor(rows[2]['nodes'][3], F(9)), 'point6 wrongly commutes vertically')
    after = closure(args, root)
    need(before == after, 'dependency changed during audit')
    report = {'status':'PASS', 'counterexample':'fixed-K laminar bands need not commute with FULL verticals',
              'checks':checks, 'eta':'1/8', 'points':list(POINTS), 'cut_beta':'9', 'orders':rows,
              'native_argv':argv, 'native_returncode':code, 'native_stdout':stdout,
              'native_stderr':stderr, 'native_meta':e.meta, 'vertical_K3_node':c3[0],
              'vertical_K2_image':image2, 'optimize_flag':sys.flags.optimize,
              'closure_before':before, 'closure_after':after,
              'cloud_sha256':sha(cloud), 'export_sha256':sha(export),
              'GCP_used':False,
              'limits':'One bounded u18 fixture; existing inherited build, no rebuild, statistics, head/EOM or timing gate.'}
    (args.out / 'receipt.json').write_text(json.dumps(report, indent=2, sort_keys=True)+'\n')
    print(json.dumps({'status':'PASS','checks':checks,'points':list(POINTS),'eta':'1/8',
                      'K2':rows[2]['blocks_beta9'],'K3':rows[3]['blocks_beta9'],
                      'Gamma_native_equal':True,'vertical_point6_commutes':False},sort_keys=True))


if __name__ == '__main__':
    main()
