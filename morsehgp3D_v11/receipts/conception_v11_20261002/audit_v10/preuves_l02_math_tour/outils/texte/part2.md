
## 3. L'objet : la tour FULL telle que la v10 la calcule

### 3.1 Définition effective (lue dans le code, vérifiée par le juge)

Entrée : $n$ sites distincts à coordonnées entières de 18 bits. Une entrée à positions répétées est refusée (`multiplicity_unsupported`, `src/tower/tower.cpp:1157-1158`). Le catalogue fournit toute sphère critique $b$ (centre $c$, niveau $\lambda_b$ rationnel exact, intérieur strict $I$ de cardinal $p$, coquille complète $U$ de cardinal $m$, plus petit cardinal de support $q$) telle que $p + q \leq K + 1$, dans l'ordre (niveau exact, support canonique), avec un rang dense par niveau exact distinct.

Pour chaque ordre $k \leq \min(K, n)$, la sortie `OrderForest` (`src/tower/tower.hpp:100-117`) contient :

| Champ | Contenu exact |
| --- | --- |
| nœuds | les naissances d'abord (à $k = 1$ : les $n$ sites, rang 0 ; sinon dans l'ordre des boules), puis les fusions dans l'ordre de création (rang croissant) |
| `rank` | 0 pour le niveau nul, sinon rang du catalogue + 1 ; le niveau exact est `cat.level[rank - 1]` |
| `parent`, `child_off`, `child_val` | forêt à fusions N-aires ; une seule racine (sinon refus `root_count`) |
| `birth` | boule de naissance (site à $k = 1$), `kNone` pour une fusion |
| `lower` | pour $k \geq 2$, nœud de l'ordre $k - 1$ **vivant à la coupe fermée** du niveau de création du nœud (vérifié ici pour tous les nœuds) |
| `point_node`, `point_level` | entrée `core` : $D_k(x)$ entier exact et nœud vivant à la coupe fermée $D_k(x)$ ; entrée `cover` : première boule couvrante |

Règles de construction (`tower.cpp`, étages L, H, G, T, P, V) :

1. **Cellules.** Une boule $b$ a une cellule $(b, k)$ pour $\max(1, p + q - 1) \leq k \leq \min(K, p + m)$ (l. 1229-1244). On pose $t = k - p$.
2. **Structure locale** (`local_structure`, l. 556-632). Coquille régulière ($m = q$) : naissance à $k = p + m$ ; jonction à $k = p + m - 1$, de représentants $I \cup U \setminus \lbrace u \rbrace$, $u \in U$. Coquille étendue : énumération des parties de $U$ de taille $t$ *séparables* (le centre hors de leur enveloppe convexe fermée, `center_in_closed_hull`, `src/catalogue/support.hpp:16-51`) ; aucune : naissance ; sinon morceaux = classes de la relation « $A \cup A'$ séparable », un représentant $I \cup A$ par morceau ; au moins deux morceaux : jonction ; **un seul morceau : cellule `inert`, rien n'est enregistré** (l. 631).
3. **Descente** (`resolve`, l. 799-993). Une $k$-partie $F$ : semis si $F$ est la population d'une naissance régulière de $k$ sites ; sinon plus petite boule englobante exacte, puis (i) au moins $k$ sites strictement intérieurs : saut aux $k$ plus proches du centre ; (ii) sphère du catalogue dont la fenêtre contient $k$ : nœud de naissance, ou mémo de la cellule ; (iii) sinon premier représentant de la structure locale. Le niveau décroît strictement à chaque pas (garde exacte, l. 836-844).
4. **Forêt** (`kruskal`, l. 1041-1116). Les jonctions de même rang exact forment un lot ; les racines pré-lot de tous les représentants sont lues avant toute union ; chaque groupe d'au moins deux racines pré-lot devient **une** fusion dont les enfants sont exactement ces racines.
5. **Verticales** (l. 1658-1747). Naissance de $b$ à l'ordre $k$ : descente à l'ordre $k - 1$ d'une $(k-1)$-partie de $I \cup U$, puis ancêtre vivant au rang de $b$. Fusion : ancêtre, à son rang, de l'image d'un enfant ; tous les enfants doivent donner le même nœud (refus `vertical_naturality`).
6. **Ancêtres** (`Jumps`, l. 717-739 ; `ancestor`, l. 1119-1126) : pointeurs de saut de Myers, résultat égal à la remontée parent par parent.

### 3.2 Ce que la sortie ne contient pas

- **Aucune contribution datée, aucune couverture.** Le mot « contribution » n'apparaît dans `src/` que dans un commentaire sans rapport (l. 1301). Une cellule à un seul morceau ne laisse aucune trace. La forêt est donc l'arbre de fusion *abstrait* de $\pi_0(L_k)$ ; les ensembles de points des composantes (les K-polyèdres de la thèse) n'en sont pas lisibles en présence de coquilles étendues (§ 4.9). `README.md:21-23` annonce pourtant « continuations datées » dans l'objet.
- **Aucune identité canonique publiée** : ni digest, ni numérotation indépendante de l'ordre des boules, ni population des naissances dans le dump du CLI (le dump de `mhgp10_tower` ne donne que parent, niveau, image, et les attaches).
- **Aucune multiplicité** : entrée pondérée refusée.
- **Coquilles étendues au-delà d'un budget** : refus `shell_quotient_budget` si $m > 24$ ou si une cellule a plus de 20 000 parties séparables (l. 22-23, 582, 594) ; ces bornes sont des budgets d'énumération, pas des bornes mathématiques.

### 3.3 Confrontation à l'objet de la thèse (Parties I et II)

| Thèse | v10 | Écart |
| --- | --- | --- |
| Niveau = rayon $r$, boules fermées (Déf. 7, 20) | niveau = $r^{2}$ rationnel exact, coupes fermée et ouverte | changement de variable monotone, aucun écart d'objet |
| $L_K(r)$, amas continus $\pi_0(L_K(r))$ (Déf. 6, 7) | même objet, pour tous les $K \leq K_{\max}$ | aucun |
| $\Gamma_K$ : sommets = $K$-parties du complexe de Čech, adjacence « $\sigma \cup \tau$ est un simplexe » (Déf. 21) ; adjacences élémentaires suffisantes (Prop. 5) | $\Gamma_k$ à adjacences élémentaires | aucun ; `SPEC_V10.md:29-31` attribue au Th. 2 seul ce qui relève du Th. 2 **et** de la Prop. 5 |
| K-polyèdres = amas discrets = **ensembles de points**, recouvrants pour $K \geq 2$ (Déf. 8, 21, 22, Th. 2, remarque 3 de la Déf. 8) | forêt abstraite + attaches de points ; couvertures non publiées | la v10 ne produit pas les K-polyèdres ; l'entrée `core` ($C \cap X$) est ce que la remarque 1 de la Déf. 8 appelle « une erreur », l'entrée `cover` suit la Déf. 8 (divergence déclarée, `CONCEPTION_V10.md` § 2.1) |
| Position générale exigée pour les Th. 4 à 7 (Déf. 26 : aucun point hors de $\sigma$ sur le bord de sa boule minimale) | aucune hypothèse : coquilles étendues traitées par le quotient local | la v10 est plus générale |
| Réduction : K-graphe de Gabriel (Déf. 29), K-arbre couvrant minimal (Déf. 30, Th. 5), composantes non triviales seulement | catalogue de sphères critiques à fenêtre de rang + descente ; toutes les composantes, isolées comprises (FULL) | **le Th. 5 est faux en général** (§ 4.11) ; la v10 ne l'utilise pas |
| Un seul $K$ à la fois ; ordre $K \leq n - 1$ au chapitre 8 | tous les ordres, $K = n$ compris, et applications verticales $K \to K - 1$ | la « tour » (bifiltration de multicouverture restreinte à $\pi_0$) est un ajout du projet, absent de la thèse |
| Partition stricte par vote pondéré (§ 9.1) | hors de cette lentille | — |

L'écart de fond est donc double : la v10 calcule **plus** que la thèse (tous les ordres, les composantes isolées, les verticales, sans position générale) et publie **moins** (pas les ensembles de points des composantes).
