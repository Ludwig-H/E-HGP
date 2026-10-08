# Proposition native R : copie des enfants pour une classe à cellule unique

**Patch proposé, non appliqué au produit, non compilé, non mesuré.** Il transpose uniquement le
[critère prouvé `q = d + 1`](../registre_classe_unique/README.md), publié en `4707f6203`, à
`src/tower/registry_branches.cpp`. Pas de nouveau constat ni de qualification héritée du modèle.
La preuve suppose une sortie valide de T/M ; ce patch ne certifie pas les historiques publics de CST-0240.

`proposition.patch` ne change qu'un fichier. La base est `7398aed7d` ; le même corps est livré en
`bebb8609d`. `pins.json` épingle onze fichiers consultés : dix identiques entre ces commits, dont
les corps T/M/R et les tests unitaires ; `tests/tower/tests.cmake` a reçu depuis les portes FULL,
sans changer les groupes R cités ci-dessous. Les hashes comparés sont distinctement conservés.
`check.py` vérifie les pins et applique le patch dans deux copies temporaires ; c'est un contrôle
textuel, pas une compilation. Rejeux normal/−O identiques dans `verification.json`.
Une seconde contre-lecture statique du patch par l’auditeur mathématique confirme le raccord
au lemme, la garde du tampon absent, le compte physique et l’admission commune ; aucun natif joué.

## Modification proposée

`plan_row` parcourt le bloc contigu d'événements d'une cellule retenue et lit l'arité `q` de son
nœud M. Le même helper sert à l'admission initiale et à la préparation des offsets ; le travail
vaut zéro si `q == d + 1`, sinon le nombre de représentants de la cellule. Les contrôles locaux
rejettent une cellule/nœud hors domaine, un nœud naissance ou une arité impossible ; ils ne
remplacent ni la validation de l'entrée ni une certification complète des tableaux de M.

Une longueur nulle dans `branch_off` marque la voie directe. `collect_row` publie alors `q`
dans `branch_count` et ajoute `q` à `branches`, sans former de pointeur dans `branch_nodes`,
sans appeler `component_at` et sans relire les représentants. La seconde passe copie les enfants
M déjà triés. Les lignes générales conservent la recherche, le tri et l'unicité actuels.
Les cellules inertes ne figurent pas dans les blocs d'événements et n'entrent jamais dans ce critère.

`branch_reads` reste physique : il devient `Q_g`, le nombre de représentants des seules lignes
générales. Les autres compteurs et toutes les lignes/valeurs du registre doivent rester identiques.
Ne pas exiger l'égalité avant/après de ce compteur, ni conserver artificiellement sa valeur ancienne.
La politique demeure déterministe entre nombres de fils et découpages de M.

## Offsets, budget et concurrence

- Les lignes retenues conservent leur ordre ; les deux offsets égaux désignent seulement une ligne
  directe, jamais une ligne de sortie vide. Toute ligne retenue a au moins deux branches.
  `q <= births <= 2^31−1`, donc sa conversion en `u32` est sûre. Offsets et totaux restent en `u64`.
- `branch_nodes` est alloué seulement si `Q_g > 0`. Une tour entièrement directe ne forme aucun
  pointeur sur son tampon absent. Sa copie part de `children.val`, non vide puisque `q >= 2`.
  Avec une naissance et aucun événement, aucune tâche de ligne n'est créée ; les offsets terminaux
  d'un élément restent alloués. Les sommes préexistantes ne sont pas élargies : `Q_g <= Q_R`,
  et la sortie `A` est inchangée. Ce patch n'est pas un audit général des additions de l'allocateur.
- Première admission, par ordre : `12R + 8(R+1) + 4Q_g + 4R`, plus les tâches et compteurs
  comme auparavant. Deuxième admission inchangée : **`4A + 8(R+1)`**, après le comptage et avant
  toute allocation de sortie. `A` n'est pas borné par `B−1`. Aucun crédit anticipé de la sortie.
  Sans cache, le tampon temporaire économise exactement `4(Q_R−Q_g)` octets ; cela ne prédit
  ni le pic global, ni le RSS, ni l'arrondi des classes sous cache. Un ancien refus sous budget
  fini peut devenir un succès valide : l'identité de tous les refus n'est pas un objectif.
- M et V sont achevés avant R. `children`, `cell_node` et les offsets de travail sont immuables
  pendant les deux passes. Chaque tâche écrit ses lignes, ses compteurs et une plage CSR disjointe.
  La barrière du Pool précède le préfixe et l'allocation de sortie ; la seconde précède libération
  et agrégation. Aucune alias de sortie vers `children`, aucun nouveau tampon ni état partagé.

## Qualification à préparer sur G4 avant adoption

Ce protocole est proposé ; **aucune des exécutions ci-dessous n'a eu lieu dans ce reçu**.
Conserver une base et un candidat épinglés, mêmes options/builds/entrées ; session gardée, verrou
commun et arrêt certifié selon le contrat du dépôt. Déclarer ce changement de R avant la campagne.

1. **Branches réelles, pas seulement FUL1.** Jouer les groupes existants
   `mhgp12_tower_forest_branches`, `admission`, `determinisme`, `refus`, `hypergraphes`, `temoins`
   et `catalogue` (préfixe `mhgp12_tower_forest_` pour chaque groupe). Le groupe `branches`
   compare déjà chaque ligne à une coupe parent-par-parent indépendante et teste 27 branches
   pour seulement six événements. `same_registry` compare les tableaux publiés, dont
   `retained_cell/ball/rank` et `branches.off/val`. **FUL1 n'écrit pas ces cinq tableaux**
   (`export_full.cpp:92–128`) : son identité reste nécessaire pour la tour, mais ne prouve pas R.
2. **Portes causales à ajouter avant les temps.** Tout-direct avec une grande cellule et doublons
   (`Q_g=0`, zéro requête), singleton sans ligne, mélange direct/général, deux classes disjointes
   au même rang, sous-arêtes `{0,1}` puis `{0,2}` et inclusion `{0,1}` puis `{0,1,2}` au même
   rang (voie générale), cible cellule inerte de rang antérieur et cibles non régulières.
   Transposer les neuf fixtures du modèle publié sans affaiblir leurs comparaisons CSR.
   Dans le harnais interne, calculer indépendamment la somme des représentants des lignes
   générales : exiger `branch_reads == Q_g`, `branches == A`, offsets terminaux et tableaux exacts.
   Pour `Q_g=0`, l'admission doit exclure `4Q_R` et les écritures ne doivent pas toucher un tampon
   absent. La famille des préfixes à sept naissances reste à 27 branches/27 lectures générales.
3. **Mémoire et concurrence.** Rejouer l'admission par étage avec `rise <= admitted` sans cache,
   puis des budgets autour des besoins propres au candidat, y compris une sortie CSR non nulle
   et l'offset du cas vide. Vérifier absence de rétention sur refus/destruction. Comparer les CSR
   à W1/W3/W8/W48 et plusieurs tailles de tranche M ; garder un cas dépassant 2 048 lignes pour
   franchir les morceaux R. ASan/UBSan, mode poison et TSan ciblé sur G4 pour les voies directe,
   générale et mixte ; ne déduire aucune réussite de leur seule présence dans ce protocole.
4. **Mutants atteints.** Forcer la copie de tous les enfants doit être tué par les sous-arêtes ;
   remplacer `d+1` par `d` doit être tué par le compteur physique du tout-direct, même si la CSR
   reste correcte ; forcer la copie depuis `branch_nodes` pour une ligne directe doit être détecté par le cas
   `Q_g=0` sous instrumentation ; retirer les offsets de la seconde admission doit être tué par la porte
   admission. Démontrer l'atteinte et le motif du refus, pas seulement un code non nul quelconque.
5. **Temps après les portes.** Avant/après appariés, plan alterné et même nombre de processus/passes,
   u21/K5 puis K10, ng00–02 et la cohorte de 37 trames de K ; R mural complet, allocations et
   admissions comprises, TMVR et FULL résident. Publier `Q_R`, `Q_g`, `A`, mémoire, première passe
   et chaudes. Comparer les CSR mot pour mot hors chrono sur ces entrées ; ajouter au besoin une
   empreinte de registre avec tailles et tableaux explicitement encodés, distincte de FUL1.
   Écarter toute comparaison des seuls minima ou somme de médianes de sous-étapes. Réutiliser
   le lecteur FULL strict et préannoncer le test statistique avant d'adopter un gain.

Cette proposition vise un travail évitable et un tampon plus petit ; aucun gain en ms n'est
acquis. Ne pas soustraire une médiane R à une médiane FULL pour annoncer une projection : une
borne conditionnelle demanderait les différences par passe. Aucune portée u24/u32, GPU natif
de R, ni conformité nouvelle n'est revendiquée.

```sh
python -B CHEMIN_DU_RECU/check.py --repo DEPOT_GIT
python -B -O CHEMIN_DU_RECU/check.py --repo DEPOT_GIT
```

L'intégrateur doit relire puis appliquer le patch dans son propre worktree ; ce reçu n'a modifié
ni `src/`, ni les tests, ni `audits/`, ni aucun reçu publié.
