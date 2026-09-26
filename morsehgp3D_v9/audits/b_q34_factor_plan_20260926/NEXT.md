# Après le Pool : représentation compacte et tubes adaptatifs

26 septembre 2026. Propositions d'audit, **pas une implémentation ni une
qualification nouvelle**. Base du prototype : `92c709bc8` ; `plan.hpp`,
`probe.cpp` et leurs captures gelées restent inchangés. CPU de référence,
grille entière u18, `public_status=not_claimed`. GCP non utilisé ici.

## 1. Le plafond du gain : le travail évité est celui de S2

Le plan remplace une partie des recherches de témoins **avant** expansion
du rectangle. Les paires qu'il rejette auraient également été rejetées
par le filtre ponctuel exact S2. À configuration et exactitude conservées,
les survivantes S2 restent donc identiques ; le plan ne supprime pas de
graines q3/q4, de census accepté, d'événements du catalogue ou de la tour
FULL. La baisse de la masse développée n'est pas une baisse dans la même
proportion du temps FULL.

Si S2 GPU coûte environ 29 ms dans une capture donnée, le gain maximal
sur **cette étape**, même en la rendant gratuite, ne dépasse pas ces 29 ms.
Il faut en retrancher le plan, ses tris, allocations, copies et transports.
Ce repère historique n'est ni une mesure GPU de ce prototype ni une borne
sur une autre configuration. L'intérêt immédiat est aussi de réduire la
mémoire des masques par paire et la croissance sur les amas. Le contrat
FULL LiDAR 100 ms exige encore de traiter le front, les lanes et FULL.

## 2. Remplacer les cellules conjointes par des bandes disjointes

Le prototype groupe chaque facteur par le couple de crédits `(c3,c4)`.
Il parcourt uniquement les classes occupées, mais paie encore
`J = somme(C_A * C_B)` tests de cellules et conserve un descripteur par
cellule résiduelle. À K borné, ce n'est pas un carré en n ; ce peut
néanmoins être beaucoup de métadonnées par petit rectangle.

Une représentation plus compacte conserve le même tri lexicographique
de B par `(b3,b4)`. Pour une classe A de crédits `(a3,a4)`, poser les seuils
résiduels `r3 = T3-a3` et `r4 = T4-a4`, avec `T3=K-1`, `T4=K-2`.
Les voies inactives sont traitées avant ces soustractions.

- Le préfixe des lignes `b3 < r3` constitue **une bande contiguë** de B.
  Tous ses éléments survivent en q3 ; q4 dépend encore de `b4 < r4`.
- Dans chacune des autres lignes `b3 >= r3`, seul le préfixe `b4 < r4`
  est gardé : ces bandes sont q4 seulement.
- Une bande vide n'est pas émise. Les deux familles sont disjointes ; une
  paire survivant dans les deux voies n'est jamais produite deux fois.

Il y a au plus K+1 bandes par classe A, au lieu de parcourir toutes les
classes B. Deux préfixes marginaux et un préfixe 2D de l'histogramme B
donnent aussi la masse de l'union par inclusion-exclusion, en O(1) par
classe A après O(K²) préparation. Il faut compter cette préparation,
même lorsque presque aucune classe n'est occupée.

**Le masque transmis à S2 doit être exact pour les crédits connus** :
deux additions/comparaisons des crédits donnent les bits q3/q4 à
l'émission de chaque paire résiduelle. Ne pas passer aveuglément mask6
pour toute la première bande, car cela ferait repayer à S2 une voie déjà
rejetée. Il faut conserver les crédits B dans l'arène ; les bandes seules
ne permettent pas de les oublier. Pour une seule voie active, réduire
directement à son histogramme et à ses préfixes.

Coût proposé du regroupement : O(K²+C_A K) par rectangle et non
O(C_A C_B), avec `C_A <= K(K-1)`. La préparation Pool reste O(KF), où
`F = somme(|A|+|B|)` sur les rectangles réellement préparés. Le résidu E,
les parcours de S2 et tout l'aval s'ajoutent à ces coûts. Mesurer F, le
nombre de bandes, leurs masses, les cases d'histogrammes initialisées et
les opérations de décodage ; aucune borne globale sous-quadratique n'en
découle.

## 3. Une arène collective, pas un objet à vecteurs par rectangle

Pour un port GPU, compter d'abord les tailles par rectangle/facteur,
effectuer les préfixes, puis allouer des tableaux collectifs possédés :
rangs groupés, crédits, classes, bandes et petits en-têtes avec offsets.
L'index et les coordonnées du nuage restent partagés. Le plan d'un
rectangle est préparé **une fois**, jamais une fois par job ou worker.

Les crédits q3/q4 tiennent chacun sur quatre bits pour K<=10. Les rangs
peuvent être u32 lorsque le domaine est validé ; les offsets globaux
d'arène demandent leur propre validation, distincte du nombre de sites.
Employer des offsets u64 si nécessaire, sans tronquer les sorties ni
imposer un plafond caché. Un descripteur peut référencer ses classes et
ses plages plutôt que recopier tous les rangs ou toutes les boîtes.

Le pic d'un plan de la sonde séquentielle n'est pas la résidence GPU de
tous les plans. Publier séparément la somme des capacités des plans,
leurs buffers transitoires, la résidence de l'arène compacte, l'index,
les préfixes, les masques des paires encore développées et les sorties
S2. La réduction des en-têtes/allocation est une proposition de mémoire,
pas un gain chronométré.

L'ordre est un autre coût réel : le batch actuel promet l'ordre rectangle,
puis ligne/colonne dans les rangs originaux. Les classes le changent.
Conserver des ordinaux originaux et payer un rassemblement/tri stable des
survivantes, ou changer explicitement ce contrat et requalifier tous ses
consommateurs. Ne pas perdre le raccord des sidecars/handles dans cette
opération. La mesure doit inclure ce coût, pas seulement les crédits.

## 4. Tubes : ce qui est sûr et ce qui a déjà échoué

Le [certificat historique](../../../morsehgp3D_v8/audits/P0_TUBES_ET_RANGS.md)
permet une largeur entière positive arbitraire. La largeur choisit des
propositions ; elle ne décide pas la géométrie. Pour chaque cellule, il
faut recalculer exactement Q, somme des carrés des étendues transverses,
puis certifier `delta > 0` et `Q <= delta²` en q3, ou
`16*Q <= 9*delta²` en q4. L'égalité de cette dernière comparaison est
autorisée par la marge stricte du lemme ; **pas** l'égalité `delta=0`.
La condition géométrique `d² >= 100*max(diagonales²)` reste vérifiée.

Le vecteur d est celui des centres du rectangle réel. Le port doit lire
les points par les rangs de son unique index, garder les IDs originaux
pour les égalités de projection et ne jamais créer un `PreparedRectangle`
avec des plages Morton traitées comme des plages d'IDs. Les populations
sont les suffixes de A privé de son ancre et de B privé de son ancre ;
h extérieur reste zéro. Le théorème de rejet porte seulement sur les
supports positifs dont l'arête considérée est de longueur maximale.

La largeur actuelle `4*max|d_i|` correspond à quelques unités de grille,
pas à la taille du facteur. Elle change donc de sens physique entre 2 cm
et 1 mm. Ce problème n'est pas nouveau : l'[audit du 14 septembre](../../../morsehgp3D_v8/audits/CREDITS_TERMINAUX_20260914.md)
a déjà balayé les multiplicateurs 1/16/64/256/1024 sur un produit d'amas
32k. La largeur initiale produisait 7 818 cellules pour 8 060 points et
aucun crédit ; les cellules très larges perdaient de nouveau tous les
crédits q3/q4. Les meilleures largeurs testées restaient proches du Pool
pour q3/q4, très loin d'une solution générale. Ces résultats historiques
ne qualifient pas un nouveau port u18 ni LiDAR.

## 5. Proposition adaptative à tester, sans annoncer une borne acquise

Pour un volume 3D régulier, un tube de largeur physique w et longueur
comparable au diamètre L laisse approximativement deux coûts : une
couche frontale de `m*w/L` ancres et K extrémités par tube, soit
`K*(L/w)²`. Leur équilibre suggère `w/L` de l'ordre de `(K/m)^(1/3)`.
Cela donnerait environ `K^(1/3)*m^(2/3)` ancres non saturées, et non une
fraction constante de m. Ce calcul est une **heuristique de régime**,
pas une preuve pour les nuages irréguliers.

En revanche viser une occupation constante O(K) par tube laisse O(m)
extrémités non saturées : cela peut préserver un résidu quadratique.
La règle d'occupation doit croître avec m. Un premier candidat fondé sur
les étendues exactes est une largeur transverse de l'ordre de la racine
cubique de `K*T*S1*S2/m`, où T est l'étendue des projections et S1/S2
deux étendues transverses majeures. Cette proposition respecte le
changement d'échelle ; elle n'est pas une estimation certifiée d'aire ni
de dimension. Traiter explicitement les étendues nulles et les divisions,
sans ouvrir un nombre non borné de tentatives par rectangle.

**Attention au LiDAR** : une surface plane projetée perpendiculairement
à d est généralement de rang 2 ; dans chaque cellule, l'étendue axiale
peut être seulement O(w), et non L. L'argument volumique précédent ne
s'applique alors pas. Si d est exactement tangent à une surface plane,
le rang transverse vaut 1 et un modèle différent suggère une racine
carrée. La courbure et l'angle changent ce diagnostic. Mesurer population,
étendue axiale, Q, saturation et rang effectif par cellule avant de
choisir un régime ; aucun choix de largeur seul ne résout toutes ces
géométries.

Le port minimal partagerait projections, tri `(cellule,projection,ID)`
et Q entre q3/q4, puis deux balayages monotones : O(m log m) préparation,
O(m) balayages, mémoire O(m) par facteur. Globalement publier
`somme m log m`, F, les tris, cellules, balayages, regroupement J et E.
Comparer une règle fixée à l'avance, éventuellement un petit nombre
constant de largeurs payées intégralement, sur 8k/16k/32k et les coupes
LiDAR. Plusieurs grilles ou Pool+Tubes se combinent par **maximum** des
crédits par facteur/voie, jamais par somme sans preuve de disjonction.
Les témoins des ancres déjà saturées restent disponibles : ne pas
préparer les tubes sur les seules ancres résiduelles en supprimant ainsi
une partie de la population de témoins.

Décision suivante : confronter cette proposition aux résultats Pool
complets, puis tester sa sélectivité et son coût avant tout port moteur.
Si le biais des cellules persiste, la dominance conique décrite dans la
note historique reste une alternative O(m log²m), avec davantage de
mémoire et un raccord GPU à étudier ; elle n'est pas implémentée ici.
