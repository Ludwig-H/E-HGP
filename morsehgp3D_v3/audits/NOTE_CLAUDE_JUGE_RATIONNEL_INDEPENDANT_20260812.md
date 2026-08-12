# Note de Claude — le juge rationnel indépendant du census des trois arités

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note répond à l'objection méthodologique de
[`NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md`](NOTE_CLAUDE_MESURE_PORTE_REGULIERE_20260812.md)
section 1, telle que l'audit l'a corrigée : le juge interne du probe partage
les constructeurs de sphère et les prédicats du sujet, donc son vert ne reçoit
rien. Elle ne prononce aucune admission.

Snapshot de l'observation : `HEAD=8c00ab07695ef353e673ab73a778a6f260c87509`,
source du juge non suivie SHA-256
`a7812b3959a2a0752a7ac6413c26947eec2e763546c979a6695439786de7ac65`,
binaire Release SHA-256
`989150541dfb7a04241f5c8d9929f394eaffc1066083b916b719f6d6d25c9d75`.
Toute modification rend les exécutions ci-dessous historiques.

## 1. Ce qui a été construit

[`oracle/locality_census_judge.cpp`](../oracle/locality_census_judge.cpp)
recalcule le census exhaustif des trois arités avec **l'arithmétique de
l'oracle** :

| élément | sujet `certified_locality_probe.cpp` | juge `locality_census_judge.cpp` |
| --- | --- | --- |
| arithmétique | `__int128` | rationnels multiprécision |
| sphère q3 | Cramer explicite sur `Na`, `Nb`, `D` | élimination de Gauss, centre explicite |
| sphère q4 | déterminant InSphere développé | élimination de Gauss, centre explicite |
| intériorité | prédicats entiers dédiés par arité | `||z-c||^2` comparé à `R^2` |
| support positif | signes des numérateurs de Cramer | barycentriques rationnelles strictement positives |
| borne de balayage | Jung, `2 d^2 <= 3 diam(S)^2` | aucune : balayage complet |

Seul le générateur de nuages est partagé — c'est l'autorité de génération
unique voulue par le dossier, pas une primitive de décision. Le juge énumère
`Theta(n^4)` supports q4 et classe jusqu'à `n` points par support : son pire cas
en travail est `Theta(n^5)`, pas `O(n^4)`. Le refus au-delà de 400 points est un
cap syntaxique, pas une borne de travail praticable.

## 2. Accord

| campagne | q2 | q3 | q4 | statut durable |
| --- | ---: | ---: | ---: | --- |
| juge rationnel, `uniform n=50 K=4 graine=1` | 456 | 592 | 137 | relancé par CTest |
| génération locale, même campagne | 456 | 592 | 137 | relancée par CTest |
| juge rationnel, `uniform n=70 K=4 graine=1` | 681 | 884 | 202 | reproduction manuelle pincée, pas un CTest |
| génération locale, même campagne | 681 | 884 | 202 | relancée par CTest |

Ces valeurs sont gravées dans les portes CMake par `--expect-q2/q3/q4`. Le juge
rationnel est toutefois relancé seulement à `n=50`; aucune porte ne recalcule
les constantes `n=70` avec ce juge. La reproduction manuelle de `n=70` a été :

```bash
build/v3/mhgp3v_locality_census_judge --points=70 --family=uniform --seed=1 --kmin=4 --expect-q2=681 --expect-q3=884 --expect-q4=202
```

Elle rend le code 0 et imprime aussi `extra-shell q2/q3/q4=44/0/0`. Elle ne
transforme pas ces constantes en porte durable.

La porte annoncée pour `insphere-sign-flip` est rouge : le binaire rend bien le
code 4, mais une fixture d'orientation antérieure tue le mutant avant la
comparaison aux valeurs gravées. Le diagnostic attendu manque donc et cette
porte ne prouve pas la non-vacuité des constantes. La fenêtre de support 20
rend séparément le code 1 avec le diagnostic de désaccord.

## 3. Extra-shell, mesurée par le juge indépendant

Le juge publie séparément le nombre de records portant une extra-shell, c'est-à-dire
un point hors du support choisi exactement sur la sphère :

| campagne | q2 | q3 | q4 |
| --- | ---: | ---: | ---: |
| `uniform n=50 K=4` | 24 sur 456 | 1 sur 592 | 0 sur 137 |
| `uniform n=70 K=4` | 44 sur 681 | 0 sur 884 | 0 sur 202 |

Conformément à la correction de l'audit, ce compteur ne prouve pas la
non-unicité du support minimal : il établit seulement `U_B \ S` non vide pour
le support choisi. Les CTests et les options `--expect-*` ne comparent pas ces
trois compteurs d'extra-shell au sujet.

## 4. Ce que cela ne reçoit pas

Le juge est borné syntaxiquement à 400 points. Trois exécutions sur la machine
partagée ont pris entre `49,79 s` et `73,89 s` à `n=50`; ces secondes ne sont
pas un benchmark, mais elles montrent que le cap 400 n'est pas une enveloppe de
ressource réaliste. Il ne dit rien des fractions à `n=1 500` publiées dans la
note corrigée, ni d'aucune taille contractuelle. Il ne compare que des
cardinalités par arité : ni identités de supports, ni listes d'intérieurs, ni
shell complet, ni `BallKey`. Fermer ces comparaisons conditionne toute réception
de la route sparse. Son parseur accepte en outre un suffixe non numérique :
`--points=50junk` est interprété comme 50. La CLI et son préflight ne sont donc
pas reçus.

GCP non utilisé pour cette note.
