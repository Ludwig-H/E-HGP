# S10 WIP — borne signée du filtre de l'API abstraite

Base Git : `311ef5e3cdd37967471d0148794ad12f81349b0f`. Les trois sources
causales de `head` sont nouvelles et non commises ; elles ont été capturées
deux fois, octets identiques, le 5 octobre à 19:23:28 UTC. `SOURCE.json`
donne leurs empreintes. Ce constat vise cette capture WIP, pas un S10 qualifié.

`head.hpp` publie `TreeView` et `LevelSource` pour les arbres abstraits,
avec `root = floor(2^64 sqrt(l))` et refus hors de `u128`. Or `score.cpp`
convertit cette racine en `i128`, puis calcule `e_hi = rt + 1` dans ce type.
`select.cpp::flat_sites` appelle ce filtre après condensation sans réduire le
domaine déclaré des racines.

Témoin : quatre sites, deux blocs racines de deux sites chacun, un seul
plateau positif, `mcs=2`, `z=1`. La table conserve `l_0=0` et prend
`l_1=((2^127-1)/2^64)^2` ; le plateau a `(t,m,q)=(1,0,0)`. C'est une forêt
abstraite permise par la vue publique. Sa racine fixe vaut exactement
`R=2^127-1`, dans `u128` et même dans `i128`, mais `R+1=2^127` dépasse la
borne de `i128`. L'addition signée du filtre déborde donc avant toute
décision EOM. Le même problème existe pour la somme signée de trois racines.

Le script établit la borne par arithmétique entière exacte et décrit la vue ;
il ne compile ni n'exécute `flat_sites` ou le filtre natif. Il ne prouve aucun
débordement des catalogues géométriques u24, dont les racines sont largement
plus petites. L'algèbre des réciproques et le programme EOM ont été lus
favorablement, sans nouveau rejeu de leurs anciennes preuves.

Correction possible : contrôler la largeur **avant** toute conversion et
addition signées, avec les marges de `rt+1` et `rt+rm-rq-1/+2`, puis marquer
le plateau sans encadrement (`open`) pour le
repli exact ; ou faire ces calculs avec `Big`. Si la solution est un refus
explicite, déclarer la restriction supplémentaire de `LevelSource`.

Rejeu autonome, bibliothèque standard seulement :

```sh
python3 -B check.py
python3 -B -O check.py
```

Deux exécutions, normal et `-O`, rendent le code 0, les mêmes octets de sortie
et des stderr vides. Aucun essai en échec ; le premier script exécuté est
conservé sous `attempts/initial_check.py`.

`executions.json` conserve chaque commande et code. Les sources copiées sont
vérifiées par le script ; `SHA256.json` ferme toute la capsule, sans se lister
lui-même. Aucun build, test natif ni GCP ; aucune donnée LiDAR.
