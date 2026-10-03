"""Campaign-owned reuse of validated summaries under the receipts' SHA256 collision assumption.

Every inspection hashes the entire current payload through a read-only mapping. Only a small JSON
summary is retained, never a payload, event, verdict or clock. A decoded ticket is not reusable until
its caller has checked the current native events against it and explicitly publishes it. Source
files and completed native artifacts belong to the campaign and must not be edited during inspection.
This is encoding-result reuse under a cryptographic assumption, not a new geometric certificate.
"""
from collections import OrderedDict
from dataclasses import asdict, dataclass
import hashlib
import json
import mmap
import os
from pathlib import Path
import time


SCHEMA = 'ehgp.v11.semantic_reuse.v1'
SUMMARY_LIMIT = 64 * 1024
BLOCK = 1 << 20


def need(value, message):
    if not value:
        raise ValueError(message)


def sha(value):
    return type(value) is str and len(value) == 64 and all(c in '0123456789abcdef' for c in value)


def decoder_digest(paths):
    """Hash explicit decoder dependencies, independent of their absolute checkout paths."""
    paths = sorted(map(Path, paths), key=lambda path: path.name)
    need(paths and len({path.name for path in paths}) == len(paths), 'decoder dependency inventory')
    digest = hashlib.sha256(b'ehgp.v11.decoder_sources.v1\0')
    for path in paths:
        name, content = path.name.encode('utf-8'), path.read_bytes()
        digest.update(len(name).to_bytes(8, 'little')); digest.update(name)
        digest.update(len(content).to_bytes(8, 'little')); digest.update(content)
    return digest.hexdigest()


@dataclass(frozen=True)
class Context:
    format: str
    decoder_version: str
    decoder_sha256: str
    coord_bits: int
    kmax: int
    count: int
    xyz_sha256: str
    ids_sha256: str

    def __post_init__(self):
        need(type(self.format) is str and 0 < len(self.format) <= 128 and
             type(self.decoder_version) is str and 0 < len(self.decoder_version) <= 256, 'decoder format/version')
        need(all(sha(value) for value in (self.decoder_sha256, self.xyz_sha256, self.ids_sha256)), 'context SHA256')
        need(type(self.coord_bits) is int and self.coord_bits in (18, 21, 24) and
             type(self.kmax) is int and 1 <= self.kmax <= 12 and
             type(self.count) is int and 0 < self.count < 2**32-1, 'context domain')


def identity(value):
    need(type(value) is list and value and all(type(v) in (str, int, bool) for v in value), 'attempt identity list')
    encoded = json.dumps(value, separators=(',', ':'), ensure_ascii=True, allow_nan=False)
    need(len(encoded) <= 4096, 'attempt identity too large')
    return encoded


def fingerprint(info):
    return info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns


class Ticket:
    """One inspection result; callers mutate independent copies, never the cached representation."""
    def __init__(self, owner, key, payload, evidence):
        self._owner, self._key, self._payload = owner, key, payload
        self._evidence = json.dumps(evidence, sort_keys=True, allow_nan=False)
        self._published = False

    @property
    def summary(self):
        return json.loads(self._payload)

    @property
    def evidence(self):
        return json.loads(self._evidence)


class SummaryCache:
    """Single-threaded, campaign-local FIFO; capacity bounds retained summaries, not geometric work."""
    def __init__(self, capacity=64):
        need(type(capacity) is int and 1 <= capacity <= 64, 'summary cache capacity')
        self.capacity, self._entries, self._owner = capacity, OrderedDict(), object()

    def __len__(self):
        return len(self._entries)

    def inspect(self, path, context, current_attempt, decode, *, expected_sha256=None, expected_bytes=None):
        need(type(context) is Context and callable(decode), 'summary inspection arguments')
        current = identity(current_attempt)
        need(expected_sha256 is None or sha(expected_sha256), 'expected raw SHA256')
        need(expected_bytes is None or type(expected_bytes) is int and expected_bytes > 0,
             'expected payload size')
        started = time.monotonic()
        with Path(path).open('rb') as source:
            before = os.fstat(source.fileno())
            need(before.st_size > 0, 'summary payload size')
            with mmap.mmap(source.fileno(), 0, access=mmap.ACCESS_READ) as data:
                digest = hashlib.sha256()
                for begin in range(0, len(data), BLOCK):
                    digest.update(data[begin:begin+BLOCK])
                raw, size = digest.hexdigest(), len(data)
                need(expected_sha256 is None or raw == expected_sha256, 'raw SHA256 changed before inspection')
                need(expected_bytes is None or size == expected_bytes, 'payload size changed before inspection')
                hash_seconds = time.monotonic()-started
                key = context, raw, size
                entry = self._entries.get(key)
                if entry is None:
                    started = time.monotonic()
                    value = decode(data)
                    need(type(value) is dict, 'decoded summary must be an object')
                    payload = json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True,
                                         allow_nan=False)
                    need(len(payload) <= SUMMARY_LIMIT, 'decoded summary exceeds bounded cache representation')
                    need(json.loads(payload) == value, 'decoded summary must preserve its JSON representation')
                    decode_seconds = time.monotonic()-started
                    origin, mode = current, 'decoded'
                else:
                    payload, origin = entry
                    decode_seconds, mode = 0.0, 'reused'
                need(fingerprint(before) == fingerprint(os.fstat(source.fileno())),
                     'payload modified during inspection')
        evidence = dict(schema=SCHEMA, mode=mode, context=asdict(context), current_attempt=json.loads(current),
                        source_attempt=json.loads(origin), raw_sha256=raw, bytes=size,
                        hash_wall_seconds=hash_seconds, decode_wall_seconds=decode_seconds)
        return Ticket(self._owner, key, payload, evidence)

    def publish(self, ticket):
        """Call only after all current-event/summary checks pass; a failure before here inserts nothing."""
        need(type(ticket) is Ticket and ticket._owner is self._owner and not ticket._published,
             'foreign or already published summary ticket')
        evidence = ticket.evidence
        if evidence['mode'] == 'decoded':
            existing = self._entries.get(ticket._key)
            need(existing is None or existing[0] == ticket._payload, 'different summary for an identical key')
            if existing is None:
                if len(self._entries) == self.capacity:
                    self._entries.popitem(last=False)
                self._entries[ticket._key] = ticket._payload, identity(evidence['current_attempt'])
        ticket._published = True
