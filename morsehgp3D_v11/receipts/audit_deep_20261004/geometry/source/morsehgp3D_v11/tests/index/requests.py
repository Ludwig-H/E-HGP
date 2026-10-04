"""Matrice bornee des requetes ; les permutations gardent les PointId et le meme reglage de feuille."""
from dataclasses import replace

from fixtures import fixtures
from fraction_model import population
from judge import Request


def requests(bits):
    out, pairs = [], []
    for fixture in fixtures(bits):
        count = len(population(fixture.records, fixture.support)['inner'])
        thresholds = sorted({1, 2, 3, 10, max(1, count - 1), max(1, count), count + 1})
        for leaf in (1, 4, 16):
            for threshold in thresholds:
                out.append(Request('%s_L%d_K%d' % (fixture.name, leaf, threshold), fixture.records,
                                   fixture.support, threshold, leaf))
        original = Request(fixture.name + '_stable', fixture.records, fixture.support, max(1, count), 4)
        out.extend((original, replace(original, name=original.name + '_inverse', records=original.records[::-1])))
        pairs.append((len(out) - 2, len(out) - 1))
        out.append(replace(original, name=fixture.name + '_u32max', threshold=2**32 - 1))
    original = next(f for f in fixtures(bits) if f.name == 'seuil_trois')
    duplicate = original.records + (original.records[0][:3] + (88888,),)
    for threshold in (3, 4):
        out.append(Request('multiplicite_geometrique_K%d' % threshold, duplicate, original.support, threshold))
    base = Request('refus', original.records, original.support, 4)
    out += [replace(base, name='seuil_zero', threshold=0, refusal='parameter_out_of_range'),
            replace(base, name='feuille_zero', leaf=0, refusal='parameter_out_of_range'),
            replace(base, name='feuille_257', leaf=257, refusal='parameter_out_of_range'),
            replace(base, name='budget_index_nul', index_budget=0, refusal='memory_budget'),
            replace(base, name='budget_census_nul', query_budget=0, refusal='memory_budget'),
            replace(base, name='budget_census_sature_nul', threshold=1, query_budget=0, refusal='memory_budget')]
    return out, pairs
