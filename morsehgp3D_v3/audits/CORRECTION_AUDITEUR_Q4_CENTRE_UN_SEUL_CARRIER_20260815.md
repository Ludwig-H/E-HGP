# Correction de l'auditeur — un centre q4 peut n'avoir qu'un seul seed aigu

Date : 15 août 2026 UTC.

Documents concernés :

- [`NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md`](NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md), § 7.2 et § 8 ;
- `PROPOSITION.md`, route `Q4SeedAxisTopR4` ;
- [`CONTRE_AUDIT_POSITIF_Q4_PROPOSITIONS_EB42B574_20260815.md`](CONTRE_AUDIT_POSITIF_Q4_PROPOSITIONS_EB42B574_20260815.md), dont la borne prudente `2*r4` reste correcte.

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=quantized_u16_input_only`, `mode=counterexample_exact`,
`public_status=not_claimed`.

> [!CAUTION]
> La phrase
>
> ```text
> chaque centre géométrique compte au moins deux incidences,
> donc centres_shallow <= r4*m_e
> ```
>
> est fausse lorsque `m_e` compte les `Q4Seed3` strictement aigus, comme le fait
> la route proposée. Le lemme de complétude promet **au moins une** face aiguë
> incidente à l'arête owner, pas deux.
>
> Un tétraèdre entier u16, strictement bien centré, possède ci-dessous un owner
> unique et exactement un seul carrier aigu incident. Son centre est donc
> proposé une seule fois par la source aiguë.
>
> La route n'est pas invalidée. La correction est un facteur deux :
>
> ```text
> N4_event <= R4_bundle <= 2*r4*C4_carrier,
> ```
>
> et non `N4_event <= r4*C4_carrier` sans hypothèse supplémentaire.

## 1. Contre-exemple entier exact

Prendre :

```text
p0 = (6,2,5),
p1 = (0,3,3),
p2 = (1,4,6),
p3 = (5,3,1).
```

Les six longueurs carrées sont :

| arête | longueur carrée |
| --- | ---: |
| `p0p1` | `41` |
| `p0p2` | `30` |
| `p0p3` | `18` |
| `p1p2` | `11` |
| `p1p3` | `29` |
| `p2p3` | `42` |

L'arête

```text
e = p2p3
```

est donc l'owner unique, sans tie d'`EdgeKey`.

## 2. Le tétraèdre est strictement bien centré

Son circumcentre est

```text
c = (83,81,97)/26,
R^2 = 7259/676.
```

Ses coordonnées barycentriques dans l'ordre `(p0,p1,p2,p3)` sont

```text
lambda = (70,49,109,110)/338.
```

Elles sont toutes strictement positives et leur somme vaut un. Le circumcentre
appartient donc à l'intérieur strict du tétraèdre : le support q4 est positif.

Les quatre points sont affinement indépendants, puisque ces barycentriques et
le circumcentre sont définis par un système tétraédrique non singulier.

## 3. Une seule face incidente à l'owner est aiguë

Pour la face `p2 p3 p0`, le produit scalaire à l'angle opposé à l'owner vaut

```text
(p2-p0) dot (p3-p0) = 3 > 0.
```

Comme `p2p3` est la plus longue arête, cette inégalité rend toute la face
strictement aiguë.

Pour la seconde face incidente :

```text
(p2-p1) dot (p3-p1) = -1 < 0.
```

La face `p2 p3 p1` est obtuse en `p1`.

Ainsi, autour de l'arête owner `p2p3` :

```text
carrier aigu : p0,
carrier non aigu : p1.
```

La source `Q4Seed3` ne crée que le seed

```text
(p2,p3,p0),
```

et `p1` intervient seulement comme apex. Le centre q4 apparaît sur la ligne du
seed `p0`, mais la ligne associée à `p1` n'est pas une ligne productrice de la
source aiguë. Il n'existe donc qu'une incidence proposée.

## 4. Où la preuve précédente change de sens

Géométriquement, tout centre de sphère `abxy` est bien l'intersection des deux
lignes `L_x` et `L_y` du plan médiateur. Cela donne deux **incidences
algébriques** dans l'arrangement complet.

Mais la route ne parcourt pas l'arrangement complet. Elle parcourt seulement
les lignes dont le troisième sommet forme un carrier aigu possédé. Lorsque `y`
n'est pas aigu :

```text
L_y existe mathématiquement,
mais aucun Q4Seed3(y) n'est schedulé.
```

On ne peut donc pas diviser le nombre de groupes produits par deux en invoquant
une incidence qui n'est précisément pas produite.

Deux choix seraient cohérents :

1. conserver la source aiguë, donc recevoir seulement la borne par `2*r4` ;
2. parcourir toutes les lignes, aiguës ou non, afin de retrouver deux incidences,
   mais perdre le prune qui justifie l'architecture et retraiter ensuite le
   primary.

Le premier choix est nettement préférable.

## 5. Bornes corrigées

Pour chaque seed aigu exact, la sélection retourne au plus

```text
2(r4-p) <= 2r4
```

groupes de roots. Par sommation :

```text
R4_bundle <= 2*r4*C4_carrier.
```

Chaque centre produit possède au moins une incidence proposée, donc seulement :

```text
N4_event <= R4_bundle
         <= 2*r4*C4_carrier.
```

Sous `RelevantGP`, si chaque root retenu contient un seul apex admissible et si
le primary exact-once est appliqué, on obtient également :

```text
M4_apex <= R4_bundle,
W4_positive <= M4_apex.
```

Hors `RelevantGP`, un bundle égal peut porter plusieurs IDs ; aucune borne sur
`M4_apex`, les `SupportKey` ou les octets ne découle du seul nombre de groupes.

La phrase de § 8 doit donc devenir :

```text
Lane4 : O(B4 + M4 + T4 + R4_bundle + output4),
R4_bundle <= 2*r4*C4_carrier,
```

sans division supplémentaire par deux.

## 6. Gate permanente

Ajouter une fixture `one_acute_incident_face_q4` avec les quatre points ci-dessus.
Elle exige :

```text
owner = EdgeKey(p2,p3),
acute_incident_faces = 1,
Q4Seed3 = 1,
root incidence du vrai centre = 1,
W4_positive = 1,
H4_rank = 1,
exact-once sous les 24 permutations.
```

Mutant causal :

```text
q4-exige-deux-carriers-aigus
```

Il doit perdre ce support. Un second mutant qui divise systématiquement
`R4_bundle` par deux doit violer le plafond observé sur cette fixture ou sur une
répétition disjointe de celle-ci.

## 7. Statut

La complétude par au moins un carrier aigu, la source shallow et la borne
`2*r4` par seed restent reçues comme route mathématique. Seule la phrase
« chaque centre a deux incidences proposées » est rétractée.

C'est une correction modeste en complexité et importante en preuve : un facteur
deux inventé n'est pas grave pour le GPU ; il l'est davantage lorsqu'il sert de
pont entre deux unités que l'algorithme a justement choisi de ne plus toutes
matérialiser.
