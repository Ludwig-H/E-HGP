"""Outils communs des portes du juge JUG-EMST.

Python 3.10 nu (bibliotheque standard seule), aucun assert : une porte rend le meme code sous python3 -O.
Contenu : ecriture des nuages .u32le, appel du juge, lecture de sa forme texte, empreintes SHA-256 recalculees ici
(memes schemas que le juge), ordre de Morton exact, ecriture d'un vidage MHGP11FUL1 a l'ordre un (format de
bench/full_probe.cpp de la v11 gelee ac081a06f).
"""
import hashlib
import json
import os
import struct
import subprocess
from fractions import Fraction

OK, DESACCORD, USAGE, INVARIANT, MUTANT_TUE = 0, 1, 2, 3, 4
SAUTE = 77
WORD = struct.Struct('<Q')
NONE = (1 << 32) - 1


def ecrire_nuage(chemin, points):
    with open(chemin, 'wb') as sortie:
        for p in points:
            sortie.write(struct.pack('<3I', *p))


def ecrire_ids(chemin, ids):
    with open(chemin, 'wb') as sortie:
        for i in ids:
            sortie.write(struct.pack('<I', i))


def lancer(juge, arguments, delai=1800):
    """(code, objet JSON de la sortie ou None, sortie d'erreur)."""
    fini = subprocess.run([juge] + list(arguments), stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=delai, check=False)
    objet = None
    texte = fini.stdout.decode('utf-8', 'replace').strip()
    if texte:
        try:
            objet = json.loads(texte)
        except ValueError:
            objet = None
    return fini.returncode, objet, fini.stderr.decode('utf-8', 'replace')


def lire_arbre(chemin):
    """Forme texte du juge : (naissances [(x, y, z)], fusions [(niveau Fraction, enfants tuple)])."""
    naissances, fusions = [], []
    with open(chemin, encoding='ascii') as entree:
        lignes = entree.read().split('\n')
    if not lignes or not lignes[0].startswith('N '):
        raise ValueError('forme texte : en-tete absent')
    for ligne in lignes[1:]:
        if not ligne:
            continue
        mots = ligne.split()
        if mots[0] == 'B' and len(mots) == 4:
            naissances.append(tuple(int(m) for m in mots[1:]))
        elif mots[0] == 'F' and len(mots) >= 5:
            fusions.append((Fraction(int(mots[1]), int(mots[2])), tuple(int(m) for m in mots[3:])))
        else:
            raise ValueError('forme texte : ligne %r' % ligne)
    if len(naissances) != int(lignes[0].split()[1]):
        raise ValueError('forme texte : nombre de naissances')
    return naissances, fusions


def lire_emst(chemin):
    with open(chemin, encoding='ascii') as entree:
        return [tuple(int(m) for m in ligne.split()) for ligne in entree.read().split('\n') if ligne]


def natural(valeur):
    """Entier naturel : longueur sur un mot puis octets petit-boutistes (natural() de la v11)."""
    octets = valeur.to_bytes(max(1, (valeur.bit_length() + 7) // 8), 'little')
    return WORD.pack(len(octets)) + octets


def _fusions(h, fusions):
    h.update(WORD.pack(len(fusions)))
    for niveau, enfants in fusions:
        h.update(natural(niveau.numerator) + natural(niveau.denominator))
        h.update(WORD.pack(len(enfants)))
        for c in enfants:
            h.update(WORD.pack(c))


def empreinte_arbre(naissances, fusions):
    h = hashlib.sha256(b'ehgp.v12.jug_emst.ordre1.v1\0')
    h.update(WORD.pack(len(naissances)))
    for p in naissances:
        for c in p:
            h.update(WORD.pack(c))
    _fusions(h, fusions)
    return h.hexdigest()


def empreinte_fusions(nombre, fusions):
    h = hashlib.sha256(b'ehgp.v12.jug_emst.fusions.v1\0')
    h.update(WORD.pack(nombre))
    _fusions(h, fusions)
    return h.hexdigest()


def empreinte_emst(nombre, aretes):
    h = hashlib.sha256(b'ehgp.v12.jug_emst.emst.v1\0')
    h.update(WORD.pack(nombre))
    for d2, a, b in aretes:
        h.update(natural(d2) + WORD.pack(a) + WORD.pack(b))
    return h.hexdigest()


def cle_morton(p, bits=32):
    return sum(((p[axe] >> bit) & 1) << (3 * bit + axe) for axe in range(3) for bit in range(bits))


def parents(nombre_naissances, fusions):
    pere = [NONE] * (nombre_naissances + len(fusions))
    for j, (_niveau, enfants) in enumerate(fusions):
        for c in enfants:
            pere[c] = nombre_naissances + j
    return pere


def entier_exact(valeur, rembourrage=0):
    signe = 1 if valeur < 0 else 0
    v = abs(valeur)
    mots = []
    while v:
        mots.append(v & ((1 << 64) - 1))
        v >>= 64
    mots = (mots or [0]) + [0] * rembourrage
    return WORD.pack(signe) + WORD.pack(len(mots)) + b''.join(WORD.pack(m) for m in mots)


def vidage_ful1(naissances, fusions, ids, bits=21, kmax=1, formes=None, poids=None):
    """Octets d'un vidage MHGP11FUL1 dont seul l'ordre un est ecrit (kmax = 1 : vidage complet a K = 1).

    naissances : positions dans l'ordre canonique (lexicographique) ; ids : PointId de chaque naissance ;
    fusions : (niveau Fraction, enfants) dans l'ordre canonique ; formes : {noeud: (num, den, rembourrage)} pour
    ecrire un niveau sous une forme non reduite ; poids : {site Morton: poids} pour fabriquer un vidage invalide."""
    formes = formes or {}
    poids = poids or {}
    n = len(naissances)
    morton = sorted(range(n), key=lambda i: cle_morton(naissances[i]))
    out = [b'MHGP11FUL1', WORD.pack(bits), WORD.pack(kmax), WORD.pack(n), WORD.pack(n)]
    for s, i in enumerate(morton):
        x, y, z = naissances[i]
        out.append(b''.join(WORD.pack(v) for v in (x, y, z, poids.get(s, 1), ids[i])))
    noeuds = n + len(fusions)
    pere = parents(n, fusions)
    out.append(b''.join(WORD.pack(v) for v in (1, n, noeuds, noeuds - 1, noeuds - 1)))
    curseur = 0
    for k in range(noeuds):
        if k < n:
            debut, cardinal, niveau = 0, 0, Fraction(0)
        else:
            niveau, enfants = fusions[k - n]
            debut, cardinal = curseur, len(enfants)
            curseur += cardinal
        out.append(WORD.pack(pere[k]) + WORD.pack(debut) + WORD.pack(cardinal))
        num, den, rembourrage = formes.get(k, (niveau.numerator, niveau.denominator, 0))
        out.append(entier_exact(num, rembourrage) + entier_exact(den, rembourrage))
        if k < n:
            out.append(b''.join(entier_exact(c) for c in naissances[k]) + entier_exact(1))
    for _niveau, enfants in fusions:
        out.append(b''.join(WORD.pack(c) for c in enfants))
    return b''.join(out)
