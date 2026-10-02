"""Controle du juge FULL : faits analytiques et corruptions, sans executable natif."""
import copy
import json
import sys

import forest_oracle as oracle


def blank_work():
    work = dict.fromkeys(oracle.WORK,0)
    work['cells'] = dict(combinations=0, passes=0, trace_tests=0, meb_calls=0,
                         meb=dict.fromkeys(oracle.data.MEB,0))
    work['classification'] = dict(combinations=0,examined=0,meb_calls=0,meb=dict.fromkeys(oracle.data.MEB,0))
    descent = dict.fromkeys(oracle.data.COUNTS,0)
    descent.update(part_meb=dict.fromkeys(oracle.data.MEB,0), trace_meb=dict.fromkeys(oracle.data.MEB,0),
                   census=dict.fromkeys(oracle.data.CENSUS,0))
    work['descent'] = descent
    return work


def answer(req, bits):
    row = dict(status='ok', reason='none', coord_bits=bits, kmax=req['kmax'], sites=[], site_ids=[],
               balls=None, orders=[], forest_memory=dict(after=0,peak=0), owner_after=0)
    reason = oracle.refusal(req,bits)
    if reason:
        row.update(reason=reason, status='resource_exhausted' if reason == 'memory_budget' else
                   'unsupported_degeneracy' if reason == 'multiplicity_unsupported' else 'invalid_input')
        return row
    sites, identifiers, expected = oracle.truth(tuple(req['records']),req['kmax'])
    _, _, balls, metadata = oracle.data.geometry(tuple(req['records']),req['kmax'])
    row.update(sites=[list(p) for p in sites], site_ids=[list(ids) for ids in identifiers],
               balls=copy.deepcopy(metadata), forest_memory=dict(after=0,peak=1))
    for order in expected:
        nodes = []
        for node in order.nodes:
            seed = None
            if node.center is not None:
                if node.level == 0:
                    seed = dict(site=sites.index(node.center), ball=None)
                else:
                    found = [i for i,b in enumerate(balls) if b.center == node.center and b.level == node.level]
                    oracle.require(len(found) == 1, 'naissance de definition dans CatK')
                    seed = dict(site=None, ball=found[0])
            nodes.append(dict(level=oracle.data.encoded(node.level), seed=seed, parent=None, children=list(node.children)))
        parent = oracle.parents(nodes)
        for node, p in zip(nodes,parent):
            node['parent'] = p
        births = sum(not n.children for n in order.nodes)
        work = blank_work()
        for key,value in oracle.geometric_work(tuple(sites),order.k).items():
            if key in ('cells','classification'):
                work[key].update(value)
            else:
                work[key] = value
        calls = work['cells']['meb_calls']
        work['cells']['meb'].update(presentations=calls,nondegenerate=calls,positive=calls,containing=calls)
        calls = work['classification']['meb_calls']
        work['classification']['meb'].update(presentations=calls,nondegenerate=calls,positive=calls,containing=calls)
        calls = work['trace_resolutions']+work['vertical_descents']
        work['descent'].update(steps=calls,catalogue_hits=calls)
        work['descent']['part_meb'].update(presentations=calls,nondegenerate=calls,positive=calls,containing=calls)
        row['orders'].append(dict(order=order.k, nodes=nodes, lower=None if order.lower is None else list(order.lower),
                                  root=parent.index(None), births=births, node_capacity=2*births-1,
                                  edge_capacity=2*births-2, ledger=work))
    row['forest_memory']['peak'] = oracle.retained_minimum(row['orders'])
    return row


def main():
    checks, corruptions, facts, totals = 0, 0, 0, []
    for bits in (18,21,24):
        reqs = oracle.requests(bits); rows = [answer(req,bits) for req in reqs]
        judged = [oracle.judge(row,req,bits) for row,req in zip(rows,reqs)]
        total = {key: sum(r[key] for r in judged) for key in judged[0]}
        checks += total['checks']; totals.append(dict(bits=bits, requests=len(reqs), **total))
        def find(name):
            i = next(i for i,r in enumerate(reqs) if r['name'] == name)
            return reqs[i], rows[i]
        req, row = find('line024'); first, second, third = row['orders']
        oracle.require(len(first['nodes']) == 4 and first['nodes'][3]['children'] == [0,1,2] and
                       oracle.rational(first['nodes'][3]['level']) == 1, 'fusion trois aire atomique')
        oracle.require([oracle.rational(n['level']) for n in second['nodes']] == [1,1,4], 'niveaux k2')
        oracle.require(second['lower'] == [3,3,3] and third['lower'] == [2], 'verticales fermees et remontees')
        oracle.require(oracle.cut(first['nodes'],oracle.parents(first['nodes']),oracle.data.F(1),False) == [0,1,2]
                       and oracle.cut(first['nodes'],oracle.parents(first['nodes']),oracle.data.F(1),True) == [3],
                       'coupes ouverte et fermee distinctes')
        oracle.require(all(oracle.rational(n['level']) != 4 for n in first['nodes']), 'racines deja reliees sans noeud')
        facts += 5
        _, two = find('two_components_same_plateau')
        merges = [n for n in two['orders'][0]['nodes'] if n['seed'] is None and oracle.rational(n['level']) == 1]
        oracle.require(len(merges) == 2 and all(len(n['children']) == 3 for n in merges), 'deux fusions de meme niveau')
        _, square = find('square_center')
        fourth = square['orders'][3]
        oracle.require(any(n['seed'] is not None and n['seed']['ball'] is not None and
                           len(square['balls'][n['seed']['ball']]['shell']) == 4 for n in fourth['nodes']),
                       'naissance etendue')
        _, tetra = find('regular_tetra')
        zero_seeds = [n['seed']['site'] for n in tetra['orders'][0]['nodes'] if n['seed'] is not None]
        oracle.require(zero_seeds != sorted(zero_seeds), 'XYZ canonique different de Morton')
        _, line = find('line12')
        oracle.require(len(line['orders']) == 12 and len(line['orders'][-1]['nodes']) == 1, 'ordre plafond12')
        facts += 4
        _, line13 = find('line13_K12')
        twelfth = line13['orders'][-1]
        oracle.require(len(twelfth['nodes']) == 3 and twelfth['births'] == 2 and
                       twelfth['nodes'][2]['children'] == [0,1] and
                       [oracle.rational(n['level']) for n in twelfth['nodes']] ==
                       [oracle.data.F(121,4),oracle.data.F(121,4),oracle.data.F(36)],
                       'coface13 relie les deux naissances K12 sans MEB de taille13')
        facts += 1
        plain_req, plain = find('square_plain')
        oracle.require(plain['orders'][0]['ledger']['continuations'] == 1 and
                       len(plain['orders'][0]['nodes']) == 5, 'continuation etendue sans nouveau noeud')
        _, unusual = find('global_q3_nonfirst_shell')
        center_ball = next(b for b in unusual['balls'] if len(b['shell']) == 5)
        oracle.require(center_ball['qmin'] == 3 and center_ball['support'] == [1,2,3], 'support global sans premier U')
        facts += 2
        for name in ('line024','square_center','maximum_tetra'):
            original_req, original = find(name); reverse_req, reverse = find(name+'_reverse')
            oracle.equal(original, reverse); facts += 1
        def corrupt(request, good, mutation):
            nonlocal corruptions
            bad = copy.deepcopy(good); mutation(bad)
            try:
                oracle.judge(bad,request,bits)
            except (ValueError,KeyError,TypeError,IndexError):
                corruptions += 1
                return
            raise ValueError('corruption non detectee')
        for mutate in (
            lambda v: v.__setitem__('status','invalid_input'),
            lambda v: v.__setitem__('reason','parameter_out_of_range'),
            lambda v: v.__setitem__('coord_bits',17),
            lambda v: v.__setitem__('owner_after',1),
            lambda v: v['forest_memory'].__setitem__('after',1),
            lambda v: v['forest_memory'].__setitem__('peak',0),
            lambda v: v['forest_memory'].__setitem__('peak',1),
            lambda v: v['forest_memory'].__setitem__('peak',True),
            lambda v: v['orders'].pop(),
            lambda v: v['orders'][0].__setitem__('order',2),
            lambda v: v['orders'][0].__setitem__('root',0),
            lambda v: v['orders'][0]['nodes'].pop(),
            lambda v: v['orders'][0]['nodes'][3]['children'].pop(),
            lambda v: v['orders'][0]['nodes'][3]['children'].append(0),
            lambda v: v['orders'][0]['nodes'][3].__setitem__('level',['0','1']),
            lambda v: v['orders'][0]['nodes'][3].__setitem__('level',['1','0']),
            lambda v: v['orders'][0]['nodes'][0].__setitem__('parent',None),
            lambda v: v['orders'][0]['nodes'][0]['seed'].__setitem__('site',1),
            lambda v: v['orders'][0]['nodes'][0]['seed'].__setitem__('ball',0),
            lambda v: v['orders'][0]['nodes'][3].__setitem__('seed',dict(site=0,ball=None)),
            lambda v: v['orders'][0].__setitem__('lower',[0,0,0,0]),
            lambda v: v['orders'][1].__setitem__('lower',[0,1,3]),
            lambda v: v['orders'][2].__setitem__('lower',[0]),
            lambda v: v['orders'][1]['lower'].__setitem__(0,True),
            lambda v: v['orders'][0]['ledger'].__setitem__('plateaus',-1),
            lambda v: v['orders'][0]['ledger'].__setitem__('unions',True),
            lambda v: v['orders'][0]['ledger'].__setitem__('birth_presentations',0),
            lambda v: v['orders'][0]['ledger'].__setitem__('trace_resolutions',0),
            lambda v: v['orders'][0]['ledger'].__setitem__('classified_cells',0),
            lambda v: v['orders'][0]['ledger'].__setitem__('touched_components',0),
            lambda v: v['orders'][1]['ledger'].__setitem__('vertical_checks',0),
            lambda v: v['orders'][1]['ledger'].__setitem__('ancestor_queries',0),
            lambda v: v['orders'][1]['ledger'].__setitem__('ancestor_activations',0),
            lambda v: v['orders'][1]['ledger'].__setitem__('ancestor_unions',0),
            lambda v: v['orders'][1]['ledger'].__setitem__('ancestor_find_steps',1000000),
            lambda v: v['orders'][1]['ledger'].__setitem__('ancestor_hops',1),
            lambda v: v['orders'][0]['ledger']['classification'].__setitem__('combinations',0),
            lambda v: v['orders'][0]['ledger']['classification'].__setitem__('examined',1),
            lambda v: v['orders'][0]['ledger']['classification'].__setitem__('meb_calls',1),
            lambda v: v['orders'][0].__setitem__('node_capacity',0),
            lambda v: v['orders'][0].__setitem__('edge_capacity',0),
            lambda v: v['orders'][0]['ledger']['cells'].__setitem__('meb_calls',1),
            lambda v: v['orders'][0]['ledger']['descent'].__setitem__('steps',1),
            lambda v: v['balls'][0]['shell'].pop(),
            lambda v: v['balls'][0].__setitem__('level',['0','1']),
            lambda v: v['sites'][0].__setitem__(0,7),
            lambda v: v['site_ids'][0].__setitem__(0,7),
            lambda v: v.__setitem__('unexpected',0),
        ):
            corrupt(req,row,mutate)
        bad_req, bad = find('no_forest_memory')
        corrupt(bad_req,bad,lambda v: v.__setitem__('orders',[copy.deepcopy(first)]))
        corrupt(bad_req,bad,lambda v: v.__setitem__('balls',[]))
        corrupt(plain_req,plain,lambda v: v['orders'][0]['ledger'].__setitem__('continuations',0))
        # Une expansion binaire du plateau a les bonnes feuilles, mais un noeud de trop et un lien non strict.
        binary = copy.deepcopy(row)
        binary['orders'][0]['nodes'][3]['children'] = [0,1]
        binary['orders'][0]['nodes'].append(dict(level=['1','1'],seed=None,parent=None,children=[2,3]))
        binary['orders'][0]['nodes'][2]['parent'] = 4; binary['orders'][0]['nodes'][3]['parent'] = 4
        binary['orders'][0]['root'] = 4
        corrupt(req,binary,lambda _: None)
    malformed = 0
    for line in ('{', '{"x":1,"x":2}', '{"x":NaN}'):
        try:
            oracle.data.parse(line)
        except ValueError:
            malformed += 1
    oracle.require(corruptions == 156 and malformed == 3 and facts == 45 and checks >= 78000, 'planchers modele')
    oracle.require(not any(name.endswith('.constructive') for name in sys.modules), 'voie constructive absente')
    print(json.dumps(dict(verdict='conforme', native=0, corruptions=corruptions, malformed=malformed,
                         facts=facts, checks=checks, profiles=totals), sort_keys=True))


if __name__ == '__main__':
    try:
        main()
    except (ValueError,KeyError,TypeError,IndexError) as error:
        print('REFUS '+str(error),file=sys.stderr)
        raise SystemExit(1)
