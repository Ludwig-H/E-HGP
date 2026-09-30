# Faithful Python port of HGP-old build_dual_graph_cython (_cython.pyx:757-925)
# comparator (lines 33-63) uses flat pointer with stride K+1; reduce uses 2D row indexing.
import itertools, functools, random
def cython_like(rows, K):
    W = len(rows[0]); flat = [v for r in rows for v in r]; Kp1 = K+1; N = len(rows)
    def key(ref):
        s, d = ref; base = s*Kp1; out=[]; ia=0
        for _ in range(Kp1-1):
            if ia == d: ia += 1
            out.append(flat[base+ia]); ia += 1
        return tuple(out)
    refs = sorted([(i,j) for i in range(N) for j in range(Kp1)], key=key)  # std::sort by comp
    faces=[]; mapped={}; cur=0
    while cur < len(refs):
        start=cur; cur+=1
        while cur < len(refs) and key(refs[start]) == key(refs[cur]): cur+=1
        s,d = refs[start]
        faces.append(tuple(rows[s][k] for k in range(Kp1) if k != d))  # 2D row indexing, true stride
        for r in refs[start:cur]: mapped[r] = len(faces)-1
    edges=[(mapped[(i,j)], mapped[(i,j+1)]) for i in range(N) for j in range(K)]
    return faces, edges
def truth(rows, K):
    # what the code would produce if stride matched: K-faces of the first K+1 vertices of each row
    fs = sorted({tuple(v for k,v in enumerate(r[:K+1]) if k!=d) for r in rows for d in range(K+1)})
    return fs
random.seed(1)
K=2; W=4  # min_samples = 4 > K+1 = 3
rows = sorted({tuple(sorted(random.sample(range(12), W))) for _ in range(20)})
f, e = cython_like(rows, K)
bad = [x for x in f if len(set(x))!=K]
print("rows", len(rows), "width", W, "K", K)
print("faces emitted", len(f), "distinct", len(set(f)), "duplicates", len(f)-len(set(f)))
print("faces that are not K-subsets of first K+1 verts:", len(set(f) - set(truth(rows,K))))
print("true K-faces (first K+1 verts)", len(truth(rows,K)))
# Control: W = K+1 must match truth exactly
rows2 = sorted({tuple(sorted(random.sample(range(12), K+1))) for _ in range(20)})
f2,_ = cython_like(rows2, K)
print("control W=K+1: faces", len(f2), "== truth", sorted(f2)==truth(rows2,K))
