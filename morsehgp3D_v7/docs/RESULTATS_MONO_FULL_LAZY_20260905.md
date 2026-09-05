# FULL mono : comparaison eager et cache facultatif

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Campagne close : sept réussites, un refus terminal

La nouvelle [politique lazy](CONTRAT_CACHE_FULL_PARESSEUX.md) est qualifiée
par [14 CTests ciblés dans chaque build Release et ASan/UBSan](../receipts/full_gabriel_lazy_20260905/README.md).
L'[admission de la sonde](../receipts/full_gabriel_lazy_probe_20260905/README.md)
est scellée séparément. La sonde v2 est distincte de la CLI F et de l'ancienne sonde FULL sans
digest. Son instrument de comparaison est spécifié dans le
[contrat sémantique](CONTRAT_DIGEST_FULL.md).
Les [huit captures lourdes](../receipts/full_gabriel_lazy_mono_20260905/README.md)
et leur [contrelecture normal/-O](../receipts/full_gabriel_lazy_mono_review_20260905/README.md)
sont closes : sept succès relatifs, un refus de budget à 32k, aucun timeout.
Les [contrôles first-C des huit captures](../receipts/full_gabriel_first_c_mono_20260905/README.md)
conservent 32 commandes en lecture seule, toutes code 0, avec le refus
moteur 32k code 2 intact. Un contrôle valide ne transforme pas ce refus
en réussite.
La première campagne interrompue reste séparée, ci-dessous.

Le binaire `1d5a38ce…`, compilé O3/NDEBUG avec les warnings en erreurs,
possède 40 dépendances utilisateur contrôlées. L'admission fonctionnelle
termine 24 tentatives à n=8 : s=8,10,12, Kmax=5 puis 10, avec eager et
lazy aux capacités 0, 1 et un million. Kmax=10 s'arrête naturellement à
K=n=8, sans ordres fictifs. Les 156 ordres sont complets relativement
aux catalogues fournis ; les digests coïncident entre politiques et s
pour chaque fenêtre. Les ordres K1..5 coïncident également entre fenêtres.

Onze refus de parsing donnent le code 2 attendu. Les 48 jugements de
reçus passent (normal et `-O`) ; les deux selftests du juge réfutent les
19 mutants. Le sérialiseur passe ses 24 sentinelles. Ces micros peuvent
chevaucher les compilations de l'auditeur et ne sont pas des mesures de
latence. La contrelecture distincte contrôle les artefacts, les digests
et les identités des compteurs sans relancer le moteur. Le
[supplément first-C qualifié séparément](../receipts/full_gabriel_first_c_qualification_20260905/README.md)
ajoute 48 contrôles sur ces captures, 12 mutants par mode et huit rejets
d'arguments ; aucune capture historique du juge v2 n'est réécrite.

## Protocole comparatif fixé avant les tentatives

Les six passages prévus portent sur uniforme/seed3/u16, 8 000 points et
les dix ordres horizontaux, avec un seul thread épinglé CPU6. À s=8 et
s=12, eager précède lazy ; à s=10, l'ordre est inversé. Le cache lazy
est limité à un million d'entrées, sans éviction. Les autres caps et le
générateur sont identiques ; `max_aliases=0` sélectionne explicitement
le contrat de la nouvelle API, pas un budget historique désactivé.

Chaque passage a un plafond mural de 600 s et une grâce de terminaison
de 10 s. Le processus baisse sa limite d'espace d'adressage à 26 GiB au
plus ; le proxy logique historique de payload reste 8 GiB. Ni cette
limite d'adresse ni le proxy ne sont présentés comme une borne de RSS.
Les tentatives lourdes ont commencé après la clôture des moteurs de
qualification indépendante, annoncée à 17:42:45 UTC. Les contrelectures
Python sont distinctes ; aucun moteur de qualification concurrent n'est
demandé pendant ces passages.

Le temps de référence va jusqu'au terminal et inclut calcul, lecture,
digests, sorties provisoires et libérations dans cet intervalle. Les
temps d'étapes et le pic GNU time sont conservés. Les échantillons RSS
par ordre arrivent après destruction du Builder : ils ne mesurent pas
isolément le dictionnaire d'alias vivant. Le HWM du processus est cumulatif.

Les deux bras utilisent le même digest. Les [anciens temps v1](RESULTATS_MONO_FULL_20260905.md)
ne sont pas des témoins appariés. Une paire par s sur hôte partagé reste
une observation ; elle ne qualifie ni une accélération statistique ni
le meilleur s. Aucune sélection du défaut s n'est faite sur ces micros.

Après six réussites 8k cohérentes, les admissions 16k puis 32k à s=8,
lazy et même capacité, sont distinctes et conditionnelles. Une tentative
refusée est conservée avec son motif, ses dépenses et son préfixe
diagnostique, sans digest global ni temps d'achèvement de la tour.

## Interruption conservée et reprise distincte

La [première campagne](../receipts/full_gabriel_lazy_interrupted_20260905/README.md)
est close en échec : eager 8k/s8 termine dix ordres en 149,951700395 s,
puis la capture lazy s'arrête à sa seule configuration. À la reprise,
la session n'est plus disponible et aucun processus correspondant ne
tourne. Aucun terminal ni code de sortie n'a été capturé : la cause,
la durée et la fin du processus lazy restent inconnues. Le plafond prévu
de 600 s n'est pas un timeout observé. La clôture administrative ne
reconstitue aucun artefact manquant.

La nouvelle campagne privée `heavy_paired_resume` répète les deux bras
sur les mêmes pins de sources et de binaire. L'ancien eager reste une
observation non appariée ; aucun ratio avec un nouveau lazy n'en sera
calculé. Le reçu clos de la nouvelle paire porte `50f22273…` ; les
six tentatives réussissent et les sources/binaire restent stables.

## Observations appariées à 8 000 points

Les six captures comparent chacune les dix ordres horizontaux. La
contrelecture retrouve les mêmes digests par ordre, le même digest
global `e6e3fa51…` et la même entrée `b7374475…`. Les 27 compteurs de
front sont identiques dans chaque paire ; ils peuvent varier entre s.
Les 30 comparaisons d'ordres respectent les identités de travail du
cache non saturé. Aucun skip ne survient à ces capacités et cette taille.

| s WSPD | Eager, s | Lazy C=1M, s | Pic eager, MiB | Pic lazy, MiB |
| --- | ---: | ---: | ---: | ---: |
| 8 | 140,956 | 142,787 | 1 790,559 | 1 292,445 |
| 10 | 137,899 | 142,968 | 1 790,680 | 1 291,516 |
| 12 | 141,521 | 143,196 | 1 790,629 | 1 290,777 |

Temps jusqu'au terminal, digest et sorties inclus ; pics du processus
GNU time convertis de KiB en MiB. Ce ne sont pas les échantillons RSS
par ordre après destruction du Builder. Le pic baisse d'environ 28 %,
mais les trois temps lazy augmentent de 1,18 à 3,68 %. Une paire par s
sur hôte partagé reste une observation, pas une qualification statistique.
Aucun meilleur s ni gain de temps du cache n'est retenu.

À K10, les 6 209 024 alias eager sont remplacés par 746 631 entrées
de cache lazy, en plus des minima et ancres obligatoires. Les visites de
facettes passent de 10 418 444 à 2 534 359. En contrepartie, 488 139
nouveaux misses J1 font passer les MEB de 714 823 à 1 202 962 ; les
456 331 pas de descente sont inchangés. Les query-nodes des portails
passent de 47 429 069 à 96 517 944. La diminution du dictionnaire ne
supprime donc pas son coût géométrique de remplacement.

La génération seule prend encore 54,69 à 58,48 s selon le passage et
le constructeur FULL 61,50 à 65,71 s. Ces temps d'étapes ne sont ni un
contrat de tour ni des bornes générales. Ils motivent deux prochains
deltas distincts : réduire les allocations des lots à une seule directe,
puis le travail de génération déjà rejetable par profondeur certifiée.
Ces deltas ne sont pas inclus dans les octets mesurés.

## Palier 16 000 points

La tentative s=8, lazy C=1M termine les dix ordres horizontaux en
319,304689 s, avec un pic GNU time de 2 652,406 MiB (2,590 GiB).
Le reçu de phase `d3656155…` est clos, sources et binaire stables.
Le digest d'entrée est `4acfbf3c…` et celui des dix forêts `e4ead0a4…` ;
ils ne sont pas comparés à ceux d'un nuage de taille différente.

La politique franchit le palier auparavant refusé à K9 sur les alias,
sans relever les autres plafonds. Ce n'est pas une comparaison de temps
avec l'ancien instrument. À K10, le cache conserve exactement un million
d'entrées et rend 677 513 résolutions sans insertion. Les 1 677 513 portails
et 1 000 201 pas entraînent 2 677 714 MEB. Les juges v2 et first-C passent
normalement et sous `-O`. Les égalités eager/lazy conditionnées à l'absence
de saturation ne sont pas appliquées aux ordres saturés K9 et K10.

## Palier 32 000 points : nouveau verrou de travail

Le palier 32k a sa propre admission sur la paire 8k close, avec les mêmes
caps, la même capacité et le même délai de 600 s. Il refuse à K9 avec
`resource_exhausted`, raison `full_gabriel_successor_budget`, code 2.
Le compteur a atteint exactement 128 000 000 opérations de successeurs.
Le cache contient un million d'entrées ; 1 313 632 résolutions ont été
rendues sans insertion dans cet ordre avant le refus.

La tentative dure 548,857375 s jusqu'au terminal et atteint un pic GNU
time de 5 130 376 KiB (4,893 GiB). **Ce n'est ni un timeout ni un temps
de tour achevée.** Huit ordres sont présents comme diagnostics ; aucune
forêt partielle globale ni digest de succès de tour n'est publié.
Le reçu de phase `86e0f1b2…` est clos, sources/binaire stables. Les juges
v2 et first-C passent normalement et sous `-O` : huit ordres lazy réussis
sont contrôlés, le neuvième ne reçoit que les bornes de préfixe refusé.

Les 3 418 030 MEB, 1 104 398 pas et 279 909 313 query-nodes déjà payés
restent visibles. Ils ne décrivent pas le coût complet de K9 ni de K10.
Le plafond d'alias n'est plus le premier obstacle ; normalisation des
ancres, travail géométrique et génération deviennent les cibles suivantes.
Le compteur de successeurs compte des opérations logiques déclarées,
pas toutes les opérations DSU ni un chrono. Une optimisation future doit
préserver ou versionner cette charge, jamais effacer les dépenses pour
franchir le plafond. Aucun cap n'a été relevé pour cette campagne.

## Contrats encore ouverts

Ces passages construisent les ordres horizontalement puis détruisent
chaque forêt ; ils ne publient ni archive, ni verticale inter-K, ni
supplément pondéré. L'égalité d'empreintes ne certifie pas la complétude
géométrique des catalogues. Les contrats 50k/tour entière en 1 s, puis
100 ms, et les dizaines de millions de points sur G4 restent non atteints.
La sonde présente est volontairement admise jusqu'à 32k seulement :
son parser et `max_points` refusent 50k, indépendamment du temps. Un
futur palier 50k devra avoir son instrument intégré, ses caps et son
admission propres. À 30M, ses quatre millions de nœuds ne suffiraient
même pas aux minima K1. Ce plafond de campagne n'est pas une limite
du format FULL, dont les identifiants sont u64 et les budgets déclarés
par l'appelant ; le relever ne qualifie pas la résidence des autres arènes.
GCP non utilisé.
