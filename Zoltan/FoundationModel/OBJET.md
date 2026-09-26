# L'objet : la hiérarchie $K$-NN comme squelette de calcul

26 septembre 2026. Ce que la tour FULL Morse HGP 3D **est**, et ce qu'une
architecture peut y lire. On pose ici que le moteur est conforme à sa
spécification et qu'il est assez rapide pour des dizaines ou des centaines de
milliers de trames, avec ou sans sol. L'audit du moteur appartient aux
auditeurs indépendants de `morsehgp3D_v9/audits/` et n'est pas le sujet.

## 1. L'objet est fixé par des théorèmes, pas par un réglage

- **Def. 20–21** — le complexe de Čech, le graphe $\Gamma_K(\mathcal{X}, r)$
  dont les sommets sont les $(K-1)$-simplexes, et le **$K$-polyèdre** comme
  composante connexe de ce graphe.
- **Théorème 2** — les $K$-polyèdres sont **exactement** les amas discrets de
  forte densité de l'estimateur $K$-NN, **niveau par niveau**. Ce n'est pas un
  regroupement heuristique : c'est l'estimation exacte d'un modèle statistique,
  celui de Hartigan.
- **Théorème 4** — tout simplexe qui change réellement la connectivité est de
  **Gabriel** : l'intérieur de sa plus petite boule englobante est vide.
- **Théorème 6** — ces simplexes sont portés par la **mosaïque de Delaunay
  d'ordre $K$**. C'est ce qui rend l'objet calculable sans énumérer
  $\binom{n}{K}$ candidats.
- **§ 9.1** — pour $K \geq 2$ l'objet naturel est un **recouvrement** des
  points, mais une **partition des $(K-1)$-simplexes**. Le retour aux points
  est un vote pondéré exact, avec $S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma))$,
  $\psi(t) = 1/t^{p}$, $T_x = \sum_{\tau \ni x} S_\tau$ et
  $w_{x\tau} = S_\tau / T_x$. La Proposition 7 garantit que l'argmax en donne
  une partition stricte.

Deux conséquences qui contraignent toute architecture :

1. **le recouvrement n'est pas un défaut à réparer.** Qu'un point appartienne à
   plusieurs $K$-polyèdres est l'information d'ordre supérieur elle-même ;
2. **la laminarité existe, mais sur les facettes.** Une hiérarchie stricte est
   donc disponible, et le passage aux points est déjà démontré. Rien n'est à
   inventer de ce côté.

## 2. Ce que l'objet Morse HGP rend calculable

Pour tout $(K,r)$, la tour encode les composantes de
$L_K(r)=\lbrace y:|B(y,r)\cap\mathcal X|\geq K\rbrace$ au sens du
manuscrit. Le graphe $\Gamma_K$ donne une représentation discrète de ces
composantes ; les minima Gabriel déterminent les événements qui changent la
connectivité. Les historiques à K fixé et les cartes verticales forment
l'objet FULL réellement consommable par le projet.

Le réseau ne lira qu'un nombre fini de coupes et de cartes. Leur condensation,
leur sélection et leur projection vers les retours sont **de nouveaux
opérateurs** : il faut définir leurs domaines, leurs coûts et leur comportement
sous décimation ou re-quantification. La justification de plusieurs K vient
des compromis sensibilité aux structures minces / résistance aux ponts de
retours décrits par Morse HGP ; leur valeur pour l'apprentissage reste une
question expérimentale.

## 3. Ce que la tour publie, champ par champ

Pour chaque ordre $K = 1 \ldots K_{\max}$ :

- une **forêt de fusion** : nœuds portant un **niveau exact** (rayon au carré,
  en fraction entière), parents en CSR — zéro parent est une naissance, un est
  une continuation, **deux ou plus une multifusion** —, successeurs et
  contributions datées ;
- les **populations** : contributions par boule, séparées en intérieur strict
  et coquille ; l'ensemble couvert par un nœud à une coupe se reconstruit ;
- la **carte verticale** : l'image du nœud d'ordre $K$ dans l'histoire d'ordre
  $K-1$, au niveau de création fermé du nœud.

En amont, le **catalogue** des boules minimales : clé primitive exacte de la
forme quadratique, niveau exact, **arité** $q_{\min} \in \lbrace 2, 3, 4 \rbrace$
— arête diamétrale, triangle aigu, tétraèdre —, intérieurs stricts et coquille.
C'est l'alphabet géométrique de la présentation du 16 septembre :
$P = \bigcup_i \mathrm{conv}(Q_i)$.

**La carte verticale est vérifiée naturelle** (invariant
`full_ball_vertical_naturality`) : l'image d'une fusion est la fusion des
images, le carré commute. La bifiltration n'est donc pas deux empilements posés
côte à côte, c'est un morphisme de filtrations vérifié à chaque construction.
Un biais d'attention le long de l'axe des ordres a un sens mathématique, pas
seulement une justification d'ingénieur.

## 4. Les six primitives qu'une architecture peut y lire

C'est la lecture utile de la section précédente.

| primitive | lue dans | ce qu'elle remplace |
| --- | --- | --- |
| une **échelle de recouvrements** emboîtés | les coupes de la forêt de fusion | taille de voxel, FPS, niveaux de superpoints |
| des **matrices d'affectation douces** | populations et poids du § 9.1 | *pooling* de cellule |
| un **graphe de fusion** pondéré par le rayon de fusion | les événements de fusion | graphe $k$-NN, fenêtre sur sérialisation |
| une **ultramétrique** sur les nœuds | le niveau de fusion, via l'équivalence dendrogramme–ultramétrique | encodage de position relative métrique |
| un **axe d'ordre** $K$ à cartes naturelles | les verticales | *rien d'équivalent* |
| des **scalaires structurels exacts** | niveaux, arités, degrés de fusion | variables et cibles à exporter |

## 5. Les ordres de grandeur, pour dimensionner

Mesurés sur des trames réelles, à titre de dimensionnement et non d'obstacle :

| entrée | $K_{\max}$ | nœuds de la tour | nœuds par site |
| --- | --- | --- | --- |
| sans sol, 35,5–45,8 k sites | 5 | 1,31–1,68 M | ≈ 33–37 |
| sans sol | 10 | 5,95–7,47 M | ≈ 149–167 |
| brut avec sol, 123–126 k sites | 5 | 3,47–3,81 M | ≈ 28–31 |
| brut avec sol | 10 | 14,97–16,27 M | ≈ 121–132 |

Ventilation pour une trame sans sol de 39 885 sites, $K \leq 5$ :

| $K$ | nœuds | naissances | fusions |
| --- | --- | --- | --- |
| 1 | 79 681 | 39 885 | 39 796 |
| 2 | 178 127 | 101 089 | 77 038 |
| 3 | 285 910 | 166 228 | 119 682 |
| 4 | 421 661 | 249 493 | 172 168 |
| 5 | 576 371 | 341 081 | 235 290 |

Ces nombres ne sont **pas** une contrainte de séquence. Un U-Net ne consomme
pas tous les nœuds : le budget illustratif de $L$ coupes est d'environ
$160\,000$ unités tous niveaux confondus sur une trame brute. La tour FULL
reste toutefois matérialisée avant ces coupes et a son propre coût. Voir
[`ARCHITECTURE.md`](ARCHITECTURE.md) § 2.

## 6. Ce que la tour ne fournit pas encore

Chacun de ces points est un poste de travail de la phase 0, pas une lacune de
conception :

- **aucune sérialisation consommable** : la sortie publiée est un JSON de
  compteurs et un condensé. Un pilote peut viser un adaptateur expérimental
  vers `CertifiedTowerInput` de
  `morsehgp3d/`, dont le réducteur exact — vote pondéré du § 9.1 par
  `SimplexPointWeighting::inverse_radius`, sélections par excès de masse,
  coupe $\lambda$ et rayon DBSCAN — **attend déjà un producteur** ;
- **aucune géométrie explicite** : les nœuds portent des identifiants de
  points, pas des coordonnées. La réalisation $P_v = \bigcup_b \mathrm{conv}(S_b)$
  se reconstruit depuis le catalogue ;
- **aucun constructeur d'échelle** : les coupes, les règles de contraction et
  les chemins dans le treillis sont à écrire ;
- **aucun attribut capteur** : la quantification ne transporte ni rémission, ni
  anneau, ni horodatage. Ils se rattachent hors moteur, par identifiant de
  point ;
- **aucune étiquette** : la tour n'en consomme ni n'en produit, par contrat.

## 7. Deux propriétés à ne pas perdre de vue en concevant

**Le déterminisme.** La tour est une fonction pure de la trame quantifiée, à
condensé reproductible. Elle se calcule une fois et se met en cache. Aucun
tokenizer appris n'a cette propriété, et c'est ce qui rend l'étude de
substitution exactement reproductible.

**L'équivariance géométrique et la quantification.** L'objet mathématique
suit isométries et homothéties avec reparamétrage des rayons. La v9 travaille
sur une grille u18 à 1 mm : une rotation ou une homothétie suivie d'une
nouvelle quantification peut modifier les événements. On recalcule donc la
tour des vues augmentées et on mesure leur dérive.
