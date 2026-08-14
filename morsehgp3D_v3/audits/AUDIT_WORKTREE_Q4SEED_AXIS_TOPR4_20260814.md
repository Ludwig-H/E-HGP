# Contre-audit worktree — `Q4SeedAxisTopR4`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot et périmètre

Le `HEAD` relu est `504f334bdfad90581b58086ced8eb6a35cf438d8`.
Les deux fichiers sont apparus non suivis pendant l'audit :

| fichier | SHA-256 relu |
| --- | --- |
| `prototype/axis_top8.hpp` | `90360f41614f0f37ffa7fe80c5ee504fa8c84e60303d988b5cfbb243197be824` |
| `prototype/axis_top8_probe.cpp` | `4c952fd29c2188f62420e3870a2cf46554293938edbb893bd0a5bcb31e10307b` |

L'auditeur n'a modifié aucun fichier de `prototype/`, `oracle/`, `tests/`,
`receipts/`, `docs/` ou `gcp-migration/`. La compilation ci-dessous a produit
uniquement `/tmp/mhgp3v_axis_top8_probe_audit`.

## Verdict court

Le noyau ponctuel confirme utilement la borne top-8 du profil fixe, mais la
porte ne reçoit encore ni une source q4 autonome, ni le payload exact
`I_B/U_B`, ni le certificat de mort par gaps, ni l'exact-once par provenance
primaire. Son domaine CLI annoncé est en outre faux : le sujet reste codé en
dur à huit alors que `--seuil` accepte `0..15`.

Le nom contractuel est `Q4SeedAxisTopR4`, interne à `Lane4`. Le triple d'entrée
est un `Q4Seed3` construit par `Lane4`; ce n'est ni un support q3, ni un record
lu depuis `Lane3`. Les commentaires `face`, `carrier q3` et le lien vers
`AUDIT_DEBLOCAGE_SOURCE_SHALLOW_SANS_DELAUNAY_20260814.md`, désormais supprimé,
doivent être corrigés par Claude sans que l'auditeur touche au code.

## Rejeu indépendant

Compilation stricte :

```text
g++ -std=c++20 -O2 -Wall -Wextra -Werror -pthread \
  -I morsehgp3D_v2/include -I morsehgp3D_v3/prototype \
  morsehgp3D_v3/prototype/axis_top8_probe.cpp \
  -o /tmp/mhgp3v_axis_top8_probe_audit
```

Les quatre fixtures nominales passent :

```text
fixture16       q4=16, candidats=16, groupes=16, manquants=0, census_faux=0
fixture_mort16  profondeur_min_Jf=8, retrait_un_min=7
Jung tendu      deux orientations, candidats=1, manquants=0
T2              obtus mort, aigu T2>0
```

Deux sweeps à `n=60`, `seuil=7`, passent sans manque, borne cassée, faux compte
ni débordement :

| famille | graines q4 aiguës owner | shallow | candidats | manquants |
| --- | ---: | ---: | ---: | ---: |
| `uniform` | 16 133 | 5 867 | 51 013 | 0 |
| `eight_clusters` | 15 856 | 3 564 | 57 253 | 0 |

Ces résultats falsifient des erreurs du noyau fixe ; ils ne qualifient aucune
complexité, aucune source WSPD et aucun SLO.

## P0 — le domaine `--seuil` est incomplet

`main` accepte `0<=seuil<=15`, mais `select_top8` ferme toujours à huit
permanents et conserve toujours `k=8-p`. Le juge, lui, emploie la valeur CLI.
Le différentiel reproduit immédiatement la perte :

```text
uniform, n=60, seuil=8         shallow=7450, manquants=440, rc=1
eight_clusters, n=60, seuil=8 shallow=5024, manquants=428, rc=1
```

Deux réparations seulement sont recevables : refuser toute valeur autre que
sept avant calcul, ou paramétrer le sujet par `r4=seuil+1=smax-3`, avec
`p>=r4`, `k=r4-p` et une borne `2*(r4-p)`. Le théorème consolidé retient la
seconde formulation ; le profil courant redonne `r4=8` et seize racines.

## P0 — le payload d'identités n'est ni produit ni jugé

`Selection` conserve seulement le **compte** `p` des permanents et un booléen
`shell_persistant`. Il ne conserve ni les `PointId` permanents, ni les IDs du
shell persistant. Le replay du probe initialise seulement `dk=sel.p` et compare
un cardinal au cardinal exhaustif. Il ne construit ni ne compare les listes
triées `I_B/U_B`.

De plus, `Verdict.shell_extra` reste toujours zéro : `juge_tetra` ne classe
jamais `insphere_j==0`. Les sorties `extra_shell=0` ne prouvent donc rien sur
`RelevantGP`, les groupes d'égalité ou le shell complet. Un shell persistant
détecté par le sujet n'entraîne actuellement aucun refus.

Réception minimale : transporter les vrais IDs permanents, range-reporter tout
groupe de racine égal et le shell persistant, reconstruire les listes complètes
`I_B/U_B`, puis les comparer à un juge `insphere_j<0/==0/>0`. Hors
`RelevantGP`, une égalité non admise rend `unsupported_degeneracy`, jamais un
succès par cardinal.

## P0 — la mort par gaps reste calculée par l'oracle

`select_top8` ne publie `face_morte` que pour `T2<=0` ou `p>=8`. Dans
`fixture_mort16`, il retourne encore seize candidats. Le champ `morte_gap` est
posé ensuite parce que le probe exhaustif ne trouve aucun q4 shallow ; ce n'est
pas le certificat de gaps constant annoncé.

Il faut une sortie typée `DEAD_GAP` accompagnée de son reçu de seuils, comparée
à la profondeur minimale exhaustive et à ses mutants de frontière. Tant
qu'elle manque, le noyau réduit des candidats mais ne prouve pas la fermeture
pré-apex promise.

## P1 — provenance q4 et exact-once non testés

Le juge vérifie positivité et owner, puis compte par `Q4Seed3`. Il ne décide pas
le primary entre les deux préfixes aigus d'un même tétraèdre et ne compare pas
un ensemble global de `SupportKey` exact-once. La fixture seize n'exerce qu'un
préfixe choisi manuellement ; son libellé `carrier_primaire=0` n'est pas une
décision du sujet.

La porte suivante doit construire les deux `Q4Seed3` possibles dans `Lane4`,
appliquer le plus petit vrai `PointId` aigu, puis comparer le multiensemble
global au brute force : zéro manque, zéro doublon, mêmes owners et mêmes
`SupportKey`.

## P1 — portée physique

La sélection actuelle est quadratique dans le nombre de sites par préfixe :
chaque racine rescane toutes les autres. La campagne exhaustive ajoute les
boucles sur paires, troisièmes rôles et quatrièmes rôles. Elle est un oracle
borné, pas la généralisation WSPD ni la recherche best-first sur Morton requise
pour 50 000 points. Le domaine `n<=400` n'est pas à lui seul un cap
d'opérations ou de temps.

Avant tout raccord G4, publier séparément `Q4Seed3Block`, visites de nœuds,
comparaisons larges, groupes d'égalité, morts permanents/gaps, racines retenues,
octets et HWM sur `1500/3000/6000`, sans produit `Lane3 x Lane4` et sans
`CellPair/Sym2`.

GCP non utilisé.
