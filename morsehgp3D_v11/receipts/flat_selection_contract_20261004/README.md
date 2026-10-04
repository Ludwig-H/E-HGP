# Contrat de sortie plate — relecture du 4 octobre 2026

Le code publié ab1a739d1 mesure la présence d'objets dans la hiérarchie : `Evaluator.close` garde séparément le
meilleur IoU de chaque objet (`bench/points_hierarchy.py:543–554`). La campagne ne réalise aucune sélection
(`points_campaign.py:12`, `docs/HIERARCHIE_POINTS.md:303–306`). Ce n'est pas un défaut de ce banc, qui déclare
sa portée ; cela ne qualifie aucune partition plate. Les modules natifs `points` et `head` sont absents de cette
publication.

Une recherche privée existe sous `build/v11-points-select/` (ce dossier n'est pas un worktree Git). La copie
BEFORE possède une condensation N-aire et une sélection globale EOM/feuilles, avec bruit et politique de racine.
Ces éléments sont utiles à reprendre explicitement ; ils ne sont pas une qualification native. Les sources
privées copiées sont identifiées dans les ledgers BEFORE/AFTER et aucun de leurs scripts de fit n'est exécuté.

## Deux témoins bornés

`check_contracts.py` extrait seulement les définitions nécessaires par AST, sans importer les modules produit ou
privés. Normal et −O produisent la même sortie ; aucune garde ne repose sur assert.

1. Six points, vérité A de taille4 et B de taille2 : B forme un bloc pur, puis les quatre A entrent dans son
   parent de taille6. Avec mcs2, les deux candidats sont B et le parent. Le vrai Evaluator retient pour A le parent
   (IoU2/3), pour B l'enfant (IoU1), soit5/6 de moyenne optimiste. Leur sélection commune est impossible. Toute
   antichaîne admissible atteint au plus1/2, même avec l'évaluation généreuse par meilleur bloc de la partition.
   Soustraire ensuite B du parent créerait un nouveau bloc A, absent de la hiérarchie ; ce serait une autre règle.

2. Dans la tête privée `equite/nary_head.py` (SHA7aa47f…), un parent de quatre points né au rayon3 et mort au
   rayon2 a stabilité2/3 pour λ=1/r. Ses deux enfants de deux points meurent au rayon
   `a=3/2−2^-70` : leur somme vaut `4/a−2>2/3`. Le vrai code flottant conserve pourtant le parent, car a devient1.5
   en binary64 et les arrondis de score inversent la comparaison. L'arbre8points est une ultramétrique abstraite
   valide ; le rayon au carré tient dans les champs192bits, mais aucune réalisation par un Cloud entier n'est
   revendiquée. Ce témoin montre que l'ordre exact des dates ne suffit pas à rendre EOM exact.

Le prototype `modele/scripts/modele_lib.py:491–560` raffine des intervalles de scores puis, au budget, choisit
explicitement le parent avec un compteur de choix forcés (`elif force`, ligne531). Cela convient à une expérience
annoncée comme indécise ; la sélection n'est certifiée optimale que si aucune comparaison n'est forcée. Avant un
port natif, il faut un contrat propre pour le signe des sommes de réciproques algébriques, l'égalité et le refus.
Les budgets du comparateur de dates à six radicaux ne s'y transfèrent pas. Un intervalle non séparé ne certifie pas
un ex aequo. Aucun plafond numérique supplémentaire n'est inventé par ce reçu.

Pendant la lecture, deux sources privées ont changé. Leurs copies AFTER sont conservées séparément :
`modele_lib.py` relève la maturité interpolée par max avec la date d'engagement (un point ne compte jamais avant
son entrée) ; le vérificateur scikit-learn sépare désormais les arbres avec et sans ex aequo. Le code EOM forcé
ci-dessus est inchangé par ce delta. `nary_head.py`, utilisé dans le témoin flottant, est resté identique.

## Contrat minimal conseillé au développeur

- Construire l'arbre **de points** N-aire compact, en atomisant chaque plateau exact et en conservant les niveaux
  créés par les attaches entre deux niveaux FULL. Ne pas compter la population de couverture FULL comme masse de
  points engagés. Après suppression des nœuds vides/unaires, une hiérarchie sur n feuilles a au plus2n−1 nœuds.
  La matrice de paires n² des petits oracles ne doit pas devenir le chemin natif.
- Le lemme de diagonale du prototype est correct : si `e_i=u(i,i)≤u(i,j)`, tout bloc non singleton au rayon r
  est déjà engagé. Pour mcs≥2, ignorer les fantômes singleton conserve donc les blocs candidats et permet la
  condensation usuelle. Cela ne vaut pas pour mcs1 ni pour compter directement la couverture FULL.
- Condenser par masses entières des points, choisir une antichaîne globale indépendante des étiquettes vraies,
  puis produire une étiquette par ID original. Déclarer λ=1/r, variantes de score, ex aequo, exclusion ou
  admission de la racine, bruit et éventuelle complétion. La complétion doit rester un bras séparé et ne fusionner
  aucun cluster choisi. Une forêt ou des points inactifs exigent une représentation explicite ; ne leur fabriquer
  ni propriétaire actif ni fusion géométrique. Le consommateur actuel refuse n<m.
- Le DP natif peut mémoriser un score/booléen par cluster puis émettre une seule fois de haut en bas. Les appels
  répétés à `descendants` et les copies complètes des membres du prototype ne sont pas nécessaires. Budgeter
  l'arbre compact, les tableaux de score/décision, le scratch des comparaisons et les étiquettes coexistants.

Portes utiles avant une campagne : plateau simultané avec grands/petits enfants ; entrée entre deux niveaux
FULL qui fait franchir mcs ; absence de branche admissible et racine seule ; mcs>n/inactifs ; forêt ; scores EOM
exactement égaux, proches et non séparés ; permutation d'IDs ; partition sans chevauchement avec conservation
points assignés+bruit. Comparer une vraie partition contre HDBSCAN, avec mcs/politique déclarés et appariement
global des objets ; conserver bestIoU comme diagnostic de plafond, pas comme sortie sélectionnée.

## Reproduction et fermeture

```
python -B check_contracts.py > checks.json
python -B -O check_contracts.py > checks_optimized.json
```

Portée : petits états abstraits et fonctions Python figées, pas une preuve de performance ou de géométrie native.
Aucun fit, build, test natif ou GCP n'est exécuté ; aucun produit ni note active n'est modifié. Les commandes,
versions, sorties et sources privées nécessaires sont gardées. Le SHA256SUMS racine exclut seulement lui-même.
