"""Attach weighted facets to an independently supplied, exact FULL forest.

This module does NOT find attachments, enumerate cofaces, infer weights, or
identify a first Gabriel coface with a facet's birth. An attachment supplied
by the caller is the facet's actual birth/connection event. The caller owns
the geometric proof of that event. FULL zero-mass branches are retained in
``geometry`` before zero/unary branches are removed from ``tree``. Different
FULL roots are never joined artificially.

Exact cuts use facet birth dates. The separate ``eom_input`` adapter permits
virtual zero-radius leaves ONLY for min_cluster_size >= 2 and strictly larger
than every facet mass. Lower thresholds retain a valid geometric output, but
are refused by this virtual-leaf EOM adapter. No frozen module is imported.
"""
from __future__ import annotations

from collections import defaultdict
from fractions import Fraction
import math
import numbers


def _need(condition, message):
    if not condition:
        raise ValueError(message)


def _integer(value, name):
    _need(not isinstance(value,bool) and isinstance(value,numbers.Integral) and value>=0,
          name+': nonnegative integer required')
    return int(value)


def _beta(value):
    _need(isinstance(value,dict) and set(value)=={'num','den'},'exact beta requires num, den')
    parts=[]
    for name in ('num','den'):
        raw=value[name]
        if isinstance(raw,str):
            _need(raw.isascii() and raw.isdecimal() and (raw=='0' or not raw.startswith('0')),
                  'canonical unsigned decimal coefficient required')
            raw=int(raw)
        else:
            raw=_integer(raw,'rational coefficient')
        parts.append(raw)
    _need(parts[1]>0,'positive rational denominator required')
    return Fraction(*parts)


def _level(value):
    return dict(num=str(value.numerator),den=str(value.denominator))


def _mass(value):
    _need(not isinstance(value,bool) and isinstance(value,numbers.Real),'real positive mass required')
    try:
        represented=float(value)
    except (OverflowError,ValueError) as error:
        raise ValueError('mass cannot be represented in binary64') from error
    _need(math.isfinite(represented) and represented>0,'positive finite representable mass required')
    return value


def _source(full_nodes,roots):
    _need(isinstance(full_nodes,(list,tuple)) and full_nodes,'nonempty FULL node array required')
    children,levels,successors={}, {}, {}
    for position,row in enumerate(full_nodes):
        _need(isinstance(row,dict) and {'id','level','children','successor'}<=set(row),'FULL node schema')
        node=_integer(row['id'],'FULL id')
        _need(node==position,'FULL IDs must be contiguous and topological')
        kids=[_integer(child,'FULL child') for child in row['children']]
        _need(len(kids)!=1 and len(kids)==len(set(kids)) and all(child<node for child in kids),
              'FULL children: zero or >=2 distinct earlier nodes')
        children[node]=kids; levels[node]=_beta(row['level'])
        successors[node]=None if row['successor'] is None else _integer(row['successor'],'FULL successor')
    incoming={}
    for node,kids in children.items():
        for child in kids:
            _need(child not in incoming and levels[child]<levels[node],'FULL parent uniqueness/strict levels')
            incoming[child]=node
    for node in children:
        _need(successors[node]==incoming.get(node),'FULL children/successor mismatch')
    roots=[_integer(root,'FULL root') for root in roots]
    expected=sorted(node for node in children if node not in incoming)
    _need(len(roots)==len(set(roots)) and sorted(roots)==expected,'FULL root set mismatch')
    return children,levels,successors,expected


def build_full_weighted_tree(full_nodes,roots,masses,attachments):
    """Return exact augmented geometry and its positive-facet merge forest.

    ``full_nodes`` uses native {id, level:{num,den}, children, successor} rows.
    Facet IDs are positions in ``masses`` and ``attachments``. Each attachment
    is {node, beta:{num,den}} with birth <= beta <= successor birth. At the
    closed upper boundary ownership moves to the successor, uniquely. Dates
    beyond that interval are refused, not silently advanced across events.

    Geometry node IDs start at F; facet leaves remain 0..F-1. Every original
    FULL node, including empty branches, has a geometry event. Continuation
    events attach all facets with the same rational date simultaneously.
    Cleaned internal IDs may have gaps and never acquire fabricated levels.
    ``masses`` values are preserved (including Fraction); only positivity and
    representability are checked. Empty facet input yields no positive roots.
    """
    children,levels,successors,source_roots=_source(full_nodes,roots)
    masses=[_mass(value) for value in masses]
    attachments=list(attachments); count=len(masses)
    _need(len(attachments)==count,'one attachment per facet required')
    by_node=defaultdict(lambda:defaultdict(list)); normalized=[]; births=[]
    for facet,attachment in enumerate(attachments):
        _need(isinstance(attachment,dict) and set(attachment)=={'node','beta'},'attachment requires node, beta')
        node=_integer(attachment['node'],'attachment node'); beta=_beta(attachment['beta'])
        _need(node in levels and beta>=levels[node],'attachment before or outside FULL node birth')
        successor=successors[node]
        _need(successor is None or beta<=levels[successor],'attachment exceeds supplied FULL segment')
        owner=successor if successor is not None and beta==levels[successor] else node
        by_node[owner][beta].append(facet); births.append(_level(beta))
        normalized.append(dict(node=owner,original_node=node,beta=_level(beta),
                               moved_at_closed_boundary=owner!=node))
    geometry=[]; tails={}; source_events={}
    def event(source,beta,kids,kind):
        identifier=count+len(geometry)
        geometry.append(dict(id=identifier,source_node=source,kind=kind,beta=_level(beta),children=kids))
        return identifier
    for node in children:
        at_birth=by_node[node].get(levels[node],[])
        kids=[tails[child] for child in children[node]]+list(at_birth)
        current=event(node,levels[node],kids,'FULL_birth')
        source_events[node]=current
        for beta,facets in sorted(by_node[node].items()):
            if beta!=levels[node]:
                current=event(node,beta,[current]+list(facets),'facet_attachment')
        tails[node]=current
    geometric_roots=[tails[node] for node in source_roots]
    # First construct the entire geometry, then remove zero mass and contract
    # unary representations. Positive masses make leaf count a safe zero test.
    representative={leaf:leaf for leaf in range(count)}
    positive_counts={leaf:1 for leaf in range(count)}
    clean_children={}; clean_levels={}; removed_zero=0; removed_unary=0; removed_equal=0
    for row in geometry:
        node=row['id']; beta=_beta(row['beta']); kids=[]
        positive_counts[node]=sum(positive_counts[child] for child in row['children'])
        row['positive_facet_count']=positive_counts[node]
        for child in row['children']:
            value=representative[child]
            if value is None:
                continue
            if value in clean_children and clean_levels[value]==beta:
                kids.extend(clean_children.pop(value)); clean_levels.pop(value)
                removed_equal+=1
            else:
                kids.append(value)
        if not kids:
            representative[node]=None; removed_zero+=1
        elif len(kids)==1:
            representative[node]=kids[0]; removed_unary+=1
        else:
            representative[node]=node; clean_children[node]=kids; clean_levels[node]=beta
    positive_roots=[representative[root] for root in geometric_roots if representative[root] is not None]
    return dict(schema='mhgp9_full_weighted_attachment_tree_v1',n_leaves=count,masses=list(masses),
        leaf_birth_betas=births,attachments=normalized,
        geometry=dict(nodes=geometry,roots=geometric_roots,source_birth_events=source_events,source_segment_tails=tails),
        tree=dict(n=count,children=clean_children,squared_levels={node:_level(beta) for node,beta in clean_levels.items()},
                  roots=positive_roots),
        statistics=dict(full_nodes=len(children),geometry_events=len(geometry),facet_leaves=count,
                        full_roots=len(source_roots),positive_roots=len(positive_roots),
                        zero_mass_events_removed=removed_zero,unary_events_contracted=removed_unary,
                        equal_level_events_contracted=removed_equal,
                        closed_boundary_moves=sum(row['moved_at_closed_boundary'] for row in normalized)),
        attachment_semantics='supplied_facet_birth_and_connection_not_first_Gabriel_coface',
        attachment_geometry_certified_here=False,artificial_root_added=False)


def _virtual_profile(result,min_cluster_size):
    threshold=_mass(min_cluster_size)
    _need(threshold>=2 and float(threshold)>max((float(mass) for mass in result['masses']),default=0),
          'virtual leaves require min_cluster_size >=2 and strictly greater than every facet mass')
    return threshold


def eom_input(result,*,min_cluster_size):
    """Keyword arguments for frozen weighted_condense_eom; no EOM is run here.

    Multiple positive roots are refused, not glued at infinity. Empty mass
    remains an empty geometric forest and has no single-root EOM adaptation.
    Distinct surviving exact radii that collide in binary64 are refused.
    """
    threshold=_virtual_profile(result,min_cluster_size)
    tree=result['tree']; _need(len(tree['roots'])==1,'single positive root required for EOM')
    heights={}; represented={}
    for node,raw_beta in tree['squared_levels'].items():
        beta=_beta(raw_beta)
        try:
            radius=math.sqrt(float(beta))
        except (OverflowError,ValueError) as error:
            raise ValueError('merge radius is unrepresentable') from error
        _need(math.isfinite(radius) and (radius>0 or beta==0),'merge radius under/overflows')
        _need(radius not in represented or represented[radius]==beta,'distinct exact merge radii collide in binary64')
        represented[radius]=beta; heights[node]=radius
    return dict(n_leaves=result['n_leaves'],children={node:list(kids) for node,kids in tree['children'].items()},
                heights=heights,masses=list(result['masses']),min_cluster_size=threshold,allow_single_cluster=False)


def tree_cut(result,beta,*,closed=True,virtual=False,min_cluster_size=None):
    """Exact active-facet partition; inactive facets are listed separately.

    Beta is a Fraction or {num,den}. Virtual cuts keep zero-born singleton
    leaves and require the same explicit mass-threshold profile as eom_input.
    They are not geometric active-facet cuts. Returns sorted facet ID blocks.
    """
    _need(type(closed) is bool and type(virtual) is bool,'boolean cut modes required')
    if virtual:
        _virtual_profile(result,min_cluster_size)
    if isinstance(beta,Fraction):
        _need(beta>=0,'nonnegative cut required'); at=beta
    else:
        at=_beta(beta)
    tree=result['tree']; parents={child:node for node,kids in tree['children'].items() for child in kids}
    levels={node:_beta(level) for node,level in tree['squared_levels'].items()}
    admitted=lambda value:value<=at if closed else value<at
    groups=defaultdict(list); inactive=[]
    for facet,raw_birth in enumerate(result['leaf_birth_betas']):
        birth=Fraction(0) if virtual else _beta(raw_birth)
        if not admitted(birth):
            inactive.append(facet); continue
        node=facet
        while node in parents and admitted(levels[parents[node]]):
            node=parents[node]
        groups[node].append(facet)
    return dict(blocks=sorted(groups.values()),inactive=inactive,virtual=virtual)
