# Dialogue courant de l'auditeur indépendant B (v8)

14 septembre 2026, après **e3af11a7**, sur main. Second auditeur du dossier
`morsehgp3D_v8/audits/`, arrivé ce jour ; l'auditeur indépendant A conserve
[DIALOGUE_COURANT.md](DIALOGUE_COURANT.md) et ses notes P0_* ; l'auditeur
complémentaire conserve `audits/morsehgp3D_v8_complementaire/`. Écritures
limitées à ce dossier. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## État de l'audit B au 15 septembre 2026

Chaque tranche publiée du chantier v8 a été confrontée à la force brute
exacte (paires q2 : intérieurs stricts, coquille complète, clé) sur des
sources identiques, haché par haché, aux blobs du commit indiqué ; reçus
dans `chaine_q2_20260914/`, rejouables par `git archive <commit>`.

| Tranche | Commit | Reçu | Résultat |
| --- | --- | --- | --- |
| Front + census q2, frère, ordre Complement | e3af11a7 | `CHAINE_Q2_CHECKS.json` | 86 × 8 combinaisons, 104,7 M paires, 0 désaccord |
| Census conjoint A × B (SharedProduct / SharedAnchors) | b2106c3c | `CHAINE_Q2_JOINT_CHECKS.json` | 86 × 18, 235,7 M paires, 0 désaccord, admission conjointe nulle comme prédit |
| Filtre Pool terminal (`pool_min_factor`) | ba11e3ab | `CHAINE_Q2_POOL_CHECKS.json` | 86 × 25, 327,3 M paires, 0 désaccord ; seuil 64 = 99,94 % de la masse filtrable pour F/57 |
| Workers du front et du census | b268cf6f | `CHAINE_Q2_PARALLEL_CHECKS.json`, `…_BALANCE_…` | 4 348 appels, W ∈ {1,2,3,4,8}, condensé et 22 compteurs identiques ; ×4,1 à ×4,9 à W = 8 ; rangées déséquilibrées |
| Redistribution dynamique (Donate) | 4e878754 | `CHAINE_Q2_DONATE_CHECKS.json` | 13 044 appels, 1,95 M dons tous repris, 5 160 appels de vivacité sans blocage ; neutre sur familles équilibrées |
| Continuations de census à ancre unique | d09e2207 | `CHAINE_Q2_RESUME_CHECKS.json` | 258 624 continuations (budgets 1/3/1000, transfert de fil), 0 désaccord, pauses des trois types exercées |
| Détachement intérieur et ordonnanceur par ancre | 897085f8 | `CHAINE_Q2_DETACH_CHECKS.json` | 63 120 lignées (262 538 détachements) et 504 960 appels parallèles, 0 désaccord, sommes et identités tenues, 20 319 exceptions propagées sans blocage |
| Équipe persistante front + census | beee3341 | `CHAINE_Q2_COOP_CHECKS.json`, `…_COOP_SCALE_…` | 5 304 appels coopératifs, 0 désaccord, identités de continuation tenues, 258 exceptions propagées ; défaut = Coarse, toute ancre en continuation +40 %, quantum 1 ×4 à ×7 |
| Plages d'ancres et Pool partagé | 2741d614 | `CHAINE_Q2_RANGES_CHECKS.json` | 5 256 appels à plages, 0 désaccord, identités de plages tenues, 258 exceptions propagées ; coût égal à Coarse, gain sur les rangées à huit fils |

Mesures publiées à côté : plafond de tout proposeur de témoins du front
(86 à 89 % des rectangles émis à uniform, fenêtre 4K à 80 à 85 % du
plafond, rangées 0 à 2 %), survivantes de Pool sur les amas (2 140 /
3 085 / 4 690 vrais supports q2 à 8k/16k/32k ; résidu Pool non
canonique), séparation s ∈ {8, 10, 12} (même objet, coûts voisins,
s = 8 confirmé), crédits terminaux et tubes (nuls sur nuages
irréguliers). Corrections acquittées : chiffres de la note des crédits
(sur son reçu), invariant I1, argument des rangées (cordes 2u, témoins W3
pour u > D/√3).

Points ouverts pour la suite : q3/q4 (réponses mathématiques ci-dessous ;
un oracle indépendant en i128 est prêt dans
[oracle_q3q4_20260915/](oracle_q3q4_20260915/README.md), identique au
catalogue rationnel de `reference/` sur 315 petits nuages, 5 811
présentations q3 et 1 358 q4, 0 désaccord, pour juger la première brique
q3/q4 au-delà de n = 12), reprise mi-plage des continuations (cas
positif absent tant que l'émission reste atomique), déséquilibre des gros
jobs de census (rangées, sous-arbres LiDAR de A), contrats FULL et G4
hors de portée de ces reçus.

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

## Tranches 8 à 10 vérifiées bout en bout : 0 désaccord

`run_wspd_q2_census` à e3af11a7, huit combinaisons de modes (Pure ou
Samples × Pairwise ou SharedBlocks × frère × ordre complément), confronté
à une force brute exacte sur tous les sites : 86 exécutions, 104,7 millions
de paires contrôlées, 3 359 624 supports vivants tous émis une fois avec
intérieurs, coquilles et clés exacts, 0 désaccord, sur sept nuages
adversariaux (cosphériques, colinéaires, extrêmes, demi-entiers) et les
quatre familles synthétiques jusqu'à n = 2 000. Le frère a rejeté 4,2 M
paires et l'ordre complément a changé de phase 1,4 M fois : les modes ont
été réellement exercés. Détail dans
[PORTES_ET_TESTS_20260914.md](PORTES_ET_TESTS_20260914.md) §1 bis, reçu
`chaine_q2_20260914/`.

## Le régime « amas » se ferme avec les crédits P0 existants

Les tranches huit à dix mesurent les amas quadratiques (173,5 s à 32k,
visites ×4,2) parce que le raccord front → census ne rejette que par
témoins extérieurs, absents des produits inter-amas. Mesuré sur da366f7f
et la fixture `clusters` du constructeur : 28 rectangles terminaux à gros
facteurs portent 112 des 116,8 millions de paires q2 du résidu à 16k ;
Pool sur ces 28 rectangles coûte 120 ms et y laisse 29 878 paires
(×0,0003) ; à 8k, 60 ms et 11 329 paires. DualBlocks fait mieux pour
q3/q4 (301 k / 370 k au lieu de 2,1 M / 3,2 M à 8k) pour 1,5 s. Sur
l'uniforme et le terrain, aucun rectangle n'atteint 64 sites et Pool ne
change rien : une politique par taille de facteur suffit. Les tubes, eux,
ne créditent rien sur ces amas irréguliers (largeur de cellule fixée à
quatre unités réelles : un site par cellule), et restent quarante fois
moins sélectifs que Pool à leur meilleure largeur ; ils avaient été
qualifiés sur des grilles alignées. Note et reçu :
[CREDITS_TERMINAUX_20260914.md](CREDITS_TERMINAUX_20260914.md). Le
constructeur relève à juste titre, dans son brouillon de raccord, que
le harnais additionne des candidates sans exécuter le census et que deux
chiffres de la prose venaient d'une exécution préliminaire : titre et
chiffres sont alignés sur le reçu (60 ms à 8k, +1,04 s au seuil 2), et
la limite est écrite ; la comparaison q2 complète lui appartient.

## Plafond de tout proposeur de témoins (note « surproposition », 15 septembre)

La note `docs/P0_SURPROPOSITION_TEMOINS_Q2.md` sépare le seuil K du nombre
L de propositions de `Front::filter`. Avant sa campagne, j'ai mesuré sur
2741d614 ce qu'aucun proposeur ne peut dépasser et ce que les fenêtres
2K et 4K atteignent, sur les rectangles émis par le front (donc non
rejetés par la fenêtre historique), avec le prédicat du front lui-même
`H_min(A.box, B.box, {z}) > 0` : dossier
[plafond_proposeur_20260915/](plafond_proposeur_20260915/README.md)
(harnais, lanceur, reçu `PLAFOND_PROPOSEUR_CHECKS.json`, 14
configurations 8k, 16k, 32k, K = 5 et 10, s = 8 et 12). Un rectangle
sans K sites universels de boîte n'est rejetable ni à son produit ni à
un ancêtre, dont les boîtes sont plus grandes : la part mesurée borne
toute extension du filtre, quelle que soit la fenêtre.

Faits (parts de rectangles émis de la voie q2, puis de leur masse de
candidats) :

- **Plafond** : uniform 86 à 89 % des rectangles (8k à 32k, montant
  avec n) et 90 à 93 % de la masse ; terrain 68 à 69 % et 79 à 82 % ;
  amas 81 à 86 % des rectangles mais 1 à 5 % de la masse ; rangées 0 à
  2 % et 0 % de la masse. À uniform 8k K = 10, la masse émise vaut
  3 194 249, exactement les candidats du reçu constructeur cité par la
  note, et la masse plafonnée (2 904 816) recouvre 99,7 % des 2 914 705
  candidats que le census rejette ensuite : le filtre historique laisse
  passer presque tout ce que le census rejette, parce que ses K
  propositions doivent toutes réussir.
- **Fenêtres** : la fenêtre K rejouée rejette zéro rectangle émis
  (contrôle de cohérence) ; la fenêtre 2K en rejette 55 à 67 %, la
  fenêtre 4K 65 à 75 % (uniform 72 à 75 %, terrain 55 à 57 %), soit 80
  à 85 % du plafond ; le pas 2K → 4K vaut encore 9 à 13 points.
- **Certificat de bloc** sur le chemin du front vers le milieu : 14 à
  30 % ; le plus haut nœud du chemin à borne conjointe positive a le
  plus souvent 2 à 9 sites.
- **Descente exacte plafonnée à K** (bornes conjointes, comptage sans
  paire) : atteint le plafond entier pour 48 à 91 bornes par rectangle,
  82 à uniform 8k K = 10 et 91 à 32k.

Réponses aux trois questions de la note :

1. **Tangences.** Le certificat tient : le prédicat reste strict par
   site (H = 0 n'est jamais crédité), les rangs de A et B sont sautés,
   les intervalles supplémentaires sont disjoints et la permutation
   rend leurs identifiants distincts ; l'extension change l'ensemble
   proposé, ni le prédicat ni les seuils par lane. La formule
   `first = min(pivot − L/2, n − L)` contient la fenêtre historique aux
   deux bords, donc tout rejet à L = K reste un rejet à 2K et 4K. Les
   deux contre-fixtures de tangence de 2741d614 ne bougent pas.
2. **Fixture discriminante.** Uniform contre rangées sur le même
   harnais : à uniform 8k K = 10 la fenêtre 2K rejette 1 238 k
   rectangles de plus que la fenêtre K ; sur les rangées elle n'en
   rejette aucun de plus et le plafond y est 2 %, chaque proposition
   supplémentaire y est un surcoût pur. Pour une fixture gravée, prendre
   un rectangle à U ≥ K dont la fenêtre K contient un rang de A ou B ou
   un site tangent (le harnais les identifie ; n = 2 000 en fournit
   234 431). Sur les amas, le gain est en nombre d'appels, pas en
   masse : 95 à 99 % des candidats sont dans les rectangles amas × amas
   sans aucun témoin universel, qui restent au Pool et au census.
3. **Certificat de bloc.** Plus faible que la fenêtre (14 à 30 % contre
   55 à 75 %), il ne la remplace pas ; mais il coûte une seule borne
   conjointe par nœud d'un chemin déjà parcouru (13 à 15) et peut la
   précéder. La descente exacte est l'autre option, au prix de 48 à 91
   bornes par rectangle, l'ordre d'une fenêtre 4K à K = 10, croissant
   lentement avec n et payé sur tous les produits visités.

Ce que ces parts ne disent pas : le coût du filtre sur tous les produits
visités par le front, le temps mur du pipeline complet et le gain net.
Elles bornent le gain brut : à uniform, au plus 86 à 89 % des appels de
census actuels et 90 à 93 % de leur masse ; la fenêtre 4K en prend 80 à
85 %. Avis : la variante « tous produits » se justifie sur uniform et
terrain ; sur les amas, viser le nombre d'appels seulement ; sur les
rangées, la mesurer comme régression attendue.

## Petits census entrelacés (tranche 19, sources en chantier) : obligations des feuilles B compactées

Question du journal : le compactage des seules feuilles B (état
singleton copié : clé de paire, rang d'ancre, B original, curseur,
compte, phase, frère dû, stade Entry/Witness/Emit, lots entrelacés par
worker) perd-il une obligation aux reprises après crédit, à l'entrée de
la phase différée ou au certificat frère encore dû ? Contrelecture sur
un instantané à manifeste SHA-256 des sources du worktree pris à
10 h 25 UTC (`wspd_q2_batched.hpp`, `q2_census.cpp` étendu).

**Réponse : aucune obligation perdue, pour les raisons suivantes.** Une
feuille B est une requête (a, b) dont le préfixe Z consommé est résolu
uniformément pour cette seule paire : le compte acquis est donc un
scalaire exact, et la reprise après crédit n'a besoin que de (compte,
curseur, phase) copiés à la soumission, ce que l'état porte ; aucun
recompte n'est possible puisque le curseur désigne le premier sous-arbre
non consommé du même ordre DFS. L'entrée en phase différée ne dépend
que du B original et de son échappement (report du B original, pas du B
courant) et du rang de l'ancre (exclusion de la contribution connue
nulle par rang) : les trois sont dans l'état. Le certificat frère dû
est payé une fois, à l'entrée, sur le nœud frère mémorisé, et son crédit
n'est jamais ajouté au compte hérité : c'est le contrat du certificat
autonome, inchangé. Ce que la spécialisation ne doit pas altérer, et que
l'en-tête promet, c'est la clé calculée une fois et jamais reconstruite,
la collecte complète de toute la coquille à l'émission, et l'interdiction
d'un crédit importé par l'API publique. Le flush avant tout rectangle
Pool et en fin de graine garde le Pairwise synchrone et sa permutation
hors du lot, donc aucun `b_order` étranger n'entre dans un état.

**Campagne (harnais `chain_verify_parallel.cpp -DMHGP8_AUDIT_BATCHED`,
option `--batched`, reçu
[CHAINE_Q2_BATCHED_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_BATCHED_CHECKS.json),
instantané de 10 h 25 UTC, sources encore en chantier).** Sur 86
nuages, chaque combinaison SharedBlocks/Individual est exécutée par
l'entrée à lots pour W ∈ {1, 2, 3, 4, 8} et quatre réglages (lots 1, 4,
16, 64 ; quantum 1 ou 8) : 5 256 appels, 13 092 120 paires
contrôlées contre la force brute, **0 désaccord, 0 doublon entre
slots**, condensé canonique et compteurs globaux du pipeline (dont
`query_tasks`, frère et phases) égaux au chemin série à chaque appel ;
identités de lot tenues partout (618 159 656 états soumis :
soumis = terminés = admis + rejetés = entrées, transitions = entrées +
témoins + admissions + collectes, clés préparées = soumis) ; les trois
cas de la question sont exercés positivement par les compteurs du
constructeur (entrées après crédit, entrées en phase différée, frères
dus : par exemple 76 485, 3 186 et 169 090 sur les amas à 800 sites en
front Pure) ; vivacité : 258 exceptions
propagées sur 258 appels, aucun blocage. Rejeu
`-O` conforme.

**Échelle** (quatre familles × 8k/16k/32k, Kmax 10, s 8, Pool 64, un
passage, hôte partagé) :

| Entrée | Série | Coarse W = 1 / W = 8 | Lots 16, quantum 1, W = 1 / W = 8 | Lot 1, quantum 1, W = 1 / W = 8 |
| --- | ---: | ---: | ---: | ---: |
| Uniforme 8k | 5,19 s | 5,19 / 1,05 s | 8,60 / 1,69 s | 7,36 / 1,52 s |
| Uniforme 16k | 12,34 s | 12,40 / 2,72 s | 20,55 / 3,89 s | 17,75 / 3,57 s |
| Uniforme 32k | 28,09 s | 28,21 / 5,89 s | 47,48 / 9,20 s | 40,64 / 8,53 s |
| Amas 8k | 2,77 s | 2,79 / 0,57 s | 4,49 / 0,93 s | 3,89 / 0,78 s |
| Amas 16k | 7,63 s | 7,59 / 1,57 s | 12,51 / 2,47 s | 10,86 / 2,26 s |
| Amas 32k | 19,37 s | 19,59 / 4,09 s | 32,31 / 6,33 s | 27,73 / 5,81 s |
| Terrain 8k | 0,97 s | 0,97 / 0,20 s | 1,36 / 0,27 s | 1,26 / 0,26 s |
| Terrain 16k | 2,03 s | 2,07 / 0,44 s | 2,91 / 0,61 s | 2,63 / 0,57 s |
| Terrain 32k | 4,48 s | 4,47 / 1,00 s | 6,37 / 1,34 s | 5,85 / 1,23 s |
| Rangées 8k | 0,25 s | 0,25 / 0,14 s | 0,27 / 0,15 s | 0,29 / 0,14 s |
| Rangées 16k | 0,53 s | 0,51 / 0,19 s | 0,62 / 0,18 s | 0,61 / 0,18 s |
| Rangées 32k | 1,03 s | 1,10 / 0,33 s | 1,18 / 0,32 s | 1,24 / 0,30 s |

Lecture : sur cet instantané, le format à lots coûte plus cher que le
chemin Shared synchrone, et l'entrelacement n'aide pas : un seul lot
(changement de format seul) vaut +40 à +45 % à un fil sur l'uniforme et
les amas, seize lots entrelacés +65 à +70 % ; à huit fils, +45 à +55 %.
Les rangées sont neutres (peu de singletons). Aucune géométrie ne
change, le surcoût est celui de la copie d'état et du parcours par
passes ; c'est cohérent avec le diagnostic du constructeur (le Shared
singleton n'alloue déjà rien) et laisse la spécialisation du contrôle
comme seul levier de cette voie. Les sources ont bougé depuis
l'instantané (boucle de témoins réécrite avec état local) : je
rejouerai au gel ou à la publication.

## Plages d'ancres et Pool partagé (2741d614) : contrelecture et campagne

Avis demandé au journal sur la durée de vie des plans parentaux, le
retour temporaire de `b_order` après une bande et les bilans globaux
quand une sortie migre de worker. Contrelecture faite sur un instantané
de 09 h 31 UTC ; le gel du moteur annoncé ensuite par le constructeur ne
change que le traqueur de mémoire des parents (verrou et addition
vérifiée à la place d'atomiques), pas la géométrie, et j'ai repris un
instantané des sources gelées, dont les fichiers de `src/` sont, octet
pour octet, les blobs du commit 2741d614 : la campagne ci-dessous est
exécutée sur ce second instantané et ancrée sur ce commit.

**Contrelecture.** La durée de vie est portée par le type : un parent
Pool (`RangePoolParent`) déclare son `Q2CensusIndexPtr` avant le plan
qui l'emprunte, donc le plan meurt avant l'index qu'il référence ; il
n'est ni copiable ni déplaçable et n'est construit qu'une fois par
rectangle sélectionné ; chaque tâche de plage tient un `shared_ptr`
constant vers lui, si bien qu'un parent vit tant qu'une plage le cite,
en file ou en cours, quel que soit le worker, et meurt à la dernière
référence, comptabilisé par un traqueur atomique de parents et d'octets
vivants dont le pic est publié tel quel. Le `b_order` du moteur privé
est sauvegardé à l'entrée d'une plage, remplacé par l'ordre B du parent
pour une plage filtrée seulement, et restauré à la sortie normale comme
sur exception ; une plage partagée garde l'ordre global et le nœud B
original. Une donation transfère le suffixe de la plage, soit la
seconde moitié (⌊m/2⌋ dernières) des m ancres restantes, avec le même
parent et la même largeur de préfixe, sans allocation ni
lancée d'exception, après construction du reçu et avant rétrécissement
du donneur ; le receveur installe l'ordre du parent dans son propre
moteur. Les bilans restent globaux par construction : masse et
descripteur du rectangle, préparation et bandes du plan sont payés une
fois par le créateur ; racines, géométrie, ancres Pool sélectionnées,
paires et collectes sont payées par l'exécutant, et les identités
`initial_anchors = completed_anchors`, `initial_pairs = completed_pairs
= candidates`, `completed_ranges = initial_ranges + donations`,
`received = donations`, offres = occupé + pleine + sans demande + dons
et attentes = réveils tiennent à la complétion ; la fermeture est celle
de l'équipe persistante (graines réclamées, file vide, aucune activité).
Rien à objecter ; une remarque : la masse donnée compte des transferts
répétés d'un même travail futur et n'est donc pas une mesure de charge
déplacée, ce que l'en-tête dit déjà et qu'il faudra rappeler dans les
lectures de reçus.

**Campagne (harnais `chain_verify_parallel.cpp -DMHGP8_AUDIT_RANGES`,
option `--ranges`, reçu
[CHAINE_Q2_RANGES_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_RANGES_CHECKS.json),
exécutée sur les sources gelées).** Sur 86 nuages, chaque combinaison
SharedBlocks/Individual (Pool 64 ou 2, front Pure ou MidpointSamples,
frère et Complement) est exécutée par l'entrée à plages d'ancres pour
W ∈ {1, 2, 3, 4, 8} et quatre réglages (grain 1, 4 ou 64, file 1 ou 8) :
5 256 appels, 13 092 120 paires contrôlées contre la force
brute, **0 désaccord, 0 doublon entre slots**, condensé canonique et
compteurs globaux du pipeline égaux au chemin série à chaque appel,
identités de plages tenues partout (211 867 dons :
plages terminées = initiales + dons, ancres et paires initiales =
terminées = candidates, offres = occupé + pleine + sans demande + dons,
attentes = réveils) ; vivacité : 258
exceptions propagées sur 258 appels, aucun
blocage. Rejeu `-O` conforme.

**Échelle** (quatre familles × 8k/16k/32k, Kmax 10, s 8, Pool 64, un
passage, hôte partagé) :

| Entrée | Série | Coarse W = 8 | Plages (grain 64, file 8) W = 1 / W = 8 | Plages (grain 1, file 8) W = 1 / W = 8 | Dons à W = 8 (grain 1) | Parents Pool vivants max |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Uniforme 8k | 5,43 s | 1,08 s | 5,42 / 1,12 s | 5,97 / 1,11 s | 1 445 | 0 |
| Uniforme 16k | 12,89 s | 2,63 s | 12,93 / 2,69 s | 12,99 / 2,76 s | 3 207 | 0 |
| Uniforme 32k | 30,23 s | 6,26 s | 30,17 / 6,32 s | 30,59 / 6,47 s | 9 214 | 0 |
| Amas 8k | 2,99 s | 0,60 s | 2,94 / 0,63 s | 3,01 / 0,63 s | 632 | 6 |
| Amas 16k | 8,20 s | 1,68 s | 8,24 / 1,71 s | 8,37 / 1,68 s | 2 763 | 5 |
| Amas 32k | 20,42 s | 4,25 s | 20,17 / 4,46 s | 20,60 / 4,64 s | 3 652 | 5 |
| Terrain 8k | 1,00 s | 0,21 s | 1,04 / 0,23 s | 1,05 / 0,23 s | 1 027 | 0 |
| Terrain 16k | 2,12 s | 0,48 s | 2,23 / 0,48 s | 2,27 / 0,48 s | 2 004 | 0 |
| Terrain 32k | 4,80 s | 1,03 s | 4,64 / 1,09 s | 4,93 / 1,05 s | 3 553 | 0 |
| Rangées 8k | 0,25 s | 0,15 s | 0,26 / 0,06 s | 0,29 / 0,07 s | 179 | 1 |
| Rangées 16k | 0,55 s | 0,20 s | 0,52 / 0,14 s | 0,57 / 0,14 s | 170 | 2 |
| Rangées 32k | 1,08 s | 0,34 s | 1,03 / 0,28 s | 1,16 / 0,29 s | 171 | 1 |

Lecture : l'entrée à plages coûte comme Coarse sur l'uniforme, les amas
et le terrain (écarts dans le bruit, dons rares au grain 64 et de
quelques milliers au grain 1, au plus cinq parents Pool vivants à la
fois), et elle est la première variante qui **gagne sur les rangées** à huit fils : 0,06 s contre 0,15 s à 8k ; 0,14 s contre 0,20 s à 16k ; 0,28 s contre 0,34 s à 32k, parce que les ancres du gros rectangle sont enfin réparties entre workers au lieu de rester dans un seul job ; le déséquilibre relevé depuis la tranche des workers se résorbe, sans surcoût ailleurs.
**Sur le compactage proposé des petits census singleton** (lots sans
pile B, contexte = B original, rang d'ancre, stade et frère encore dû,
collecte séparée mais complète). Aucune obligation ne l'interdit, à
condition que l'état compact conserve exactement ce dont la reprise
d'une ancre singleton a besoin, et rien de ce qu'elle recalcule : le
compte acquis, le curseur Z et la phase (le préfixe consommé est
uniforme pour la seule paire (a, b), donc un compte scalaire suffit),
le B original et son échappement pour la phase Complement, le rang
spatial de l'ancre (pour l'exclusion de l'ancre connue nulle en
Complement, qui se fait par rang), l'indicateur « frère encore dû » et
le stade ; les bornes préparées se recalculent depuis (a, b) et n'ont
pas à voyager. Deux obligations à garder explicites : la collecte,
même séparée, doit parcourir tout l'index et rendre la coquille
complète (l'admission à budget vaut avant collecte, comme dans la
continuation), et le compte ne doit jamais être préchargé par un crédit
Pool ou un témoin de l'amont. Un lot de singletons peut alors être
transféré comme valeur ; mon harnais de continuations (budgets 1/3/1000,
transfert de fil, compteurs finaux) se rejoue tel quel sur cette forme
dès qu'elle expose `advance` et `pending`.

## Équipe persistante front + census (beee3341) : contrelecture et campagne

Avis demandé au journal sur la fermeture, l'annulation et le bilan des
masses de `run_wspd_q2_census_cooperative`, sur un instantané à
manifeste SHA-256 des sources du worktree (07 h 08 UTC, repris tel quel
à 07 h 32 après le redémarrage d'environnement) ; ses fichiers de
`src/` sont, octet pour octet, les blobs du commit beee3341 publié
ensuite, ce qui ancre contrelecture et reçus sur ce commit.

**Contrelecture.** La fermeture est celle des deux répartiteurs
précédents, étendue aux graines : `take` sert d'abord la file des frères
détachés, puis une graine du front, sinon conclut à `active == 0`, sinon
attend sous le mutex avec le prédicat (annulé, file non vide, graine
restante, ou `active == 0`) ; l'offre sous `try_lock` refuse sans
attendre (occupé, file pleine, aucun slot libre, aucun frère) et ne
détache qu'avec une case réservée ; la libération notifie tous à
`active == 0` ; l'annulation, sous mutex, réveille tous. La graine reste
active pendant tout son `run_job`, y compris les continuations racines
locales exécutées dans son callback, qui n'ajoutent donc pas d'activité
mais peuvent céder leurs frères à la file : un enfant détaché survit à
sa graine, possédé par la file puis par le worker qui le prend. Pas de
réveil perdu, pas d'obligation abandonnée : après la jointure,
`completed()` exige graines épuisées, file vide, aucune activité et
aucune annulation, sinon exception. L'annulation coopérative ne fusionne
jamais un fragment inachevé, ne masque pas l'exception d'origine, et
laisse finir les rectangles synchrones (Pool, petits census) comme la
doc l'annonce. Le bilan des masses est additif par construction : un
rectangle paie une fois candidates et descripteur ; une racine locale
ajoute sa population à `continued_pairs` ; chaque fragment terminé
ajoute ses candidates restantes à `completed_pairs` ; les enfants
détachés emportent leur masse et le donneur la perd, de sorte que
`continued_pairs = completed_pairs`, `fragments_started =
completed_fragments = continued_anchors + donations` et
`donations = detached = imported` à la complétion. Rien à objecter ; le
compteur `donations_after_seeds_exhausted` est le bon indicateur de
l'équilibrage de queue de traitement, à publier avec les mesures.

**Campagne (harnais `chain_verify_parallel.cpp -DMHGP8_AUDIT_COOP`,
option `--coop`, reçu
[CHAINE_Q2_COOP_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_COOP_CHECKS.json)).**
Sur 86 nuages, chaque combinaison SharedBlocks/Individual (Pool 64 ou 2,
front Pure ou MidpointSamples, frère et Complement) est exécutée par
l'équipe coopérative pour W ∈ {1, 2, 3, 4, 8} et quatre réglages
(min_b_size 1 ou 64, quantum 1 ou 256, file 1 ou 8) : 5 304 appels,
13 092 120 paires contrôlées contre la force brute, **0 désaccord,
0 doublon entre slots**, condensé canonique et compteurs globaux du
pipeline (candidates, admises, rejetées, rectangles, visites et tests du
comptage, supports et coquilles, frère, phases, Pool, front) égaux au
chemin série pour chaque appel ; identités de continuation tenues
partout (384 085 760 ancres continuées, 2 420 854 dons, `continued_pairs
= completed_pairs`, fragments = ancres + dons, dons = détachés =
importés, offres = occupé + pleine + sans demande + tentatives) ;
vivacité : 258 appels où tout slot lève à son premier support, 258
exceptions propagées, aucun blocage. Rejeu `-O` conforme.

**Échelle et coût réel des continuations** (reçu
[CHAINE_Q2_COOP_SCALE_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_COOP_SCALE_CHECKS.json),
quatre familles × 8k/16k/32k, Kmax 10, s 8, Pool 64, frère et
Complement, condensés et compteurs égaux au chemin série à chaque appel,
hôte partagé, un passage) :

| Entrée | Série | Coarse W = 8 | Coop défaut (min_b 64, q 256) W = 8 | Coop toute ancre (min_b 1, q 256) W = 1 / W = 8 | Coop toute ancre, quantum 1, W = 1 / W = 8 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Uniforme 8k | 5,40 s | 1,06 s | 1,03 s | 7,47 / 1,46 s | 22,33 / 7,38 s |
| Uniforme 16k | 12,33 s | 2,53 s | 2,51 s | 17,66 / 3,56 s | 53,15 / 16,45 s |
| Uniforme 32k | 28,97 s | 5,96 s | 6,05 s | 41,94 / 8,40 s | 127,35 / 41,39 s |
| Amas 8k | 2,95 s | 0,59 s | 0,59 s | 4,15 / 0,83 s | 11,51 / 3,62 s |
| Amas 16k | 8,08 s | 1,89 s | 1,65 s | 11,29 / 2,25 s | 32,56 / 10,38 s |
| Amas 32k | 20,11 s | 4,17 s | 4,18 s | 28,54 / 5,87 s | 84,60 / 27,73 s |
| Terrain 8k | 0,99 s | 0,20 s | 0,21 s | 1,40 / 0,30 s | 3,69 / 1,29 s |
| Terrain 16k | 2,07 s | 0,44 s | 0,44 s | 2,94 / 0,63 s | 7,67 / 2,54 s |
| Terrain 32k | 4,51 s | 0,99 s | 0,99 s | 6,51 / 1,36 s | 17,17 / 5,44 s |
| Rangées 8k | 0,24 s | 0,14 s | 0,14 s | 0,30 / 0,14 s | 0,52 / 0,17 s |
| Rangées 16k | 0,51 s | 0,19 s | 0,18 s | 0,63 / 0,18 s | 1,08 / 0,31 s |
| Rangées 32k | 1,03 s | 0,27 s | 0,26 s | 1,29 / 0,31 s | 2,23 / 0,61 s |

Trois lectures. Avec le réglage par défaut, l'équipe coopérative coûte
exactement comme Coarse et ne crée aucune continuation : sur ces
familles aucune racine non filtrée n'a un B d'au moins 64 sites, ce que
le constructeur constate aussi. En forçant toute ancre en continuation
avec un grand quantum, le surcoût est de 38 à 45 % à un fil et d'environ
40 % à huit, soit 1,2 µs par continuation racine (uniforme 32k :
+12,9 s pour 11,08 millions de racines), avec des dons rares (au plus
1 559) ; c'est le prix des objets de continuation eux-mêmes. Au
quantum 1, le coût devient ×4,4 à un fil et ×6,9 à huit (uniforme 32k :
127,4 s contre 29,0 s ; 41,4 s contre 6,0 s), soit près de 9 µs par
ancre : le pas de reprise domine, pas le don. Les rangées, où la file
sert vraiment, ne gagnent rien non plus (0,26 → 0,31 s à 32k, W = 8).
Conclusion conforme à celle du constructeur : acquis d'exactitude et
d'architecture, aucun gain de vitesse ; la voie suivante qu'il désigne
(curseurs d'ancres, plans parentaux, états singleton compacts) attaque
justement le coût par racine que ces chiffres isolent.


## Détachement intérieur du census (897085f8) : contrelecture et campagne

Réponse à la question du journal (conservation des obligations après
détachements récursifs, comptabilité additive, clôture avec workers
endormis ou lancement partiel en échec), sur un instantané à manifeste
SHA-256 des sources du worktree pris à 05 h 47 UTC (`detach_pending`,
`q2_census_parallel.hpp/.cpp`) ; les 28 fichiers de `src/` de cet
instantané sont, octet pour octet, les blobs du commit 897085f8 publié
ensuite, ce qui ancre la contrelecture et le reçu sur ce commit.

**Contrelecture.** L'ordonnanceur par ancre reprend exactement le
protocole du répartiteur de front : prise sous mutex avec prédicat
(annulé, file non vide, ou `active == 0`), offre sous `try_lock` qui ne
détache l'enfant **qu'après** avoir réservé une case libre de la file
préallouée (le déplacement dans la case est `noexcept`, un échec
d'allocation laisse le donneur intact, un échec de comptabilité annule
tout en gardant l'obligation dans la file), donneur actif jusqu'à la fin
de son fragment, libération qui notifie tous à `active == 0`, annulation
sous mutex qui réveille tous ; réduction jointe de `run_joined_workers`,
y compris après lancement partiel. Les obligations se conservent par
construction : un enfant prend exactement le plus ancien cadre sœur non
visité (population m), ses candidates valent m et celles du donneur
diminuent de m, aucun `root_start`, descripteur ni compteur historique
n'est copié, et le grand livre final exige candidates = racine,
admises + rejetées = candidates, supports = admises, une seule racine,
détachés = importés = dons, fragments démarrés = terminés = dons + 1.
Un fragment interrompu par l'annulation n'est pas fusionné et le
résultat n'est pas rendu : pas d'obligation silencieusement perdue.
Rien à objecter ; deux remarques de lecture seulement : les cadres
détachés portent chacun leur triplet compte/curseur/phase figé (point
que le constructeur a rendu explicite), et la file possède des
continuations complètes (6 272 octets de pile chacune), ce qui borne les
objets vivants à `queue_capacity + workers + 1` mais pas la mémoire
totale des tampons de collecte.

**Campagne (harnais `chain_verify_detach.cpp`, runner
`run_chain_verify_detach.py`, reçu
[CHAINE_Q2_DETACH_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_DETACH_CHECKS.json)).**
Sur 74 nuages et 10 520 couples (ancre, nœud B à au moins deux
sites), trois réglages d'options :

- *Lignées par détachement récursif* (mono-fil, budgets 1 et 5 ; à chaque
  pause on détache si possible et chaque enfant est traité de même) :
  63 120 lignées, 262 538 détachements ; réunion des
  émissions égale à la référence série et à la force brute
  (20 634 supports), **0 désaccord** ; onze compteurs géométriques
  et masses sommés sur la lignée égaux à la référence à chaque fois ;
  identités de détachement vérifiées à chaque détachement (candidates de
  l'enfant = décrément du donneur, `max_pending_tasks` de l'enfant = 1,
  `detached_frames` et `transferred_pairs` du donneur incrémentés
  d'autant, détachés = importés sur la lignée).
- *`run_q2_anchor_parallel`* pour W ∈ {1, 2, 4, 8} × quantum ∈ {1, 256}
  × file ∈ {1, 8} : 504 960 appels, réunion des slots égale à la
  référence, somme égale à la référence sur les mêmes compteurs,
  0 doublon entre slots.
- *Vivacité* : tout slot lève à son premier support (W = 8, quantum 1,
  file 1) : 20 319 exceptions propagées sur
  20 319 appels, aucun blocage (chien de garde silencieux).
  Rejeu `-O` conforme.

**Deux remarques du constructeur, acquittées.** Sur la plage multiple
admise : le cas est structurellement inatteignable avec la règle des
diagonales actuelle, pour la même raison que l'admission conjointe. Pour
un nœud B à deux sites distincts b, b', le segment ouvert (b', b)
contient des z avec H(a, b, z) = (1 − t)·[(b' − a)·(b − b') + t|b − b'|²]
strictement positif près de b, donc le maximum continu de H sur
boîte(B) × boîte(B) est strictement positif tandis que le minimum est
nul (z = b) : le nœud Z = B est toujours indécis, et la règle divise
alors B (diagonale de Z non strictement supérieure). Une plage admise
est donc toujours un singleton ; mon compteur nul et le vôtre disent la
même chose, et une fixture ne pourra l'exercer qu'avec une autre règle
de descente. Sur W3 : exact, le fuseau est {H > 0 et 3H² > Ξ} ; le carré
seul admettrait la branche u > 3 (H < 0) ; mes formulations « 3H² > Ξ »
sous-entendaient H > 0, comme dans le code.

Le constructeur a resserré deux points de ce qui précède, à juste titre :
pour l'indécision de Z = B, prendre b le site de B le plus éloigné de a
et δ = b − b' ; z = b − δ/4 donne H(a, b, z) = (b − a)·δ/4 − |δ|²/16 ≥
|δ|²/16 > 0 puisque 2(b − a)·δ ≥ |δ|² par maximalité de b, ce qui prouve
la positivité pour toute orientation de la paire (mon paramétrage en
(1 − t) ne la garantissait que près de b), et le minimum sur la boîte
est ≤ 0, pas nécessairement nul ; et la borne des continuations vivantes
est Q + W, non Q + W + 1, l'enfant en construction occupant déjà la case
réservée. Sa note détaille la preuve en suivant la lignée d'un B*
supposé admis ; je n'ai rien à y ajouter.

## Continuations de census (d09e2207) et ouverture q3/q4 : réponses aux questions du journal, continuations vérifiées

Réponse à la section « continuations census et ouverture q3/q4 » du
constructeur. La brique de continuation a été vérifiée sur un instantané
à manifeste SHA-256 pris à 20 h 51 UTC ; les 26 fichiers de `src/` de
cet instantané sont, octet pour octet, les blobs du commit d09e2207
publié ensuite, ce qui ancre la campagne ci-dessous sur ce commit. Les
mathématiques ne dépendent pas des sources.

**Invariants pour transférer une continuation entre workers.** L'état
listé dans l'en-tête (index possédé, B original, compte acquis, curseur
et phase Z, frère, état d'entrée, bornes préparées, plage admise avec sa
position d'émission, pile des requêtes sœurs pendantes) est complet à
une condition près qui n'y figure pas explicitement : chaque entrée de la
pile des sœurs pendantes doit porter **son propre** triplet (compte,
curseur, phase) figé à l'instant de la division de B, et non le triplet
courant, puisque les deux enfants héritent du même préfixe résolu et que
le premier enfant avance le curseur avant que le second ne commence.
Sans cela, le second enfant recompterait des témoins déjà crédités ou en
perdrait. Trois invariants de transfert valent en plus : (i) l'adoption
n'est licite que pour le même objet index et les mêmes options (K,
ordre, frère), ce que la fabrique valide ; (ii) tout ce qui est
thread-local (tampons d'intérieurs et de coquille, bornes préparées)
doit appartenir à la continuation ou être reconstruit à l'identique
depuis (ancre, B courant), jamais emprunté au worker précédent ; (iii)
la seule synchronisation requise est un « arrive-avant » entre la fin
d'un `advance` et le début du suivant, ce que dit l'en-tête. Le fait
que la collecte d'un support reste atomique et que la masse admise
soit incrémentée avant les émissions paire par paire impose de comparer
les compteurs discrets **à la complétion seulement**, comme l'en-tête
le prévient.

**Cas positifs de suspension à exiger.** Après un crédit (compte > 0 et
curseur avancé, reprise sans recompte) ; exactement au changement de
phase (curseur en fin de préordre, passage à B différé) ; juste après la
division de B (sœur empilée, reprise de la sœur avec le triplet hérité) ;
après un certificat frère qui rejette un enfant sans émission ; au début
d'une plage admise (masse incrémentée, aucune paire émise) et entre deux
paires d'une plage ; enfin budget 1 partout sur tout un corpus, comparé
à la référence non reprenable et à la force brute, et budget 3 avec
chaque pas exécuté dans un fil neuf joint avant le suivant. C'est ce que
fait `chaine_q2_20260914/chain_verify_resume.cpp` (couples ancre × nœud
B tirés de l'index, trois réglages d'options, budgets 1, 3, 1000,
transfert de fil au budget 3, pas supplémentaire après Done, compteurs
finaux, pauses classées), runner `run_chain_verify_resume.py`, reçu
[CHAINE_Q2_RESUME_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_RESUME_CHECKS.json) :
74 nuages, 21 552 couples (ancre, B), 258 624 continuations dont 64 656
avec transfert de fil à chaque pas, **0 désaccord** avec la référence
série (elle-même égale à la force brute sur 27 341 supports), compteurs
discrets finaux identiques, aucune émission après Done ; les trois
suspensions demandées sont exercées massivement (2 629 838 pauses après
crédit, 852 339 en phase différée, 135 901 pendant une émission), avec
au plus 8 sœurs pendantes ; rejeu `-O` conforme. La brique tient donc
son contrat sur les sources publiées. Sur la remarque du constructeur,
j'ai ajouté le compteur d'une pause à l'intérieur d'une plage à plusieurs
paires (même plage qu'à la pause précédente, `emit_next` strictement
avancé et encore inférieur à `emit_end`) : 0 sur 6 325 518 pauses du reçu régénéré, donc aucune : dans cette brique une plage admise n'est jamais suspendue entre deux de ses paires, en accord avec le compteur du constructeur ; ce cas positif n'existera qu'avec l'émission paire par paire annoncée, et le harnais est prêt à le compter.

**q3/q4, point (1) : l'arête rejetée à q2 peut posséder un simplexe de
profondeur nulle.** C'est exact, et l'argument est géométrique : pour un
triangle aigu d'arête maximale ab (rayon r = |ab|/2, hauteur du
circumcentre y0 au-dessus du milieu m, rayon R = (r² + y0²)^(1/2)), la
boule diamétrale B(m, r) n'est contenue dans la circumboule que si
|c0 − m| + r ≤ R, soit y0 + r ≤ R, soit R ≤ r : jamais pour un triangle
strictement aigu. La lunule B(m, r) ∖ circumboule est non vide du côté
opposé au troisième sommet, à distance de m supérieure à R − y0 ; K
sites qui y sont placés tuent l'arête à q2 et laissent la circumboule
vide. La fixture de la porte (a = (900,1000,1000), b = (1100,1000,1000),
c = (1000,1120,1000), témoins en y = 910) est précisément dans cette
lunule : y0 = 55/3, R − y0 ≈ 83,3 < 90. Les voies doivent donc être
séparées, avec le seuil h_q = Kmax + 2 − q propre à chaque voie et son
propre lieu de témoins universels : pour q3, le fuseau
W3 = {z : 3H² > Ξ} est l'intersection des circumboules de tous les
triangles aigus d'arête maximale ab (vérifié sur la bissectrice : rayon
r/√3 = celui de la boule équilatérale la plus serrée), ce qui rend le
crédit « ≥ h_3 témoins universels ⇒ toutes les complétions mortes » sûr.
L'objet « arête × groupe de complétions » est le bon : les circumcentres
des triangles (a, b, z) balaient une famille à **deux** paramètres dans
le plan bissecteur de ab, il n'y a donc pas d'ordre total des complétions
comme pour q4, seulement des certificats par blocs. Deux limites déjà
mesurées s'y transposent : un certificat uniforme sur trois boîtes
A × B × Z ne porte les témoins intra-facteur qu'après découpage le long
de l'axe (mon analyse du census conjoint q2), et un certificat à ancre
fixe (Pool) reste préférable là où il existe.

**Point (2) : événements groupés sur l'axe d'une graine q3.** Pour une
graine (a, b, c) de circumcentre c0, de rayon R0 et de normale n, les
sphères contenant le triangle sont S(t) de centre c0 + t·n et de rayon²
R0² + t². Avec s(z) = (z − c0)·n et w(z) = |z − c0|² − R0², z est
strictement intérieur à S(t) si et seulement si 2t·s(z) > w(z) : pour
s(z) > 0 c'est t > w/(2s) =: t(z), pour s(z) < 0 c'est t < t(z), et pour
s(z) = 0 c'est w(z) < 0 indépendamment de t (point coplanaire, intérieur
au circumcercle ou non). Le compte intérieur de la sphère passant par d
vaut donc #{z au-dessus : t(z) < t(d)} + #{z au-dessous : t(z) > t(d)} +
#{z coplanaires : w(z) < 0}, calculable pour toutes les complétions d en
un tri des événements t(z) de chaque côté ; les plateaux sont les
égalités t(z) = t(d), c'est-à-dire les cosphéricités. L'ordre exact ne
demande ni centre rationnel ni quotient : t(z1) < t(z2) équivaut, selon
les côtés, au signe du déterminant InSphere de (a, b, c, z1 ; z2), dont
les entrées réduites valent au plus 2^16 (différences) et 2^34
(relèvements) et le déterminant 4 × 4 au plus 2^87 : i128 suffit, et le
plateau est le déterminant nul. Pour q3 de même, « z strictement
intérieur à la circumboule de (a, b, c) » se décide par le signe de
2D·|z − a|² − 2 (z − a)·W avec D = |u × v|², u = b − a, v = c − a et
W = |v|²(u·u − u·v)·u + |u|²(v·v − u·v)·v, entier, borné par 2^104 :
i128 suffit encore, sans U192/U320. Le brouillon
`Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md` (§ 6.1) écrit la même famille
avec P(z) = G(|z − c0|² − R0²) et B_z = n·(z − a) : attention, comparer
deux racines par P1·B2 contre P2·B1 dépasse i128 (P ≈ 2^102, B ≈ 2^50,
produit ≈ 2^152) ; le facteur G y est commun et la différence réduite
est précisément le déterminant InSphere ci-dessus. Ordonner les racines
par ce déterminant, calculé depuis les coordonnées, garde tout en i128 ;
c'est la requalification des « bornes de produits croisés larges » que
le brouillon demande.

**Point (3) : les m² arêtes entre deux rangées.** Sur deux rangées de m
sites à distance D et pas δ, une arête croisée (a, b') décalée de u le
long des rangées n'est arête maximale d'un triangle (a, b', z), z dans
la rangée de a à l'abscisse t, que si |u − t| ≤ |u|, soit t ∈ [0, 2u], et
ce triangle est aigu pour t ∈ (u, 2u] : Θ(u/δ) complétions par arête,
donc Θ(m²) par ancre et **Θ(m³) triangles candidats** au total, tous
aigus, presque tous morts (leur circumboule, de rayon ≥ D/2 et tangente
aux deux rangées, contient de l'ordre de D/δ sites). Deux faits fixent
le verrou. D'abord, aucune de ces arêtes n'a de témoin universel : pour
une arête perpendiculaire, un site de rangée à distance t de a donne
H = −t² < 0, donc n'est pas dans W3 ; les crédits par arête sont nuls,
comme les crédits Pool q2 l'étaient sur cette famille. Ensuite, la
séparation s ne change rien à cette masse : elle règle la granularité
des produits A × B, pas l'ensemble des complétions d'une arête (ma
note de séparation le montre déjà pour q2). Éviter de matérialiser les
m² arêtes est donc possible dans la forme (un produit A × B × Z de
blocs, propriétaire canonique = arête maximale déterminée par la
géométrie, pas par une liste), mais ne réduit la masse que si un
certificat par blocs sait rejeter d'un coup les triangles morts : la
circumboule d'un triangle plat entre deux rangées contient un segment
de chaque rangée, ce qui est un témoin **en bloc** (un sous-arbre de la
rangée uniformément intérieur), l'analogue exact du crédit de bloc Z du
census conjoint, et c'est là que porte l'effort, pas sur s ni sur la
liste des arêtes.

**Précisions demandées par le constructeur (journal, d09e2207), toutes
deux fondées et vérifiées numériquement.** Avec a = (0, 0), b = (D, u)
et x = (0, t) dans le plan des deux rangées, le circumcercle coupe la
rangée de a en [0, t] et celle de b en [t − u, u] : les cordes totalisent
2u, pas D, et j'ai écrit à tort « de l'ordre de D/δ sites ». Le compte
strictement intérieur d'un tel triangle vaut environ 2u/δ − 2 (les
extrémités des cordes sont des sommets ou des sites de coquille), donc
un triangle candidat est vivant si et seulement si u < (h_3/2 + 1)·δ :
les supports q3 retenus sur deux rangées sont en O(m·h_3²), linéaires en
m, quand les candidats sont Θ(m³). Ensuite, pour un témoin z = (0, r) sur
la rangée de a, H = r(u − r) et Ξ = D²r², donc 3H² > Ξ équivaut à
√3·(u − r) > D : des témoins W3 existent sur la rangée dès que
u > D/√3 (r < u − D/√3, et symétriquement sur la rangée de b), et
l'absence de témoin universel que j'ai déduite du seul cas
perpendiculaire ne vaut que pour |u| ≤ D/√3. Conséquence corrigée sur la
masse : les arêtes plus inclinées que D/√3 reçoivent de l'ordre de
(u − D/√3)/δ crédits et sont tuées dès que cet excès atteint h_3 ; la
masse de candidats **sans** crédit se réduit à la bande |u| ≤ D/√3 + h_3δ,
soit Θ(m·(D/δ)²) triangles au lieu de Θ(m³) quand les rangées sont
longues devant leur écart. Le verrou reste celui de la génération par
blocs, mais la borne est celle-là, et elle dépend de D/δ, pas de m.

Vérification numérique par l'oracle q3 indépendant (reçu
[ROWS_Q3_GROWTH_CHECKS.json](oracle_q3q4_20260915/ROWS_Q3_GROWTH_CHECKS.json),
deux rangées de m sites au pas δ = 4, D/δ = 10 et 50) : les
présentations q3 vivantes valent 176 / 376 / 776 pour m = 25 / 50 / 100
à Kmax 5 et 920 / 2 120 / 4 520 à Kmax 10, linéaires en m, **identiques
pour D/δ = 10 et 50**, toutes à arête maximale croisée ; les triangles
aigus candidats valent 4 414 / 20 076 / 62 272 à D/δ = 10 et
4 600 / 39 200 / 323 400 à D/δ = 50, cubiques en m quand D domine. Un
fait de plus, utile pour les portes q3/q4 : sur cette famille, 94 à 99 %
des triangles aigus ont une coquille excédentaire (le quatrième point
(D, t − u) du circumcercle est toujours un site), donc les deux rangées
sont hors du domaine générique pour q3 comme le constructeur le note
déjà pour q4 ; une campagne q3 sur rangées exercerait surtout la
déduplication par boule, pas les présentations génériques.

Précision de bord demandée par le constructeur, vérifiée en fractions et
par l'oracle : pour u = iδ et t = jδ avec i < j ≤ 2i, le compte
strictement intérieur vaut p = (j − 1) + max(2i − j − 1, 0), soit
2i − 2 pour j < 2i mais 2i − 1 à la tangence j = 2i, où la corde de la
rangée de b se réduit au seul sommet b. Le « si et seulement si » de ma
prose vaut donc pour les complétions non tangentes : vivant ⟺
i < h_3/2 + 1 ; à la tangence il devient i < (h_3 + 1)/2, un cran plus
tôt. Exemple à δ = 4, D = 200, Kmax = 10 (h_3 = 9) : i = 5, j = 9 donne
p = 8, coquille de 4, vivant ; i = 5, j = 10 donne p = 9, coquille de 3,
mort. La borne O(m·h_3²) et les comptes publiés ne changent pas.

## Redistribution dynamique des produits DFS (4e878754) : contrelecture et campagne

Réponse à la demande A/B du journal (terminaison sans perte de réveil,
exception avec workers dormants, annulation après échec de lancement,
bilan de tous les produits, coût réel du dispatch), sur l'instantané des
sources gelées de la tranche 14 pris à 19 h 59 UTC (manifeste SHA-256) ;
les 25 fichiers de `src/` de cet instantané sont, octet pour octet, les
blobs du commit 4e878754 publié ensuite, ce qui ancre la contrelecture et
le reçu sur ce commit.

**Contrelecture du répartiteur (`front.cpp`, `WspdFrontDispatch::Impl`).**

- *Pas de réveil perdu.* `take` fait tout sous un seul mutex : test
  d'annulation, test de complétion, prise d'un don ou d'une graine, sinon
  `active == 0` ⇒ complétion, sinon attente sur la variable de condition
  avec un prédicat (annulation, file non vide, graine restante, ou
  `active == 0`) réévalué sous le mutex au réveil. Chaque changement du
  prédicat se fait sous ce mutex et est suivi d'une notification :
  `offer` pousse puis `notify_one`, `release_fragment` décrémente `active`
  et notifie tous si c'est la dernière libération, `cancel` pose le
  drapeau sous le mutex puis notifie tous. La lecture relâchée du
  drapeau externe ne sert qu'à l'observation ; la doc exige `cancel()`
  pour réveiller, et le réducteur l'appelle sur toute défaillance.
- *Terminaison.* Un donneur reste `active` pendant tout son travail
  local et ne libère qu'à pile vide ; un preneur devient `active` à la
  prise ; le prédicat de complétion exige `active == 0`, file vide et
  graines épuisées, ce qui vaut « aucun produit non visité nulle part ».
  Une offre ne cède qu'un produit entier non visité, jamais un terminal
  (exception), jamais le dernier de la pile (`stack.size() > 1`), et le
  donneur ne le retire qu'après publication réussie : aucun produit ne
  peut être perdu ni traité deux fois. Avec moins d'appelants que de
  slots déclarés, le donneur reprend ses propres dons.
- *Exception et lancement.* `run` attrape tout, appelle `cancel()`,
  libère son fragment s'il en possède un et relance ; `run_worker`
  couvre les échecs antérieurs à la région de nettoyage ; le réducteur
  `run_joined_workers` enregistre l'exception du slot, pose l'annulation,
  appelle la notification de réveil, et joint tous les fils lancés, y
  compris après un échec de lancement partiel. Les dormeurs se réveillent
  sur `cancelled` et rendent `false`. Rien à objecter.

**Campagne (reçu [CHAINE_Q2_DONATE_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_DONATE_CHECKS.json),
harnais `chain_verify_parallel.cpp -DMHGP8_AUDIT_DONATE`, option `--donate`).**
Chaque appel parallèle est répété pour Coarse, Donate{64, 64} et
Donate{1, 1} (file d'un seul produit, intervalle 1 : le cas le plus
contentieux) ; un chien de garde tue tout appel dépassant 600 s.

- **Bilan de tous les produits** : 86 nuages, cinq combinaisons,
  W ∈ {1, 2, 3, 4, 8}, lots 1/16, trois ordonnancements = 13 044 appels
  parallèles, 13 092 120 paires contre la force brute, **0 désaccord,
  0 doublon entre slots**, condensé canonique et 22 compteurs discrets
  égaux au chemin série pour chaque appel ; 1 945 101 dons, et
  `donations = stolen_completed` sur chacun des 13 044 appels (aucun
  don perdu ni pris deux fois).
- **Vivacité** : 5 160 appels où le dernier slot lève une exception
  après cinq supports (Donate{1, 1} maximise les dormeurs) ; 3 385 ont
  propagé l'exception (les autres n'ont pas atteint cinq supports dans ce
  slot), aucun blocage, chien de garde silencieux. Le rejeu `-O` sur les
  familles adversariales est conforme.
- **Coût réel du dispatch** (mode échelle, W = 8, hôte partagé, un
  passage) :

| Entrée | Coarse | Donate 64/64 | Donate 1/1 | Part des visites du worker le plus chargé |
| --- | ---: | ---: | ---: | ---: |
| Uniforme 8k / 16k / 32k | ×4,25 / ×4,63 / ×5,63 | ×4,55 / ×5,15 / ×5,43 | ×4,57 / ×5,31 / ×4,24 | 0,14 / 0,14 / 0,13 |
| Amas 8k / 16k / 32k | ×5,69 / ×6,53 / ×4,60 | ×4,74 / ×6,61 / ×4,37 | ×4,66 / ×6,55 / ×4,59 | 0,14 / 0,14 / 0,13 |
| Terrain 8k / 16k / 32k | ×4,32 / ×4,53 / ×4,52 | ×4,29 / ×4,61 / ×4,46 | ×4,47 / ×4,62 / ×4,49 | 0,14 / 0,13 / 0,14 |
| Rangées 8k / 16k / 32k | ×1,80 / ×2,38 / ×3,73 | ×1,64 / ×2,75 / ×3,87 | ×1,81 / ×2,95 / ×4,04 | 0,79 / 0,39 / 0,20 |

Sur les familles déjà équilibrées, le don est neutre au bruit près
(±5 %, sauf Donate{1, 1} sur l'uniforme 32k, +33 %, qui est le réglage
adversarial et paie 649 154 offres refusées pour file pleine). Sur les
rangées, la part du worker le plus chargé ne bouge pas (79 % à 8k) et le
gain reste ×1,6 à ×1,8 : comme prévu par la note du constructeur, le
don de produits non visités ne divise pas un census déjà engagé, et
c'est ce census-là qui domine. Deux précisions de lecture : en mode
Donate, le temps par worker inclut les attentes sur la file, donc le
rapport max/moyenne des temps vaut 1,00 par construction et ne mesure
plus le déséquilibre ; seule la part des visites (ou du temps hors
attente) le mesure. Et les familles synthétiques n'exercent pas la
redistribution utile ; le constructeur a raison de vouloir la juger sur
les sous-arbres LiDAR déséquilibrés relevés par A.


## Workers du front et du census (b268cf6f) : multiensemble et compteurs identiques de 1 à 8 fils

La tranche « sous-arbres du front et workers q2 » (`wspd_q2_parallel.hpp`,
`parallel/joined_workers.hpp`, `parallel/work_reduction.hpp`, `front.cpp`
et `q2_census.cpp` étendus) a été vérifiée d'abord sur un instantané des
sources gelées (14 h 58 UTC, manifeste SHA-256), puis rejouée contre
`git archive b268cf6f` une fois publiée ; les 25 fichiers de `src/` de
l'instantané sont les blobs du commit, et les reçus ci-dessous sont ceux
du rejeu ancré sur le commit. Rejeu en `python3 -O` sur les familles adversariales conforme. Le contrat de sa note en
chantier est précis : ordre inter-workers non déterministe, mais
multiensemble des supports complets déterministe, compteurs de travail
conservés par somme, décisions géométriques indépendantes de
l'ordonnancement. C'est exactement ce que vérifie mon nouveau harnais
[chain_verify_parallel.cpp](chaine_q2_20260914/chain_verify_parallel.cpp)
(runner `run_chain_verify_parallel.py`), reçu
[CHAINE_Q2_PARALLEL_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_PARALLEL_CHECKS.json) :

- **Exactitude** : sur 86 nuages (sept familles adversariales × K
  1/2/5/10 × s 8/12, quatre familles à 800 sites, uniforme et amas à
  2 000), cinq combinaisons d'options (Pool 64 ou 2, Pairwise, front
  Pure, SharedAnchors), W ∈ {1, 2, 3, 4, 8} fils et lots de 1 ou 16
  produits : 4 348 appels parallèles, 13 092 120 paires contrôlées
  contre la force brute, **0 désaccord**, **0 doublon entre slots**.
- **Identité** : le condensé canonique des supports réunis est le même
  pour tous les W et égal à celui du chemin série ; 22 compteurs
  discrets (candidates, admises, rejetées, rectangles, visites et tests
  du comptage, tâches, racines, frère, phases, Pool, front) sont égaux
  au chemin série pour chaque W (aucune rupture sur 4 348 appels).
- **Échelle** (mode sans force brute, condensés et compteurs comparés au
  série, un passage, hôte partagé à 8 cœurs, temps indicatifs) :

| Entrée | Série | W = 1 | W = 2 | W = 4 | W = 8 |
| --- | ---: | ---: | ---: | ---: | ---: |
| Uniforme 8k | 5,34 s | 5,29 s | 2,87 s (×1,86) | 1,42 s (×3,77) | 1,08 s (×4,93) |
| Uniforme 16k | 12,69 s | 12,70 s | 6,89 s (×1,84) | 3,52 s (×3,61) | 2,69 s (×4,72) |
| Uniforme 32k | 29,67 s | 29,93 s | 16,01 s (×1,85) | 8,22 s (×3,61) | 6,44 s (×4,61) |
| Amas 8k | 2,91 s | 2,94 s | 1,60 s (×1,82) | 0,80 s (×3,65) | 0,61 s (×4,81) |
| Amas 16k | 8,13 s | 8,03 s | 4,38 s (×1,86) | 2,21 s (×3,68) | 1,69 s (×4,81) |
| Amas 32k | 20,52 s | 20,68 s | 11,41 s (×1,80) | 5,59 s (×3,67) | 4,32 s (×4,75) |
| Terrain 8k | 1,03 s | 1,04 s | 0,60 s (×1,72) | 0,34 s (×3,06) | 0,24 s (×4,25) |
| Terrain 16k | 2,16 s | 2,18 s | 1,33 s (×1,62) | 0,74 s (×2,92) | 0,57 s (×3,80) |
| Terrain 32k | 4,69 s | 4,76 s | 2,74 s (×1,71) | 1,44 s (×3,26) | 1,16 s (×4,06) |
| Rangées 8k | 0,28 s | 0,27 s | 0,18 s (×1,50) | 0,15 s (×1,88) | 0,17 s (×1,59) |
| Rangées 16k | 0,55 s | 0,54 s | 0,35 s (×1,57) | 0,25 s (×2,23) | 0,24 s (×2,28) |
| Rangées 32k | 1,15 s | 1,13 s | 0,75 s (×1,52) | 0,43 s (×2,67) | 0,38 s (×3,05) |

**Déséquilibre par worker.** Mesure séparée avec les statistiques par
worker (reçu
[CHAINE_Q2_PARALLEL_BALANCE_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_PARALLEL_BALANCE_CHECKS.json),
quatre familles dont les rangées, mêmes identités vérifiées) :

| Entrée | W = 4 | W = 8 | Déséquilibre W = 8 (max / moyenne des temps worker) | Part des visites du worker le plus chargé |
| --- | ---: | ---: | ---: | ---: |
| Uniforme 8k / 16k / 32k | ×3,23 / ×3,10 / ×3,63 | ×4,35 / ×4,44 / ×4,74 | 1,02 / 1,02 / 1,02 | 0,16 / 0,14 / 0,13 |
| Amas 8k / 16k / 32k | ×3,69 / ×3,60 / ×3,70 | ×4,93 / ×4,66 / ×4,74 | 1,02 / 1,01 / 1,02 | 0,14 / 0,14 / 0,13 |
| Terrain 8k / 16k / 32k | ×3,74 / ×3,52 / ×3,37 | ×4,95 / ×4,65 / ×4,59 | 1,03 / 1,04 / 1,04 | 0,14 / 0,13 / 0,15 |
| Rangées 8k / 16k / 32k | ×1,94 / ×2,99 / ×2,99 | ×1,80 / ×2,61 / ×3,91 | 3,07 / 1,72 / 1,03 | 0,79 / 0,39 / 0,20 |

Uniforme, amas et terrain sont équilibrés (chaque worker porte 13 à
16 % des visites à W = 8, contre 1/8 = 12,5 %) : les 28 gros produits
inter-amas ne pèsent plus rien après Pool. Les rangées sont le
contre-exemple attendu : à 8k, un seul worker porte 79 % des visites de
comptage (le produit rangée × rangée, sans réduction Pool, dont le
census reste entier dans un worker) et le gain plafonne à ×1,8 ; le
déséquilibre décroît avec n (1,72 à 16k, 1,03 à 32k) parce que le front
y découpe davantage de produits. C'est exactement la limite que le constructeur
écrit (« un gros travail de census dans un worker ne sera pas résolu par
le seul don des produits du front ») : elle ne concerne, sur ces
familles, que les rangées, et seulement aux petites tailles.

**Contrelecture couverture / masques / identités (demande A/B du
journal).** `expand` traite chaque produit une seule fois : une diagonale
non feuille se scinde en RR, LR, LL qui partitionnent ses paires non
ordonnées ; un produit disjoint non séparé se scinde en deux enfants qui
partitionnent son produit cartésien ; les deux héritent du masque déjà
filtré du parent, dont les masses rejetées par voie sont comptées au
parent et jamais retestées ; une feuille diagonale et un produit à masque
nul n'engendrent aucun job. La préparation en largeur s'arrête quand
jobs stockés + produits pendants atteignent la granularité ; un terminal
stocké est déjà compté dans le préfixe et `run` n'appelle que le
consommateur (un terminal ne peut être retesté : exception), un produit
pendant reprend avec masque, profondeur et compte de frères pendants
hérités. Les paires du nuage sont donc l'union disjointe des paires des
jobs et des paires rejetées dans le préfixe, chaque paire dans un seul
job : c'est ce que les 4 348 appels confirment sur les compteurs. Les
handles de rectangles désignent l'index immuable possédé par le plan ;
chaque worker garde son moteur, ses vues B et son plan Pool synchrone.
Rien à objecter.

Un worker coûte comme le chemin série (écarts de quelques pour cent,
dans les deux sens). Huit workers
donnent ×3,9 à ×5,0 sur un hôte qui n'a que 4 cœurs physiques pour
8 fils logiques (2 fils par cœur, d'après `lscpu`), partagés
avec les campagnes des autres acteurs : ×4,5 est donc proche du plafond
physique, et les gains de W = 4 à W = 8 (×1,2 à ×1,4) sont ceux du SMT,
pas de cœurs supplémentaires. C'est une accélération murale à travail
identique, sans changement de borne, comme la note du constructeur le
dit elle-même. Le déséquilibre des gros jobs (28
produits inter-amas, sous-arbres LiDAR relevés par A) reste la limite à
mesurer par worker. Rien à objecter sur le contrat.

## Séparation s ∈ {8, 10, 12} : même objet, coûts voisins

Sur la sonde produit de ba11e3ab, quatre familles à 8k et deux à 32k,
avec et sans Pool 64 : le condensé canonique des supports est identique
pour s = 8, 10 et 12 et pour les deux réglages de Pool (30 exécutions),
et le temps englobant varie de moins de 5 % à 8k entre ces trois
valeurs ; à 32k l'écart reste dans le bruit de l'hôte partagé (environ
20 % entre deux exécutions du même binaire). La séparation n'est donc ni
un paramètre d'exactitude ni un levier de coût dans son domaine ;
s = 8 est un point de fonctionnement confirmé. Le domaine est s ≥ 8 :
aucune valeur inférieure n'est mesurée ni proposée. Note
[SEPARATION_20260914.md](SEPARATION_20260914.md), reçu
`separation_20260914/SEPARATION_CHECKS.json`.

## Contrelecture du port Pool terminal (ba11e3ab) : exact de bout en bout, seuil 64 justifié

Réponse à la demande A/B du journal (IDs/rangs, couverture des préfixes,
compte nul, coût cumulé des facteurs), sur les sources gelées du worktree
instantanées à 14 h 11 UTC avec manifeste SHA-256 ; ces sources sont,
fichier par fichier, les blobs publiés à ba11e3ab (`q2_node_pool.hpp`,
`q2_census.cpp` et `wspd_q2_census.hpp` étendus, paramètre
`pool_min_factor`).

- **IDs et rangs.** Les crédits sont indexés par rang spatial moins le
  premier rang du facteur ; `a_ranks()` porte des rangs globaux et
  l'ancre est résolue par `order_[rang]`, `b_order()` porte des IDs
  originaux et `pair_task(a_id, j)` lit `b_order[j]` pendant que la vue
  B est temporairement remplacée, puis restaurée même sur exception.
  L'échange qui fait de B le plus grand facteur précède le test
  `|B| ≥ pool_min_factor`, donc le seuil porte bien sur max(|A|, |B|).
  Rien ne fait passer une plage de rangs pour une plage d'IDs.
- **Couverture des préfixes.** Tri par comptage stable par crédit ;
  pour la classe A_i, préfixe = fin de la classe B_{K−i−1}, nul pour
  i = K ; candidates = Σ_i |A_i|·préfixe(i). Les classes A partitionnent
  A, donc aucune paire n'est dupliquée, et aucune boucle ne visite les
  paires rejetées. La paire croisée de distance minimale n'a jamais de
  crédit (un témoin strict dans A ou B donnerait une paire croisée plus
  courte) : un plan ne vide donc jamais un rectangle, et le repli
  « aucune réduction » est l'autre extrême, atteint très souvent sur les
  petits rectangles.
- **Compte nul.** `pair_task` fait `root_start(0)`, compte 0, clé de la
  paire, `count_pair` depuis la racine de Z global ; les crédits ne sont
  jamais préchargés, la coquille garde les extrémités. Les invariants de
  masse annoncés (S = R + R_Pool, C = P − R_Pool, racines = résiduelle −
  passthrough, partition conjointe sur C − pair_roots) sont recontrôlés
  par mon vérificateur à chaque exécution.
- **Coût cumulé.** Deux passes par facteur (sélection à tampon fixe de
  K+1 propositions, certification à au plus K+1 témoins par site) :
  `factor_read_visits = 2F`, `grouping_visits = 2F`, préfixes en K+1
  visites par plan. C'est bien O(KF) par plan ; F lui-même n'est borné
  que par la famille (7n sur les amas), pas par la WSPD.

**Campagne de force brute.** Mon vérificateur de chaîne porte sept
combinaisons filtrées de plus (`-DMHGP8_AUDIT_POOL`, option `--pool`) :
seuils 1, 2 et 64, fronts Pure et MidpointSamples, Pairwise ou
SharedBlocks avec frère et Complement, avec ou sans modes conjoints.
Reçu : [CHAINE_Q2_POOL_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_POOL_CHECKS.json)
(86 exécutions × 25 combinaisons, 327 303 000 paires contrôlées,
10 498 825 supports attendus et émis, **0 désaccord**, invariants de
masse tenus partout, rejoué en `python3 -O`). Les hachés des cinq
fichiers produit consommés sont ceux de ba11e3ab.

Deux faits structurels pour la politique de seuil (sommes sur les 86
exécutions, front MidpointSamples) :

| Seuil | Rectangles sélectionnés | dont sans réduction | Paires filtrées | Racines de paires | F = Σ(|A|+|B|) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 1 425 383 | 1 424 123 | 3 697 970 | 13 626 | 3 354 407 |
| 2 | 341 275 | 340 015 | 3 697 970 | 13 626 | 1 186 191 |
| 64 | 231 | 7 | 3 695 603 | 10 681 | 58 800 |

Le seuil 64 capture 99,94 % de la masse filtrable pour 57 fois moins
de sites de facteurs relus : sur ces nuages, le filtre ne retire rien
sur 99,9 % des rectangles sélectionnés aux seuils 1 et 2, et le repli
sans réduction y est la règle. Avec le front **Pure** au seuil 2, la
situation change : 1 492 975 rectangles sélectionnés dont 1 163 187
sans réduction, 425 850 bandes et 785 944 racines de paires, contre
13 626 en MidpointSamples. Les rectangles partiellement réduits y sont
nombreux, et c'est précisément le cas où une expansion paire par paire
peut coûter plus que la route partagée : le « cas partiellement
sélectif » que le constructeur annonce mesurer devrait l'être avec les
compteurs passthrough sur ce front-là, pas seulement sur Samples.


## Onzième tranche (b2106c3c) : les modes conjoints sont exacts de bout en bout

Les sources de la onzième tranche (`Q2AnchorMode::SharedProduct` /
`SharedAnchors`, `q2_joint_bounds.hpp`, `q2_census.cpp` étendu) ont
d'abord été vérifiées sur un instantané du worktree pris à 13 h 14 UTC,
avant leur publication ; les 25 fichiers de `src/` de cet instantané
sont octet pour octet ceux du commit b2106c3c. La vérification a ensuite
été rejouée contre `git archive b2106c3c` avec les mêmes résultats et le
même condensé stable ; c'est ce rejeu ancré sur le commit que le reçu
conserve. Mon vérificateur de chaîne porte dix combinaisons conjointes
de plus (`chain_verify.cpp -DMHGP8_AUDIT_JOINT`, option `--joint` du
runner) : front Pure ou MidpointSamples, SharedBlocks, frère
Disabled/Saturating, ordre GlobalDfs/ComplementFirst, ancre
SharedProduct/SharedAnchors, toujours contre la force brute exacte sur
tous les sites. Reçu :
[CHAINE_Q2_JOINT_CHECKS.json](chaine_q2_20260914/CHAINE_Q2_JOINT_CHECKS.json),
rejoué en `python3 -O` sur les familles adversariales.

Résultat : 86 exécutions × 18 combinaisons, 235 658 160 paires
contrôlées, 7 559 154 supports attendus et émis, **0 désaccord**, aucun
doublon ; les deux invariants annoncés par le constructeur tiennent
partout (partition des candidates en rejetées / admises / transmises,
et `count_root_starts` = `root_products` = rectangles d'entrée). Le
compteur d'admission conjointe vaut 0 sur les 860 exécutions conjointes,
comme le prédit la note (à A non singleton, le bloc Z = A est toujours
indécis et le test de diagonale impose la division de requête) ; ce n'est
pas une branche morte à masquer, c'est une conséquence de la politique.
J'ai aussi relu le gate `q2_joint_gate.cpp` en chantier : ses planchers
de non-vacuité couvrent les deux bras (fixture à six sites exigeant au
moins six rejets conjoints et un crédit pour SharedProduct comme pour
SharedAnchors, relais après crédit, `splits_b = 0` propre à SharedAnchors,
cinq modèles mutants). Rien à redire de ce côté.

Deux faits structurels sur ces petits nuages (n ≤ 2 000, aucun temps,
sommes sur les 86 exécutions, front MidpointSamples) : SharedProduct
rejette 858 620 des 6 775 343 candidates en phase conjointe (12,7 %,
16,1 % en ComplementFirst) au prix de 7,15 M tâches et 15,3 M tests de
bornes ; SharedAnchors n'en rejette que 1 980 (0,03 %, 10 136 en
ComplementFirst) pour 1,59 M tâches et 1,13 M tests. Le certificat frère
tombe à 171 263 rejets sous SharedProduct contre 1 737 232 en mode
individuel ou SharedAnchors : les divisions de B faites en phase
conjointe ne testent pas le frère, et les relais reçoivent des B déjà
petits. C'est cohérent avec la note du constructeur (frère limité au
chemin à ancre fixe) ; à lui de dire, sur 8k/16k/32k, si les rejets
conjoints de SharedProduct paient leurs tâches.

## Pourquoi le census conjoint ne remplace pas Pool sur les amas (lecture des 36 mesures r2)

Les mesures appariées du reçu r2 en chantier (`receipts/q2_joint_r2_20260914/paired_8k`,
Kmax 10, s 8/10/12, Complement/frère, un fil, hôte partagé) disent la
même chose que mes comptages structurels. À 8k, `joint` (SharedProduct)
ne gagne nulle part : uniforme 5,00 → 5,07 s, terrain 0,92 → 0,94 s,
amas 12,08 → 13,28 s bien qu'il rejette 7,49 des 29,7 millions de
candidates en phase conjointe (25 %), et deux rangées 0,24 → 1,88 s
(×8, 16,3 millions de tâches conjointes pour 16,1 millions de candidates,
soit le relais singleton partout). `joint-a` (SharedAnchors) est neutre
en temps (amas 12,12 s, rangées 0,23 s) avec 391 540 rejets conjoints
sur les amas et zéro sur les rangées.

L'explication est géométrique, pas une question de constantes. Le
certificat conjoint est **uniforme sur trois boîtes** : il faut
H(a, b, z) > 0 pour tout a ∈ A', b ∈ B', z ∈ Z. Sur un produit
inter-amas, les seuls témoins existants sont les sites de A « en avant »
de a vers B (les autres amas sont hors des boules diamétrales). Un bloc
Z de l'amas A n'est uniformément intérieur pour A' × B' que si Z est
tout entier devant A' le long de l'axe du produit, avec une marge de
l'ordre de |z − a|² / D : il faut donc d'abord découper A en blocs
assez petits et séparés le long de cet axe, ce que la bisection au
milieu ne fait qu'après plusieurs niveaux, et chaque niveau double les
tâches. C'est ce que montrent 31 millions de tâches conjointes pour
7,5 millions de rejets sur les amas, et sur mes petits nuages 87 % de
la masse transmise au relais singleton après 15,3 millions de tests de
bornes. Le crédit Pool `h_a`, lui, fixe l'ancre : il ne boxe que B, et
l'ensemble des z ∈ A devant a est un suffixe de l'ordre de projection,
obtenu en un tri par rectangle. Il certifie 99,98 % des paires du
produit 0×1 pour 7 ms, là où le certificat à trois boîtes en certifie un
quart pour une seconde de plus.

Conséquence pour la décision en cours : garder Individual ou
SharedAnchors comme chemin de census (neutres), et confier les gros
rectangles au filtre Pool sur le propriétaire global, comme A le mesure
(amas 32k : 204,7 → 21,9 s, supports identiques). Le partage conjoint
reste correct (vérifié ci-dessus) mais ne porte pas les témoins
intra-facteur ; il ne faut pas lui demander ce que seul un certificat à
ancre fixe peut donner. Les rangées sont la contre-fixture à conserver
pour SharedProduct.

## Survivantes de Pool : ce que le census résiduel devra produire (question A/B du raccord)

Réponse à la question posée au journal (« partager le plan local sur le
propriétaire global, puis restreindre les requêtes sans ajouter ses
minorants au compte du census »). L'auditeur A construit déjà ce raccord
sur le propriétaire global (`q2_pool_bridge_20260914/`, bras
baseline / pool-pair / pool-shared sur LiDAR et amas) : je ne le double
pas. Ma part est la vérité terrain sur ce que le filtre laisse passer,
mesurée sur les sources da366f7f et les mêmes 28 rectangles que la note
des crédits terminaux
([§ 3 bis](CREDITS_TERMINAUX_20260914.md#3-bis-ce-que-les-survivantes-contiennent-réellement),
reçu `credits_terminaux_20260914/SURVIVANTS_CHECKS.json`).

| Amas, n | Survivantes Pool | Survivantes DualBlocks | Vrais supports q2 | Rejets non sûrs (échantillon) |
| --- | ---: | ---: | ---: | ---: |
| 8k | 11 329 | 7 718 | 2 140 | 0 / 559 790 |
| 16k | 29 878 | 14 019 | 3 085 | 0 / 559 870 |
| 32k | 102 993 | 31 419 | 4 690 | 0 / 559 866 |

Les produits inter-amas ne sont donc pas une pure certification : le
census résiduel doit y retrouver 4 690 supports à 32k, presque tous dans
les douze produits d'arêtes, et cette population croît comme une surface
(×1,5 par doublement) quand les survivantes de Pool croissent ×3,4. Le
rapport survivantes/vérité passe de 5,3 à 22 pour Pool, de 3,6 à 6,7
pour DualBlocks : le filtre le moins cher se dégrade avec n, et c'est
le nombre de survivantes, pas le nombre de rectangles, qui fixera le
coût du census résiduel. Une précision utile pour les portes à
empreintes : A trouve 29 688 et 102 336 survivantes à 16k/32k contre
mes 29 878 et 102 993, parce que les projections égales sont départagées
par IDs (originaux chez A, locaux dans mon harnais). Le résidu Pool
n'est donc pas une grandeur canonique ; seuls les supports complets le
sont, et ce sont eux qui doivent porter l'empreinte d'une porte.

Sur la composition elle-même, trois points mathématiques, sans surprise
mais qu'il vaut mieux écrire :

- **Filtre sans crédit hérité : sûr par construction.** Un crédit Pool
  pour (a, b) est un minorant certifié du nombre de sites strictement
  intérieurs à la boule diamétrale, calculé dans A ∪ B ; il reste un
  minorant dans tout nuage contenant A ∪ B. Le rejet à crédit ≥ Kmax
  est donc sûr quel que soit le propriétaire, et le census sur les
  survivantes, reparti de zéro et de la racine, est exact par lui-même.
  Aucun double compte n'est possible puisque rien n'est hérité. Les
  seuls transferts interdits sont vers une autre paire, un autre seuil
  ou une égalité (H = 0 est coquille, jamais crédit : fixture 5 du
  brouillon du constructeur).
- **Somme ou maximum.** Deux minorants totaux d'une même paire se
  combinent par le maximum ; ils ne s'additionnent que si leurs familles
  de témoins sont prouvées disjointes. C'est ce qui autorise les crédits
  côté A et côté B d'un même plan (facteurs disjoints d'un rectangle
  WSPD) et ce qui interdit d'ajouter un crédit parental à un crédit
  local recalculé sur le même facteur.
- **Plages sélectionnées et amortissement.** Le résidu Pool d'une ancre
  est un intervalle de l'ordre de projection de B, pas un sous-arbre de
  l'index : un census partagé sur ce résidu exige soit une reprise paire
  par paire depuis la racine, soit un arbre de requête construit sur les
  rangs résiduels (ce que fait le pont de A par classe de crédit, une
  fois par classe et non par ancre). Le plan q2 seul coûte 7 / 15 / 32 ms
  sur les 28 rectangles (linéaire en Σ(|A|+|B|) = 7n) ; au seuil 2, mon
  reçu précédent le voyait monter à une seconde à 8k parce que le nombre
  de plans explose : c'est R·(|A|+|B|) qu'il faut borner, et le seuil de
  taille est la seule politique qui le fait aujourd'hui.

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

Fichiers de B : ce dialogue, sept notes datées et les reçus
`wspd_regime_20260914/`, `propagation_temoins_20260914/`,
`credits_terminaux_20260914/` (deux reçus : crédits et survivantes),
`chaine_q2_20260914/` (six reçus : e3af11a7, modes conjoints b2106c3c,
filtre Pool ba11e3ab, chaîne parallèle et équilibre b268cf6f,
redistribution dynamique 4e878754, continuations d09e2207, détachement 897085f8, équipe coopérative beee3341, plages d'ancres 2741d614, lots singleton sur sources en chantier) et
`separation_20260914/` et `oracle_q3q4_20260915/`. Aucun
fichier des autres auditeurs ni du constructeur n'est modifié. Mes
propositions d'archivage des anciens reçus Rectangle/Tubes sont retirées :
A indique que leurs reproductions les utilisent encore, et ils restent
donc à leur chemin. Ne pas déplacer `P0_INPUT_ALIAS_CHECKS.json` (épinglé
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
