# Polyèdres HGP : représenter la surface observée

6 octobre 2026. Proposition d'architecture en réponse à l'utilisateur ; source relue : **87f1621053635e76f21908146e53b790feda0af1**.
Cadre moteur : exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed.
Aucune implémentation de mosaïque, qualification native, reconstruction LiDAR ni expérience apprise n'est acquise par cette note.

## 1. Décision et diagnostic

**Cible confirmée par l'utilisateur : la géométrie effectivement observée par le LiDAR, avec ses trous et occlusions.** Le résultat recherché est donc un complexe de petites faces observées, généralement ouvert, organisé par HGP. Une fusion regroupe des morceaux ; elle ne justifie pas de remplir leur enveloppe convexe.

Les vidéos [supports.js](../../../Zoltan/demos/player/supports.js) dessinent les supports q2/q3/q4, y compris les quatre faces de chaque tétraèdre. C'est une superposition de témoins de fusion. Une réduction par Kruskal enlève la redondance de connexion ; elle ne reconstruit pas une surface.

Le [format SPv2](../../docs/SORTIES.md) garde X entier, S*, les comptes p/m et l'arbre, mais ni les listes I/U ni les boules retirées. I/U sont recalculables par census pour une boule conservée. Le catalogue vivant les possède déjà. Toutefois, [leaf.cpp](../../src/catalogue/leaf.cpp) ne retient que les MEB positives avec p+q_min <= K_max+1 : **le catalogue de connectivité lui-même n'est pas une mosaïque complète**. Le [lemme H](../../docs/MATHEMATIQUES.md) reconstruit des ensembles de sites FULL datés ; il ne les transforme pas en surface.

## 2. La vraie cellule d'ordre supérieur

Pour une sphère de partition exacte intérieur I, coquille U, extérieur X privé de I∪U, poser p=|I| et j=k−p. Pour 1 <= j < |U|, la cellule duale exposée par son centre s'écrit

\[
P_{I,U,k}=\operatorname{conv}\left\{
 \frac{\sum_{x\in I}x+\sum_{u\in J}u}{k}:
 J\subseteq U,\ |J|=j
\right\}.
\]

Les sommets portent les identités des k-ensembles I∪J. Il ne s'agit ni du support S*, ni de conv(U), ni de toutes les k-parties de I∪U. Aux extrémités j=0 ou |U|, on obtient un sommet à dédupliquer par son k-ensemble. La construction barycentrique et les cellules non simpliciales sont décrites par [Edelsbrunner–Osang, §2–3](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf).

Exemple vérifié ici : une coquille tétraédrique régulière sans intérieur donne un tétraèdre à k=1 et l'octaèdre des six milieux d'arêtes à k=2. Le dessin du tétraèdre original ne représente donc pas la cellule d'ordre 2.

Pour une coquille dégénérée, conserver sa cellule exacte, les incidences et les égalités ; une triangulation de rendu reste une subdivision identifiée. Des k-ensembles différents peuvent avoir le même barycentre, et certains barycentres sont intérieurs à la cellule : ils ne deviennent pas automatiquement des sommets. La subdivision reste cohérente au sens du relèvement pondéré. Les résultats publiés cités utilisent souvent la position générale : leur qualification ne se transfère pas automatiquement à la grille u21. Éviter l'énumération de toutes les combinaisons d'une grande coquille : une représentation implicite possible est la projection du polytope 0<=θ_u<=1, Σθ_u=j. Sa fonction de support additionne les j plus grands produits scalaires sur U, avec la translation ΣI, puis divise par k. L'extraction des faces reste un travail à qualifier.

## 3. Filtrer avec la bonne distance

Soit d_k(y) la distance au k-ième voisin. Pour |Q|=k, V_k(Q) est sa région de Voronoï d'ordre k. Les ensembles convexes

\[
C_Q(r)=V_k(Q)\cap\bigcap_{x\in Q}B(x,r)
\]

recouvrent exactement Ω_k(r)={y:d_k(y)<=r}. Ils donnent le lien topologique entre la mosaïque et FULL ; l'équivalence d'homotopie ne garantit pas une proximité métrique avec une surface LiDAR. Voir [Edelsbrunner–Osang, §3–4](https://research-explorer.ista.ac.at/download/9317/10394/2021_DisCompGeo_Edelsbrunner_Osang.pdf).

Une cellule σ, duale d'une face F_σ, doit recevoir le niveau

\[
a_\sigma=\min_{y\in F_\sigma}d_k(y)^2.
\]

Les faces doivent être présentes à un niveau au plus égal à celui de leurs cofaces. Une sphère témoin donne un niveau admissible ; l'égalité avec le minimum demande son certificat. Pour une MEB positive dont la coquille entière contraint la cellule, le certificat MEB fournit cette minimalité. Cela ne couvre pas toutes les cellules.

**Piège de l'implémentation pondérée :** les poids des barycentres permettent de construire les bonnes cellules de Voronoï, mais leur puissance est une moyenne de distances carrées. Elle ne donne pas le seuil k-NN. Pour Q={0,2}, y=0, la moyenne vaut 2 et d_2² vaut 4 ; le seuil 3 les distingue. Il faut le rayon contraint de la mosaïque d'ordre k, pas un alpha pondéré ordinaire substitué sans preuve.

**Complétion nécessaire.** Le triangle (0,0,0),(4,0,0),(1,1,0), avec un quatrième site (0,0,10), est une face de Delaunay. Sa sphère témoin vide a pour centre (2,−1,0) et rayon carré 5. Ce centre est hors du triangle : ce cercle n'est pas une MEB positive et manque au catalogue HGP. Même un tétraèdre aigu vide est absent de Cat1/Cat2 par la garde p+q_min<=K_max+1. Partir des boules FULL est une amorce ; fermer leurs faces ne recrée pas les cofaces manquantes.

**Construction proposée de la mosaïque.** Sur de petits nuages, un oracle peut former les barycentres des relèvements (x,||x||²) des k-ensembles, puis les faces inférieures de leur enveloppe convexe, projetées en 3D. Cela permet de vérifier cellules et incidences avant toute réduction. Pour une construction plus grande, l'algorithme d'Edelsbrunner–Osang sélectionne les barycentres pertinents par ordres successifs puis utilise une construction de Delaunay pondérée ; il évite de proposer tous les k-ensembles. Les incidences, dégénérescences et dates contraintes restent à qualifier dans notre profil exact. Une amorce tirée de FULL doit être complétée ou rester explicitement partielle.

Dans une vue diagnostique de cette mosaïque, afficher la frontière des volumes réunis et les éventuelles faces libres, après déduplication des incidences ; dessiner toutes les faces de chaque cellule recréerait l'amas des vidéos. Cette frontière reste une géométrie de multi-couverture, distincte de la surface observée.

Les mosaïques d'ordres différents ne sont pas des maillages géométriquement emboîtés. Les constructions de multi-couverture utilisent des complexes et applications intermédiaires : [Corbet–Kerber–Lesnick–Osang](https://drops.dagstuhl.de/storage/00lipics/lipics-vol189-socg2021/LIPIcs.SoCG.2021.27/LIPIcs.SoCG.2021.27.pdf).

## 4. Proposition pour la cible LiDAR

Construire une **géométrie observée commune**, une fois, avant découpe d'objet :

1. Préserver le lien site→tous les retours bruts, la trame, ses transformations, les origines de visée et l'incertitude disponible. Les outils actuels possèdent certains indices bruts mais ne les exportent pas ; le champ sensor des vidéos est facultatif et arrondi.
2. Construire de petits éléments de surface sur les voisinages d'acquisition, ou des surfels lorsque le maillage n'est pas justifié. Contrôler l'étendue des interpolations, les sauts de profondeur et l'appui des observations. Conserver séparément surface observée, espace effectivement traversé par un rayon et zone inconnue. Une absence de retour n'est pas une preuve de vide ; un masque de sol ou une découpe ne doit pas créer une fermeture.
3. Chaque face porte ses observations d'origine, sa mesure d'aire et sa confiance. Les points sans face restent représentés. Une estimation de normale ou une interpolation reste déclarée comme estimation.
4. HGP attribue ensuite les faces aux états datés des branches. La forme affichée d'un état est l'union de ses faces, avec les faces ambiguës gardées en réserve visible. Aucun convex hull au moment des fusions.
5. Ajouter la mosaïque d'ordre k comme canal structurel : cellules, incidences, échelles, appartenance FULL. Si elle est partielle, publier ce statut et les frontières incomplètes. La filtrer par un masque de surface ne conserve pas automatiquement les composantes de FULL.

L'emploi des lignes de visée avec Delaunay a un précédent explicite chez [Labatut–Pons–Keriven](https://imagine.enpc.fr/publications/papers/CGF09.pdf). Leur reconstruction volumique n'est pas adoptée ici : la présente proposition vise une surface ouverte et conserve l'inconnu.

## 5. Une hiérarchie qui ne déforme pas la géométrie

Pour le premier pilote, utiliser la hiérarchie laminaire de points native pour le regroupement de cette surface fixe, en conservant FULL et ses incidences comme autre canal. La laminarisation est une lecture déclarée de FULL ; les couvertures FULL natives peuvent partager des sites.

Règle constructive : partir d'un complexe de surface à incidences communes. Chaque atome τ est porté par des sites V_τ, avec V_η inclus dans V_τ pour toute face η de τ ; prendre les sites des sommets, ou les unions cohérentes de leurs provenances, satisfait cette condition. Pour des surfels indépendants sans ces incidences, ne revendiquer que des unions d'atomes, pas le lemme de sous-complexe ci-dessous. Attendre que tous soient entrés dans la hiérarchie de points et aient le même ancêtre vivant. Sa date est le maximum des dates d'entrée et de leur date de réunion ; après cette date, son propriétaire suit les parents. Avant cette date, τ reste dans une réserve géométrique. Un atome n'est pas recréé lorsque sa branche fusionne. Les poids d'aire ou de retours restent fixes et distincts.

Formellement, si (o_i,e_i) est l'attache datée du site i, poser w_τ=LCA{o_i:i∈V_τ} et β_τ=max({e_i}, naissance(w_τ)). Sans ancêtre commun, β_τ=∞. Les coupes sont fermées et traitent le plateau atomiquement. Pour η face de τ, β_η<=β_τ : les atomes actifs forment donc un sous-complexe. Cette règle est une proposition déduite de la hiérarchie, pas un export déjà implémenté.

À k fixé, les unions d'atomes sont alors emboîtées par construction. Un bloc peut rester géométriquement déconnecté, notamment quand l'objet est occulté ; ne pas inventer une face pour le rendre connexe. Pour changer k, employer les applications FULL ou un transport explicite ; ne pas inférer un pooling conservatif de l'emboîtement spatial des barycentres. Ce choix prolonge le [contrat des coupes et masses](../../../Zoltan/FoundationModel/CONTRAT_COUPES_ET_MASSES_20260926.md).

## 6. Premier livrable et critères de décision

Un petit prototype doit comparer, sur les mêmes observations et aux mêmes coupes : supports actuels, cellules d'ordre k correctement construites, et surface observée regroupée par HGP. Commencer par k=1/2 puis 5, avec un anneau percé, deux nappes proches, une discontinuité de profondeur et un objet partiellement occulté. Sur données réelles, conserver la provenance capteur et utiliser les mêmes faces pour toutes les variantes.

Mesurer : distance des retours à la surface, surface sans appui observationnel, traversées d'espace libre observé, ponts entre nappes, conservation des ouvertures, stabilité sous permutation et coût/volume exporté. Les scènes synthétiques peuvent donner une vérité géométrique ; les scènes réelles demandent de distinguer erreur et zone inconnue. La justesse des cellules ne garantit pas qu'une branche corresponde à un objet sémantique.

Pour le modèle : conserver les éléments observés, les masses et les réserves ; HGP fournit les regroupements, les niveaux de fusion et les relations. La mosaïque ajoute un descripteur structurel dont l'apport doit être comparé à ce socle. Le coût de cette géométrie s'ajoute à celui de FULL.

## 7. Vérification bornée de cette note

Le script verify.py vérifie en fractions exactes les quatre exemples (tétraèdre/octaèdre, cercle non MEB, moyenne contre k-ième distance, carré dégénéré), ainsi que les empreintes des sources relues et la garde d'admission du catalogue. Ce sont des contre-exemples et des contrôles de formule, pas un constructeur de mosaïque ni un oracle de surface. Rejeu : python3 -B verify.py, puis python3 -O -B verify.py. Aucun binaire natif, entraînement, donnée LiDAR ou appel GCP.
