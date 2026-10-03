# FULL → points : conseil mathématique courant au développeur

3 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Moteurs relus c40 puis publication
b872 ; [qualification, corrections et temps](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
Les réponses Q1–Q5 et leurs invariants sont intégrés dans
[MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) ; leurs preuves restent dans
[le reçu historique](../receipts/audit_independant_20261002/math_locks_review/README.md).
Cette note remplace mon suivi précédent ; seuls les conseils encore utiles
restent ici, les preuves et campagnes closes sont dans receipts/Git.

**Priorité actuelle : qualifier H_m en rayon avec comparaisons exactes.**
Cette pendaison intérieure préserve la fidélité à FULL et traite les triangles
et la frontière partagée. La garantie peut être renforcée à3ε pour entrées
et réunions. Le plafond en niveau carré doit être corrigé et deux helpers
arithmétiques du WIP rayon ont leurs contre-gardes ci-dessous.
Les campagnes montrent des succès ciblés, sans avantage uniforme.
La fermeture extérieure reste une référence stable et une borne inférieure
canonique ; aucune des deux règles ne résout seule la synthèse multi-k.

## Aide au nouveau candidat intérieur H_m

3 octobre 2026, contrelecture du WIP développeur **e26b48055**, distinct de
la campagne c40/a12 ci-dessus. [Source documentaire figée](../receipts/hm_review_20261003/source/morsehgp3D_v11/docs/HIERARCHIE_POINTS.md)
SHA088dd067 et [preuves/relecture](../receipts/hm_review_20261003/README.md).
Le candidat choisit une pendaison fidèle : t_i est la première couverture
qualifiée, D_i la durée maximale d'un rival avant sa réunion avec la première
lignée, e_i=t_i+D_i. Il suit ensuite le propriétaire FULL vivant à e_i.
**C'est une réponse pertinente au problème de frontière :** il conserve les
triangles exacts et évite la fusion anticipée de la fixture à cinq points.
Les réserves suivantes corrigent ses garanties et sa variante arithmétique ;
elles ne réfutent pas la construction intérieure.

### Stabilité : correction du plafond et meilleure constante

La preuve abstraite sous un véritable entrelacement transportant les
couvertures qualifiées est favorable : |Δt_i|≤δ, |ΔD_i|≤2δ, donc |Δe_i|≤3δ.
On peut aussi améliorer **|Δu(i,j)|≤3δ**, au lieu de5δ. Pour les premières
attaches p_X,p_Y, la définition de D_X donne
m_X(p_X,ψp_Y)≤D_X+t_Y+δ≤e_X+2δ. Appliquer φ puis
φψ=Up(2δ) donne m_Y(φp_X,p_Y)≤e_X+3δ : remonter un point ne diminue pas
son niveau absolu de rencontre. Les pendaisons finales g_X,g_Y satisfont
m_Y(φg_X,g_Y)≤e_X+3δ ; l'ultramétrie par la chaîne
g_Yi→φg_Xi→φg_Xj→g_Yj et u_X(i,j)≥e_Xi,e_Xj donne la borne3δ.
Symétrie. Ces identités d'entrelacement et la commutation aux remontées sont
nécessaires ; un seul transport ne suffit pas. Aucune constante optimale établie.

**La conversion δ=2ε√Λ+ε² est fausse si Λ est la naissance de la racine.**
Des couvertures peuvent encore grandir par continuation après cette date.
Contre-garde entière k3/m4, ε85 :

| Quantité | X | Y |
|---|---:|---:|
| Naissance de l'unique racine FULL | 1221025 | 1249924 |
| Premières qualifications, entrées et réunions H_4 | 52200625 | 53436100 |

X={(2775,10000,0),(17225,10000,0),(17140,11105,0),(17140,8895,0)},
Y={(2690,10000,0),(17310,10000,0),(17224,11118,0),(17224,8882,0)}.
Tous les déplacements ont norme85 ; aucun rival, D_i=0. Même avec
Λ=max des deux racines, δ197285 : la variation1235475 dépasse3δ591855
**et**5δ986425. Un cercle plus petit donne déjà racine16, couverture
qualifiée25. Le [vérificateur autonome](../receipts/hm_review_20261003/check_scale.py)
passe69gardes en normal/−O, avec coupes Γ exactes.

Correction suffisante en β : un plafond commun couvrant les dates pertinentes,
p.ex. max des deux MEB globales. Une borne peu coûteuse suffit aussi :
max_X,Y[Σ_axes(max−min)²/4], rayon carré des boules circonscrites aux boîtes ;
inutile de calculer une MEB globale native pour obtenir ce certificat.
À ce plafond Γ est connexe et couvre tous les sites ; pour m≤n,
e_i et u restent en dessous. Au-delà on prolonge l'unique branche racine.
**En rayon, la variante bénéficie directement de3ε pour les entrées ET réunions**,
sans dépendance d'échelle. Son arithmétique doit toutefois rester exacte.
La margeκ1 minimise les bornes de la famille proposée, sans preuve d'optimalité
globale ; m>n doit avoir une politique explicite de points inactifs/refus.

### Variante rayon : deux corrections arithmétiques concrètes

Le snapshot30447f75, puis le helper ajouté dans f023f6d0, sont figés dans
le nouveau reçu ; aucun transfert de campagne native à ces nouveaux fichiers.
Le [contrôle borné](../receipts/hm_review_20261003/check_radicals.py) passe
**1536gardes** normal/−O, incluant les véritables helpers extraits et hachés.

1. `RValue.cmp` raffine jusqu'à3072bits puis rend0 sans certificat d'égalité.
   Deux dates valides, (t,m,q)=(2,162,50) et(8,98,32), valent exactement5√2
   malgré six radicandes différents. Les classes de carrés rationnelles les
   reconnaissent sans factorisation : tester si le ratio a/b a numérateur
   et dénominateur carrés parfaits, puis cumuler les coefficients signés.
   Des classes distinctes sont linéairement indépendantes sur Q. Si le résultat
   non nul reste indécidable dans le budget, **refuser**, jamais déclarer un plateau.
2. `cmp_level` filtre avec une erreur relative à la date finale. La soustraction
   √m−√q peut annuler de grands termes ; l'erreur dépend de leur taille.
   Témoin scalaire exact : t=1/4, m=14000000², q=(14000000−6/997)²,
   date=1/2+6/997 ; cible=date+2^-70. Le helper rend+1, son calcul exact−1.
   Les radicandes tiennent en192bits et dans les magnitudes u24 ; aucune fixture
   Cloud génératrice ni mauvais owner natif u21 n'est démontré. Borner l'erreur
   **absolue des trois racines et de la cible**, puis repli exact si ambigu.

Avant port natif, nommer un type de date de points : β=t+meet−q n'est pas
un `LevelRank` et peut demander576bits,577pour la somme intermédiaire,
avec produits croisés à borner séparément. En rayon, conserver les trois
radicandes et comparer les six termes exactement. Les nouveaux événements
doivent participer au même ordre exact et aux mêmes plateaux fermés que FULL.

### Mesures : succès réels, périmètre corrigé

Les deux sessions développeur `claudepts1/2` sont closes avec arrêts ciblés
certifiés ; résultats et manifestes intègres. Le consommateur joué est
**5df63b60**, distinct du LIVE0da8 et de la variante rayon.
[Recoupe indépendante](../receipts/hm_review_20261003/campaign_review.json) :

- pts1 :128synthétiques/704objets complets, grille isotrope adaptative par
  scène (environ20–273µm), mêmes sites pour toutes les méthodes ; ce n'est
  pas notre campagne à1mm. Les44noms LiDAR sont **42entrées distinctes**
  après déduplication par SHA(XYZ,labels), soit497instances,53sauvetages
  (objet,k),11pertes et **7sauvetages forts**, au lieu de524/57/11/8 lignes.
  Les doublons sont démo01↔08/001176 et démo03↔08/000048. Les succès restent acquis.
- pts2 :72trames voisines sélectionnées autour des difficultés et20témoins.
  Voisines :101sauvetages/23pertes,24lignes fortes correspondant à9couples
  classe/instance répétés entre trames ; ce ne sont pas24objets indépendants.
- Les20témoins donnent **zéro sauvetage et zéro perte** au seuil1/2 ; la moyenne
  H_m−HDBSCAN vaut environ−0,00359/−0,00437/−0,00216/+0,000685 à k2/3/5/10.
  Ils doivent accompagner les cas favorables. La sélection connue et les
  voisins corrélés excluent une revendication générale de supériorité.

Ces scores restent des meilleurs nœuds disponibles, sans partition sélectionnée,
ni confrontation identique aux15seuls objets de notre campagne. Ne pas agréger
les deux campagnes ni transférer ces résultats à une nouvelle recette numérique.
Aucun contrat100ms, GPU ou massif acquis.

### Conseil de conception

Aux mêmes(k,m), toute pendaison fidèle **qualifiée** H vérifie
**u_fermeture(i,j)≤w_qualifiée(i,j)≤u_H(i,j)** : à leur réunion les deux sites
sont couverts par la même composante qualifiée. La fermeture reste donc une
borne inférieure canonique utile, même lorsqu'on choisit H_m pour sa fidélité.
Sur la fixture frontière,100/9≤36. Exposer t_i, D_i et e_i séparément rend le
retard de frontière mesurable sans labels ; m reste distinct de la condensation.
L'écart ne mesure ni une erreur statistique ni une distance à une vérité terrain.

Le prochain travail utile est de corriger/qualifier la comparaison exacte en
rayon, intégrer ces fixtures et publier la preuve3ε avec ses hypothèses.
H_m reste défini à k fixé : la synthèse multi-k demeure une question distincte.

## Référence extérieure et raccord direct déjà prouvés

Les deux premières parties de la thèse imposent de distinguer couverture,
core et partition. Définition8 : $E_C(r)=X\cap\delta_r(C)$ ; core et une
première attache ne rendent pas tout ce recouvrement. La fermeture
qualifiée garde les E_C de cardinal≥m puis ferme leur équivalence, suit
les continuations et publie les plateaux ; elle est laminaire et stable
à1ε en rayon, pour k/m et IDs appariés fixés. Son optimum minmax porte
sur les échéances de co-couverture, sans optimalité statistique.

[Campagne close c40/a12](../receipts/full_points_20261003/README.md) :
12synthétiques et5Zoltan entiers, u21/1mm, k2/3/5/10, seuils{3,k+1,20},
68fits officiels sklearn1.7.2,2553comparaisons recoupées, arrêts certifiés.
À k5, meilleur IoU moyen par objet :

| Exemples | Fermeture m3 | Premières attaches/LCA | HDBSCAN |
|---|---:|---:|---:|
| Synthétiques,96objets | 0,817622 | 0,845813 | 0,774573 |
| Zoltan,15objets | 0,611804 | 0,645993 | 0,602980 |

m3 face à HDBSCAN :63/16/17 gains/pertes/égalités en synthétique et8/3/4
sur Zoltan ; face à LCA :20/60/16 et3/9/3. Les triangles **exactement**
équilatéraux montrent néanmoins le défaut de premières attaches
irréversibles ; m3 les conserve à k2. Sur la frontière à cinq points,
m3 réunit tout àβ100/9, avant FULLβ36. Stabilité de hauteur et absence
de percolation sont donc des critères distincts. Des perturbations bornées
respectent les hauteurs tout en pouvant changer l'IoU.

[Projection directe](../receipts/full_points_20261003/check_strong_projection.py),
1348gardes normal/−O : m≤k ne change rien ; `(k,k+1)=(k+1,k+1)` pour
blocs actifs, entrées et réunions. Pour k≥2/m≤k, fermer les populations
complètes I∪U des forts `p+qmin≤k≤p+|U|` donne exactement le quotient
extérieur, sans propriétaires FULL. Pour m=k+1, les forts d'ordre k+1
sont déjà les faibles de Cat_k. Plus généralement Cat_(q−1) complet
suffit aux forts(q), q≥2 : qmin≥2 impose p≤q−2, tous les contacts sont
conservés. Cette possibilité ne construit pas FULL, ne réhabilite pas le
foldv4/E5, **et ne remplace pas les propriétaires nécessaires à H_m**.
Pour m>k+1 aucun remplacement analogue des composantes FULL n'est prouvé.
Aucun port natif ni gain de temps acquis.

À mêmes coupes, unir les partitions raffinées des k redonne le plus petit
ordre, les intersecter le plus grand ; à dates différentes des branches
peuvent encore se croiser. Toute synthèse non triviale doit expliciter
son critère de niveaux/densité/frontière. Les oracles de fermeture et H_m
serviront à juger ce critère, sans ajuster un seuil après lecture des labels.
