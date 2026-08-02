#!/usr/bin/env python3
"""Fail-closed runtime primitives for resumable profiling campaigns.

This module is deliberately free of MorseHGP3D scientific semantics.  It
provides strict canonical JSON, content hashes, durable single-writer spool
transactions, and subprocess-group timeout cleanup.  Scientific harnesses
must validate a receipt before passing it to :class:`CampaignSpool`.
"""

from __future__ import annotations

from dataclasses import dataclass
import fcntl
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import stat
import subprocess
import tempfile
from typing import Any, NoReturn, Sequence


IDENTITY_SCHEMA = "morsehgp3d.profiling.campaign_identity.v1"
HEAD_SCHEMA = "morsehgp3d.profiling.campaign_spool_head.v1"
COMMIT_SCHEMA = "morsehgp3d.profiling.campaign_spool_commit.v1"
ACTIVE_RUN_SCHEMA = "morsehgp3d.profiling.campaign_spool_active_run.v1"
CAMPAIGN_DIRECTORY_PREFIX = "campaign-"
INITIAL_RECEIPT_CHAIN_SHA256 = hashlib.sha256(
    b"MorseHGP3D/profiling/campaign-spool/receipt-chain/v1/initial"
).hexdigest()
RECEIPT_CHAIN_DOMAIN = (
    b"MorseHGP3D/profiling/campaign-spool/receipt-chain/v1/extend/"
)
SHA40_RE = re.compile(r"[0-9a-f]{40}\Z")
SHA256_RE = re.compile(r"[0-9a-f]{64}\Z")
FAILPOINT_ENVIRONMENT = "MORSEHGP3D_TRUE_HGP_FAIL_AFTER"


class RuntimeContractError(RuntimeError):
    """A runtime operation escaped its fail-closed contract."""


def fail(message: str) -> NoReturn:
    raise RuntimeContractError(message)


def require(condition: bool, message: str) -> None:
    if not condition:
        fail(message)


def reject_duplicate_keys(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            fail(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def reject_nonfinite(value: str) -> NoReturn:
    fail(f"non-finite JSON constant: {value}")


def canonical_json(value: Any) -> str:
    try:
        return json.dumps(
            value,
            allow_nan=False,
            ensure_ascii=True,
            sort_keys=True,
            separators=(",", ":"),
        )
    except (TypeError, ValueError) as error:
        fail(f"cannot encode canonical JSON: {error}")


def canonical_bytes(value: Any) -> bytes:
    return (canonical_json(value) + "\n").encode("ascii")


def parse_json_text(raw: str, label: str, *, canonical_line: bool) -> Any:
    try:
        value = json.loads(
            raw,
            object_pairs_hook=reject_duplicate_keys,
            parse_constant=reject_nonfinite,
        )
    except (json.JSONDecodeError, RuntimeContractError) as error:
        fail(f"cannot parse {label}: {error}")
    if canonical_line:
        require(
            raw == canonical_json(value) + "\n",
            f"{label} is not canonical one-line JSON",
        )
    return value


def read_canonical_json(path: Path, label: str) -> tuple[dict[str, Any], bytes]:
    require_regular_file(path, label)
    try:
        payload = path.read_bytes()
        raw = payload.decode("ascii")
    except (OSError, UnicodeError) as error:
        fail(f"cannot read {label}: {error}")
    value = parse_json_text(raw, label, canonical_line=True)
    require(isinstance(value, dict), f"{label} must be an object")
    return value, payload


def sha256_bytes(payload: bytes) -> str:
    return hashlib.sha256(payload).hexdigest()


def sha256_file(path: Path, label: str = "file") -> str:
    require_regular_file(path, label)
    digest = hashlib.sha256()
    try:
        with path.open("rb") as stream:
            for block in iter(lambda: stream.read(1024 * 1024), b""):
                digest.update(block)
    except OSError as error:
        fail(f"cannot hash {label}: {error}")
    return digest.hexdigest()


def require_regular_file(path: Path, label: str) -> None:
    require(path.is_file() and not path.is_symlink(), f"{label} must be a regular file")


def fsync_directory(path: Path) -> None:
    try:
        descriptor = os.open(path, os.O_RDONLY | getattr(os, "O_DIRECTORY", 0))
        try:
            os.fsync(descriptor)
        finally:
            os.close(descriptor)
    except OSError as error:
        fail(f"cannot synchronize directory {path}: {error}")


def atomic_publish(path: Path, payload: bytes) -> None:
    require(path.parent.is_dir() and not path.parent.is_symlink(), "publication parent is invalid")
    descriptor, temporary_name = tempfile.mkstemp(
        dir=path.parent, prefix=f".{path.name}.", suffix=".tmp"
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "wb") as stream:
            stream.write(payload)
            stream.flush()
            os.fsync(stream.fileno())
        require(
            sha256_file(temporary, "temporary publication") == sha256_bytes(payload),
            f"temporary publication digest differs for {path.name}",
        )
        os.replace(temporary, path)
        fsync_directory(path.parent)
    except OSError as error:
        fail(f"cannot atomically publish {path}: {error}")
    finally:
        temporary.unlink(missing_ok=True)


def safe_relative_file(value: Any, label: str) -> str:
    require(isinstance(value, str) and value, f"{label} must be a path")
    path = Path(value)
    require(
        not path.is_absolute()
        and all(part not in {"", ".", ".."} for part in path.parts),
        f"{label} escapes the campaign spool",
    )
    return path.as_posix()


def exact_sha256(value: Any, label: str) -> str:
    require(
        isinstance(value, str) and SHA256_RE.fullmatch(value) is not None,
        f"{label} must be a lowercase SHA-256",
    )
    return value


def exact_git_sha(value: Any, label: str = "Git SHA") -> str:
    require(
        isinstance(value, str) and SHA40_RE.fullmatch(value) is not None,
        f"{label} must be a lowercase 40-hex commit ID",
    )
    return value


def natural(value: Any, label: str, *, positive: bool = False) -> int:
    require(type(value) is int and value >= 0, f"{label} must be a natural")
    require(not positive or value > 0, f"{label} must be positive")
    return value


def campaign_identity(
    *,
    git_sha: str,
    binary_sha256: str,
    plan_sha256: str,
    capabilities_sha256: str,
) -> dict[str, str]:
    identity = {
        "binary_sha256": exact_sha256(binary_sha256, "binary SHA-256"),
        "capabilities_sha256": exact_sha256(
            capabilities_sha256, "capabilities SHA-256"
        ),
        "git_sha": exact_git_sha(git_sha),
        "plan_sha256": exact_sha256(plan_sha256, "plan SHA-256"),
        "schema": IDENTITY_SCHEMA,
    }
    identity["campaign_id"] = sha256_bytes(canonical_bytes(identity))
    return identity


def validate_identity(value: Any) -> dict[str, str]:
    require(isinstance(value, dict), "campaign identity must be an object")
    require(
        set(value)
        == {
            "binary_sha256",
            "campaign_id",
            "capabilities_sha256",
            "git_sha",
            "plan_sha256",
            "schema",
        },
        "campaign identity fields differ",
    )
    require(value["schema"] == IDENTITY_SCHEMA, "campaign identity schema differs")
    expected = campaign_identity(
        git_sha=value["git_sha"],
        binary_sha256=value["binary_sha256"],
        plan_sha256=value["plan_sha256"],
        capabilities_sha256=value["capabilities_sha256"],
    )
    require(value == expected, "campaign identity digest differs")
    return value


def extend_receipt_chain(previous: str, ordinal: int, receipt_sha256: str) -> str:
    exact_sha256(previous, "previous receipt chain")
    exact_sha256(receipt_sha256, "receipt SHA-256")
    natural(ordinal, "receipt ordinal")
    return hashlib.sha256(
        RECEIPT_CHAIN_DOMAIN
        + bytes.fromhex(previous)
        + ordinal.to_bytes(8, "big")
        + bytes.fromhex(receipt_sha256)
    ).hexdigest()


def maybe_fail_after(label: str) -> None:
    if os.environ.get(FAILPOINT_ENVIRONMENT) == label:
        raise RuntimeContractError(f"injected true-HGP campaign failure after {label}")


class CampaignSpool:
    """A local single-writer, process-loss-safe campaign receipt chain."""

    def __init__(self, spool_parent: Path, identity: dict[str, str]) -> None:
        self.identity = validate_identity(dict(identity))
        self.spool_parent = spool_parent
        self.root = spool_parent / (
            CAMPAIGN_DIRECTORY_PREFIX + self.identity["campaign_id"]
        )
        self.lock_path = spool_parent / f".{self.root.name}.lock"
        self._lock_descriptor = -1
        self.head: dict[str, Any] | None = None

    def __enter__(self) -> CampaignSpool:
        require(
            not self.spool_parent.is_symlink(),
            "campaign spool parent cannot be a symbolic link",
        )
        self.spool_parent.mkdir(parents=True, exist_ok=True)
        self._lock_descriptor = os.open(
            self.lock_path,
            os.O_CREAT | os.O_RDWR | getattr(os, "O_NOFOLLOW", 0),
            0o600,
        )
        try:
            fcntl.flock(
                self._lock_descriptor, fcntl.LOCK_EX | fcntl.LOCK_NB
            )
        except BlockingIOError as error:
            os.close(self._lock_descriptor)
            self._lock_descriptor = -1
            fail(f"another writer owns campaign spool {self.root}: {error}")
        try:
            if self.root.exists() or self.root.is_symlink():
                self._open_existing()
            else:
                self._initialize()
            return self
        except BaseException:
            self.close()
            raise

    def __exit__(self, *_: object) -> None:
        self.close()

    def close(self) -> None:
        if self._lock_descriptor >= 0:
            try:
                fcntl.flock(self._lock_descriptor, fcntl.LOCK_UN)
            finally:
                os.close(self._lock_descriptor)
                self._lock_descriptor = -1

    @property
    def next_ordinal(self) -> int:
        self._require_open()
        assert self.head is not None
        return self.head["next_ordinal"]

    def _require_open(self) -> None:
        require(self._lock_descriptor >= 0 and self.head is not None, "campaign spool is not open")

    def _initial_head(self) -> dict[str, Any]:
        return {
            "active_run": None,
            "campaign_id": self.identity["campaign_id"],
            "commits": [],
            "next_ordinal": 0,
            "ordered_receipt_chain_sha256": INITIAL_RECEIPT_CHAIN_SHA256,
            "schema": HEAD_SCHEMA,
            "sequence_number": 0,
        }

    def _initialize(self) -> None:
        temporary = Path(
            tempfile.mkdtemp(
                dir=self.spool_parent,
                prefix=f".{self.root.name}.",
                suffix=".init",
            )
        )
        try:
            directory_names = (
                "requests",
                "receipts",
                "checkpoints",
                "attempts",
                "artifacts",
            )
            for name in directory_names:
                (temporary / name).mkdir()
            atomic_publish(temporary / "identity.json", canonical_bytes(self.identity))
            initial_head = self._initial_head()
            atomic_publish(temporary / "HEAD.json", canonical_bytes(initial_head))
            for path in (
                temporary,
                *(temporary / name for name in directory_names),
            ):
                fsync_directory(path)
            os.replace(temporary, self.root)
            fsync_directory(self.spool_parent)
            self.head = initial_head
        except OSError as error:
            fail(f"cannot initialize campaign spool: {error}")
        finally:
            if temporary.exists():
                shutil.rmtree(temporary)

    def _open_existing(self) -> None:
        require(self.root.is_dir() and not self.root.is_symlink(), "campaign spool root is invalid")
        self._require_internal_directories()
        observed_identity, _ = read_canonical_json(
            self.root / "identity.json", "campaign spool identity"
        )
        validate_identity(observed_identity)
        require(observed_identity == self.identity, "campaign spool identity differs")
        head, _ = read_canonical_json(self.root / "HEAD.json", "campaign spool HEAD")
        self.head = self._validate_head(head)

    def _require_internal_directories(self) -> None:
        require(
            self.root.is_dir() and not self.root.is_symlink(),
            "campaign spool root is invalid",
        )
        # ``artifacts`` was added by the v4 scientific harness.  It is not a
        # structural prerequisite here so pre-existing v1--v3 spools remain
        # readable; the importing v4 path creates and validates it below.
        for name in ("requests", "receipts", "checkpoints", "attempts"):
            directory = self.root / name
            require(
                directory.is_dir() and not directory.is_symlink(),
                f"campaign spool {name} directory is invalid",
            )

    def physical_root(self) -> Path:
        """Return the existing canonical absolute spool root."""

        self._require_open()
        self._require_internal_directories()
        try:
            physical = self.root.resolve(strict=True)
        except OSError as error:
            fail(f"cannot resolve campaign spool root: {error}")
        require(
            physical.is_absolute() and physical.is_dir() and not physical.is_symlink(),
            "campaign spool physical root is invalid",
        )
        return physical

    def prepare_attempt_directory(self, ordinal: int, request_sha256: str) -> Path:
        """Create or reopen one stable physical artifact staging directory."""

        self._require_open()
        self._require_internal_directories()
        require(
            ordinal == self.next_ordinal,
            "attempt ordinal is not the next campaign run",
        )
        digest = exact_sha256(request_sha256, "attempt request SHA-256")
        parent = self.physical_root() / "attempts"
        path = parent / f"{ordinal:05d}-{digest}"
        if path.exists() or path.is_symlink():
            require(path.is_dir() and not path.is_symlink(), "attempt directory is invalid")
        else:
            try:
                path.mkdir(mode=0o700)
            except OSError as error:
                fail(f"cannot create attempt directory: {error}")
            fsync_directory(parent)
        try:
            physical = path.resolve(strict=True)
        except OSError as error:
            fail(f"cannot resolve attempt directory: {error}")
        require(
            physical.parent == parent and physical.is_dir() and not physical.is_symlink(),
            "attempt directory escapes the campaign spool",
        )
        return physical

    def import_regular_file(
        self,
        *,
        source_root: Path,
        source_file: str,
        destination_file: str,
        expected_size_bytes: int,
        expected_sha256: str,
        label: str,
    ) -> str:
        """Stream, authenticate, fsync and immutably publish one staged artifact."""

        self._require_open()
        self._require_internal_directories()
        source_relative = Path(safe_relative_file(source_file, f"{label} source path"))
        destination_relative = safe_relative_file(
            destination_file, f"{label} destination path"
        )
        size = natural(expected_size_bytes, f"{label} size", positive=True)
        expected_digest = exact_sha256(expected_sha256, f"{label} SHA-256")
        try:
            physical_source_root = source_root.resolve(strict=True)
        except OSError as error:
            fail(f"cannot resolve {label} source root: {error}")
        require(
            source_root.is_absolute()
            and source_root == physical_source_root
            and source_root.is_dir()
            and not source_root.is_symlink(),
            f"{label} source root is not a physical directory",
        )
        current = source_root
        for component in source_relative.parts[:-1]:
            current /= component
            require(
                current.is_dir() and not current.is_symlink(),
                f"{label} source parent is not a physical directory",
            )
        source = source_root / source_relative
        spool_root = self.physical_root()
        artifact_parent = spool_root / "artifacts"
        if artifact_parent.exists() or artifact_parent.is_symlink():
            require(
                artifact_parent.is_dir() and not artifact_parent.is_symlink(),
                "campaign artifact archive is invalid",
            )
        else:
            try:
                artifact_parent.mkdir(mode=0o700)
            except OSError as error:
                fail(f"cannot create campaign artifact archive: {error}")
            fsync_directory(spool_root)
        destination = spool_root / destination_relative
        require(
            destination.parent == artifact_parent,
            f"{label} destination is outside the artifact archive",
        )
        source_descriptor = -1
        temporary: Path | None = None
        try:
            source_descriptor = os.open(
                source,
                os.O_RDONLY
                | getattr(os, "O_NOFOLLOW", 0)
                | getattr(os, "O_NONBLOCK", 0),
            )
            source_stat = os.fstat(source_descriptor)
            require(stat.S_ISREG(source_stat.st_mode), f"{label} source is not regular")
            require(source_stat.st_size == size, f"{label} source size differs")
            destination_descriptor, temporary_name = tempfile.mkstemp(
                dir=destination.parent,
                prefix=f".{destination.name}.",
                suffix=".tmp",
            )
            temporary = Path(temporary_name)
            digest = hashlib.sha256()
            copied = 0
            with os.fdopen(source_descriptor, "rb", closefd=True) as source_stream:
                source_descriptor = -1
                with os.fdopen(destination_descriptor, "wb") as destination_stream:
                    for block in iter(lambda: source_stream.read(1024 * 1024), b""):
                        destination_stream.write(block)
                        digest.update(block)
                        copied += len(block)
                    destination_stream.flush()
                    os.fsync(destination_stream.fileno())
            require(copied == size, f"{label} copied size differs")
            require(digest.hexdigest() == expected_digest, f"{label} SHA-256 differs")
            try:
                os.link(temporary, destination)
            except FileExistsError:
                require_regular_file(destination, f"existing {label}")
                require(
                    destination.stat().st_size == size,
                    f"existing {label} size differs",
                )
                require(
                    sha256_file(destination, f"existing {label}") == expected_digest,
                    f"existing {label} SHA-256 differs",
                )
            fsync_directory(destination.parent)
        except OSError as error:
            fail(f"cannot import {label}: {error}")
        finally:
            if source_descriptor >= 0:
                os.close(source_descriptor)
            if temporary is not None:
                temporary.unlink(missing_ok=True)
        return destination_relative

    def _validate_head(self, value: Any) -> dict[str, Any]:
        self._require_internal_directories()
        require(isinstance(value, dict), "campaign spool HEAD must be an object")
        require(
            set(value)
            == {
                "active_run",
                "campaign_id",
                "commits",
                "next_ordinal",
                "ordered_receipt_chain_sha256",
                "schema",
                "sequence_number",
            },
            "campaign spool HEAD fields differ",
        )
        require(value["schema"] == HEAD_SCHEMA, "campaign spool HEAD schema differs")
        require(value["campaign_id"] == self.identity["campaign_id"], "campaign spool HEAD identity differs")
        sequence = natural(value["sequence_number"], "campaign spool sequence")
        next_ordinal = natural(value["next_ordinal"], "campaign spool next ordinal")
        commits = value["commits"]
        require(isinstance(commits, list), "campaign spool commits must be an array")
        require(len(commits) == next_ordinal, "campaign spool commit prefix is not contiguous")
        chain = INITIAL_RECEIPT_CHAIN_SHA256
        for ordinal, commit_value in enumerate(commits):
            commit = self._validate_commit(commit_value, ordinal)
            request = self._validate_request_reference(
                request_file=commit["request_file"],
                request_sha256=commit["request_sha256"],
                run_id=commit["run_id"],
                ordinal=ordinal,
            )
            artifact_path = self.root / commit["receipt_file"]
            artifact, payload = read_canonical_json(
                artifact_path, f"campaign receipt {ordinal}"
            )
            require(
                sha256_bytes(payload) == commit["receipt_sha256"],
                f"campaign receipt {ordinal} digest differs",
            )
            require(
                artifact.get("run_id") == commit["run_id"]
                and artifact.get("request_sha256") == commit["request_sha256"]
                and artifact.get("role") == commit["role"]
                and artifact.get("status") == commit["status"],
                f"campaign receipt {ordinal} identity differs",
            )
            require(
                artifact.get("run_id") == request.get("run_id"),
                f"campaign receipt {ordinal} is not bound to its request file",
            )
            require(
                commit["role"] == request.get("role"),
                f"campaign receipt {ordinal} role differs from its request file",
            )
            chain = extend_receipt_chain(chain, ordinal, commit["receipt_sha256"])
            require(
                commit["receipt_chain_sha256"] == chain,
                f"campaign receipt {ordinal} breaks the ordered chain",
            )
        require(
            value["ordered_receipt_chain_sha256"] == chain,
            "campaign spool HEAD receipt chain differs",
        )
        active = value["active_run"]
        if active is not None:
            self._validate_active_run(active, next_ordinal)
        require(
            sequence >= len(commits) + int(active is not None),
            "campaign spool sequence is behind its durable state",
        )
        return value

    def _validate_commit(self, value: Any, ordinal: int) -> dict[str, Any]:
        require(isinstance(value, dict), f"campaign commit {ordinal} must be an object")
        require(
            set(value)
            == {
                "ordinal",
                "receipt_chain_sha256",
                "receipt_file",
                "receipt_sha256",
                "request_file",
                "request_sha256",
                "role",
                "run_id",
                "schema",
                "status",
            },
            f"campaign commit {ordinal} fields differ",
        )
        require(value["schema"] == COMMIT_SCHEMA, f"campaign commit {ordinal} schema differs")
        require(
            natural(value["ordinal"], f"campaign commit {ordinal} ordinal")
            == ordinal,
            f"campaign commit {ordinal} ordinal differs",
        )
        for field in ("run_id", "role", "status"):
            require(isinstance(value[field], str) and value[field], f"campaign commit {ordinal}.{field} is invalid")
        exact_sha256(value["request_sha256"], f"campaign commit {ordinal} request SHA-256")
        exact_sha256(value["receipt_sha256"], f"campaign commit {ordinal} receipt SHA-256")
        exact_sha256(value["receipt_chain_sha256"], f"campaign commit {ordinal} chain SHA-256")
        receipt_file = safe_relative_file(value["receipt_file"], f"campaign commit {ordinal} receipt path")
        require(
            receipt_file
            == f"receipts/{ordinal:05d}-{value['receipt_sha256']}.json",
            f"campaign commit {ordinal} receipt path does not match its identity",
        )
        safe_relative_file(value["request_file"], f"campaign commit {ordinal} request path")
        return value

    def _validate_request_reference(
        self,
        *,
        request_file: Any,
        request_sha256: Any,
        run_id: Any,
        ordinal: int,
    ) -> dict[str, Any]:
        digest = exact_sha256(
            request_sha256, f"campaign request {ordinal} SHA-256"
        )
        relative = safe_relative_file(
            request_file, f"campaign request {ordinal} path"
        )
        require(
            relative == f"requests/{ordinal:05d}-{digest}.json",
            f"campaign request {ordinal} path does not match its identity",
        )
        request, payload = read_canonical_json(
            self.root / relative, f"campaign request {ordinal}"
        )
        require(
            sha256_bytes(payload) == digest,
            f"campaign request {ordinal} digest differs",
        )
        require(
            request.get("run_id") == run_id,
            f"campaign request {ordinal} run identity differs",
        )
        return request

    def _validate_active_run(self, value: Any, next_ordinal: int) -> dict[str, Any]:
        require(isinstance(value, dict), "active campaign run must be an object")
        require(
            set(value)
            == {
                "checkpoint_file",
                "checkpoint_sha256",
                "ordinal",
                "request_file",
                "request_sha256",
                "run_id",
                "schema",
            },
            "active campaign run fields differ",
        )
        require(value["schema"] == ACTIVE_RUN_SCHEMA, "active campaign run schema differs")
        require(
            natural(value["ordinal"], "active campaign run ordinal")
            == next_ordinal,
            "active campaign run ordinal differs",
        )
        require(isinstance(value["run_id"], str) and value["run_id"], "active campaign run ID is invalid")
        exact_sha256(value["request_sha256"], "active campaign request SHA-256")
        exact_sha256(value["checkpoint_sha256"], "active checkpoint SHA-256")
        self._validate_request_reference(
            request_file=value["request_file"],
            request_sha256=value["request_sha256"],
            run_id=value["run_id"],
            ordinal=next_ordinal,
        )
        relative = safe_relative_file(value["checkpoint_file"], "active checkpoint path")
        require(
            relative
            == f"checkpoints/{next_ordinal:05d}-{value['checkpoint_sha256']}.json",
            "active checkpoint path does not match its identity",
        )
        _, payload = read_canonical_json(self.root / relative, "active checkpoint")
        require(sha256_bytes(payload) == value["checkpoint_sha256"], "active checkpoint digest differs")
        return value

    def _publish_head(self, value: dict[str, Any]) -> None:
        validated = self._validate_head(value)
        atomic_publish(self.root / "HEAD.json", canonical_bytes(validated))
        self.head = validated

    def publish_request(self, ordinal: int, request: dict[str, Any]) -> tuple[str, str]:
        self._require_open()
        self._require_internal_directories()
        require(ordinal == self.next_ordinal, "request ordinal is not the next campaign run")
        payload = canonical_bytes(request)
        digest = sha256_bytes(payload)
        relative = f"requests/{ordinal:05d}-{digest}.json"
        path = self.root / relative
        if path.exists() or path.is_symlink():
            _, existing = read_canonical_json(path, "existing campaign request")
            require(existing == payload, "existing campaign request differs")
        else:
            atomic_publish(path, payload)
        return relative, digest

    def commit_checkpoint(
        self,
        *,
        ordinal: int,
        run_id: str,
        request_file: str,
        request_sha256: str,
        checkpoint: dict[str, Any],
    ) -> dict[str, Any]:
        self._require_open()
        self._require_internal_directories()
        assert self.head is not None
        require(ordinal == self.next_ordinal, "checkpoint ordinal is not the active campaign run")
        exact_sha256(request_sha256, "checkpoint request SHA-256")
        require(isinstance(run_id, str) and run_id, "checkpoint run ID is invalid")
        self._validate_request_reference(
            request_file=request_file,
            request_sha256=request_sha256,
            run_id=run_id,
            ordinal=ordinal,
        )
        active = self.head["active_run"]
        if active is not None:
            require(
                active["ordinal"] == ordinal
                and active["run_id"] == run_id
                and active["request_file"] == request_file
                and active["request_sha256"] == request_sha256,
                "checkpoint does not continue the active campaign run",
            )
        payload = canonical_bytes(checkpoint)
        digest = sha256_bytes(payload)
        relative = f"checkpoints/{ordinal:05d}-{digest}.json"
        atomic_publish(self.root / relative, payload)
        maybe_fail_after("checkpoint_artifact")
        updated = dict(self.head)
        updated["active_run"] = {
            "checkpoint_file": relative,
            "checkpoint_sha256": digest,
            "ordinal": ordinal,
            "request_file": request_file,
            "request_sha256": request_sha256,
            "run_id": run_id,
            "schema": ACTIVE_RUN_SCHEMA,
        }
        updated["sequence_number"] += 1
        self._publish_head(updated)
        maybe_fail_after("checkpoint_head")
        return updated["active_run"]

    def commit_receipt(
        self,
        *,
        ordinal: int,
        request_file: str,
        request_sha256: str,
        receipt: dict[str, Any],
    ) -> dict[str, Any]:
        self._require_open()
        self._require_internal_directories()
        assert self.head is not None
        require(ordinal == self.next_ordinal, "receipt ordinal is not the next campaign run")
        exact_sha256(request_sha256, "receipt request SHA-256")
        run_id = receipt.get("run_id")
        role = receipt.get("role")
        status = receipt.get("status")
        require(isinstance(run_id, str) and run_id, "receipt run ID is invalid")
        require(isinstance(role, str) and role, "receipt role is invalid")
        require(isinstance(status, str) and status, "receipt status is invalid")
        self._validate_request_reference(
            request_file=request_file,
            request_sha256=request_sha256,
            run_id=run_id,
            ordinal=ordinal,
        )
        require(
            receipt.get("request_sha256") == request_sha256,
            "receipt is not bound to its request",
        )
        active = self.head["active_run"]
        if active is not None:
            require(
                active["ordinal"] == ordinal
                and active["run_id"] == run_id
                and active["request_file"] == request_file
                and active["request_sha256"] == request_sha256,
                "receipt does not close the active campaign run",
            )
        payload = canonical_bytes(receipt)
        receipt_digest = sha256_bytes(payload)
        relative = f"receipts/{ordinal:05d}-{receipt_digest}.json"
        atomic_publish(self.root / relative, payload)
        maybe_fail_after("receipt_artifact")
        chain = extend_receipt_chain(
            self.head["ordered_receipt_chain_sha256"], ordinal, receipt_digest
        )
        commit = {
            "ordinal": ordinal,
            "receipt_chain_sha256": chain,
            "receipt_file": relative,
            "receipt_sha256": receipt_digest,
            "request_file": request_file,
            "request_sha256": request_sha256,
            "role": role,
            "run_id": run_id,
            "schema": COMMIT_SCHEMA,
            "status": status,
        }
        updated = dict(self.head)
        updated["active_run"] = None
        updated["commits"] = [*self.head["commits"], commit]
        updated["next_ordinal"] = ordinal + 1
        updated["ordered_receipt_chain_sha256"] = chain
        updated["sequence_number"] += 1
        self._publish_head(updated)
        maybe_fail_after("receipt_head")
        return commit


@dataclass(frozen=True)
class GuardedProcessResult:
    command: tuple[str, ...]
    returncode: int
    stdout: bytes
    stderr: bytes
    timed_out: bool
    term_sent: bool
    kill_sent: bool


def _signal_process_group(process: subprocess.Popen[bytes], signum: int) -> bool:
    try:
        os.killpg(process.pid, signum)
        return True
    except ProcessLookupError:
        return False


def run_process_group(
    command: Sequence[str],
    *,
    input_bytes: bytes | None,
    timeout_seconds: float,
    terminate_grace_seconds: float = 1.0,
    cwd: Path | None = None,
) -> GuardedProcessResult:
    require(command and all(isinstance(item, str) and item for item in command), "process command is invalid")
    require(timeout_seconds > 0.0, "process timeout must be positive")
    require(terminate_grace_seconds > 0.0, "process termination grace must be positive")
    try:
        process = subprocess.Popen(
            list(command),
            stdin=subprocess.PIPE if input_bytes is not None else subprocess.DEVNULL,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            start_new_session=True,
            cwd=cwd,
        )
    except OSError as error:
        fail(f"cannot start guarded process: {error}")
    stdout = b""
    stderr = b""
    timed_out = False
    term_sent = False
    kill_sent = False
    try:
        try:
            stdout, stderr = process.communicate(
                input=input_bytes, timeout=timeout_seconds
            )
        except subprocess.TimeoutExpired:
            timed_out = True
            term_sent = _signal_process_group(process, signal.SIGTERM)
            try:
                stdout, stderr = process.communicate(
                    timeout=terminate_grace_seconds
                )
            except subprocess.TimeoutExpired:
                pass
            # The direct child may exit on SIGTERM while a descendant that
            # closed the inherited pipes remains in the process group.  Never
            # use the direct child's return status as proof that the group is
            # empty: issue SIGKILL to the saved group ID after every timeout.
            kill_sent = _signal_process_group(process, signal.SIGKILL)
            stdout, stderr = process.communicate()
            process.wait()
    except BaseException:
        _signal_process_group(process, signal.SIGKILL)
        process.wait()
        raise
    finally:
        if process.poll() is None:
            kill_sent = _signal_process_group(process, signal.SIGKILL) or kill_sent
            process.wait()
    return GuardedProcessResult(
        command=tuple(command),
        returncode=process.returncode,
        stdout=stdout,
        stderr=stderr,
        timed_out=timed_out,
        term_sent=term_sent,
        kill_sent=kill_sent,
    )


def run_canonical_json_process(
    command: Sequence[str],
    *,
    request: dict[str, Any] | None,
    timeout_seconds: float,
    label: str,
    cwd: Path | None = None,
) -> dict[str, Any]:
    result = run_process_group(
        command,
        input_bytes=None if request is None else canonical_bytes(request),
        timeout_seconds=timeout_seconds,
        cwd=cwd,
    )
    require(not result.timed_out, f"{label} reached its process-group timeout")
    require(result.returncode == 0, f"{label} failed with status {result.returncode}")
    require(not result.stderr, f"{label} emitted stderr")
    try:
        raw = result.stdout.decode("ascii")
    except UnicodeDecodeError as error:
        fail(f"{label} did not emit ASCII: {error}")
    value = parse_json_text(raw, label, canonical_line=True)
    require(isinstance(value, dict), f"{label} must emit one object")
    return value
