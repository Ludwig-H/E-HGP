from pathlib import Path
import collections
import csv
import hashlib
import json
import math
import sys
import numpy as np

BASE = Path('/workspaces/E-HGP/build/v9-point-clustering-benchmark-20260927-r1')
SRC = Path('/workspaces/E-HGP/build/v9-resume-20260927/morsehgp3D_v9/audits/b_point_hierarchy_k_20260927')
sys.path.insert(0, str(SRC))
from projection import SourceTree
from eom import condense_eom, equivalent_labels, fit_hdbscan, sklearn_provenance

def read(p):
    return json.loads(Path(p).read_text())

def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def check(value, what):
    if not value:
        raise RuntimeError(what)

def canon(value):
    return json.loads(json.dumps(value))

def compare(a, b, what):
    if isinstance(a, dict):
        check(set(a) == set(b), f'{what}: keys')
        for k in a:
            compare(a[k], b[k], f'{what}.{k}')
    elif isinstance(a, float):
        check(isinstance(b, (int, float)) and math.isclose(a, b, rel_tol=1e-11, abs_tol=1e-12), f'{what}: {a} != {b}')
    else:
        check(a == b, f'{what}: {a} != {b}')

def contingency(a, b):
    aa, bb, table = collections.Counter(a), collections.Counter(b), collections.Counter(zip(a, b))
    return aa, bb, table

def ari(a, b):
    n = len(a)
    if n < 2:
        return 1.0
    aa, bb, table = contingency(a, b)
    c2 = lambda x: x * (x - 1) // 2
    row, col, both = sum(map(c2, aa.values())), sum(map(c2, bb.values())), sum(map(c2, table.values()))
    total = c2(n)
    num = 2 * (both * total - row * col)
    den = (row + col) * total - 2 * row * col
    return num / den if den else 1.0

def nmi(a, b):
    n = len(a)
    aa, bb, table = contingency(a, b)
    ent = lambda v: -math.fsum((x / n) * math.log(x / n) for x in v)
    denominator = (ent(aa.values()) + ent(bb.values())) / 2
    if denominator == 0:
        return 1.0
    mutual = math.fsum((v / n) * math.log(v * n / (aa[x] * bb[y])) for (x, y), v in table.items())
    return max(mutual, 0.0) / denominator

def independent_metrics(truth, labels):
    retained = [i for i, label in enumerate(labels) if label >= 0]
    inliers = [i for i, label in enumerate(truth) if label >= 0]
    truth_in = [truth[i] for i in inliers]
    labels_in = [labels[i] for i in inliers]
    singleton = [x if x >= 0 else ('rejected', i) for i, x in enumerate(labels_in)]
    result = dict(ari_all=ari(truth, labels), nmi_all=nmi(truth, labels),
        clusters=len(set(labels[i] for i in retained)), coverage=len(retained)/len(truth),
        noise_count=len(truth)-len(retained), ari_true_inliers=ari(truth_in, labels_in) if len(inliers)>=2 else None,
        ari_inliers_noise_singletons=ari(truth_in, singleton) if len(inliers)>=2 else None,
        ari_classified=ari([truth[i] for i in retained], [labels[i] for i in retained]) if len(retained)>=2 else None)
    if len(inliers) != len(truth):
        tp = sum(t < 0 and p < 0 for t, p in zip(truth, labels))
        fp = sum(t >= 0 and p < 0 for t, p in zip(truth, labels))
        fn = sum(t < 0 and p >= 0 for t, p in zip(truth, labels))
        result.update(noise_precision=tp/(tp+fp) if tp+fp else 0., noise_recall=tp/(tp+fn),
                      noise_f1=2*tp/(2*tp+fp+fn) if 2*tp+fp+fn else 0.)
    else:
        result.update(noise_precision=None, noise_recall=None, noise_f1=None)
    return result

def typed_tree(tree):
    return dict(n=tree['n'], children={int(k): v for k,v in tree['children'].items()},
                heights={int(k): v for k,v in tree['heights'].items()})

def pairwise_purity(raw_tree, truth):
    # Independent pair/LCA oracle, not the benchmark's pair-count recurrence.
    tree = typed_tree(raw_tree)
    n, children, heights = tree['n'], tree['children'], tree['heights']
    nodes = set(range(n)) | set(children)
    parent = {}
    for p, row in children.items():
        for c in row:
            check(c not in parent, 'purity multiple parent')
            parent[c] = p
    roots = nodes - set(parent)
    check(len(roots) == 1, 'purity root')
    root = next(iter(roots))
    depth = np.zeros(max(nodes)+1, dtype=np.int64)
    order, stack = [], [root]
    while stack:
        node = stack.pop()
        order.append(node)
        for child in children.get(node, []):
            depth[child] = depth[node]+1
            stack.append(child)
    levels = max(1, int(depth.max()).bit_length())
    up = np.full((levels, len(depth)), root, dtype=np.int64)
    for node,p in parent.items():
        up[0,node] = p
    for j in range(1, levels):
        up[j] = up[j-1,up[j-1]]
    classes = sorted(set(x for x in truth if x>=0))
    class_index = {x:i for i,x in enumerate(classes)}
    counts = np.zeros((len(depth), len(classes)), dtype=np.int64)
    for leaf,label in enumerate(truth):
        if label>=0:
            counts[leaf,class_index[label]] = 1
    for node in reversed(order):
        if node in parent:
            counts[parent[node]] += counts[node]
    plateau = np.arange(len(depth))
    for node in order:
        p = parent.get(node)
        if p is not None and node >= n and heights[node] == heights[p]:
            plateau[node] = plateau[p]
    terms, pairs = [], 0
    truth_array = np.asarray(truth)
    for label in classes:
        members = np.flatnonzero(truth_array == label)
        i,j = np.triu_indices(len(members), 1)
        a,b = members[i].copy(),members[j].copy()
        swap = depth[a]<depth[b]
        a[swap],b[swap] = b[swap].copy(),a[swap].copy()
        diff = depth[a]-depth[b]
        for h in range(levels):
            select = (diff & (1<<h)) != 0
            a[select] = up[h,a[select]]
        same = a == b
        for h in reversed(range(levels)):
            select = up[h,a] != up[h,b]
            a[select], b[select] = up[h,a[select]],up[h,b[select]]
        lca = np.where(same,a,up[0,a])
        lca = plateau[lca]
        terms.append(float(np.sum(counts[lca,class_index[label]] / counts[lca].sum(axis=1))))
        pairs += len(lca)
    return math.fsum(terms)/pairs if pairs else None

r = read(BASE/'receipt.json')
check(r['status']=='completed' and not r['failures'], 'run status')
check(r['sources_before']==r['sources_after'], 'source closure')
for path,expected in r['sources_before'].items():
    check(sha(path)==expected, f'current source {path}')
check(sha(r['native_binary'])==r['native_binary_sha256']==r['native_binary_sha256_after'], 'native binary')
check(sha(r['input_manifest'])==r['input_manifest_sha256'], 'manifest SHA')
m = read(r['input_manifest'])
check(m['complete'] and len(m['cases'])==12, 'full corpus')
check(sha(Path(r['input_manifest']).parent/'plan.json')==m['plan_sha256'], 'plan SHA')
check(sklearn_provenance()==r['sklearn'], 'sklearn source/version closure')
cases = {c['id']: c for c in m['cases']}
expected = {(c,k,size,z,method) for c in cases for k in (2,5,10) for size in (20,50)
            for z in (1,2) for method in ('first_coverage','entry_vote','hdbscan_common')}
expected |= {(c,k,size,1,'hdbscan_standard') for c in cases for k in (2,5,10) for size in (20,50)}
rows = {(x['case'],x['k'],x['min_cluster_size'],x['exp_z'],x['method']):x for x in r['rows']}
check(len(rows)==len(r['rows'])==504 and set(rows)==expected,'504 unique complete parameter tuples')
commands = {tuple(x['argv']):x for x in r['commands']}
check(len(commands)==36,'36 unique native commands')
counters = collections.Counter()
warnings = collections.Counter()
disagreements = []
for c in m['cases']:
    for key,expected_sha in c['prepared_sha256'].items():
        check(sha(c[key])==expected_sha,f'{c["id"]} {key} source SHA')
    if 'source_file' in c:
        check(sha(c['source_file'])==c['source_sha256'],f'{c["id"]} raw SHA')
    points = np.load(c['points_npy'],allow_pickle=False)
    binary = np.fromfile(c['points_u32le'],dtype='<u4').reshape(-1,3)
    truth = read(c['labels_json'])
    check(points.shape==(c['n'],3) and np.array_equal(points,binary),'same full geometry')
    check(len(truth)==c['n'] and len(np.unique(binary,axis=0))==c['n'],'all distinct points')
    check(binary.max()<2**18,'u18 range')
    check(c['dimension']!=2 or np.all(binary[:,2]==0),'planar embedding')
    for k in (2,5,10):
        d = BASE/f'{c["id"]}_k{k}'
        cmd = read(d/'command.json')
        check(tuple(cmd['argv']) in commands and commands[tuple(cmd['argv'])]==cmd,'command receipt binding')
        check(cmd['argv']==[r['native_binary'],'--input',c['points_u32le'],'--k',str(k),'--workers','4'],'argv params')
        check(cmd['input_sha256']==c['prepared_sha256'],'command input hashes')
        check(cmd['returncode']==0 and sha(d/'native.json')==cmd['stdout_sha256'] and sha(d/'native.stderr')==cmd['stderr_sha256'],'native capture hashes')
        native = read(d/'native.json')
        check(native['point_count']==c['n'] and native['k']==k and native['computed_orders']==k,'native n/K')
        check(np.array_equal(native['points'],binary),'native full coordinates')
        source = SourceTree(native)
        trees, purity = {}, {}
        for method in ('first_coverage','entry_vote'):
            for z in (1,2):
                saved = read(d/f'{method}_z{z}_tree.json')
                check(canon(source.project(method,z))==saved,f'projection replay {c["id"]} {k} {method} {z}')
                trees[method,z] = saved['tree']
                purity[method,z] = pairwise_purity(saved['tree'],truth)
                counters['projection_replays'] += 1
        for size in (20,50):
            hb = read(d/f'hdbscan_m{size}.json')
            check(hb['provenance']==r['sklearn'],'HDB provenance')
            check(hb['parameters']['k_self_included']==hb['parameters']['min_samples']==k and hb['parameters']['min_cluster_size']==size,'HDB params')
            refit = fit_hdbscan(points,k=k,min_cluster_size=size)
            check(canon(refit)==hb,f'HDB whole refit {c["id"]} {k} {size}')
            counters['hdbscan_refits'] += 1
            if not hb['common_z1_matches_standard']:
                disagreements.append((c['id'],k,size))
            counters['preserved_tree_matches'] += hb['preserved_tree_z1_matches_standard']
            hp = pairwise_purity(hb['tree'],truth)
            for z in (1,2):
                for method in ('first_coverage','entry_vote','hdbscan_common') + (('hdbscan_standard',) if z==1 else ()):
                    row = rows[c['id'],k,size,z,method]
                    check((row['n'],row['dimension'],row['split'])==(c['n'],c['dimension'],c['split']),'row metadata')
                    saved = read(d/f'{method}_m{size}_z{z}_eom.json')
                    labels = saved['labels']
                    check(len(labels)==c['n'] and all(type(x)is int and x>=-1 for x in labels),'full valid prediction')
                    compare(independent_metrics(truth,labels),row['metrics'],f'metrics {c["id"]} {k} {size} {z} {method}')
                    compare(hp if method.startswith('hdbscan') else purity[method,z],row['dendrogram_purity'],'independent purity')
                    check(saved['warnings']==row['warnings'],'warnings exact')
                    if method=='hdbscan_standard':
                        check(labels==hb['standard_labels_z1'],'standard labels binding')
                    else:
                        tree = hb['tree'] if method=='hdbscan_common' else trees[method,z]
                        replay = condense_eom(**typed_tree(tree),min_cluster_size=size,exp_z=z,atomize_ties=True)
                        check(canon(replay)==saved,'full EOM replay')
                        counters['eom_replays'] += 1
                    counters['metric_rows'] += 1
                    warnings.update(row['warnings'])
        print('OK',c['id'],k,flush=True)

with (BASE/'scores.csv').open() as handle:
    csvrows = list(csv.DictReader(handle))
check(len(csvrows)==504,'CSV count')
for flat in csvrows:
    key = (flat['case'],int(flat['k']),int(flat['min_cluster_size']),int(flat['exp_z']),flat['method'])
    row = rows[key]
    for metric,value in row['metrics'].items():
        compare(value,None if flat[metric]=='' else float(flat[metric]),f'CSV {metric}')
    compare(row['dendrogram_purity'],float(flat['dendrogram_purity']),'CSV purity')
for path,expected_sha in r['sources_before'].items():
    check(sha(path)==expected_sha,f'after readonly replay source {path}')
print(json.dumps(dict(status='passed',receipt_sha256=sha(BASE/'receipt.json'),counts=dict(counters),
    warnings=dict(warnings),common_standard_disagreements=disagreements,inputs_total=sum(c['n'] for c in cases.values())),sort_keys=True))
