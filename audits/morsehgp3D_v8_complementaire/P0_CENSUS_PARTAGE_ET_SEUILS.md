# Partager un census q2 entre supports et seuils

13 septembre 2026, après `f5430f57`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`.
Proposition de contrat, sans index produit ni mesure de performance.
Elle complète les [bornes et tâches de la section 9](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md)
et nos [fixtures de coquille](P0_CENSUS_Q2_ET_COQUILLE.md).

## Une clé géométrique, plusieurs demandes

Pour q2, S=a+b et D²=|a−b|² déterminent exactement centre S/2 et rayon
carré D²/4. L'égalité des boules équivaut donc à l'égalité du quadruplet
(S_x,S_y,S_z,D²). Sous u16, les composantes de S sont au plus 131070,
et D²≤3·65535² : u32 suffit pour S, mais D² nécessite plus de 32 bits,
donc par exemple u64, avec promotion avant les carrés.
Un hash doit résoudre ses collisions par cette égalité exacte.

Le census dépend aussi du **nuage immuable et de son espace d'IDs**.
Deux nuages peuvent porter les mêmes supports avec des intérieurs
supplémentaires ; un réindexage peut changer les IDs à restituer sans
changer la géométrie. Le propriétaire global du nuage doit donc borner
la portée du cache. La seule identité du rectangle empêcherait le
partage entre rectangles ; la seule clé géométrique permettrait des
réutilisations fausses entre nuages. Aucun partage implicite entre
les copies privées actuelles de `PreparedRectangle` n'est proposé.

Kmax et les crédits du préfiltre ne font pas partie de la géométrie.
Les demandes de différents supports peuvent référencer le même résultat,
mais gardent leur seuil et, si le contrat l'exige, leur incidence source.
Le census repart de zéro sur tous les IDs ; son seuil est Kmax,
pas Kmax diminué du cœur local du rectangle.

## Saturation et reprise : ne pas perdre le surplus d'un bloc

Un état seulement déclaré « au moins h intérieurs » permet de rejeter
les demandes de seuil au plus h. Il ne décide pas un seuil supérieur.
Pour reprendre un parcours, conserver un compte **exact sur les blocs
entièrement consommés**, avec leur partition d'IDs et la frontière
restante. Un nœud certifié intérieur de cardinal m contribue m à cet
état, même si le seuil courant est dépassé. Une autre solution est de
conserver la partie non consommée du nœud. Sans cette information,
un état saturé reste réutilisable comme minorant, mais la recherche
pour un seuil plus élevé doit recommencer.

Fixture : a=(0,1,1), b=(3,2,2), c=(1,0,1), d=(2,3,2),
z1=(1,1,1), z2=(2,2,2), u=(1,1,0), e=(1,1,4), dans cet ordre.
Les supports ab et cd donnent S=(3,3,3), D²=11, intérieurs {z1,z2}
et coquille {a,b,c,d,u}. La boîte [1,2]³ contenant z1,z2 est entièrement
strictement intérieure : un vrai crédit de bloc de cardinal 2 est possible.

Traiter ab à Kmax1 puis cd à Kmax3 doit rejeter le premier et conserver
le second, avec les deux IDs intérieurs. Si le premier parcours stocke
seulement count=1, supprime tout le bloc de sa frontière puis reprend,
il annoncera faussement p=1. Cela conserverait aussi la boule à Kmax2,
alors que son vrai p=2 impose le rejet. À l'inverse, réutiliser un booléen
« rejeté » sans seuil perdrait la demande Kmax3.

Deux détails de la même fixture testent la clé : z1z2 a le même centre
mais D²=3, avec zéro intérieur ; déplacer z2 en (10,10,10) dans un autre
nuage laisse la boule ab avec un seul intérieur. Ni le rayon ni l'identité
du nuage ne peuvent être omis du cache.

Pour une campagne dont tous les seuils sont connus, grouper les demandes
par boule et traiter leur **maximum** évite ces reprises. Si p atteint
ce maximum, toutes les demandes sont rejetées. Sinon p est complet ;
la coquille et les IDs intérieurs sont collectés une seule fois pour
les demandes ayant p<Kmax. Un flux recevant plus tard un seuil supérieur
a besoin du contrat de reprise ci-dessus ou d'un recalcul explicite.

## Reconstruire les diamètres sans tester toutes les paires de coquille

À boule positive fixée, le partenaire de x est uniquement S−x. Cette
application est une involution sans point fixe sur les sites de coquille
qui possèdent leur antipode dans le nuage. Les supports q2 forment donc
un **appariement**, avec au plus floor(|coquille|/2) paires distinctes.
Dans la fixture, il y en a deux : ab et cd ; u n'a pas d'antipode présent.
Les dix paires possibles de la coquille ne sont pas toutes des supports.

Un dictionnaire exact coordonnée→ID de la coquille permet un passage
linéaire en espérance, en n'émettant que ID(x)<ID(S−x). Une recherche
binaire dans la coquille triée donne O(s log s) déterministe, s étant
son cardinal. Préserver séparément les demandes sources si elles doivent
être restituées : cet appariement reconstruit les supports géométriques,
pas leur provenance dans les rectangles ou les appels antérieurs.

Une boule fixée ne peut occuper un produit A×B de plusieurs paires :
fixer a force b=S−a, et fixer b force a. Partager des certificats de
parcours sur un gros produit et partager le résultat d'une même boule
sont donc deux opérations distinctes. La canonisation seule ne supprime
pas le travail nécessaire pour découvrir les clés des paires développées.

## Travail et qualification bornés

Compter séparément demandes, supports distincts, boules distinctes,
visites de reprise, IDs intérieurs et IDs de coquille matérialisés.
Les survivants ont p<Kmax ; leur coquille peut toujours contenir O(n)
sites. Le partage évite une copie par support, mais ne borne pas la somme
des coquilles des boules distinctes ni leur résidence simultanée.
La collecte différée ou la reconstruction des supports paie son propre
parcours. Aucun gain général n'est déduit du seul nombre de clés fusionnées.

Le [modèle borné](q2_shared_census_gate.py) n'implémente pas d'index :
il utilise une partition explicite d'IDs, compare chaque demande à H
direct, et vérifie les six ordres de seuils 1/2/3. Le
[reçu](Q2_SHARED_CENSUS_CHECKS.json) porte 42 demandes par mode normal/−O,
une seule collecte de coquille par état survivant et cinq modèles faux
rejetés. Le modèle conserve les IDs consommés ; ce stockage de test
n'est pas un format mémoire proposé pour les grands états rejetés.
Pas de qualification du consommateur en chantier ni de FULL. GCP non utilisé.
