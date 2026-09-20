# q3/q4 : une région de témoins partagée par arête

20 septembre 2026, tranche24 après785d0589. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`.

## Ce qui change, simplement

La tranche précédente recevait un triangle et parcourait le nuage entier.
La nouvelle entrée reçoit une arête. Elle trouve ses triangles admissibles
par groupes de points, puis leur fait partager une même région de témoins.
Les points trop éloignés sont retirés une seule fois pour cette arête.
Le comptage q3 et la préparation des événements q4 lisent ensemble les
points conservés : un seul parcours initial, pas deux. Ni l'index global ni
les coordonnées ni une liste supplémentaire de tous les points du cover ne
sont recopiés par triangle. Les IDs d'événements et de coquilles sont bien
réécrits par face ; les tris relisent les coordonnées.

Ce n'est **pas** encore le générateur global WSPD q3/q4. Aucune liste de
toutes les arêtes ou de tous les triangles n'est matérialisée. L'entrée
travaille sur une arête fournie ; les tests exhaustifs de petites tailles
énumèrent les arêtes uniquement pour juger sa complétude.

## Objets et propriété

- `Q34EdgeCover` possède un pointeur vers l'index global immuable. Il
  conserve seulement des plages de **rangs spatiaux**, disjointes et
  fusionnées lorsqu'elles sont adjacentes. Ses extrémités restent des IDs
  originaux. Il est non copiable/non déplaçable ; partager son pointeur.
- `run_q34_cover_seed_candidates` reçoit ce cover et un sommet de face.
  Ses buffers d'événements et coquilles sont privés à l'appel. Les deux
  parties de coquille émises sont disjointes et triées par IDs originaux.
- `run_q34_edge_candidates` descend les boîtes de l'index, émet les faces
  admissibles au fur et à mesure et appelle le raccord couvert. Aucun
  tableau de faces n'est créé. Le même cover sert à toutes ses faces.

Le cover et l'index peuvent être partagés entre appels concurrents ; cela
ne signifie pas qu'une équipe de workers ou une file GPU soit implémentée.
Les callbacks sont synchrones, leurs vues empruntées ; une exception laisse
les sorties antérieures au client. Chaque appel possède ses parents pendant
toute son exécution, même si le client abandonne ses propres pointeurs.

## Pourquoi le compte publié est exact

Pour l'arête propriétaire ab de longueur D, on conserve exactement les
sites satisfaisant l'inégalité fermée $|2z-a-b|^2\leq4D^2$.
Une boîte est admise entière si son maximum satisfait cette inégalité,
rejetée si son minimum la viole strictement, sinon subdivisée. Les liens
de sortie de l'index permettent un parcours sans pile. Les tests entiers
sont bornés par $12\cdot65535^2<2^{36}$.

La preuve géométrique S1§2 de l'audit v7 est reprise **comme raisonnement**,
pas comme qualification du nouveau code. Pour une boule de support
strictement positif à q sommets, les poids barycentriques du centre donnent
$R^2\leq\frac{q-1}{2q}D^2$. Avec m milieu de ab, on a
$|c-m|^2=R^2-D^2/4$. Toute la boule fermée vérifie alors
$4|z-m|^2\leq3D^2$ en q3 et
$4|z-m|^2\leq(2+\sqrt{3})D^2<4D^2$ en q4.
Le cover commun contient donc **tous les intérieurs et toute la coquille**.

Attention : cette propriété ne vaut pas pour toutes les sphères du balayage
d'une face. Avant certification de positivité et de propriété, le compte
couvert n'est qu'un minorant. Un minorant déjà au seuil autorise un rejet ;
un compte faible n'autorise pas une émission. Après les deux certifications,
la boule entière est couverte, le compte et la coquille deviennent globaux.
`run_q4_family` conserve son ancien contrat de compte global à toute racine ;
il n'a pas été modifié silencieusement.

Le balayage retire les sorties d'un groupe avant lecture et ajoute ses
entrées après. Il ne s'arrête pas à la première profondeur excessive.
Le refus q3 ne supprime jamais la famille q4. Une coquille q3 peut contenir
des sites hors du plan de la face : elle ne doit pas être confondue avec
la coquille constante q4, qui exige simultanément puissance et côté nuls.

## Accès à toutes les faces utiles d'une arête fournie

Une face propriétaire doit appartenir à la lentille fermée
$|x-a|^2\leq D^2$, $|x-b|^2\leq D^2$ et être strictement extérieure
à la boule diamètre : $|x-a|^2+|x-b|^2>D^2$.
Les boîtes sont écartées si un minimum de distance dépasse D² ou si le
maximum de la somme reste au plus D². Les égalités de longueur ne sont
pas éliminées : le départage par IDs se fait au site. Les feuilles
vérifient les trois angles stricts et la propriété exacte.

Tout tétraèdre strictement positif a au moins une face aiguë incidente
à son arête maximale : sinon ses deux autres sommets seraient dans la
boule diamètre ; celle-ci contiendrait tout le support avec un rayon au
plus D/2, incompatible avec un centre strictement intérieur au tétraèdre.
La plus petite face aiguë par ID conserve le représentant canonique.
L'acceptation q2 de l'arête et q3 de la face n'est jamais requise.

## Coût : gain local, obstacle global encore présent

Avec V boîtes visitées, m sites couverts et S faces propriétaires, le coût
de préparation du cover est O(V), au plus O(n), et son stockage O(plages).
La génération de faces visite au plus les 2n−1 nœuds de l'index. Le raccord
coûte O(m log(1+m)) par face et O(m) de buffers temporaires. Cette borne
comprend aussi le tri des coquilles par IDs, pas seulement les événements.
La relecture a corrigé une première borne qui oubliait ces tris.

**Le terme S·m subsiste.** Si S et m sont tous deux proportionnels à n,
le scan partagé seulement au niveau du stockage reste quadratique dans
son travail. Cette brique ne doit donc pas être branchée aveuglément sur
toutes les arêtes WSPD comme si P0 était clos. Ce n'est ni une preuve
sous-quadratique générale ni un chemin industriel déjà qualifié.

Les compteurs séparent préparation du cover, génération des faces,
lectures couvertes, événements, tris de coquilles, cascade et sorties.
Les capacités agrégées et la taille maximale d'un groupe sont des maxima,
pas des sommes ; les compteurs de travail sont des sommes. Le pic de
buffers par face comprend le chevauchement réel des trois vecteurs pendant
le scan fusionné, hors index, nuage, métadonnées et allocations du callback.
Les `rejected_sites` de la génération concernent les boîtes internes ;
avec les feuilles visitées, ils partitionnent les n sites.

## Qualification et suites

Les [mesures et portes](../receipts/q34_cover_20260920/README.md) propres à
cette tranche sont closes :85 CTests Release, trois portes/six sondes
Clang ASan/UBSan,186 arêtes et184 seeds jugées, trois mutants compilés,
32 mesures et lectures normal/−O. Les deux nouveaux builds q34_cover du
20260920 sont épinglés, sans modifier les captures antérieures.
Trois régimes sont requis : fond lointain (m=6, S=2), fond dans le cover
mais hors lentille (m=n, S=2), puis petit régime adverse (m=n, S=n−2).
Le dernier expose le carré sans lancer une campagne8k inutilement coûteuse.
Les préparations du nuage, de l'index et du cover doivent être réellement
chronométrées ; les comparer hors de ces coûts ne suffit pas.

Suite prioritaire : rejets collectifs avant ouverture des familles,
réutilisation de groupes de témoins entre faces, puis accès par rectangles
WSPD en conservant les masques q3/q4 indépendants. Pour la parallélisation,
les tâches posséderont le parent cover et des plages de faces, avec petits
buffers privés ; pas une copie du cover par tâche, pas une pile générale
par petite face. Le choix s8/10/12 n'intervient pas dans cette brique à
arête fournie et devra être mesuré au raccord WSPD réel.
Catalogue canonique, q minimal, collecte des intérieurs après regroupement,
reconstruction FULL, GPU/G4 et contrats restent ouverts. GCP non utilisé.

### Proposition mathématique pour la tranche suivante, non implémentée

Une contrelecture propose un certificat universel d'une famille avant de
construire ses événements. Ici D désigne **la distance au carré** ab²,
E=ax², X=bx², F=(b−a)·(x−a), G=DE−F² ; P et B sont les formes exactes
de `Q4FamilySeed`. Pour une face aiguë propriétaire,
$R_0^2=DEX/(4G)\leq D/3$, donc
$J=D(3G-2EX)\geq DG/3>0$.
Tout q4 positif de cette arête satisfait $R^2\leq3D/8$, ce qui impose
$2\mu^2\leq J$ dans la paramétrisation actuelle.

Choisir l'entier $U=\lceil\sqrt{\lceil J/2\rceil}\rceil$.
Si $P(z)+U|B(z)|<0$, le site z est strictement intérieur à **toutes**
les boules q4 positives de cette famille. K−2 IDs distincts ainsi
certifiés autorisent donc son rejet complet, sans fabriquer ses événements.
Le test q3 reste distinct, P(z)<0 avec seuil K−1. Le rejet q4 n'autorise
pas à supprimer q3 : les deux voies doivent être certifiées pour éviter
entièrement le scan de la face ; sinon conserver la voie restante.

Pour M=65535 : J≤81M⁶<2¹⁰³, |P|≤135M⁶, |B|≤6M³, U≤7M³ ; ainsi
|P|+U|B|≤177M⁶<2¹⁰⁴. Une implémentation pourrait donc rester en i128,
avec promotion avant chaque produit et racine carrée **entière**.
Calcul envisagé : q=J/2+J%2, r=floor_isqrt(q), U=r+(r*r<q).
Ne pas calculer P² en i128 ; ne pas remplacer U par sa très large borne
arithmétique, qui rendrait le certificat géométriquement inutile.

Le pool serait préparé **une seule fois par arête**. Un petit ensemble
de propositions de taille O(K) donne O(S·K) tests supplémentaires, sans
scan complet par face pour chercher ces propositions ; un échec revient
au traitement exact, il ne tronque pas la recherche. L'exploration
mathématique du petit adversaire suggère un effet, mais aussi des échecs
même avec tous les témoins. Ces calculs exploratoires ne figurent pas
dans les151 sources qualifiées : ils ne constituent ni un résultat du
moteur ni une garantie sous-quadratique. Avant port, reproduire en oracle,
tester les cas sans témoin universel et mesurer aussi le résidu aval.
