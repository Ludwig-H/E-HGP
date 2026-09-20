# Carte q4 partagée à la demande — qualification du20 septembre2026

Tranche27 après2920b8b5, `public_status=not_claimed`, profil u16 en entrée,
CPU mono. [Contrat et preuve](../../docs/Q4_CARTE_CENTRES_PARTAGEE_20260920.md).
Cette expérience ajoute une carte de certificats commune aux faces d'une
arête, après le filtre Variance/Collective26, avec **le même pool C64**.
La carte ne calcule pas la tour et ne remplace pas les tris déjà payés
par26. Le prototype indépendant A utilisait tous les témoins : ses gains
ne sont pas transférés à ce port.

## Résultat à retenir

Le raccord conserve les sorties exactes jugées, mais le supplément de
rejet est faible et **aucun gain de vitesse stable n'est acquis**. Le
défaut reste inchangé. La préparation du domaine par blocs est utile ;
elle ne résout pas le résidu faces survivantes × taille du cover.
Ni P0 global, ni un générateur global sous-quadratique, ni FULL, ni les
contrats50k/G4 ne sont qualifiés. GCP non utilisé.

## Preuves closes

| Capture | Contenu | Verdict |
|:---|:---|:---|
| `smoke_errjw8oa` | Release : six gates +24 mesures,30 commandes | PASS |
| `smoke_qlhb8kul` | Clang ASan/UBSan : mêmes six gates +24 mesures | PASS |
| `scale_4crhbuq1` | Release mono fixé au CPU0 : six gates +80 mesures | PASS |
| `regression_v8hao39k` | CTest Release complet,88 tests | PASS |
| `readers_o7wbht7_` | Dix lectures/selftests normal/−O | PASS identiques |
| `mutants/compiled_yonakzn0` | Baseline puis trois mutations compilées | Trois fautes tuées |
| `mutants/differential_lkomphpg` |20 paires ancien binaire26/nouveau | Champs hors temps identiques |
| `dense_forecast/forecast_ysxpufca` |48 filtres denses8k/16k/32k, aucun repli exécuté | PASS |

Les inventaires, commandes, environnements sélectionnés, sorties, codes,
hashes source/artefact avant et après sont conservés dans chaque capture.
Les170 sources produit et les deux builds
`build/v8_q4_center_map_20260920` et
`build/v8_q4_center_map_sanitize_20260920` sont épinglés après clôture ;
reprendre le développement dans des builds neufs. Les sources auxiliaires
denses sont deux pins supplémentaires, pas une extension silencieuse
de l'inventaire produit. Les préflights sont distingués dans [PREFLIGHT](PREFLIGHT.md).

La gate nouvelle compte1956 contrôles :14 domaines,46 cartes,268 requêtes,
51 appels par arête et118 appels par seed comparés aux références,
684 complétions rationnelles, coquille30, quatre erreurs d'allocation
injectées, requêtes répétées et ordres différents, rotations et extrêmes
u16. La profondeur maximale réellement atteinte est21 avec option44.
La compression profonde est positivement exercée deux fois ; la compression
entièrement hors domaine ne l'est pas. Les quatre appels parallèles
utilisent des cartes distinctes : aucune carte mutable partagée n'est
qualifiée pour des requêtes concurrentes. Pas de nouvelle gate TSan.

Les six gates sanitizers sont exact_ball, q34_seed, q34_cover,
q34_family_pruning, q34_collective et q4_center_map. **Pas88 tests sous
sanitizers.** Les43 altérations du lecteur principal sont détectées en
normal/−O : ce sont des fautes de reçus, pas43 fautes géométriques.
Les [trois mutants compilés](mutants/README.md) visent une exclusion de
droite mémorisée globalement, une tangence créditée et une complétion
obtuse oubliée. Les relectures auxiliaires normal/−O historique/live
concordent ; leurs fermetures sont propres et distinctes.

## Mesures principales : même pool, travail total payé

128 mesures =24 Release smoke +24 sanitizers smoke +80 Release scale.
La campagne scale comprend48 grands fonds à8k/16k/32k, K5/10,
Disk/Positive, profondeur5/7 et budget4096, plus32 adversaires
n32/64/128/256. Sur les grands fonds, seules deux faces sont produites :
ces48 essais ne constituent pas une étude dense à nombre de faces croissant.
La carte et le domaine sont préparés à la première requête utile, au sein
du temps englobant ; nuage/index/cover/pool, callbacks et validations sont
distingués. Les sommes de préparation+run partagent la même préparation
mesurée et ne sont pas deux chronométrages indépendants.

À l'adversaire n256/C64 :

| Option | Lectures K5 | Lectures K10 | Sorties K5/K10 |
|:---|---:|---:|:---|
| Référence26 Variance/Collective |10667|21266|14/54|
| Disk, profondeur5 ou7 |10667|21266|14/54|
| Positive, profondeur5 |10667|21266|14/54|
| Positive, profondeur7 |10155|21266|14/54|

Positive7 ajoute1649/1871 tests témoins,1260/2584 visites de requête,
4240/4536 copies d'indices dans les listes héritées et atteint un pic
carte+workspace+buffers simultanés de29192/32152 octets, contre4640 pour26.
Les frères créés mais jamais interrogés ne sont donc pas gratuits.
Le maximum de cette table d'options atteint45824 octets pour Disk7/K10.
Ces capacités excluent objets fixes, métadonnées/transitoires d'allocateur,
callbacks et stockage global partagé ; ce n'est pas le RSS.

Les temps préparation+run Positive7 sont5,763→5,647ms à K5 et
11,751→11,697ms à K10 sur cette seule observation. Sur toute la série
adverse, option/référence varie de×0,792 à×2,080 pour Disk et de×0,959
à×1,227 pour Positive. Les autres qualifications tournaient simultanément :
ces fluctuations ne prouvent aucun gain stable. Des variations favorables
avec exactement le même travail ne doivent pas être attribuées au filtre.

## Dense8k/16k/32k : le mauvais régime reste ouvert

La [campagne auxiliaire](dense_forecast/README.md) exécute vraiment les
filtres, leur préparation et leurs requêtes. Elle calcule ensuite
`cover_sites × q4_survivors`, minimum de lectures du repli actuel, **sans
exécuter ce repli, son tri ni son census**. Ce n'est pas une estimation
du temps GPU. Préfixe et permutation SplitMix64 avant préfixe sont deux
recettes distinctes, pas la permutation SHA256 de l'auditeur.

À32k, C64, Positive/profondeur7 :

| Entrée | K | Familles26→27 | Lectures futures minimales | Dernier doublement |
|:---|---:|:---|---:|---:|
| Préfixe |5|5792→5792|185344000|×9,694|
| Préfixe |10|11195→11171|357472000|×6,210|
| Permutée |5|3977→3868|123776000|×3,336|
| Permutée |10|9394→9394|300608000|×3,716|

Dans ces cas, le domaine positif demande sept visites d'index, zéro test
scalaire et admet n−2 complétions. Il ne déplace donc pas ici le carré
vers un scan scalaire systématique du domaine. Mais le pool reste limité
aux mêmes64 témoins, les tris26 sont encore payés et le résidu demeure.
Disk et Positive5 n'ajoutent aucun rejet dans les48 configurations.
Aucun budget de nœuds n'est épuisé. Augmenter simplement ce budget n'est
donc pas la suite justifiée par ces mesures. Les ratios favorables de
la permutation sur trois tailles ne prouvent pas une borne générale.

## Suite de développement

Combiner blocs témoins et traitement exact **local** du résidu plutôt
que rescanner le cover pour chaque face. Le futur état doit distinguer
intérieur certain max<0, extérieur certain min>0 et formes actives,
contacts inclus ; les listes du filtre27 retirant min≥0 et les comptes
comprimés ne constituent pas ce census exact. Payer fragmentation,
duplication des actives, tris locaux, propriété des frontières et coquilles.
Le dialogue avec A précise ces obligations ; ses fichiers en préparation
et ses expériences restent indépendants. Aucun changement du registre formel.

Contrôles de publication :615 Markdown actifs et20 phases du registre
validés. `git diff --cached --check` signale uniquement une ligne vide
finale dans le helper différentiel déjà épinglé ; elle est conservée
pour ne pas modifier rétroactivement ses hashes. Les autres fichiers
de la publication passent le contrôle d'espaces. Aucun audit indépendant
en préparation ni changement v6/v7 n'est inclus.
