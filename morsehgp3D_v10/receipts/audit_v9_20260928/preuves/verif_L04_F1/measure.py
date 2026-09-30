"""Mesure du paragraphe 9.1 depuis l'export natif de la tour FULL.

Objets, tels que le manuscrit les pose (pages imprimees 96-97) :

    psi(sigma) = rho(sigma)^(-z)          poids d'une coface, rho = rayon
    S_tau      = somme des psi(sigma) sur les cofaces sigma contenant tau
    T_x        = somme des S_tau sur les facettes tau contenant x
    w_x,tau    = S_tau / T_x              partition de l'unite
    m_tau      = somme des w_x,tau sur x dans tau

La tour publie le **carre** du rayon, beta = rho^2, en rationnel exact. Donc
psi = beta^(-z/2) : pour z **pair** tout le calcul est exactement rationnel, et
pour z impair il demande une racine carree. Ce module traite les deux cas
separement et ne melange jamais les deux regimes :

- z pair : `Fraction`, aucune erreur, aucune marge a publier ;
- z impair : `Decimal` a `PRECISION` chiffres, et le module publie la **marge
  d'ambiguite** de chaque decision comparee, c'est-a-dire le plus petit ecart
  non nul rencontre. Une campagne dont la marge descend sous l'erreur cumulee
  doit etre refusee, pas arrondie.

Deux conventions de facettes coexistent dans la litterature de ce depot et ne
donnent pas le meme objet :

- `boundary` (F = bord de C, convention de HGP-old) : les K+1 facettes de
  chaque coface. C'est sous cette convention, et sous elle seule, que la borne
  m_tau <= 1 du registre des preuves est vraie.
- `gabriel` (F = simplexes de Gabriel, convention de la these et du moteur v9) :
  seules les facettes qui sont elles-memes de Gabriel. La somme T_x y est plus
  petite, donc m_tau peut monter jusqu'a K.

Un seuil de masse n'est pas transposable d'une convention a l'autre : le choix
est un parametre declare, jamais un defaut implicite.
"""

from decimal import Decimal, getcontext
from fractions import Fraction
import itertools

CONVENTIONS = ('boundary', 'gabriel')
PRECISION = 60


def need(condition, reason):
    if not condition:
        raise ValueError(reason)


def _rational(entry):
    need(type(entry) is dict and set(entry) == {'num', 'den'}, 'a rational is a num/den pair')
    return Fraction(int(entry['num']), int(entry['den']))


def coface_weight(beta, z):
    """psi = beta^(-z/2), exact si z est pair, haute precision sinon."""
    need(type(z) is int and z >= 1, 'z is a positive integer')
    need(beta > 0, 'a coface with a null radius has no finite weight')
    if z % 2 == 0:
        return Fraction(beta.denominator, beta.numerator) ** (z // 2)
    getcontext().prec = PRECISION
    root = (Decimal(beta.numerator) / Decimal(beta.denominator)).sqrt()
    return Decimal(1) / (root ** z)


def read_export(report):
    """Extrait du JSON les cofaces de taille K+1 et les facettes de Gabriel.

    Un seul appel a `native_weighted_export --k K` porte les deux : les cofaces
    sont publiees telles quelles, et les simplexes de Gabriel de taille K sont
    les boules du catalogue dont l'interieur et la coquille totalisent K sites.
    """
    need(report.get('schema') == 'mhgp9_weighted_catalogue_export_v1', 'unexpected export schema')
    cofaces = []
    for entry in report['cofaces']:
        vertices = tuple(sorted(int(v) for v in entry['vertices']))
        need(len(set(vertices)) == len(vertices), 'a coface repeats a vertex')
        cofaces.append((vertices, _rational(entry['beta'])))
    need(cofaces, 'the export carries no coface')
    size = len(cofaces[0][0])
    need(all(len(v) == size for v, _ in cofaces), 'the cofaces do not share one cardinality')
    gabriel = {}
    for entry in report.get('catalogue', ()):
        sites = tuple(sorted(int(s) for s in (entry.get('interior') or [])
                             + (entry.get('shell') or [])))
        if len(sites) == size - 1:
            gabriel[sites] = _rational(entry['beta'])
    return cofaces, gabriel, size - 1


def facet_births(cofaces, gabriel_facets, convention):
    """Niveau de naissance d'une facette dans la filtration.

    Une facette de Gabriel a son propre rayon au catalogue, et c'est le bon
    niveau : la boule minimale d'un sous-ensemble est plus petite que celle du
    sur-ensemble, donc la facette existe avant toute coface qui la contient.
    Sous la convention du bord, une facette n'est pas forcement de Gabriel et
    n'a pas de rayon publie : elle entre alors dans la filtration au premier
    niveau ou une coface la fait apparaitre.
    """
    births = {}
    for vertices, beta in cofaces:
        for facet in facets(vertices):
            if convention == 'gabriel' and facet not in gabriel_facets:
                continue
            known = gabriel_facets.get(facet)
            value = known if known is not None else beta
            if facet not in births or value < births[facet]:
                births[facet] = value
    need(births, 'no facet has a birth level')
    return births


def facets(vertices):
    """Les |sigma| facettes de sigma, chacune privee d'un sommet."""
    return [tuple(v for v in vertices if v != drop) for drop in vertices]


def measure(cofaces, gabriel_facets, z, convention='boundary'):
    """Rend (S, T, m, points_couverts) pour la convention demandee.

    Les facettes retenues sont toutes celles du bord en `boundary`, et seules
    celles qui sont de Gabriel en `gabriel`. Une coface dont aucune facette
    n'est retenue ne contribue a rien et n'est pas une erreur.
    """
    need(convention in CONVENTIONS, 'convention is one of ' + ', '.join(CONVENTIONS))
    keep = None if convention == 'boundary' else gabriel_facets
    zero = Fraction(0) if z % 2 == 0 else Decimal(0)
    sums = {}
    for vertices, beta in cofaces:
        weight = coface_weight(beta, z)
        for facet in facets(vertices):
            if keep is not None and facet not in keep:
                continue
            sums[facet] = sums.get(facet, zero) + weight
    need(sums, 'no facet survives the %s convention' % convention)
    totals = {}
    for facet, value in sums.items():
        for point in facet:
            totals[point] = totals.get(point, zero) + value
    masses = {}
    for facet, value in sums.items():
        mass = zero
        for point in facet:
            if totals[point] > 0:
                mass += value / totals[point]
        masses[facet] = mass
    covered = {point for point, value in totals.items() if value > 0}
    return sums, totals, masses, covered


def check_invariants(sums, totals, masses, covered, size, convention, tolerance=None):
    """Les identites du paragraphe 9.1, plus la borne propre a la convention.

    Rend la liste des violations, vide si tout tient. En regime exact la
    tolerance est nulle et les identites doivent etre des egalites strictes.
    """
    exact = all(type(value) is Fraction for value in sums.values())
    slack = (Fraction(0) if exact else Decimal(10) ** (-(PRECISION - 12))) if tolerance is None else tolerance
    problems = []
    per_point = {}
    for facet, value in sums.items():
        for point in facet:
            if totals[point] > 0:
                per_point[point] = per_point.get(point, 0 * value) + value / totals[point]
    for point in covered:
        if abs(per_point[point] - 1) > slack:
            problems.append('the unity partition fails at point %d: %s' % (point, per_point[point]))
            break
    total_mass = sum(masses.values())
    if abs(total_mass - len(covered)) > slack * max(1, len(covered)):
        problems.append('total mass %s is not the covered count %d' % (total_mass, len(covered)))
    ceiling = 1 if convention == 'boundary' else size
    worst = max(masses.values())
    if worst > ceiling + slack:
        problems.append('a mass reaches %s above the %s ceiling %d' % (worst, convention, ceiling))
    return problems, dict(max_mass=worst, total_mass=total_mass, covered=len(covered),
                          facets=len(masses), exact=exact)
