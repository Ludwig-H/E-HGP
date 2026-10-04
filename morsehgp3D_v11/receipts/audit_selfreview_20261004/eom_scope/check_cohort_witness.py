#!/usr/bin/env python3
"""Témoin autonome : EOM z16 réfute « toute EOM, à tout z ».

Entrée = cohortes figées de la condensation A/mcs20, copiées par un modèle
indépendant depuis les métadonnées PointTree du lot clos. Aucun point, ID,
étiquette, NPZ, bibliothèque numérique, moteur ou fit n'est nécessaire ici.

S_C(z)=sum_w w*(r_cohorte**(-z)-r_top**(-z)). Pour chaque racine carrée,
L=floor(sqrt(q)*2**p)/2**p et U=L ou L+2**(-p) encadrent exactement sqrt(q).
Donc les dates H=√t+√M−√q et leurs puissances inverses sont encadrées par
Fractions. On certifie une inégalité stricte entre l'upper score du parent
et les lower scores de ses deux enfants admissibles. Le meilleur score
récursif des descendants est au moins cette somme : l'EOM ne retient donc
pas ce parent, même si les descendants sont ensuite raffinés.

La persistance positive après condensation est indispensable : un bloc brut
fugace ou un bon IoU à une coupe ne suffit pas. Les trois masses et les
cohortes sont conservées ; z16 ne change pas la condensation. Aucune qualité
d'objet ni recommandation de choisir z16 n'est déduite.
"""
from fractions import Fraction as F
from pathlib import Path
import hashlib,json,math

CHECKS=0

def need(ok,message):
    global CHECKS
    CHECKS+=1
    if not ok:raise ValueError(message)


def sqrt_bounds(q,p):
    need(q>=0,'radicande positif'); scale=1<<p
    w=math.isqrt(q.numerator*scale*scale//q.denominator)
    lo=F(w,scale);hi=lo if lo*lo==q else F(w+1,scale)
    need(lo*lo<=q<=hi*hi,'racine encadree')
    return lo,hi


def radius_bounds(row,p):
    t,m,q=[F(x) for x in row]
    tl,th=sqrt_bounds(t,p);ml,mh=sqrt_bounds(m,p);ql,qh=sqrt_bounds(q,p)
    lo,hi=tl+ml-qh,th+mh-ql
    need(0<lo<=hi,'date positive')
    return lo,hi


def score_bounds(c,z,p):
    top=radius_bounds(c['top'],p)
    low=high=F()
    for row in c['cohorts']:
        count=row['count'];need(type(count) is int and count>0,'cohorte entiere positive')
        lo,hi=radius_bounds(row['radius'],p)
        need(hi<top[0],'cohorte strictement avant sortie')
        low+=count*(F(1)/hi**z-F(1)/top[0]**z)
        high+=count*(F(1)/lo**z-F(1)/top[1]**z)
    need(low<=high,'score encadre')
    return low,high


def main():
    path=Path(__file__).with_name('cohort_witness.json');w=json.loads(path.read_text());c={x['cluster']:x for x in w['clusters']}
    parent=c[w['parent']];kids=[c[i] for i in parent['children']]
    need(len(kids)==2,'deux enfants admissibles')
    need(parent['parent']>=0,'parent non racine exclue')
    for x in [parent]+kids:
        need(x['mass']==sum(y['count'] for y in x['cohorts']),'masse preservee')
        need(x['mass']>=w['mcs'],'cluster admissible')
    for x in kids:need(x['parent']==parent['cluster'],'parent commun')
    pb=score_bounds(parent,16,80);kb=[score_bounds(x,16,80) for x in kids]
    children_low=sum((v[0] for v in kb),F())
    need(pb[1]<children_low,'enfants battent strictement parent z16')
    factor=100**16; dyad=1<<32
    v=pb[1]*factor*dyad; upper=F(-(-v.numerator//v.denominator),dyad)
    v=children_low*factor*dyad; lower=F(v.numerator//v.denominator,dyad)
    need(pb[1]*factor<=upper<lower<=children_low*factor,'certificat dyadique final')
    gap=lower-upper
    print(json.dumps(dict(schema='ehgp.audit.eom_score_certificate.v1',checks=CHECKS,z=16,mcs=w['mcs'],witness_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),source_pointtree_sha256=w['source_pointtree_sha256'],
         normalized_parent_upper_exact=str(upper),normalized_children_lower_exact=str(lower),normalized_gap_lower_exact=str(gap),strict_comparison=True,
         normalized_by_common_positive_factor='100**16',normalized_parent_upper=float(pb[1]*100**16),normalized_children_lower=float(children_low*100**16)),indent=2,sort_keys=True))
if __name__=='__main__':main()
