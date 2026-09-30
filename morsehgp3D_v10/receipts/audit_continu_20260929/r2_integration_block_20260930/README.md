# Raccord des correctifs et proposition de rejet par blocs

30 septembre 2026. Audit continu v10, `cpu_reference`, grille u18,
`public_status=not_claimed`. Quatre petites sondes sur un binaire existant,
contrôles scalaires indépendants et observations de campagnes déjà terminées.
Aucun moteur modifié ou reconstruit, aucun GCP. Les rapports restent dans
[le contre-audit R2](../../../audits/audit_continu_20260929/CONTRE_AUDIT_R2_20260930.md)
et [la question au développeur](../../../audits/audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md).

## CLI et tête numérique

[cli/receipt.json](cli/receipt.json) conserve les quatre appels, commandes,
codes, contenus et hashes avant/après, identiques. Deux refus numériques
code 2 sans sortie, un contrôle labels de 20 octets, et une collision
labels/arbre code 0 qui les remplace par 126 octets de texte. Le refus tardif
porte sur une deuxième configuration dans la même hiérarchie, pas sur
l'atomicité de tous les ordres ou entrées. Les copies CLI et tête restent
distinctes ; cette capture ne qualifie pas le binaire d'intégration.

[cli/analysis.json](cli/analysis.json) précise les limites. Les journaux
de construction et de portes sont des observations du développeur, pas
des rejeux supplémentaires par cet audit. Les sources ciblées sont copiées,
les binaires seulement hachés. Le script de sonde référence ses chemins
de travail d'origine et écrit des sorties : **ne pas le rejouer dans ce
paquet clos**.

## Campagnes Pool et SiteTree

[Le reçu terminal](terminals/README.md) sépare les clôtures et leur portée :
Pool catalogue/tour 24/24, clustering/mutual reachability 24/24,
oracles 2/2 ; SiteTree tour 7/7, têtes 18/18 et CTest 11/11.
Les comparateurs Pool conservent des préfixes SHA de 96 ou 64 bits, pas
les empreintes complètes ni les dumps. Les codes natifs et chaque ligne
ont été vérifiés ; une simple disparition de PID ne sert pas de terminal.

Le timeout de la sonde à barrière reste distinct d'un blocage du Pool.
Le mutant de chemin d'arrondi SiteTree survit, alors que le produit
actuel prend bien le repli exact en mode dirigé. Ce sont des réserves de
juge, pas de nouveaux défauts géométriques démontrés. Les sources et caches
observés après build n'établissent pas un gel préalable de toutes les
dépendances. Le manifeste développeur partiellement copié est une pièce
de provenance ; seul notre manifeste propre couvre cette capture complète.

## Indices et certificat de blocs

[scalar/check_block_rank.py](scalar/check_block_rank.py) n'appelle aucun
moteur. Deux diagnostics initiaux puis deux exécutions enregistrées
normal/−O sont listés dans [execution.json](scalar/execution.json).
[normal.json](scalar/normal.json) et [optimized.json](scalar/optimized.json)
sont identiques : douze valeurs de longueur, dont L=2³²−2 révélant le
débordement de `lo*64` ; 120 configurations rationnelles de certificat,
504 blocs testés, 126 rejets, 1 272 centres contrôlés. Une égalité exacte
doit être conservée. Aucun tableau de milliards de niveaux n'est alloué.

Ce certificat contrôle un rejet boîte de centres × bloc de sites à partir
de K témoins. Il ne mesure ni la sélection des témoins, ni un producteur
LiDAR, ni sa croissance. Les sources publiées sont celles effectivement
lues, pas une implémentation de la proposition. Aucune qualification
sous-quadratique, statistique, FULL/GPU ou 100 ms nouvelle.

## Vérification

`SHA256SUMS` à la racine ferme tous les fichiers, sauf lui-même.
Les trois manifestes propres `cli/SHA256SUMS`, `scalar/SHA256SUMS` et
`terminals/SHA256SUMS` sont aussi vérifiés depuis leurs dossiers.
Les chemins d'origine contenus dans les JSON restent de la provenance,
pas des destinations de fichiers actuelles.
