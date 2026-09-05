# État de livraison v7 — 5 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Chantier sur `main` uniquement. Cette entrée décrit le travail courant ;
les récits des anciens jalons sont retirés, leurs preuves restent liées.

## Objet à conserver

La [lecture mathématique](docs/AUDIT_NIVEAUX_GABRIEL_20260905.md),
contre-vérifiée par l'[auditeur](audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md),
fixe FULL : feuilles minima Gabriel de cardinal K, niveaux exacts,
multifusions véritables induites par les cofaces Gabriel de cardinal K+1
et parents pré-lot, sans imposer K+1 parents à une fusion. La couverture
se dérive des feuilles, sans fusionner deux identités par égalité de
points. Les portails silencieux résolvent les parents mais ne sont pas
des nœuds de sortie. Aucun Gamma exhaustif n'est nécessaire.

K=n est traité séparément. Les plateaux hors régularité, ancres verticales
et profils pondérés gardent leurs obligations propres. Les minima FULL
ne sont pas toutes les facettes contributrices des poids du manuscrit.
Un validateur structurel et une égalité de digests ne certifient pas la
complétude géométrique d'un fournisseur de catalogues.

## Code courant et preuves du delta

Le [producteur FULL](src/forest/full_gabriel.hpp) est
un composant horizontal autonome. Son succès reste relatif à des
catalogues fournis complets, exacts et réguliers. La CLI, l'archive et les reçus F
restent des témoins séparés, jamais requalifiés FULL par héritage.

`build_full_gabriel_order` garde la politique eager historique.
`build_full_gabriel_order_lazy` choisit explicitement le cache des
premières C résolutions strictes : C=0 est permis, sans éviction ni remise
à zéro des budgets. Les minima et les ancres fermées de toutes les
directes restent obligatoires, y compris les connexions sans fusion.
J=1 retrouve l'ancre F+z sans recalculer sa MEB. Le
[contrat du cache](docs/CONTRAT_CACHE_FULL_PARESSEUX.md) détaille ses limites.

Le [lot à une seule directe](docs/CONTRAT_LOT_UNITAIRE_FULL.md) emploie
désormais un tableau de quatre racines au lieu de la DSU locale. Les q
demandes restent dans le même ordre, même avec racines répétées ; les
minima simultanés, ancres no-op, budgets et 33 compteurs sont conservés.
Les lots multi-directes gardent le chemin général. La qualification
singleton ci-dessous appartient au header `21b77d29…`.

Le delta courant [normalisation v2](docs/CONTRAT_NORMALISATION_FULL.md)
supprime la dernière lecture/écriture redondante de compression, sans
changer l'état final des successeurs. Son API et la sonde v3 nomment le
nouveau calendrier de travail ; les plafonds numériques ne sont pas relevés.
Le header `85c27ab9…` passe ses propres [20/20 tests Release et
20/20 ASan/UBSan](receipts/full_gabriel_successor_20260905/README.md), avec
fuites contrôlées, 585 pins et huit binaires par build. Le nouveau gate
qualifie 560 préfixes, 180 paires FULL positives, 3 320 coupes et seize
admissions numériques distinctes attendues. Les 17 portes antérieures
sont rejouées, dont les 49/209 fautes d'allocation. La première compilation
échouée du test est conservée, sans réétiquette ; R2 repart de builds neufs.
Aussi sur `85c27ab9…`, le [rejeu indépendant](docs/CONTRAT_NORMALISATION_FULL.md#qualification-indépendante-du-même-header)
ferme 114 ordres, 912 sorties et 69 120 coupes par build O2/SAN,
avec forêts et 32 autres compteurs conservés, 16 plafonds exacts et
180 refus cap−1. Sa porte primitive juge 3 851 appels par build et
réfute deux mutants causaux ; les 20+20 CTests constructeur sont
contre-vérifiés sur captures seulement. Les jugements et pins sont
liés dans le contrat ; ils ne qualifient pas la performance.
Aucune modification q4 ou MEB. Les mesures ci-dessous restent historiques
dans les sections qui nomment leurs anciens headers ; les captures propres
à la normalisation sont distinguées ci-dessous.

| Preuve exécutée | Résultat et portée |
| --- | --- |
| [Qualification fraîche FULL/lazy/singleton/digest](receipts/full_gabriel_singleton_20260905/README.md) | 17/17 Release et 17/17 ASan/UBSan, LeakSanitizer actif ; sept binaires par build et 584 pins stables |
| Différentiel singleton inclus | 181 paires positives, 3 320 coupes ; mode budgets : 357 refus identiques, q2/q3/q4 et no-op consommé réellement exercés |
| Portes mémoire réexécutées | 49 pannes eager et 209 lazy, toutes refusées sans échappement ; ordinaux propres au nouveau programme |
| [Admission de la sonde v2](receipts/full_gabriel_lazy_probe_20260905/README.md) | 24 micros, 156 ordres, 11 rejets parser ; juge normal/-O et 19 mutants par mode |
| [Supplément first-C](receipts/full_gabriel_first_c_qualification_20260905/README.md) | 58 commandes : 48 contrôles, 2 selftests de 12 mutants, 8 rejets argv ; 117 ordres lazy par mode |
| [Audit indépendant du header singleton `21b77d29…`](audits/receipts_full_singleton_20260905/README.md) | 114 ordres, 912 sorties et 69 120 coupes par build O2/ASan-UBSan ; mutant du quatrième token réfuté, hors qualification des temps lourds |

Le [précontrôle 12/14](receipts/full_lazy_development_20260905/README.md)
reste un échec conservé : une fixture partagée n'était pas globalement
régulière. Sa contradiction devient un négatif permanent, pas un
relâchement du domaine. Les 14 anciennes portes sont réexécutées dans
les 17 tests ; la suite F complète ne l'est pas ici. Les trois anciens
mutants lazy restent attachés à leur pin historique ; ils ne sont pas
présentés comme rejoués par le nouvel audit singleton.

## Mesures courantes et contrats ouverts

La [campagne de normalisation v2](docs/RESULTATS_MONO_FULL_SUCCESSOR_20260905.md)
ferme les cinq captures prévues : 8k/s10 et s12 à 143,301 et 145,404 s,
16k/s8 à 321,643 s, puis refus 32k/K9 sur quatre millions d'appels MEB
après 567,439 s. Le premier temps 8k/s8 est exclu pour chevauchement
CPU signalé par l'auditeur ; son rejeu distinct termine à 138,221 s avec
dix ordres identiques hors mesures. Le paquet de 1 162 fichiers et
les 29 comparaisons historiques/204 ordres sont clos : le 32k n'a que
son préfixe réussi K1..8 comparé. Même forêt ne signifie pas gain de
temps robuste ni réussite du contrat 50k.

La [campagne singleton historique](docs/RESULTATS_MONO_FULL_SINGLETON_20260905.md)
compare les deux headers avec la même sonde, lazy C1M, à s=8/10/12.
Ses six passages sont clos, 30 ordres appariés identiques. Les temps
du nouveau code sont 144,337 / 145,201 / 145,544 s, avec des variations
avant/après de −0,83 % / +2,36 % / +0,07 % : aucune accélération robuste
établie malgré l'économie d'allocations sur les fixtures.
La [campagne lazy antérieure](docs/RESULTATS_MONO_FULL_LAZY_20260905.md),
header `13c6cc72…`, conserve ses propres six réussites appariées 8k,
son succès 16k (319,305 s) et son refus 32k/K9 sur 128 millions
d'opérations de successeurs. Le gain de pic d'environ 28 % opposait
eager à lazy, pas les deux headers de la campagne singleton. Son
[interruption initiale](receipts/full_gabriel_lazy_interrupted_20260905/README.md)
reste conservée sans terminal inventé.

Ces sondes détruisent chaque forêt après lecture : ni archive FULL, ni
verticale intégrée, ni mémoire de toute la tour retenue ne sont mesurées.
Les contrats **50k/tour 1..10 sous 1 s**, repli tour 1..5, puis 100 ms et
dizaines de millions sur G4 sont **non atteints**. Les plafonds de sortie,
catalogues, RAM/VRAM et reprise ne se résolvent pas par le seul cache.

Suite mono : qualifier le proposeur MEB privé filtré et considérer le
rejet précoce des blocs q4 sur sa preuve propre. Le coût MEB doit aussi être mesuré
sur les appels FULL réels avant un éventuel dispatch de proposition.
L'auditeur a précisé le [filtrage stable des pivots natifs](audits/MEB_DOUBLE_BUDGET_COURANT.md#réduction-démontrée-des-formes-de-pivot) :
violateur obligatoire, aucun q2 après l'initialisation par diamètre global,
première base acceptée conservée à P non limitant sur toute la séquence Work.
La préparation privée `build/v7_meb_filtered_preparation_20260905/pivot.hpp`,
pin `484a89bc…`, est relue mais **non compilée, non qualifiée et non intégrée**.
Elle ne change aucun résultat ni aucune source de la présente campagne.
L'export FULL transactionnel avec son autorité terminale et ses ancres
inter-K reste à raccorder avant qualification de la tour. Multi-CPU et
GPU viennent après la réduction du coût mono, avec leurs reçus séparés.
Les [fausses pistes](docs/FAUSSES_PISTES.md) ne sont pas réintroduites
comme raccourcis implicites.

## Témoins conservés, non réattribués

- [Port v6 et lecture des fondements](docs/LECTURE_ET_CONTRATS.md), avec [octets consommés](docs/V6_SOURCE_SNAPSHOT.json) : worktree v6 intact.
- [Qualification F 48/48/339](receipts/witness_stack_integrated_20260905/README.md) et [mesures F](docs/RESULTATS_MONO_F_20260905.md) : objet réduit, distinct de FULL.
- [Premier producteur FULL](receipts/full_gabriel_20260905/README.md) et [mesures eager historiques](docs/RESULTATS_MONO_FULL_20260905.md) : sources et instrument antérieurs, pas bras appariés de la sonde v2.
- [Primitives et autorités mathématiques](docs/QUALIFICATION_S1_PRIMITIVES.md), [MEB à double budget](docs/RESULTATS_MEB_DOUBLE_BUDGET_20260905.md) et [coût local défavorable](docs/RESULTATS_COUT_MEB_20260905.md) : qualification locale, pas accélération de tour intégrée.
- [G4 historique](docs/RESULTATS_G4_20260904.md) et [arrêt certifié historique](receipts/gcp_handoff_20260905.json) : aucune qualification FULL ou nouvelle mesure massive héritée.

GCP non utilisé pour ce delta ; aucune VM créée ou démarrée. Les CTests
locaux, la CI et les sessions G4 sont trois autorités distinctes. Aucun
succès CI d'un ancien commit n'est attribué automatiquement au nouveau.
Le registre officiel reste inchangé. Avant publication : contrôle des
documents, du registre et des octets des reçus effectivement dans l'index.
Ne pas indexer les fichiers de l'auditeur en préparation.
