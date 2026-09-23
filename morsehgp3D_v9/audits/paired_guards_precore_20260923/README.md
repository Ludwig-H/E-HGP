# Avant le cœur q3/q4 : des paires de gardes complémentaires

23 septembre 2026. **Certificat exact nouveau dans cet audit**, et shadow
CPU seulement. Il cible les grosses arêtes q3/q4 survivantes au filtre
ponctuel S2 ; il ne modifie aucun code produit. Les deux [échantillons](RESULT.json)
et [second échantillon](RESULT_SEED2.json) viennent de la même trame brute
SemanticKITTI 08/000000 avec sol, grille commune 1 mm/u18, K5/s8. La
sélection de gardes de ce shadow balaie **les 123 389 sites pour chaque
arête** : ses succès sont un potentiel géométrique, pas un algorithme
rapide ni une baisse de temps mesurée.

## Certificat sans subdivision du disque

Pour une arête `ab` **réellement propriétaire comme plus longue arête**,
poser `d=b−a`, `D=|d|²>0`, `m=(a+b)/2`. Tout centre d'une présentation
q3 aiguë possédée s'écrit `c=m+t`, `t·d=0`, `|t|²≤D/12` ; pour q4
positive, `|t|²≤D/8`. Ces bornes viennent de la variance des poids
barycentriques positifs, respectivement trois et quatre supports.
Pour un site réel `g`, sa marge d'intériorité de la boule passant par
`a,b` est

`P_g(t)=D/4−|g−m|²+2(g−m)·t`.

Elle est strictement positive exactement lorsque `g` est intérieur.
Un garde seul doit la rendre positive sur **tout** le disque, ce que
teste déjà S2. Une paire distincte `g,h` n'exige que `P_g+P_h>0` :
alors, à tout centre, **au moins un** de ces deux vrais sites est
strictement intérieur, même si aucun des deux n'est universel.

Avec `w_g=2g−a−b`, `w_h=2h−a−b`,
`H=2D−|w_g|²−|w_h|²`, `X=|d×(w_g+w_h)|²`, on a
`P_g(t)+P_h(t)=H/4+(w_g+w_h)·t`. Son minimum sur le disque est
`H/4−ρ·|proj_(d⊥)(w_g+w_h)|`. Le certificat entier **strict** est donc :

| Voie | Condition pour une paire |
| --- | --- |
| q3 | `H>0` et `3H²>4X` |
| q4 | `H>0` et `H²>2X` |

Une égalité ne crédite rien. Pour K5, **quatre paires disjointes**
ferment q3 (`K−1` intérieurs) et **trois** ferment q4 (`K−2`). Les
paires peuvent changer entre les deux voies, mais leurs comptes ne
s'additionnent jamais. Un site ne peut appartenir à deux paires d'une
même preuve, ni être une extrémité. Les gardes n'ont pas besoin d'être
dans le cœur diamétral pour que le lemme soit valide ; le shadow les y
restreint seulement pour proposer une palette. Sous u18, `|H|<2^41`,
`H²<2^82`, `X<2^80` et les comparaisons pondérées tiennent en i128
signé, après promotion **avant** les produits. Les centres q3/q4 et
les coquilles à égalité restent couverts par le repli exact.

La [fixture native](fixture.cpp) appelle réellement
`filter_q34_witnesses` du produit sur `a=(5,10,10)`, `b=(15,10,10)`
et les huit gardes `(x,13,10),(x,7,10)` pour `x=8,9,10,11`. S2 garde
les deux voies ouvertes (`mask=6`, zéro crédit), et chaque singleton
échoue aux deux tests universels. Les quatre paires de même `x`
satisfont pourtant les deux inégalités ci-dessus, avec IDs distincts ;
un mutant réutilisant un garde est refusé. Le tétraèdre de sommets
`a,b,(10,10,16),(10,16,10)` a des poids barycentriques positifs
`25/72,25/72,11/72,11/72` au circumcentre, et `ab` est strictement
sa plus longue arête : le domaine q4 possédé n'est pas vide par
construction géométrique. Ces deux sommets supplémentaires ne sont pas
ajoutés au nuage de l'appel S2 de la fixture.

## Potentiel mesuré sur arêtes S2 réellement lourdes

Chaque graine tire **20 arêtes par tranche** de taille du cœur `F`
`[1000,3000)`, `[3000,6000)`, `[6000,∞)` parmi les 20 252, 41 146,
29 869 arêtes disponibles. Réservoir déterministe sur les huit traces
S2 épinglées ; les 60 IDs, `F` et masques restent dans chaque JSON.
Pour chaque arête, le [script](pair_shadow.py) recalcule indépendamment
le cœur diamétral (`F`, extrémités incluses), classe ses sites dans les
quatre quadrants définis par deux bases entières indépendantes du plan
bissecteur, **pas nécessairement orthogonales**, puis garde les `B=4/8/16`
sites les plus proches du milieu **par quadrant**, soit au plus
16/32/64 candidats par arête.
L'appariement glouton trie les marges exactes et accepte uniquement
des paires disjointes. Son échec ne prouve pas l'absence d'un meilleur
appariement ou d'autres sites.
Dans les JSON, les comptes `summary.q3/q4` incluent par convention les
voies **inactives** comme satisfaites ; seules `edges` et `F` comptent
les fermetures conjointes réellement utiles.

| Graine / B par quadrant | Arêtes fermables / 60 | F fermable / F des 60 | Traversantes fermables / traversantes échantillonnées |
| --- | ---: | ---: | ---: |
| 230923 / 4 | 20 | 82 118 / 260 032 | 8 / 37 |
| 230923 / 8 | 22 | 94 938 / 260 032 | 10 / 37 |
| 230923 / 16 | **27** | **111 883 / 260 032** | **12 / 37** |
| 230924 / 4 | 32 | 124 221 / 260 599 | 20 / 45 |
| 230924 / 8 | 34 | 137 364 / 260 599 | 22 / 45 |
| 230924 / 16 | **40** | **168 845 / 260 599** | **27 / 45** |

Les 37 puis 45 arêtes « traversantes » ont leurs extrémités dans deux
quarts physiques différents ; 37 puis 44 franchissent `x=0` du capteur.
Ce sont **deux petits échantillons stratifiés d'une seule trame**, pas
une proportion du flux complet ni une loi de croissance. Un premier
essai avec les 16/32 plus proches du milieu **sans** équilibre
directionnel fermait 0/60 sur la graine initiale : la sélection est
le verrou. Le shadow directionnel lit 7 403 340 sites pour seulement
260 032 formes de cœur échantillonnées à la première graine ; le
porter tel quel déplacerait et amplifierait le coût. Les 111 883/
168 845 `F` sont uniquement une charge **potentiellement évitable
avant `load`** si un prouveur peu coûteux retrouve les mêmes gardes.
Ni cover, catalogue, FULL, temps CPU/G4, K10 ou borne sous-quadratique
ne sont qualifiés.

**Suite concrète à juger en shadow :** quatre requêtes à budget fixe sur
l'index spatial immuable, par quadrant du plan propre à l'arête. Pour
un nœud, les intervalles exacts des deux produits scalaires contre sa
boîte permettent d'écarter les quadrants impossibles ; la distance
minimale de sa boîte au milieu sert de priorité et de borne pour les
`B` candidats. Si un budget s'épuise, la palette partielle reste sûre
pour les **preuves positives**, et l'arête non prouvée revient au chemin
actuel. Mesurer visites de nœuds/feuilles, candidats, appariement,
masques conjoints, covers et `F` réellement évités sur les traces
entières, les demi-scènes et les quarts aux trois densités, puis brut
et sans-sol de plusieurs séquences. Une réussite sur les grandes
arêtes ne suffit pas si le coût des recherches sur toutes les arêtes
domine.

## Relecture et rejeu

Le [lecteur statique](verify.py) recalcule les 299 puis 383 inégalités
strictes des paires émises comme preuves positives, leurs IDs disjoints,
les seuils, les agrégats et les fermetures traversantes, avec les
coordonnées conservées dans les JSON. Sa preuve positive suppose que
l'association ID–coordonnée sauvegardée est fidèle au nuage : le rejeu
sur les entrées aux SHA épinglés vérifie cette association et les `F`.
**Il ne peut pas prouver les échecs** de la palette ni l'origine du
réservoir depuis ces seuls résumés ; pour cela, rejouer le script sur le
nuage complet et, pour le tirage initial, sur les traces S2 épinglées.
Le [reçu](PROVENANCE.json) épingle les SHA des coordonnées, IDs,
quarts physiques et huit parties de trace ; [SHA256SUMS](SHA256SUMS)
épingle les petits fichiers versionnés. Lecture sans données `/tmp` :

```sh
python3 -B morsehgp3D_v9/audits/paired_guards_precore_20260923/verify.py
python3 -B -O morsehgp3D_v9/audits/paired_guards_precore_20260923/verify.py RESULT_SEED2.json
(cd morsehgp3D_v9/audits/paired_guards_precore_20260923 && sha256sum -c SHA256SUMS)
```

Les entrées peuvent être **régénérées sans les traces temporaires** depuis
les reçus v8 versionnés. Depuis la racine du dépôt, avec un répertoire
neuf sous `/tmp` :

```sh
python3 -B morsehgp3D_v9/audits/lidar_raw_physical_scaling_20260923/generate.py \
  --repo "$PWD" --out /tmp/mhgp9-pair-replay-inputs
python3 -B morsehgp3D_v9/audits/paired_guards_precore_20260923/pair_shadow.py \
  --inputs /tmp/mhgp9-pair-replay-inputs \
  --sample morsehgp3D_v9/audits/paired_guards_precore_20260923/RESULT.json \
  --output /tmp/mhgp9-pair-replay-result.json
cmp morsehgp3D_v9/audits/paired_guards_precore_20260923/RESULT.json \
  /tmp/mhgp9-pair-replay-result.json
```

Remplacer `RESULT.json` par `RESULT_SEED2.json` et choisir un autre nom
de sortie pour la seconde graine. Les huit traces complètes peuvent aussi
être reconstruites par
[`edge_matched_core_20260923/replay.sh`](../edge_matched_core_20260923/replay.sh)
dans un nouveau `/tmp` ; leur répartition entre workers peut changer,
donc leurs SHA de partie et l'ordre du réservoir peuvent différer.
Le mode `--sample` évite cette dépendance et vérifie à nouveau les `F`
des 60 arêtes sur les coordonnées régénérées.

Pour refaire la fixture native avec l'archive CPU de SHA dans le reçu :

```sh
g++ -std=c++20 -O2 -Wall -Wextra -Werror \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/paired_guards_precore_20260923/fixture.cpp \
  build/v9-open-worktree/build/v9-dev/libmhgp9_gen.a -pthread \
  -o /tmp/mhgp9-paired-guard-fixture
/tmp/mhgp9-paired-guard-fixture
```
