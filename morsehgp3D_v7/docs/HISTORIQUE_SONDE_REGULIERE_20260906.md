# Historique de la sonde FULL régulière au 6 septembre 2026

Notes déplacées de la passation le 10 septembre pour séparer les témoins
réguliers antérieurs du nouveau raccord par boules. Les mentions « courant »,
« en cours » et « prochain » ci-dessous appartiennent au jalon du 6 septembre,
pas à l'état actuel. Les reçus liés conservent leurs pins et leurs limites.

## Code courant et preuves du delta

Le [producteur FULL](../src/forest/full_gabriel.hpp) est
un composant horizontal autonome. Son succès reste relatif à des
catalogues fournis complets, exacts et réguliers. La CLI, l'archive et les reçus F
restent des témoins séparés, jamais requalifiés FULL par héritage.

`build_full_gabriel_order` garde la politique eager historique.
`build_full_gabriel_order_lazy` choisit explicitement le cache des
premières C résolutions strictes : C=0 est permis, sans éviction ni remise
à zéro des budgets. Les minima et les ancres fermées de toutes les
directes restent obligatoires, y compris les connexions sans fusion.
J=1 retrouve l'ancre F+z sans recalculer sa MEB. Le
[contrat du cache](CONTRAT_CACHE_FULL_PARESSEUX.md) détaille ses limites.

Le [lot à une seule directe](CONTRAT_LOT_UNITAIRE_FULL.md) emploie
désormais un tableau de quatre racines au lieu de la DSU locale. Les q
demandes restent dans le même ordre, même avec racines répétées ; les
minima simultanés, ancres no-op, budgets et 33 compteurs sont conservés.
Les lots multi-directes gardent le chemin général. La qualification
singleton ci-dessous appartient au header `21b77d29…`.

Le nouveau [raccord MEB](CONTRAT_MEB_FULL.md) porte la proposition
filtrée dans les API eager et lazy, via `max_meb_proposal_supports`.
P=0 reste le défaut. Un Work persistant par ordre et le même Builder F
conservent les charges ; cinq diagnostics distinguent p, pivots,
certificats, replis et A physique. Le corps F est inchangé. Les données
historiques qui suivent restent attribuées à leurs propres headers.
Le [reçu propre au raccord](RESULTATS_MEB_FULL_20260906.md) ferme
30/30 CTests Release et 30/30 ASan/UBSan, avec 774 fautes d'allocation
par build, 1 488 sorties FULL / 33 792 coupes Gamma par mode de la porte,
quatre mutants réfutés et douze injections tardives par build séparé.
Les 9 344 comparaisons et 3 430 appels rationnels locaux sont recompilés
sur le header produit. Les deux incidents de configuration/compilation
restent conservés. Aucun nouveau temps de tour ni activation par défaut.

Le jalon [normalisation v2](CONTRAT_NORMALISATION_FULL.md)
supprime la dernière lecture/écriture redondante de compression, sans
changer l'état final des successeurs. Son API et la sonde v3 nomment le
nouveau calendrier de travail ; les plafonds numériques ne sont pas relevés.
Le header `85c27ab9…` passe ses propres [20/20 tests Release et
20/20 ASan/UBSan](../receipts/full_gabriel_successor_20260905/README.md), avec
fuites contrôlées, 585 pins et huit binaires par build. Le nouveau gate
qualifie 560 préfixes, 180 paires FULL positives, 3 320 coupes et seize
admissions numériques distinctes attendues. Les 17 portes antérieures
sont rejouées, dont les 49/209 fautes d'allocation. La première compilation
échouée du test est conservée, sans réétiquette ; R2 repart de builds neufs.
Aussi sur `85c27ab9…`, le [rejeu indépendant](CONTRAT_NORMALISATION_FULL.md#qualification-indépendante-du-même-header)
ferme 114 ordres, 912 sorties et 69 120 coupes par build O2/SAN,
avec forêts et 32 autres compteurs conservés, 16 plafonds exacts et
180 refus cap−1. Sa porte primitive juge 3 851 appels par build et
réfute deux mutants causaux ; les 20+20 CTests constructeur sont
contre-vérifiés sur captures seulement. Les jugements et pins sont
liés dans le contrat ; ils ne qualifient pas la performance.
Ce jalon ne modifiait ni q4 ni MEB. Les mesures ci-dessous restent historiques
dans les sections qui nomment leurs anciens headers ; les captures propres
à la normalisation sont distinguées ci-dessous.

| Preuve exécutée | Résultat et portée |
| --- | --- |
| [Qualification fraîche FULL/lazy/singleton/digest](../receipts/full_gabriel_singleton_20260905/README.md) | 17/17 Release et 17/17 ASan/UBSan, LeakSanitizer actif ; sept binaires par build et 584 pins stables |
| Différentiel singleton inclus | 181 paires positives, 3 320 coupes ; mode budgets : 357 refus identiques, q2/q3/q4 et no-op consommé réellement exercés |
| Portes mémoire réexécutées | 49 pannes eager et 209 lazy, toutes refusées sans échappement ; ordinaux propres au nouveau programme |
| [Admission de la sonde v2](../receipts/full_gabriel_lazy_probe_20260905/README.md) | 24 micros, 156 ordres, 11 rejets parser ; juge normal/-O et 19 mutants par mode |
| [Supplément first-C](../receipts/full_gabriel_first_c_qualification_20260905/README.md) | 58 commandes : 48 contrôles, 2 selftests de 12 mutants, 8 rejets argv ; 117 ordres lazy par mode |
| [Audit indépendant du header singleton `21b77d29…`](../audits/receipts_full_singleton_20260905/README.md) | 114 ordres, 912 sorties et 69 120 coupes par build O2/ASan-UBSan ; mutant du quatrième token réfuté, hors qualification des temps lourds |

Le [précontrôle 12/14](../receipts/full_lazy_development_20260905/README.md)
reste un échec conservé : une fixture partagée n'était pas globalement
régulière. Sa contradiction devient un négatif permanent, pas un
relâchement du domaine. Les 14 anciennes portes sont réexécutées dans
les 17 tests ; la suite F complète ne l'est pas ici. Les trois anciens
mutants lazy restent attachés à leur pin historique ; ils ne sont pas
présentés comme rejoués par le nouvel audit singleton.

## Mesures courantes et contrats ouverts

Le [triplet direct s8/P=unlimited](RESULTATS_MONO_FULL_SANS_QUOTAS_20260906.md)
termine maintenant les dix ordres à chaque taille : 133,038 / 307,643 /
684,574 s pour 8k/16k/32k. La croissance observée est sous-quadratique sur
ce régime uniforme, sans garantie générale ni qualification 50k. Les
[captures directes](../receipts/full_direct_scaling_20260906/README.md) gardent
les sorties brutes et leurs volumes. À 32k, K9 et K10 dépassent l'ancien
quota de quatre millions de MEB et terminent. Les résultats qui suivent
restent historiques, avec leurs propres plafonds et sources.

Le [réemploi terminal q2](ELIMINATION_BLOCS_WSPD.md) est intégré,
avec différentiel O2/ASan-UBSan et 19 CTests ciblés réussis. Le [nouveau
binaire à 8k](../receipts/full_wspd_q2_separation_20260906/README.md) termine
à 131,482 / 132,138 / 137,247 s pour s=8/10/12, mêmes dix forêts. Ni ces
trois passages ni l'écart q2 de −1,17 % à s8 ne prouvent un gain robuste.
La [paire P0/P∞ sur le même binaire antérieur](../receipts/full_direct_p0_comparison_20260906/README.md)
est close à 8k/s8 : 154,837 contre 133,047 s externes, mêmes forêts.
Les crédits h_a/h_b par sous-arbres sont corrects, mais non intégrés sur
le cas mesuré : aucun facteur >8 à 8k/s8, 93,819 ms au scalaire contre
186,560 ms aux blocs forcés, dans une sonde instrumentée. Le [terminal à un
seul passage](../receipts/wspd_terminal_once_negative_20260906/README.md) n'est
pas intégré : résultats identiques mais coins doublés sur le cas 8k,
sans gain mesuré sur le front O2. Cette paire est distincte des temps FULL.
Le [contrôle q2 positif](../receipts/wspd_q2_positive_core_20260906/README.md)
ajouté à la demande de l'auditeur passe O2/SAN et réfute l'omission de copie.
Le [triplet de deux amas](../receipts/wspd_large_factor_histograms_20260906/README.md)
ferme aussi 8k/16k/32k, avec 48k/96k/192k valeurs d'histogramme égales.
Les blocs positifs accélèrent q2 mais restent presque quadratiques ;
q4 ralentit à 32k. Pas d'intégration de cette variante seule. Le rejet
angulaire est prouvé par l'auditeur. Le [helper privé rejet/saturation](../receipts/wspd_noncredit_saturation_20260906/README.md)
passe 432 comparaisons O2/SAN, centre non-site et mutant Xi_max inclus,
avec comptes physiques distincts du volume logique. Aucun benchmark
lourd supplémentaire ni intégration produit de cette variante.
Le [raccord multi-CPU de la sonde](PARALLELISME_FULL_20260906.md)
est appliqué aux étapes déjà parallélisables. Cinq micros et seize rejets
passent. Les mesures appariées 8k terminent en 132,962 / 98,195 /
74,577 / 69,853 s externes à 1/2/4/8 threads, mêmes dix forêts et
mêmes champs d'ordre non mesurés. Le coût FULL reste autour de 50 s ;
`--threads` ne parallélise pas les ordres. Le dernier bras emploie le
SMT sur quatre cœurs physiques, pas huit cœurs. Dépenses GCP à minimiser.
L'[admission mémoire par phases](../receipts/full_census_payload_20260906/README.md)
est maintenant corrigée dans la sonde : 176U puis 144U+240S sur l'ABI
observée, contre l'ancien 608U. Les 40 contrôles O2/SAN, quatre micros
et deux nouveaux CTests passent. Aucun résultat antérieur n'est réétiqueté,
et le proxy logique ne devient pas une borne RSS. La session G4 SPOT
CPU48 FULL distincte est close : l'admission corrigée et le census passent
à 50k, puis les deux exécutions refusent sur la régularité. Démarrage et
arrêt ciblés sont certifiés ; aucune campagne massive supplémentaire.

La [campagne de normalisation v2](RESULTATS_MONO_FULL_SUCCESSOR_20260905.md)
ferme les cinq captures prévues : 8k/s10 et s12 à 143,301 et 145,404 s,
16k/s8 à 321,643 s, puis refus 32k/K9 sur quatre millions d'appels MEB
après 567,439 s. Le premier temps 8k/s8 est exclu pour chevauchement
CPU signalé par l'auditeur ; son rejeu distinct termine à 138,221 s avec
dix ordres identiques hors mesures. Le paquet de 1 162 fichiers et
les 29 comparaisons historiques/204 ordres sont clos : le 32k n'a que
son préfixe réussi K1..8 comparé. Même forêt ne signifie pas gain de
temps robuste ni réussite du contrat 50k.

La [campagne singleton historique](RESULTATS_MONO_FULL_SINGLETON_20260905.md)
compare les deux headers avec la même sonde, lazy C1M, à s=8/10/12.
Ses six passages sont clos, 30 ordres appariés identiques. Les temps
du nouveau code sont 144,337 / 145,201 / 145,544 s, avec des variations
avant/après de −0,83 % / +2,36 % / +0,07 % : aucune accélération robuste
établie malgré l'économie d'allocations sur les fixtures.
La [campagne lazy antérieure](RESULTATS_MONO_FULL_LAZY_20260905.md),
header `13c6cc72…`, conserve ses propres six réussites appariées 8k,
son succès 16k (319,305 s) et son refus 32k/K9 sur 128 millions
d'opérations de successeurs. Le gain de pic d'environ 28 % opposait
eager à lazy, pas les deux headers de la campagne singleton. Son
[interruption initiale](../receipts/full_gabriel_lazy_interrupted_20260905/README.md)
reste conservée sans terminal inventé.

Ces sondes détruisent chaque forêt après lecture : ni archive FULL, ni
verticale intégrée, ni mémoire de toute la tour retenue ne sont mesurées.
Les contrats **50k/tour 1..10 sous 1 s**, repli tour 1..5, puis 100 ms et
dizaines de millions sur G4 sont **non atteints**. Les plafonds de sortie,
catalogues, RAM/VRAM et reprise ne se résolvent pas par le seul cache.

Le [filtre MEB privé](RESULTATS_MEB_FILTREE_20260906.md), pin
`484a89bc…`, est désormais compilé et qualifié localement : 9 344 appels
F/Trace/NoObserver, 59 frontières ciblées et 3 430 appels jugés rationnellement
par build O2/ASan-UBSan, puis captures rejugées normalement et sous `-O`.
Le triangle passe de cinq formes à deux. La correction avec l'auditeur
établit une base positive unique dans le pivot admissible ; l'ordre des
essais affecte P et ses admissions. Le complément R2 réfute précisément
un changement de ce calendrier sur le tétraèdre régulier, à support égal.
**Ce reçu qualifie le helper privé, pas son nouveau port produit. Aucun
gain de tour n'est annoncé.**

Suite mono en cours : la [sonde v5](CONTRAT_SONDE_FULL_MEB.md) retire
les sept quotas arbitraires de travail FULL, les listes fermées de n/cache
et le garde du fold F non appelé. Elle publie P, le calendrier MEB et les
cinq diagnostics, avec `unlimited` explicite. Les types et admissions
mémoire restent contrôlés. La compilation fraîche passe, ainsi que
52 contrôles des limites par build O2/SAN et six CTests ciblés. La micro
r1 s'arrête après 36 configurations Kmax5 valides et une tentative Kmax10
sur un champ manquant du selftest first-C, corrigé séparément ; elle
**n'est pas une campagne entière réussie**. Les captures restent intactes.

Sur demande utilisateur, les mesures passent maintenant par un
[enregistreur direct](../bench/run_full_probe.py), avec suivi de progression
et arrêt si le calcul tourne en rond, sans nouvelle admission de format.
Comparer P0 et l'opt-in sur 8k/16k/32k et s=8/10/12 ; le premier triplet
s8/P=unlimited est clos, les autres cellules ne lui sont pas attribuées.
La paire P0/P∞ 8k/s8 et les trois s du binaire q2 sont également closes,
dans leurs paquets propres. L'utilisateur a confirmé la transition après
ce lot mono : multi-CPU local, puis G4 SPOT à 48 CPU et GPU. La sonde
propose maintenant le front multi-CPU, mesuré localement puis sur G4
CPU48 ; le traitement FULL reste séquentiel. Le contrôleur GCP historique
mesure F et des primitives device, pas la nouvelle tour FULL. Le premier
contrôleur de sonde FULL CPU est qualifié séparément et ses deux processus
50k refusent sur les coquilles supplémentaires avant tout ordre.
La priorité de génération demandée ensuite est l'[élimination par blocs
WSPD](ELIMINATION_BLOCS_WSPD.md), h_cœur/h_a/h_b : distinguer nouvelles
morts, tests de seuil mutualisés et comptages redondants supprimés.
Aucune activation par défaut ni extrapolation 50k/G4. La piste
distincte de [réutilisation des terminaisons certifiées](../audits/MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise)
demande de mesurer T−U, puis de conserver le premier recalcul complet et
de normaliser le token courant, sans mémoriser une racine périmée.
L'auditeur fournit désormais une [fixture n=12/K=7](../audits/receipts_filtered_review_20260906/terminal_reuse_fixture.md)
où deux visites du même terminal doivent suivre une racine qui a fusionné ;
le mutant de racine ancienne crée réellement une fausse fusion. Elle
servira au delta de réutilisation, pas à revendiquer ce mémo déjà livré.
Le rejet précoce des blocs q4 reste une autre optimisation à qualifier.
L'export FULL transactionnel avec son autorité terminale et ses ancres
inter-K reste à raccorder avant qualification de la tour. Multi-CPU et
GPU viennent après la réduction du coût mono, avec leurs reçus séparés.
Les [fausses pistes](FAUSSES_PISTES.md) ne sont pas réintroduites
comme raccourcis implicites.
