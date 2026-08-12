# Audit du probe de localité `778372f`

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

| objet | SHA-256 |
| --- | --- |
| `CMakeLists.txt` | `ed5d00aefd1e962098e3a4d74ef3c0974eca22852d6a5ac371f9a3ec8a233ebf` |
| `prototype/certified_locality_probe.cpp` | `778372fc22d018c0bbafd3f80e23a06476602004c787969a3fb885fcecb01dda` |
| binaire Release | `68d914bbfbfcbdc61523102cba48959cc968ad2f27cca18526e2ae7510db15e3` |

Ce snapshot est un diff concurrent par rapport au `HEAD` suivi
`df9dc7768156cfb24cf8e011f55f215115b22ca1`. L'audit n'a modifié aucun code.

## Verdict

Le lemme d'inversion et le seuil par cellule sont des propositions
mathématiques légitimes. Le probe raccordé n'est cependant pas reçu : deux
erreurs P0 subsistent dans les décisions/juges, son oracle q2 compare seulement
une cardinalité et le « census » n'est pas le census fermé du pipeline. Les
`20/20` CTests verts ne justifient donc ni source exacte, ni complexité, ni
port CUDA, ni session G4.

## Construction et tests observés

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel --target mhgp3v_certified_locality_probe
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_locality_'
```

Résultat : build vert, puis `20/20` verts en `180,31 s`. Ce temps n'est pas une
mesure de performance. Le test `mhgp3v_locality_juge_census_borne`, annoncé
comme un refus « avant tout calcul », consomme à lui seul `118,61 s` : la garde
`n>4000` est exécutée après le calcul directionnel. Le code de sortie attendu
est exact, mais la propriété de préflight annoncée par CMake est fausse.

Une contre-commande hors matrice passe également :

```bash
build/v3/mhgp3v_certified_locality_probe --mode=directional --points=120 \
  --family=terrain --grid=4 --seed=3 --kmin=10 --max-neighbours=10 \
  --threads=2 --judge-census=1
```

Elle rend zéro et compare `1840` à `1840`, mais déclenche `120` balayages
complets. Ce vert montre seulement que le fallback masque le cas sur cette
entrée; il ne reçoit ni la fermeture top-M ni une ordonnance scalable.

## P0 — fermeture top-M inversée

Soit `d_M^2` la dernière distance réellement lue et `r_c^2` le seuil d'une
cellule. Des voisins omis peuvent encore être candidats lorsque
`d_M^2<=r_c^2`, égalité comprise. C'est donc ce cas qui exige le voisin suivant
certifié, un range-report résiduel ou un fallback exact.

Le snapshot active au contraire `beyond_window` lorsque
`r_c^2<d_M^2`. Il rescane ainsi des cellules déjà fermées et peut ne pas
rescanner le cas qui ne l'est pas. Son compteur `unbounded` ne protège pas la
décision : sans full scan, chaque cible parcourue provient du top-M et vérifie
par construction `d_y^2<=d_M^2`; avec full scan, le test est désactivé.

Une fixture déterministe doit placer au moins dix témoins directionnellement
répartis à une distance commune, fermer toutes les cellules avec
`r_c^2>=d_M^2`, puis ajouter une cible possédée au-delà du top-M. Le juge doit
comparer les identités des paires, pas leur seul nombre.

## P0 — signe InSphere q4

Le juge q4 conserve un point lorsque le déterminant InSphere a le **même**
signe que l'orientation. Avec la convention implémentée, l'intérieur porte le
signe opposé.

La fixture entière u16
`a=(11,11,11)`, `b=(11,9,9)`, `c=(9,11,9)`, `d=(9,9,11)` et
`z=(10,10,10)` donne orientation `-16` et déterminant `+48`. Le point est le
centre et doit être intérieur; le juge le rejette. La fixture randomisée des
trois arités peut rester verte parce qu'elle recalcule ensuite la localité sur
une mauvaise liste d'intérieurs. Ce défaut doit être gravé avant d'appeler le
juge indépendant.

## Portée réelle du juge q2

Le `judge-census` exhaustif compte les paires diamétrales ayant moins de dix
intérieurs et compare ce scalaire à `emitted`. Il ne compare ni identités de
paires, ni listes d'intérieurs, ni contacts de coquille, ni rang fermé, ni
`BallKey`. Une omission et un doublon peuvent se compenser dans ce total.

Même si toutes les identités étaient égales, cette cardinalité n'est pas le
census fermé requis : le parcours s'arrête à dix intérieurs et ne produit
aucun record aval. Les chaînes « census q2 EXACT » du sujet et les commentaires
CMake doivent donc être lues comme un compte q2 borné jusqu'à réception d'un
payload complet.

Le vecteur de direction nul reste un autre trou de domaine : `locate_cells`
rend une liste vide et omet une paire de `PointId` colocalisés. Le profil actuel
génère des positions distinctes, mais une API générique doit refuser les
doublons avant calcul ou les conserver fail-open selon une politique reçue.

## Complexité et causalité

Le profil full-sphere répète des couvertures et n'a pas un coût `O(sum M*)` tel
qu'annoncé. Le directionnel effectue un top-M par ancre, puis jusqu'à
`8m^2Mn` tests cellule--point, avant la classification ponctuelle. Ses
compteurs n'incluent pas toutes les visites LBVH, opérations de heap,
localisations, allocations ou octets high-water. Un fallback complet peut
ajouter un scan par paire.

Le facteur `384` commenté n'est pas un rapport à la vraie région diamétrale.
Sous un modèle isotrope homogène, cette région a déjà le volume de la boule de
rayon `D/2`; une chambre congruente sur 48 en conserve un quarante-huitième.
Le rapport volumique idéalisé est 48. Ni 48 ni 384 ne prouvent la cause des
pentes réelles, particulièrement pour le dual-tree toutes directions.

## Déblocage d'implémentation

1. Déplacer toutes les gardes de domaine et de juge avant génération, LBVH et
   calcul directionnel.
2. Fermer le top-M avec une borne stricte du prochain voisin omis, ou conserver
   un résidu cible par cellule. Ne jamais inférer cette borne de `d_M` seul.
3. Remplir les seuils `10/9/8` par best-first `LBVH x cell_mask`, avec le
   minorant rationnel documenté dans
   [`AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md`](AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md).
4. Le reçu de cellule porte identités distinctes des témoins, fractions,
   cellule, plage cible, digest nuage/LBVH, epoch et bornes de nœuds; le juge
   recalcule les trois sommets et le prédicat ponctuel.
5. À petit `n`, comparer la liste canonique de paires et son contenu fermé,
   jamais uniquement le total. Ajouter tangences, ex æquo top-M, frontières de
   cellule, colocalisés et la fixture q4 ci-dessus.
6. Mesurer séparément `sum candidates`, visites LBVH, tests cellule--nœud,
   splits, fallback, tests ponctuels et mémoire aux quatre familles et aux
   trois tailles. La localité complète le dual adaptatif; elle ne l'a pas
   réfuté.

Le chemin ne matérialise encore aucune mosaïque, cellule de Delaunay, coface,
incidence ou Gamma global. L'énumérateur inversé q3/q4 demeure une réouverture
conditionnelle, pas une dépendance de ce probe q2.

GCP non utilisé.
