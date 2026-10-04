"""Serialisations au format des dumps de la v10 (cli/mhgp10_catalogue.cpp et cli/mhgp10_tower.cpp), octet pour octet.

Ce qui est l'OBJET (independant de toute convention) : l'ensemble des boules admises avec leur interieur, leur
coquille, q_min, leurs poids et leur niveau exact ; par ordre, l'arbre de fusion, ses niveaux exacts, les images
verticales, les entrees core (niveau D_k et noeud).
Ce qui est une CONVENTION de la v10, reproduite ici pour l'identite d'octets :
  - l'ordre des sites (cle de Morton) et l'ordre des boules (niveau exact, puis support canonique S* compare comme
    suite de rangs de Morton, completee par un rang plus grand que tous) ;
  - le rang d'un niveau (rang dense des niveaux exacts distincts des boules de q_min >= 2) et son ECRITURE non
    reduite num / den : celle de la premiere boule du rang, dans la forme de sa premiere presentation (emitted_level) ;
  - la numerotation des noeuds : naissances dans l'ordre des boules (a k = 1, les sites dans l'ordre de Morton),
    puis fusions par (niveau, plus petite naissance du sous-arbre) ;
  - l'entree cover aux egalites exactes : la composante de la PREMIERE boule couvrante dans l'ordre des boules. La
    verite est un ensemble de composantes (Entry.nodes) ; ce choix depend du rang de Morton.
Le catalogue accepte les multiplicites (poids p et u, drapeau de coquille ponderee) ; la tour de la v10 les refuse :
tower_dump leve ValueError sur un nuage a doublons.

Lignes. Catalogue : "rang q p u drapeaux | S* | I | U", un site = " x,y,z".
Tour : "order k noeuds sites", "node v parent num den image", puis par site "point x y z noeud niveau" (core, et
ordre 1 de cover) ou "point x y z noeud r<rang>" (cover, k >= 2 ; rang = rang du niveau de la boule + 1).
Les options des juges du raccord R2 (--dump-levels, --dump-births, niveau exact de l'entree cover) ne sont pas
reproduites : le binaire fige ne les a pas, et un dump complet se compare sans elles.
"""
from math import gcd

from . import intgeom as G
from .model import InvariantError

EXTENDED_SHELL = 1
WEIGHTED_SHELL = 2


def emitted_level(ref, ball):
    """Ecriture (num, den) du niveau d'une boule de q_min >= 2, regle de generator.cpp (emitted_level) : la forme de
    la presentation qui l'emettait en premier dans l'enumeration "paires, puis chaque triplet suivi de ses
    quadruplets". q_min = 2 : forme de la paire S*. q_min = 4 : forme du tetraedre S*. q_min = 3 : forme du premier
    tetraedre de coquille a centre strictement interieur dont le prefixe (i, j, k) precede S*, sinon forme de S*."""
    pos = [ref.sites[s] for s in ball.support]
    if ball.qmin == 2:
        return G.level2(pos[0], pos[1])
    if ball.qmin == 4:
        return G.level4(G.center4(pos[0], pos[1], pos[2], pos[3]))
    shell = ball.shell_sites
    m = len(shell)
    for i in range(m):
        for j in range(i + 1, m):
            for k in range(j + 1, m):
                if not (shell[i], shell[j], shell[k]) < ball.support:
                    return G.level3(pos[0], pos[1], pos[2])
                for h in range(k + 1, m):
                    t = [ref.sites[shell[x]] for x in (i, j, k, h)]
                    c4 = G.center4(t[0], t[1], t[2], t[3])
                    if c4 is not None and G.tetra_position(t, t[0], c4) == 1:
                        return G.level4(c4)
    return G.level3(pos[0], pos[1], pos[2])


class EngineCatalogue(object):
    """Vue du catalogue telle que la v10 la publie : boules de q_min >= 2, rangs denses, table des ecritures."""

    def __init__(self, ref):
        self.ref = ref
        self.balls = [b for b in ref.balls if b.qmin >= 2]
        self.rank = []     # par boule
        self.table = []    # par rang : (num, den) de la premiere boule du rang
        self.rank_of = {}  # niveau exact -> rang
        for b in self.balls:
            if b.level not in self.rank_of:
                self.rank_of[b.level] = len(self.table)
                self.table.append(emitted_level(ref, b))
            self.rank.append(self.rank_of[b.level])

    def flags(self, ball):
        return (EXTENDED_SHELL if ball.extended else 0) | (WEIGHTED_SHELL if ball.weighted else 0)

    def written(self, level):
        """"num den" d'un niveau de noeud : 0 1 pour le niveau nul, sinon l'ecriture du rang."""
        if level == 0:
            return '0 1'
        if level not in self.rank_of:
            raise InvariantError('niveau %s absent du catalogue' % level)
        return '%d %d' % self.table[self.rank_of[level]]


def _site(ref, s):
    return ' %d,%d,%d' % ref.sites[s]


def catalogue_dump(ref):
    """Dump de mhgp10_catalogue pour K = ref.kmax (poids p et u, multiplicites comprises)."""
    cat = EngineCatalogue(ref)
    out = []
    for b, rank in zip(cat.balls, cat.rank):
        head = '%d %d %d %d %d' % (rank, b.qmin, b.p, b.m, cat.flags(b))
        fields = [''.join(_site(ref, s) for s in sites) for sites in (b.support, b.inner_sites, b.shell_sites)]
        out.append(head + ' |' + ' |'.join(fields) + '\n')
    return ''.join(out)


def v10_numbering(ref, k, nodes):
    """Numerotation des noeuds d'un ordre dans la convention de la v10 : rend remap[canonique] = identifiant v10."""
    position = []
    for v, node in enumerate(nodes):
        if node.center is None:
            break
        ball = ref._by_key.get((node.center, node.level))
        if ball is None:
            raise InvariantError('ordre %d : boule de naissance absente du catalogue' % k)
        position.append((ball.index, v))
    nb = len(position)
    remap = [None] * len(nodes)
    for new, (_index, v) in enumerate(sorted(position)):
        remap[v] = new
    leaf = list(remap)
    merges = []
    for v in range(nb, len(nodes)):
        leaf[v] = min(leaf[c] for c in nodes[v].children)
        merges.append((nodes[v].level, leaf[v], v))
    for new, (_level, _leaf, v) in enumerate(sorted(merges)):
        remap[v] = nb + new
    return remap


def tower_dump(ref, tower=None, entry='core'):
    """Dump de mhgp10_tower pour K = ref.kmax, ordres 1 .. min(K, sites).

    tower : objet qui rend order(k) et node_at(k, partie, niveau) ; par defaut ref (etage B). Passer une Definition
    serialise la verite de l'etage A dans les conventions de la v10 (le catalogue de ref reste necessaire : il porte
    l'ordre des boules et l'ecriture des niveaux).
    entry : 'core' ou 'cover'.
    """
    if entry not in ('core', 'cover'):
        raise ValueError('entree %r inconnue' % (entry,))
    if len(ref.sites) != ref.n:
        raise ValueError('nuage a doublons : la tour de la v10 refuse les multiplicites')
    tower = tower if tower is not None else ref
    cat = EngineCatalogue(ref)
    out = []
    below = None
    for k in range(1, ref.orders + 1):
        res = tower.order(k)
        remap = v10_numbering(ref, k, res.nodes)
        parent = [-1] * len(res.nodes)
        for v, node in enumerate(res.nodes):
            for c in node.children:
                parent[c] = remap[v]
        lines = [None] * len(res.nodes)
        for v, node in enumerate(res.nodes):
            lines[remap[v]] = 'node %d %d %s %d\n' % (remap[v], parent[v], cat.written(node.level),
                                                      -1 if below is None else below[res.lower[v]])
        out.append('order %d %d %d\n' % (k, len(res.nodes), len(ref.sites)))
        out.extend(lines)
        ref.order(k)  # premieres boules couvrantes (cover_choice)
        for s in range(len(ref.sites)):
            x = ref.inp[s]
            where = 'point %d %d %d' % ref.sites[s]
            if entry == 'core' or k == 1:
                e = res.core[x]
                if e.level.denominator != 1:
                    raise InvariantError('entree core non entiere')
                out.append('%s %d %d\n' % (where, remap[e.nodes], e.level.numerator))
                continue
            ball = ref.balls[ref.cover_choice[k][x][0]]
            part = [ref.inp[y] for y in sorted(ball.inner + ball.shell)[:k]]
            node = tower.node_at(k, part, ball.level)
            out.append('%s %d r%d\n' % (where, remap[node], cat.rank_of[ball.level] + 1))
        below = remap
    return ''.join(out)


def reduce_tower_levels(text):
    """Dump de tour dont chaque niveau "num den" des lignes node est reecrit en fraction reduite. Deux dumps egaux
    apres cette reecriture decrivent le meme objet dans les memes numerotations : seule l'ecriture non reduite des
    niveaux, convention de la v10, peut les distinguer."""
    out = []
    for line in text.splitlines(True):
        t = line.split(' ')
        if t[0] == 'node':
            num, den = int(t[3]), int(t[4])
            g = gcd(num, den)
            t[3], t[4] = str(num // g), str(den // g)
            line = ' '.join(t)
        out.append(line)
    return ''.join(out)


def write_u32le(path, points):
    """Nuage au format d'entree de la v10 : x, y, z en u32 petit-boutiste par point."""
    with open(path, 'wb') as f:
        for p in points:
            for c in p:
                f.write(int(c).to_bytes(4, 'little'))
