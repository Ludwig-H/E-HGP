# Dialogue courant de l'auditeur indépendant B (v8)

14 septembre 2026, après **e3af11a7**, sur main. Second auditeur du dossier
`morsehgp3D_v8/audits/`, arrivé ce jour ; l'auditeur indépendant A conserve
[DIALOGUE_COURANT.md](DIALOGUE_COURANT.md) et ses notes P0_* ; l'auditeur
complémentaire conserve `audits/morsehgp3D_v8_complementaire/`. Écritures
limitées à ce dossier. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Verdict de la campagne adversariale : aucun défaut du produit

Huit dimensions ont été attaquées par des agents indépendants sur
1bf806f0 (prédicats et bornes, tubes, filtre axial et census, robustesse
et mutants, objet mathématique contre le manuscrit, trajectoire et
contrat, corpus d'audit, oracle de référence), puis chaque constat a été
soumis à trois réfutateurs. Aucun défaut d'exactitude n'a survécu : les
harnais exacts (≈ 8 300 nuages adversariaux, 5 006 triplets de boîtes,
1 728 plans, 1 357 rectangles pleine échelle u16, 997 rectangles à la
frontière `D = 10R`) concordent avec le code, et sept mutants causaux sont
tués par les portes enregistrées. Builds neufs : 34/34 CTests à 1bf806f0
en Release et sous Clang ASan/UBSan ; 37 tests à 85015a8c avec un seul
échec, intermittent et documenté ci-dessous. Détail et fixtures dans
[PORTES_ET_TESTS_20260914.md](PORTES_ET_TESTS_20260914.md).

Ce qui survit est de trois natures : une lacune de couverture confirmée
(le mutant « égalité W3/W4 créditée » passe toute la suite), des points
de doctrine à écrire avant q3/q4 et FULL
([VERROUS_MATHEMATIQUES_20260914.md](VERROUS_MATHEMATIQUES_20260914.md)),
et des ordres de grandeur pour le contrat
([BUDGET_CONTRAT_50K_20260914.md](BUDGET_CONTRAT_50K_20260914.md)).

## Réponses aux questions du journal encore ouvertes

**Produits ancêtres avant séparation (question du 14 septembre).** Sûr sans
hypothèse de séparation, utile seulement quand la lentille des boules
diamétrales est non vide, testable exactement avant toute recherche
d'index ; transmettre les identifiants de témoins du parent aux enfants
avec exclusion, jamais les rejets négatifs ; ledger de masse par lane.
Détail et chiffres dans la note des verrous, §6.

**Question d'ouverture n°4 (graphe daté et contraction parallèle).** Le
K-MST de la définition 30 ne peut pas servir de définition : la
proposition 6 est fausse en position générale (E5, registre). Les
obligations minimales sont celles du reçu v7
`receipts_gabriel_vertices_20260906` §3 : sommets = minima Gabriel stricts
datés, poids = premier niveau de connexion dans Γ_K, forêt à L − R liens,
tout lot de même niveau fermé atomiquement, coupes ouvertes et fermées.
Rien n'oblige la géométrie à suivre le calendrier ; la contraction
parallèle ne peut être qualifiée qu'après le port de cette définition.

**Question d'ouverture n°5 (sortie implicite).** Hors périmètre P0, mais
elle conditionne le repli 100 ms : 27,3 M nœuds à 64 octets valent 1,75 Go,
soit environ 63 ms d'écriture seule mesurés ici. Toute représentation implicite doit
être un contrat distinct nommant les requêtes conservées et leur coût
d'expansion ; ne pas y répondre revient à renoncer au 100 ms en silence.

## Vérité terrain disponible pour la tranche FULL minimale

`reference/morsehgp3d_oracle` calcule l'objet de la thèse en rationnels
exacts, mais ses deux entrées ne se valent pas. `run_oracle` et le contrat
v2 refusent toute coquille cosphérique pertinente : le carré, le triangle
avec un point sur sa sphère diamétrale et même le nuage d'exemple à quatre
points sont rejetés (`UnsupportedDegeneracyError`), donc presque toutes
les fixtures de plateaux u16. En revanche `build_exhaustive_hierarchy`
(Γ_K par définition, forêt par lots de niveau exact, verticales
naturelles) accepte carré, octaèdre, pyramide, coquille à sept sites et
extrêmes u16, sans hypothèse de position générale ; coût 1,3 / 4,1 /
14,2 s à n = 8 / 10 / 12 pour Kmax = 3. Les doublons y sont acceptés
comme labels distincts de même position (multiplicité), alors que
`run_oracle` les refuse : fixer côté v8 la sémantique des doublons avant le
différentiel. Contrat de sortie minimal confrontable sans adaptateur : par
ordre K, nœuds avec facette témoin de K identifiants, niveau ρ² en fraction
réduite, parents d'arité libre, union des points couverts, coupes ouverte
et fermée à chaque niveau de lot. Familles à graver : les sept plateaux v7
(abcz, carré, triangle rectangle, pont extérieur, abczxy, coquille 7,
tétraèdre origine), E5, octaèdre, pyramide carrée, doublons, extrêmes,
colinéaires, plus des graines aléatoires u16 à n ≤ 12.

## Premier front réel (da366f7f) : lentille réfutée, propagation chiffrée

Le constructeur avait raison sur le test de lentille : mesuré sur une copie
instrumentée de son front, seuls 0 à 1 % des 63,5 millions de recherches
(uniforme 32k) portent sur un produit à lentille vide ; 71 % des recherches
ne rejettent rien faute de proposition. En revanche, la transmission des
témoins certifiés du parent à ses enfants (par voie, rangs distincts, sûre
par restriction) réduit le résidu à 32k uniforme à ×0,40 (q2), ×0,53 (q3),
×0,60 (q4), les rectangles émis à ×0,70 et les visites à ×0,80, à
prédicats, proposeur et seuils inchangés ; 0 rejet non sûr en force brute
exacte sur 24 vérifications. Sur huit amas et rangées, le résidu ne bouge
pas : ce sont les régimes des groupes collectifs. Note et reçu rejouable :
[PROPAGATION_TEMOINS_20260914.md](PROPAGATION_TEMOINS_20260914.md). Le
prototype est un outil d'audit ; compiler contre une extraction de
da366f7f, le worktree partagé bougeant déjà (`front.hpp`). Build neuf à
da366f7f : 40/40 CTests.

Réponse à la demande du constructeur : oui au raccord direct du census sur
les nœuds B du front ; la propagation ci-dessus et les blocs Z certifiés
pendant la descente sont les deux gisements du front lui-même, le premier
étant mesuré, le second restant à prototyper.

## Le régime « amas » se ferme avec les crédits P0 existants

Les tranches huit à dix mesurent les amas quadratiques (173,5 s à 32k,
visites ×4,2) parce que le raccord front → census ne rejette que par
témoins extérieurs, absents des produits inter-amas. Mesuré sur da366f7f
et la fixture `clusters` du constructeur : 28 rectangles terminaux à gros
facteurs portent 112 des 116,8 millions de paires q2 du résidu à 16k ;
Pool sur ces 28 rectangles coûte 120 ms et y laisse 29 878 paires
(×0,0003) ; à 8k, 85 ms et 11 329 paires. DualBlocks fait mieux pour
q3/q4 (301 k / 370 k au lieu de 2,1 M / 3,2 M à 8k) pour 1,5 s. Sur
l'uniforme et le terrain, aucun rectangle n'atteint 64 sites et Pool ne
change rien : une politique par taille de facteur suffit. Les tubes, eux,
ne créditent rien sur ces amas irréguliers (largeur de cellule fixée à
quatre unités réelles : un site par cellule), et restent quarante fois
moins sélectifs que Pool à leur meilleure largeur ; ils avaient été
qualifiés sur des grilles alignées. Note et reçu :
[CREDITS_TERMINAUX_20260914.md](CREDITS_TERMINAUX_20260914.md).

## Correction acquittée et contrelecture du census sur produit A×B×Z

L'auditeur A a raison : `h_q ≤ h_{q_min}` rend un rejet de lane sûr
**pour les présentations de support q**, pas inerte pour la boule (mon
propre exemple q_min = 2, p = Kmax − 1 le montre). La note des verrous §2
est corrigée : ce qui rend le rejet sûr est la rétention par la lane
minimale ; seule `p ≥ h_{q_min}` prouve l'inertie. Merci.

Le constructeur demande une contrelecture de la variante de census où la
tâche porte un produit de requêtes U×V et un bloc de témoins Z. Lecture
favorable, avec quatre points à tenir :

1. **Extrema exacts sur trois boîtes.** Le minimum de H sur U×V×Z est
   `h_minimum(U, V, Z)` (séparable, affine en a et b, concave en z ; exact,
   vérifié par 5 006 triplets). Le maximum sur U×V×Z est le maximum, sur
   les huit coins b₀ de V, de `h_maximum_times_four(U, b₀, Z)/4` : H est
   affine en b, donc son maximum sur la boîte V est atteint à un coin, et
   le maximum sur U×Z à b₀ fixé est déjà exact. Aucune nouvelle primitive
   n'est nécessaire ; la symétrie a↔b permet de choisir le petit facteur.
2. **Invariant de préfixe.** Avec l'ordre Z fixe et ses échappements, la
   preuve de §9.1 de A se transporte mot pour mot au produit : un bloc
   décidé (L > 0 crédite sa population à toutes les paires de U×V ;
   M ≤ 0 l'écarte pour toutes) avance le curseur ; un bloc indécis descend
   à son premier enfant ; scinder U ou V donne deux produits disjoints qui
   héritent du même curseur et du même compte, sans avancer Z. Le compte
   est exact et uniforme sur le produit pour le préfixe consommé, saturé à
   Kmax, donc un rejet à saturation vaut pour toutes ses paires.
3. **Coquille et collecte.** M ≤ 0 ne dit rien de la coquille (M = 0
   possible) : la collecte des intérieurs et de la coquille reste par
   paire depuis la racine, comme aujourd'hui ; ne pas réemployer le
   préfixe de comptage pour elle.
4. **Efficacité, pas sûreté.** Un crédit uniforme L > 0 exige que Z soit
   dans la lentille du produit ; sur un rectangle séparé, la lentille
   contient la région centrale mais pas les voisinages des facteurs, où
   se trouvent pourtant les premiers intérieurs des petites boules. Les
   blocs proches des facteurs forceront la scission de U et V jusqu'aux
   singletons : mesurer la part des crédits obtenus uniformément et le
   nombre de tâches, comme pour Shared, avant de conclure. Les bornes de
   produit sont plus lâches que les bornes de paire : garder le chemin
   singleton à bornes exactes de paire.

Directive utilisateur relayée par A, notée : aucune hypothèse
d'alignement ; les colonnes exactes restent un chemin facultatif, ce qui
rejoint mes constats sur les nappes et le régime réel. Sur les scans
KITTI 08 de A, le front Pure émet 18,4 millions de rectangles à 50k avec
la convention v8, du même ordre que mes séries synthétiques.

## Sixième tranche lue, non requalifiée

Le partage du nuage et de l'index Z (85015a8c) répond au coût fixe par
rectangle signalé hier ; ses 66 essais sont lus, non rejoués, et le
constructeur en tire lui-même la bonne conclusion : un terme Ω(R|B|)
subsiste dans Pool/Axis, la subdivision affaiblit les minorants (35,6 M
candidates sur la nappe 32k contre 3,9 M sans subdivision), l'arbre de
plages originales est un pont. Le pilote de front en chantier doit porter
la convention de `s`, le test de lentille et les compteurs du reçu de
régime ; je le relirai dès sa publication.

## Entretien du dossier

Fichiers de B : ce dialogue, six notes datées et les reçus
`wspd_regime_20260914/`, `propagation_temoins_20260914/` et
`credits_terminaux_20260914/`. Aucun fichier des autres auditeurs ni du
constructeur n'est modifié. Propositions d'archivage, à exécuter par
l'auteur A puisqu'elles touchent ses liens : `P0_RECTANGLE_CHECKS.json` et
`p0_rectangle_probe.cpp` (queues 1D, dépassées par le filtre additif),
`P0_AXIS_UNION_CHECKS.json` et `p0_axis_union_probe.py` (addition portée à
f5430f57), `P0_TUBES_CHECKS.json` et `p0_tube_probe.py` (modèle intégré à
3589a2c9) ; la note `P0_TUBES_ET_RANGS.md` doit rester en place, un reçu
immuable l'épingle. Ne pas déplacer `P0_INPUT_ALIAS_CHECKS.json` (épinglé
par `receipts/p0_local_credits_20260913/QUALIFICATION.json`),
`p0_q2_census_bounds_probe.py` (importé par la sonde de collecte) ni
`p0_collective_probe.py` (rejoué par un reçu complémentaire). Précédent à
ne pas répéter : `P0_OWNER_CHECKS.json`, supprimé à 7f4ba045, reste cité
par ce même reçu immuable ; il se retrouve par `git show 7f4ba045^:…`.
Signalé à l'auditeur complémentaire : sa note d'identités cite un reçu
`CLOUD_RECTANGLE_IDENTITY_CHECKS.json` absent du dossier, et son reçu des
tubes épingle d'anciens octets de `tube_credits.hpp`. Contrôles : les
Markdown de ce dossier hors périmètre canonique sont validés par la
fonction `validate` de `tools/check_docs.py` ; reçu rejoué en `python3 -O`.
