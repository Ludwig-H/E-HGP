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

Le [producteur FULL](src/forest/full_gabriel.hpp), pin `13c6cc72…`, est
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

| Preuve exécutée | Résultat et portée |
| --- | --- |
| [Qualification fraîche FULL/lazy/digest](receipts/full_gabriel_lazy_20260905/README.md) | 14/14 Release et 14/14 ASan/UBSan, LeakSanitizer actif ; six binaires par build et 582 pins stables |
| Portes lazy incluses | 81 appels, 3 192 coupes, 127 rejets et 434 pannes persistantes d'allocation par build ; aucune forêt partielle |
| [Admission de la sonde v2](receipts/full_gabriel_lazy_probe_20260905/README.md) | 24 micros, 156 ordres, 11 rejets parser ; juge normal/-O et 19 mutants par mode |
| [Supplément first-C](receipts/full_gabriel_first_c_qualification_20260905/README.md) | 58 commandes : 48 contrôles, 2 selftests de 12 mutants, 8 rejets argv ; 117 ordres lazy par mode |
| [Audit indépendant lazy](audits/CACHE_FULL_COURANT.md), commit `e7fa5da7` | 109 ordres, quatre politiques, 67 920 coupes par build ; trois mutations ciblées réfutées, hors qualification des temps lourds |

Le [précontrôle 12/14](receipts/full_lazy_development_20260905/README.md)
reste un échec conservé : une fixture partagée n'était pas globalement
régulière. Sa contradiction devient un négatif permanent, pas un
relâchement du domaine. Les sept anciennes portes eager/lecteur sont
réexécutées dans les 14 tests ; la suite F complète ne l'est pas ici.

## Mesures courantes et contrats ouverts

La [campagne mono lazy](docs/RESULTATS_MONO_FULL_LAZY_20260905.md) compare
eager et lazy sur le même instrument, avec digest de chaque forêt et
lecture inclus dans le temps. Elle conserve séparément la
[première tentative interrompue](receipts/full_gabriel_lazy_interrupted_20260905/README.md),
sans lui inventer de terminal. La paire neuve répète aussi eager.
Les huit nouvelles captures sont closes : six réussites appariées à 8k,
une réussite à 16k et un refus terminal à 32k, aucun timeout. À 8k le pic
diminue d'environ 28 %, mais aucun gain de temps n'est observé. 16k termine
dix ordres en 319,305 s ; 32k refuse à K9 sur les 128 millions d'opérations
de successeurs, sans relever les caps ni publier une tour partielle.

Ces sondes détruisent chaque forêt après lecture : ni archive FULL, ni
verticale intégrée, ni mémoire de toute la tour retenue ne sont mesurées.
Les contrats **50k/tour 1..10 sous 1 s**, repli tour 1..5, puis 100 ms et
dizaines de millions sur G4 sont **non atteints**. Les plafonds de sortie,
catalogues, RAM/VRAM et reprise ne se résolvent pas par le seul cache.

Suite mono : cibler la normalisation des ancres, les allocations des
lots unitaires et le coût de génération sur leurs preuves propres.
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
