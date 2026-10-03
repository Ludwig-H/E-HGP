"""Certificat de contact limite aux sites de presentation, juge par Gram/Fraction."""
from itertools import combinations

from fixtures import fixtures
from fraction_model import circumsphere, distance2, morton, require


def scan(points, sphere, threshold, known=()):
    center, radius, _ = sphere
    inner, shell = [], []
    for index, point in enumerate(points):
        power = 0 if index in known else distance2(center, point) - radius
        if power < 0:
            if len(inner) == threshold:
                return 'saturated', tuple(inner), tuple(shell), index + 1
            inner.append(index)
        elif power == 0:
            shell.append(index)
    return 'complete', tuple(inner), tuple(shell), len(points)


def main():
    selected = {'line3', 'acute_triangle', 'regular_tetra', 'obtuse_prefix',
                'extended_q4', 'octa_center', 'extended_q3', 'maximum_face',
                'maximum_tetra', 'random0'}
    checks = presentations = shortcuts = corruptions = 0
    arities, saturated, extended = set(), set(), set()
    for bits in (18, 21, 24):
        for fixture in fixtures(bits):
            if fixture.name not in selected:
                continue
            points = tuple(sorted(fixture.points, key=morton))
            for q in (2, 3, 4):
                for support in combinations(range(len(points)), q):
                    sphere = circumsphere(tuple(points[i] for i in support))
                    if sphere is None or not all(w > 0 for w in sphere[2]):
                        continue
                    presentations += 1
                    require(all(distance2(sphere[0], points[i]) == sphere[1] for i in support),
                            'contact exact de chaque site constructeur')
                    checks += 1
                    for k in range(max(1, q - 1), 6):
                        threshold = k + 1 - q
                        expected = scan(points, sphere, threshold)
                        actual = scan(points, sphere, threshold, support)
                        require(actual == expected, 'census ou prefixe sature divergent')
                        checks += 1
                        shortcuts += sum(i < actual[3] for i in support)
                        arities.add((bits, q))
                        if actual[0] == 'saturated':
                            saturated.add((bits, q))
                        elif len(actual[2]) > q:
                            extended.add((bits, q))
                        # Une appartenance au support local n'autorise jamais les q premiers sites.
                        wrong = scan(points, sphere, threshold, tuple(range(q)))
                        if wrong != expected:
                            corruptions += 1
    require(len(arities) == 9 and len(saturated) == 9 and len(extended) == 9,
            'arites, saturation et coquilles etendues non vacantes aux trois profils')
    require(presentations > 500 and checks > 2000 and corruptions > 500 and shortcuts > 5000,
            'planchers du modele de contact')
    print('support_contact_model_verdict conforme presentations%d checks%d contacts%d corruptions%d native0'
          % (presentations, checks, shortcuts, corruptions))


if __name__ == '__main__':
    main()
