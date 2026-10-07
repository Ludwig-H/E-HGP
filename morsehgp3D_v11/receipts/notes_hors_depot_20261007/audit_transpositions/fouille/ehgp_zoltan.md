# Fouille E-HGP et Zoltan : ce qui se transpose utilement dans la v11

4 octobre 2026 ; heures lues par `date -u` : mesures locales de 12 h 55 à 13 h 04 UTC, rapport achevé à 13 h 06 UTC. Auditeur de la source
**E-HGP** (`/workspaces/E-HGP/E-HGP/`) et **Zoltan** (`/workspaces/E-HGP/Zoltan/`), pour l'audit des
transpositions (`../CONTEXTE.md`). Contrat rappelé par l'utilisateur : **100 ms** à K = 5 (et si possible K = 10)
sur G4, trames SemanticKITTI sans sol, moteur entier exact.

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Étiquettes : **M** mesuré (reçu ou sortie nommés), **M-loc** mesuré localement sur un oracle borné (petits
nuages, codespace), **E** estimé (arithmétique sur des mesures citées), **C** conjecturé. Chemins v11 relatifs à
`morsehgp3D_v11/` sur `origin/main` (`eb036dbe2`, lu dans `build/v11-claude-20261003`). Les fichiers E-HGP cités
sont ceux de l'arbre de travail ; leur dernier commit est donné quand il existe (plusieurs sont modifiés ou non
suivis : `bench/gate_separation.py` n'est pas commité).

## 0. Verdict

1. **Vers 100 ms : rien.** Aucune brique d'E-HGP ni de Zoltan ne divise le CPU de la passe unique (167–180 ms à W48)
   ou de la résolution régulière (86–116 ms), ni ne casse le squelette séquentiel (~100 ms) chiffré par
   `../cartes/CARTE_V11.md` § 6. E-HGP est une référence Python rationnelle à budget **cubique à quartique**
   (`E-HGP/docs/MOTEUR_ET_COUTS.md` § 4) ; ses briques de calcul ont déjà leur équivalent exact dans la v11 (la
   descente MEB-Lloyd est la descente T5, la restriction par tube est le census local G2, la MEB est déjà à ~1 % du
   CPU). Zoltan est la conception d'un réseau qui **consomme** la tour ; il ne touche pas au moteur.
2. **Retenu : trois juges à l'échelle, tous tirés d'E-HGP**, qui comblent un manque déclaré de la v11 : sur les
   trames du contrat, la v11 n'a **aucun oracle géométrique** (`bench/full_semantic.py` l. 1 : « without a geometric
   oracle on the measured cloud » ; `docs/FULL_FORESTS.md` l. 144–146), alors que `docs/ARCHITECTURE.md` l. 150–152
   exige « (c) des invariants globaux à l'échelle » et que le différentiel v10/v11 sur LiDAR reste ouvert
   (`docs/DEVELOPPEMENT.md` l. 65 : « les 81 prises comparent deux v11 »). Une faute déterministe commune à tous les
   modes et à tous les W est invisible aux comparaisons appariées ; ces juges la voient.
3. **Mesuré localement** (oracle de définition de la v11, script `ehgp_zoltan_verif.py`) : 0 violation sur
   565 311 contrôles de tranche et 27 754 paires de segments ; mutants détectés un par un dans 30,7 à 48,8 % des cas
   sur des nuages minuscules et denses ; convention N-aire de l'ordre 1 : 0 désaccord sur 1 610 fusions (304 N-aires).
4. **Zoltan : rien de retenu.** Ses exigences sur la condensation (ne jamais binariser, départage canonique,
   état daté d'un nœud) sont déjà dans la v11 ; le reste (masses fractionnaires du § 9.1, seuil relatif α,
   CutBundle) vise le tokenizer ou n'est pas mesuré.
5. Aucune piste fermée n'est rouverte. Une piste fermée par E-HGP (DTM et Delaunay pondéré comme raccourci vers
   D_k) est rappelée au § 5 pour qu'elle ne revienne pas.

## 1. Ce qui a été lu

- **E-HGP** : `README.md` ; `docs/OBJET_ET_DIMENSION.md`, `OBSTRUCTION_GRANDE_DIMENSION.md`,
  `REGULARISATION_ENTROPIQUE.md` (§ 1, 4, 5), `MOTEUR_ET_COUTS.md` ; `audits/ETAT_COURANT.md`,
  `AUDIT_OUVERTURE_20260926.md` (§ 2–3), `FIXTURES_AUDIT_20260926.md` ; code `engine/critical.py`, `segment.py`,
  `separation.py`, `fast_point_tower.py` (en-tête), `exact/grid_judge.py` (en-tête) ; portes `bench/gate_separation.py`
  (en-tête), `tests/test_moteur_segment.py` (ordre un). Les couches `soft/` et `spectral/` ont été lues par leurs
  documents seulement : elles changent l'objet (§ 5).
- **Zoltan** : `README.md` ; `FoundationModel/README.md`, `AUDIT_V9_ET_ARCHITECTURE_20260926.md`,
  `CONTRAT_COUPES_ET_MASSES_20260926.md`, `SPECIFICATION.md` (§ 2–3), `ARCHITECTURE.md` (§ 4), `OBJET.md` (§ 5) ;
  `reference/verify_*.py` (en-têtes) ; `demos/README.md`, `demos/tools/hierarchy.py`, `ground.py`.
- **v11, pour vérifier la présence de chaque idée** : `docs/MATHEMATIQUES.md`, `HIERARCHIE_POINTS.md`,
  `SORTIE_PLATE.md`, `FULL_FORESTS.md`, `FULL_CENSUS_REUTILISE.md`, `ARCHITECTURE.md` § 5–6, `DEVELOPPEMENT.md`,
  `reference/README.md`, `reference/hgp11_ref/{model,definition}.py`, `bench/full_semantic.py`,
  `bench/points_gate.py`, `bench/points_flat_gate.py`, `src/tower/descent.cpp`,
  `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md`, reçu `receipts/qualification_performance_20261003/review/analysis.md`.
  Recherches `git grep` : aucune occurrence d'un certificat de séparation, d'un juge d'arbre couvrant minimal, ni
  d'une identité d'Euler dans `src/`, `tests/`, `reference/`, `bench/`.

## 2. Vitesse : pourquoi rien ne se transpose

| Mécanisme E-HGP / Zoltan | Équivalent v11 | Verdict pour 100 ms |
| --- | --- | --- |
| Descente MEB-Lloyd : m plus proches, centre de leur MEB, recommencer (`engine/critical.py` l. 8–23 ; théorème C, `REGULARISATION_ENTROPIQUE.md` l. 275–318) | Descente T5 (`docs/MATHEMATIQUES.md` l. 212–217) : même décroissance stricte de β, forme combinatoire exacte avec choix d'une t-partie séparable ; c'est l'étage 3d de la v11 | déjà là ; la v11 a en plus la table de populations (3,62 M succès sur 4,80 M pas, ng00, CARTE_V11 § 2.5) |
| Catalogue « sans énumération », par départs de descente (`README.md` E-HGP l. 39–45) | Catalogue énuméré, complet sous G1–G4 (`MATHEMATIQUES.md` § 3) | contraire à la doctrine : complétude « non acquis » (`README.md` E-HGP l. 148), faux positif cosphérique B1 (`AUDIT_OUVERTURE_20260926.md` l. 68–80) |
| MEB par Welzl exact proposé (`MOTEUR_ET_COUTS.md` l. 44) | `bounded_meb` ≈ 0,9 % du CPU W1 (CARTE_V11 § 2.4, M) | gain ≤ 1 % |
| Restriction par tube et monotonie par sous-nuage (`engine/fast_point_tower.py` l. 64–92) | census local G2 des feuilles (`MATHEMATIQUES.md` § 3) ; census emprunté et lecture du catalogue (`docs/FULL_CENSUS_REUTILISE.md` l. 17–24) | déjà là ; le certificat de tube exige le site le plus proche hors de la liste, donc une requête globale |
| Balayage aux rangs k−1, k, k+1 le long d'un segment (`fast_point_tower.py` l. 28–33) | sans objet dans le moteur | sert seulement au juge de l'idée 02 |
| Niveau de Fermi, comptage doux, log-densité spectrale (`soft/`, `spectral/`) | sans objet | change l'objet (« tour régularisée », `OBJET_ET_DIMENSION.md` § 6) |
| Coupes par lot, CutBundle, flux d'incidences (Zoltan `AUDIT_V9_ET_ARCHITECTURE_20260926.md` § 1 ; `CONTRAT_COUPES_ET_MASSES_20260926.md` § 3) | hors du chrono FULL | sert l'export vers un réseau, pas le contrat |

Conclusion : pour la vitesse, la réponse de cette source est **aucune**. Les leviers vers 100 ms sont ailleurs
(compteurs de la boucle chaude, q3 différé, arènes, census des descentes, mémo daté, GPU : `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` § B).

## 3. Idées retenues

Les trois sont des **outils de test** : gain direct vers 100 ms nul. Elles jugent la **référence elle-même** sur les
trames du contrat, à W48 et à K = 5 comme à K = 10, ce que ne fait aucune comparaison appariée entre variantes v11.
Elles respectent la doctrine du dépôt : invariants globaux et juge d'échantillon, aucun juge O(n³), aucun tableau
indexé par paire, décisions en entiers ou rationnels exacts (le flottant ne fait que proposer des surfaces ou des
paires), conditions **nécessaires** seulement (jamais une promotion de statut).

### ehgp_zoltan_01 — Juge de tranche sur tout le dump FULL (minorants exacts, invariant global)

**Source.** Le certificat de tranche d'E-HGP (`docs/MOTEUR_ET_COUTS.md` l. 111–121, commit `110f375e3`) : « Soit
H une hypersurface qui sépare strictement x_i de x_j. Si au plus k−1 observations ont leur boule B(x_l, √a) qui
rencontre H, alors H ne rencontre pas L_k(a) ». Implanté pour plans et sphères dans `src/ehgp/engine/separation.py`
(l. 9–33 énoncé et preuve, l. 48–100 plans, l. 171–240 sphères sans racine carrée ; `94df32d51`). Sûreté auditée :
0 faux certificat sur 244 confrontés à une ultramétrique recalculée (`audits/AUDIT_OUVERTURE_20260926.md` l. 59).
Leçon de porte : F-AUD-4 (`audits/FIXTURES_AUDIT_20260926.md` l. 80–98), un mutant « d'un cran » acceptait 41 668
certificats faux sous une porte qui ne regardait que la valeur finale ; d'où les trois mutants causaux et les
planchers de `bench/gate_separation.py` l. 1–80 (non commité).

**Mécanisme transposé.** E-HGP l'emploie paire par paire ; dans la v11 on l'applique **à toute la forêt d'un coup**,
en O(nœuds) par surface. Pour une surface fermée S (bord d'une boîte alignée aux axes, bornes demi-entières) et un
ordre k, poser A_k(S) = k-ième plus petite distance carrée site → S (rationnelle, O(n)). Alors, à la coupe fermée :

- T1 : toute naissance de niveau < A_k(S) a son centre exact hors de S ;
- T2 : tout nœud de niveau < A_k(S) a toutes les naissances de son sous-arbre **du même côté** de S (masques de côtés
  propagés des enfants au parent ; 64 surfaces par mot machine) ;
- T3 : tout nœud v d'ordre k ≥ 2 de niveau < A_{k−1}(S) a une image verticale dont les naissances du sous-arbre sont
  du même côté que celles de v.

Preuve : la composante d'un nœud à son niveau est connexe, contient le centre de chacune de ses naissances (pour une
naissance, D_k(c) = λ), et ne rencontre pas S si son niveau est sous A_k(S) ; L_k ⊆ L_{k−1} donne T3. Le dump v11
contient tout ce qu'il faut : sites, niveaux exacts, centres rationnels des naissances, parents, enfants et
verticales (`bench/full_semantic.py` l. 104–160). Sur une trame entière, un plan infini traverse la scène et ne
certifie rien : les surfaces utiles sont des **bords de boîtes** posés dans le vide qui entoure un objet ou sépare
deux lignes de balayage (proposition par grille d'occupation flottante, décision exacte de A_k(S) et des côtés).

**Ce qu'il attrape.** Toute fusion parasite à travers un vide certifié, à tout ordre et tout W ; un centre de
naissance faux ; une verticale qui change de côté. Aussi une corruption du dump que le codec peut accepter : un
niveau de fusion abaissé à l'intérieur de l'intervalle (niveau du plus haut enfant, niveau du parent) reste
structurellement valide pour `full_semantic.py` tant que l'ordre canonique des fusions est préservé.

**Preuve d'intérêt.** M-loc (§ 4) : 0 violation sur 378 772 contrôles T1, 89 935 T2 et 96 604 T3 (150 nuages de
8 sites, K = 3, 240 surfaces aléatoires par nuage) ; mutants détectés un par un : fusion précoce 30,7 %
(13 229 / 43 048), verticale fausse 37,3 % (2 922 / 7 838). Ces nuages sont denses et sans vide : le minorant de
tranche ne vaut que 0,495 fois le vrai niveau de rencontre en médiane. Sur LiDAR sans sol, objets et lignes de
balayage sont séparés par du vide : les certificats devraient y être bien plus serrés pour les fusions entre objets
(C). Une faute systématique qui produit m fusions fausses échappe avec une probabilité de l'ordre de (1 − p)^m si les
détections sont indépendantes (C).

**Déjà dans la v11 ?** Non : codec structurel et naturalité seulement (`full_semantic.py` l. 1, 36–59, 155),
bilans de compteurs (`analysis.md` du reçu de qualification), identité entre W et modes.

**Coût (E).** ~300 lignes de Python nu greffées sur le décodage en flux de `full_semantic.decode`, plus fixtures et
mutants : les trois mutants du juge repris d'E-HGP (compte « au plus k » au lieu de k−1, seuil non strict, côté non
strict) et deux mutants injectés dans un dump (niveau de fusion abaissé, verticale échangée). Calcul : A_k(S) en
O(n) par surface ; côtés de 0,90 M naissances (ng00, K = 5 ; CARTE_V11 § 2.5) × 64 surfaces ≈ 6·10⁷ opérations
entières, de l'ordre d'une minute en Python nu par trame, en plus du décodage (E, non mesuré).

### ehgp_zoltan_02 — Juge d'échantillon par segments (majorants exacts des rencontres)

**Source.** Le fait de segment (`E-HGP/docs/OBJET_ET_DIMENSION.md` l. 110–136 ; `src/ehgp/engine/segment.py`
l. 1–25 et 56–103, `94df32d51`) : le long de y(t) = p + t(q − p), toutes les énergies partagent le coefficient
dominant, donc a_k(y(t)) = A t² + (k-ième plus petite de n fonctions affines) et son maximum est atteint en t = 0,
t = 1 ou à un croisement ; calcul exact à une variable. Audité « tient » (`AUDIT_OUVERTURE_20260926.md` l. 57).
Localité : monotonie par sous-nuage et certificat de tube (`engine/fast_point_tower.py` l. 64–92, `8cd859193`) ;
balayage aux rangs k−1, k, k+1 (l. 28–33).

**Mécanisme transposé.** Pour deux naissances b1, b2 d'un même ordre, de centres exacts c1, c2 (dans le dump), le
niveau de leur plus petit ancêtre commun est au plus max_t a_k(c1 + t(c2 − c1)) : le segment entier est dans L_k de
ce niveau et relie les deux composantes. Calcul sur C = sites d'une boîte autour du segment : a_k sur C majore a_k sur
X, donc **toute** liste C donne un majorant encore valide (le certificat de tube ne sert qu'à le rendre exact).
Échantillon : pour des nœuds de fusion tirés au sort, deux naissances proches prises dans deux enfants distincts
(recherche bornée par une grille des centres).

**Ce qu'il attrape.** Les fusions manquantes ou tardives : boules perdues par le catalogue (la v10 a laissé survivre
un mutant de frontière qui perdait 2 134 à 9 523 boules avec `status ok`, CARTE_V10_VITESSE § 4, N3), plateau N-aire
mal découpé, multifusion publiée trop haut.

**Preuve d'intérêt.** M-loc : 0 violation sur 27 754 paires de naissances ; égalité dans 4,0 % des paires (1 109) ;
rapport majorant / vrai niveau de rencontre : médiane 1,162, neuvième décile 1,444 ; mutant « fusion repoussée au
niveau du parent » détecté dans 48,8 % des cas (13 539 / 27 754).

**Déjà dans la v11 ?** Non.

**Coût (E).** Par paire O(|C|²) en version naïve, O(|C| log |C|) avec le balayage aux trois rangs ; |C| de quelques
dizaines à quelques centaines de sites ; 10³ à 10⁴ paires par ordre en minutes de Python (E, non mesuré). Juge
d'échantillon, sans tableau par paire.

### ehgp_zoltan_03 — Juge exact de l'ordre 1 à l'échelle (arbre couvrant minimal entier, plateaux N-aires)

**Source.** `E-HGP/docs/OBJET_ET_DIMENSION.md` l. 81–97 (`94df32d51`) : π₀(L₁(a)) = composantes du graphe
{‖x_i − x_j‖² ≤ 4a}, la tour d'ordre 1 est le dendrogramme de l'arbre couvrant minimal euclidien, niveaux d²/4 ;
porte `tests/test_moteur_segment.py` l. 272–275 et 828–865 (`a2aebeb52`) ; juge extérieur `scipy` : 28 accords,
0 désaccord, d de 1 à 60 (`AUDIT_OUVERTURE_20260926.md` l. 53). Dans la v11, c'est le juge **J2** de
`docs/MATHEMATIQUES.md` l. 345–349 (« Comparer la structure N-aire aux plateaux, pas seulement le multiensemble des
longueurs »), **énoncé mais non implanté** ; seule la fixture F13 de `bench/points_flat_gate.py` l. 313–323 contrôle
k = 1, sur 24 petits nuages et par la route des points.

**Mécanisme.** Arbre couvrant minimal exact en entiers (Borůvka sur grille uniforme ; coordonnées entières u21,
longueurs carrées entières), puis Kruskal sur ses n − 1 arêtes avec **unions simultanées** à longueur égale (tout
arbre couvrant minimal donne les mêmes composantes fermées à chaque seuil) ; forme canonique (niveau, enfants vus
comme ensembles de sites) comparée à la forêt K1 du dump. Sur 08/000000, cette forêt compte 79 681 nœuds :
39 885 naissances et 39 796 fusions (`Zoltan/FoundationModel/OBJET.md` l. 114–118, même trame en v9 ; mêmes
cardinalités en v11, total K1..5 = 1 541 750, CARTE_V11 § 2.5), donc 39 884 − 39 796 = **88 enfants
surnuméraires** dans des plateaux N-aires (E) : le juge exerce la publication des plateaux à W48 sur de vrais ex
æquo de la grille de 1 mm.

**Preuve d'intérêt.** Théorème (J2) ; M-loc : l'ordre 1 de l'étage A de la référence v11 égale Kruskal à unions
simultanées sur 200 nuages de 11 sites dans une grille 7³, 1 610 fusions dont 304 N-aires, 0 désaccord
(`ehgp_zoltan_verif_j2.out`) ; cela valide la convention de comparaison, pas le moteur.

**Déjà dans la v11 ?** Énoncé (J2), non implanté à l'échelle.

**Coût (E).** O(n log n) ; 1 à 3 minutes de Python nu pour 40–46 k sites (E). Le Borůvka flottant de la bibliothèque
`hdbscan` employé par `Zoltan/demos/tools/hierarchy.py` (l. 1–21, 36–46) peut servir de recoupement rapide, jamais de
décideur. Il juge exactement un ordre sur K : q2 manquantes sur des arêtes de l'arbre couvrant, plateaux mal groupés,
course à W48 sur la forêt K1.

## 4. Mesure locale (oracle borné, codespace)

Script : [`ehgp_zoltan_verif.py`](ehgp_zoltan_verif.py) (Python nu, `fractions` ; sha256 `ac38d28f…`), oracle =
étage A de la référence v11 (`reference/hgp11_ref/definition.py`, graphe Γ_k exhaustif). Commandes et sorties :

```text
python3 -B ehgp_zoltan_verif.py 150 4102026   -> ehgp_zoltan_verif.out    (sha256 208456af…, code 0, ~70 s CPU)
python3 -B ehgp_zoltan_verif.py j2 2 200      -> ehgp_zoltan_verif_j2.out (sha256 984f459e…, code 0, ~3 s CPU)
```

| Grandeur (150 nuages, 8 sites dans 10³, K = 3, 450 forêts, 8 465 nœuds, 36 000 surfaces) | Valeur |
| --- | ---: |
| contrôles T1 / T2 / T3 ; violations | 378 772 / 89 935 / 96 604 ; **0** |
| paires de naissances (segment) ; violations ; égalités | 27 754 ; **0** ; 1 109 (4,0 %) |
| minorant de tranche / vrai niveau : médiane, premier décile, part exacte | 0,495 ; 0,318 ; 0,2 % |
| majorant de segment / vrai niveau : médiane, neuvième décile, part exacte | 1,162 ; 1,444 ; 4,0 % |
| mutant MA, fusion précoce au niveau d'une coupe : détectés | 13 229 / 43 048 (30,7 %) |
| mutant MB, fusion repoussée au niveau du parent : détectés | 13 539 / 27 754 (48,8 %) |
| mutant MC, verticale remplacée par un autre nœud vivant : détectés | 2 922 / 7 838 (37,3 %) |
| ordre 1 contre Kruskal à unions simultanées (200 nuages, 11 sites dans 7³) | 1 610 fusions, 304 N-aires, **0** désaccord |

Portée : petits nuages denses, surfaces tirées au hasard (120 plans, 120 boîtes par nuage), aucune trame LiDAR,
aucun moteur natif. Cela établit la **validité** des trois invariants sur l'objet v11 et un pouvoir de détection non
nul ; la force à l'échelle reste à mesurer sur G4 (C).

## 5. Fausses bonnes idées écartées

| Idée (source) | Raison |
| --- | --- |
| Remplacer l'énumération du catalogue par des départs de descente MEB-Lloyd (E-HGP `engine/critical.py`, `README.md` l. 39–45) | complétude seulement mesurée, « non acquis » (`README.md` E-HGP l. 148) ; faux positif cosphérique B1 (`AUDIT_OUVERTURE_20260926.md` l. 68–80), régime fréquent sur grille de 1 mm ; la descente exacte existe déjà (T5) |
| MEB par Welzl (`MOTEUR_ET_COUTS.md` l. 44) | `bounded_meb` ≈ 0,9 % du CPU W1 : gain ≤ 1 % |
| Census des descentes restreint par tube (`fast_point_tower.py` l. 64–92) | déjà G2 et census emprunté ; certifier le tube coûte une requête globale |
| Résoudre des faces régulières par segments certifiés au lieu de descentes | un segment exige a_k le long du segment (requête de type census) pour un simple majorant ; la descente v11 fait ~1,3 pas par trace avec ~75 % de succès de table (4,80 M pas, 3,62 M succès, ng00) |
| Niveau de Fermi, comptage doux, log-densité spectrale (`soft/`, `spectral/`) | changent l'objet ; incompatibles avec FULL exact |
| DTM, distance puissance, Delaunay pondéré comme raccourci vers D_k (`REGULARISATION_ENTROPIQUE.md` l. 41–47, Fait 2 bis) | **piste fermée** par E-HGP : entrelacement seulement multiplicatif, jamais égalité |
| Relever le nuage en (x, y, z, λn) pour séparer des surfaces en contact (Zoltan `ARCHITECTURE.md` l. 303–311) | change l'objet et la dimension ; E-HGP mesure la croissance des naissances avec la dimension intrinsèque et sous bruit ambiant (`OBSTRUCTION_GRANDE_DIMENSION.md` § 1.4) ; hors contrat |
| Juge par grille (`exact/grid_judge.py`) | unilatéral, décisivement faux possible (F-AUD-9) ; la v11 a deux étages exacts et l'attendu d'intervalles (`reference/README.md`) |
| Second juge de Γ_k (`judge/gamma_bfs.py`) | doublon de l'étage A de la v11 |
| Tour à témoins (`engine/witness_tower.py`) | majorant seulement ; la v11 a la projection exacte `core` (P1) |
| « L'axe k nuit, la règle d'extraction manque » (`MOTEUR_ET_COUTS.md` § 5 bis) | mesuré en distances brutes, d ≥ 2, coupe unique ; la v11 a déjà l'EOM N-aire et ses propres mesures LiDAR (`SORTIE_PLATE.md` § 3) |
| Condensation à seuil relatif α (Zoltan `SPECIFICATION.md` l. 66–87) | non mesurée (« transférabilité reste à tester ») ; ne fixe aucune masse minimale (arbre équilibré, α ≤ 1/2) ; P08 est préenregistré avec mcs : au plus un bras E1-bis |
| Masses fractionnaires du § 9.1, flux d'incidences pondérées (Zoltan `CONTRAT_COUPES_ET_MASSES_20260926.md` § 3) | la v11 a tranché pour des masses entières (`HIERARCHIE_POINTS.md` § 5 : les triangles de T0 pèsent 8/3 < 3) |
| État daté d'un nœud, fixture `growth_ABCZ` (Zoltan `CONTRAT_COUPES_ET_MASSES_20260926.md` § 2 ; v9 `tests/tower/full_ball_tower_gate.cpp` l. 131, 509–512) | déjà couvert : relation boule forte → nœud vivant (`MATHEMATIQUES.md` P3), porte des points contre l'étage A à toutes les coupes |
| Ne jamais binariser, départage canonique (Zoltan `SPECIFICATION.md` l. 88–99, 168–186) | déjà dans la v11 (plateaux atomiques, EOM N-aire, `SORTIE_PLATE.md` § 1) |
| Coupes par lot, CutBundle (Zoltan `AUDIT_V9_ET_ARCHITECTURE_20260926.md` § 1) | export vers un réseau, hors contrat FULL |

## 6. Recommandation d'ordre et ce qu'il faut mesurer sur G4

1. **ehgp_zoltan_03** d'abord : le plus simple, exact, et il pose l'outillage commun (lecture du dump, sites,
   forme canonique). Mesurer sur les trois trames du contrat à W48 : temps du juge, nœuds comparés, plateaux N-aires.
2. **ehgp_zoltan_01** ensuite : publier, par ordre et par trame, le nombre de surfaces actives, la distribution de
   A_k(S), la part des nœuds contrôlés par au moins un certificat, et le taux de détection des deux mutants injectés
   dans un dump réel. C'est cette part qui dira si le juge est fort à l'échelle (C aujourd'hui).
3. **ehgp_zoltan_02** en complément, avec le même rapport : part des paires où le majorant est exact, mutants tués.

Complément **hors de ma source**, signalé pour la synthèse : l'identité d'Euler J3 (`docs/MATHEMATIQUES.md`
l. 350–366) est elle aussi énoncée et non implantée ; elle viserait la perte de boules du catalogue, que les juges
01–03 ne voient qu'à travers ses effets sur la forêt.

FIN
