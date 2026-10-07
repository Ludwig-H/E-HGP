"""PLY (ascii, binary_little_endian, binary_big_endian) en numpy pur : lit l'element `vertex` et ses proprietes
scalaires (les listes ne sont admises que dans les elements qui suivent `vertex`)."""
from __future__ import annotations

from pathlib import Path

import numpy as np

_TYPES = {'char': 'i1', 'int8': 'i1', 'uchar': 'u1', 'uint8': 'u1', 'short': 'i2', 'int16': 'i2',
          'ushort': 'u2', 'uint16': 'u2', 'int': 'i4', 'int32': 'i4', 'uint': 'u4', 'uint32': 'u4',
          'float': 'f4', 'float32': 'f4', 'double': 'f8', 'float64': 'f8'}


def read_header(path: Path) -> dict:
    with open(path, 'rb') as handle:
        if handle.readline().strip() != b'ply':
            raise ValueError('%s : pas un PLY' % path)
        fmt, elements, comments = None, [], []
        while True:
            line = handle.readline()
            if not line:
                raise ValueError('en-tete PLY tronque')
            words = line.decode('ascii', 'replace').split()
            if not words:
                continue
            if words[0] == 'format':
                fmt = words[1]
            elif words[0] in ('comment', 'obj_info'):
                comments.append(' '.join(words[1:]))
            elif words[0] == 'element':
                elements.append(dict(name=words[1], count=int(words[2]), properties=[]))
            elif words[0] == 'property':
                if words[1] == 'list':
                    elements[-1]['properties'].append((words[4], 'list', words[2], words[3]))
                else:
                    elements[-1]['properties'].append((words[2], words[1]))
            elif words[0] == 'end_header':
                return dict(format=fmt, elements=elements, comments=comments, data_offset=handle.tell())


def read_vertices(path: Path) -> tuple[np.ndarray, dict]:
    header = read_header(path)
    if not header['elements'] or header['elements'][0]['name'] != 'vertex':
        raise ValueError('le premier element PLY doit etre vertex')
    vertex = header['elements'][0]
    if any(p[1] == 'list' for p in vertex['properties']):
        raise ValueError('liste dans vertex non prise en charge')
    fmt = header['format']
    if fmt == 'ascii':
        names = [p[0] for p in vertex['properties']]
        dtype = np.dtype([(p[0], _TYPES[p[1]]) for p in vertex['properties']])
        with open(path, 'rb') as handle:
            handle.seek(header['data_offset'])
            table = np.loadtxt(handle, dtype=np.float64, max_rows=vertex['count'], ndmin=2)
        out = np.empty(vertex['count'], dtype=dtype)
        for i, name in enumerate(names):
            out[name] = table[:, i]
        return out, header
    endian = '<' if fmt == 'binary_little_endian' else '>'
    if fmt not in ('binary_little_endian', 'binary_big_endian'):
        raise ValueError('format PLY inconnu : %s' % fmt)
    dtype = np.dtype([(p[0], endian + _TYPES[p[1]]) for p in vertex['properties']])
    data = np.fromfile(path, dtype=dtype, count=vertex['count'], offset=header['data_offset'])
    if len(data) != vertex['count']:
        raise ValueError('PLY tronque : %d sommets sur %d' % (len(data), vertex['count']))
    return data, header
