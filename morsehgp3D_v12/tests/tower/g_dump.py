#!/usr/bin/env python3
"""Lecteur de l'export de TEST de l'etage G (bench/tower_export.hpp) : res.bin au format MHGP12DP version 1, genre 4
<< resolution >>, et cat.bin (genre 1, lecteur tests/catalogue/catalogue_dump.py). Par ordre k : naissances (cles,
rangs), cellules (boules, rangs, drapeaux, decalages des traces), traces (masques de A dans U), cibles, compteurs.
Un fichier tronque, une section absente ou de taille inattendue, des octets en trop sont refuses (Refusal).
Python 3.10 nu, aucun assert.
"""
import os
import struct
import sys
from array import array

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'catalogue'))
import catalogue_dump  # noqa: E402  (lecteur MHGP12DP du catalogue)

Refusal = catalogue_dump.Refusal
CELL_BIT = 0x80000000
INDEX_MASK = 0x7FFFFFFF
INERT, EXTENDED = 1, 2
COUNTERS = ('births', 'cells', 'inert_cells', 'extended_cells', 'representatives', 'probes', 'first_probe_hits',
            'probe_hits_after_steps', 'route_t1', 'route_cert_table', 'route_cert_census', 'route_fallback_table',
            'route_fallback_census', 'fallback_no_proposal', 'fallback_not_in_part', 'fallback_certificate',
            'census_saturated', 'census_complete', 'census_sites', 'census_sites_max', 'census_nodes',
            'jumps_catalogue', 'jumps_census', 'inert_steps', 'cell_stops', 'birth_stops', 'controls', 'max_chain')
CHAIN_BINS = 16
COLUMNS = (('BKEY', 4, 'I'), ('BRNK', 4, 'I'), ('CBAL', 4, 'I'), ('CRNK', 4, 'I'), ('CFLG', 1, 'B'),
           ('COFF', 8, 'Q'), ('TMSK', 8, 'Q'), ('TARG', 4, 'I'), ('CNTR', 8, 'Q'))


def words(blob, code):
    values = array(code)
    values.frombytes(blob)
    if sys.byteorder != 'little':
        values.byteswap()
    return values


class Order(object):
    """Sorties de l'ordre k (tableaux du module array)."""

    def __init__(self, k, sections):
        self.k = k
        self.birth_keys, self.birth_ranks = sections['BKEY'], sections['BRNK']
        self.cell_balls, self.cell_ranks = sections['CBAL'], sections['CRNK']
        self.cell_flags, self.cell_offsets = sections['CFLG'], sections['COFF']
        self.masks, self.targets = sections['TMSK'], sections['TARG']
        values = sections['CNTR']
        if len(values) != len(COUNTERS) + CHAIN_BINS:
            raise Refusal('ordre %d : %d compteurs' % (k, len(values)))
        self.counters = dict(zip(COUNTERS, values[:len(COUNTERS)]))
        self.chains = list(values[len(COUNTERS):])
        if (len(self.cell_offsets) != len(self.cell_balls) + 1 or self.cell_offsets[-1] != len(self.masks) or
                len(self.targets) != len(self.masks) or len(self.birth_ranks) != len(self.birth_keys)):
            raise Refusal('ordre %d : colonnes incoherentes' % k)

    def traces(self, cell):
        return range(self.cell_offsets[cell], self.cell_offsets[cell + 1])


def read_resolution(path):
    try:
        with open(path, 'rb') as handle:
            data = handle.read()
    except OSError as error:
        raise Refusal('illisible %s : %s' % (path, error))
    if len(data) < 64 or data[:8] != b'MHGP12DP':
        raise Refusal('magie absente : %s' % path)
    version, kind, bits, kmax, orders, sections, sites = struct.unpack_from('<6IQ', data, 8)
    if version != 1 or kind != 4 or sections != len(COLUMNS) * orders:
        raise Refusal('en-tete hors format (version %d, genre %d) : %s' % (version, kind, path))
    tags = {}
    at = 64
    for _ in range(sections):
        if at + 24 > len(data):
            raise Refusal('section tronquee : %s' % path)
        tag = data[at:at + 8].rstrip(b'\0').decode('ascii', 'replace')
        size, _reserved, count = struct.unpack_from('<IIQ', data, at + 8)
        at += 24
        if at + size * count > len(data):
            raise Refusal('donnees tronquees (%s) : %s' % (tag, path))
        tags[tag] = (size, data[at:at + size * count])
        at += size * count + (-(size * count)) % 8
    if at != len(data):
        raise Refusal('octets en trop : %s' % path)
    out = {'bits': bits, 'kmax': kmax, 'sites': sites, 'orders': []}
    for k in range(1, orders + 1):
        columns = {}
        for name, size, code in COLUMNS:
            tag = '%s_%02d' % (name, k)
            if tag not in tags or tags[tag][0] != size:
                raise Refusal('section %s absente ou de taille inattendue : %s' % (tag, path))
            columns[name] = words(tags[tag][1], code)
        out['orders'].append(Order(k, columns))
    return out


def read_catalogue(path):
    return catalogue_dump.Catalogue(catalogue_dump.read_dump(path))


def bits_of(mask):
    out, j = [], 0
    while mask:
        if mask & 1:
            out.append(j)
        mask >>= 1
        j += 1
    return out
