#!/usr/bin/env python3
"""F0 borné du fold horizontal Gate D.

La vérité reconstruit les composantes par matrice booléenne et fermeture de
Warshall. Le sujet emploie un DSU séparé. Ce fichier est un falsificateur
combinatoire d'audit : il ne construit ni Gamma, ni cellule, ni mosaïque de
Delaunay d'ordre supérieur et n'est pas un chemin produit.
"""

from __future__ import annotations

from dataclasses import dataclass
from fractions import Fraction
from itertools import combinations
from typing import Callable, Iterable


class FoldError(ValueError):
    """Lot refusé avant commit."""


class GateFailure(AssertionError):
    """Obligation de la porte non tenue.

    Les obligations étaient portées par `assert`. Sous `python3 -O` elles
    disparaissent toutes et le script imprime `PASS` sans rien avoir vérifié :
    une porte qui peut être désactivée par un drapeau d'interpréteur n'est pas
    une porte. Chaque obligation passe donc par `_require`, qui lève
    inconditionnellement.
    """


def _require(condition: object, message: str) -> None:
    if not condition:
        raise GateFailure(message)


@dataclass(frozen=True, order=True)
class Node:
    kind: str
    token: str

    def __post_init__(self) -> None:
        if self.kind not in {"R", "L", "N"}:
            raise FoldError(f"unknown namespace: {self.kind}")


@dataclass(frozen=True)
class RootSig:
    kind: str
    level: Fraction | None = None
    label: str = ""
    children: tuple["RootSig", ...] = ()
    sources: tuple["RecordStamp", ...] = ()

    def key(self) -> tuple[object, ...]:
        """Clef structurelle, jamais une concaténation ambiguë de labels libres."""

        level_key = (
            ()
            if self.level is None
            else (self.level.numerator, self.level.denominator)
        )
        return (
            self.kind,
            level_key,
            self.label,
            self.sources,
            tuple(child.key() for child in self.children),
        )

    def encode(self) -> str:
        """Encodage injectif lisible pour les tokens synthétiques de mutation."""

        return repr(self.key())


@dataclass(frozen=True)
class Record:
    kind: str
    key: str
    endpoints: tuple[Node, ...]
    raw_arity: int
    provenances: tuple[str, ...] = ()
    source_handles: tuple[str, ...] = ()

    def __post_init__(self) -> None:
        if self.kind not in {"direct", "attach"}:
            raise FoldError(f"unknown record kind: {self.kind}")


@dataclass(frozen=True, order=True)
class RecordStamp:
    """Identité canonique complète d'un record logique publié."""

    kind: str
    key: str
    endpoints: tuple[Node, ...]
    raw_arity: int
    provenances: tuple[str, ...]
    source_handles: tuple[str, ...]


@dataclass(frozen=True)
class ProjectedBatch:
    level: Fraction
    nodes: tuple[Node, ...]
    records: tuple[Record, ...]


@dataclass(frozen=True)
class Component:
    nodes: tuple[Node, ...]
    q_roots: int
    action: str
    active_root: RootSig | None
    logical_records: tuple[RecordStamp, ...]


@dataclass(frozen=True)
class BatchResult:
    components: tuple[Component, ...]


@dataclass(frozen=True)
class Binding:
    handle: str
    activation: Fraction
    strict_projection: Node | None = None


@dataclass(frozen=True)
class RawRecord:
    kind: str
    key: str
    handles: tuple[str, ...]
    provenances: tuple[str, ...] = ()


@dataclass(frozen=True)
class RawBatch:
    level: Fraction
    bindings: tuple[Binding, ...]
    records: tuple[RawRecord, ...]


def _node_key(node: Node) -> tuple[str, str]:
    return (node.kind, node.token)


def _record_key(
    record: Record,
) -> tuple[str, str, tuple[Node, ...], int, tuple[str, ...], tuple[str, ...]]:
    return (
        record.kind,
        record.key,
        record.endpoints,
        record.raw_arity,
        record.provenances,
        record.source_handles,
    )


def _record_stamp(record: Record) -> RecordStamp:
    return RecordStamp(
        record.kind,
        record.key,
        record.endpoints,
        record.raw_arity,
        record.provenances,
        record.source_handles,
    )


def _normalize_records(records: Iterable[Record]) -> tuple[Record, ...]:
    by_key: dict[tuple[str, str], Record] = {}
    for record in records:
        endpoints = (
            tuple(sorted(record.endpoints, key=_node_key))
            if record.kind == "direct"
            else record.endpoints
        )
        normalized = Record(
            record.kind,
            record.key,
            endpoints,
            record.raw_arity,
            tuple(sorted(set(record.provenances))),
            (
                tuple(sorted(record.source_handles))
                if record.kind == "direct"
                else record.source_handles
            ),
        )
        semantic_key = (record.kind, record.key)
        previous = by_key.get(semantic_key)
        if previous is not None:
            previous_payload = (
                previous.endpoints,
                previous.raw_arity,
                previous.source_handles,
            )
            normalized_payload = (
                normalized.endpoints,
                normalized.raw_arity,
                normalized.source_handles,
            )
            if previous_payload != normalized_payload:
                raise FoldError(f"contradictory duplicate: {record.kind}:{record.key}")
            normalized = Record(
                normalized.kind,
                normalized.key,
                normalized.endpoints,
                normalized.raw_arity,
                tuple(sorted(set(previous.provenances) | set(normalized.provenances))),
                normalized.source_handles,
            )
        by_key[semantic_key] = normalized
    return tuple(sorted(by_key.values(), key=_record_key))


def _validate_projected(batch: ProjectedBatch) -> tuple[tuple[Node, ...], tuple[Record, ...]]:
    nodes = tuple(sorted(set(batch.nodes), key=_node_key))
    if len(nodes) != len(batch.nodes):
        raise FoldError("duplicate projected node")
    node_set = set(nodes)
    records = _normalize_records(batch.records)
    attachment_target: dict[Node, Node] = {}
    for record in records:
        if not record.endpoints or any(endpoint not in node_set for endpoint in record.endpoints):
            raise FoldError(f"unknown or empty endpoint: {record.key}")
        if record.kind == "direct":
            if not 2 <= record.raw_arity <= 11:
                raise FoldError(f"direct arity outside product domain: {record.key}")
            if len(set(record.endpoints)) > record.raw_arity:
                raise FoldError(f"projected arity exceeds raw arity: {record.key}")
        else:
            if record.raw_arity != 2 or len(record.endpoints) != 2:
                raise FoldError(f"attachment is not a pair: {record.key}")
            fresh, target = record.endpoints
            if fresh.kind != "N" or target.kind != "R":
                raise FoldError(f"attachment is not N_a -> R^-: {record.key}")
            previous_target = attachment_target.get(fresh)
            if previous_target is not None and previous_target != target:
                raise FoldError(f"one fresh facet has two attachment targets: {fresh.token}")
            attachment_target[fresh] = target
    return nodes, records


def _root_from_nodes(nodes: Iterable[Node]) -> tuple[RootSig, ...]:
    roots = {RootSig("seed", label=node.token) for node in nodes if node.kind == "R"}
    return tuple(sorted(roots, key=RootSig.key))


def _truth_classify(
    level: Fraction,
    nodes: tuple[Node, ...],
    records: tuple[Record, ...],
    roots: tuple[RootSig, ...],
    *,
    q_override: int | None = None,
    roots_override: tuple[RootSig, ...] | None = None,
    ledger_override: tuple[RecordStamp, ...] | None = None,
) -> Component:
    q = len(roots) if q_override is None else q_override
    decision_roots = roots if roots_override is None else roots_override
    logical_records = (
        tuple(sorted(_record_stamp(record) for record in records))
        if ledger_override is None
        else ledger_override
    )
    if not records:
        if q == 0:
            return Component(nodes, 0, "latent", None, ())
        if q == 1:
            return Component(nodes, 1, "untouched_root", decision_roots[0], ())
        raise FoldError("invalid strict snapshot: several roots already connected")
    # LA GARDE DE CARRIER EST RETIRÉE, ET C'ÉTAIT UN P0.
    #
    # L'ancienne ligne refusait toute composante portant un record sans `R` ni
    # `L`. Elle contredisait la table du fold général : une composante à zéro
    # racine stricte portant une `DirectHyperedge` DOIT créer une naissance. Le
    # carré tout `N_a` (ABCD de la note §2.3) est exactement ce cas, et il est
    # géométrique — quatre facettes de même miniboule, deux supports diagonaux.
    # La vérité et le sujet le rejetaient ENSEMBLE, si bien que le différentiel
    # ne pouvait pas le voir : deux implémentations d'une règle fausse
    # concordent parfaitement.
    #
    # L'invariant « au moins deux facettes strictes » existe bien, mais il
    # appartient au validateur de source RÉGULIÈRE, qui juge chaque record BRUT
    # avant projection. Une garde posée après fermeture, au niveau de la
    # composante, ne valide même pas cet invariant : un second record porteur
    # d'un `L` masque le premier record malformé.
    has_direct = any(record.kind == "direct" for record in records)
    if q == 0:
        if not has_direct:
            raise FoldError("attachment-only q=0 component")
        direct_sources = tuple(
            sorted(_record_stamp(record) for record in records if record.kind == "direct")
        )
        root = RootSig("birth", level=level, sources=direct_sources)
        return Component(nodes, 0, "birth", root, logical_records)
    if q == 1:
        return Component(nodes, 1, "continuation", decision_roots[0], logical_records)
    if not has_direct:
        raise FoldError("attachment-only q>=2 component")
    root = RootSig(
        "merge",
        level=level,
        children=tuple(sorted(decision_roots, key=RootSig.key)),
    )
    return Component(nodes, q, "multifusion", root, logical_records)


def _subject_classify(
    level: Fraction,
    nodes: tuple[Node, ...],
    records: tuple[Record, ...],
    roots: tuple[RootSig, ...],
    *,
    q_override: int | None = None,
    roots_override: tuple[RootSig, ...] | None = None,
    ledger_override: tuple[RecordStamp, ...] | None = None,
    reject_carrierless: bool = False,
) -> Component:
    """Décision du sujet, volontairement séparée de la vérité."""

    root_count = len(roots) if q_override is None else q_override
    decision_roots = roots if roots_override is None else roots_override
    ledger = (
        tuple(sorted(_record_stamp(record) for record in records))
        if ledger_override is None
        else ledger_override
    )
    if len(records) == 0:
        if root_count == 0:
            return Component(nodes, 0, "latent", None, ())
        if root_count == 1:
            return Component(nodes, 1, "untouched_root", decision_roots[0], ())
        raise FoldError("subject found several roots in an untouched component")
    # LA POLARITÉ EST INVERSÉE : refuser est désormais LE MUTANT.
    if reject_carrierless and not any(node.kind in {"R", "L"} for node in nodes):
        raise FoldError("subject found a record-bearing component without a strict carrier")
    if root_count == 0:
        direct_sources = tuple(
            sorted(_record_stamp(record) for record in records if record.kind == "direct")
        )
        if not direct_sources:
            raise FoldError("subject found an attachment-only rootless component")
        active = RootSig("birth", level=level, sources=direct_sources)
        return Component(nodes, 0, "birth", active, ledger)
    if root_count == 1:
        return Component(nodes, 1, "continuation", decision_roots[0], ledger)
    if not any(record.kind == "direct" for record in records):
        raise FoldError("subject found an attachment-only multiroot component")
    active = RootSig(
        "merge",
        level=level,
        children=tuple(sorted(decision_roots, key=RootSig.key)),
    )
    return Component(nodes, root_count, "multifusion", active, ledger)


def fold_warshall(batch: ProjectedBatch) -> BatchResult:
    """Vérité indépendante : matrice booléenne, puis Warshall."""

    nodes, records = _validate_projected(batch)
    index = {node: i for i, node in enumerate(nodes)}
    matrix = [[False] * len(nodes) for _ in nodes]
    for i in range(len(nodes)):
        matrix[i][i] = True
    for record in records:
        pivot = index[record.endpoints[0]]
        for endpoint in record.endpoints[1:]:
            other = index[endpoint]
            matrix[pivot][other] = True
            matrix[other][pivot] = True
    for k in range(len(nodes)):
        for i in range(len(nodes)):
            if not matrix[i][k]:
                continue
            for j in range(len(nodes)):
                matrix[i][j] = matrix[i][j] or matrix[k][j]

    groups: dict[int, list[Node]] = {}
    for i, node in enumerate(nodes):
        representative = next(j for j in range(len(nodes)) if matrix[i][j])
        groups.setdefault(representative, []).append(node)
    records_by_group: dict[int, list[Record]] = {group: [] for group in groups}
    for record in records:
        i = index[record.endpoints[0]]
        representative = next(j for j in range(len(nodes)) if matrix[i][j])
        records_by_group[representative].append(record)

    components = []
    for representative, members in groups.items():
        ordered = tuple(sorted(members, key=_node_key))
        local_records = tuple(sorted(records_by_group[representative], key=_record_key))
        components.append(
            _truth_classify(batch.level, ordered, local_records, _root_from_nodes(ordered))
        )
    return BatchResult(
        tuple(sorted(components, key=lambda component: tuple(map(_node_key, component.nodes))))
    )


def _set_partitions(items: tuple[Node, ...]) -> Iterable[tuple[tuple[Node, ...], ...]]:
    """Toutes les partitions d'un petit ensemble, par récurrence de Bell."""

    if not items:
        yield ()
        return
    head, rest = items[0], items[1:]
    for partition in _set_partitions(rest):
        for i in range(len(partition)):
            yield partition[:i] + ((head,) + partition[i],) + partition[i + 1 :]
        yield ((head,),) + partition


PARTITION_ORACLE_MAX_NODES = 5


def fold_partitions(batch: ProjectedBatch) -> BatchResult:
    """Troisième vérité, sans fermeture transitive du tout.

    Warshall et le DSU calculent tous les deux une fermeture transitive : ils
    peuvent partager une erreur de FERMETURE et concorder. Cet oracle ne ferme
    rien. Il énumère toutes les partitions de l'ensemble des sommets projetés,
    retient celles dont chaque bloc contient entièrement les endpoints de chaque
    record, puis exige qu'il en existe une, et une seule, qui RAFFINE toutes les
    autres. C'est cette partition-là qui est la bonne, par définition du
    quotient, et son unicité est vérifiée et non supposée.

    Le domaine est explicitement borné : au-delà de cinq sommets, Bell explose
    et l'oracle se déclare inapplicable au lieu de mentir par omission.
    """

    nodes, records = _validate_projected(batch)
    if len(nodes) > PARTITION_ORACLE_MAX_NODES:
        raise FoldError("partition oracle outside its declared domain")

    valid: list[tuple[frozenset[Node], ...]] = []
    for partition in _set_partitions(nodes):
        blocks = tuple(frozenset(block) for block in partition)
        ok = True
        for record in records:
            endpoints = set(record.endpoints)
            if not any(endpoints <= block for block in blocks):
                ok = False
                break
        if ok:
            valid.append(blocks)
    if not valid:
        raise FoldError("no partition keeps every record inside one block")

    def refines(fine: tuple[frozenset[Node], ...], coarse: tuple[frozenset[Node], ...]) -> bool:
        return all(any(block <= other for other in coarse) for block in fine)

    finest = [candidate for candidate in valid if all(refines(candidate, other) for other in valid)]
    if len(finest) != 1:
        raise GateFailure(
            f"the finest record-respecting partition is not unique: {len(finest)} candidates"
        )

    components = []
    for block in finest[0]:
        ordered = tuple(sorted(block, key=_node_key))
        member = set(ordered)
        local = tuple(
            sorted(
                (record for record in records if set(record.endpoints) <= member),
                key=_record_key,
            )
        )
        components.append(
            _truth_classify(batch.level, ordered, local, _root_from_nodes(ordered))
        )
    return BatchResult(
        tuple(sorted(components, key=lambda component: tuple(map(_node_key, component.nodes))))
    )


# ---------------------------------------------------------------------------
# LE VALIDATEUR DE SOURCE RÉGULIÈRE, QUI N'EST PAS LE CLASSIFICATEUR F0
# ---------------------------------------------------------------------------
#
# Sous la porte régulière forte, Q = I(B) union U(B) avec support unique
# essentiel. Retirer un u de U(B) fait tomber le rayon, retirer un x de I(B) le
# laisse : une `DirectHyperedge` régulière a donc EXACTEMENT |U(B)| facettes
# strictes, et |U(B)| >= 2 pour des points distincts.
#
# Cet invariant est réel, mais il appartient au validateur de source, PAS au
# fold. Et il est LOCAL À CHAQUE RECORD BRUT. Une garde posée après fermeture,
# au niveau de la composante, ne le valide même pas :
#
#     bad_all_new      = DirectHyperedge(N0, N1)
#     good_with_strict = DirectHyperedge(L0, N1)
#
# les deux records deviennent connexes par N1, et le L0 du second masque le
# premier. Le validateur ci-dessous juge donc record par record, sur le lot
# BRUT, avant toute projection.
def validate_regular_source(raw: RawBatch) -> int:
    """Vérifie l'invariant régulier par record brut. Rend le nombre de records jugés."""

    by_handle: dict[str, Binding] = {}
    for binding in raw.bindings:
        if binding.handle in by_handle:
            raise FoldError(f"duplicate activation manifest: {binding.handle}")
        by_handle[binding.handle] = binding
    judged = 0
    for record in raw.records:
        if record.kind != "direct":
            continue
        if any(handle not in by_handle for handle in record.handles):
            raise FoldError(f"unknown raw handle: {record.key}")
        strict = sum(1 for handle in record.handles if by_handle[handle].activation < raw.level)
        if strict < 2:
            raise FoldError(
                f"irregular source: {record.key} carries {strict} strict facet(s), two are required"
            )
        judged += 1
    return judged


def validate_regular_source_by_component_mutant(raw: RawBatch) -> int:
    """Mutation : la même exigence, mais après projection et fermeture."""

    batch = resolve_batch(raw)
    truth = fold_warshall(batch)
    judged = 0
    for component in truth.components:
        if not component.logical_records:
            continue
        strict = sum(1 for node in component.nodes if node.kind in {"R", "L"})
        if strict < 2:
            raise FoldError("irregular component")
        judged += len(component.logical_records)
    return judged


class _Dsu:
    def __init__(self, values: Iterable[object]) -> None:
        self.parent = {value: value for value in values}

    def find(self, value: object) -> object:
        parent = self.parent[value]
        if parent != value:
            self.parent[value] = self.find(parent)
        return self.parent[value]

    def union(self, left: object, right: object) -> None:
        left_root = self.find(left)
        right_root = self.find(right)
        if left_root == right_root:
            return
        if repr(left_root) > repr(right_root):
            left_root, right_root = right_root, left_root
        self.parent[right_root] = left_root


def fold_dsu(
    batch: ProjectedBatch,
    *,
    alias_namespaces: bool = False,
    drop_latents: bool = False,
    count_root_occurrences: bool = False,
    drop_projected_unary_ledger: bool = False,
    drop_record_provenances: bool = False,
    retain_record_keys_only: bool = False,
    reject_carrierless: bool = False,
) -> BatchResult:
    """Sujet DSU; les options sont des mutations volontairement fausses."""

    nodes, records = _validate_projected(batch)
    def storage_key(node: Node) -> object:
        return node.token if alias_namespaces else node

    dsu = _Dsu(storage_key(node) for node in nodes)
    effective: dict[Record, tuple[Node, ...]] = {}
    for record in records:
        kept = tuple(node for node in record.endpoints if not (drop_latents and node.kind == "L"))
        effective[record] = kept
        if not kept:
            continue
        pivot = storage_key(kept[0])
        for endpoint in kept[1:]:
            dsu.union(pivot, storage_key(endpoint))

    groups: dict[object, list[Node]] = {}
    for node in nodes:
        groups.setdefault(dsu.find(storage_key(node)), []).append(node)
    records_by_group: dict[object, list[Record]] = {group: [] for group in groups}
    for record, kept in effective.items():
        if not kept:
            continue
        records_by_group[dsu.find(storage_key(kept[0]))].append(record)

    components = []
    for representative, members in groups.items():
        ordered = tuple(sorted(members, key=_node_key))
        local_records = tuple(sorted(records_by_group[representative], key=_record_key))
        roots = _root_from_nodes(ordered)
        q_override = None
        roots_override = None
        if count_root_occurrences and local_records:
            occurrences = tuple(
                RootSig("seed", label=endpoint.token)
                for record in local_records
                for endpoint in record.endpoints
                if endpoint.kind == "R"
            )
            if occurrences:
                q_override = len(occurrences)
                roots_override = occurrences
        ledger_override = None
        if (
            drop_projected_unary_ledger
            or drop_record_provenances
            or retain_record_keys_only
        ):
            stamps = []
            for record in local_records:
                if (
                    drop_projected_unary_ledger
                    and len({storage_key(node) for node in effective[record]}) <= 1
                ):
                    continue
                stamp = _record_stamp(record)
                if drop_record_provenances:
                    stamp = RecordStamp(
                        stamp.kind,
                        stamp.key,
                        stamp.endpoints,
                        stamp.raw_arity,
                        (),
                        stamp.source_handles,
                    )
                if retain_record_keys_only:
                    stamp = RecordStamp("", stamp.key, (), 0, (), ())
                stamps.append(stamp)
            ledger_override = tuple(sorted(stamps))
        components.append(
            _subject_classify(
                batch.level,
                ordered,
                local_records,
                roots,
                q_override=q_override,
                roots_override=roots_override,
                ledger_override=ledger_override,
                reject_carrierless=reject_carrierless,
            )
        )
    return BatchResult(
        tuple(sorted(components, key=lambda component: tuple(map(_node_key, component.nodes))))
    )


def fold_record_by_record_mutant(batch: ProjectedBatch) -> BatchResult:
    """Mutation : publie une racine intermédiaire après chaque record égal."""

    nodes, records = _validate_projected(batch)
    dsu = _Dsu(nodes)
    active_root: dict[object, RootSig | None] = {
        node: RootSig("seed", label=node.token) if node.kind == "R" else None
        for node in nodes
    }
    ledger: dict[object, set[RecordStamp]] = {node: set() for node in nodes}
    for record in records:
        prior_representatives = {dsu.find(endpoint) for endpoint in record.endpoints}
        roots = {
            active_root[representative]
            for representative in prior_representatives
            if active_root[representative] is not None
        }
        prior_ledger = set().union(*(ledger[representative] for representative in prior_representatives))
        pivot = record.endpoints[0]
        for endpoint in record.endpoints[1:]:
            dsu.union(pivot, endpoint)
        representative = dsu.find(pivot)
        ordered_roots = tuple(sorted(roots, key=RootSig.key))
        if not ordered_roots:
            if record.kind != "direct":
                raise FoldError("recordwise attachment-only q=0")
            new_root = RootSig(
                "birth",
                level=batch.level,
                sources=(_record_stamp(record),),
            )
        elif len(ordered_roots) == 1:
            new_root = ordered_roots[0]
        else:
            new_root = RootSig("merge", level=batch.level, children=ordered_roots)
        active_root[representative] = new_root
        ledger[representative] = prior_ledger | {_record_stamp(record)}

    groups: dict[object, list[Node]] = {}
    for node in nodes:
        groups.setdefault(dsu.find(node), []).append(node)
    components = []
    for representative, members in groups.items():
        ordered = tuple(sorted(members, key=_node_key))
        record_stamps = tuple(sorted(ledger[representative]))
        roots = _root_from_nodes(ordered)
        if not record_stamps:
            components.append(_subject_classify(batch.level, ordered, (), roots))
            continue
        q = len(roots)
        if q == 0:
            action = "birth"
        elif q == 1:
            action = "continuation"
        else:
            action = "multifusion"
        components.append(
            Component(ordered, q, action, active_root[representative], record_stamps)
        )
    return BatchResult(
        tuple(sorted(components, key=lambda component: tuple(map(_node_key, component.nodes))))
    )


def fold_drop_parallel_records_mutant(batch: ProjectedBatch) -> BatchResult:
    """Mutation : déduplique à tort les records par support déjà projeté."""

    nodes, records = _validate_projected(batch)
    seen: set[tuple[str, tuple[Node, ...]]] = set()
    kept = []
    for record in records:
        projected_key = (record.kind, record.endpoints)
        if projected_key in seen:
            continue
        seen.add(projected_key)
        kept.append(record)
    return fold_dsu(ProjectedBatch(batch.level, nodes, tuple(kept)))


def resolve_batch(raw: RawBatch) -> ProjectedBatch:
    """Projette les handles avec la politique stricte <a / courante =a."""

    by_handle: dict[str, Binding] = {}
    for binding in raw.bindings:
        if binding.handle in by_handle:
            raise FoldError(f"duplicate activation manifest: {binding.handle}")
        by_handle[binding.handle] = binding

    projected: dict[str, Node] = {}
    for handle, binding in by_handle.items():
        if binding.activation < raw.level:
            if binding.strict_projection is None or binding.strict_projection.kind not in {"R", "L"}:
                raise FoldError(f"strict handle without R-/L- projection: {handle}")
            projected[handle] = binding.strict_projection
        elif binding.activation == raw.level:
            projected[handle] = Node("N", handle)

    records: list[Record] = []
    for record in raw.records:
        if record.kind not in {"direct", "attach"}:
            raise FoldError(f"unknown raw record kind: {record.kind}")
        if len(set(record.handles)) != len(record.handles):
            raise FoldError(f"duplicate raw endpoint: {record.key}")
        if any(handle not in by_handle for handle in record.handles):
            raise FoldError(f"unknown raw handle: {record.key}")
        if any(handle not in projected for handle in record.handles):
            raise FoldError(f"endpoint activated after cutoff: {record.key}")
        endpoints = tuple(projected[handle] for handle in record.handles)
        if record.kind == "direct":
            if not 2 <= len(record.handles) <= 11:
                raise FoldError(f"raw direct arity outside [2,11]: {record.key}")
            records.append(
                Record(
                    "direct",
                    record.key,
                    endpoints,
                    len(record.handles),
                    record.provenances,
                    tuple(sorted(record.handles)),
                )
            )
            continue
        if len(record.handles) != 2:
            raise FoldError(f"raw attachment is not a pair: {record.key}")
        fresh, target = record.handles
        if by_handle[fresh].activation != raw.level:
            raise FoldError(f"attachment source is not activated in current lot: {record.key}")
        if by_handle[target].activation >= raw.level or endpoints[1].kind != "R":
            raise FoldError(f"attachment target is not rooted in strict snapshot: {record.key}")
        records.append(
            Record(
                "attach",
                record.key,
                endpoints,
                2,
                record.provenances,
                record.handles,
            )
        )
    batch = ProjectedBatch(
        raw.level,
        tuple(sorted(set(projected.values()), key=_node_key)),
        _normalize_records(records),
    )
    _validate_projected(batch)
    return batch


def resolve_with_closed_cutoff_mutant(raw: RawBatch) -> ProjectedBatch:
    """Mutation : laisse les directes du lot fabriquer les cibles d'attache."""

    by_handle = {binding.handle: binding for binding in raw.bindings}
    if len(by_handle) != len(raw.bindings):
        raise FoldError("duplicate activation manifest")
    open_projection: dict[str, Node] = {}
    for handle, binding in by_handle.items():
        if binding.activation < raw.level:
            if binding.strict_projection is None:
                raise FoldError("missing strict projection")
            open_projection[handle] = binding.strict_projection
        elif binding.activation == raw.level:
            open_projection[handle] = Node("N", handle)

    direct_records = []
    for record in raw.records:
        if record.kind != "direct":
            continue
        endpoints = tuple(open_projection[handle] for handle in record.handles)
        direct_records.append(
            Record(
                "direct",
                record.key,
                endpoints,
                len(record.handles),
                record.provenances,
                tuple(sorted(record.handles)),
            )
        )
    direct_batch = ProjectedBatch(
        raw.level,
        tuple(sorted(set(open_projection.values()), key=_node_key)),
        tuple(direct_records),
    )
    closed = fold_warshall(direct_batch)
    closed_projection: dict[Node, Node] = {}
    for component in closed.components:
        if component.active_root is None:
            for node in component.nodes:
                closed_projection[node] = node
        else:
            projected_root = Node("R", component.active_root.encode())
            for node in component.nodes:
                closed_projection[node] = projected_root

    records = []
    for record in raw.records:
        endpoints = tuple(
            closed_projection[open_projection[handle]]
            for handle in record.handles
        )
        records.append(
            Record(
                record.kind,
                record.key,
                endpoints,
                len(record.handles),
                record.provenances,
                (
                    tuple(sorted(record.handles))
                    if record.kind == "direct"
                    else record.handles
                ),
            )
        )
    return ProjectedBatch(
        raw.level,
        tuple(sorted(set(closed_projection.values()), key=_node_key)),
        _normalize_records(records),
    )


class TransactionalSubject:
    """Petit sujet transactionnel; aucun champ autoritatif ne bouge avant commit."""

    STAGES = ("resolve", "closure", "classify", "prepare")

    def __init__(self) -> None:
        self.next_commit_id = 0
        self.commits: list[tuple[int, BatchResult]] = []

    def snapshot(self) -> tuple[int, tuple[tuple[int, BatchResult], ...]]:
        return (self.next_commit_id, tuple(self.commits))

    def apply(self, batch: ProjectedBatch, fail_after: str | None = None) -> BatchResult:
        _validate_projected(batch)
        if fail_after == "resolve":
            raise FoldError("injected after resolve")
        result = fold_dsu(batch)
        if fail_after == "closure":
            raise FoldError("injected after closure")
        classified = result
        if fail_after == "classify":
            raise FoldError("injected after classify")
        prepared = classified
        if fail_after == "prepare":
            raise FoldError("injected after prepare")
        commit_id = self.next_commit_id
        self.commits.append((commit_id, prepared))
        self.next_commit_id += 1
        return prepared


class EagerAllocatorMutant(TransactionalSubject):
    def apply(self, batch: ProjectedBatch, fail_after: str | None = None) -> BatchResult:
        self.next_commit_id += 1
        return super().apply(batch, fail_after)


def _outcome(function: Callable[[], BatchResult]) -> tuple[str, BatchResult | None]:
    try:
        return ("ok", function())
    except FoldError:
        return ("error", None)


def _assert_differential(batch: ProjectedBatch) -> BatchResult:
    truth = _outcome(lambda: fold_warshall(batch))
    subject = _outcome(lambda: fold_dsu(batch))
    if truth != subject:
        raise GateFailure(f"Warshall/DSU divergence:\ntruth={truth}\nsubject={subject}")
    # Warshall et le DSU ferment tous les deux transitivement. Le troisieme
    # oracle ne ferme rien : quand son domaine le permet, il doit concorder.
    if len(set(batch.nodes)) <= PARTITION_ORACLE_MAX_NODES:
        partitions = _outcome(lambda: fold_partitions(batch))
        if partitions != truth:
            raise GateFailure(
                f"partition-oracle divergence:\ntruth={truth}\npartitions={partitions}"
            )
    if truth[0] != "ok" or truth[1] is None:
        raise FoldError("fixture expected an accepted batch")
    return truth[1]


def _raw(
    bindings: Iterable[Binding],
    records: Iterable[RawRecord],
    level: Fraction = Fraction(1),
) -> ProjectedBatch:
    return resolve_batch(RawBatch(level, tuple(bindings), tuple(records)))


def exhaustive_small_domain() -> tuple[int, int, int]:
    """Tous les hypergraphes projetés jusqu'à 5 sommets et 2 records distincts."""

    pool = (
        Node("R", "0"),
        Node("R", "1"),
        Node("L", "0"),
        Node("N", "0"),
        Node("N", "1"),
    )
    checked = accepted = rejected = 0
    for size in range(1, len(pool) + 1):
        for node_tuple in combinations(pool, size):
            direct_records = [
                Record(
                    "direct",
                    "d:" + ",".join(f"{n.kind}{n.token}" for n in endpoints),
                    endpoints,
                    max(2, len(endpoints)),
                )
                for arity in range(1, size + 1)
                for endpoints in combinations(node_tuple, arity)
            ]
            attach_records = [
                Record("attach", f"a:{fresh.token}:{root.token}", (fresh, root), 2)
                for fresh in node_tuple
                for root in node_tuple
                if fresh.kind == "N" and root.kind in {"R", "L"}
            ]
            candidates = tuple(direct_records + attach_records)
            record_sets = [()] + [(record,) for record in candidates]
            record_sets.extend(combinations(candidates, 2))
            for records in record_sets:
                batch = ProjectedBatch(Fraction(1), node_tuple, tuple(records))
                truth = _outcome(lambda batch=batch: fold_warshall(batch))
                subject = _outcome(lambda batch=batch: fold_dsu(batch))
                if truth != subject:
                    raise GateFailure(f"exhaustive F0 mismatch: {batch}\n{truth}\n{subject}")
                partitions = _outcome(lambda batch=batch: fold_partitions(batch))
                if partitions != truth:
                    raise GateFailure(
                        f"exhaustive partition mismatch: {batch}\n{truth}\n{partitions}"
                    )
                checked += 1
                if truth[0] == "ok":
                    accepted += 1
                else:
                    rejected += 1
    return checked, accepted, rejected


def targeted_fixtures() -> dict[str, ProjectedBatch]:
    zero = Fraction(0)
    one = Fraction(1)
    fixtures: dict[str, ProjectedBatch] = {}

    fixtures["namespace_collision"] = _raw(
        (
            Binding("root", zero, Node("R", "7")),
            Binding("latent", zero, Node("L", "7")),
            Binding("7", one),
        ),
        (RawRecord("direct", "birth-on-latent", ("latent", "7")),),
    )
    fixtures["latent_bridge"] = _raw(
        (
            Binding("r1", zero, Node("R", "1")),
            Binding("z", zero, Node("L", "9")),
            Binding("r2", zero, Node("R", "2")),
        ),
        (
            RawRecord("direct", "left", ("r1", "z")),
            RawRecord("direct", "right", ("z", "r2")),
        ),
    )
    fixtures["q0_birth"] = _raw(
        (Binding("latent", zero, Node("L", "1")), Binding("n1", one), Binding("n2", one)),
        (RawRecord("direct", "new-component", ("latent", "n1", "n2")),),
    )
    fixtures["same_root_q1"] = _raw(
        (
            Binding("a", zero, Node("R", "1")),
            Binding("b", zero, Node("R", "1")),
            Binding("n", one),
        ),
        (RawRecord("direct", "same-root", ("a", "b", "n")),),
    )
    fixtures["q3_atomic"] = _raw(
        (
            Binding("r1", zero, Node("R", "1")),
            Binding("r2", zero, Node("R", "2")),
            Binding("r3", zero, Node("R", "3")),
        ),
        (
            RawRecord("direct", "12", ("r1", "r2")),
            RawRecord("direct", "23", ("r2", "r3")),
        ),
    )
    fixtures["valid_attachment"] = _raw(
        (Binding("root", zero, Node("R", "1")), Binding("facet", one)),
        (RawRecord("attach", "facet-attachment", ("facet", "root")),),
    )
    fixtures["provenance_aggregation"] = _raw(
        (Binding("root", zero, Node("R", "1")), Binding("facet", one)),
        (
            RawRecord(
                "attach",
                "same-incidence",
                ("facet", "root"),
                ("certificate-A",),
            ),
            RawRecord(
                "attach",
                "same-incidence",
                ("facet", "root"),
                ("certificate-B",),
            ),
        ),
    )
    fixtures["projected_unary"] = _raw(
        (Binding("a", zero, Node("R", "1")), Binding("b", zero, Node("R", "1"))),
        (RawRecord("direct", "unary-after-projection", ("a", "b")),),
    )
    fixtures["parallel_projected_records"] = _raw(
        (Binding("a", zero, Node("R", "1")), Binding("b", zero, Node("R", "1"))),
        (
            RawRecord("direct", "parallel-A", ("a", "b")),
            RawRecord("direct", "parallel-B", ("a", "b")),
        ),
    )
    fixtures["birth_signature_delimiters"] = ProjectedBatch(
        one,
        (
            Node("L", "1"),
            Node("N", "1"),
            Node("L", "2"),
            Node("N", "2"),
            Node("N", "3"),
        ),
        (
            Record("direct", "a,b", (Node("L", "1"), Node("N", "1")), 2),
            Record("direct", "a", (Node("L", "2"), Node("N", "2")), 2),
            Record("direct", "b", (Node("N", "2"), Node("N", "3")), 2),
        ),
    )

    # LE CARRÉ TOUT N_a — LA FIXTURE QUE LA GARDE DE CARRIER SUPPRIMAIT.
    #
    # A=(0,0,0) B=(2,0,0) C=(2,2,0) D=(0,2,0), plus E=(0,0,10) hors du plan pour
    # que le nuage soit de dimension affine trois. À l'ordre trois, Q=ABCD a pour
    # miniboule le cercle de centre (1,1,0) et de rayon carré 2. Chacun des
    # quatre triangles de Q contient encore une diagonale du carré, donc a la
    # MÊME miniboule et le même niveau : les quatre facettes sont des N_a.
    #
    # Cette coface a deux supports diagonaux, elle échoue donc la porte régulière
    # forte — mais elle reste une coface directe valide du contrat général, et
    # elle doit créer une naissance q_R = 0. La vérité et le sujet la rejetaient
    # ENSEMBLE, ce qu'un différentiel ne peut par construction pas voir.
    fixtures["square_all_new"] = _raw(
        (
            Binding("ABC", one),
            Binding("ABD", one),
            Binding("ACD", one),
            Binding("BCD", one),
        ),
        (RawRecord("direct", "square-coface-ABCD", ("ABC", "ABD", "ACD", "BCD")),),
    )

    # LA FIXTURE RÉGULIÈRE QUI ATTEINT LA BORNE.
    #
    # A=(0,0,0) B=(4,0,0) z=(2,1,0) D=(1,2,10). Pour Q=ABz à l'ordre deux,
    # U={A,B} : les facettes Az et Bz sont STRICTES, AB est égale. Le record
    # porte donc exactement deux endpoints stricts et un N_a — la borne
    # |U(B)| >= 2 est atteinte, pas dépassée.
    fixtures["regular_abz"] = _raw(
        (
            Binding("Az", zero, Node("L", "Az")),
            Binding("Bz", zero, Node("L", "Bz")),
            Binding("AB", one),
        ),
        (RawRecord("direct", "coface-ABz", ("Az", "Bz", "AB")),),
    )

    arity_bindings = [
        Binding("h0", zero, Node("R", "1")),
        Binding("h1", zero, Node("R", "1")),
        Binding("h2", zero, Node("R", "2")),
        Binding("h3", zero, Node("L", "7")),
    ] + [Binding(f"h{i}", one) for i in range(4, 11)]
    fixtures["arity_11"] = _raw(
        arity_bindings,
        (RawRecord("direct", "k10-coface", tuple(f"h{i}" for i in range(11))),),
    )
    return fixtures


def invalid_fixtures() -> int:
    zero = Fraction(0)
    one = Fraction(1)
    two = Fraction(2)
    cases = (
        RawBatch(
            one,
            (Binding("latent", zero, Node("L", "1")), Binding("facet", one)),
            (RawRecord("attach", "latent-target", ("facet", "latent")),),
        ),
        RawBatch(
            one,
            (Binding("future", two), Binding("now", one)),
            (RawRecord("direct", "future-endpoint", ("future", "now")),),
        ),
        RawBatch(
            one,
            (Binding("a", zero, Node("L", "1")), Binding("b", one)),
            (RawRecord("direct", "duplicate-endpoint", ("a", "a")),),
        ),
        RawBatch(
            one,
            (
                Binding("a", zero, Node("R", "1")),
                Binding("b", zero, Node("R", "2")),
                Binding("c", one),
            ),
            (
                RawRecord("direct", "same-key", ("a", "b")),
                RawRecord("direct", "same-key", ("a", "c")),
            ),
        ),
        RawBatch(
            one,
            (
                Binding("r1", zero, Node("R", "1")),
                Binding("r2", zero, Node("R", "2")),
                Binding("f", one),
            ),
            (
                RawRecord("attach", "owner-one", ("f", "r1")),
                RawRecord("attach", "owner-two", ("f", "r2")),
            ),
        ),
        RawBatch(
            one,
            (
                Binding("a", zero, Node("R", "1")),
                Binding("b", zero, Node("R", "1")),
                Binding("c", zero, Node("R", "1")),
            ),
            (
                RawRecord("direct", "projected-conflict", ("a", "b")),
                RawRecord("direct", "projected-conflict", ("a", "c")),
            ),
        ),
    )
    for raw in cases:
        try:
            resolve_batch(raw)
        except FoldError:
            continue
        raise AssertionError(f"invalid raw batch accepted: {raw}")

    projected_attachment_only = ProjectedBatch(
        one,
        (Node("N", "f"), Node("L", "target")),
        (Record("attach", "forged", (Node("N", "f"), Node("L", "target")), 2),),
    )
    _require(
        _outcome(lambda: fold_warshall(projected_attachment_only))[0] == "error",
        "attachment-only projected batch accepted by the truth",
    )
    _require(
        _outcome(lambda: fold_dsu(projected_attachment_only))[0] == "error",
        "attachment-only projected batch accepted by the subject",
    )

    exact_duplicate = RawBatch(
        one,
        (Binding("a", zero, Node("R", "1")), Binding("b", one)),
        (
            RawRecord("direct", "same", ("a", "b"), ("certificate-A",)),
            RawRecord("direct", "same", ("b", "a"), ("certificate-B",)),
        ),
    )
    normalized = resolve_batch(exact_duplicate)
    _require(len(normalized.records) == 1, "exact duplicate was not merged")
    _require(
        normalized.records[0].provenances == ("certificate-A", "certificate-B"),
        "duplicate merge lost a provenance",
    )
    _assert_differential(normalized)

    # LA SOURCE IRRÉGULIÈRE EST REFUSÉE PAR SON VALIDATEUR, PAS PAR LE FOLD.
    #
    # `bad_all_new` n'a AUCUNE facette stricte : sous la porte régulière forte
    # elle est malformée. `good_with_strict` en a deux, elle est parfaitement
    # régulière. Les deux records deviennent connexes par `n1`, si bien que la
    # composante en porte deux : une garde posée sur la COMPOSANTE la déclare
    # régulière et laisse passer le premier record. Le bon record MASQUE le
    # mauvais, et c'est exactement ce qu'un jugement par record refuse.
    smuggling = RawBatch(
        one,
        (
            Binding("l0", zero, Node("L", "0")),
            Binding("l1", zero, Node("L", "1")),
            Binding("n0", one),
            Binding("n1", one),
        ),
        (
            RawRecord("direct", "bad_all_new", ("n0", "n1")),
            RawRecord("direct", "good_with_strict", ("l0", "l1", "n1")),
        ),
    )
    try:
        validate_regular_source(smuggling)
    except FoldError:
        pass
    else:
        raise GateFailure("the regular validator accepted a record without two strict facets")
    # Et le fold GÉNÉRAL, lui, doit l'accepter : ce lot est une naissance valide
    # hors porte régulière. Les deux jugements sont indépendants, et c'est le
    # point.
    _require(
        _outcome(lambda: fold_warshall(resolve_batch(smuggling)))[0] == "ok",
        "the general fold refused a batch that only the regular gate may refuse",
    )
    return len(cases) + 2


def permutation_campaign(fixtures: dict[str, ProjectedBatch]) -> int:
    for batch in fixtures.values():
        permuted_records = tuple(
            Record(
                record.kind,
                record.key,
                tuple(reversed(record.endpoints)) if record.kind == "direct" else record.endpoints,
                record.raw_arity,
                record.provenances,
                record.source_handles,
            )
            for record in reversed(batch.records)
        )
        permuted = ProjectedBatch(batch.level, tuple(reversed(batch.nodes)), permuted_records)
        expected = fold_warshall(batch)
        _require(fold_warshall(permuted) == expected, "the truth is not permutation-invariant")
        _require(fold_dsu(permuted) == expected, "the subject is not permutation-invariant")
    return len(fixtures)


def mutation_campaign(fixtures: dict[str, ProjectedBatch]) -> tuple[str, ...]:
    killed: list[str] = []

    def kill(name: str, truth: BatchResult, mutant: Callable[[], BatchResult]) -> None:
        if _outcome(mutant) == ("ok", truth):
            raise AssertionError(f"mutation survived: {name}")
        killed.append(name)

    kill(
        "erase_R_L_namespace",
        fold_warshall(fixtures["namespace_collision"]),
        lambda: fold_dsu(fixtures["namespace_collision"], alias_namespaces=True),
    )
    kill(
        "drop_latent_before_closure",
        fold_warshall(fixtures["latent_bridge"]),
        lambda: fold_dsu(fixtures["latent_bridge"], drop_latents=True),
    )
    kill(
        "count_root_occurrences",
        fold_warshall(fixtures["same_root_q1"]),
        lambda: fold_dsu(fixtures["same_root_q1"], count_root_occurrences=True),
    )
    kill(
        "drop_projected_unary_source",
        fold_warshall(fixtures["projected_unary"]),
        lambda: fold_dsu(fixtures["projected_unary"], drop_projected_unary_ledger=True),
    )
    kill(
        "drop_record_provenances",
        fold_warshall(fixtures["provenance_aggregation"]),
        lambda: fold_dsu(fixtures["provenance_aggregation"], drop_record_provenances=True),
    )
    kill(
        "retain_record_keys_only",
        fold_warshall(fixtures["valid_attachment"]),
        lambda: fold_dsu(fixtures["valid_attachment"], retain_record_keys_only=True),
    )
    kill(
        "deduplicate_parallel_projected_records",
        fold_warshall(fixtures["parallel_projected_records"]),
        lambda: fold_drop_parallel_records_mutant(fixtures["parallel_projected_records"]),
    )
    kill(
        "commit_record_by_record",
        fold_warshall(fixtures["q3_atomic"]),
        lambda: fold_record_by_record_mutant(fixtures["q3_atomic"]),
    )

    # Une validation à coupe fermée accepterait cette attache, car la directe du même
    # lot fait naître la cible. La règle stricte doit au contraire la refuser.
    closed_cutoff = RawBatch(
        Fraction(1),
        (
            Binding("target", Fraction(0), Node("L", "target")),
            Binding("bridge", Fraction(1)),
            Binding("facet", Fraction(1)),
        ),
        (
            RawRecord("direct", "birth-target", ("target", "bridge")),
            RawRecord("attach", "too-late", ("facet", "target")),
        ),
    )
    _require(
        _outcome(lambda: fold_dsu(resolve_batch(closed_cutoff)))[0] == "error",
        "the strict cutoff accepted an attachment onto a target born in the same lot",
    )
    _require(
        _outcome(lambda: fold_dsu(resolve_with_closed_cutoff_mutant(closed_cutoff)))[0] == "ok",
        "the closed-cutoff mutant did not even accept what it is built to accept",
    )
    killed.append("find_closed_instead_of_strict")

    # LE MUTANT A CHANGÉ DE CAMP. Refuser une composante sans carrier strict
    # était la règle; c'est maintenant la mutation, et elle doit tuer le carré.
    kill(
        "reject_carrierless_birth",
        fold_warshall(fixtures["square_all_new"]),
        lambda: fold_dsu(fixtures["square_all_new"], reject_carrierless=True),
    )

    # Le validateur régulier par composante laisse passer le record malformé que
    # le validateur par record brut refuse.
    smuggling = RawBatch(
        Fraction(1),
        (
            Binding("l0", Fraction(0), Node("L", "0")),
            Binding("l1", Fraction(0), Node("L", "1")),
            Binding("n0", Fraction(1)),
            Binding("n1", Fraction(1)),
        ),
        (
            RawRecord("direct", "bad_all_new", ("n0", "n1")),
            RawRecord("direct", "good_with_strict", ("l0", "l1", "n1")),
        ),
    )
    try:
        validate_regular_source(smuggling)
    except FoldError:
        pass
    else:
        raise GateFailure("mutation survived undetected: validate_regular_by_component")
    _require(
        validate_regular_source_by_component_mutant(smuggling) == 2,
        "the by-component regular mutant did not accept the smuggled record",
    )
    killed.append("validate_regular_by_component")

    # Le validateur régulier doit rester VRAI sur la fixture régulière : un
    # validateur qui refuse tout tuerait aussi le mutant précédent sans rien
    # valider.
    regular = RawBatch(
        Fraction(1),
        (
            Binding("Az", Fraction(0), Node("L", "Az")),
            Binding("Bz", Fraction(0), Node("L", "Bz")),
            Binding("AB", Fraction(1)),
        ),
        (RawRecord("direct", "coface-ABz", ("Az", "Bz", "AB")),),
    )
    _require(
        validate_regular_source(regular) == 1,
        "the regular validator refuses the regular fixture that attains the bound",
    )
    return tuple(killed)


def rollback_campaign(batch: ProjectedBatch) -> tuple[int, bool]:
    subject = TransactionalSubject()
    before = subject.snapshot()
    for stage in TransactionalSubject.STAGES:
        try:
            subject.apply(batch, fail_after=stage)
        except FoldError:
            pass
        else:
            raise AssertionError(f"failure injection did not fail: {stage}")
        _require(subject.snapshot() == before, f"a field moved before commit at stage {stage}")

    invalid = ProjectedBatch(
        Fraction(1),
        (Node("N", "f"), Node("L", "l")),
        (Record("attach", "invalid", (Node("N", "f"), Node("L", "l")), 2),),
    )
    try:
        subject.apply(invalid)
    except FoldError:
        pass
    else:
        raise AssertionError("invalid attachment committed")
    _require(subject.snapshot() == before, "an invalid batch moved an authoritative field")
    _require(subject.apply(batch) == fold_warshall(batch), "the committed result is not the truth")

    mutant = EagerAllocatorMutant()
    mutant_before = mutant.snapshot()
    try:
        mutant.apply(batch, fail_after="resolve")
    except FoldError:
        pass
    _require(mutant.snapshot() != mutant_before, "the eager allocator mutant survived")
    return (len(TransactionalSubject.STAGES) + 1, True)


def main() -> None:
    checked, accepted, rejected = exhaustive_small_domain()
    fixtures = targeted_fixtures()
    results = {name: _assert_differential(batch) for name, batch in fixtures.items()}

    _require(
        any(component.action == "birth" for component in results["q0_birth"].components),
        "q0_birth did not produce a birth",
    )
    _require(
        any(
            component.action == "continuation"
            for component in results["same_root_q1"].components
        ),
        "same_root_q1 did not produce a continuation",
    )
    # LE CARRÉ TOUT N_a EST UNE NAISSANCE, ET C'EST LE P0 QUI SE FERME ICI.
    square = next(
        component
        for component in results["square_all_new"].components
        if component.logical_records
    )
    _require(
        square.action == "birth" and square.q_roots == 0,
        f"the all-N_a square is {square.action}, not a birth",
    )
    _require(
        len(square.nodes) == 4 and all(node.kind == "N" for node in square.nodes),
        "the all-N_a square lost a facet or gained a carrier",
    )
    _require(
        len(square.logical_records) == 1 and square.logical_records[0].raw_arity == 4,
        "the all-N_a square did not keep one complete direct record of arity four",
    )
    regular_component = next(
        component
        for component in results["regular_abz"].components
        if component.logical_records
    )
    _require(
        sum(1 for node in regular_component.nodes if node.kind in {"R", "L"}) == 2
        and sum(1 for node in regular_component.nodes if node.kind == "N") == 1,
        "the regular ABz fixture no longer carries exactly two strict facets and one equal",
    )
    q3 = next(component for component in results["q3_atomic"].components if component.logical_records)
    _require(q3.q_roots == 3 and q3.action == "multifusion", "q3_atomic is not a multifusion")
    arity = next(component for component in results["arity_11"].components if component.logical_records)
    _require(arity.q_roots == 2 and arity.action == "multifusion", "arity_11 is not a multifusion")
    provenance = next(
        component
        for component in results["provenance_aggregation"].components
        if component.logical_records
    )
    _require(
        provenance.logical_records[0].provenances == ("certificate-A", "certificate-B"),
        "provenance aggregation lost a certificate",
    )
    parallel = next(
        component
        for component in results["parallel_projected_records"].components
        if component.logical_records
    )
    _require(len(parallel.logical_records) == 2, "parallel projected records were deduplicated")
    delimiter_births = {
        component.active_root
        for component in results["birth_signature_delimiters"].components
        if component.action == "birth"
    }
    _require(len(delimiter_births) == 2, "two distinct births collapsed to one signature")
    split_children = RootSig(
        "merge",
        level=Fraction(1),
        children=(RootSig("seed", label="a"), RootSig("seed", label="b")),
    )
    forged_label = RootSig(
        "merge",
        level=Fraction(1),
        children=(RootSig("seed", label="a),R(b"),),
    )
    _require(split_children.key() != forged_label.key(), "a forged label collides with a merge key")
    _require(
        split_children.encode() != forged_label.encode(),
        "a forged label collides with a merge encoding",
    )

    invalid_count = invalid_fixtures()
    permutation_count = permutation_campaign(fixtures)
    killed = mutation_campaign(fixtures)
    rollback_count, allocator_mutant_killed = rollback_campaign(fixtures["arity_11"])

    _require(Fraction(1, 2) == Fraction(2, 4), "rational equality is not canonical")
    _require(
        float(Fraction(2**53 + 1, 2**53)) == float(Fraction(1, 1)),
        "the double collapse witness no longer collapses",
    )
    _require(
        Fraction(2**53 + 1, 2**53) != Fraction(1, 1),
        "the exact level comparison collapsed like a double",
    )

    print(f"exhaustive={checked} accepted={accepted} rejected={rejected}")
    print(
        f"targeted={len(fixtures)} invalid={invalid_count} "
        f"permutations={permutation_count} arity11=PASS"
    )
    print("mutations=" + ",".join(killed))
    print(f"rollback_faults={rollback_count} allocator_mutant_killed={allocator_mutant_killed}")
    print("Gate_D_F0_kernel=PASS GCP=not_used")


if __name__ == "__main__":
    main()
