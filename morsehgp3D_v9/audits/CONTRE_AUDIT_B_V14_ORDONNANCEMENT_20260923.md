# Contre-audit B — sonde v14 et ordonnanceur q3/q4

23 septembre 2026. Commit produit [v14](../docs/PROVENANCE.md)
`67fce4e9` (même patch v9 que le commit local `391a5140`). Audit du code
et des portes, sans GCP ni nouveau benchmark lourd. Cadre
`exploration_v9_hors_registre`, CPU référence, u18/1 mm,
`not_claimed`.

La v14 active par défaut deux leviers indépendants : préparer les jobs
du front en scindant d'abord le produit de plus grande masse, puis les
réclamer par masse décroissante ; viser **64** jobs par fil au lieu de
**16**. Le plan reste une partition des mêmes produits non visités : les
terminaux sont comptés une fois, les enfants sont disjoints, et les
plages publiées du rectangle partitionnent A. L'index est immuable,
les états restent privés, les threads sont joints. Je n'ai pas trouvé de
perte de tâche ni de course dans le patch statique.

Les portes ont été élargies de façon pertinente : plus de 1 000 plans
par masse sont rejoués contre le front monolithique jugé par oracle ;
les cas q3/q4 parallèles sont aussi rejoués par masse, avec comparaison
normalisée des supports, profondeurs et coquilles aux oracles rationnels,
y compris certains cas de cache actif. La porte de chaîne exerce les
nouveaux défauts sur de petites entrées K5/K10 et le contrat de sonde
v14 ajoute ses champs. Cela fournit une **preuve bornée d'identité de
sortie** nettement meilleure que le seul digest. Le cache de témoins
appartient cependant au worker : en changeant l'ordre des arêtes, on
change légitimement hits, visites et pics, sans changer les candidats.
Comparer tout le ledger brut à l'identique serait donc une mauvaise porte.

## Performance : signal local, pas reçu G4

Le commit/`PROVENANCE.md` annoncent localement un plus long job de
14,8→2,9 s et `wait_sum_s` de 11,9→0,01 s sur 08/000000/K5/W8.
**Aucun brut v14 versionné, hash de binaire/source, chrono total apparié
ou reçu G4** ne permet encore de qualifier ce signal. En particulier,
les deux leviers ont changé ensemble : ni l'effet propre de l'ordre,
ni celui du grain, ni leur interaction ne sont isolés. Réduire
l'attente peut seulement déplacer du temps vers la préparation
**sérielle** du plan ; le critère est le mur q3/q4 et celui de toute la
chaîne, avec travail géométrique et objet conservés.

`job_sum_s` et `max_job_ms` ne chronomètrent que `plan->run_job`.
Ils incluent le travail en ligne d'un job, **pas** les plages publiées
exécutées via `rectangle_range` par un worker. La nouvelle sonde ne
publie ni maximum de plage ni heure de fin des deux catégories ; le
`max_job_ms` seul ne peut expliquer l'occupation de R8. Le lecteur
borne les nouveaux temps par les murs, mais autorise encore
`jobs>0, job_sum_s=max_job_ms=0` et n'impose pas leurs identités de
non-vacuité. Les mutants ajoutés n'exercent que les dépassements du
mur et du budget fils×mur.

Le runner LiDAR v14 ne propose pas de `--lever` pour séparer les deux
options : il épingle tous les leviers ON. Son résumé de campagne ne
remonte ni le temps q3/q4 ni `q34_occupancy` (ces champs restent dans les
JSON de cas). Sa liste de schémas connus garde v12 et v14 mais retire
v13 : aucune campagne locale v13 versionnée n'a été trouvée, pourtant
une éventuelle archive v13 ne serait plus revalidable par ce lecteur.
Une campagne de performance v14 exige donc un runner/plan qui publie
explicitement la matrice des leviers et garde ses bruts.

La masse diagonale emploie encore `a*(a−1)/2` en `u64` **avant** la
division, dans `front.cpp` (préparation et getter). Pour le domaine
public `n=2^32+1`, elle renvoie `2^31` au lieu de
`9 223 372 039 002 259 456` ; l'ordre des jobs devient faux, la
géométrie non. C'est sans effet sur les trames LiDAR R8 ou sur quelques
dizaines de millions de sites, mais la fabrique ne pose pas de plafond
2^32. Un correctif non committé était en cours lors de la relecture :
ne pas l'attribuer à `67fce4e9`.

## Porte G4 proportionnée

Même binaire, même trame difficile 08/000000 **ou** 000200 sans sol,
K5/s8/W48, comparer les **quatre** combinaisons ordre normal/par masse
× grain 16/64, deux répétitions entrelacées : huit cas plus préflight.
Rendre visibles q3/q4, chaîne, CPU·s, attente **absolue**, préparation
du front, somme/maximum et fin des jobs **et** des plages, refus de
file, ainsi que catalogue et digest. Comparer le multiensemble normalisé
des candidats et les compteurs géométriques indépendants du cache ;
publier séparément les compteurs de cache, sensibles à l'ordre. Si le
budget G4 ne permet qu'une paire OFF/ON, elle peut constater un gain net,
pas l'attribuer à l'un des deux leviers. Ajouter une porte de non-vacuité
des nouveaux chronos avant de les utiliser comme explication.

Même un gain d'ordonnancement réel ne réduit pas les **77,320 CPU·s**
q3/q4 du meilleur cas R8/K5 à travail inchangé, ni son résidu hors
q3/q4 de **1,147 s**. La réduction des formes et la refonte FULL restent
des chantiers distincts ; aucun contrat <1 s n'est acquis.
