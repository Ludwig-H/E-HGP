# Complément sur la sortie plate — 4 octobre 2026

Lecture de la recherche privée `build/v11-points-select/`, distincte du produit publié et sans qualification
native. Le dépôt principal est épinglé à `3f285274b1a5df7894bfcfacf18ab6de3aca7b18` ; l'acteur développeur est
laissé intact, y compris ses deux scripts Zoltan non suivis. Les sources privées ont été copiées avant lecture ;
les hashes et les éventuelles copies AFTER séparent toute dérive.

## Résultats nouveaux, bornés

1. **Complétion et lignée persistante.** `modele/scripts/modele_lib.py:574–621` utilise toutes les premières
   couvertures qualifiées, puis rattache le bruit lorsqu'un seul cluster sélectionné est atteint. Cela préserve
   une partition et traite tous les ex aequo de première qualification, mais diffère de la lignée du propriétaire
   P1 retenu. Témoin abstrait : première qualification du site0 au rayon1 sur A et B, rencontre au rayon2 ; P1 le
   fait entrer à2 dans le parent fermé. Si A seul est admissible/sélectionné, son intervalle est [1,2), la fonction
   complète pourtant site0 dans A par son ancienne première remontée. Ce n'est pas un défaut de partition : c'est
   un bras distinct, qui abandonne pour ce point la date/lignée persistantes. Nommer et mesurer séparément ce bras
   avant de le présenter comme « le long de sa lignée » ; le choix de compléter ne découle pas de la stabilité P1.

2. **Racine et taille minimale.** Sur un dendrogramme de deux points, `nary_head.head(mcs=3,asc=False)` rend
   correctement deux bruits ; `asc=True` rend un cluster des deux points. Le code privé n'exige pas n≥mcs.
   Déclarer explicitement cette exception de racine, ou retourner bruit/refus lorsque n<mcs avant la sélection.
   La politique par défaut est protégée ; le cas concerne seulement l'option qui admet la racine. Aucune
   assertion concernant sklearn public ou la production v11 n'en est déduite.

`check_followup.py` contrôle ces seuls cas nouveaux par AST figé et rationnels. Le premier est un état abstrait
de couverture/hiérarchie, sans réalisation Cloud entière revendiquée. Le second exécute les vraies fonctions
privées de tête sur deux feuilles. Aucun import du module de recherche, aucun sklearn/fit, natif, FULL ou GCP.
Normal et −O donnent la même sortie ; les gardes sont explicites, sans assert.

## Progrès confirmés et limites conservées

- `nary_head.py` a ajouté `d.exact` aux nœuds venant du tri exact ; `tower.check_blocks` recoupe maintenant
  directement ces valeurs contre l'oracle. L'atomisation N-aire et la politique ε cohérente restent en place.
  La variante ε de sklearn est un bras de diagnostic déclaré : l'exception de racine et l'asymétrie aux égalités
  sont déjà documentées et testées par la recherche privée ; aucun ancien reproche n'est réintroduit.
- Le lemme diagonal mcs≥2 reste valide. La lecture des entrées n'oblige pas à compter les singleton fantômes
  pour les clusters admissibles ; mcs1 et inactifs requièrent leur contrat propre.
- L'EOM n'a pas changé de statut numérique : tête N-aire en flottant ; modèle par intervalles avec choix forcé
  signalé au budget. Les gardes closes du reçu précédent ne sont pas rejouées. L'ordre exact des dates ne suffit
  pas à qualifier les sommes de réciproques ; un port exact exige signe/égalité/refus propres aux scores.
- La construction compacte sans matrice par paire est un **plan**, correctement déclaré dans
  `equite/RAPPORT.md:464–466`. L'implémentation actuelle `equite/tower.exact_pairs` demeure quadratique pour les
  petits oracles ; `lidar_lib.hier_from_hanging` conserve des partitions successives. Ne pas en déduire un coût
  natif O(n). Le balayage natif devrait produire directement les blocs de points N-aires, pliant les nœuds FULL
  vides/unaires, sans recopier une liste de membres par cluster ni construire une matrice de distances.

La sortie doit conserver un seul label par ID original, distinguer bruit/inactifs/refus, et publier sa politique
de racine, de ε et de complétion. Les choix de référence actuels sont utilisables ; les tableaux de recherche
locaux et leurs fits antérieurs ne sont ni rejoués ni transformés en qualification G4 par ce reçu.

Le relevé AFTER constate les neuf sources de code/autre rapport inchangées et une dérive du seul
`equite/RAPPORT.md`, conservée séparément (ba406d2f… → d52b5f09…). Ce delta documentaire précise les petits
oracles, le cas sans ancêtre sélectionné et l'instabilité EOM ; il ne change pas les deux fonctions exécutées
ici. La nouvelle copie ne remplace pas la capture BEFORE.

## Reproduction

```
python -B check_followup.py > checks.json
python -B -O check_followup.py > checks_optimized.json
```

Le manifeste SHA256SUMS racine couvre tous les fichiers, à l'exclusion de lui-même. Aucun produit, ancienne
capsule ou note active n'est modifié.
