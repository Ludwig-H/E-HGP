# Arithmétique des fuseaux et comptes de témoins

Cette preuve complète la [géométrie du front](FRONT_ET_TEMOINS_COURANT.md) et fournit les bornes citées par l'[optimisation de pile constructeur](../docs/OPTIMISATION_PILE_TEMOINS.md#4-conservation-du-parcours-des-masques-et-des-statistiques). Domaine : positions u16, boîtes valides, ancrages disjoints, n<2^30 et [ABI CPU déclarée](DOMAINE_CPU_COURANT.md). Les [sources et bornes](receipts_front_20260905/spindle_bounds.json) restent épinglées ; `public_status=not_claimed`.

## H, Xi et boîtes

Poser M=65535. Différences ≤M, produits ≤M² et trois sommes partielles ≤3M² rendent `h_point=(z−a)·(b−z)` exact en i64. La forme alternative `dot(d,w)−norm2(w)` est formée sous 6M² avant de retrouver la même borne finale. Chaque composante de croix est ≤2M² ; les carrés promus donnent Ξ≤12M⁴<2^68. H² puis 3H²≤27M⁴<2^69 tiennent en i128.

Par axe, `z*(a+b)−a*b−z*z` est formé sous 2M² et vaut exactement (z−a)(b−z). Ses minima et leur somme tiennent en i64. Dans `hmax4_boxes`, s=a+b et y=clamp(s,2lo,2hi) sont dans [0,2M] ; la somme des `(b−a)²−(y−s)²` est dans [−12M²,3M²], sous 2^36. Les sentinelles ne sont jamais additionnées : chaque boîte, même plate, possède au moins un coin évalué. Le sens minimax est celui de la preuve géométrique, pas une exclusion de tous les témoins de chaque ancre.

## Boule de cœur : échelles et arrondis

Les échelles sont D=2^30 et E=2^20 ; aq∈{D,619000000,555000000}, cq∈{2E,ceil(4E/3),1329545}. Le reçu contrôle ces constantes avant le calcul des bornes. `center4` représente exactement quatre fois le milieu des centres de boîtes, avec coordonnées ≤4M.

| Intermédiaire écrit | Borne suffisante |
| --- | --- |
| Distance carrée quadruplée des centres `d2q` | ≤12M²=51 538 034 700 |
| Carré de diagonale de chaque boîte | ≤3M²=12 884 508 675 |
| Racine plancher de distance `d2u` | ≤227 019 |
| Racines plafond des diagonales `ra2u/rb2u` | ≤113 510 chacune |
| Somme des racines, `gap` | ≤227 020 ; gap dans [−227020,227019] |
| `aq*gap` quand gap>0, promu i128 | ≤243 759 795 142 656 ; quotient/D≤227019 |
| `s2=ra2u²+rb2u²` en i64 | ≤25 769 040 200 |
| `2*cq*s2+E−1` en i64 | ≤108 083 188 388 069 375<2^57 |
| Quotient plafond `sub2` | ≤103 076 160 800 |
| Racine plafond de `sub2` | ≤321 055 |
| Rayon couplé avant maximum | Dans [−321055,227019] |
| Rayon final non négatif | ≤227019 ; carré ≤51 537 626 361 |

Les constantes vérifient aq/D≤2κq et cq/E≥4κq²+1. Les contrôles quadratiques en D tiennent sous 2^62 ; leur test q4 au quatrième degré utilise i128 et reste sur 122 bits. Chaque multiplication et somme du tableau est bornée avant simplification.

`d2u` minore deux fois la distance de centres ; les rayons de boîtes sont majorés. Les divisions positives par D sont des planchers, et `(2*cq*s2+E−1)/E` est le plafond exact du rationnel soustrait. Les rayons calculés minorent donc quatre fois les deux rayons géométriques, de centre commun. Leur maximum est sûr. Remplacer les valeurs non positives par zéro ne crée aucun crédit : la boule ouverte de rayon zéro est vide.

Pour points et boîtes, |4x−center4|≤4M, donc `llabs` est sûr ; les carrés promus et leur somme sont ≤48M². Les comparaisons `far2<r2`, `near2>=r2`, `d2<r2` excluent exactement le contact de bord.

## 4. Racines corrigées : ce qui est prouvé, ce que l'environnement fournit

Les arguments de `floor_sqrt/ceil_sqrt` sont dans [0,103076160800], sous 2^37 : leur conversion binaire64 est exacte. Il suffit que `sqrt` propose une graine finie, non négative et convertible en i64. Tout carré d'une telle graine tient en i128. Les décréments terminent avec r²≤x ; les incréments suivants, bornés ici par 321055, terminent avec x<(r+1)². Le plafond ajoute un seulement si r²<x. Une racine finale correctement arrondie n'est pas nécessaire ; un NaN ou un cast invalide ne serait pas réparé.

La contre-fixture A=(0,0,0), B=(1,1,0), z=(0,1,0) est sur le shell q2, de distance quadruplée carrée 8. Le rayon correct vaut floor(sqrt(8))=2 ; le mutant plafond donne 3 et crédite faussement z puisque 8<9.

## Comptes, masques et capacité

La [partition d'index et de parcours](AUDIT_INDEX_20260905.md) rend Z∩A et Z∩B disjoints, de poids total ≤poids(Z). Les deux soustractions unsigned de `credit_weight` sont donc sûres. Une lane créditée sur Z perd son bit avant la descente : ses sous-arbres crédités forment une antichaîne. Chaque compteur est ≤n avant écrêtage ; les seuils actifs sont au plus 10/9/8 et les lanes nulles restent fermées.

Un appel visite au plus 2m−1 nœuds et 64m coins : les compteurs locaux tiennent sous 2^31 et 2^36. La collecte écrit au plus min(cap,m) indices uniques, avec garde avant chaque écriture ; l'appelant fournit le tampon correspondant. Ces bornes ne s'étendent pas implicitement aux cumuls de toute une génération. L'appel direct avec A=B ne respecte pas le contrat.

## Témoins de raccord conservés

Les [reçus O2/UBSan](receipts_front_compiled_20260905/spindle/summary.json) scellent le [pont C++](spindle_compiled_probe.cpp), son [pilote](spindle_compiled_probe.py), dépendances et commandes. Chaque binaire vérifie 4 116 racines par dichotomie indépendante, 5 184 classifications par identités de distances/Gram, 432 boules de cœur, 560 comptes fusionnés et 90 collectes. Les normales rendent 0 sans alerte UBSan ; les deux mutants produit `core-ball-ceil-distance` et `witness-no-lane-mask` rendent 4 avec faux intérieur et double crédit effectivement observés.

Le helper non consommé `true_spindle_count` peut dépasser h malgré son commentaire : sur {0,4,5,6,10}, ancre 0/10 et h=1, il rend 3. Cette contre-fixture reste dans les sorties ; le compteur produit possède son `min` terminal. La correction documentaire est regroupée parmi les [questions secondaires](QUESTIONS_SECONDAIRES.md). Aucun nouveau test ni modification produit dans cette condensation.
