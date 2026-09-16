# cython: language_level=3
# cython: boundscheck=False
# cython: cdivision=True
# cython: wraparound=False
# cython: nonecheck=False
# cython: initializedcheck=False

import numpy as np

cimport numpy as cnp
from libc.math cimport NAN, isnan, pow
from libcpp cimport bool
from libcpp.vector cimport vector

cnp.import_array()

ctypedef cnp.float32_t DTYPE_t
ctypedef cnp.float64_t FTYPE_t
ctypedef cnp.int32_t ITYPE_t

cdef double MIN_LEVEL = 1e-12


cdef inline double _level_weight(double level, double expZ) noexcept nogil:
    return pow(level if level > MIN_LEVEL else MIN_LEVEL, -expZ)


cdef extern from *:
    """
    #include <algorithm>

    struct FaceRef {
        int simplex;
        int dropped;
    };

    struct FaceRefLess {
        const int* simplices;
        int width;

        FaceRefLess() : simplices(nullptr), width(0) {}
        FaceRefLess(const int* simplices_in, int width_in)
            : simplices(simplices_in), width(width_in) {}

        bool operator()(const FaceRef& a, const FaceRef& b) const {
            const int* sa = simplices + static_cast<std::ptrdiff_t>(a.simplex) * width;
            const int* sb = simplices + static_cast<std::ptrdiff_t>(b.simplex) * width;
            int ia = 0;
            int ib = 0;
            for (int k = 0; k < width - 1; ++k) {
                if (ia == a.dropped) ++ia;
                if (ib == b.dropped) ++ib;
                if (sa[ia] != sb[ib]) return sa[ia] < sb[ib];
                ++ia;
                ++ib;
            }
            return false;
        }
    };

    template <typename Iterator, typename Compare>
    inline void sort_range(Iterator first, Iterator last, Compare compare) {
        std::sort(first, last, compare);
    }
    """
    cdef struct FaceRef:
        int simplex
        int dropped

    cdef cppclass FaceRefLess:
        FaceRefLess()
        FaceRefLess(const int* simplices, int width)
        bool operator()(const FaceRef& a, const FaceRef& b) nogil

    void sort_range[Iterator, Compare](Iterator first, Iterator last, Compare compare) nogil


cdef class _UnionFind:
    cdef ITYPE_t[::1] parent
    cdef ITYPE_t[::1] size

    def __init__(self, int n):
        self.parent = np.arange(n, dtype=np.int32)
        self.size = np.ones(n, dtype=np.int32)

    cdef int find(self, int x) noexcept:
        cdef int root = x
        cdef int nxt
        while self.parent[root] != root:
            root = self.parent[root]
        while self.parent[x] != root:
            nxt = self.parent[x]
            self.parent[x] = root
            x = nxt
        return root

    cdef bint union(self, int x, int y) noexcept:
        cdef int rx = self.find(x)
        cdef int ry = self.find(y)
        if rx == ry:
            return False
        if self.size[rx] < self.size[ry]:
            rx, ry = ry, rx
        self.parent[ry] = rx
        self.size[rx] += self.size[ry]
        return True


def dual_graph(const int[:, ::1] simplices, const float[::1] levels, int K, double expZ):
    """Build the graph of (K-1)-faces linked by the K-simplices containing them.

    Parameters
    ----------
    simplices : ndarray of shape (m, K + 1), int32
        Vertex indices of the K-simplices, sorted within each row.
    levels : ndarray of shape (m,), float32
        Filtration level of each K-simplex (relative radius).
    K : int
        Order of the simplices.
    expZ : float
        Exponent of the weight ``level ** -expZ`` of a simplex.

    Returns
    -------
    faces : ndarray of shape (n_faces, K), int32
        Distinct (K-1)-faces.
    u, v : ndarrays of shape (m * K,), int32
        Endpoints of the edges; each simplex links its K + 1 faces by a path.
    w : ndarray of shape (m * K,), float32
        Filtration level of each edge.
    face_weights : ndarray of shape (n_faces,), float64
        Sum of the weights of the simplices containing each face.
    """
    cdef Py_ssize_t n_simplices = simplices.shape[0]
    cdef int width = K + 1
    if simplices.shape[1] != width:
        raise ValueError("simplices must have K + 1 columns")
    if levels.shape[0] != n_simplices:
        raise ValueError("levels must have one entry per simplex")
    if n_simplices == 0:
        return (
            np.empty((0, K), dtype=np.int32),
            np.empty(0, dtype=np.int32),
            np.empty(0, dtype=np.int32),
            np.empty(0, dtype=np.float32),
            np.empty(0, dtype=np.float64),
        )

    cdef Py_ssize_t total = n_simplices * width
    cdef vector[FaceRef] refs
    refs.resize(total)
    cdef Py_ssize_t i, j, g, start
    cdef Py_ssize_t cursor = 0

    with nogil:
        for i in range(n_simplices):
            for j in range(width):
                refs[i * width + j].simplex = <int>i
                refs[i * width + j].dropped = <int>j

    cdef FaceRefLess less = FaceRefLess(&simplices[0, 0], width)
    with nogil:
        sort_range(refs.begin(), refs.end(), less)

    cdef Py_ssize_t n_faces = 0
    with nogil:
        while cursor < total:
            start = cursor
            cursor += 1
            while cursor < total and not less(refs[start], refs[cursor]):
                cursor += 1
            n_faces += 1

    cdef cnp.ndarray[ITYPE_t, ndim=2] faces = np.empty((n_faces, K), dtype=np.int32)
    cdef cnp.ndarray[FTYPE_t, ndim=1] face_weights = np.empty(n_faces, dtype=np.float64)
    cdef ITYPE_t[:, ::1] faces_view = faces
    cdef FTYPE_t[::1] face_weights_view = face_weights
    cdef vector[int] face_of
    face_of.resize(total)

    cdef Py_ssize_t face = 0
    cdef int s, d, k, col
    cdef double accumulated
    cursor = 0
    with nogil:
        while cursor < total:
            start = cursor
            accumulated = 0.0
            while True:
                accumulated = accumulated + _level_weight(levels[refs[cursor].simplex], expZ)
                cursor += 1
                if cursor >= total or less(refs[start], refs[cursor]):
                    break

            s = refs[start].simplex
            d = refs[start].dropped
            col = 0
            for k in range(width):
                if k != d:
                    faces_view[face, col] = simplices[s, k]
                    col = col + 1
            face_weights_view[face] = accumulated

            for g in range(start, cursor):
                face_of[<Py_ssize_t>refs[g].simplex * width + refs[g].dropped] = <int>face
            face += 1

    cdef Py_ssize_t n_edges = n_simplices * K
    cdef cnp.ndarray[ITYPE_t, ndim=1] u = np.empty(n_edges, dtype=np.int32)
    cdef cnp.ndarray[ITYPE_t, ndim=1] v = np.empty(n_edges, dtype=np.int32)
    cdef cnp.ndarray[DTYPE_t, ndim=1] w = np.empty(n_edges, dtype=np.float32)
    cdef ITYPE_t[::1] u_view = u
    cdef ITYPE_t[::1] v_view = v
    cdef DTYPE_t[::1] w_view = w

    with nogil:
        for i in range(n_simplices):
            for j in range(K):
                u_view[i * K + j] = face_of[i * width + j]
                v_view[i * K + j] = face_of[i * width + j + 1]
                w_view[i * K + j] = levels[i]

    return faces, u, v, w, face_weights


def kruskal(U, V, int N):
    """Kruskal's algorithm on edges already sorted by non-decreasing weight.

    Returns one array of edge indices per connected component of the graph.
    """
    U = np.ascontiguousarray(U, dtype=np.int32)
    V = np.ascontiguousarray(V, dtype=np.int32)
    cdef Py_ssize_t M = U.shape[0]
    if V.shape[0] != M:
        raise ValueError("U and V must have the same length")

    cdef ITYPE_t[::1] Uv = U
    cdef ITYPE_t[::1] Vv = V
    cdef _UnionFind uf = _UnionFind(N)
    cdef cnp.ndarray[ITYPE_t, ndim=1] kept = np.empty(M, dtype=np.int32)
    cdef Py_ssize_t n_kept = 0
    cdef Py_ssize_t i
    cdef int components = N
    cdef int r, c, C

    for i in range(M):
        if uf.union(Uv[i], Vv[i]):
            kept[n_kept] = <ITYPE_t>i
            n_kept += 1
            components -= 1
            if components == 1:
                break

    cdef cnp.ndarray[ITYPE_t, ndim=1] roots = np.empty(N, dtype=np.int32)
    for i in range(N):
        roots[i] = uf.find(<int>i)

    cdef cnp.ndarray[ITYPE_t, ndim=1] root_to_cc = np.full(N, -1, dtype=np.int32)
    C = 0
    for i in range(N):
        r = roots[i]
        if root_to_cc[r] == -1:
            root_to_cc[r] = C
            C += 1

    cdef cnp.ndarray[cnp.intp_t, ndim=1] offsets = np.zeros(C + 1, dtype=np.intp)
    for i in range(n_kept):
        offsets[root_to_cc[roots[Uv[kept[i]]]] + 1] += 1
    np.cumsum(offsets, out=offsets)

    cdef cnp.ndarray[cnp.intp_t, ndim=1] position = offsets[:C].copy()
    cdef cnp.ndarray[ITYPE_t, ndim=1] ordered = np.empty(n_kept, dtype=np.int32)
    for i in range(n_kept):
        c = root_to_cc[roots[Uv[kept[i]]]]
        ordered[position[c]] = kept[i]
        position[c] += 1

    return np.split(ordered, offsets[1:C])


def condense_tree(
    DTYPE_t[::1] W_nodes,
    ITYPE_t[::1] U_mst,
    ITYPE_t[::1] V_mst,
    DTYPE_t[::1] W_mst,
    double min_cluster_size,
    double expZ,
):
    """HDBSCAN-like condensed tree of a minimum spanning tree.

    Parameters
    ----------
    W_nodes : ndarray of shape (N,), float32
        Mass of each node.
    U_mst, V_mst : ndarrays of shape (N - 1,), int32
        Endpoints of the tree edges.
    W_mst : ndarray of shape (N - 1,), float32
        Edge levels in non-decreasing order.
    min_cluster_size : float
        Minimal mass of a cluster.
    expZ : float
        Exponent of the density scale ``lambda = level ** -expZ``.

    Returns
    -------
    dict
        ``children`` (list of lists of cluster ids, parents after children),
        ``r`` (birth level), ``size`` (mass at birth), ``stability``,
        ``lambda_birth``, ``lambda_death``, ``initial_membership`` (first cluster
        joined by each node, -1 if none) and ``join_r`` (level at which it joined).
    """
    cdef Py_ssize_t N = W_nodes.shape[0]
    cdef Py_ssize_t M = W_mst.shape[0]
    if U_mst.shape[0] != M or V_mst.shape[0] != M:
        raise ValueError("U_mst, V_mst and W_mst must have the same length")
    if N != M + 1:
        raise ValueError("a spanning tree on N nodes has N - 1 edges")

    cdef vector[vector[ITYPE_t]] children
    cdef vector[float] birth_r
    cdef vector[float] death_r
    cdef vector[double] stability
    cdef vector[double] size_at_birth
    cdef vector[double] mass
    cdef vector[double] sum_join_lambda

    cdef vector[ITYPE_t] parent = vector[ITYPE_t](N)
    cdef vector[double] comp_weight = vector[double](N)
    cdef vector[ITYPE_t] comp_cid = vector[ITYPE_t](N)
    cdef vector[vector[ITYPE_t]] comp_nodes = vector[vector[ITYPE_t]](N)
    cdef vector[vector[ITYPE_t]] tracked = vector[vector[ITYPE_t]](N)
    cdef vector[Py_ssize_t] last_batch = vector[Py_ssize_t](N, -1)
    cdef vector[ITYPE_t] roots

    cdef cnp.ndarray[ITYPE_t, ndim=1] initial_membership = np.full(N, -1, dtype=np.int32)
    cdef cnp.ndarray[DTYPE_t, ndim=1] join_r = np.full(N, np.inf, dtype=np.float32)

    cdef double threshold = min_cluster_size * (1.0 - 1e-6)
    cdef Py_ssize_t i, j, k, t, node_count
    cdef ITYPE_t ru, rv, cid, node, new_cid
    cdef float r
    cdef double lam, added, n_parent

    for i in range(N):
        parent[i] = <ITYPE_t>i
        comp_weight[i] = W_nodes[i]
        comp_cid[i] = -1
        comp_nodes[i].push_back(<ITYPE_t>i)

    i = 0
    while i < M:
        j = i
        while j < M and W_mst[j] <= W_mst[i]:
            j += 1
        r = W_mst[j - 1]
        lam = _level_weight(r, expZ)
        roots.clear()

        for k in range(i, j):
            ru = U_mst[k]
            while parent[ru] != ru:
                parent[ru] = parent[parent[ru]]
                ru = parent[ru]
            rv = V_mst[k]
            while parent[rv] != rv:
                parent[rv] = parent[parent[rv]]
                rv = parent[rv]
            if ru == rv:
                continue
            if comp_weight[ru] < comp_weight[rv]:
                ru, rv = rv, ru

            if comp_cid[ru] != -1:
                tracked[ru].push_back(comp_cid[ru])
                comp_cid[ru] = -1
            if comp_cid[rv] != -1:
                tracked[rv].push_back(comp_cid[rv])
                comp_cid[rv] = -1
            if not tracked[rv].empty():
                tracked[ru].insert(tracked[ru].end(), tracked[rv].begin(), tracked[rv].end())
                tracked[rv].clear()

            parent[rv] = ru
            comp_weight[ru] += comp_weight[rv]
            comp_nodes[ru].insert(comp_nodes[ru].end(), comp_nodes[rv].begin(), comp_nodes[rv].end())
            comp_nodes[rv].clear()

            if last_batch[ru] != i:
                roots.push_back(ru)
                last_batch[ru] = i

        for t in range(<Py_ssize_t>roots.size()):
            ru = roots[t]
            if parent[ru] != ru:
                continue

            if comp_weight[ru] >= threshold:
                node_count = <Py_ssize_t>comp_nodes[ru].size()
                if tracked[ru].empty():
                    cid = <ITYPE_t>children.size()
                    children.push_back(vector[ITYPE_t]())
                    birth_r.push_back(r)
                    death_r.push_back(NAN)
                    stability.push_back(0.0)
                    size_at_birth.push_back(comp_weight[ru])
                    mass.push_back(comp_weight[ru])
                    sum_join_lambda.push_back(comp_weight[ru] * lam)
                    for k in range(node_count):
                        node = comp_nodes[ru][k]
                        initial_membership[node] = cid
                        join_r[node] = r
                    comp_nodes[ru].clear()
                    comp_cid[ru] = cid

                elif tracked[ru].size() == 1:
                    cid = tracked[ru][0]
                    added = 0.0
                    for k in range(node_count):
                        node = comp_nodes[ru][k]
                        initial_membership[node] = cid
                        join_r[node] = r
                        added += W_nodes[node]
                    comp_nodes[ru].clear()
                    mass[cid] += added
                    sum_join_lambda[cid] += added * lam
                    comp_cid[ru] = cid

                else:
                    n_parent = 0.0
                    for k in range(<Py_ssize_t>tracked[ru].size()):
                        cid = tracked[ru][k]
                        if isnan(death_r[cid]):
                            death_r[cid] = r
                            stability[cid] += sum_join_lambda[cid] - mass[cid] * lam
                        n_parent += mass[cid]

                    new_cid = <ITYPE_t>children.size()
                    children.push_back(tracked[ru])
                    birth_r.push_back(r)
                    death_r.push_back(NAN)
                    stability.push_back(0.0)

                    added = 0.0
                    for k in range(node_count):
                        node = comp_nodes[ru][k]
                        initial_membership[node] = new_cid
                        join_r[node] = r
                        added += W_nodes[node]
                    comp_nodes[ru].clear()

                    n_parent += added
                    size_at_birth.push_back(n_parent)
                    mass.push_back(n_parent)
                    sum_join_lambda.push_back(n_parent * lam)
                    comp_cid[ru] = new_cid

            tracked[ru].clear()

        i = j

    cdef Py_ssize_t n_clusters = <Py_ssize_t>children.size()
    cdef cnp.ndarray[FTYPE_t, ndim=1] lambda_birth = np.empty(n_clusters, dtype=np.float64)
    cdef cnp.ndarray[FTYPE_t, ndim=1] lambda_death = np.empty(n_clusters, dtype=np.float64)
    for i in range(n_clusters):
        lambda_birth[i] = _level_weight(birth_r[i], expZ)
        if isnan(death_r[i]):
            stability[i] += sum_join_lambda[i]
            lambda_death[i] = 0.0
        else:
            lambda_death[i] = _level_weight(death_r[i], expZ)

    return {
        "children": [children[i] for i in range(n_clusters)],
        "r": np.asarray(birth_r, dtype=np.float32),
        "size": np.asarray(size_at_birth, dtype=np.float64),
        "stability": np.asarray(stability, dtype=np.float64),
        "lambda_birth": lambda_birth,
        "lambda_death": lambda_death,
        "initial_membership": initial_membership,
        "join_r": join_r,
    }
