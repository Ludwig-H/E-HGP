"""Trace logique du hit prioritaire : capture statique, aucun C++ execute."""
from itertools import product
import json


def require(ok, message):
    if not ok:
        raise ValueError(message)


def reference(foreign_scratch, foreign_memo):
    return 'parameter_out_of_range' if foreign_scratch or foreign_memo else 'success'


def with_population(hit, foreign_scratch, foreign_memo):
    # resolve_descent -> population.descend/descend_each_step peut rendre
    # le hit avant les controles de descent_step et DescentMemo.resolve.
    if hit:
        return 'success'
    return reference(foreign_scratch, foreign_memo)


def guarded_population(hit, foreign_scratch, foreign_memo):
    if foreign_scratch or foreign_memo:
        return 'parameter_out_of_range'
    return 'success'


def main():
    rows = []
    for hit, scratch, memo in product((False, True), repeat=3):
        slow = reference(scratch, memo)
        fast = with_population(hit, scratch, memo)
        fixed = guarded_population(hit, scratch, memo)
        require(slow == fixed, 'contexte valide avant tout hit')
        rows.append({'population_hit': hit, 'foreign_scratch': scratch,
                     'foreign_memo': memo, 'reference': slow, 'current_fast': fast,
                     'guarded_fast': fixed})
    require(sum(r['reference'] != r['current_fast'] for r in rows) == 3,
            'trois cas de contexte contourne')
    print(json.dumps({'rows': rows, 'counterexamples': 3,
        'fixture': 'deux domaines distincts, memes coordonnees; k=1, part={SiteIdx0}; population du domaine A, scratch ou memo de B',
        'native_executions': 0, 'scope': 'source control-flow model, not a native reproduction'}, sort_keys=True))


if __name__ == '__main__':
    main()
