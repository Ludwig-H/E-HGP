# Parallélisme de la sonde FULL

6 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Périmètre du raccord

La [sonde horizontale](../bench/full_gabriel_lazy_probe.cpp) accepte
`--threads=N`. Le raccord utilise les primitives existantes pour le front
WSPD, les corps de rectangles, le tri des candidats, le préfiltre, le
census et l'expansion des événements. Les compteurs `workers_*` rapportent
les équipes réellement créées ; N n'est que le budget demandé.

La construction de chaque forêt FULL, la boucle K, le comptage des
événements et les empreintes restent séquentiels. Il ne s'agit ni d'un
constructeur FULL GPU ni d'une tour intégrée avec ancres verticales et
archive. L'autorité horizontale reste conditionnelle aux catalogues
exacts, complets et réguliers fournis au constructeur.

L'absence de l'option conserve le schéma mono v5 et un thread. L'option
explicite, y compris `--threads=1`, sélectionne le schéma v6 et déclare
`pipeline_workers_full_order_serial_v1`, `full_order_builder_threads=1`
et `order_schedule=sequential_k1_to_kmax`. Les lecteurs mono historiques
ne sont pas élargis silencieusement au nouveau schéma.

Le [recorder direct](../bench/run_full_probe.py) accepte `--cpu-list`.
Pour N>1, cette affinité doit être explicite ; une liste singleton permet
de mesurer volontairement la surallocation. Aucun délai automatique ni
quota de travail n'est ajouté. Une sortie interrompue ou sans terminal
ne devient pas un succès.

## Expérience locale

La machine locale expose quatre cœurs physiques et huit CPU logiques :
0/1, 2/3, 4/5, 6/7. Le plan apparié emploie CPU6 pour un thread, 4,6 pour
deux et 0,2,4,6 pour quatre. Huit threads sur 0-7 sera un essai SMT distinct,
pas huit cœurs physiques. Même binaire, entrée uniforme seed3/u16,
s=8, K=1..10, cache lazy 1 000 000 et P=unlimited.

Les [micros publiés](../receipts/full_pipeline_threads_micro_20260906/README.md)
ferment les cinq appels n=8 sans option puis N=1/2/4/8 : huit ordres
effectifs par appel, mêmes empreintes et champs non mesurés d'ordre.
Neuf rejets de sonde et sept rejets de recorder ont le code attendu 2.
La comparaison Python normal/-O est identique. Ces micros ne mesurent
pas une accélération. Le raccord compile en C++20 Release O3 avec
`-Wall -Wextra -Wpedantic -Werror`.

Les [quatre mesures n=8000](../receipts/full_pipeline_threads_8000_20260906/README.md)
sont closes, sans benchmark concurrent, avec le même ELF `4f5ba475…`.

| Threads | CPU autorisés | Temps externe | Génération | Construction FULL | Pic RSS GNU |
| ---: | --- | ---: | ---: | ---: | ---: |
| 1 | 6 | 132,962 s | 59,562 s | 51,094 s | 1 290,5 Mio |
| 2 | 4,6 | 98,195 s | 30,093 s | 53,935 s | 1 398,6 Mio |
| 4 | 0,2,4,6 | 74,577 s | 15,483 s | 50,139 s | 1 452,1 Mio |
| 8 | 0-7, SMT | 69,853 s | 11,538 s | 50,765 s | 1 460,3 Mio |

Les dix empreintes d'ordre et l'empreinte finale concordent. Tous les
champs d'ordre hors mesures et la configuration hors nombre de threads
sont égaux. Seuls les six compteurs de workers diffèrent au terminal,
hors mesures : aucun changement des volumes, du travail compté ni des
pics logiques n'est masqué. Chaque étape parallèle atteint son budget
de workers sur cette taille.

Les rapports de temps externes observés sont ×1,354, ×1,783 et ×1,903
face au mono. Une seule observation par bras : pas de qualification
statistique. La génération accélère nettement, tandis que FULL reste
autour de 50 s. Le temps externe inclut davantage que les deux phases
isolées du tableau. Aucun de ces temps ne qualifie une tour intégrée.

## Travail entre ordres

Les API FULL prennent un index constant et les deux catalogues Gabriel
adjacents, pas la forêt de l'ordre précédent. Un parallélisme entre K
est donc envisageable avec Builder, DSU, cache et compteurs privés, puis
publication ordonnée par K. Le déplacement du catalogue dans la boucle
courante économise du stockage ; ce n'est pas une dépendance mathématique.

Le stockage des catalogues partagés, les espaces de construction
simultanés et les pics cumulés restent à mesurer. Il faut joindre tous
les travaux avant de libérer leurs données et conserver des refus
déterministes, sans exception échappant d'un thread. Cette étape n'est
pas implémentée par le seul paramètre `--threads`.

## Contrats

Les objectifs restent la tour complète 50k K=1..10 sous une seconde,
puis K=1..5 en repli et 100 ms après le premier jalon. Les dizaines de
millions de points et le GPU G4 nécessitent une qualification séparée.
Le wrapper GCP historique F/primitives device ne qualifie pas FULL.
GCP non utilisé pour ce raccord local. À la demande de l'utilisateur,
minimiser les dépenses : préparer et vérifier les changements localement,
ne pas répéter les campagnes closes sans hypothèse nouvelle, et réserver
une session SPOT courte et ciblée aux questions nécessitant réellement G4.

L'admission mémoire de la sonde est désormais corrigée et
[qualifiée séparément](../receipts/full_census_payload_20260906/README.md).
Le proxy historique réservait deux BallData par candidate, alors que le
census nominal v7 n'a plus qu'une destination. Les captures ci-dessus
ne sont pas modifiées rétroactivement. Le nouveau champ
`census_payload_accounting=preflight_survivor_then_direct_census_v2`
identifie cette admission par phases, distincte des capacités d'allocateur
et de la mémoire résidente globale.

La lecture du [census direct](../src/pipeline/expand.hpp) donne, avec U
candidates uniques et S survivantes, un proxy préfiltre de 176U octets
(candidates et deux populations Survivor), puis 144U+240S avant census
(candidates, Survivor et une seule BallData). Les tailles sont celles de
l'ABI mesurée ; le second tableau BallData n'existe que sous mutant.
Ces deux expressions remplacent le 608U historique dans la sonde seulement ;
le garde du moteur réduit `run.hpp` reste inchangé. Les 40 contrôles
arithmétiques passent en O2 et ASan/UBSan, ainsi que deux nouveaux CTests.
Quatre micros n=8, P=0/illimité et 1/4 threads, conservent les mêmes forêts.
Cette qualification locale n'est pas une mesure 50k ni une preuve de complétude.
Les capacités, index, piles et équipes restent hors de ce proxy nommé ;
la limite d'espace d'adressage est un mécanisme distinct.

La [session G4 SPOT suivante](RESULTATS_G4_FULL_20260906.md) ferme ensuite
deux vrais processus 50k/K10 et 50k/K5 avec 48 workers. L'admission et le
census passent, mais la régularité refuse avant tout ordre FULL, après
21,372 s et 5,646 s. Pas de temps de tour complète, ni de speedup par
rapport au local à n différent. La VM est confirmée `TERMINATED` et les
captures sont récupérées ; le verrou des coquilles supplémentaires est
transmis à l'auditeur avant de payer une autre campagne.
