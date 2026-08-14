# Contre-audit worktree — `Q4SeedAxisTopR4`

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Snapshot et périmètre

Le snapshot a d'abord été relu non suivi au parent
`504f334bdfad90581b58086ced8eb6a35cf438d8`, puis Claude l'a commis sans changer
ces deux octets au `HEAD=069acf7c26312b6146cb8d1ce890cb5f4d681cac` :

| fichier | SHA-256 relu |
| --- | --- |
| `prototype/axis_top8.hpp` | `90360f41614f0f37ffa7fe80c5ee504fa8c84e60303d988b5cfbb243197be824` |
| `prototype/axis_top8_probe.cpp` | `4c952fd29c2188f62420e3870a2cf46554293938edbb893bd0a5bcb31e10307b` |

Le correctif paramétrique est ensuite commis au
`HEAD=840a2e28679aa3e5e3d8ec706daa680a52ac1bde` :

| fichier | SHA-256 relu au `840a2e2` |
| --- | --- |
| `prototype/axis_top8.hpp` | `78282847f4395cada2d2b572b2ad8d81f40f76dcefea8e5e89576fa19d4c1ddb` |
| `prototype/axis_top8_probe.cpp` | `af9c87adb8250a3c3897ed1ca1716bc7decef7bb72e1c7c908cf1292b3c18966` |

L'auditeur n'a modifié aucun fichier de `prototype/`, `oracle/`, `tests/`,
`receipts/`, `docs/` ou `gcp-migration/`. La compilation ci-dessous a produit
uniquement `/tmp/mhgp3v_axis_top8_probe_audit`.

## Verdict court

Le noyau ponctuel confirme utilement la borne extrémale et le commit `840a2e2`
répare son ancien codage en dur à huit. La porte ne reçoit toutefois encore ni
une source q4 autonome, ni le payload exact `I_B/U_B`, ni le certificat de mort
par gaps, ni l'exact-once par provenance primaire. Les groupes d'égalité, les
caps et plusieurs options CLI restent également sans contrat fail-closed.

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

Après configuration CMake et build Release, les 23 CTests
`^mhgp3v_axis_top8_` passent en `10,23 s`. Cette couverture ne change aucun des
P0 ci-dessous : aucun test ne demande `--seuil=8`, ne compare les listes d'IDs
du census, ne classe le shell complet ou ne décide le primary global.

## Historique P0 — domaine `--seuil`, réparé au `840a2e2`

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

### Réparation commise, noms encore à aligner

Claude a ensuite paramétré `select_top8` par `mort=seuil+1` au commit
`840a2e2`. Les SHA-256 relus sont `78282847f4395cada2d2b572b2ad8d81f40f76dcefea8e5e89576fa19d4c1ddb`
pour le header et `af9c87adb8250a3c3897ed1ca1716bc7decef7bb72e1c7c908cf1292b3c18966`
pour le probe. Après rebuild :

```text
uniform, n=60, seuil=8         shallow=7450, manquants=0, cand_max=17, rc=0
eight_clusters, n=60, seuil=8 shallow=5024, manquants=0, cand_max=18, rc=0
uniform, n=60, seuil=15       shallow=21940, manquants=0, cand_max=28, rc=0
```

Le défaut de domaine est donc réparé au `HEAD`. Les noms et diagnostics restent
toutefois codés top-8
(`select_top8`, `morte_perm8`, messages `16-2p`) et les fixtures fixes ignorent
la valeur CLI. Aucun CTest ne fixe encore une régression avec un seuil différent
de sept ; ils doivent être alignés avant une ABI paramétrique.

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

Le replay exact ne demande aucun second census global. Noter `P` les IDs
permanents, `H` le shell persistant, `E` tous les IDs des groupes entrants
retenus et `S` ceux des groupes sortants retenus. Pour l'apex `y` de racine
`alpha_y`, il faut rendre exactement
`I_y=P union {z in E:alpha_z<alpha_y} union {z in S:alpha_z>alpha_y}` et
`U_y={a,b,x} union H union {z in E union S:alpha_z=alpha_y}`. Sur une branche
vive, `|P|<mort`; un intérieur omis hors des extrêmes impliquerait déjà une
profondeur au moins `mort`. En revanche `H` et un groupe égal peuvent être de
masse arbitraire : RLE/continuation ou refus typé sont obligatoires, jamais un
booléen ou un tableau partiel.

## P0 — la mort par gaps reste calculée par l'oracle

`select_top8` ne publie `face_morte` que pour `T2<=0` ou `p>=8`. Dans
`fixture_mort16`, il retourne encore seize candidats. Le champ `morte_gap` est
posé ensuite parce que le probe exhaustif ne trouve aucun q4 shallow ; ce n'est
pas le certificat de gaps constant annoncé.

Il faut une sortie typée `DEAD_GAP` accompagnée de son reçu de seuils, comparée
à la profondeur minimale exhaustive et à ses mutants de frontière. Elle se
calcule directement à partir des mêmes extrêmes : avec `k=mort-p`, grouper les
`k` racines entrantes minimales et les `k` sortantes maximales, puis évaluer
chaque intervalle de leur ordre fusionné **et chaque racine elle-même**. À une
égalité, le groupe est shell et ne crédite aucun intérieur. Le prédicat tronqué
est exact : `depth(tau)>=mort` si et seulement si
`p+min(k,N_in(alpha<tau))+min(k,N_out(alpha>tau))>=mort`. Si tous les morceaux
passent, ce replay `O(k log k)` rend `DEAD_GAP`; sinon il rend les intervalles
shallow. Tant qu'il manque, le noyau réduit des candidats mais ne prouve pas la
fermeture pré-apex promise.

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

## P1 — groupes d'égalité et débordement ne sont pas reçus

La borne `2*(mort-p)` porte sur les **groupes de racines**, pas sur leur nombre
d'IDs. Une dégénérescence peut donc contenir arbitrairement plus de candidats.
Le tableau fixe de 64 IDs par côté peut devenir partiel ; le probe comptabilise
alors `debordement`, mais il a déjà laissé circuler la sélection tronquée et
rend un plancher générique. Une fixture de 65 IDs égaux manque.

La consommation doit être impossible après overflow. Le fate attendu est une
continuation `PENDING_CAP` portant le groupe complet, ou
`unsupported_degeneracy` si le profil le permet ; aucun `SupportKey` ni verdict
OK ne peut provenir d'un préfixe de groupe. Ajouter les mutants `drop_equal_id`,
`drop_persistent_shell` et `continue_after_overflow`.

## P1 — le contrat CLI et plusieurs mutants sont poreux

Les appels suivants réussissent alors que leurs options sont ignorées ou ne
mordent pas le mode choisi :

```text
--fixture-16 --seuil=8 --points=999999 --coord=999999
--fixture-t2-positif --inject=a8-cap15
--sweep --family=two_lines --min-faces=-1 --min-shallow=-1
```

Le parseur `atoll` accepte aussi suffixes et overflows ; `coord` n'est pas
prévalidé dans le domaine u16, les modes multiples peuvent s'écraser et
`threads` n'a pas de borne contractuelle. Il faut un parse entier strict, une
table d'options admissibles par mode, `min_*> =0`, `1<=threads<=cap`, le domaine
coordonné u16 et le cardinal réel du générateur.

La campagne actuelle ne distingue pas non plus réellement tous les mutants :
`kSigneBInverse` et `kAbsAvantTri` produisent les mêmes métriques sur le replay
uniforme, `kBoutsOuverts` ne couvre qu'une frontière tandis que l'autre est
portée par `kPermanenceLarge`, et `kKFixe7` suit désormais `mort-1`. Ces noms ne
constituent donc pas trois preuves de mutation indépendantes.

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

Le header conserve enfin un lien vers
`AUDIT_DEBLOCAGE_SOURCE_SHALLOW_SANS_DELAUNAY_20260814.md`, brouillon incorrect
qui a été supprimé. Claude doit le remplacer par l'autorité active
`NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`; l'auditeur ne modifie pas le
code.

GCP non utilisé.
