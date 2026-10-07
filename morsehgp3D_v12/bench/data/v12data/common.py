"""Coeur commun : empreintes, telechargement verifie, grille de 1 mm exacte, translation, sites distincts,
ecriture au format v11 (`<nom>.u32le` = x, y, z en u32 petit-boutiste entrelaces ; `<nom>.ids.u32le` = un u32 par
point) et manifestes JSON.

Bibliotheque standard + numpy seulement ; compatible Python 3.10 (VM G4) et numpy >= 1.21.

Conventions (identiques pour tous les jeux) :
- grille h = 1 mm, arrondi q = floor(x / h + 1/2) calcule EXACTEMENT (jamais par un produit flottant arrondi) ;
- translation au minimum de chaque axe (u = q - min q), donc u >= 0 ; la translation (en mm, entier signe) est publiee ;
- bits par axe = bit_length(max u) ; bits necessaires = max sur les axes ; profil minimal u21 / u24 / u32 ;
- ordre canonique : lexicographique (x, puis y, puis z) des coordonnees ecrites, comme les entrees du contrat v11
  (ng00-ng02) ; le fichier de coordonnees ne depend donc que de l'ensemble des positions ; les identifiants
  (`.ids.u32le`) portent l'indice du retour dans la source ;
- doublons au millimetre : positions egales apres quantification. Variante brute (un site par retour, refusee par
  defaut par le moteur, decision D8) et variante `distinct` (une position par site, identifiant = plus petit indice
  source a cette position, multiplicites publiees dans `.mult.u32le`, et `.inverse.u32le` = rang du site de chaque
  retour, en ordre source). L'objet calcule sur la variante distincte est la tour des positions distinctes, pas la
  tour ponderee (D8).
"""
from __future__ import annotations

import datetime
import fractions
import hashlib
import json
import os
import platform
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path

import numpy as np

SCHEMA_SCENE = 'mhgp12.input_scene.v1'
SCHEMA_SET = 'mhgp12.benchmark_inputs.v1'
GRID_STEP = dict(numerator=1, denominator=1000)
ROUNDING = 'floor(x/h+1/2), h = 1 mm, exact (rationnel) depuis la valeur source'
PROFILES = ((21, 'u21'), (24, 'u24'), (32, 'u32'))
USER_AGENT = 'mhgp12-data-prep/1 (research; contact via repository owner)'


# ---------------------------------------------------------------------------------------------- empreintes, JSON
def sha256_file(path: Path, chunk: int = 1 << 22) -> str:
    digest = hashlib.sha256()
    with open(path, 'rb') as handle:
        while True:
            block = handle.read(chunk)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def sha256_array(array: np.ndarray) -> str:
    return hashlib.sha256(np.ascontiguousarray(array).tobytes()).hexdigest()


def write_json(path: Path, obj) -> None:
    tmp = Path(str(path) + '.tmp')
    tmp.write_text(json.dumps(obj, indent=1, sort_keys=True, ensure_ascii=False) + '\n', encoding='utf-8')
    os.replace(tmp, path)


def utc_now() -> str:
    return datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%SZ')


def generator_info(script: str) -> dict:
    here = Path(__file__).resolve()
    files = sorted(p for p in here.parent.rglob('*.py'))
    return dict(script=Path(script).name, script_sha256=sha256_file(Path(script)),
                library_sha256={str(p.relative_to(here.parent)): sha256_file(p) for p in files},
                python=platform.python_version(), numpy=np.__version__)


# ---------------------------------------------------------------------------------------------- telechargement
def download(url: str, dest: Path, expected_sha256: str | None = None, expected_size: int | None = None,
             retries: int = 8, timeout: int = 120, headers: dict | None = None) -> dict:
    """Telecharge `url` vers `dest` (reprise par Range), puis verifie taille et SHA-256.

    Si `dest` existe deja avec la bonne empreinte, rien n'est telecharge. Une empreinte inconnue (None) est
    calculee et publiee : c'est l'epingle a reporter dans le script (premier usage), jamais une verification.
    """
    dest = Path(dest)
    dest.parent.mkdir(parents=True, exist_ok=True)
    if dest.is_file() and expected_sha256 is not None and sha256_file(dest) == expected_sha256:
        return dict(url=url, file=dest.name, bytes=dest.stat().st_size, sha256=expected_sha256, downloaded=False,
                    verified=True)
    part = Path(str(dest) + '.part')
    hdrs = {'User-Agent': USER_AGENT}
    hdrs.update(headers or {})
    total = None
    for attempt in range(retries):
        have = part.stat().st_size if part.exists() else 0
        req_headers = dict(hdrs)
        if have:
            req_headers['Range'] = 'bytes=%d-' % have
        try:
            request = urllib.request.Request(url, headers=req_headers)
            with urllib.request.urlopen(request, timeout=timeout) as response:
                status = response.status
                if have and status != 206:
                    have = 0  # le serveur ignore Range : on repart de zero
                length = response.headers.get('Content-Length')
                if length is not None:
                    total = int(length) + (have if status == 206 else 0)
                with open(part, 'ab' if (have and status == 206) else 'wb') as out:
                    while True:
                        block = response.read(1 << 22)
                        if not block:
                            break
                        out.write(block)
            if total is None or part.stat().st_size == total:
                break
        except (urllib.error.URLError, TimeoutError, ConnectionError, OSError) as error:
            if attempt + 1 == retries:
                raise RuntimeError('telechargement impossible : %s (%s)' % (url, error)) from error
            time.sleep(min(60, 2 ** attempt))
    size = part.stat().st_size
    if expected_size is not None and size != expected_size:
        raise RuntimeError('taille inattendue pour %s : %d au lieu de %d' % (url, size, expected_size))
    digest = sha256_file(part)
    if expected_sha256 is not None and digest != expected_sha256:
        raise RuntimeError('empreinte inattendue pour %s : %s au lieu de %s' % (url, digest, expected_sha256))
    os.replace(part, dest)
    return dict(url=url, file=dest.name, bytes=size, sha256=digest, downloaded=True,
                verified=expected_sha256 is not None)


def verify_file(path: Path, expected_sha256: str | None) -> dict:
    digest = sha256_file(path)
    if expected_sha256 is not None and digest != expected_sha256:
        raise RuntimeError('empreinte inattendue pour %s : %s au lieu de %s' % (path, digest, expected_sha256))
    return dict(file=Path(path).name, bytes=Path(path).stat().st_size, sha256=digest,
                verified=expected_sha256 is not None)


# ---------------------------------------------------------------------------------------------- grille exacte
_LIMIT_METRES = float(2 ** 40)  # au-dela, refus (aucune donnee terrestre ne s'en approche)


def quantize_float_mm(values: np.ndarray) -> np.ndarray:
    """q = floor(1000 x + 1/2) EXACT pour des flottants binaires (float32 ou float64), en int64.

    x = M 2^E (M entier, |M| < 2^53). Si E >= 0 : q = 1000 M 2^E. Sinon, s = -E et
    1000 M 2^-s + 1/2 = (125 M + 2^(k-1)) / 2^k avec k = s - 3 : division entiere par plancher, sans debordement
    (|125 M| < 2^60). s <= 3 signifierait |x| >= 2^49 m : refuse. k >= 62 signifie |x| < 2^-12 m, donc q = 0.
    """
    v = np.asarray(values)
    if v.dtype == np.float32:
        v = v.astype(np.float64)  # exact
    elif v.dtype != np.float64:
        raise TypeError('quantize_float_mm attend float32 ou float64, pas %s' % v.dtype)
    shape = v.shape
    v = v.reshape(-1)
    if not np.all(np.isfinite(v)):
        raise ValueError('coordonnee non finie')
    if v.size and float(np.max(np.abs(v))) >= _LIMIT_METRES:
        raise ValueError('coordonnee hors domaine (|x| >= 2^40 m)')
    mant, expo = np.frexp(v)
    big_m = np.ldexp(mant, 53).astype(np.int64)  # exact : |mant| in [1/2, 1)
    e = expo.astype(np.int64) - 53
    out = np.zeros(v.size, dtype=np.int64)
    pos = e >= 0
    if np.any(pos):
        out[pos] = (big_m[pos] * 1000) << e[pos]
    s = -e
    if np.any((s >= 1) & (s <= 3) & (big_m != 0)):
        raise ValueError('coordonnee hors domaine (exposant)')
    mid = (s >= 4) & (s - 3 < 62)
    if np.any(mid):
        k = s[mid] - 3
        num = big_m[mid] * 125 + (np.int64(1) << (k - 1))
        out[mid] = np.floor_divide(num, np.int64(1) << k)
    # s - 3 >= 62 : |x| < 2^-12 m, q = 0 (deja nul)
    return out.reshape(shape)


def quantize_float_mm_reference(values) -> list:
    """Reference lente en arithmetique rationnelle (pour les auto-controles)."""
    out = []
    for x in np.asarray(values, dtype=np.float64).reshape(-1).tolist():
        f = fractions.Fraction(x) * 1000 + fractions.Fraction(1, 2)
        out.append(f.numerator // f.denominator)
    return out


def decimal_fraction(value: float) -> fractions.Fraction:
    """Interpretation decimale la plus courte d'un double (repr), ex. 0.01 -> 1/100 exactement."""
    return fractions.Fraction(repr(float(value)))


def quantize_scaled_int_mm(raw: np.ndarray, scale: float, offset: float) -> tuple[np.ndarray, dict]:
    """LAS : x = X * scale + offset, scale et offset lus comme decimaux (repr). q = floor(1000 x + 1/2) exact."""
    a = decimal_fraction(scale) * 1000
    c = decimal_fraction(offset) * 1000
    raw = np.asarray(raw, dtype=np.int64)
    info = dict(scale=repr(float(scale)), offset=repr(float(offset)), scale_mm=str(a), offset_mm=str(c),
                interpretation='decimal_repr')
    if a.denominator == 1 and c.denominator == 1:
        if abs(a.numerator) * (1 << 32) + abs(c.numerator) >= (1 << 62):
            raise ValueError('echelle LAS hors domaine int64')
        return raw * a.numerator + c.numerator, info
    # cas general : floor((2 X p d + 2 r q + q d) / (2 q d)), p/q = a, r/d = c
    p, q, r, d = a.numerator, a.denominator, c.numerator, c.denominator
    bound = 2 * abs(p) * d * (1 << 32) + 2 * abs(r) * q + q * d
    if bound >= (1 << 62):
        obj = raw.astype(object)
        num = obj * (2 * p * d) + (2 * r * q + q * d)
        res = np.array([int(v) // (2 * q * d) for v in num.tolist()], dtype=np.int64)
        info['path'] = 'python_int'
        return res, info
    num = raw * (2 * p * d) + (2 * r * q + q * d)
    info['path'] = 'int64'
    return np.floor_divide(num, 2 * q * d), info


def selftest_quantize(seed: int = 12) -> dict:
    """Controle de la grille exacte contre la reference rationnelle (cas limites et tirages)."""
    rng = np.random.default_rng(seed)
    cases = [0.0, -0.0, 0.0625, -0.0625, 0.0005, -0.0005, 0.0015, 2.5e-4, 1e-9, -1e-9, 123.4565, -123.4565,
             6.5e6 + 0.0005, 4.8e6 - 0.0004999, 2 ** 30 + 0.25, -(2 ** 30) - 0.75, 1.0 / 3.0, 79.9995]
    sample64 = np.concatenate([np.array(cases), rng.normal(0, 100, 4000), rng.uniform(-7e6, 7e6, 4000),
                               rng.uniform(-1e-3, 1e-3, 2000)])
    sample32 = np.concatenate([np.array(cases[:12], dtype=np.float32), rng.normal(0, 40, 6000).astype(np.float32)])
    bad = 0
    for arr in (sample64.astype(np.float64), sample32):
        got = quantize_float_mm(arr).tolist()
        ref = quantize_float_mm_reference(arr)
        bad += sum(1 for g, r in zip(got, ref) if g != r)
    # LAS : echelles decimales usuelles et non entieres
    raw = rng.integers(-(2 ** 31), 2 ** 31 - 1, 3000)
    for scale, offset in ((0.01, 650000.0), (0.001, -12.345), (0.00025, 0.0), (0.0001, 4800000.12345)):
        got, _ = quantize_scaled_int_mm(raw, scale, offset)
        a, c = decimal_fraction(scale) * 1000, decimal_fraction(offset) * 1000
        ref = []
        for x in raw.tolist():
            f = a * x + c + fractions.Fraction(1, 2)
            ref.append(f.numerator // f.denominator)
        bad += sum(1 for g, r in zip(got.tolist(), ref) if g != r)
    if bad:
        raise AssertionError('grille exacte fausse sur %d valeurs' % bad)
    return dict(values_checked=int(sample64.size + sample32.size + 4 * raw.size), mismatches=0)


# ---------------------------------------------------------------------------------------------- domaine, doublons
def bits_of(maximum: int) -> int:
    return int(maximum).bit_length()


def translate_to_min(q: np.ndarray) -> tuple[np.ndarray, list, list, list]:
    """u = q - min(q) par axe ; refuse une etendue >= 2^32 mm (hors u32)."""
    if q.ndim != 2 or q.shape[1] != 3 or q.shape[0] == 0:
        raise ValueError('nuage vide ou mal forme')
    lo = q.min(axis=0)
    hi = q.max(axis=0)
    extent = (hi - lo).astype(np.int64)
    if int(extent.max()) >= (1 << 32):
        raise ValueError('etendue hors u32 : %s mm' % extent.tolist())
    u = (q - lo).astype(np.uint32)
    bits = [bits_of(e) for e in extent.tolist()]
    return u, [int(v) for v in lo.tolist()], [int(v) for v in extent.tolist()], bits


def profile_for(bits: int) -> str:
    for width, name in PROFILES:
        if bits <= width:
            return name
    raise ValueError('plus de 32 bits')


def _keys(u: np.ndarray):
    """Cle de tri par position ; uint64 empaquete si les bits tiennent, sinon None (lexsort)."""
    bits = [bits_of(int(u[:, a].max())) if len(u) else 0 for a in range(3)]
    if sum(bits) <= 63:
        by, bz = bits[1], bits[2]
        key = (u[:, 0].astype(np.uint64) << np.uint64(by + bz)) | (u[:, 1].astype(np.uint64) << np.uint64(bz)) \
            | u[:, 2].astype(np.uint64)
        return key
    return None


def distinct_sites(u: np.ndarray) -> dict:
    """Tri lexicographique stable (x, puis y, puis z ; egalites par indice source) et positions distinctes.

    Rend order (permutation stable des retours), first (indice source du premier retour de chaque position, dans
    l'ordre lexicographique des positions), counts (multiplicites), inverse (rang du site de chaque retour, en ordre
    source) et l'histogramme des multiplicites. L'ordre lexicographique est celui des entrees du contrat v11
    (ng00-ng02) : le fichier de coordonnees ne depend que de l'ensemble des positions.
    """
    n = len(u)
    key = _keys(u)
    if key is not None:
        order = np.argsort(key, kind='stable')
        sk = key[order]
        new = np.empty(n, dtype=bool)
        new[0] = True
        np.not_equal(sk[1:], sk[:-1], out=new[1:])
        del sk
    else:
        order = np.lexsort((u[:, 2], u[:, 1], u[:, 0]))  # tri indirect stable, x prioritaire
        su = u[order]
        new = np.empty(n, dtype=bool)
        new[0] = True
        new[1:] = np.any(su[1:] != su[:-1], axis=1)
        del su
    starts = np.flatnonzero(new)
    first = order[starts]  # stable : plus petit indice source du groupe
    counts = np.diff(np.append(starts, n))
    inverse = np.empty(n, dtype=np.int64)
    inverse[order] = np.cumsum(new) - 1
    hist_values, hist_counts = np.unique(counts, return_counts=True)
    return dict(order=order, first=first, counts=counts, inverse=inverse,
                histogram={int(k): int(v) for k, v in zip(hist_values.tolist(), hist_counts.tolist())})


# ---------------------------------------------------------------------------------------------- ecriture
def write_u32(path: Path, array: np.ndarray) -> str:
    data = np.ascontiguousarray(np.asarray(array).astype('<u4', copy=False))
    tmp = Path(str(path) + '.tmp')
    data.tofile(tmp)
    os.replace(tmp, path)
    return sha256_file(path)


def write_case(out_dir: Path, name: str, u: np.ndarray, ids: np.ndarray, extra: dict | None = None) -> dict:
    """Ecrit `<nom>.u32le` (x, y, z entrelaces) et `<nom>.ids.u32le` ; `extra` = {suffixe: tableau u32}."""
    out_dir.mkdir(parents=True, exist_ok=True)
    if u.dtype != np.uint32 or u.ndim != 2 or u.shape[1] != 3:
        raise TypeError('coordonnees attendues en (n, 3) uint32')
    if len(ids) != len(u) or (len(ids) and int(np.max(ids)) >= (1 << 32)):
        raise ValueError('identifiants incoherents')
    coords = out_dir / (name + '.u32le')
    idf = out_dir / (name + '.ids.u32le')
    record = dict(name=name, coordinates=coords.name, point_ids=idf.name, count=int(len(u)),
                  coordinate_encoding='little_endian_u32_xyz', point_id_encoding='little_endian_u32',
                  sha256=write_u32(coords, u), ids_sha256=write_u32(idf, ids), bytes=int(12 * len(u)))
    for suffix, array in (extra or {}).items():
        path = out_dir / ('%s.%s.u32le' % (name, suffix))
        record[suffix] = dict(file=path.name, sha256=write_u32(path, array), count=int(len(array)))
    return record


def prepare_outputs(out_dir: Path, name: str, q: np.ndarray, ids: np.ndarray | None = None,
                    variants: tuple = ('raw', 'distinct'), labels: np.ndarray | None = None,
                    primary: str = 'raw') -> dict:
    """q (n, 3) int64 en mm -> translation au minimum, mesures, ecriture des variantes (ordre lexicographique).

    Sans doublon : un seul fichier `<nom>`. Avec doublons et primary='raw' : `<nom>` = tous les retours (refuse par
    defaut par le moteur, D8) et `<nom>.distinct` = positions distinctes + `.mult` + `.inverse`. Avec
    primary='distinct' : `<nom>` = positions distinctes + `.mult` (+ `<nom>.raw` si 'raw' est demande).
    """
    n = len(q)
    if ids is None:
        ids = np.arange(n, dtype=np.int64)
    ids = np.asarray(ids)
    if n >= (1 << 32):
        raise ValueError('plus de 2^32 points')
    u, lo, extent, bits = translate_to_min(q)
    d = distinct_sites(u)
    n_distinct = int(len(d['first']))
    dup = n - n_distinct
    measures = dict(n_points=int(n), n_distinct=n_distinct, duplicate_points=int(dup),
                    duplicate_rate=(dup / n if n else 0.0),
                    positions_with_multiplicity_gt1=int(sum(v for k, v in d['histogram'].items() if k > 1)),
                    max_multiplicity=int(max(d['histogram'])) if d['histogram'] else 0,
                    multiplicity_histogram={str(k): v for k, v in sorted(d['histogram'].items())[:16]},
                    translation_mm=lo, extent_mm=extent, extent_m=[e / 1000.0 for e in extent],
                    bits_per_axis=bits, bits_needed=max(bits), minimal_profile=profile_for(max(bits)))
    order_note = 'lexicographic (x, y, z) of the written coordinates; ties by source index'
    outputs = {}

    def write_raw(case_name):
        order = d['order']
        extra = {'labels': labels[order]} if labels is not None else None
        rec = write_case(out_dir, case_name, u[order], ids[order], extra)
        rec['duplicate_sites'] = int(dup)
        rec['site_order'] = order_note
        if dup:
            rec['engine_default'] = 'refused (D8: multiplicities refused by default)'
        return rec

    def write_distinct(case_name, with_inverse):
        first = d['first']
        extra = dict(mult=d['counts'])
        if with_inverse:
            extra['inverse'] = d['inverse']
        if labels is not None:
            extra['labels'] = labels[first]
        rec = write_case(out_dir, case_name, u[first], ids[first], extra)
        rec['duplicate_sites'] = 0
        rec['declared_object'] = 'tower of distinct positions (D8), not the weighted tower'
        rec['site_order'] = order_note + '; id = smallest source id at that position'
        return rec

    if dup == 0:
        outputs['raw'] = write_raw(name)
    elif primary == 'raw':
        if 'raw' in variants:
            outputs['raw'] = write_raw(name)
        if 'distinct' in variants:
            outputs['distinct'] = write_distinct(name + '.distinct', True)
    else:
        outputs['distinct'] = write_distinct(name, True)
        if 'raw' in variants:
            outputs['raw'] = write_raw(name + '.raw')
    return dict(measures=measures, outputs=outputs)


def scene_manifest(name: str, family: str, dataset: dict, source: dict, conversion: dict, result: dict,
                   script: str, extra: dict | None = None) -> dict:
    record = dict(schema=SCHEMA_SCENE, name=name, family=family, dataset=dataset, source=source,
                  conversion=dict(grid_step_metres=GRID_STEP, grid_rounding=ROUNDING,
                                  translation='minimum of each axis of the written cloud', **conversion),
                  measures=result['measures'], outputs=result['outputs'], generator=generator_info(script),
                  created_utc=utc_now(), public_status='not_claimed')
    if extra:
        record.update(extra)
    return record


def log(message: str) -> None:
    sys.stderr.write('[%s] %s\n' % (time.strftime('%H:%M:%S'), message))
    sys.stderr.flush()
