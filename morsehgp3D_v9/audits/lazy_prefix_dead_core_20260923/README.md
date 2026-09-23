# Préfixe paresseux des formes du cœur diamétral — audit K5 du 23 septembre 2026

Le coût de fabrication des formes du `Q34DeadLaneProver::load` a une réduction
possible **mesurable** sur le LiDAR brut 08/000000, surtout pour les arêtes
traversant le plan `x=0` du capteur. Un préfixe paresseux dans **l'ordre actuel**
aurait eu besoin de 316 679 908 formes sur les 559 661 741 fabriquées à pleine
densité : 242 981 833 formes de suffixe, soit 43,42 %, n'ont jamais été lues
par le certificat. Il s'agit d'une limite de travail évitable par une
implémentation future, **pas** d'un gain CPU ou mémoire constaté.

| Densité globale | Sites | Cœurs chargés | Formes fabriquées `Σn` | Préfixe visité `Σh` | Suffixe non consulté `Σ(n−h)` |
|---|---:|---:|---:|---:|---:|
| 1/4 | 30 847 | 774 494 | 37 009 904 | 23 995 004 | 13 014 900 (35,17 %) |
| 1/2 | 61 694 | 1 684 675 | 128 852 821 | 77 848 321 | 51 004 500 (39,58 %) |
| 1 | 123 389 | 3 986 433 | 559 661 741 | 316 679 908 | 242 981 833 (43,42 %) |

À pleine densité, 124 423 arêtes traversant `x=0` concentrent 383 619 073
formes ; 196 982 817 (51,35 %) de ces formes appartiennent à un suffixe
jamais consulté. Les arêtes de longueur intrinsèque `|ab|≥4 m` concentrent
450 005 733 formes, dont 219 714 302 non consultées (48,82 %). Le critère
`x=0` sert uniquement à **diagnostiquer** la scène ; un futur chemin produit
ne doit pas supposer des passages alignés ou une coupe particulière. Les
densités 1/4 et 1/2 sont des sous-ensembles globaux emboîtés de la même trame,
pas des moitiés ou quarts spatiaux.

Le cover suivant fabrique encore 364 354 011 formes à pleine densité. Même
en omettant tous les suffixes identifiés, le gain de fabrication représenterait
26,30 % seulement du total des formes cœur + cover de ce run ; les autres
coûts restent inchangés.

Les suffixes non consultés apparaissent uniquement sur les arêtes dont les
deux voies encore candidates sont finalement fermées par le cœur. Pour toute
arête dont au moins une voie reste ouverte, `h=n` dans ces trois exécutions.
La fermeture est connue **après** le parcours du prouveur ; elle ne peut donc
pas servir à sélectionner gratuitement les arêtes avant la charge. Le préfixe
paresseux doit se construire à la demande pendant la preuve exacte, sans
recompter un simple voisinage central déjà traité par le filtre ponctuel.

Le témoin `h` est le plus grand ordinal `id+1` de forme réellement consulté
dans les scans de `cell`. Les tests de point sur `next` réutilisent uniquement
des ID déjà traversés. Donc `n−h` est exactement le suffixe qu'une version
paresseuse **gardant cet ordre et ce certificat** pourrait omettre de
matérialiser. `n` et `h` incluent les deux extrémités de l'arête, dont les
formes nulles sont aussi fabriquées aujourd'hui ; ne pas confondre cette
différence avec le compteur `dead_core_form_sites`, qui écarte ces deux sites.
Le scan est profond de 2 au premier niveau actif (`min_depth=2` par défaut) ;
toute cellule de cette profondeur qui descend a dû parcourir le frontier
entier, d'où `h=n`. L'observateur n'a changé ni l'ordre des sites ni les
conditions de sortie.

La pente finie de `Σh` entre 1/2 et pleine densité est encore **2,024**,
et celle de `Σh` sur les arêtes traversant `x=0` est **2,364**. La pente du
travail de fabrication initial est respectivement 2,119 et 2,306. Un préfixe
paresseux peut donc réduire le coût absolu, mais cet audit ne démontre aucune
croissance sous-quadratique, et les visites du cover, les tests de cellules,
l'atlas, le catalogue et la tour demeurent. La prochaine ablation utile est
un chemin opt-in qui calcule les formes dans l'ordre actuel au premier accès,
compare le travail, le temps et la mémoire **de toute la chaîne** à source et
entrée identiques, puis répète sur plusieurs trames brutes et sans sol, K5 et
K10. Les témoins de cellules restent exacts ; une simple recomptabilisation
des témoins de la boule centrale n'apporterait pas de filtre inédit.

## Preuve et provenance

Le source produit est le commit `c265a5dae4dd92059fc78acc0a1d7f52de9c1435`.
[`prepare.py`](prepare.py) archive ce commit et injecte **dans un arbre sous
`/tmp`** le seul champ observateur `h`, les compteurs de profondeur 2 et le
[`traceur`](lazy_prefix_trace_audit.hpp). Le patch source, ses hashes et ceux
du traceur sont dans [`PATCH_PROVENANCE.json`](PATCH_PROVENANCE.json). Compilation
Release, CPU uniquement, `K=5`, `s=8`, huit workers et huit threads statiques,
filtre q3/q4 batch CPU actif, filtre GPU désactivé. Les coordonnées sont le
profil entier optionnel à grille isotrope 1 mm préparé à partir des retours
float32 de la trame brute. Le [manifeste d'entrée](../lidar_raw_physical_scaling_20260923/MANIFEST.json)
fixe les identifiants de retours, les sélections globales et les coupes
physiques faites avant arrondi. Aucun alignement des points n'est supposé.

[`run_case.py`](run_case.py) recalcule le même probe avec trace et compare
`input`, options, ledger, générateur, catalogue, ordres, digest et travail de
tour au batch CPU antérieur. [`analyze.py`](analyze.py) joint **chaque** arête
par ses identifiants de retour à la [trace initiale](../edge_matched_core_20260923/README.md),
vérifie les hashes et les sommes `Σn=core_sites=dead_core_form_sites+2·charges`,
le nombre de fermetures, `h≤n`, `full/descente⇒h=n`, et au plus 16 cellules
actives à profondeur 2. Au plein, les masques post-cœur et les formes par
classe égalent aussi le [reçu post-cœur](../edge_matched_core_20260923/POST_CORE_FULL.json).
Toutes ces vérifications ont réussi ; les comptes, digests et SHA des traces
binaires temporaires figurent dans [`RESULTS.json`](RESULTS.json). Les gros
binaires, la trame et les traces restent sous `/tmp` ; les scripts permettent
leur régénération en suivant les deux audits liés. Les SHA des morceaux de
trace scellent cette capture ; leur répartition entre workers peut changer
au rejeu, alors que les sommes et le joint par arête doivent rester égaux.
`sha256sum -c SHA256SUMS` contrôle les fichiers compacts conservés.
Les durées du build instrumenté ne sont pas interprétées, car l'écriture
des traces et la mise à jour de `h` modifient le chemin chaud.

Limites : une seule trame de la séquence 08, une répétition par densité, K5
seul, profil grille 1 mm ; statut du probe `complete_relative` par rapport au
catalogue recroisé. Aucun nouveau résultat FULL, float32 par défaut, G4/GPU,
K10 ou borne asymptotique générale n'en découle.
