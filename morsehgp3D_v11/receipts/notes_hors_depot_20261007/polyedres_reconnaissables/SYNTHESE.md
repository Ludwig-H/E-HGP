# Polyèdres reconnaissables tirés de la tour FULL : synthèse

6 octobre 2026, rédigée à partir de 20 h 27 UTC (heure lue par `date -u`). Réponse à la demande « je veux des polyèdres et non des points » pour le jeton polyédrique de Zoltan. Elle s'appuie sur :

- trois lectures : doctrine, mathématiques, état de l'art ;
- un harnais commun ;
- trois approches mesurées, chacune avec ses prédictions gelées ;
- une critique adverse qui a rejoué les points litigieux ;
- l'audit `7d41562c3`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference (prototypes Python en aval de mhgp11 à 07428324e, au plus 2 fils)
profile=quantized_u21_input_only (grille de 1 mm)
public_status=not_claimed
```

**Conditions de travail.**

- GCP non utilisé. Aucun fichier du dépôt modifié, aucun commit.
- Dépôt lu à `origin/main` = `9d10de213`. Les documents cités n'ont pas changé depuis `56586139c`, où les lectures les ont relevés : leurs numéros de ligne restent valables.
- Chemins relatifs à `build/v11-persist/polyedres_reconnaissables/`, sauf mention contraire.
- Aucune donnée SemanticKITTI hors de `build/`.

**Étiquettes.**

- [M] : mesure, avec son fichier.
- [E] : estimation.
- [D] : dérivation algébrique ; « vérifiée » quand un oracle borné l'a contrôlée.
- [C] : conjecture.
- [doc] : texte de la thèse, de la doctrine Zoltan ou de l'audit.

Une sélection **oracle** choisit le meilleur nœud d'après la vérité terrain. Elle borne ce que l'arbre contient ; ce n'est pas une méthode.

## 0. En bref

1. **Pourquoi les supports dessinent mal.** Un support S* témoigne d'une fusion à l'échelle de sa boule ; il ne décrit pas la surface de l'objet. [M] Sur la trame 08/000000 sans sol :
   - l'arbre d'ordre 5 compte 14,45 nœuds par site ;
   - 68 % de ces nœuds meurent à moins de 1 % au-dessus de leur rayon de naissance ;
   - le diamètre médian d'un conv(S*) y est de 34 à 36 cm.

   Le meilleur nœud d'un objet de trame réunit en moyenne 2 441 simplexes. Au moins 24 % de leur aire est traversée par des rayons capteur → retour, donc posée dans de l'espace libre observé. Sur 44 ouvertures de roues et d'anneaux synthétiques, les supports n'en laissent que 6 ouvertes.

2. **L'idée « mosaïque » est juste en mathématiques, mais elle ne dessine pas l'objet.**
   - [M] Chaque boule publiée donne exactement une cellule de la mosaïque de Delaunay d'ordre K, à son niveau exact : aucun écart sur 1 185 753 boules.
   - Ce ne sont que les cellules critiques. Une naissance donne un point ; une fusion donne un simplexe K fois plus petit que conv(S*). À K = 5, ces cellules forment 0,3 à 0,8 % des 3-cellules de la mosaïque [M].
   - Une fois complétée, la mosaïque est exacte et laminaire à K fixé. Mais elle dessine la multi-couverture : un bloc de barycentres qui remplit les roues et enjambe les occultations, avec environ 1 000 faces par point à K = 5 [M].

3. **Méthode recommandée, en deux temps** (§ 2) :
   - **quels polyèdres** : l'arbre condensé N-aire de la hiérarchie laminaire de FULL, avec un seuil absolu de masse, une EOM N-aire et les ordres K traités en branches parallèles. On obtient environ 1 000 nœuds et 128 à 192 jetons par trame [M] ;
   - **comment les dessiner** : la surface effectivement observée, construite une fois sur la grille d'acquisition. Ses faces sont attribuées aux états datés de la hiérarchie par la règle de l'audit (LCA, date β, réserve), sans aucune enveloppe convexe ;
   - **budget mesuré** : une médiane de 31 à 44 faces par jeton. Les jetons d'un même niveau se partagent les faces de la trame, si bien qu'un niveau entier coûte au plus 0,9 à 1,3 face par point.

4. **Ce qui n'est pas acquis** :
   - une sélection sans vérité meilleure que la tête plate existante ;
   - la masse vraie du § 9.1 : la sortie supports v2 ne permet pas de la calculer ;
   - une surface robuste sur KITTI réel : la variante déclarée y sur-découpe, et les variantes qui marchent ont été choisies après coup ;
   - des parties sur LiDAR réel, faute de vérité de parties ;
   - un coût compatible avec le budget de 100 ms.

## 1. Diagnostic

### 1.1 Trop nombreux : un arbre couvrant d'ordre K, pas une hiérarchie d'objets

[doc] La sortie `--sortie=supports` (MHGP11SP v2) publie l'arbre couvrant d'ordre K (`morsehgp3D_v11/docs/SORTIES.md:293-307`) :

- toutes les naissances ;
- les fusions retenues par Kruskal dans les plateaux ;
- un support S* par boule.

[M] Comptes, tirés de `lecture_art/mesures_comptes_08_000000_k5.json`, `lecture_art/mesures_persistance_08_000000_k5.json` (et `_k10`) et `harnais/references/synthese.json` :

| Trame sans sol | K = 5 : nœuds (par site) | K = 10 : nœuds (par site) |
| --- | --- | --- |
| 08/000000 (39 885 sites) | 576 371 (14,45) | 1 638 573 (41,08) |
| 08/000100 | 478 265 (13,45) | 1 265 065 (35,59) |
| 08/000200 | 609 376 (13,29) | 1 555 780 (33,94) |

- À K = 5, 59 % des nœuds sont des naissances (341 081).
- 67,8 % des nœuds à K = 5, et 85,5 % à K = 10, meurent à moins de 1 % au-dessus de leur rayon de naissance (rapport des rayons parent / nœud < 1,01). Ce sont des nœuds de passage.
- Rien, dans cet arbre, ne distingue le nœud « vélo » de ses milliers d'ancêtres et de descendants de passage : il n'y a ni masse, ni seuil, ni sélection. Le premier défaut est un défaut de **sélection**.

### 1.2 Mal dessinés : des témoins de fusion à l'échelle de la boule

- **[D] Taille d'un support.** Une boule d'ordre K contient au plus K − 1 points intérieurs. Son rayon est donc celui du K-ième voisin, et S* (2 à 4 sites sur la sphère, centre dans l'intérieur relatif de conv S*) en couvre le diamètre.
- **[M] Ordres de grandeur.**
  - À K = 5 : rayon médian de 174 mm aux naissances et de 187 mm aux fusions ; diamètre médian de conv(S*) de 340 et 364 mm.
  - À K = 10 : diamètre médian de 520 et 543 mm.
  - [E] C'est l'échelle du rayon d'une roue de vélo (environ 33 cm). Un support posé sur la jante coupe donc l'intérieur de la roue.
- **[M] Empilement.** Le polyèdre d'un nœud est la réunion des conv(S*) de son sous-arbre. Pour le meilleur nœud oracle des 19 objets de trames à K = 5 (`harnais/references/synthese.json`), on mesure :
  - 2 441 simplexes ;
  - une précision de 0,109 (part de l'aire à 30 mm au plus de l'objet) ;
  - une aire traversée par des rayons capteur → retour de 0,241 ;
  - une aire sans appui relative de 0,236.

  L'aire traversée n'utilise que les rayons du jeu sans sol : c'est une borne basse.
- **[M] Ouvertures.** Sur les 44 ouvertures propres des objets synthétiques (roues et anneaux), les supports en laissent 6 ouvertes (`critique/resultats/c4_ouvertures.json`).
- **[doc] Ce que fait Kruskal.** Il retire la redondance de **connexion**, pas celle de la **géométrie** : les conv(S*) se recouvrent (`MATHEMATIQUES.md:855-858`).
  - L'audit parle de « superposition de témoins de fusion », qui « ne reconstruit pas une surface ».
  - S* n'est pas stable aux cosphéricités : une translation entière peut changer le support choisi (`SORTIES.md:497`).

### 1.3 Ce que dessinent les boules publiées dans la mosaïque d'ordre K

C'est l'idée de l'utilisateur, prise au sérieux.

**[D, vérifiée] La cellule d'ordre K d'une boule.** Soit b une boule, I_b son intérieur, U_b sa coquille et t = K − p. Sa cellule est P_{I,U,K} = conv{(ΣI_b + ΣJ)/K : J ⊆ U_b, |J| = t} (audit § 2 ; Edelsbrunner–Osang).

- À une naissance (t = m), c'est un point : le barycentre de P_b.
- À une fusion en coquille régulière (t = q − 1), c'est conv(S*) réfléchi et réduit d'un facteur K, par l'homothétie de centre bary(P_b) et de rapport −1/K (`lecture_maths`).

**[M] Ces cellules sont exactement des cellules de Del_K, au bon niveau.** Sur 1 185 753 boules publiées, aucune cellule n'est hors de la mosaïque et aucun niveau r_b² ne diffère de la fonction rayon exacte (`approche_mosaique/resultats/synthese.json`). Le contrôle porte sur 38 jeux entiers, à k = 1, 2 et 5 ; les trames ne sont couvertes qu'à k = 1 et 2. L'oracle borné du harnais (28 petits nuages, 1 089 boules) donne le même résultat.

**[M] Mais ce ne sont que les cellules critiques.** Part de la mosaïque complète présente dans l'amorce (médianes par jeu) :

| k | sommets | arêtes | 2-faces | 3-cellules |
| --- | --- | --- | --- | --- |
| 1 | 1 | 0,13–0,14 | 0 | 0 |
| 2 | 0,31–0,34 | 0,014–0,019 | 0,018–0,020 | 0 |
| 5 | 0,08–0,12 | 0,001 | 0,006 | 0,003–0,008 |

- À k = 1, l'amorce est l'arbre couvrant euclidien minimal : 14 % des arêtes de Delaunay.
- Dans le jeton d'un nœud à k = 5, les cellules de ses propres boules publiées ne font que 1,3 à 3,5 % de ses 3-cellules.
- Dessinée seule, l'amorce est un semis de points et de petits simplexes disjoints (image 2) : seulement 47 % des retours d'un objet de trame sont à moins de 30 mm des cellules.

**[M] La mosaïque complétée est un vrai polyèdre, mais pas celui de l'objet.** La composante du nœud dans Del_K(X, a) est compacte, disjointe des autres nœuds vivants et emboîtée à k fixé. Mais :

- elle coûte 166 à 172 3-cellules par point à k = 5, soit environ 1 000 à 1 200 faces de toutes dimensions par point ;
- ses sommets sont des barycentres de K retours, tirés vers l'intérieur. À k = 5, la part des retours d'un objet à moins de 30 mm du solide vaut 0,77 (synthétique), 0,52 (découpes) et 0,69 (trames). Pour la surface observée de l'objet, ces parts sont 0,92, 0,76 et 0,87 ;
- les roues sont remplies au niveau où le nœud naît : 15 ouvertures propres conservées sur 44 à k = 5 ;
- 14 % (synthétique), 40 % (découpes) et 17 % (trames) de ses pièces relient des morceaux disjoints de la surface observée : c'est une connexité inventée ;
- sur une trame réelle, le jeton d'un cycliste compte 46 735 3-cellules, pour une précision de 0,07.

**[doc] Statut dans le dépôt.** La doctrine Zoltan ne prévoit aucune réalisation « mosaïque ». Comme produit du moteur, la mosaïque est une piste fermée (`docs/archive/abandoned/README.md:14`), et ne pas la matérialiser est un invariant de `CLAUDE.md`.

**Conclusion du diagnostic.** L'intuition est exacte : les boules de la tour sont les cellules critiques de la mosaïque d'ordre K, au bon niveau. Mais la mosaïque dessine la géométrie de la multi-couverture, pas celle de l'objet observé. C'est le bon objet pour la topologie (composantes, trous), pas le bon dessin pour reconnaître une roue.

### 1.4 Ce qui manque vraiment : la sélection

[M] L'arbre de la tour contient bien les objets et les parties. Rappel en sélection oracle, sur les mêmes jeux (`harnais/references/synthese.json`) ; le rappel est la part des objets ou des parties dont le meilleur nœud a un IoU supérieur à 0,5 :

| | trames, objets (K = 5 / 10) | découpes, objets | synthétique, objets | synthétique, 126 parties |
| --- | --- | --- | --- | --- |
| hiérarchie de points HGP H^r_{K+1} | 1,00 / 1,00 | 0,852 / 0,759 | 0,938 / 0,969 | 0,651 / 0,556 |
| HDBSCAN (sklearn, arbre condensé) | 0,947 / 0,947 | 0,593 / 0,426 | 0,938 / 0,844 | 0,444 / 0,309 |

Ce que l'arbre contient est donc favorable à HGP. Le verrou est de choisir les nœuds **sans** la vérité.

[doc, mesuré ailleurs] La tête plate EOM de la v11 manque les séparations fugaces (`SORTIE_PLATE.md:108-122`) :

- le vélo 3 n'est un bloc séparé qu'entre 107,4 et 107,8 mm ;
- une antichaîne oracle retrouverait 252 objets sur 258, contre 198 pour l'EOM.

## 2. Méthode recommandée

### 2.0 Ce qui change par rapport à la demande initiale

La demande initiale voulait des polyèdres « construits à partir des boules et des supports ». L'audit `7d41562c3` a fixé une autre cible, confirmée par l'utilisateur : la géométrie observée par le LiDAR, trous et occultations compris.

Les mesures donnent raison à cette cible. Aucune géométrie tirée des boules ne garde les ouvertures et n'évite l'espace libre comme le fait la surface d'acquisition (§ 3.1) : ni les supports, ni l'amorce, ni la mosaïque complète, ni le complexe alpha d'ordre 1.

La recommandation sépare donc trois rôles :

- les **boules** (la tour) décident du regroupement, des dates et des masses ;
- les **faces observées** portent la géométrie ;
- la **mosaïque** reste un canal structurel optionnel.

Les points restent les sommets du complexe et servent à la mesure. Un retour sans face reste un sommet isolé, déclaré comme tel (audit § 4.3) ; ce n'est pas un jeton.

### 2.1 Premier temps : quels polyèdres (sélection condensée)

1. **Hiérarchie.** On part de la lecture laminaire native de FULL, `--sortie=points` (MHGP11PT, H^r_{K+1}) : une forêt de blocs datés par indice de plateau, où chaque site reçoit son attache datée (o_i, e_i). On ne part pas de l'arbre des supports, qui compte 14 nœuds par site, en majorité de passage.

2. **Condensation N-aire**, sans binarisation (doctrine), avec un **seuil absolu de masse**, celui de la thèse.
   - Un enfant est lourd si sa masse à sa mort atteint m_min. Aucun enfant lourd termine le segment ; un seul le prolonge ; deux ou plus le scindent.
   - [M] Configuration ABS (m_min = 20) sur les trames entières (`approche_groupement/resultats/combinatoire/`) : 949 à 1 183 nœuds condensés par trame à K = 5. Cela fait 0,027 nœud par site, contre 14,45 pour les supports. Les 19 objets restent présents (rappel oracle de 1,00).
   - [M] Le seuil **relatif au parent** (α = 1/10, configuration REL20) est rejeté par sa propre règle de décision, écrite d'avance. Il ne garde que 9 objets sur 19 dans les trames, et 0 vélo sur 3 contre le mur.
   - [D] Raison structurelle : un objet de masse m qui fusionne directement avec une composante de masse supérieure à 9m est jugé léger, donc mis en réserve, quelle que soit sa persistance (image 8).

3. **Masse.** C'est celle du § 9.1 : m_τ = S_τ Σ_{x∈τ} 1/T_x.
   - [doc, D] Cette masse est un **comptage doux des points**. Chaque point distribue une masse totale de 1 entre ses faces (thèse, p. 97), d'où Σ_τ m_τ = Σ_x Σ_{τ∋x} S_τ/T_x = #{x : T_x > 0}. « Jamais un comptage » veut dire « jamais un comptage de faces ».
   - [M] La critique a vérifié que la masse de substitution (S_τ ≡ 1 sur les faces de la surface) donne la même sélection qu'un comptage des retours : même PQ au bit près, au plus 3 jetons différents sur environ 190 (`critique/resultats/c2_masse_comptage.json`). C'est attendu.
   - [C] La masse vraie (cofaces de Gabriel, ψ(ρ) = ρ⁻³) ne s'écarte d'un comptage que par la répartition des points partagés entre branches et par la réserve. Son effet serait faible sur les objets ; il reste à mesurer sur les ponts minces.
   - En attendant son export (§ 5.1), le critère A existant, qui compte les sites, en est le substitut nommé.

4. **Sélection.** Une EOM N-aire (parent en cas d'égalité, racine exclue) donne une antichaîne de jetons. On y ajoute la **vue à trois niveaux** : les jetons, leurs ancêtres (les groupes) et leurs enfants directs (les parties).
   - [M] ABS donne 128 à 192 jetons par trame. Son PQ sans vérité vaut 0,797 (K = 5) et 0,807 (K = 10), contre 0,845 et 0,769 pour la tête plate et 0,756 et 0,781 pour HDBSCAN. Ces PQ ne portent que sur 19 objets.
   - [M] La vue à trois niveaux compte 333 à 523 polyèdres par trame et contient 18 objets sur 19.
   - [M, critique] Ce n'est pas un progrès sur la tête plate. La stabilité en log r (limite z → 0) favorise les fusions, alors que l'utilisateur préfère découper que fusionner. Il faut revenir à z = 1 et faire l'ablation sur z ∈ {log r, 1, 2, 3, feuilles}.

5. **Axe des ordres.** On calcule K = 2, 5 et 10 en branches parallèles (doctrine, fait 2).
   - [M] Les parties fines sont mieux tenues aux petits K. Pour les roues du vélo à 10 m, l'IoU vaut 0,67 et 0,75 à K = 2, contre 0,48 à K = 5, où roue et cadre sont mêlés.
   - [M] La réunion oracle de K = 2, 3, 5 et 10 apporte +0,03 à +0,05 de rappel des parties.
   - [D] Avec une géométrie commune à tous les K (§ 2.2), changer d'ordre ne fait que regrouper autrement les mêmes faces.
   - [C] Le transport entre ordres pourrait donc passer par les faces partagées. Il reste à construire, avec les verticales de FULL.

### 2.2 Second temps : comment les dessiner

1. **Une géométrie observée commune**, construite une fois par trame brute, sol compris, avant toute découpe (audit § 4). C'est la **surface d'acquisition** (`approche_surface/surface_acquisition.py`) :
   - les voisins sont les voisins de tir : deux tirs consécutifs d'une même ligne (écart d'azimut ≤ 0,27°), et une triangulation « en fermeture éclair » entre lignes voisines ;
   - la continuité se juge par l'angle β de Bogoslavskyi–Stachniss, au moins 10°, qui ne dépend pas de la portée ;
   - les atomes sont les faces, les arêtes 1D (structures minces) et les sommets isolés. Chaque face porte ses retours, son aire, sa normale et sa confiance ;
   - l'espace libre est celui des rayons capteur → retour. Une face traversée par un rayon observé est retirée : c'est la « taille ».

   [D] Un retour manquant, ou un tir qui traverse une ouverture, interdit toute face par-dessus. Les ouvertures sont donc conservées par construction.

2. **Attribution datée** (audit § 5). On pose w_τ = LCA{o_i : i ∈ V_τ} et β_τ = max({e_i}, naissance(w_τ)).
   - Avant β_τ, l'atome est en réserve, visible en gris. Ensuite, il suit les parents.
   - [M] Les atomes actifs forment un sous-complexe, et les états sont emboîtés à K fixé.
   - [M] Le résultat est identique sous permutation : 25 cas sur 25 et 30 sur 30 pour le groupement, 141 sur 141 pour la surface.
   - [M] Coût : 0,12 à 0,20 s par trame en numpy.

3. **Le jeton** est l'état daté d'un segment condensé : ses faces, arêtes et sommets actifs, plus sa réserve visible. Il n'y a aucune enveloppe convexe, et aucune face n'est inventée pour rendre un bloc connexe : un vélo occulté reste en plusieurs morceaux.

4. **Réglage sur KITTI.**
   - La variante déclarée (β plus test de tendance avec un σ global) sur-découpe : 0,20 face par point, et 75 % des retours sans face.
   - La variante β seul plus taille donne 1,08 face par point et 20 % de retours sans face, dont les trois quarts portés par une arête 1D. Sur le vélo réel 08/002852, 73 % des retours sont sur une face, contre 16 % avec la variante déclarée (`critique/resultats/c8_surface_kitti.json`).
   - Cette variante a été choisie après coup. Elle doit être requalifiée avec des prédictions gelées, sur des blocs des séquences d'entraînement (§ 4).
   - Le test de tendance supprime les ponts entre nappes proches, mais il demande un modèle de bruit par laser : σ̂ vaut 5,6 à 7,1 mm sur le sol, environ 20 mm sur les voitures, et 22 à 38 mm entre lasers.

**Options de dessin comparées** (détail au § 3.1) :

| Option | Verdict | Raison principale |
| --- | --- | --- |
| supports conv(S*) | écartée comme géométrie ; gardée comme témoin de connexion | vide traversé, roues pleines, 2 441 simplexes par objet |
| mosaïque restreinte aux boules publiées (amorce) | écartée comme géométrie ; utile comme squelette exact des événements | points et petits simplexes disjoints, moins de 1 % des 3-cellules |
| mosaïque complète d'ordre K (composante de Del_K) | canal structurel optionnel, local, déclaré partiel | bloc de barycentres, environ 1 000 faces par point à K = 5, trou de complétude à l'échelle |
| complexe alpha d'ordre 1 | témoin | remplit les intérieurs de tétraèdres ; 6 à 14 % de son aire traverse l'espace libre |
| enveloppes convexes | interdites (audit, cible confirmée) | remplissent les ouvertures par définition |
| surface de base du harnais (Delaunay angulaire) | témoin | couvre l'intérieur de 25 à 28 ouvertures propres sur 44 ; 36 ponts entre les deux nappes |
| **surface d'acquisition regroupée par HGP** | **recommandée** | en synthétique : aucune traversée, 44 ouvertures sur 44, aucun pont avec le test de tendance |

### 2.3 Budget par jeton

- [M] Jetons EOM d'ABS sur les trames entières : médiane de 31 à 44 faces par jeton, moyenne de 188 à 456. La queue est lourde : façades et végétation.
- [M] Nœud d'un objet (oracle), trames à K = 5 : 254 atomes (β plus tendance) à 364 atomes (β seul taillée), soit 5,5 à 7,6 ko. Pour comparaison :
  - le meilleur nœud des supports réunit 2 441 simplexes (33,8 ko) ;
  - le jeton mosaïque compte 4 762 pièces de frontière (163 ko).
- [D] Les jetons d'une antichaîne se partagent les atomes actifs sans recouvrement. Un niveau entier coûte donc au plus le nombre de faces de la trame, soit 0,9 à 1,3 face par point [M], quel que soit K. Pour les supports, le nombre de simplexes est au moins celui des nœuds : 14,45 par site à K = 5, 41 à K = 10.
- [M] Export : la surface plus 28 octets par nœud, soit 1,2 à 1,7 Mo par trame.
- [doc, E] La règle « ×4 » de `ARCHITECTURE.md:38-57` donnerait environ 9 970, 2 490, 620, 160 et 40 jetons par niveau sur 08/000000. L'arbre condensé (environ 1 000 nœuds) et ses jetons EOM (environ 190) correspondent à ses deux ou trois niveaux les plus grossiers.
- [C] Les niveaux fins viendraient des coupes de la hiérarchie à petit r.
- **Proposition de budget** [E, non mesurée] :
  - au plus 512 atomes par jeton pour le descripteur de `JETON.md`, les faces exactes restant la référence ;
  - au-delà, une simplification QEM ou un échantillonnage par l'aire, déclaré comme estimation.

  Ce plafond vaut 12 à 16 fois la médiane mesurée, mais il ne couvre pas la queue (façades). Il faut publier la part des jetons qui le dépassent.

## 3. Preuves

### 3.1 Mesures : mêmes nœuds oracle, mêmes observations, δ = 30 mm

**Synthétique, K = 5, 32 objets** (vérité géométrique). Sources : `harnais/references/synthese.json` et `approche_*/resultats/` ; ouvertures recomptées par la critique.

| Réalisation | pièces par jeton | aire traversée | sans appui relatif | ouvertures propres conservées | hors vérité | précision | octets |
| --- | --- | --- | --- | --- | --- | --- | --- |
| supports conv(S*) | 2 153 | 0,187 | 0,126 | 6/44 | 0,179 | 0,353 | 30 293 |
| amorce (cellules des boules publiées) | 2 153 | 0,017 | 0,135 | 21/44 | 0,179 | n.d. | 37 880 |
| mosaïque complète Del_5 (frontière, création) | 6 167 | 0,020 | 0,099 | 15/44 | 0,169 | 0,385 | 135 299 |
| complexe alpha d'ordre 1 (K = 1) | 431 | 0,057 | 0,042 | n.d. | 0,085 | 0,575 | 9 784 |
| surface de base regroupée (groupement ABS) | 557 | 0 | 0,019 | 19/44 | 0,084 | 0,543 | 11 775 |
| surface d'acquisition β + tendance regroupée | 409 | 0 | 0,003 | 44/44 | 0,011 | 0,708 | 8 847 |
| surface d'acquisition β seul taillée regroupée (choisie après coup) | 430 | 0 | 0,016 | 44/44 | 0,027 | 0,651 | 9 272 |

**Trames entières sans sol, K = 5, 19 objets** :

| Réalisation | pièces par jeton | aire traversée (rayons du jeu) | précision | Chamfer (mm) | octets |
| --- | --- | --- | --- | --- | --- |
| supports conv(S*) | 2 441 | 0,241 | 0,109 | 143 | 33 799 |
| amorce | 2 441 | 0,019 | n.d. | n.d. | 44 383 |
| mosaïque Del_5 (construction locale certifiée) | 4 762 | 0,025 | 0,079 | 202 | 163 476 |
| complexe alpha d'ordre 1 | 800 | 0,113 | 0,156 | 114 | 16 410 |
| surface de base regroupée (ABS) | 346 | 0 | 0,208 | 90 | 7 640 |
| surface d'acquisition β + tendance regroupée | 254 | 0,005 | 0,298 | 76 | 5 535 |
| surface d'acquisition β seul taillée regroupée (choisie après coup) | 364 | 0 (par construction) | 0,216 | 89 | 7 561 |

**Pour lire ces deux tableaux :**

- les ouvertures propres sont celles de l'objet lui-même, soit 44 paires sur 122 ; les 78 autres paires concernent des ouvertures d'autres objets ;
- la précision est la part de l'aire à 30 mm au plus des retours de l'objet ; elle est biaisée par la portée ;
- sur les trames, l'aire sans appui exclut les retours void, ce qui crée un artefact. Elle est donc omise ici. Recalculée hors faces void, elle vaut 0,8 à 1,2 % pour la base et 2,4 à 3,8 % pour β seul.

**Ponts entre nappes proches** (synthétique, faces pontantes ; `critique/resultats/c3_surface.json`) :

| Jeu | surface de base | β seul | β + tendance |
| --- | --- | --- | --- |
| deux nappes à 6 cm (10 m) | 36 | 32 | 0 |
| rue (7 objets) | 11 | 102 | 0 |
| vélo devant le mur (10 m) | 0 | 44 | 0 |
| deux vélos parallèles (10 m) | 0 | 18 | 7 (échec) |

**Sélection et taille, trames entières, K = 5** (`approche_groupement/resultats/combinatoire/`, `critique/resultats/c2_masse_comptage.json`) :

| | nœuds par trame | jetons EOM | rappel oracle (19 objets) | PQ sans vérité (K = 5 / 10) |
| --- | --- | --- | --- | --- |
| arbre d'ordre K (supports v2) | 478 265 à 609 376 | — | 1,00 | — |
| blocs de H^r_{K+1} | 11 576 à 14 917 | — | 1,00 | — |
| condensé ABS (masse ≥ 20) | 949 à 1 183 | 128 à 192 | 1,00 | 0,797 / 0,807 |
| condensé REL20 (α = 1/10 du parent) | 381 à 615 | 22 à 65 | 0,47 / 0,53 | 0,325 / 0,380 |
| tête plate v11 (critère A, mcs 20, z = 1) | n.d. | n.d. | — | 0,845 / 0,769 |
| HDBSCAN (sklearn, mcs 20, min_samples = k) | n.d. | n.d. | 0,947 | 0,756 / 0,781 |

Les PQ portent sur 8, 6 et 5 objets selon la trame. L'échantillon est très petit : le PQ d'ABS va de 0,69 à 0,99 d'une trame à l'autre.

**Mosaïque : correction et coût** (`approche_mosaique/resultats/`, `critique/resultats/c6_trou_trame.json`) :

- oracle borné : 96 nuages, 468 comparaisons pour k = 1 à 5, aucun écart ;
- boules publiées : aucune hors de la mosaïque, sur 1 185 753 ;
- composantes : leur nombre égale celui des nœuds vivants sur 9,6 M de niveaux. C'est une égalité de comptes, pas une bijection établie nœud à nœud ;
- coût : 6,0 à 6,5, 23 à 26 et 166 à 172 3-cellules par point à k = 1, 2 et 5 ;
- trame entière : 251 à 360 s en Python à k = 2 ; non tentée à k = 5 (estimation : 6 à 8 M de 3-cellules) ;
- jetons locaux à k = 5 : au plus 1 157 points et 69 s chacun ;
- **défaut trouvé par la critique** : la mosaïque d'ordre 2 de 08/000100 a un trou de 29 220 mm³.
  - Trois propositions dégénérées d'une même cellule, autour de cinq retours presque cosphériques, ont été refusées, et rien n'a reproposé la vraie cellule.
  - La tolérance relative de 1e-6 du certificat de volume masque ce trou.

### 3.2 Prédictions écrites d'avance

| Approche | Fichier gelé (sha256) | Bilan |
| --- | --- | --- |
| groupement (surface de base regroupée) | `PREDICTIONS_gele_20261006T155530Z.md` (`b2dd64b7…`) | 18 prédictions : 10 confirmées, 5 partielles, 3 réfutées. La règle de décision écrite d'avance **rejette** REL20. |
| mosaïque complète | `PREDICTIONS.md` (`5354a516…`, 15 h 57) | P1, P2, P5, P7 et P8 confirmées. P3 confirmée, sauf la fermeture des sommets (11 à 18 % au lieu de 20 à 70 %). P4 réfutée sur le rapport faces / boules (87 à 128 au lieu de 5 à 30). P6 confirmée à k = 5, réfutée en partie à k = 1 et 2. L'affirmation « complète » est réfutée par la critique. |
| surface d'acquisition | `PREDICTIONS.md` (`c2ec4182…`, 15 h 53) | 15 prédictions : 7 confirmées (P3 à P7, P12, P14), 8 réfutées (P1, P2, P8 à P11, P13, P15), jugées par `verifier.py`. |

Le groupement a aussi ajouté deux variantes après coup, mais avec des prédictions écrites avant leur mesure : REL20L (4 prédictions vérifiées sur 6) et la vue à trois niveaux (V1 et V2 vérifiées, V3 réfutée).

Deux réfutations sont instructives :

- **P2 de la surface** (0,20 face par point sur KITTI, au lieu de 0,8 à 1,4) : le σ global, estimé sur le sol, ne convient pas à KITTI ;
- **P8 du groupement** (13 roues sur 32 emboîtées avec REL20) : c'est l'absorption par le seuil relatif.

### 3.3 Points de la critique retenus

Retenus tels quels :

1. **ABS n'est pas une nouvelle sélection.** C'est la tête plate (critère A, mcs 20, EOM N-aire), avec une stabilité en log r. Son PQ est inférieur à celui de la tête plate à K = 5 sur les trames, et à celui de HDBSCAN sur le synthétique (0,369 contre 0,424).
2. **Les gains géométriques du groupement sont ceux de la surface de base.** Or celle-ci couvre l'intérieur de 25 à 28 ouvertures propres sur 44.
3. **Mesures biaisées.**
   - « Couverture » et « retours à ≤ δ » mesurent l'appartenance au bloc dès que les retours sont des 0-cellules.
   - Les ouvertures étaient diluées par celles des autres objets.
   - L'aire sans appui exclut le void.
   - Les traversées n'utilisent que les rayons du jeu.

   Les tableaux du § 3.1 ne gardent que les grandeurs qui restent comparables.
4. **« Roue ⊂ vélo » est un résultat oracle, en partie trivial.** L'emboîtement compte « descendant ou égal ». Le recompte donne 24 roues sur 33, dont 2 triviales.
5. **Mosaïque.**
   - Elle a un trou sur 08/000100 à k = 2.
   - Sa « bijection » n'est qu'une égalité de comptes sur le 1-squelette.
   - L'oracle et le constructeur partagent `facettes_exactes` et la récursion des niveaux.
   - Seules 33 des 54 découpes sont couvertes.
6. **Surface.**
   - La méthode déclarée échoue sur KITTI.
   - Les résultats LiDAR favorables viennent de variantes choisies après coup.
   - La traversée nulle de la variante taillée est vraie par construction.
   - Les rappels de parties sont ceux de H^r en sélection oracle.
7. **Manques.**
   - Aucune mesure de la géométrie de jetons choisis sans vérité.
   - Aucun témoin HDBSCAN qui regroupe la même surface.
   - Aucun des témoins négatifs T1 à T5.
   - Pas de masse du § 9.1.
   - Pas de transport entre K.
   - Pas de mesure aux tailles d'intérêt.

Nuancé :

- **« La masse est un comptage déguisé. »** La mesure est juste. Mais la masse du § 9.1 est, par construction, une partition de l'unité sur les points, donc un comptage doux (§ 2.1). La mesure établit que le substitut ψ ≡ 1 n'apporte rien de mesurable ; elle ne montre pas que l'idée de masse est vide. L'effet de la vraie masse (ψ(ρ) = ρ⁻³ sur les cofaces de Gabriel) reste à mesurer.

### 3.4 Les images à regarder

Toutes sous `build/v11-persist/polyedres_reconnaissables/` :

1. `approche_surface/png/synth_velo_10m_o0_k5.png` : le même vélo en six réalisations. On y voit les supports, le complexe alpha d'ordre 1, la surface de base, la surface d'acquisition en deux variantes et le regroupement HGP.
2. `approche_mosaique/png/planche_synth_velo_10m_k5_objet0_creation.png` : pour le même nœud, les supports, l'amorce (cellules des boules publiées), la mosaïque complète Del_5 et la surface observée.
3. `approche_groupement/images/etats_dates_velo_05m_k5.png` : les états datés de la branche du vélo, de r = 30 à 152 mm. On voit d'abord des morceaux de pneus, puis deux roues, puis le vélo ; la réserve est en gris.
4. `approche_surface/png/synth_velo_05m_parties_k5.png` : le meilleur bloc de chaque partie (roues 0,85 et 0,83, cadre 0,29) et celui de l'objet.
5. `approche_mosaique/png/progression_synth_velo_occulte_10m_objet0.png` : la composante de Del_k à k = 1, 2 et 5. Le bloc grossit, les roues se remplissent, et une pièce enjambe l'occultation.
6. `approche_surface/png/synth_velo_occulte_10m_o0_k5.png` : la surface d'acquisition respecte l'occultation, alors que les supports et le complexe alpha d'ordre 1 l'enjambent.
7. `approche_surface/png/synth_deux_nappes_10m_ponts.png` : 36, puis 32, puis 0 face pontante.
8. `approche_groupement/images/echec_velo_mur_10m_k5_REL20.png` : le seuil relatif au parent absorbe le vélo dans le mur (IoU 0,049).
9. `approche_surface/png/decoupe_08_002852_deux_velos_6_51_sans_sol_o0_k5.png` : un vélo réel de 148 retours, en six réalisations. À cette densité, aucune n'est franchement lisible.
10. `approche_surface/png/trame_08_000000_scene_1_4.png` : une trame réelle. La surface d'acquisition y est colorée par les clusters de la tête plate EOM (K = 5, mcs 20), donc **sans vérité** ; la réserve est en gris.

## 4. Limites et risques

- **Sélection oracle.** Toutes les mesures de forme portent sur le meilleur nœud choisi par la vérité. La sélection automatique n'est mesurée que par le PQ : 19 objets de trames, 54 de découpes, 32 synthétiques.
- **Synthétique optimiste.** Bruit de 3 mm, aucun biais entre lasers, aucun retour mixte.
- **Parties.**
  - Il n'y a de vérité de parties qu'en synthétique.
  - Guidon, jante, cadre et torse ne sont presque jamais des nœuds.
  - À 10 m et pour K ≥ 5, H^r mêle roue et cadre.
  - Une partie de moins de K retours ne peut pas être un nœud d'ordre K.
- **Séquence 08.**
  - Plusieurs variantes ont été choisies sur des trames et des découpes de la séquence 08 : β seul plus taille, ABS plutôt que REL20, vue à trois niveaux.
  - Or `Zoltan/FoundationModel/MESURE.md:45-53` réserve 08 au bilan : les variantes doivent être choisies sur des blocs des séquences d'entraînement, puis figées.
  - Les chiffres sur 08 donnés ici sont donc des **mesures exploratoires**, pas un bilan.
- **Échelle et coût.**
  - Aucune mesure aux tailles n = 8 000, 16 000 et 32 000, et seulement 3 trames (35 551 à 45 845 sites).
  - Les temps ont été pris en Python, sur un codespace partagé. La surface coûte 1,0 à 1,3 s par trame brute, plus 0,12 à 0,20 s d'attribution : on est loin des 100 ms visés pour la tour sur G4.
  - Ce coût s'ajoute à celui de FULL.
- **Masse.** Le § 9.1 n'est pas calculé. La sortie v2 n'a ni I_b, ni U_b, ni boules internes (`SORTIES.md:300`).
- **Doctrine.** `CLAUDE.md` demande un « seuil RELATIF » et l'attribue au § 9.1. Or la thèse prescrit un seuil **absolu** (p. 97), et le seuil relatif au parent échoue structurellement. Changer ce texte relève de l'utilisateur (§ 5.6).
- **Mosaïque.**
  - Sa complétude à l'échelle est réfutée sur une trame.
  - Hors position générale (grille de 1 mm), ni Del_k ≃ Cover_k ni la bijection entre composantes et nœuds ne sont prouvées.
  - Les cellules sont proposées par qhull, en flottant.
  - La porter dans le moteur rouvrirait une piste fermée sans théorème.
- **Exactitude.** Le test β, la stabilité et l'EOM sont calculés en flottant, dans un ordre canonique : ils sont reproductibles, mais non certifiés. Aucun statut exact n'est revendiqué.
- **Inconnu.** Les rayons sans retour ne sont pas exportés. « Non observé » et « vide » ne sont séparés que par les traversées comptées.
- **Raccourci géométrique** (doctrine, fait 6). Des jetons qui portent leur géométrie locale peuvent nourrir le raccourci de Sonata. C'est à mesurer par une sonde linéaire, comme le prévoit `MESURE.md`.

## 5. Suite concrète

### 5.1 Dans la v11 (natif, sans mosaïque ni catalogue global)

1. **N1 — Export de la masse du § 9.1, par flux d'incidences** (`Zoltan/FoundationModel/CONTRAT_COUPES_ET_MASSES_20260926.md:52,62-66`).
   - Pour chaque boule de W_K, on énumère localement les cofaces de Gabriel I_b ∪ A′, avec A′ ⊆ U_b, |A′| = t + 1, et A′ contenant un support.
   - On accumule T_x = Σ_{τ∋x} S_τ et la masse des nœuds aux coupes.
   - Au-delà du plafond de coquille, le calcul refuse explicitement. Les comptes sont publiés, selon la séquence count → preflight → fill → validate → publish.
   - Le coût est en Σ_b C(m_b, t_b + 1), jamais en C(n, K).
2. **N2 — Critère « masse » dans `--sortie=plat`**, à côté du critère A.
   - Seuil absolu, EOM N-aire inchangée, z en paramètre.
   - La vue à trois niveaux fait partie de la sortie.
3. **N3 (optionnel) — Mode « atomes ».**
   - Entrée : une liste de faces, données par des indices de sites.
   - Sortie : (w_τ, β_τ) et le propriétaire aux coupes demandées.
   - Ce ne sont que des requêtes LCA sur la forêt des blocs. Le but est de compter cette étape dans le budget mesuré sur G4.
4. **Hors du moteur** :
   - la surface d'acquisition, qui demande la trame brute avec son sol, l'ordre de balayage et l'origine du capteur, que le moteur ne connaît pas ;
   - la mosaïque, à cause de l'invariant.

### 5.2 Dans l'outillage Zoltan (en aval)

1. **Corriger les mesures du harnais** : distances aux seules faces, ouvertures propres, aire sans appui comptant le void, traversées avec tous les rayons de la trame brute, précision relative à l'espacement.
2. **Expérience combinée, jamais faite.** Croiser la surface d'acquisition (β plus taille, et plan plus taille), la condensation N-aire à seuil absolu et l'EOM, en sélection **automatique**. L'accompagner :
   - du témoin HDBSCAN, qui regroupe la même surface ;
   - des témoins négatifs T1 à T5 de `MESURE.md` : tour brouillée, niveaux permutés, densité seule ;
   - de prédictions gelées, avec des variantes choisies sur des blocs des séquences 00 à 07, 09 et 10.
3. **Surface d'acquisition sur KITTI.**
   - Un modèle de bruit par laser et par réflectivité.
   - L'élévation par laser, avec la parallaxe.
   - Une borne sur l'étendue 3D des faces.
   - Des prédicats entiers pour β.
4. **Mosaïque**, en diagnostic seulement.
   - Un certificat de volume exact : en rationnels, sans tolérance.
   - Tout refus « famille » compté comme un échec, avec une reproposition exacte locale.
   - Une fixture du trou de 08/000100. Sa reproduction minimale reste à trouver : des sous-nuages de 12 à 50 points ne le reproduisent pas (`critique/resultats/c7_fixture_trou.json`).

### 5.3 Portes

Les portes rendent un code exact : 0 conforme, 1 désaccord, 2 refus, 3 invariant violé, 4 mutant tué. Chacune a ses planchers `--min-*`.

- **Masse.**
  - Partition de l'unité vérifiée en rationnels exacts.
  - Masse d'un nœud égale à la somme des masses de ses faces.
  - Mutants : ψ ≡ 1, masse prise à la naissance, multifusions binarisées.
- **Atomes.**
  - β_η ≤ β_τ pour toute face η de τ.
  - Sous-complexe à chaque coupe, emboîtement à K fixé, état haut égal au propriétaire à la coupe.
  - Mutants : LCA remplacée par le propriétaire du premier site, β sans le maximum avec la naissance de w_τ, coupe ouverte.
- **Équivariance.** Atomes et états identiques sous permutation des points et réétiquetage des PointId ; résultats bit-identiques quel que soit le nombre de fils.
- **Fixtures gravées.**
  - Vélo contre le mur : le seuil relatif au parent perd le vélo, le seuil absolu le garde.
  - Deux nappes à 6 cm.
  - Anneau percé.
  - Vélo occulté : aucune face sur la bande cachée.
  - Plancher de faces par point sur une trame, contre la sur-découpe.

### 5.4 Mesures

- Coût de N1 à N3 aux tailles n = 8 000, 16 000 et 32 000, puis sur les trames de 30 000 à 60 000 sites.
- Temps sur G4, en session gardée (hors de ce travail : GCP non utilisé).
- Surface : scènes synthétiques à balayage de 8 000, 16 000 et 32 000 points (rue étendue), plus 20 à 30 trames des séquences d'entraînement.
- Stabilité au sous-échantillonnage (`MESURE.md`, 0.3), à la portée et au capteur.

### 5.5 Vidéo de la hiérarchie de polyèdres

- **Données par scène** : les atomes (faces, arêtes 1D et sommets, avec leurs indices de sites), les (w_τ, β_τ), la forêt des blocs datés (MHGP11PT) et les jetons EOM, pour K = 2, 5 et 10.
- **Lecteur** : une page `Zoltan/demos/player/polyedres.html`, sur le modèle de `supports.html` et `duel.html`, rendue par `tools/render_video.cjs`, en thème sombre et en thème clair.
- **Animation** :
  - la coupe r balaie les rayons croissants ;
  - chaque atome prend la couleur de son bloc propriétaire à la coupe ; la réserve est en gris ; les jetons EOM sont soulignés ;
  - la vidéo marque une pause aux fusions roue → vélo → groupe et au maximum d'IoU ;
  - trois panneaux côte à côte, K = 2, 5 et 10, montrent l'axe des ordres.
- **Vidéo jumelle** : la même surface, regroupée par la hiérarchie de HDBSCAN.
- **Scènes** : vélo à 5 m, piéton tenant un vélo, rue, vélo occulté ; découpes réelles 08/002852, 00/002140 et 06/000800.

### 5.6 Décisions qui reviennent à l'utilisateur

1. Confirmer la répartition des rôles du § 2.0 : les faces observées portent la géométrie ; la tour fournit regroupement, dates et masses ; la mosaïque est un canal optionnel.
2. Seuil : faut-il prendre le seuil absolu de la thèse par défaut, et amender la mention « seuil RELATIF » de `CLAUDE.md` ? Tout critère relatif futur devrait passer la fixture du vélo contre le mur.
3. Masse : accepter le comptage doux (critère A) comme substitut nommé jusqu'à N1, et fixer la priorité de N1.
4. Mosaïque : la garder en diagnostic seulement, ou investir dans un constructeur local certifié (A-local) pour les jetons choisis ?
5. Choix des variantes : le faire sur des blocs des séquences d'entraînement, et réserver 08 au bilan.

## 6. Fichiers

- **Lectures** : `lecture_doctrine/`, `lecture_maths/` (oracle `oracle_mosaique.py`, comptes `comptes_v2_ng00.json`), `lecture_art/` (mesures et bibliographie).
- **Harnais** :
  - `harnais/README.md` ;
  - `harnais/harnais.py` (sha256 `3718699376a855bf…`) ;
  - `harnais/references/synthese.json` (`f8ff33d5146cdf89…`) ;
  - `harnais/references/synthese_audit.json` (`b5d59ebe06db7202…`).
- **Approche groupement** :
  - `approche_groupement/recu.json` (`f85b99231de1a5ae…`) ;
  - `resultats/synthese_combinatoire.json` (`c87636f1d7fd2e5f…`) ;
  - `resultats/synthese_geometrie.json` (`4fc4f9105bf9ddde…`).
- **Approche mosaïque** :
  - `approche_mosaique/resultats/synthese.json` (`0e256ee1de04522f…`) ;
  - `resultats/mesures_finales.json` (`36b0c2de8980dbf0…`).
- **Approche surface** :
  - `approche_surface/resultats/synthese.json` (`b62f411f7df312fc…`) ;
  - `resultats/verdicts.json` (`e0b4a017d910db6a…`).
- **Critique** : scripts `critique/scripts/c1_emboitement.py` à `critique/scripts/c8_surface_kitti.py`, et `critique/resultats/` :
  - c2 `e8193e2a0d0a4ce3…` ;
  - c3 `4b4f4b855d22189f…` ;
  - c4 `ffe175204cdf0e3f…` ;
  - c6 `40743d8697058e13…` ;
  - c7 `1c2db8233d1952c3…` ;
  - c8 `4d3e7d48e459effd…`.
- **Réponse à l'audit** : `REPONSE_AUDIT.md`, à relire avant de la déposer dans `morsehgp3D_v11/audits/`.
