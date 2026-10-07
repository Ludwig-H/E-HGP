"""Fichier distant en lecture seule par requetes HTTP Range (bibliotheque standard) : permet d'ouvrir une archive zip
distante avec `zipfile` et d'en extraire un membre sans telecharger l'archive entiere. Refuse un serveur qui ignore
Range (reponse autre que 206)."""
from __future__ import annotations

import io
import time
import urllib.error
import urllib.request

USER_AGENT = 'mhgp12-data-prep/1 (research)'


class HttpRangeFile(io.RawIOBase):
    def __init__(self, url: str, block: int = 1 << 22, retries: int = 6):
        self.url, self.block, self.retries, self.pos = url, block, retries, 0
        request = urllib.request.Request(url, method='HEAD', headers={'User-Agent': USER_AGENT})
        with urllib.request.urlopen(request, timeout=60) as response:
            self.size = int(response.headers['Content-Length'])
            self.final_url = response.geturl()
        self.cache = {}
        self.fetched = 0

    def readable(self):
        return True

    def seekable(self):
        return True

    def tell(self):
        return self.pos

    def seek(self, offset, whence=0):
        self.pos = offset if whence == 0 else (self.pos + offset if whence == 1 else self.size + offset)
        return self.pos

    def _fetch(self, start: int, stop: int) -> bytes:
        for attempt in range(self.retries):
            try:
                request = urllib.request.Request(self.final_url, headers={'Range': 'bytes=%d-%d' % (start, stop - 1),
                                                                          'User-Agent': USER_AGENT})
                with urllib.request.urlopen(request, timeout=120) as response:
                    if response.status != 206:
                        raise IOError('HTTP %d au lieu de 206 : Range ignore' % response.status)
                    data = response.read()
                if len(data) != stop - start:
                    raise IOError('plage incomplete')
                self.fetched += len(data)
                return data
            except (urllib.error.URLError, TimeoutError, ConnectionError, OSError):
                if attempt + 1 == self.retries:
                    raise
                time.sleep(min(30, 2 ** attempt))
        raise IOError('inaccessible')

    def readinto(self, buffer) -> int:
        if self.pos >= self.size:
            return 0
        want = min(len(buffer), self.size - self.pos)
        out = bytearray()
        while len(out) < want:
            index = (self.pos + len(out)) // self.block
            if index not in self.cache:
                start = index * self.block
                self.cache = {index: self._fetch(start, min(start + self.block, self.size))}
            chunk = self.cache[index]
            offset = self.pos + len(out) - index * self.block
            out += chunk[offset:offset + want - len(out)]
        buffer[:want] = out
        self.pos += want
        return want
