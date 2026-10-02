"""Petite contre-verification autonome F3/F4, sans moteur HGP."""

from fractions import Fraction
from itertools import product
from pathlib import Path
import hashlib
import json
import sys


U = Fraction(1, 2**52)
T = 1 - U
C = 1 - Fraction(1, 2**40)
guards = 0


def require(condition, label):
    global guards
    guards += 1
    if not condition:
        raise RuntimeError(label)


def bounded(ratio, exposure, label):
    require(T**exposure <= ratio <= T**(-exposure), label)


def endpoints(exposure):
    return (T**exposure, T**(-exposure))


for ea, eb in product((0, 1, 3), repeat=2):
    for qa, qb, rounding in product(
        endpoints(ea), endpoints(eb), endpoints(1)
    ):
        bounded(qa * qb * rounding, ea + eb + 1, "produit")
        bounded(qa / qb * rounding, ea + eb + 2, "quotient direct")
        # Reciproque arrondi, puis produit arrondi.
        for reciprocal_rounding in endpoints(1):
            bounded(
                qa / qb * reciprocal_rounding * rounding,
                ea + eb + 2,
                "quotient par inverse",
            )
        # Somme de termes positifs, ou negatifs avec les memes ratios.
        bounded((2 * qa + 5 * qb) / 7 * rounding, max(ea, eb) + 1,
                "somme de meme signe")

for exposure in (0, 1, 3):
    for q, rounding in product(endpoints(exposure), endpoints(1)):
        bounded(q * q * rounding, 2 * exposure + 1, "reutilisation carre")

# Contraction a*b+c : avant l'unique arrondi, moyenne ponderee de deux ratios.
for qa, qb, qc, rounding in product(
    endpoints(1), endpoints(3), endpoints(2), endpoints(1)
):
    bounded((6 * qa * qb + 5 * qc) / 11 * rounding, 5,
            "FMA de termes de meme signe")

# Bernoulli : le calcul exact au seuil 4096 reste volontairement borne.
for exposure_sum in (1, 2, 4095, 4096):
    bernoulli = 1 - exposure_sum * U
    require(T**exposure_sum >= bernoulli >= C, "constante F4 Bernoulli")
require(C == 1 - 4096 * U, "egalite unite F4")

# Pour y>0 : c*q_y*delta/q_x <= 1 garantit l'ordre strict.
for ea, eb in product((0, 1, 3), repeat=2):
    for qa, qb, rounding in product(
        endpoints(ea), endpoints(eb), endpoints(1)
    ):
        require(C * qb * rounding / qa <= 1, "F4 positif")

# Contre-exemple de DOMAINE, pas un appel de filtre produit.
# c est exactement representable : ce cas ne depend d'aucune erreur d'arrondi.
cf = 1.0 - 2.0**-40
x = y = -1.0
rhs = cf * y
require(Fraction.from_float(cf) == C, "c binary64 exact")
require(Fraction.from_float(rhs) == -C, "produit negatif exact")
require(x < rhs and not x < y, "F4 faux si y negatif")

if "--manifest" in sys.argv:
    here = Path(__file__).resolve().parent
    for line in (here / "SHA256SUMS").read_text().splitlines():
        digest, name = line.split("  ", 1)
        require(hashlib.sha256((here / name).read_bytes()).hexdigest() == digest,
                "empreinte " + name)

print(json.dumps({
    "guards": guards,
    "f3_rules": "conditional interval bounds verified on endpoint models",
    "f4_positive": "Bernoulli S<=4096; positive comparison bound verified",
    "negative_domain_witness": {
        "x": x.hex(), "y": y.hex(), "c": cf.hex(), "fl_c_y": rhs.hex(),
        "filter_accepts_x_lt_y": x < rhs, "exact_x_lt_y": x < y,
    },
}, sort_keys=True, indent=2))
