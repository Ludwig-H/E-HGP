#!/usr/bin/env python3
"""Prepare the local institutional graphics; no network access is required."""
from pathlib import Path
import base64
import hashlib
import sys

ROOT = Path(__file__).resolve().parent

def main() -> None:
    try:
        import cairosvg
    except ImportError as exc:
        raise SystemExit('Install CairoSVG: python3 -m pip install cairosvg') from exc
    for source, target, width in [
        ('theme/imgs/Inria-logo-rouge.svg', 'theme/imgs/Inria-logo-rouge.png', 600),
        ('theme/imgs/Filet-7pt.svg', 'theme/imgs/Filet-7pt.png', 600),
        ('theme/imgs/coin_degrade.svg', 'theme/imgs/angle.png', 150),
    ]:
        cairosvg.svg2png(url=str(ROOT / source), write_to=str(ROOT / target), output_width=width)
    payload = ''.join((ROOT / 'assets/szte.png.b64').read_text(encoding='ascii').split())
    graphic = base64.b64decode(payload, validate=True)
    if not graphic.startswith(b'\x89PNG\r\n\x1a\n'):
        raise SystemExit('Invalid SZTE PNG payload')
    (ROOT / 'assets/szte.png').write_bytes(graphic)
    print('Institutional graphics prepared locally.')

if __name__ == '__main__':
    main()
