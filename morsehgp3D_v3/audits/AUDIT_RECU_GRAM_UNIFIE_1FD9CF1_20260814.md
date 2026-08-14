# Reçu borné du microkernel Gram q4

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pin documentaire : `HEAD=1fd9cf1d40dda12bcad569fab1227657551941cb`.

Ce reçu vérifie seulement deux identités algébriques du microkernel Gram q4 :
`Delta=O^2` et `Phi=O*J`. Il ne reçoit ni enclosure de boîtes support, ni
positivité, ni source WST, ni census, ni performance. La preuve algébrique reste
l'autorité ; ce différentiel déterministe est une porte de falsification bornée.

Environnement : Python `3.12.1`, seed `20260814`, coordonnées entières uniformes
dans `[-10,10]`, bornes incluses. La cible est exactement 10 000 q4 non
dégénérés.

```bash
python3 - <<'PY'
import hashlib
import random

SEED = 20260814
LOW = -10
HIGH = 10
TARGET = 10_000
rng = random.Random(SEED)

def dot(a, b):
    return sum(x * y for x, y in zip(a, b))

def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))

def det2(a):
    return a[0][0] * a[1][1] - a[0][1] * a[1][0]

def det3(a):
    return (
        a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
        - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
        + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0])
    )

def det4(a):
    total = 0
    for j in range(4):
        minor = [row[:j] + row[j + 1:] for row in a[1:]]
        total += (-1) ** j * a[0][j] * det3(minor)
    return total

def adj3(a):
    return [
        [
            (-1) ** (i + j)
            * det2(
                [
                    [a[row][col] for col in range(3) if col != i]
                    for row in range(3)
                    if row != j
                ]
            )
            for j in range(3)
        ]
        for i in range(3)
    ]

def matvec(a, x):
    return [dot(row, x) for row in a]

accepted = 0
draws = 0
digest = hashlib.sha256()

while accepted < TARGET:
    draws += 1
    anchor = tuple(rng.randrange(LOW, HIGH + 1) for _ in range(3))
    points = [
        tuple(rng.randrange(LOW, HIGH + 1) for _ in range(3))
        for _ in range(3)
    ]
    witness = tuple(rng.randrange(LOW, HIGH + 1) for _ in range(3))
    vectors = [sub(point, anchor) for point in points]
    matrix = list(map(list, zip(*vectors)))
    orient = det3(matrix)
    if orient == 0:
        continue

    gram = [
        [dot(vectors[i], vectors[j]) for j in range(3)]
        for i in range(3)
    ]
    delta = det3(gram)
    ell = [gram[i][i] for i in range(3)]
    r = matvec(adj3(gram), ell)
    t = matvec(matrix, r)
    s = sub(witness, anchor)
    phi = delta * dot(s, s) - dot(s, t)

    rows = []
    for point in [anchor] + points:
        w = sub(point, witness)
        rows.append([*w, dot(w, w)])
    insphere = det4(rows)

    if delta != orient * orient or phi != orient * insphere:
        raise SystemExit(
            f"FAIL accepted={accepted} draws={draws} "
            f"delta={delta} orient={orient} phi={phi} J={insphere} "
            f"a={anchor} points={points} z={witness}"
        )

    digest.update(repr((anchor, points, witness)).encode("ascii"))
    accepted += 1

print(
    f"PASS seed={SEED} range=[{LOW},{HIGH}] "
    f"accepted={accepted} draws={draws}"
)
print(f"fixture_sha256={digest.hexdigest()}")
PY
```

Sortie attendue et observée :

```text
PASS seed=20260814 range=[-10,10] accepted=10000 draws=10029
fixture_sha256=47656f6c64c4f18f6533f1ef59a578452606f3e5297752fea7d12107b0877140
```

GCP non utilisé.
