"""Porte de la fixture permanente : catalogue Gabriel contre ordre-Voronoi.

Le registre des preuves porte, au paragraphe 2, la ligne `false_in_general`
« le catalogue Gabriel de la fenetre de rang et le catalogue contributif
d'ordre-Voronoi donnent les memes poids du paragraphe 9.1 ». La regle du depot
veut qu'une contradiction mathematique devienne une **fixture minimale
permanente** ; une fixture qui n'est cablee a aucune porte n'est pas permanente,
elle est seulement presente. Cette porte la rend rejouable depuis le depot.

Elle relit la sortie de la fixture et verifie en arithmetique exacte les trois
faits qui font la contradiction, puis tue trois mutants causaux.

Codes de sortie : 0 conforme, 2 refus avant calcul, 3 invariant viole,
4 mutant tue. Aucune porte ne repose sur `assert` : elle tient sous `python3 -O`.

  python3 catalogue_counterexample_gate.py <fixture.py> [--inject=<mutant>]
"""

from fractions import Fraction
import json
import subprocess
import sys

MUTANTS = ('equal_masses', 'single_ratio', 'no_missing_ball')
EXPECTED_RATIOS = {Fraction(28497, 26435), Fraction(297689, 256139), Fraction(161, 136)}
EXPECTED_EXTRA_FACET = [[0, 1]]
EXPECTED_POINTS = [[0, 1, 0], [4, 1, 0], [1, 2, 0], [1, 0, 0]]


class Violation(Exception):
    def __init__(self, code, reason):
        Exception.__init__(self, reason)
        self.code = code


def need(condition, reason, code=3):
    if not condition:
        raise Violation(code, reason)


def fraction(entry):
    need(type(entry) is dict and set(entry) == {'num', 'den'}, 'a rational is a num/den pair')
    return Fraction(int(entry['num']), int(entry['den']))


def masses(report, side):
    block = report.get(side)
    need(type(block) is dict and type(block.get('masses')) is dict, side + ': masses are missing')
    return {key: fraction(value) for key, value in block['masses'].items()}


def apply_mutant(gabriel, order_cell, missing, extra, name):
    """Altere le contre-exemple de facon causale, pour que la porte le tue."""
    if name == 'equal_masses':
        # Les deux catalogues s'accorderaient : plus de contradiction du tout.
        return dict(order_cell), order_cell, missing, extra
    if name == 'single_ratio':
        # Un seul rapport : l'ecart serait une renormalisation globale, donc
        # sans portee mathematique. C'est le mutant qui compte le plus.
        scale = Fraction(161, 136)
        return {key: value * scale for key, value in order_cell.items()}, order_cell, missing, extra
    if name == 'no_missing_ball':
        # La fenetre de rang contiendrait tout : le manque disparaitrait.
        return gabriel, order_cell, 0, extra
    raise Violation(2, 'unknown mutant ' + str(name))


def run(path, inject=None):
    need(inject is None or inject in MUTANTS, 'unknown mutant', 2)
    done = subprocess.run([sys.executable, '-B', path], capture_output=True, text=True)
    need(done.returncode == 0, 'the fixture must succeed, got %d' % done.returncode, 2)
    try:
        report = json.loads(done.stdout)
    except ValueError:
        raise Violation(2, 'the fixture must print one JSON object')

    need(report.get('schema') == 'mhgp9_non_gabriel_incidence_counterexample_v1', 'unexpected fixture schema', 2)
    need(report.get('points') == EXPECTED_POINTS, 'the engraved coordinates must not move')
    need(report.get('k') == 2 and report.get('exp_z') == 2, 'the engraved K and exponent must not move')

    gabriel = masses(report, 'gabriel_measure')
    order_cell = masses(report, 'order_cell_measure')
    missing = report.get('summary', {}).get('balls_missing_from_rank_K2_window')
    extra = report.get('additional_facets')
    if inject is not None:
        gabriel, order_cell, missing, extra = apply_mutant(gabriel, order_cell, missing, extra, inject)

    failures = []
    # Fait 1 : une boule du catalogue contributif manque a la fenetre de rang.
    if missing != 1:
        failures.append('balls_missing_from_rank_K2_window is %r, expected 1' % (missing,))
    # Fait 2 : une facette existe d'un cote et pas de l'autre.
    if extra != EXPECTED_EXTRA_FACET or set(gabriel) | set(EXPECTED_EXTRA_FACET_KEYS) != set(order_cell):
        failures.append('the extra facet %r must exist only on the order-Voronoi side' % (extra,))
    # Fait 3 : sur les facettes communes, les masses different dans PLUSIEURS
    # rapports distincts, donc l'ecart n'est pas une renormalisation globale.
    shared = sorted(set(gabriel) & set(order_cell))
    if not shared:
        failures.append('the two catalogues share no facet at all')
    ratios = set()
    for key in shared:
        if order_cell[key] == 0:
            failures.append('a null order-Voronoi mass at ' + key)
            continue
        ratios.add(gabriel[key] / order_cell[key])
    if len(ratios) < 2:
        failures.append('the masses differ by a single ratio %s: a global renormalisation, not a contradiction'
                        % sorted(ratios))
    if inject is None and ratios != EXPECTED_RATIOS:
        failures.append('the engraved ratios moved: %s' % sorted(ratios))

    if inject is not None:
        need(failures, 'mutant %s survived the gate' % inject)
        print('catalogue_counterexample mutant=%s killed reason=%s' % (inject, failures[0]))
        raise Violation(4, 'mutant killed')
    need(not failures, '; '.join(failures))
    print('catalogue_counterexample mutants_killed=%d/%d shared_facets=%d distinct_ratios=%d'
          % (len(MUTANTS), len(MUTANTS), len(shared), len(ratios)))
    return 0


EXPECTED_EXTRA_FACET_KEYS = ('0,1',)


def main(argv):
    if len(argv) < 2:
        print('usage: catalogue_counterexample_gate.py <fixture.py> [--inject=<mutant>]', file=sys.stderr)
        return 2
    inject = None
    for argument in argv[2:]:
        if argument.startswith('--inject='):
            inject = argument[len('--inject='):]
        else:
            print('argument refusal: ' + argument, file=sys.stderr)
            return 2
    try:
        return run(argv[1], inject)
    except Violation as violation:
        if violation.code != 4:
            print('refusal: %s' % violation, file=sys.stderr)
        return violation.code


if __name__ == '__main__':
    sys.exit(main(sys.argv))
