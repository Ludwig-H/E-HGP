#!/usr/bin/env python3
"""FOR-instance (forets survolees par drone, ULS RIEGL ; CC BY 4.0) -> format v11 a 1 mm (famille b).

    python3 -I prepare_forinstance.py --raw DIR --out DIR [--plots NOM ...]

Archive Zenodo `FORinstance_dataset.zip` (1 643 251 814 octets, enregistrement 8287792, telechargement direct sans
compte). Seuls les membres voulus sont extraits, par requetes HTTP Range (v12data/httprange.py + zipfile, CRC32 du
zip verifie a l'extraction), puis epingles par SHA-256 du LAS extrait. LAS 1.2 NON compresse, echelle 0,001 : lu en
numpy pur (v12data/formats/las.py), donc toute la chaine tourne en bibliotheque standard + numpy (VM G4 comprise).
Classes du producteur (readMe.txt) : 0 non classe, 1 vegetation basse, 2 terrain, 3 points hors placette, 4 tronc,
5 branches vivantes, 6 branches mortes. Variantes : `tout` et `sans_sol` (classe 2 retiree). Doublons au mm :
`<nom>` brut + `<nom>.distinct` (D8).
"""
from __future__ import annotations

import argparse
import json
import shutil
import sys
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from v12data import common, lasconv  # noqa: E402
from v12data.httprange import HttpRangeFile  # noqa: E402

URL = 'https://zenodo.org/api/records/8287792/files/FORinstance_dataset.zip/content'
ZIP_SIZE = 1643251814
DATASET = dict(name='FOR-instance (Puliti et al. 2023), placettes forestieres annotees',
               sensor='ULS, drone (scanners RIEGL ; collections CULS, NIBIO, RMIT, SCION, TUWIEN)',
               licence='CC BY 4.0', licence_url='https://zenodo.org/records/8287792',
               access='Zenodo, telechargement direct sans compte ; requetes Range acceptees',
               density='tres forte (de l\'ordre de 1 000 a 10 000 points par m2 de placette)')
CLASSES = {0: 'unclassified', 1: 'low vegetation', 2: 'terrain', 3: 'out-points', 4: 'stem', 5: 'live branches',
           6: 'woody branches'}  # readMe.txt de l'archive
PLOTS = {  # nom : (membre, octets du LAS extrait, sha256 du LAS extrait, epingle le 7 oct. 2026)
    'forinst_scion_plot61': ('SCION/plot_61_annotated.las', 122035371,
                             '13721d7121db86fe1bdcc010ea64c4013950f6a8cc587473a2529ae7340e42d1'),
    'forinst_tuwien_train': ('TUWIEN/train.las', 317891921,
                             '9395980682dd05af61baae5c7380307e68f11eae99a18b45bbb0aa71cbed6ca9'),
    'forinst_nibio_plot12': ('NIBIO/plot_12_annotated.las', 283183251,
                             '64f03731a41c595163c026f7c1e5c6a8c21b9eb6130c77e03bec740ae9d3e4a9'),
}


def extract(member: str, size: int, dest: Path, digest) -> dict:
    if dest.is_file() and dest.stat().st_size == size and digest is not None and common.sha256_file(dest) == digest:
        return dict(member=member, bytes=size, sha256=digest, downloaded=False, verified=True)
    remote = HttpRangeFile(URL)
    if remote.size != ZIP_SIZE:
        raise RuntimeError('taille de l\'archive inattendue : %d' % remote.size)
    dest.parent.mkdir(parents=True, exist_ok=True)
    part = Path(str(dest) + '.part')
    with zipfile.ZipFile(remote) as archive:
        info = archive.getinfo(member)
        if info.file_size != size:
            raise RuntimeError('taille du membre %s inattendue : %d' % (member, info.file_size))
        with archive.open(info) as source, open(part, 'wb') as out:  # zipfile verifie le CRC32 en fin de lecture
            shutil.copyfileobj(source, out, 1 << 22)
    got = common.sha256_file(part)
    if digest is not None and got != digest:
        raise RuntimeError('empreinte inattendue pour %s : %s' % (member, got))
    part.replace(dest)
    return dict(member=member, bytes=size, sha256=got, downloaded=True, verified=digest is not None,
                range_bytes_fetched=remote.fetched)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--raw', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--plots', nargs='*', default=list(PLOTS))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    common.selftest_quantize()
    entries = []
    for name in args.plots:
        member, size, digest = PLOTS[name]
        dest = args.raw / member
        got = extract(member, size, dest, digest)
        common.log('%s : %s (%d octets, sha256 %s)' % (member, 'extrait' if got['downloaded'] else 'present',
                                                      got['bytes'], got['sha256']))
        source = dict(url=URL, archive_bytes=ZIP_SIZE, member=got,
                      pin='sha256 of the extracted LAS pinned on first extraction (7 Oct 2026)' if digest else
                      'NOT PINNED (first use)')
        for m in lasconv.convert(dest, args.out, name, DATASET, source, __file__, variants=('tout', 'sans_sol'),
                                 class_names=CLASSES):
            entries.append(lasconv.set_entry(m))
    manifest_path = args.out / 'manifest.json'
    old = json.loads(manifest_path.read_text()) if manifest_path.is_file() else {}
    cases = {c['name']: c for c in old.get('cases', [])}
    cases.update({e['name']: e for e in entries})
    out = dict(schema=common.SCHEMA_SET, family='multi_millions', dataset=DATASET,
               cases=[cases[k] for k in sorted(cases)], generator=common.generator_info(__file__),
               created_utc=common.utc_now(), public_status='not_claimed')
    if old.get('crops'):
        out['crops'] = old['crops']
    common.write_json(manifest_path, out)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
