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
`chaine_q2_20260914/` (trois reçus : e3af11a7, modes conjoints b2106c3c,
filtre Pool ba11e3ab) et `separation_20260914/`. Aucun
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
