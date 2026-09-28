"""Portes de la mesure du paragraphe 9.1 : identites, bornes, exactitude.

La fixture `fixture_tiny64_k2.json` est l'export natif d'une scene de
soixante-quatre points du banc calibre (spherical, quatre groupes, niveau
`easy`, graine 3, quantifiee au millimetre), a K = 2. Elle est gravee pour que
ces portes tiennent sans reconstruire le binaire natif.

Aucune porte ne repose sur `assert` : elles tiennent sous `python3 -O`.

  python3 -m unittest discover -s . -p 'test_*.py'
"""

from fractions import Fraction
import json
import pathlib
import unittest

import measure as M

FIXTURE = pathlib.Path(__file__).with_name('fixture_tiny64_k2.json')


def need(condition, reason):
    if not condition:
        raise AssertionError(reason)


def load():
    return M.read_export(json.loads(FIXTURE.read_text()))


class Weights(unittest.TestCase):
    def test_even_exponents_are_exactly_rational(self):
        """A z pair, psi = beta^(-z/2) est un rationnel : aucun arrondi nulle part."""
        beta = Fraction(9, 4)
        need(M.coface_weight(beta, 2) == Fraction(4, 9), 'z=2 gives the inverse of beta')
        need(M.coface_weight(beta, 4) == Fraction(16, 81), 'z=4 gives its square')
        need(type(M.coface_weight(beta, 2)) is Fraction, 'an even exponent stays rational')

    def test_odd_exponents_are_high_precision_and_agree_with_the_square(self):
        beta = Fraction(9, 4)
        from decimal import Decimal
        odd = M.coface_weight(beta, 1)
        even = M.coface_weight(beta, 2)
        reference = Decimal(even.numerator) / Decimal(even.denominator)
        need(abs(odd * odd - reference) < Decimal(10) ** -40, 'psi squared at z=1 is psi at z=2')

    def test_a_null_or_negative_radius_is_refused(self):
        for bad in (Fraction(0), Fraction(-1, 2)):
            with self.assertRaises(ValueError):
                M.coface_weight(bad, 2)
        with self.assertRaises(ValueError):
            M.coface_weight(Fraction(1), 0)


class Section91(unittest.TestCase):
    def test_the_fixture_is_the_expected_export(self):
        cofaces, gabriel, size = load()
        need(len(cofaces) == 223, 'the engraved fixture carries 223 cofaces, got %d' % len(cofaces))
        need(size == 2, 'facets of a K=2 run have two sites')
        need(len(gabriel) > 0, 'the Gabriel facets must be readable from the same run')
        need(len(gabriel) < 3 * len(cofaces), 'Gabriel facets are fewer than the boundary ones')

    def test_invariants_hold_for_every_exponent_and_convention(self):
        """Partition de l'unite et masse totale : les deux identites du 9.1."""
        cofaces, gabriel, size = load()
        for z in (1, 2, 3):
            for convention in M.CONVENTIONS:
                sums, totals, masses, covered = M.measure(cofaces, gabriel, z, convention)
                problems, info = M.check_invariants(sums, totals, masses, covered, size, convention)
                need(not problems, 'z=%d %s: %s' % (z, convention, '; '.join(problems)))
                need(info['covered'] > 0, 'z=%d %s: nothing is covered' % (z, convention))

    def test_the_mass_ceiling_separates_the_two_conventions(self):
        """La borne m_tau <= 1 tient au bord et TOMBE sous la convention Gabriel.

        C'est la ligne du registre des preuves : elle repose sur F = bord de C,
        ou T_x = K fois la somme des poids des cofaces contenant x. Sous la
        convention Gabriel cette egalite devient une inegalite et la masse peut
        depasser un. Un temoin vaut mieux qu'un argument.
        """
        cofaces, gabriel, size = load()
        boundary = M.measure(cofaces, gabriel, 2, 'boundary')
        need(max(boundary[2].values()) <= 1, 'the boundary convention must respect the unit ceiling')
        witness = M.measure(cofaces, gabriel, 2, 'gabriel')
        worst = max(witness[2].values())
        need(worst > 1, 'the Gabriel convention must exhibit a mass above one, got %s' % worst)
        need(worst <= size, 'and it must stay under the ceiling K = %d' % size)

    def test_the_gabriel_convention_keeps_fewer_facets(self):
        cofaces, gabriel, size = load()
        wide = M.measure(cofaces, gabriel, 2, 'boundary')[0]
        narrow = M.measure(cofaces, gabriel, 2, 'gabriel')[0]
        need(set(narrow) <= set(wide), 'Gabriel facets are boundary facets')
        need(len(narrow) < len(wide), 'and strictly fewer of them')

    def test_an_unknown_convention_is_refused(self):
        cofaces, gabriel, _ = load()
        with self.assertRaises(ValueError):
            M.measure(cofaces, gabriel, 2, 'whatever')


if __name__ == '__main__':
    unittest.main()
