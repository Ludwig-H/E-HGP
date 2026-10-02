# Maturité géométrique : trois gardes de contrat pour v11

2 octobre 2026. Relecture ciblée de la [réponse § 4/Q13](sources/REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md),
de [CONCEPTION § 5](sources/CONCEPTION.md) et d'[em_geo.py](sources/em_geo.py).
Les [lignes utiles](sources/contexte_lignes.txt) et les
[réponses Q13–15 déjà publiées](sources/Q13_15_lignes_1_118.txt) sont figées.
Le [ledger](SOURCE_LEDGER.json) constate quatre sources avant = après.

**Aucun défaut nouveau constaté.** La définition actuelle distingue correctement
τ géométrique de θ interpolé et utilise le continuum de centres `C_r`.
Elle reconnaît les appartenances multiples et indique que la règle géométrique
complète n'est pas implémentée. Ce reçu précise ce qu'un port v11 doit conserver ;
il ne refait pas les 633 gardes du théorème F/C ni le contre-exemple du carré.

## Tailles géométriques et masses exclusives

Prendre trois sites distincts `X = {(0,0,0), (2,0,0), (4,0,0)}`, K = 2,
τ = 1/2, mcs = 2. Les deux lentilles `B(0,r) ∩ B(2,r)` et
`B(2,r) ∩ B(4,r)` naissent à r = 1 et restent dans des composantes distinctes
pour `1 ≤ r < 2` : toute intersection entre elles doit contenir les trois
contraintes, dont les sites extrêmes à distance 4. Elles fusionnent à r = 2.

À r = 4/3, la première composante rend mûrs les sites `{0,2}` et la seconde
`{2,4}`. Chaque taille géométrique vaut 2, mais leur somme vaut 4 sur **trois**
sites. Le site central est réellement mûr dans les deux composantes.
Une projection exclusive donne des tailles `(2,1)` ou `(1,2)` si elle affecte
les trois sites à ces deux branches. Même en laissant un site seul, elle ne
peut produire deux blocs de taille
au moins mcs. Le même obstacle vaut avec masses fractionnaires conservées :
une masse totale de 3 ne finance pas deux blocs de masse 2.

Ainsi, `s_τ(C,r) ≥ mcs` peut légitimement admettre une **composante géométrique**,
sans assurer mcs membres exclusifs après projection. Pour préserver ce dernier
contrat, compter chaque site sur une lignée fixée avant admission, ou recondenser
après attribution. La seconde politique peut supprimer une branche ; cette perte
fait partie du contrat et doit être mesurée. Une condition nécessaire à la
préservation d'une famille de branches est que l'union de leurs sites admissibles
pour attribution ait au moins `mcs × nombre de branches` sites, pas seulement
que chaque branche atteigne mcs séparément.

## Le continuum de centres est essentiel

Pour deux sites 0 et 2 sur l'axe, la projection sur cet axe de leur lentille
est l'intervalle `[2−r, r]`, pour r ≥ 1. Chaque point de cet intervalle est
effectivement un centre admissible sur l'axe ; projeter orthogonalement un
centre hors de l'axe diminue ses distances aux sites. La distance d'un site
de l'axe à la lentille est donc exactement sa distance à cet intervalle.

Pour x = 0 et r = 4/3 :

| Objet utilisé pour la distance | Distance | Test `distance ≤ τr = 2/3` |
| --- | ---: | --- |
| Composante réelle de centres `C_r` | 2/3 | vrai, contact exact |
| Seul centre MEB représentatif, situé en 1 | 1 | faux |
| Ensemble des points couverts `{0,2}` | 0 | vrai dès la naissance |

La vraie maturité est `2/(1+τ) = 4/3`. Remplacer `C_r` par les seuls centres
critiques ou par les sites couverts change la règle. L'oracle actuel ne fait
aucun de ces remplacements. Ce témoin est une garde contre ces raccourcis futurs.

## Plancher de la projection et vie des branches

Une projection admissible peut retarder l'entrée de x = 0 jusqu'à e = 3/2,
dans la première composante : celle-ci est vivante et couvre x à cette date.
Sa maturité géométrique intrinsèque vaut toujours 4/3, donc elle précède e.
Pour appliquer les formules causales F/C à **cette projection**, fixer sa lignée
et prendre `m = max(e, maturité dans la lignée)`. Ici m = 3/2. Sans ce plancher,
un site pourrait financer l'admission avant son entrée ; l'hypothèse `m ≥ e`
utilisée par F/C et la preuve de masse minimale ne serait plus satisfaite.

La vie d'un enfant est `[naissance, mort[` : une maturité égale à sa mort est
traitée dans le parent, après la multifusion du plateau. Le port doit garder
cette convention et dédupliquer les sites comptés ; le document actuel l'énonce.
Ces précisions n'interdisent pas une variante géométrique non exclusive ou une
validation rétroactive, à condition de nommer leur contrat séparément.

## Vérifications bornées

[check.py](check.py) dérive Γ₂ sur la droite à partir des MEB des unions,
compare six coupes exactes et les 36 décisions site/lentille du helper figé à
la formule indépendante des intervalles, puis contrôle conservation, contact,
distances et entrée retardée : **45 gardes**. Aucun générateur ni moteur natif
appelé. Pas de nouvelle preuve de complétude générale ou de stabilité du routage.

[normal.json](normal.json) et [optimized.json](optimized.json) sont identiques ;
[RUN.json](RUN.json) conserve les commandes et codes retour.
[SHA256SUMS](SHA256SUMS) ferme les payloads ; le lecteur le vérifie lorsqu'il
existe. Les scripts restent actifs sous `python -O`. Sources privées exploratoires
copiées pour rendre ce petit reçu portable, sans modification des originaux.
