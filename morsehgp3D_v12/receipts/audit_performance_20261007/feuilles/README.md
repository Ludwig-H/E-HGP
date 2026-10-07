# Feuilles CPU : travail physique évitable

7 octobre 2026, audit indépendant, pin `58d384721678d11ef8ccd86c76cca182f41a716c`.
`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `public_status=not_claimed`.
Lecture et sept feuilles synthétiques isolées ; aucun banc LiDAR, GCP, temps de performance ou FULL.

**Cause prouvée au niveau du travail : la simulation CPU de J3 effectue des calculs que la DFS v11 évitait.**
Les quinze compteurs logiques sont égaux par construction, mais ne comptent pas ce travail supplémentaire.
Ce constat ne suffit pas à chiffrer sa part dans la régression G4 : la fin de catalogue séquentielle reste prioritaire.

## Constats et preuve légère

- [`leaf_common.hpp`](../../../src/catalogue/leaf_common.hpp), `prepare` : pour chaque `x`, visite de tous les
  `y != x`, avec calcul géométrique dans l'orientation canonique `i < j`. Chaque paire est donc calculée **deux
  fois**, tandis que `finish` publie `dominance_tests = m(m-1)/2`. La
  [v11 gelée](../../../../morsehgp3D_v11/src/catalogue/leaf.cpp) évalue directement `i < j`, une fois.
- [`leaf_census.hpp`](../../../src/catalogue/leaf_census.hpp), `census` : tous les sites sont classés, puis le vote
  trouve l'indice du `(theta+1)`-ième intérieur, `theta = K+1-q`. Le compteur logique s'arrête à cet indice ;
  les prédicats exécutés au-delà sont déjà payés. La v11 interrompt réellement sa boucle au même indice.
- [`simt.hpp`](../../../src/catalogue/simt.hpp) : sur l'hôte, une phase de voies est une boucle de largeur 32
  ou 256, pas une instruction de vote matériel. Les cas `m < N` paient des cases inactives et les collectives ;
  cela n'implique aucun facteur de temps égal à `N/m`.

[`probe.py`](probe.py) construit dans `/tmp` trois versions des mêmes en-têtes : originale, instrumentée, puis
expérience d'arrêt scalaire. Aucun fichier produit n'est modifié. Comparaison **de tous les mots des émissions**
(supports, populations, cardinalités), statuts et quinze compteurs avant réduction des captures à des SHA-256.
L'instrumentation mesure les opérations de la source, pas les instructions retirées par le processeur.

| Feuille isolée | Dominances logiques / calculées | Classifications logiques / effectuées | Prédicats `side` effectués / au-delà du rejet |
| --- | ---: | ---: | ---: |
| 24 points alignés, K5 | 276 / 552 | 4 515 / 6 624 | 6 072 / **1 938** |
| 24 points alignés, K10 | 276 / 552 | 5 805 / 6 624 | 6 072 / 728 |
| Cube et centre, 9 sites, K5 | 36 / 72 | 522 / 522 | 382 / 0 |
| 24 sites synthétiques, K5 | 276 / 552 | 34 305 / 53 400 | 46 031 / **16 203** |
| 24 sites synthétiques, K10 | 276 / 552 | 49 343 / 53 400 | 46 031 / 3 458 |
| 40 points alignés, warp virtuel 256, K5 | 780 / 1 560 | 17 515 / 31 200 | 29 640 / 13 090 |

La ligne de 24 points K5 est aussi exécutée par la politique `Exact` : mêmes résultats et comptes que `Narrow`.
L'expérience d'arrêt scalaire supprime **exactement** les prédicats de la dernière colonne et conserve toutes les
émissions et tous les compteurs sur les sept cas. Ces proportions ne sont ni des mesures LiDAR ni des gains de temps.

## Correction proposée et preuve

**1. Restaurer l'arrêt effectif sur CPU, en gardant une seule classification.** Écrire la classification d'un site
une fois, comme aujourd'hui, mais confier sa visite à une politique d'exécution : ordre croissant avec arrêt sur
l'hôte ; classification en parallèle et vote sur l'appareil. L'expérience ici est seulement l'arrêt dans la copie
CPU de la boucle ; ce n'est pas un second produit ni un retour à l'énumération DFS.

Preuve : soit `r_i` le classement exact du site d'indice `i` et `t` le plus petit indice dont le préfixe contient
`theta+1` intérieurs. Si `t` existe, les positions ultérieures ne peuvent réduire leur nombre : le rejet est déjà
irréversible et aucune population ni boule ne sera publiée. Le compteur vaut `t+1` dans les deux voies. Si `t`
n'existe pas, toute la liste est visitée, donc les mêmes `I`, `U`, `S*`, compteurs et émissions sont obtenus.
Le seuil dépend de la présentation courante `q`, comme en v11 ; ne pas le remplacer par un `qmin` inconnu.
Cette preuve suppose le domaine arithmétique certifié de la feuille : masquer un débordement possible n'est pas
une justification d'adoption. Le budget de `Narrow`/`Exact` reste une précondition.

Cela **préserve les quinze compteurs** et l'algorithme géométrique, mais demande une décision explicite sur la
politique CPU de la source commune, puis différentiel complet et microbanc G4. Une boucle séquentielle peut aussi
perdre de la vectorisation : aucune baisse de temps n'est présumée à partir du seul nombre de prédicats.

**2. Calculer une seule fois la relation de chaque paire.** Une table triangulaire temporaire de relations,
remplie par les voies puis lue pour former chaque ligne `dom/domby/nbr`, conserve exactement les masques : les
deux lectures actuelles utilisent déjà les mêmes coordonnées dans la même orientation. Le compteur logique
reste `C(m,2)` ; publier séparément les évaluations réellement effectuées. Le calcul du popcount d'union des
dominances pour les lignes `live` est également symétrique. Sur GPU, la table peut coûter du partage mémoire et
de la synchronisation : comparer cette variante au noyau adopté, ne pas remplacer à l'aveugle `j3_r168`.

**3. Effacer uniquement les lignes H accessibles**, à titre secondaire. Aujourd'hui `run_leaf` efface les
`C(N,2)` lignes, même quand `m << N`. Seules les lignes `(i,j)` avec `i<j<m` peuvent être lues : les masques
`nbr/live` n'ont aucun site extérieur à la feuille. Effacer ces lignes via `hrow<N>(i,j)` garde le même sens
et les mêmes compteurs. Pour 40 sites dans le warp virtuel, cela passe de 32 640 à 780 lignes de 32 octets.
Ne pas en déduire un problème sur les trames LiDAR où les feuilles larges sont absentes ou rares.

**À ne pas faire.** Diminuer arbitrairement K, les coquilles ou les feuilles ; annoncer la moitié des temps parce
que les dominances sont calculées deux fois ; ajouter un crédit de population qui serait déjà dans `dom` ;
modifier les compteurs logiques pour afficher moins de travail ; supprimer tout comptage avant réservation.
Le rejeu de certaines feuilles lors de l'écriture est réel, mais le reçu développeur n'en compte que
3 990 sur 123 581 à ng00 K5 : il n'y a pas de double passage général des feuilles.

## Reproduction et limites

```sh
python3 morsehgp3D_v12/receipts/audit_performance_20261007/feuilles/probe.py
python3 -O morsehgp3D_v12/receipts/audit_performance_20261007/feuilles/probe.py
```

Le lecteur vérifie les 27 en-têtes compilés, avant et après, contre [`result.json`](result.json), et refuse une
source changée. `--repo-root` permet de viser un checkout du pin. Les fichiers temporaires sont supprimés.
Les sept cas et leurs sorties sont relus, pas seulement les empreintes mémorisées. Aucun binaire ni copie d'en-tête
n'est conservé dans le dépôt. `SHA256SUMS` ferme les fichiers du reçu.

Périmètre : preuve de travail évitable et prototype causal borné, pas qualification d'un catalogue optimisé.
Les constats sur la symétrie et H sont des preuves par lecture, pas des prototypes mesurés. Les nouveaux diagnostics
physiques à capter sur G4 sont `dominance_evaluations`, `census_side_calls`, `census_side_calls_after_logical_stop`,
remplissage des voies et feuilles rejouées ; les quinze compteurs logiques restent inchangés.
La seule erreur de préparation du harnais a été un refus de l'ancienne capture après ajout du SHA-256 des émissions
(schéma enrichi), avant sa régénération ; aucune différence géométrique n'a été observée.
