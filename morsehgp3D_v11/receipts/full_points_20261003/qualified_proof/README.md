# Hyperarêtes de couverture qualifiées : preuve et limites

Revue indépendante du 3 octobre 2026, source documentaire figée `a45daff3a` (moteur c40). Cadre : exploration v11, CPU de référence, sites distincts de poids un, `public_status=not_claimed`. Aucun import des oracles produit, aucun natif/GCP, aucune modification du moteur. Ce reçu propose un **extracteur de recherche**, pas une règle choisie ou qualifiée du produit.

## Construction

À rayon r, pour une composante C de L_k(r²), écrire E_C(r)={i : dist(x_i,C)≤r}. Fixer un entier de transmission 1≤m≤n, indépendant de la condensation. Garder les hyperarêtes E_C(r) de cardinal au moins m ; les blocs de points sont la fermeture d'équivalence engendrée par ces hyperarêtes. Un point sans hyperarête reste singleton inactif ; sa date d'entrée est e_i=inf{r : i appartient à une hyperarête qualifiée}. Publier toutes les activations/fusions d'un niveau **après** le plateau fermé, sans chaîne binaire artificielle.

Cette règle utilise toute la couverture dynamique, pas seulement les premières couvertures. Dans le modèle exact Γ_k, E_C est la réunion des k-parties de la composante. Cela suit la définition de couverture, reprise dans [L03 §3.1](sources/morsehgp3D_v11/receipts/audit_full_hierarchie_20261002/suivi_verrous/snapshot/private/L03_MATH_POINTS.md), et les relations [T1/P2–P5](sources/morsehgp3D_v11/docs/MATHEMATIQUES.md). L'incidence peut apparaître dans une continuation sans nouveau nœud de fusion : ne pas la perdre.

## Garanties

1. **Laminarité en r.** Une composante s'inclut dans son ancêtre et sa couverture ne perd aucun ID. Une hyperarête qualifiée est donc incluse dans une hyperarête qualifiée ultérieure. Les connexions de points ne disparaissent jamais. Les niveaux sont des rayons ; les sorties bornées les codent en rayons carrés exacts.
2. **Équivariance.** La définition ne dépend ni d'un ordre des IDs, ni d'un support canonique, ni d'un axe. Une renumérotation ou une isométrie transporte les ensembles et blocs ; une homothétie de facteur s multiplie les rayons par |s|. Les fabriques/grilles publiques gardent leurs propres contraintes de domaine.
3. **Stabilité 1ε en rayon.** Reprendre P5, déjà présent dans MATHEMATIQUES : si les IDs sont appariés et chaque site bouge d'au plus ε, une composante X à r se transporte dans une composante Y à r+ε, qui couvre tous les mêmes IDs. Le cardinal au moins m survit. Toute chaîne d'hyperarêtes X se transporte donc en une chaîne Y. Par symétrie, pour les hauteurs de réunion u et dates d'entrée e, |u_X(i,j)−u_Y(i,j)|≤ε et |e_X(i)−e_Y(i)|≤ε. Aucun appariement de supports/nœuds ni stabilité des labels après sélection n'est promis. Ce n'est pas une nouvelle preuve de P5 : c'est une conséquence pour cet extracteur.
4. **Verticalité aux mêmes coupes.** E_C^k(r) est inclus dans la couverture de son image FULL à k−1. Si m_(k−1)≤m_k, chaque hyperarête qualifiée haute reste qualifiée en bas. Ainsi Π_(k,m_k)(r) raffine Π_(k−1,m_(k−1))(r), et u_(k−1)≤u_k. Cela ne rend pas laminaire l'union des familles statiques de tous les ordres à des dates différentes.
5. **Optimalité limitée.** Pour i≠j, soit w(i,j) le premier rayon où une même composante qualifiée couvre i et j ; fixer w(i,i)=0. La fermeture est u(i,j)=min_{chemins i→j} max_{arêtes} w. Elle est l'ultramétrique **la plus grande dominée par w** : toute ultramétrique v≤w vérifie v(i,j)≤max w le long de chaque chemin, donc v≤u ; et u≤w par l'arête directe. Elle retarde autant que possible les réunions tout en respectant toutes ces échéances. Cette optimalité formelle ne signifie pas meilleur clustering.

## Résultats exacts bornés

`check.py` calcule les MEB en Fraction par minimum des circonsphères englobantes de sous-parties affinement indépendantes, puis Γ_k et la fermeture des hyperarêtes ; aucun code des deux routes de référence n'est importé. Cinq fixtures, 52 contrôles de verticalité aux coupes, 32 coupes K2/m2 contre liaison simple, 10 fermetures matricielles minmax. Normal et −O doivent rendre les mêmes octets.

| Fixture K2 | Résultat |
|---|---|
| Deux triangles entiers, pont 2000/1998/1700 | m=3 rend ABC et DEF, dès β=249978000484/187489, puis les réunit à β=3731956 / 3728225 / 3194656 ; l'intervalle séparé est positif dans chaque cas. m=2 les percole trop tôt. |
| 0,2,100,102 | m=3 refuse les deux branches de taille2 ; les quatre points entrent seulement à β2500. m=2 garde les deux paires, puis les réunit à β2401, avant la fusion FULL β2500. |
| (6,2),(0,0),(0,4),(12,0),(12,4) | À β100/9, deux composantes FULL distinctes couvrent {0,1,2} et {0,3,4}. Le point0 est commun et chaque branche n'a que2 exclusifs. m=3 percole les cinq points à β100/9 ; FULL ne fusionne qu'à β36. Le point0 entre core à β40. |

**K2/m2 redonne toujours la liaison simple au seuil de distance 2r.** Toute paire à distance≤2r est un sommet actif et fournit l'hyperarête de ses deux sites. Inversement, chaque k-partie d'une composante Γ2 est une paire de distance≤2r et les cofaces triples relient leurs sites dans ce même graphe. Les couvertures qualifiées ne peuvent joindre deux blocs distincts de ce graphe. L'égalité n'est donc pas seulement un fait des fixtures.

La preuve pour la thèse exacte ne dépend pas de l'arrondi entier. Pour deux triangles équilatéraux de côté a, orientés comme la figure face à face, et un pont d avec a/2≤d≤a, les triangles qualifient m=3 à r=a/√3. Hors CD, toute paire transversale a longueur au moins sqrt(a²+d²+√3ad), donc toute triple mixte a rayon au moins la moitié de cette longueur, strictement supérieur à a/√3. Jusqu'à cette borne, les seules couvertures sont ABC, CD et DEF ; m=3 garde les deux triangles. Les coordonnées entières du dépôt ont côtés² 4000000 et 3999824 : elles sont proches de l'équilatéral, sans l'être.

## Concession obligatoire

Les blocs de points n'ont pas nécessairement un propriétaire FULL unique. La contre-garde à cinq points le démontre ; elle est imposée par les échéances w et l'inégalité ultramétrique, pas par un mauvais ordre de fermeture. Pour retarder cette percolation, il faut rejeter/différer au moins une échéance de co-couverture, conserver une sortie recouvrante ou accepter une autre garantie.

m est un seuil de transmission, **pas min_cluster_size** et pas un certificat de membres exclusifs. Fixer m=3 peut sauver les triangles même lorsque la condensation utilise mcs=2 ; le même choix supprime aussi les structures légitimes de deux points. La règle ne garantit ni les 125 cibles anciennes, ni qualité statistique, ni sélection EOM, ni absence de filament. Aucune calibration de m ni supériorité face à HDBSCAN n'est acquise.

## Coût et raccord restant à établir

Le modèle Γ est exponentiel et seulement borné. Un futur consommateur natif peut balayer les incidences de couverture complètes en conservant deux DSU séparés : composantes FULL et points. Pour une composante non qualifiée, une liste dédupliquée de moins de m IDs suffit ; après qualification, un ancrage du bloc de points suffit pour transmettre les nouvelles incidences. Lors d'une fusion FULL, réunir les petites listes ou leurs ancrages. Cela évite une matrice n×n mais ne dispense ni de toutes les incidences, ni des continuations, ni d'un compte de coût/mémoire réellement payé. Aucun port natif ni banc LiDAR/Zoltan n'est joué dans ce reçu.

Toutes les sources/captures sont dans cette capsule ; SOURCE_BEFORE et SOURCE_AFTER vérifient les objets Git identiques. SHA256SUMS inventorie tous les fichiers sauf lui-même. La publication et les campagnes gardées restent sous contrôle de l'agent principal.
