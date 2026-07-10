"""Deterministic minimum spanning forests of cubical activation fields.

The vertices are the cells of a one-, two-, or three-dimensional regular grid.
Two cells are adjacent when their Chebyshev distance is one (26-connectivity in
3-D, 8-connectivity in 2-D, and 2-connectivity in 1-D).  A vertex ``u`` is born
at ``f[u]`` and an edge ``(u, v)`` is born at ``max(f[u], f[v])``.  Consequently,
a minimum spanning tree preserves the connected components of every sublevel
set of the field.

The CPU implementation is the exact, deliberately simple reference.  It never
materializes the cubical edge list: each undirected edge is visited when the
later of its endpoints in ``(birth, vertex_id)`` order is activated.  The
optional CUDA implementation uses CuPy ``RawKernel`` kernels and Boruvka
phases; it scans only the 13 canonical forward neighbours of every vertex.
CuPy is imported lazily so importing this module remains CPU-only.
"""

from __future__ import annotations

from dataclasses import dataclass
import math
import time
from typing import Any, Sequence

import numpy as np

from .progress import ProgressCallback, emit_progress


_NEIGHBOUR_OFFSETS_26 = tuple(
    (dx, dy, dz)
    for dx in (-1, 0, 1)
    for dy in (-1, 0, 1)
    for dz in (-1, 0, 1)
    if (dx, dy, dz) != (0, 0, 0)
)

# Codes 13..25 in the lexicographically ordered 26-neighbour stencil are the
# 13 canonical forward offsets.  Visiting only these offsets sees every
# undirected grid edge exactly once.
_FORWARD_OFFSETS_13 = _NEIGHBOUR_OFFSETS_26[13:]
_UINT32_MAX = np.iinfo(np.uint32).max
_INT32_MAX = np.iinfo(np.int32).max


def _validate_dims(dims: int | Sequence[int]) -> tuple[int, ...]:
    if isinstance(dims, (bool, np.bool_)):
        raise TypeError("grid dimensions must be integers")
    if isinstance(dims, (int, np.integer)):
        result = (int(dims),)
    else:
        raw_dims = tuple(dims)
        if any(
            isinstance(d, (bool, np.bool_))
            or not isinstance(d, (int, np.integer))
            for d in raw_dims
        ):
            raise TypeError("grid dimensions must be integers")
        result = tuple(int(d) for d in raw_dims)
    if not 1 <= len(result) <= 3:
        raise ValueError("dims must contain one, two, or three dimensions")
    if any(d <= 0 for d in result):
        raise ValueError("all grid dimensions must be strictly positive")
    n_vertices = math.prod(result)
    if n_vertices > _INT32_MAX:
        raise ValueError(
            "the cubical MSF currently uses int32 vertex identifiers; "
            f"got {n_vertices} vertices"
        )
    return result


def _dims3(dims: tuple[int, ...]) -> tuple[int, int, int]:
    return dims + (1,) * (3 - len(dims))


def _validate_edge_ids(n_vertices: int) -> None:
    # An undirected edge is owned by the endpoint that is later in
    # (birth, vertex_id) order.  Its deterministic ID is owner * 26 + the
    # directed neighbour code.  Packing it beside a float32 weight therefore
    # requires 32 bits.
    max_edge_id = (n_vertices - 1) * 26 + 25
    if max_edge_id > _UINT32_MAX:
        raise ValueError(
            "the packed CUDA (weight, edge_id) key supports at most "
            f"{(_UINT32_MAX - 25) // 26 + 1} grid vertices"
        )


def _prepare_cpu_values(
    activations: Any, dims: int | Sequence[int] | None
) -> tuple[np.ndarray, tuple[int, ...]]:
    values = np.asarray(activations)
    if dims is None:
        if not 1 <= values.ndim <= 3:
            raise ValueError(
                "activations must be a 1-D, 2-D, or 3-D array when dims is omitted"
            )
        grid_dims = _validate_dims(values.shape)
    else:
        grid_dims = _validate_dims(dims)
        if values.size != math.prod(grid_dims):
            raise ValueError(
                f"activations has {values.size} values but dims={grid_dims} "
                f"requires {math.prod(grid_dims)}"
            )
    if values.dtype.kind != "f":
        raise TypeError("activation values must have a floating-point dtype")
    values = np.ascontiguousarray(values).reshape(-1).copy()
    if not np.all(np.isfinite(values)):
        raise ValueError("activation values must all be finite")
    return values, grid_dims


def _find(parent: np.ndarray, vertex: int) -> int:
    root = vertex
    while int(parent[root]) != root:
        root = int(parent[root])
    while vertex != root:
        next_vertex = int(parent[vertex])
        parent[vertex] = root
        vertex = next_vertex
    return root


def _union_min_root(parent: np.ndarray, u: int, v: int) -> bool:
    root_u = _find(parent, u)
    root_v = _find(parent, v)
    if root_u == root_v:
        return False
    if root_u > root_v:
        root_u, root_v = root_v, root_u
    parent[root_v] = root_u
    return True


@dataclass(frozen=True, slots=True)
class CubicalMSFResult:
    """Compact hierarchy representation returned by the cubical backend.

    ``births`` is flat in C order.  ``edges[i]`` is a tree edge born at
    ``weights[i]``; the edges are sorted by the deterministic key
    ``(weight, edge_id)``.  For a non-empty regular grid, exactly ``N - 1``
    edges are stored.
    """

    births: np.ndarray
    edges: np.ndarray
    weights: np.ndarray
    dims: tuple[int, ...]

    def __post_init__(self) -> None:
        dims = _validate_dims(self.dims)
        births = np.asarray(self.births)
        edges = np.asarray(self.edges)
        weights = np.asarray(self.weights)
        n_vertices = math.prod(dims)
        if births.ndim != 1 or births.size != n_vertices:
            raise ValueError("births must be a flat array with prod(dims) values")
        if births.dtype.kind != "f":
            raise TypeError("births must have a floating-point dtype")
        if edges.shape != (max(0, n_vertices - 1), 2):
            raise ValueError("a connected cubical grid must store exactly N - 1 edges")
        if edges.dtype.kind not in "iu":
            raise TypeError("edges must contain integer vertex identifiers")
        if weights.shape != (max(0, n_vertices - 1),):
            raise ValueError("weights must contain one value per MSF edge")
        if weights.dtype.kind != "f":
            raise TypeError("weights must have a floating-point dtype")
        if edges.size and (int(edges.min()) < 0 or int(edges.max()) >= n_vertices):
            raise ValueError("an MSF edge endpoint is outside the grid")
        if not np.all(np.isfinite(births)) or not np.all(np.isfinite(weights)):
            raise ValueError("births and weights must all be finite")
        if weights.size > 1 and np.any(weights[1:] < weights[:-1]):
            raise ValueError("MSF edges must be sorted by nondecreasing weight")

        # Normalize representation without exposing mutable views of backend
        # work buffers.  The four arrays are the complete stored hierarchy.
        object.__setattr__(self, "dims", dims)
        object.__setattr__(self, "births", np.ascontiguousarray(births))
        object.__setattr__(self, "edges", np.ascontiguousarray(edges, dtype=np.int32))
        object.__setattr__(
            self, "weights", np.ascontiguousarray(weights, dtype=births.dtype)
        )

    @property
    def n_vertices(self) -> int:
        """Number of cells in the grid."""

        return int(self.births.size)

    def components_at_cut(self, threshold: float, *, reshape: bool = False) -> np.ndarray:
        """Return canonical component labels in the sublevel set at ``threshold``.

        Inactive vertices receive ``-1``.  Every active component is labelled
        by its smallest flat vertex identifier, making labels deterministic as
        well as the represented partition.  Set ``reshape=True`` to recover
        the original grid shape.
        """

        level = float(threshold)
        if math.isnan(level):
            raise ValueError("threshold must not be NaN")

        active = self.births <= level
        parent = np.full(self.n_vertices, -1, dtype=np.int32)
        active_ids = np.flatnonzero(active)
        parent[active_ids] = active_ids.astype(np.int32, copy=False)

        # The MSF is sorted by weight, so no edge after the first rejected one
        # can be present at this cut.
        stop = int(np.searchsorted(self.weights, level, side="right"))
        for u_raw, v_raw in self.edges[:stop]:
            u = int(u_raw)
            v = int(v_raw)
            if active[u] and active[v]:
                _union_min_root(parent, u, v)

        labels = np.full(self.n_vertices, -1, dtype=np.int32)
        for vertex in active_ids:
            labels[vertex] = _find(parent, int(vertex))
        if reshape:
            return labels.reshape(self.dims)
        return labels

    # A short alias is convenient for callers that already use "cut" as a
    # hierarchy operation.  It intentionally has identical semantics.
    def cut(self, threshold: float, *, reshape: bool = False) -> np.ndarray:
        return self.components_at_cut(threshold, reshape=reshape)

    def shifted(self, delta: float) -> "CubicalMSFResult":
        """Translate every birth and merge level without rebuilding the MSF.

        Adding a spatially constant value preserves the total ordering of all
        vertices and edges.  This is the uniform-grid optimization used to
        derive the inner field ``g + H`` and outer field ``g - H`` from one
        implicit MSF topology.
        """

        shift = float(delta)
        if not math.isfinite(shift):
            raise ValueError("delta must be finite")
        typed_shift = self.births.dtype.type(shift)
        if not np.isfinite(typed_shift):
            raise ValueError("delta is not representable in the result dtype")
        with np.errstate(over="ignore", invalid="ignore"):
            births = self.births + typed_shift
            weights = self.weights + typed_shift
        if not np.all(np.isfinite(births)) or not np.all(np.isfinite(weights)):
            raise ValueError("delta makes a birth or merge level non-finite")
        return CubicalMSFResult(
            births=births,
            edges=self.edges,
            weights=weights,
            dims=self.dims,
        )


def cubical_msf_cpu(
    activations: Any, dims: int | Sequence[int] | None = None
) -> CubicalMSFResult:
    """Build the exact deterministic 26-connected cubical MSF on the CPU.

    This is a Kruskal reference specialized to vertex-induced edge weights.
    Processing vertices by ``(birth, vertex_id)`` lets it generate edges in
    exact ``(weight, edge_id)`` order while retaining only the output tree.
    """

    births, grid_dims = _prepare_cpu_values(activations, dims)
    n_vertices = int(births.size)
    _validate_edge_ids(n_vertices)
    if n_vertices == 1:
        return CubicalMSFResult(
            births=births,
            edges=np.empty((0, 2), dtype=np.int32),
            weights=np.empty(0, dtype=births.dtype),
            dims=grid_dims,
        )

    nx, ny, nz = _dims3(grid_dims)
    yz = ny * nz
    vertex_ids = np.arange(n_vertices, dtype=np.int32)
    # lexsort uses its final key as the primary key.
    event_order = np.lexsort((vertex_ids, births))
    # ``vertex_ids`` is no longer needed after sorting, so reuse its storage as
    # the Union-Find parent array instead of retaining a second int32[N] copy.
    parent = vertex_ids
    mst_edges = np.empty((n_vertices - 1, 2), dtype=np.int32)
    mst_weights = np.empty(n_vertices - 1, dtype=births.dtype)
    n_tree_edges = 0

    for owner_raw in event_order:
        owner = int(owner_raw)
        owner_birth = births[owner]
        x = owner // yz
        remainder = owner - x * yz
        y = remainder // nz
        z = remainder - y * nz

        # Directed neighbour codes are already increasing, hence edge IDs
        # owner * 26 + code are increasing inside an equal-birth event.
        for code, (dx, dy, dz) in enumerate(_NEIGHBOUR_OFFSETS_26):
            neighbour_x = x + dx
            neighbour_y = y + dy
            neighbour_z = z + dz
            if not (
                0 <= neighbour_x < nx
                and 0 <= neighbour_y < ny
                and 0 <= neighbour_z < nz
            ):
                continue
            neighbour = (neighbour_x * ny + neighbour_y) * nz + neighbour_z
            neighbour_birth = births[neighbour]

            # The later endpoint owns the edge.  Equality is resolved by flat
            # vertex ID, independently of sort stability or input strides.
            if not (
                neighbour_birth < owner_birth
                or (neighbour_birth == owner_birth and neighbour < owner)
            ):
                continue
            if _union_min_root(parent, owner, neighbour):
                mst_edges[n_tree_edges] = (owner, neighbour)
                weight = owner_birth
                if weight == 0:  # Give +/-0 one numeric equality class.
                    weight = births.dtype.type(0)
                mst_weights[n_tree_edges] = weight
                n_tree_edges += 1
                if n_tree_edges == n_vertices - 1:
                    break
        if n_tree_edges == n_vertices - 1:
            break

    if n_tree_edges != n_vertices - 1:
        raise RuntimeError(
            "the implicit cubical grid unexpectedly remained disconnected "
            f"({n_tree_edges} tree edges for {n_vertices} vertices)"
        )
    return CubicalMSFResult(
        births=births,
        edges=mst_edges,
        weights=mst_weights,
        dims=grid_dims,
    )


# Keep this source header-free: minimal NVRTC wheel environments do not expose
# host libc headers such as stdint.h, and these kernels only need CUDA built-ins.
_CUDA_SOURCE = r"""
#define INF_KEY 0xffffffffffffffffULL

__device__ __forceinline__ int find_root(const int* parent, int vertex) {
    int next = parent[vertex];
    while (next != vertex) {
        vertex = next;
        next = parent[vertex];
    }
    return vertex;
}

__device__ __forceinline__ unsigned int ordered_float(float value) {
    unsigned int bits = __float_as_uint(value);
    return (bits & 0x80000000U) ? ~bits : (bits ^ 0x80000000U);
}

__device__ __forceinline__ void decode_direction(
    int code, int* dx, int* dy, int* dz
) {
    // Insert the omitted centre (raw stencil position 13).
    int raw = code + (code >= 13 ? 1 : 0);
    *dx = raw / 9 - 1;
    int remainder = raw % 9;
    *dy = remainder / 3 - 1;
    *dz = remainder % 3 - 1;
}

extern "C" __global__ void init_state(
    int* parent, unsigned long long* best_key, int n
) {
    int vertex = blockDim.x * blockIdx.x + threadIdx.x;
    if (vertex < n) {
        parent[vertex] = vertex;
        best_key[vertex] = INF_KEY;
    }
}

extern "C" __global__ void reset_best(
    unsigned long long* best_key, int n
) {
    int vertex = blockDim.x * blockIdx.x + threadIdx.x;
    if (vertex < n) {
        best_key[vertex] = INF_KEY;
    }
}

extern "C" __global__ void compress_parents(int* parent, int n) {
    int vertex = blockDim.x * blockIdx.x + threadIdx.x;
    if (vertex >= n) return;
    int root = find_root(parent, vertex);
    parent[vertex] = root;
}

extern "C" __global__ void scan_implicit_edges(
    const float* values,
    int nx,
    int ny,
    int nz,
    int n,
    const int* parent,
    unsigned long long* best_key
) {
    int u = blockDim.x * blockIdx.x + threadIdx.x;
    if (u >= n) return;

    int yz = ny * nz;
    int x = u / yz;
    int remainder = u - x * yz;
    int y = remainder / nz;
    int z = remainder - y * nz;
    int root_u = find_root(parent, u);
    float value_u = values[u];

    // Forward direction codes 13..25 visit every undirected edge once.
    for (int direction = 0; direction < 13; ++direction) {
        int positive_code = direction + 13;
        int dx, dy, dz;
        decode_direction(positive_code, &dx, &dy, &dz);
        int vx = x + dx;
        int vy = y + dy;
        int vz = z + dz;
        if (vx < 0 || vx >= nx || vy < 0 || vy >= ny || vz < 0 || vz >= nz) {
            continue;
        }
        int v = (vx * ny + vy) * nz + vz;
        int root_v = find_root(parent, v);
        if (root_u == root_v) continue;

        float value_v = values[v];
        bool u_is_owner = (value_u > value_v) ||
                          ((value_u == value_v) && (u > v));
        unsigned int owner = (unsigned int)(u_is_owner ? u : v);
        unsigned int directed_code = (unsigned int)(
            u_is_owner ? positive_code : (12 - direction)
        );
        unsigned int edge_id = owner * 26U + directed_code;
        float weight = value_u > value_v ? value_u : value_v;
        if (weight == 0.0f) weight = 0.0f;
        unsigned long long key =
            ((unsigned long long)ordered_float(weight) << 32) |
            (unsigned long long)edge_id;
        atomicMin(best_key + root_u, key);
        atomicMin(best_key + root_v, key);
    }
}

extern "C" __global__ void collect_selected_edges_grid(
    int nx,
    int ny,
    int nz,
    int n,
    const int* parent,
    const unsigned long long* best_key,
    int* output_u,
    int* output_v,
    unsigned int* output_edge_id,
    unsigned int* output_count
) {
    int root = blockDim.x * blockIdx.x + threadIdx.x;
    if (root >= n || parent[root] != root) return;
    unsigned long long key = best_key[root];
    if (key == INF_KEY) return;

    unsigned int edge_id = (unsigned int)key;
    int owner = (int)(edge_id / 26U);
    int code = (int)(edge_id % 26U);
    int dx, dy, dz;
    decode_direction(code, &dx, &dy, &dz);
    int yz = ny * nz;
    int x = owner / yz;
    int remainder = owner - x * yz;
    int y = remainder / nz;
    int z = remainder - y * nz;
    int neighbour = ((x + dx) * ny + (y + dy)) * nz + (z + dz);

    int root_owner = parent[owner];
    int root_neighbour = parent[neighbour];
    if (root_owner == root_neighbour) return;
    int other_root = root_owner == root ? root_neighbour : root_owner;
    if (root_owner != root && root_neighbour != root) return;

    // The same edge can be cheapest for both incident components.  Only the
    // smaller root emits that duplicate; all other selected edges are unique.
    if (best_key[other_root] == key && root > other_root) return;

    unsigned int output_position = atomicAdd(output_count, 1U);
    output_u[output_position] = owner;
    output_v[output_position] = neighbour;
    output_edge_id[output_position] = edge_id;
}

extern "C" __global__ void union_selected_edges(
    const int* output_u,
    const int* output_v,
    int start,
    int end,
    int* parent
) {
    int position = start + blockDim.x * blockIdx.x + threadIdx.x;
    if (position >= end) return;
    int u = output_u[position];
    int v = output_v[position];

    // Concurrent union by minimum root.  Retrying a failed CAS guarantees
    // that every selected edge is represented in the new components.
    while (true) {
        int root_u = find_root(parent, u);
        int root_v = find_root(parent, v);
        if (root_u == root_v) return;
        int high = root_u > root_v ? root_u : root_v;
        int low = root_u > root_v ? root_v : root_u;
        if (atomicCAS(parent + high, high, low) == high) return;
    }
}
"""


_RAW_MODULE: Any | None = None


def _require_cupy() -> Any:
    try:
        import cupy as cp
    except Exception as exc:  # pragma: no cover - depends on optional package
        raise ImportError(
            "cubical_msf_gpu requires CuPy built for the installed CUDA runtime "
            "(for example cupy-cuda12x)"
        ) from exc
    try:
        device_count = int(cp.cuda.runtime.getDeviceCount())
    except Exception as exc:  # pragma: no cover - depends on CUDA driver
        raise RuntimeError(
            "CuPy is installed, but no usable CUDA runtime/device was found"
        ) from exc
    if device_count < 1:  # pragma: no cover - depends on CUDA driver
        raise RuntimeError("cubical_msf_gpu requires at least one CUDA device")
    return cp


def _raw_kernels(cp: Any) -> dict[str, Any]:
    global _RAW_MODULE
    names = (
        "init_state",
        "reset_best",
        "compress_parents",
        "scan_implicit_edges",
        "collect_selected_edges_grid",
        "union_selected_edges",
    )
    if _RAW_MODULE is None:  # pragma: no branch - a one-time lazy compilation
        _RAW_MODULE = cp.RawModule(
            code=_CUDA_SOURCE,
            options=("-std=c++11",),
            name_expressions=names,
        )
    return {name: _RAW_MODULE.get_function(name) for name in names}


def cubical_msf_gpu(
    activations: Any,
    dims: int | Sequence[int] | None = None,
    *,
    threads_per_block: int = 256,
    progress: ProgressCallback | None = None,
) -> CubicalMSFResult:
    """Build the cubical MSF with an implicit-edge CuPy Boruvka backend.

    CUDA keys pack an order-preserving float32 weight and a uint32 edge ID.
    The function therefore requires float32 activations and never silently
    downcasts float64 input.  The compact final forest is copied to NumPy so it
    has the same result contract and cut operation as the CPU reference.
    """

    cp = _require_cupy()
    values_array = cp.asarray(activations)
    if dims is None:
        if not 1 <= values_array.ndim <= 3:
            raise ValueError(
                "activations must be a 1-D, 2-D, or 3-D array when dims is omitted"
            )
        grid_dims = _validate_dims(values_array.shape)
    else:
        grid_dims = _validate_dims(dims)
        if int(values_array.size) != math.prod(grid_dims):
            raise ValueError(
                f"activations has {int(values_array.size)} values but dims={grid_dims} "
                f"requires {math.prod(grid_dims)}"
            )
    if values_array.dtype.kind != "f":
        raise TypeError("activation values must have a floating-point dtype")
    if values_array.dtype != cp.float32:
        raise TypeError(
            "cubical_msf_gpu requires float32 activations; cast explicitly so "
            "the precision change is not silent"
        )
    values = cp.ascontiguousarray(values_array).reshape(-1)
    if not bool(cp.all(cp.isfinite(values)).item()):
        raise ValueError("activation values must all be finite")

    n_vertices = int(values.size)
    _validate_edge_ids(n_vertices)
    births = cp.asnumpy(values)
    if n_vertices == 1:
        return CubicalMSFResult(
            births=births,
            edges=np.empty((0, 2), dtype=np.int32),
            weights=np.empty(0, dtype=np.float32),
            dims=grid_dims,
        )
    if not 32 <= int(threads_per_block) <= 1024:
        raise ValueError("threads_per_block must be between 32 and 1024")

    nx, ny, nz = _dims3(grid_dims)
    kernels = _raw_kernels(cp)
    threads = int(threads_per_block)
    vertex_blocks = ((n_vertices + threads - 1) // threads,)
    block = (threads,)

    parent = cp.empty(n_vertices, dtype=cp.int32)
    best_key = cp.empty(n_vertices, dtype=cp.uint64)
    output_u = cp.empty(n_vertices - 1, dtype=cp.int32)
    output_v = cp.empty(n_vertices - 1, dtype=cp.int32)
    output_edge_id = cp.empty(n_vertices - 1, dtype=cp.uint32)
    output_count = cp.zeros(1, dtype=cp.uint32)

    kernels["init_state"](
        vertex_blocks, block, (parent, best_key, np.int32(n_vertices))
    )
    phase_start = 0
    max_phases = int(math.ceil(math.log2(n_vertices))) + 2
    progress_started = time.perf_counter()
    for _phase in range(max_phases):
        kernels["reset_best"](
            vertex_blocks, block, (best_key, np.int32(n_vertices))
        )
        kernels["scan_implicit_edges"](
            vertex_blocks,
            block,
            (
                values,
                np.int32(nx),
                np.int32(ny),
                np.int32(nz),
                np.int32(n_vertices),
                parent,
                best_key,
            ),
        )
        kernels["collect_selected_edges_grid"](
            vertex_blocks,
            block,
            (
                np.int32(nx),
                np.int32(ny),
                np.int32(nz),
                np.int32(n_vertices),
                parent,
                best_key,
                output_u,
                output_v,
                output_edge_id,
                output_count,
            ),
        )
        phase_end = int(output_count.get()[0])
        if phase_end == phase_start:
            raise RuntimeError(
                "CUDA Boruvka made no progress on a connected cubical grid"
            )
        selected_count = phase_end - phase_start
        selected_blocks = ((selected_count + threads - 1) // threads,)
        kernels["union_selected_edges"](
            selected_blocks,
            block,
            (
                output_u,
                output_v,
                np.int32(phase_start),
                np.int32(phase_end),
                parent,
            ),
        )
        kernels["compress_parents"](
            vertex_blocks, block, (parent, np.int32(n_vertices))
        )
        phase_start = phase_end
        if progress is not None:
            emit_progress(
                progress,
                stage="cubical_msf",
                kind="progress",
                completed=phase_end,
                total=n_vertices - 1,
                unit="edges",
                started_at=progress_started,
                details={
                    "phase": int(_phase + 1),
                    "max_phases": int(max_phases),
                    "selected_edges": int(selected_count),
                },
            )
        if phase_end == n_vertices - 1:
            break

    if phase_start != n_vertices - 1:
        raise RuntimeError(
            "CUDA Boruvka did not produce a spanning tree within the expected "
            f"number of phases ({phase_start}/{n_vertices - 1} edges)"
        )

    if progress is not None:
        emit_progress(
            progress,
            stage="cubical_msf",
            kind="message",
            completed=phase_start,
            total=n_vertices - 1,
            unit="edges",
            started_at=progress_started,
            details={"operation": "device_to_host_and_deterministic_sort"},
        )

    # Atomic output positions are intentionally unordered.  Sorting the compact
    # N-1 result by the common total key makes the public result bitwise stable.
    edges = np.column_stack(
        (cp.asnumpy(output_u), cp.asnumpy(output_v))
    ).astype(np.int32, copy=False)
    edge_ids = cp.asnumpy(output_edge_id)
    weights = np.maximum(births[edges[:, 0]], births[edges[:, 1]])
    weights[weights == 0] = np.float32(0)
    order = np.lexsort((edge_ids, weights))
    return CubicalMSFResult(
        births=births,
        edges=edges[order],
        weights=weights[order],
        dims=grid_dims,
    )


def build_cubical_msf(
    activations: Any,
    dims: int | Sequence[int] | None = None,
    *,
    backend: str = "cpu",
    threads_per_block: int = 256,
    progress: ProgressCallback | None = None,
) -> CubicalMSFResult:
    """Dispatch to the explicit ``cpu`` or ``cupy`` cubical MSF backend."""

    normalized_backend = backend.lower().replace("-", "_")
    if normalized_backend in {"cpu", "reference"}:
        return cubical_msf_cpu(activations, dims=dims)
    if normalized_backend in {"cupy", "cuda", "gpu"}:
        return cubical_msf_gpu(
            activations,
            dims=dims,
            threads_per_block=threads_per_block,
            progress=progress,
        )
    raise ValueError("backend must be one of: cpu, reference, cupy, cuda, gpu")


def _implicit_edge_count(dims: tuple[int, ...]) -> int:
    nx, ny, nz = _dims3(dims)
    count = 0
    for dx, dy, dz in _FORWARD_OFFSETS_13:
        count += (nx - abs(dx)) * (ny - abs(dy)) * (nz - abs(dz))
    return int(count)


def estimate_cubical_msf_memory(
    dims: int | Sequence[int],
    *,
    backend: str = "gpu",
    activation_dtype: Any = np.float32,
) -> dict[str, Any]:
    """Estimate peak storage without allocating a grid or its implicit edges.

    The CUDA estimate follows the arrays used by :func:`cubical_msf_gpu` and is
    intentionally conservative for the host-side final sort.  The returned
    ``explicit_edge_list_bytes_avoided`` quantifies a conventional
    ``(u, v, weight, edge_id)`` edge list that this implementation does not
    allocate.  Activation storage is counted once; a caller that retains an
    additional non-contiguous CuPy source view must add that view separately.
    """

    grid_dims = _validate_dims(dims)
    n_vertices = math.prod(grid_dims)
    _validate_edge_ids(n_vertices)
    dtype = np.dtype(activation_dtype)
    if dtype.kind != "f":
        raise TypeError("activation_dtype must be floating point")
    normalized_backend = backend.lower().replace("-", "_")
    n_tree_edges = max(0, n_vertices - 1)
    n_implicit_edges = _implicit_edge_count(grid_dims)
    explicit_edge_list_bytes = n_implicit_edges * (4 + 4 + dtype.itemsize + 4)

    births_bytes = n_vertices * dtype.itemsize
    output_edges_bytes = n_tree_edges * 2 * np.dtype(np.int32).itemsize
    output_weights_bytes = n_tree_edges * dtype.itemsize
    result_bytes = births_bytes + output_edges_bytes + output_weights_bytes

    if normalized_backend in {"cpu", "reference"}:
        parent_bytes = n_vertices * np.dtype(np.int32).itemsize
        event_order_bytes = n_vertices * np.dtype(np.intp).itemsize
        host_peak_bytes = result_bytes + parent_bytes + event_order_bytes
        device_peak_bytes = 0
        workspace_bytes = parent_bytes + event_order_bytes
    elif normalized_backend in {"cupy", "cuda", "gpu"}:
        if dtype != np.dtype(np.float32):
            raise TypeError(
                "the CUDA memory model requires float32 activations, matching "
                "cubical_msf_gpu"
            )
        parent_bytes = n_vertices * np.dtype(np.int32).itemsize
        best_key_bytes = n_vertices * np.dtype(np.uint64).itemsize
        output_edge_ids_bytes = n_tree_edges * np.dtype(np.uint32).itemsize
        counter_bytes = np.dtype(np.uint32).itemsize
        device_peak_bytes = (
            births_bytes
            + parent_bytes
            + best_key_bytes
            + output_edges_bytes
            + output_edge_ids_bytes
            + counter_bytes
        )
        # During deterministic host sorting, retain raw output, edge IDs, an
        # intp permutation, and the final compact result.
        host_sort_workspace = (
            2 * output_edges_bytes
            + output_edge_ids_bytes
            + output_weights_bytes
            + n_tree_edges * np.dtype(np.intp).itemsize
        )
        host_peak_bytes = result_bytes + host_sort_workspace
        workspace_bytes = (
            parent_bytes + best_key_bytes + output_edge_ids_bytes + counter_bytes
        )
    else:
        raise ValueError("backend must be one of: cpu, reference, cupy, cuda, gpu")

    return {
        "dims": grid_dims,
        "n_vertices": n_vertices,
        "n_tree_edges": n_tree_edges,
        "n_implicit_edges": n_implicit_edges,
        "activation_dtype": dtype.name,
        "backend": "cpu" if normalized_backend in {"cpu", "reference"} else "gpu",
        "result_bytes": int(result_bytes),
        "workspace_bytes": int(workspace_bytes),
        "device_peak_bytes": int(device_peak_bytes),
        "host_peak_bytes": int(host_peak_bytes),
        "estimated_peak_bytes": int(max(device_peak_bytes, host_peak_bytes)),
        "explicit_edge_list_bytes_avoided": int(explicit_edge_list_bytes),
        "materializes_implicit_edge_list": False,
        "assumes_single_contiguous_activation_buffer": True,
    }


__all__ = [
    "CubicalMSFResult",
    "build_cubical_msf",
    "cubical_msf_cpu",
    "cubical_msf_gpu",
    "estimate_cubical_msf_memory",
]
