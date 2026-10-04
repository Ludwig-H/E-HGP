#!/usr/bin/env python3
"""Lemme R, contrôle autonome borné : coins fermés, Gram et Fraction.

Pour une boîte fermée Q et deux sites s,z, le gap
 ||z-c||²-||s-c||² est affine en c. Son maximum <0 certifie
 z strictement plus proche que s sur TOUT Q ; son minimum >0
 certifie l'inverse. Si s est un générateur exactement sur une
 sphère de centre c dans Q, ces deux certificats signifient
 intérieur strict / extérieur strict. Les unions sur les générateurs
 sont disjointes ; aucun contact, y compris un autre site de coquille,
 ne peut appartenir à ces unions. Une transposée n'est pas une
 approximation : elle encode les mêmes couples avec rôles inversés.

Le scan masqué et le scan exact donnent donc chaque même signe,
dans le même ordre, et s'arrêtent au même premier intérieur de rang
theta+1. Sur les sorties non saturées, ils gardent tout I et tout U.
La complétude de la liste locale envers le nuage global requiert
séparément G2 / liste K-certifiée ; ce programme ne la qualifie pas.
Ni calcul natif, ni campagne produit, ni gain de temps revendiqué.
"""
from fractions import Fraction as F
from itertools import product
import json

CHECKS = 0

def need(value, message):
    global CHECKS
    CHECKS += 1
    if not value:
        raise ValueError(message)

def dist2(a, b):
    return sum((x-y)**2 for x,y in zip(a,b))

def dot(a, b):
    return sum(x*y for x,y in zip(a,b))

def sphere(points):
    """Solve the affine Gram system, not the C++ q2/q3/q4 formulas."""
    a = points[0]
    us = [tuple(x-y for x,y in zip(p,a)) for p in points[1:]]
    n = len(us)
    rows = [[F(dot(u,v)) for v in us] + [F(dot(u,u),2)] for u in us]
    for i in range(n):
        pivot = next((j for j in range(i,n) if rows[j][i]), None)
        if pivot is None:
            raise ValueError('fixture dependent support')
        rows[i],rows[pivot] = rows[pivot],rows[i]
        r = rows[i][i]
        rows[i] = [v/r for v in rows[i]]
        for j in range(n):
            if j != i:
                r = rows[j][i]
                rows[j] = [x-r*y for x,y in zip(rows[j],rows[i])]
    weights = [r[-1] for r in rows]
    c = tuple(F(a[d])+sum(w*u[d] for w,u in zip(weights,us)) for d in range(3))
    return c, dist2(c,a)

def build_masks(points, box):
    """Independent eight-corner oracle, cross-checking source affine formula."""
    m = len(points)
    words = (m+63)//64
    inner = [[0]*words for _ in points]
    outer = [[0]*words for _ in points]
    lo,hi = box
    corners = list(product(*zip(lo,hi)))
    for i in range(m):
        for j in range(i+1,m):
            x,y = points[i],points[j]
            gaps = [dist2(y,c)-dist2(x,c) for c in corners]
            delta = tuple(v-u for u,v in zip(x,y))
            base = dist2(y,(0,0,0))-dist2(x,(0,0,0))
            cmin = sum((lo[d] if delta[d]>0 else hi[d])*delta[d] for d in range(3))
            cmax = sum((hi[d] if delta[d]>0 else lo[d])*delta[d] for d in range(3))
            need(max(gaps)==base-2*cmin, 'closed maximum affine / corners')
            need(min(gaps)==base-2*cmax, 'closed minimum affine / corners')
            if max(gaps)<0:
                inner[i][j//64] |= 1<<(j%64)
                outer[j][i//64] |= 1<<(i%64)
            elif min(gaps)>0:
                inner[j][i//64] |= 1<<(i%64)
                outer[i][j//64] |= 1<<(j%64)
    for i in range(m):
        need(not (inner[i][i//64]>>(i%64)&1), 'no reflexive dominance')
        for j in range(m):
            need(bool(inner[i][j//64]>>(j%64)&1)==bool(outer[j][i//64]>>(i%64)&1), 'exact transpose')
        for masks in (inner,outer):
            need(masks[i][-1]>>(m-64*(words-1))==0, 'no unused high bits')
    return inner,outer

def signs(points, support, ball, matrices):
    c,r2 = ball
    inside,outside = matrices
    words = len(inside[0])
    im = [0]*words
    om = [0]*words
    for s in support:
        need(dist2(points[s],c)==r2, 'factory generator is shell')
        for w in range(words):
            im[w] |= inside[s][w]
            om[w] |= outside[s][w]
    for a,b in zip(im,om):
        need(a&b==0, 'no contradictory mask for an owned center')
    full,masked = [],[]
    numeric = 0
    for i,p in enumerate(points):
        power = dist2(p,c)-r2
        truth = (power>0)-(power<0)
        full.append(truth)
        ib = im[i//64]>>(i%64)&1
        ob = om[i//64]>>(i%64)&1
        if i in support:
            relation = 0
        elif ib:
            relation = -1
        elif ob:
            relation = 1
        else:
            numeric += 1
            relation = truth
        masked.append(relation)
        need(relation==truth, 'exact per-site census relation')
        if truth==0:
            need(not ib and not ob, 'all contacts retained, not only generators')
    return full,masked,im,om,numeric

def scan(relations, support, threshold):
    interior,shell = [],[]
    cursor = 0
    for i,relation in enumerate(relations):
        if cursor<len(support) and i==support[cursor]:
            cursor += 1
        if relation<0:
            if len(interior)==threshold:
                return {'complete':False,'interior':interior,'shell':shell,'logical_tests':i+1,'support_cursor':cursor,'stop':i}
            interior.append(i)
        elif relation==0:
            shell.append(i)
    return {'complete':True,'interior':interior,'shell':shell,'logical_tests':len(relations),'support_cursor':cursor,'stop':None}

def large_fixture(m):
    """Indices 63/64 and 127/128 cross a REAL mask-word boundary."""
    points = [(10,20,20),(30,20,20)]
    points += [(45+i,40,40) for i in range(m-2)]
    points[63] = (20,20,20)     # certified interior, bit63
    points[64] = (40,20,20)     # certified exterior, next word bit0
    if m>65:
        points[65] = (20,30,20) # additional shell, must not be masked
    if m>128:
        points[127] = (20,21,20)
        points[128] = (20,10,20)
    return points,(0,1),((19,19,19),(21,21,21))

def run_case(name, points, support, box):
    need(len(set(points))==len(points), 'distinct unit sites')
    support = tuple(sorted(support))
    ball = sphere([points[s] for s in support])
    c,r2 = ball
    need(all(l<=x<h for l,x,h in zip(box[0],c,box[1])), 'half-open ownership before census')
    masks = build_masks(points,box)
    full,masked,im,om,numeric = signs(points,support,ball,masks)
    p = full.count(-1)
    for threshold in range(p+2):
        a,b = scan(full,support,threshold),scan(masked,support,threshold)
        need(a==b, 'same lists, exact saturation index, logical ledger and support cursor')
        if a['complete']:
            need(len(a['interior'])==p, 'complete strict I')
            need(len(a['shell'])==full.count(0), 'complete entire U')
            need(a['support_cursor']==len(support), 'all ordered generators encountered')
        else:
            need(a['logical_tests']==a['stop']+1 and full[a['stop']]==-1, 'same first extra interior causes rejection')
    return {'name':name,'sites':len(points),'arity':len(support),'words':len(im),'interiors':p,'shell':full.count(0),'logical_tests_complete':len(points),'masked_numeric_calls_complete':numeric,'off_support_exact_calls_complete':len(points)-len(support),'inside_indices':[i for i in range(len(points)) if im[i//64]>>(i%64)&1],'outside_indices':[i for i in range(len(points)) if om[i//64]>>(i%64)&1]}

def main():
    results = []
    for m in (65,66,129):
        points,support,box = large_fixture(m)
        for name,order in (('forward',list(range(m))),('reversed',list(reversed(range(m))))):
            pos = {old:new for new,old in enumerate(order)}
            results.append(run_case('q2_'+str(m)+'_'+name,[points[i] for i in order],tuple(pos[i] for i in support),box))
    for q,points,support in (
        (3,[(30,20,20),(14,28,20),(14,12,20),(20,20,20),(20,30,20),(20,20,30),(50,20,20)],(0,1,2)),
        (4,list(product((18,22),repeat=3))+[(20,20,20),(40,20,20)],(0,3,5,6)),
    ):
        results.append(run_case('q'+str(q)+'_extra_shell',points,support,((19,19,19),(21,21,21))))
    # Contact on the CLOSED box face: replacing < by <= would lose a shell site.
    points = [(0,0,0),(2,0,0),(1,1,0)]
    box = ((1,0,0),(2,1,1))
    results.append(run_case('closed_face_contact',points,(0,1),box))
    gaps = [dist2(points[2],c)-dist2(points[0],c) for c in product(*zip(*box))]
    need(max(gaps)==0 and min(gaps)<0, 'non-strict mutant would falsely mark a shell site interior')
    need(dist2(points[2],(1,0,0))==1, 'contact witness at owned lower face')
    # Half-open owner rejects its upper face, although certificates always use closure.
    need(not all(l<=x<h for l,x,h in zip((0,0,0),(1,0,0),(1,1,1))), 'upper-face ownership is not silently closed')
    # Same exact mask arithmetic near each explicit coordinate profile maximum.
    profiles = []
    for bits in (18,21,24):
        M = 1<<bits
        pts = [(M-4,M-2,M-2),(M-2,M-2,M-2),(M-3,M-2,M-2),(0,0,0)]
        results.append(run_case('u'+str(bits)+'_high',pts,(0,1),((M-3,M-2,M-2),(M,M,M))))
        need(12*M*M < 1<<63, 'source i64 coarse affine bound remains safe')
        profiles.append({'bits':bits,'bound_12M2_bits':(12*M*M).bit_length()})
    # Workspace scalar model, not allocation execution or Buffer/RSS qualification.
    memories = []
    for C in (65,129,1024):
        words = (C+63)//64
        before=8*C*words
        after=16*C*words
        need(after-before==8*C*words, 'extra transpose mask bytes per worker')
        memories.append({'capacity':C,'words':words,'both_mask_bytes_per_worker':after,'added_bytes_per_worker':after-before})
    row0 = results[0]
    need(63 in row0['inside_indices'] and 64 in row0['outside_indices'], 'explicit 63/64 causal mask test')
    row129 = next(r for r in results if r['name']=='q2_129_forward')
    need(127 in row129['inside_indices'] and 128 not in row129['inside_indices']+row129['outside_indices'], '127/128 interior versus shell')
    print(json.dumps({'status':'ok','checks':CHECKS,'fixtures':results,'profiles':profiles,'memory_scalar':memories,'native_run':False,'gain_of_time_claimed':False},sort_keys=True,indent=2))

if __name__=='__main__':
    main()
