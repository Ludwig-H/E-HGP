# Reprise du 27 septembre : publier les pistes vérifiées, conserver les limites

Suite publiée : [bandes directement construites, raccord ordonné et
encodeur structurel FULL](PROTOTYPES_DIRECTS_ET_FULL_20260927.md).
Le dernier paragraphe ci-dessous décrit la perte constatée **avant** cette
reconstruction ; l'encodeur est maintenant retesté avec de nouveaux reçus.
Ce document conserve le périmètre du premier lot du 27 septembre.

Base `ddf4776d7`, grille entière 1 mm, objectif tour **FULL K1..5 explicite
en 100 ms sur G4**, sans sol prioritaire. Exploration hors registre,
`public_status=not_claimed`. Cette tranche publie du code de prototype et
des tests ; elle ne change pas encore les défauts ni le moteur.

## Deux résultats utiles

| travail | résultat établi | décision de développement |
| --- | --- | --- |
| Bandes q3/q4 sur les classes de crédits | mêmes paires et masques ; descripteurs LiDAR 34,810 → 4,913 Mo | conserver le format compact ; remplacer les anciennes cellules avant de mesurer un gain net |
| G4, préfiltre diamétral ON/OFF | FULL chaude 923,417 ms ON contre 975,936 ms OFF | conserver le préfiltre ; le supprimer est une régression sur ce cas |

Les [bandes](../audits/b_q34_bands_20260927/README.md) passent 18 928 cas,
4,46 millions de comparaisons de paires, les sanitizers et deux mutations
natives dans chaque build. Les [dix mesures locales](../receipts/q34_bands_20260927/README.md)
couvrent uniforme/terrain/amas 8k/16k/32k et une trame LiDAR entière.
Sur cette trame, fabriquer les bandes **en plus** du plan de référence coûte
49,377 ms CPU local. Cela n'est pas un gain de chaîne. Les facteurs restent
stockés, les capacités sont cumulées et non des pics RAM/VRAM.

La [campagne G4](../receipts/g4_core_warm_20260927/README.md) ferme six
processus : deux ON et deux OFF GPU, quatre passages par processus,
et deux témoins CPU. Les trois digests sont identiques dans les 18 passages ;
chaque bras GPU garde aussi le même travail de certification que son témoin
CPU de même option. ON gagne 48,582 puis 56,457 ms sur les médianes chaudes
appariées. Au premier passage, les médianes valent 943,023 et 993,804 ms.
La chaleur économise donc environ 20 ms côté ON, pas le facteur neuf requis.
Les sous-chronos détaillés ne sont publiés que pour le premier passage.

Une seule entrée : 08/000000 sans sol, 39 885 sites, K5/s8/W48.
Les répétitions d'une entrée ne sont pas plusieurs scènes. Segmentation,
lecture et digests restent hors chrono de chaîne ; aucun nouveau test
K10/brut/s10/s12/multi-séquence dans cette séance. Leurs reçus antérieurs
restent inchangés, sans transfert de qualification.

G4 SPOT utilisée 248,353 secondes d'allocation, sans montant facturé inféré.
Génération `2026-09-27T05:45:14.960Z`, arrêt `05:49:23.313Z`, même
génération relue `TERMINATED`. Le premier essai local a été refusé avant
tout appel GCP pour le mode de la clé éphémère ; la relance a explicitement
fixé 0600. Le refus est conservé dans les
[contrôles de publication](../receipts/g4_core_warm_checks_20260927/README.md).

## Ce qui reste à gagner

1. Construire directement les bandes dans une arène collective, sans les
   anciennes cellules ni allocations par rectangle. Préserver exactement
   masques, propriété des facteurs, offsets 64 bits globaux et ordre requis
   par les consommateurs. Mesurer préparation + transport + S2 + aval FULL.
2. Porter le front compact par vagues sur GPU et réduire les lectures
   répétées de témoins. Les bandes seules ne réduisent pas le résidu E :
   amas 8k/16k/32k conserve ×3,724 puis ×3,942. Les coupes spatiales LiDAR
   du plan précédent ne deviennent pas de nouvelles mesures ici.
3. Implémenter l'écriture parallèle FULL et le raccord événementiel de
   [tous les ordres](../audits/FULL_PARTAGE_INTER_ORDRES_20260926.md), en
   gardant multifusions, continuations, plateaux, verticales et parents
   explicites. Aucun encodeur parallèle FULL n'est qualifié par cette tranche.

Ne pas chercher les 100 ms dans un simple changement de défaut : les
coûts q3/q4, catalogue et tour restent tous significatifs. Ne pas annuler
des temps de phases recouvertes comme s'ils étaient additifs.

## Point de correction indépendant des performances

L'[audit de banque FULL](../audits/b_population_alias_20260927/README.md)
reproduit un alias mutable conservé par la factory publique prenant un
vecteur déplacé. La lecture d'une forêt déjà construite peut changer.
L'appel interne du moteur utilise un stockage privé : pas de corruption
des résultats G4 démontrée. Corriger la frontière publique tout en réservant
l'adoption sans copie à une construction interne réellement possédée.

## Continuité et preuve

Le worktree `/tmp/mhgp9-audit-resume-20260926` n'était plus présent à la
reprise. Les commits publiés restent disponibles. Les brouillons non
publiés de bandes, d'encodeur FULL et d'alias ne sont pas des livrables
accessibles ni des preuves héritables ; quelques anciens binaires seuls
ne suffisent pas. Bandes et alias ont été reconstruits et retestés dans un
worktree persistant, avec nouvelles captures datées. L'encodeur FULL reste
à reconstruire. Le worktree racine v8 et les autres acteurs sont préservés.
