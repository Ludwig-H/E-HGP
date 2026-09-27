# Audit mathématique — masses de facettes à ordre K fixé

27 septembre 2026. Lecture indépendante sur la base `69367457d`.
Ce document formule des conditions et des portes de qualification ; il ne
constitue pas un reçu de tests de la nouvelle implémentation. Aucun moteur,
ancienne capture, source d'un autre auteur ou service cloud modifié.

## Verdict et frontière de l'objet

Oui, on peut construire une référence pondérée **à K fixé** en formant le
graphe des facettes des cofaces de Gabriel, puis en utilisant Kruskal.
Aucun autre ordre ni application verticale n'est nécessaire à la sélection.
Mais ce calcul ne se déduit **pas de T_K nu** : parents, niveaux et unions
de points ne déterminent pas les sommes d'incidences du §9.1.

Deux formulations honnêtes sont possibles :

- « référence Gabriel pondérée à K fixé, confrontée au T_K natif » si l'on
  reconstruit le graphe des facettes ;
- « T_K enrichi d'un supplément d'incidences qualifié » si les facettes,
  poids et dates sont effectivement raccordés à ses composantes.

Une identité des ensembles de points couverts à certaines coupes ne suffit
pas à la seconde formulation. Il faut aussi retrouver l'appartenance des
facettes positives et leurs dates d'entrée, y compris les apports qui ne
modifient pas la couverture ponctuelle.

La thèse, proposition 6 et théorème 5, p.90–91, compare les **couvertures
des composantes non triviales**, sous les hypothèses géométriques annoncées ;
elle ne livre pas l'identité de toutes les feuilles de deux arbres pondérés.
Les coordonnées entières peuvent avoir des dégénérescences exclues par la
position générale du théorème. Leur traitement natif par plateaux doit être
recoupé, pas hérité du seul énoncé de la thèse.

## 1. Catalogue, scores et conservation

Fixer un catalogue complet C de cofaces de Gabriel distinctes de taille K+1,
au sens miniball de la définition 28, p.87. Les triangles obtus ne sont pas
exclus. Fixer F=∂C, **toutes** leurs facettes de taille K, et des poids
positifs finis ψσ=ρ(σ)^(-z), avec z=1 ou 2. Une coface est comptée une fois,
pas une fois par support minimal, arête du graphe dual ou événement FULL.

Pour x appartenant à au moins une facette :

$$S_\tau=\sum_{\sigma\in C,\,\tau\subset\sigma}\psi_\sigma,\quad T_x=\sum_{\tau\in F,\,x\in\tau}S_\tau,\quad w_{x\tau}=S_\tau/T_x,\quad m_\tau=\sum_{x\in\tau}w_{x\tau}.$$

Pour T_x=0, déclarer le point non couvert et fixer ses poids à zéro.
Les invariants sont : Στ∋x w_xτ=1 sur X+={x:T_x>0}, puis
Στ∈F mτ=|X+|. Il ne faut pas remplacer une masse de facettes par la
cardinalité de l'union des points de la composante.

Le dénominateur est fixé sur **tout le catalogue**, pas recalculé par coupe,
après condensation, ou après sélection. Les votes des seuls clusters
sélectionnés peuvent donc sommer à moins de 1 ; la masse restante n'est
pas redistribuée implicitement. Le chemin historique ignore les faces
étiquetées bruit dans l'argmax, et classe un point sans aucun vote en bruit.

### Borne utile : chaque facette a une masse au plus égale à 1

Chaque coface contenant x possède exactement K facettes contenant x.
En échangeant les sommes finies :

$$T_x=K\sum_{\sigma\in C,\,x\in\sigma}\psi_\sigma.$$

Pour x∈τ, les cofaces contenant τ sont incluses dans celles contenant x.
Donc Sτ≤T_x/K, w_xτ≤1/K, et, puisque |τ|=K :

$$0<m_\tau\leq1.$$

Cette borne est indépendante de K. Elle utilise F=∂C complet et les mêmes
contributions positives dans S et T. Une restriction de F suivie d'une
renormalisation peut l'invalider. Une petite violation numérique de 1 doit
être diagnostiquée, pas devenir une nouvelle branche géométrique.

## 2. Naissances des facettes : la condition qui suffit

Trois conventions sont différentes :

1. **Filtration géométrique** : τ existe à partir de bτ=ρ(τ).
2. **Graphe à sommets fixes** : toutes les facettes de F existent dès r=0 ;
   une liaison apparaît à ρ(σ).
3. **Première incidence** : τ apparaît à aτ=minσ⊃τ ρ(σ).

Toujours bτ≤aτ. Avant aτ, τ est isolée dans le graphe de Gabriel, quelle
que soit sa date d'activation parmi ces conventions. Dès qu'une liaison
incidente existe, ses deux extrémités sont présentes dans les trois cas.
Leurs composantes **non triviales** sont donc identiques à chaque rayon.

Pour un seuil m>maxτ mτ, aucune facette isolée n'est admissible. Les
premières différences d'activation concernent seulement des composantes
qui seront éliminées : elles ne changent ni les masses des composantes
admissibles, ni leurs dates d'entrée/sortie, ni la stabilité EOM associée.
En particulier, **m>1 suffit uniformément** ; pour des seuils entiers,
m≥2 suffit. Il n'est pas nécessaire d'imposer m>K ou un autre plafond
arbitraire dépendant de K.

Portée exacte de cette conclusion : même C, mêmes mτ, mêmes liaisons,
atomisation des mêmes niveaux et même politique racine. Elle compare les
trois activations du graphe Gabriel, pas automatiquement l'arbre natif
compressé et sa mesure, ni un catalogue historique différent.

Pour m≤maxτ mτ, une feuille-facette peut devenir un cluster : les dates
comptent alors. Dans la convention géométrique, sa durée terminale s'arrête
à λ=bτ^(-z), généralement finie pour K≥2 ; une feuille virtuelle à rayon
zéro donne λ=∞. Présenter les deux comme équivalentes serait incorrect.
Une API EOM générale peut autoriser ce cas, mais doit nommer sa convention.

Le Cython historique initialise chaque facette avec sa masse, sans créer
de cluster singleton éligible à l'initialisation. Il n'est donc pas une
autorité générale pour le cas m≤maxτ mτ. Le profil m≥2 évite ce désaccord.

Les facettes de Čech isolées absentes de F ne portent pas de masse selon
le §9.1 choisi ici. Elles restent pertinentes à l'objet topologique FULL,
mais leur ajouter une masse unitaire changerait le modèle statistique.
Les cas C vide, K=n, point non couvert ou forêt disjointe doivent avoir un
statut explicite ; ne pas fabriquer une liaison finie pour les masquer.

## 3. Construire le graphe sans perdre la mesure

Pour chaque σ∈C, accumuler ψσ dans les K+1 scores de ses facettes.
Pour la connexité seulement, une étoile de K liaisons entre ces facettes,
toutes au niveau exact ρ(σ), remplace leur clique. Cette économie ne permet
pas de supprimer des contributions au score.

Puis Kruskal préserve toutes les composantes seuillées de ce graphe fini.
Traiter simultanément les égalités **exactes de rayon carré** : une chaîne
binaire de fusions de même niveau n'est pas un ordre statistique de départage.
Le passage λ=ρ^(-z), z>0, ne change ni cet ordre ni les plateaux.

Les cofaces redondantes pour Kruskal doivent continuer à contribuer à Sτ.
De même, les apports de facettes à une composante peuvent modifier sa masse
sans créer une fusion FULL ni ajouter un nouveau point à sa couverture.
Il faut préserver ces entrées datées dans tout raccord pondéré à T_K.

Point d'implémentation à ne pas manquer : l'ancien `native_export.cpp`
met `keep_catalogue=false` et n'exporte que les populations référencées
par les contributions de couverture de T_K. Ce sous-ensemble n'est pas un
catalogue de cofaces contributives qualifié. La référence pondérée doit
retenir explicitement le catalogue canonique complet nécessaire à C.

## 4. Énumération depuis une boule : conditions et coût

Soit une boule canonique B, avec intérieur strict complet I et coquille
complète S. Les cofaces de Gabriel σ dont la miniball est B sont exactement :

$$\sigma=I\cup U,\qquad U\subseteq S,\qquad |U|=K+1-|I|,\qquad c_B\in\mathrm{conv}(U).$$

La vacuité impose d'inclure **tout I**. La condition convexe assure que B
est réellement la miniball, et pas seulement une boule contenant σ.
Sur une coquille dégénérée, la cardinalité ne suffit pas. Exiger qu'U
contienne un unique support choisi est également incomplet : plusieurs
supports minimaux peuvent certifier la même boule. On peut utiliser une
table exacte « contient le centre » ou tous les supports minimaux, puis
énumérer chaque U une fois, pas chaque couple (U, support).

En dimension 3, un support minimal a au plus quatre points. Cela borne
la certification locale, **pas** le nombre de cofaces : avec B boules,

$$Q=\sum_B { |S_B| \choose K+1-|I_B| }.$$

borne le nombre de sous-ensembles candidats avant le test convexe, avec
coefficient nul pour une cardinalité impossible. Le nombre G de cofaces
acceptées peut être important même quand les événements FULL sont peu
nombreux. Il faut publier B, Q, G, |F|, (K+1)G contributions, KG liaisons
proposées, temps de tri/Kruskal et mémoire réelle.

Le domaine natif actuel borne |S| à 12 et refuse explicitement au-delà ;
le maximum combinatoire par boule y est 924, et une table de masques a
au plus 4096 entrées. Ce sont des bornes **de ce domaine**, pas une preuve
de complexité globale ni une autorisation de tronquer une plus grande
coquille. En position générale S est un support unique : le cas régulier
n'émet une coface que lorsque |I|+|S|=K+1.

La clé exacte canonique de B et l'identité triée de σ évitent les doublons.
Une même coface a une unique miniball ; plusieurs supports d'une même
boule ne doivent donc jamais multiplier sa masse. Une limite de ressources
éventuelle doit produire un refus explicite, jamais un catalogue partiel
présenté comme complet ou un seuil m artificiellement augmenté.

Avec F facettes explicites, la mémoire de cette référence comprend leurs
K incidences : ce n'est plus simplement O(n+C_condensé). La borne sur les
nœuds condensés peut encore utiliser la masse totale |X+|, si ses feuilles
ont des ensembles de facettes disjoints de masse au moins m ; elle ne
borne ni |F| ni le travail de production de C.

## 5. Protocole EOM, exposants, racine et numérique

- Recalculer S, T, m et les votes pour chaque z ; changer seulement λ
  ne reproduit pas la méthode. La topologie brute du graphe ne change pas,
  mais sa condensation peut changer puisque ses masses changent.
- Pour un rayon carré exact q, ψ=q^(-z/2), pas q^(-z). En z=2, tout est
  rationnel sur les petits oracles : scores, masses et stabilités finies.
  En z=1, les masses impliquent des quotients de sommes de radicaux ; un
  juge exact des seuls numérateurs de vote ne qualifie pas ces quotients.
- Une implémentation flottante doit publier son statut numérique et ses
  marges pour m, EOM et votes. Une égalité/ambiguïté n'est pas rendue exacte
  par un epsilon. Ne pas confondre exactitude des niveaux natifs et exactitude
  de toutes les décisions statistiques.
- Poser la même politique `allow_single_cluster=false` au comparatif EOM
  commun, avec la même notion de racine globale. Conserver séparément la
  reproduction historique qui autorise sa racine ; ne pas attribuer à la
  masse un gain dû à ce changement de politique. Pour une forêt, déclarer
  le rôle d'une éventuelle racine virtuelle à λ=0 et de ses composantes.
- Accepter tout seuil positif du domaine annoncé ; m>|X+| donne zéro
  cluster admissible. Aucun besoin de plafonner m à K ou au nombre de
  facettes. Le raccord sans dates individuelles revendique m>maxτ mτ.
- Sur une feuille virtuelle à λ=∞ : durée [∞,∞]=0 avant toute soustraction,
  durée depuis une naissance finie égale à ∞. Les choix entre stabilités
  infinies ont une convention, pas une preuve de qualité géométrique.
- Les masses fractionnaires au seuil ne garantissent pas que les labels
  durs après compétition des votes contiennent chacun au moins m points.
  Les anciennes bornes F1 fondées sur ce minimum cardinal ne se transfèrent
  pas automatiquement.
- Déclarer le départage des votes, le bruit et toute propagation 1-NN.
  Le vote plat du §9.1 n'est pas un routage emboîté. Un futur parent→enfant
  exclusif est une méthode distincte, avec restes et dates conservés.

## 6. Oracles et portes proposés, pas encore exécutés ici

1. **Catalogue indépendant sur petits nuages** : énumérer tous les
   (K+1)-sous-ensembles, calculer leur miniball par supports de taille ≤4,
   puis tester tous les autres points pour la vacuité stricte. Comparer
   identités et rayons aux sorties I/coquille, pas seulement un compte.
   Inclure triangle aigu, triangle obtus, carré cocirculaire, tétraèdre,
   points collinéaires, intrus strict et plusieurs supports d'une boule.
2. **Filtration complète** : énumérer les K-facettes de Čech et leurs
   naissances ; comparer les couvertures non triviales avec Gabriel,
   Kruskal et FULL avant/à chaque niveau critique. Publier séparément les
   facettes isolées. Inclure une continuation sans nouveau point couvert.
3. **Mesure** : calcul direct par incidences contre
   T_x=KΣσ∋x ψσ ; vérifier toutes les conservations et mτ≤1. Un catalogue
   à coface unique donne exactement mτ=1 pour ses K+1 facettes. Retirer
   une coface redondante doit modifier les scores même si les coupes restent
   identiques ; un mutant « poids seulement sur les arêtes MST » doit mourir.
4. **Naissances et seuils** : comparer sommets à zéro, naissance géométrique
   et première incidence à m=2 et plusieurs seuils supérieurs : mêmes
   composantes condensées pondérées, stabilités et labels. À m=1, tester
   explicitement le désaccord attendu lorsque mτ=1 et bτ>0. Tester aussi
   m>|X+|, aucune coface et points non couverts.
5. **Sélection indépendante** : sur de petits arbres pondérés, énumérer les
   antichaînes admissibles pour vérifier l'optimum EOM, en interdisant la
   racine dans les deux méthodes comparées. Tester multifusions atomiques,
   égalité parent/somme des enfants, feuilles lourdes et infinis dans le
   profil général, indépendamment du profil Gabriel m≥2.
6. **Métamorphismes** : translation et isométrie exacte ; dilatation commune
   a>0 (S et λ multipliés par a^(-z), masses et labels inchangés hors
   ambiguïtés numériques) ; permutations d'entrée et d'énumération.
   Pour les votes exactement exæquos, un départage par ID peut changer la
   partition après renommage : publier cette convention plutôt que prétendre
   une invariance impossible à partir du seul test de labels numériques.
7. **Raccord à T_K** : si cette revendication est maintenue, comparer aussi
   l'affectation datée de chaque facette positive, les masses par composante
   et les entrées tardives. Une égalité des seules unions de points ne ferme
   pas cette porte. Vérifier qu'aucun autre ordre ne décide les labels.
8. **Lectures normales et -O** : mêmes validations, refus et objets ; pas
   d'invariant porté uniquement par `assert`. Ne pas réutiliser les succès
   de première couverture comme preuve de cette nouvelle mesure.

## Sources vérifiées

- [Thèse](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages
  imprimées 86–91 (filtration et Gabriel), 96–97 (mesure et vote), 100
  (scores avant MST). Ajouter 26 pour les pages PDF.
- [Contrat d'incidence v7](../../../morsehgp3D_v7/audits/CONTRAT_MASSES_VOTE_COURANT.md)
  et [portée du juge de votes](../../../morsehgp3D_v7/audits/AUTORITE_VOTE_P3_COURANTE.md).
- [core.py historique](../../../HGP-old/src/hgp_clusterer/core.py), lignes
  202–215, 255–263 et 297–324 ; [_cython.pyx](../../../HGP-old/src/hgp_clusterer/_cython.pyx),
  lignes 446–461 (contrat arbre, initialisation), 481–483 (λ), 840–848 (scores).
- [BallData](../../src/tower/forest/ball_data.hpp), lignes 1–32 ;
  [plateaux locaux](../../src/tower/forest/local_plateau.hpp), `contains_center()`
  et `minimal_supports()` ; [FULL](../../src/tower/forest/full_ball_tower.hpp),
  `visit_block_at` distingue représentants/contributions de couverture.
- [Export antérieur](../../audits/b_point_hierarchy_k_20260927/native_export.cpp),
  `run()` et `output()` ; [relecture précédente](../../audits/RELECTURE_THESE_ET_HGP_OLD_20260927.md).
