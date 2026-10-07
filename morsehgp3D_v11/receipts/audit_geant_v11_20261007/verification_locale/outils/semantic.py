#!/usr/bin/env python3
"""Empreinte semantique d'un vidage MHGP11FUL1 par le lecteur du depot (bench/full_semantic.py), sans ecriture.

Usage : python3 -B -S semantic.py <dump> <bits> <kmax> <sites>
"""
import json
import sys
import time

sys.path.insert(0, '/workspaces/E-HGP/morsehgp3D_v11/bench')
import full_semantic  # noqa: E402


def main():
    path, bits, kmax, count = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), int(sys.argv[4])
    t0 = time.monotonic()
    out = full_semantic.inspect(path, bits, kmax, count)
    out['decode_s'] = round(time.monotonic() - t0, 1)
    out.pop('orders', None)
    print(json.dumps(out))


if __name__ == '__main__':
    main()
