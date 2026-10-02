#!/usr/bin/env python3
"""F3 : sonde binary64 au plus proche et propagation positive par DAG.

Toutes les décisions de ce juge sont rationnelles exactes ; aucune sonde
moteur/C++/FENV/GPU. Normal et -O doivent rendre les mêmes octets.
"""
from fractions import Fraction as F
from itertools import product
from pathlib import Path
import hashlib
import json
import math
import sys

U = F(1, 2**52)


def require(ok, why):
    if not ok:
        raise ValueError(why)


def interval_mul(a,b):
    return (1-U)*a[0]*b[0], (1+U)*a[1]*b[1]


def interval_div(a,b):
    require(b[0] > 0, "dénominateur certifié positif")
    return (1-U)*a[0]/b[1], (1+U)*a[1]/b[0]


def interval_sum(a,b):
    return (1-U)*min(a[0],b[0]), (1+U)*max(a[1],b[1])


def check_manifest(base):
    path = base/"SHA256SUMS"
    if path.exists():
        for row in path.read_text().splitlines():
            expected,relative = row.split("  ",1)
            require(hashlib.sha256((base/relative).read_bytes()).hexdigest() == expected, "SHA256 "+relative)


def run():
    require(sys.float_info.radix == 2 and sys.float_info.mant_dig == 53, "binary64 attendu")
    n,d = 2**53+1,2**53
    nf,df = float(n),float(d)
    rows = []
    tests = 0
    def record(name,value,local_exact,exposure):
        nonlocal tests
        require(math.isfinite(value) and value > 0, "valeur finie positive")
        error = F.from_float(value)/local_exact-1
        require(abs(error) <= U, "hypothèse élémentaire F3")
        rows.append({"operation":name,"binary64_hex":value.hex(),
                     "local_relative_error":str(error),"exposure_E":exposure})
        tests += 1
    record("conversion_N",nf,F(n),1)
    record("conversion_D",df,F(d),1)
    value = nf/df
    exact = F(n,d)
    exposure = 3
    record("quotient",value,F.from_float(nf)/F.from_float(df),exposure)
    for i in range(4):
        local_exact = F.from_float(value)**2
        value *= value
        exact *= exact
        exposure = 2*exposure+1
        record("square_"+str(i+1),value,local_exact,exposure)
    require(value == 1.0 and len(rows) == 7, "sept opérations observées")
    actual_error = abs(F.from_float(value)/exact-1)
    wrong_bound = (1+U)**len(rows)-1
    require(actual_error > wrong_bound, "contre-exemple à m instructions")
    ratio = F.from_float(value)/exact
    safe_bound = (1-U)**(-exposure)-1
    require((1-U)**exposure <= ratio <= (1-U)**(-exposure), "enveloppe E constructive")
    require(actual_error <= safe_bound, "borne relative corrigée")
    tests += 4

    # Une clé voisine fournie exactement rend la fausse borne dangereuse pour F4.
    other = 1+8*U
    require(F.from_float(float(other)) == other, "clé voisine dyadique exacte")
    alleged_upper = 1/(1-wrong_bound)
    require(alleged_upper < other < exact, "faux ordre certifié par la borne documentaire")
    tests += 2

    # Pas un nouveau contre-modèle de réutilisation : contrôle des règles de composition proposées.
    conv = (1-U,1+U)
    for kind,combine in (("mul",interval_mul),("div",interval_div),("sum",interval_sum)):
        lo,hi = combine(conv,conv)
        for da,db,dc in product((-U,U),repeat=3):
            a,b = 2*(1+da),3*(1+db)
            if kind == "mul":
                approximate,ideal = a*b*(1+dc),F(6)
            elif kind == "div":
                approximate,ideal = a/b*(1+dc),F(2,3)
            else:
                approximate,ideal = (a+b)*(1+dc),F(5)
            q = approximate/ideal
            require(lo <= q <= hi, "intervalle "+kind)
            e = 3 if kind != "sum" else 2
            require((1-U)**e <= q <= (1-U)**(-e), "exposition "+kind)
            tests += 2
    quotient_factor = (1+U)/(1-U)
    require(quotient_factor-1 > (1+U)**2-1, "inversion du dénominateur dans le modèle relatif")
    tests += 1

    return {"status":"F3_REAL_BINARY64_COUNTEREXAMPLE_PASS", "guards":tests,
            "u":"2^-52","exact_integer_N":n,"exact_integer_D":d,"executed_operations":len(rows),
            "repeated_squares":4,"exact_final":"((2^53+1)/2^53)^16","binary64_final":value.hex(),
            "actual_error_over_u_display":float(actual_error/U),
            "wrong_bound_over_u_display":float(wrong_bound/U),
            "actual_error_exceeds_wrong_bound":True,"corrected_exposure_E":exposure,
            "corrected_bound_over_u_display":float(safe_bound/U),"trace":rows,
            "synthetic_F4_exact_other_key":"1+8u","wrong_upper_below_other_but_true_key_above":True,
            "quotient_factor_model":"(1+u)/(1-u)","native_engine_calls":0,
            "scope":"concordant avec audit ouverture existant; aucune implémentation de filtre v11 qualifiée"}


if __name__ == "__main__":
    check_manifest(Path(__file__).resolve().parent)
    print(json.dumps(run(),ensure_ascii=False,sort_keys=True,indent=2))
