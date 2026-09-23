# Contre-audit B — paires de gardes avant le cœur q3/q4

23 septembre 2026. Relecture indépendante du
[reçu de A](paired_guards_precore_20260923/README.md), commit
`4dd6f8a1e`. Audit du **certificat**, de la provenance et du sens des
mesures ; aucun port moteur, build G4 ni chrono nouveau.

## La preuve positive tient

Pour une arête `ab` propriétaire comme plus longue arête de ses futurs
supports, la variance des poids barycentriques positifs borne le
déplacement `t` du centre par `|t|²≤D/12` en q3 et `≤D/8` en q4,
où `D=|b−a|²`. La marge d'un témoin réel `g` est
`P_g(t)=D/4−|g−m|²+2(g−m)·t`. Pour deux témoins distincts `g,h`,
le minimum de `P_g+P_h` vaut `H/4−√(X/12)` ou `H/4−√(X/8)` selon la
voie. Ainsi `H>0` avec `3H²>4X` (q3) ou `H²>2X` (q4) garantit
**strictement** au moins un intrus réel de cette paire à tout centre
admissible. Quatre ou trois paires **disjointes** ferment respectivement
q3 ou q4 à K5. Les égalités ne créditent rien. La lecture algébrique
ne trouve pas d'hypothèse géométrique manquante ; le propriétaire de
plus longue arête et les sites distincts sont essentiels. Le bornage
u18 annoncé tient en i128 après promotion avant produit.

Les huit SHA de la capture passent. J'ai rejoué `verify.py` normal et
`-O` sur les deux JSON : **60 arêtes** et **299 / 383** contrôles
stricts de paires, fermetures `B=4/8/16` **20/22/27** puis
**32/34/40**. Les associations et les `F` sont revérifiables contre
les coordonnées u18 épinglées ; le replay local des 120 arêtes
retrouve `ΣF=520 631` et les deux JSON octet pour octet. Le mode
`--sample` recalcule palettes, échecs et preuves sur ces arêtes mais
reprend leurs IDs/masques du JSON ; la provenance du réservoir exige
les huit traces S2 et leurs SHA. Celles conservées localement ont
resélectionné les mêmes 60 arêtes par graine. Ces relectures LIVE
n'ajoutent pas un reçu GPU ni une nouvelle trame. Les huit parties de
trace et le binaire de fixture ne sont pas versionnés dans ce reçu
compact : les SHA les identifient, mais le rejeu autonome de la
**sélection** dépend de leur régénération ou de leur conservation ;
le rejeu des **preuves sur les arêtes épinglées** reste autonome avec
les entrées v8 régénérables.

## Ce que les fermetures ne disent pas

À `B=16`, la composition des deux réservoirs diffère :

| Graine | q3 seul | q4 seul | q3 et q4 |
| --- | ---: | ---: | ---: |
| 230923 | 0 | 17/29 fermées | 10/31 fermées |
| 230924 | 3/3 fermées | 30/38 fermées | 7/19 fermées |

Les deux ensembles de 60 arêtes sont disjoints, mais viennent **de la
même trame brute 08/000000** et sont stratifiés par `F`, pas par masque.
Le passage 27→40/60 reflète aussi davantage de q4-seul dans la seconde
graine ; ce ne sont ni deux répétitions du même cas ni une estimation
du taux de fermeture du flux entier. `F` fermable 111 883/260 032 et
168 845/260 599 n'est qu'un potentiel sur ces échantillons, pas du
travail effectivement économisé. Aucun résultat K10, sans-sol,
multi-séquence, s10/s12 ou G4 n'en découle.

Surtout, le shadow sélectionne ses candidats en balayant **123 389
sites par arête**, soit **7 403 340 lectures** pour 60 arêtes contre
260 032 formes de cœur dans la première graine (×28,47) ; son
appariement vient **en plus**. Porté tel quel, ce premier stade
restaurerait un terme `Ω(nE)` pour `E` arêtes tentées. De plus, les
60 arêtes ont été sélectionnées parce que leur `F≥1000` était **déjà
connu par une trace après construction du cœur**. Pour l'utiliser
réellement *avant* le cœur, il faut un déclencheur bon marché
disponible avant `Q34EdgeCover::make_diametral`, pas un second scan
qui mesure `F` ; après le cœur, la preuve peut encore épargner le
chargement des formes et l'aval, mais pas la construction déjà payée.

La prochaine porte pertinente est un **shadow** de quatre recherches
indexées et bornées par arête, avec au plus `B` sites par quadrant,
IDs et contacts exacts. Une palette partielle peut fermer une voie
positivement ; si le budget s'épuise sans preuve, repli inchangé.
Mesurer sur **toutes** les arêtes S2 le coût des tentatives et du
déclencheur, visites de nœuds/feuilles, `F` et covers réellement évités,
résidu q3/q4, puis chaîne complète. Inclure 8k/16k/32k et trames
brutes/sans-sol de plusieurs séquences, s8/10/12, avant une ablation
G4. Un budget constant par arête ne suffit pas à prouver le
sous-quadratique si `E(n)` ou l'aval restent trop grands. Ce schéma
est prometteur **mathématiquement**, pas encore industriellement.
