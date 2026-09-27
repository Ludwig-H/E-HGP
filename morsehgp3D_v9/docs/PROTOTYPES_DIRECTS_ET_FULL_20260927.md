# Bandes directes et écriture FULL : deux prototypes publiables

27 septembre 2026, suite de `24308be81`. Cadre
`phase=exploration_v9_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_parallelisation_structures`,
`public_status=not_claimed`. Objectif inchangé : tour FULL K1..5 explicite
en 100 ms sur G4, LiDAR sans sol prioritaire. Cette tranche ne modifie
ni le moteur ni ses défauts ; GCP non utilisé.

## Ce qui est désormais implémenté

### 1. Construire directement les bandes q3/q4

Le [nouveau propriétaire](../audits/b_q34_direct_bands_20260927/README.md)
prépare les crédits locaux puis les bandes, sans fabriquer les anciennes
cellules de classes A×B. Ce n'est plus l'ajout d'une représentation compacte
à une représentation ancienne déjà payée. Les facteurs, masques, témoins
et vingt compteurs géométriques restent identiques à la référence.

Sur les trois trames sans sol K5/s8, les préparations CPU locales appariées
ancien/nouveau donnent environ 975→638, 643→432 et 1 122→740 ms.
Sur 08/000000, les capacités complètes cumulées passent de 125,200 à
101,118 Mo ; le gain mémoire n'est donc pas celui des seuls descripteurs.
Les [mesures et limites](../receipts/q34_direct_bands_20260927/README.md)
incluent 8k/16k/32k, K10 et s10/12.

Attention à l'interprétation : ce sont **deux prototypes**, pas le moteur
GPU avant/après. Le nouveau corps de préparation permet aussi d'autres
spécialisations du compilateur ; on ne peut attribuer tout l'écart aux
seules bandes. Les chronos couvrent construction et allocations, pas la
destruction. Huit buffers de facteurs par rectangle demeurent. Il faut
encore une arène collective et un port parallèle, pas déplacer tels quels
ces objets sur GPU.

### 2. Raccorder les survivants sans changer leur sens

Une [porte de raccord](../audits/b_q34_batch_seam_20260927/README.md) vérifie
le vecteur complet et ordonné des survivants face au filtre CPU natif :
576 configurations, Release et sanitizers, deux mutants par build.
479 064 paires logiques sont représentées ; 388 468 seulement sont testées
ponctuellement, après 90 596 rejets implicites par les bandes. Les 380 712
survivants restent exactement les mêmes.

L'ordre des classes de crédits n'est pas l'ordre attendu par le moteur :
208 cas nécessitent un réordonnancement. On trie uniquement les survivants
avec leur ordinal original **avant S3** ; pas besoin d'un tableau de toutes
les paires éliminées. Le coût de ce tri reste à payer et à mesurer sur GPU.

Le moteur actuel confond dans un compteur la masse logique des rectangles
et le nombre de recherches ponctuelles. Au port, publier séparément P
(masse), E (paires réellement testées), rejets par blocs et rejets ponctuels.
Conserver les identités de couverture : remplacer P partout par E serait
faux. Aucun crédit du Pool n'est transmis au census.

### 3. Écrire la structure FULL par préfixes et incidences

Le [prototype d'encodeur](../audits/b_full_batch_encoder_20260927/README.md)
reconstruit les mêmes nœuds, parents, successeurs, contributions et mots
des niveaux que le constructeur natif. La validation des parents vivants
est reformulée avec des incidences regroupées par parent ; elle n'est pas
supprimée. Une réduction ordonnée conserve aussi le premier motif de refus,
même quand les contrôles sont exécutés dans un autre ordre.

Par build : 6 838 entrées, deux calendriers, 13 676 comparaisons. Les 404
entrées acceptées et 6 434 refusées sont jugées ; trois mutants échouent
causalement. Release et ASan/UBSan/LSan passent. La
[contrelecture](../audits/b_full_batch_review_20260927/README.md) confirme
les priorités d'erreurs, notamment le premier réemploi après une fusion.

Cela ouvre une écriture à destinations disjointes, mais **aucun thread ni
noyau GPU n'est encore exécuté par ce prototype**. Son tri scalaire peut
coûter plus cher que l'ancien contrôle CPU linéaire. Il consomme le draft
déjà calculé ; il ne construit ni les événements en amont ni les verticales.
L'objet miroir de cette porte n'est pas à lui seul la tour FULL du contrat.

## Sous-quadratique : ce que ces changements établissent

Les bandes changent la représentation, pas le résidu. Sur amas
8k/16k/32k, les 2,09/7,79/30,70 millions de paires restantes gardent leurs
ratios ×3,724 puis ×3,942. Aucun nouvel algorithme sous-quadratique global
en nombre de sites n'est démontré. Les mesures LiDAR sont prioritaires,
mais les trois trames 08/000000, 08/000100 et 08/000200 restent une seule
séquence. Les coupes capteur publiées précédemment ne sont pas de nouveaux
tests de cette tranche.

Le coût de préparation reste O(KF), plus regroupement des classes occupées
et lignes B. F est la somme des tailles des facteurs effectivement préparés,
pas n. L'encodeur est borné par la taille du draft et de ses incidences,
pas directement par n. Aucun test de cette tranche ne permet de transférer
ces bornes locales à toute la géométrie.

## Décisions pour le développement vers 100 ms

1. Garder les bandes et la préparation exacte ; remplacer leurs nombreux
   buffers par des arènes collectives avant port GPU. Mesurer préparation,
   transport, S2, réordonnancement et FULL ensemble. La porte de raccord
   fournit les invariants, pas un branchement public prêt à l'emploi.
2. Garder le préfiltre diamétral : la dernière
   [G4 close](../receipts/g4_core_warm_20260927/README.md) mesure environ
   923 ms ON contre 976 ms OFF à chaud sur une trame. Les bandes ne changent
   pas les survivants S2 ; elles ne supprimeront pas tout le travail aval.
3. Alimenter l'encodeur par de vrais drafts, profiler tri/préfixes/dispersion,
   puis porter ces opérations parallèles. Garder multifusions, continuations,
   plateaux et niveaux exacts. Son branchement ne remplace pas le chantier
   [événementiel de tous les ordres](../audits/FULL_PARTAGE_INTER_ORDRES_20260926.md).
4. Continuer le front GPU compact et la réduction du travail q3/q4 en
   parallèle. Ni un gain local de préparation, ni la suppression de l'ancien
   encodeur seul ne fournissent le facteur neuf manquant.

Les builds et reçus clos sont conservés ; aucun fichier d'un autre acteur
n'est supprimé. Le défaut public d'alias de banque reste un correctif
distinct à porter, pas une optimisation ni une corruption G4 démontrée.
