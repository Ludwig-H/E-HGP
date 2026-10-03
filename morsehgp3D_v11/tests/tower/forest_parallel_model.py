"""Faits indépendants du plateau mêlant paires régulières et cercle étendu.

Definition balaie les parties/cofaces de Gamma_1 ; aucune construction native
ni DSU produit ne fournit les attendus de forest_parallel_test.cpp.
"""
from fractions import Fraction
import forest_oracle as oracle


def run():
    points = ((0,10,0),(2,10,0),(10,10,0),(11,9,0),(12,10,0),(11,11,0),(20,10,0),(22,10,0))
    records = oracle.data.fixtures.records(points)
    sites, _, orders = oracle.truth(records, 1)
    _, _, _, balls = oracle.data.geometry(records, 1)
    work = oracle.geometric_work(sites, 1)
    checks = 0
    def same(got, expected):
        nonlocal checks
        if got != expected:
            raise ValueError(f'fait indépendant divergent: {got!r} != {expected!r}')
        checks += 1
    same(len(balls), 9)
    same([(len(b['shell']), b['qmin']) for b in balls if oracle.rational(b['level']) == 1],
         [(2,2),(4,2),(2,2)])
    for key, expected in {'classified_cells':9, 'replayed_cells':9, 'plateaus':3,
                          'trace_resolutions':20, 'continuations':1, 'touched_components':12}.items():
        same(work[key], expected)
    same(len(orders[0].nodes), 12)
    same([(n.level, len(n.children)) for n in orders[0].nodes if n.children],
         [(Fraction(1,2),4),(Fraction(1),2),(Fraction(1),2),(Fraction(16),3)])
    same(work['cells'], dict(combinations=20, passes=2, trace_tests=8, meb_calls=0))
    same(work['unions'], 7)
    print(f'forest_parallel_model_verdict conforme facts{checks} native0')


if __name__ == '__main__':
    run()
