# À contre-lire : énoncés nouveaux de la conception de la tour (pour les deux auditeurs)

2 octobre 2026, déposé avec `CONCEPTION_TOUR.md`. Aucun de ces énoncés n'a été relu par un tiers ; chacun porte une décision de conception. Les preuves sont à l'annexe A du document, les contrôles exécutés dans `preuves_tour/`.

| Id | Énoncé | Ce qui en dépend | Angle d'attaque suggéré |
| --- | --- | --- | --- |
| T1 | Si $S^{*} \subseteq F \subseteq P_b$ pour une boule $b$ du catalogue, alors $B(F) = b$ | 76 à 79 % des plus petites boules de la descente ne font plus aucune arithmétique exacte | la proposition flottante peut-elle rendre un $S^{*}$ d'une autre boule avec $F \subseteq P_b$ sans que $B(F) = b$ ? (le lemme dit non) ; coquille étendue à plusieurs supports minimaux |
| T3 | Arrêt d'une descente sur la première cellule de fenêtre non naissance ; la cible finale est celle du premier représentant de cette cellule, suivie après coup | plus de mémo partagé ; compteurs déterministes | date d'usage : la cible n'est lue qu'à des coupes de niveau au moins $\beta(F)$ ; vérifier le noyau en flux (lecture d'une cible de rang strictement inférieur) |
| T4 | Noyau union-find sans lots, événements binaires, contraction des événements liés de même rang : arbre de fusion à plateaux atomiques, pour tout ordre dans un rang et toute règle d'union | forêt de chaque ordre | plateau où une composante produite au rang $t$ est reprise comme opérande perdante au même rang ; clé (rang, plus petite naissance) |
| T5 | Historique d'attache (union par taille) : profondeur au plus $\log_2$ ; `component_at` rend le nœud vivant à la coupe fermée | verticales des fusions, attaches | coupe fermée exactement au rang d'un plateau ; naissance jamais fusionnée |
| T6 | Image d'une naissance d'ordre $k$ = nœud du sommet laissé par la jonction de la même boule à l'ordre $k - 1$, remonté d'un cran si le parent a le même rang | verticales des naissances en $O(1)$ | jonction qui n'unit rien, suivie au même rang d'une jonction qui absorbe sa composante |
| T7 | Quotient local d'une coquille étendue par ensembles séparables maximaux (sommets de l'arrangement des grands cercles, fenêtres semi-ouvertes) ; morceaux = composantes du graphe « intersection d'au moins $t$ sites » | coquilles étendues en temps polynomial ; contributions de couverture | antipodes ; plusieurs plans par le centre partageant une droite ; coquille coplanaire au centre ; $m = 2$ |
| T8 | Témoins de `cover` : boules avec $p + q_{\min} \leq k \leq p + m$ ; en régulier, les naissances de l'ordre ; $E_k(x)$ sans aucune descente | entrée `cover` ensembliste | boule de niveau $A_k(x)$ contenant $x$ avec $k < p + q_{\min}$ (l'énoncé dit qu'elle n'existe pas) |
| I1 | Élagage de l'index par une borne inférieure flottante certifiée à sens unique (centre rationnel approché à erreur absolue certifiée) ; décisions par clés exactes | index sans marge fixe, valable à tout profil | l'élagage emploie une soustraction entre un entier exact et une approximation, hors F3 : quelle forme de la borne acceptez-vous ? |

Deux questions de fond pour vous :

1. La tour arrête d'office la vérification exacte de la décroissance des niveaux à chaque pas (théorème D) et ne garde qu'une garde gratuite par rangs entre boules du catalogue et un plafond de pas. Acceptez-vous que la décroissance complète ne soit jugée que par les portes (certificats de descente par échantillon, § 8.3) ?
2. L'index des sites est indépendant des listes du générateur (§ 3.6) : le contrôle croisé « recensement de l'index contre recensement du catalogue » n'a lieu que pour les coquilles étendues rencontrées hors proposition. Faut-il un juge d'échantillon plus large dans les portes du catalogue, ou celui de L08 (`mhgp11_catalogue_boxes`) suffit-il ?
