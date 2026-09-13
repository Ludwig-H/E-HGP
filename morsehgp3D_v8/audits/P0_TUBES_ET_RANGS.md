# P0 : compter des suffixes certifiés plutôt que des couples locaux

13 septembre 2026. Preuve et modèle indépendants, ouverts après e409aa46.
Cadre initial de cette analyse : `exploration_v8_hors_registre`, `backend=none`,
`profile=quantized_u16_input_only`, `mode=audit_v7_math_and_architecture`,
`public_status=not_claimed`. Les tubes et leur préparation partagée sont
désormais implémentés et qualifiés séparément dans le
[contrat constructeur](../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md). Cette note
conserve leurs preuves sources et le modèle indépendant ; elle ne
constitue pas une demande d'intégration encore ouverte.

**Un rang projeté devient un crédit sûr si une borne transverse l’accompagne.**
Une partition en tubes, un tri et trois balayages peuvent produire les
minorants des trois voies en O(m log m) et O(m) mémoire par facteur de
m points. Aucun histogramme m×m ni choix préalable de h témoins n’est requis.
Le résidu et la somme des préparations sur les rectangles restent à payer.

Cette voie complète la comparaison Pool/DualBlocks du constructeur.
Elle ne remplace pas les obligations du [plan P0](../docs/PLAN_DE_REFONTE.md).

## 1. Un cône suffisant, avec marge stricte

Soient c_A,c_B les centres des boîtes, D leur distance, R le maximum de
leurs rayons circonscrits, et e=(c_B−c_A)/D. On suppose D>0 et D≥10R.
C’est impliqué par le [test WSPD v7](../../morsehgp3D_v7/src/wspd/wavefront.hpp)
à s≥8. Le contrat initial v8, gap_boîtes≥s·diamètre_max, implique même
D≥16R à s≥8 et suffit donc à ce lemme. Ce contrat est plus strict que
le test v7 : les deux occurrences de s ne définissent pas les mêmes
rectangles. R=0 signifie ici deux positions isolées : aucun
témoin local distinct, donc crédits nuls.

Fixons a,z dans A, z≠a, et b dans toute la boîte B. Posons u=z−a,
v=b−z, x=e·u et y=e·v. La séparation donne y≥D−2R≥8R et
la norme de la composante transverse de v est au plus 2R, donc au plus y/4.
Pour 0≤ρ<4, si x>0 et la composante transverse de u a une norme au plus ρx :

$$H=u\cdot v\geq(1-\rho/4)xy,\qquad \Xi=\lVert u\times v\rVert^2\leq(\rho+1/4)^2x^2y^2.$$

La deuxième borne suit de l’identité norme-produit moins produit-scalaire
au carré, en utilisant H>0. Elle ne remplace pas un produit vectoriel par
sa seule projection. Les choix suivants ont tous une marge stricte :

| Voie | ρ | Garantie |
| --- | ---: | --- |
| q2 | 3 | H≥xy/4>0 |
| q3 | 1 | 3H²−Ξ≥x²y²/8>0 |
| q4 | 3/4 | 2H²−Ξ≥41x²y²/128>0 |

Le [certificat de fuseau](../../morsehgp3D_v7/audits/FRONT_ET_TEMOINS_COURANT.md)
implique alors que z est intérieur à toute boule de la voie possédée par
ab. En particulier z est un témoin universel de la ligne {a}×B. Aucune
régularité du nuage n’est requise pour ce rejet strict. Pour les crédits
de B, inverser la direction e et échanger les facteurs.

## 2. Tubes, projections et suffixes

Utiliser le vecteur entier d=2(c_B−c_A), puis t(p)=d·p et u(p)=d×p.
Une grille de largeur entière positive fixée partage les trois coordonnées
de u(p). Elles appartiennent à un plan, mais trois indices entiers suffisent
à décrire les cellules sans choisir une base orthonormale flottante.
La division en cellules doit suivre la même convention pour les valeurs
négatives ; le modèle emploie le plancher.

Trier les points par (cellule,t,identifiant). Pour chaque cellule C, calculer :

$$Q_C=\sum_{j=1}^{3}\left(\max_{p\in C}u_j(p)-\min_{p\in C}u_j(p)\right)^2.$$

Pour a,z de cette cellule, Q_C majore la norme au carré de d×(z−a).
Avec Δ=t(z)−t(a), les tests de crédit deviennent entièrement entiers :

| Voie | Condition, en plus de Δ>0 |
| --- | --- |
| q2 | Q_C≤9Δ² |
| q3 | Q_C≤Δ² |
| q4 | 16Q_C≤9Δ² |

**L’égalité est permise dans ces trois tests** : la marge stricte vient
de la section 1. Elle ne permet pas de remplacer Δ>0 par Δ≥0. Lorsque
Q_C=0, les sites de même projection restent exclus, dont l’ancre elle-même.

À a fixé, tous les z admis forment un suffixe de la cellule triée.
Le premier indice admissible avance de façon monotone quand a avance :
deux pointeurs suffisent par voie. Le crédit est la taille de ce suffixe,
saturée à need=h_q−h_cœur. Le suffixe désigne une population certifiée ;
il ne faut pas parcourir ses membres uniquement pour les compter.

Le tri, la grille et Q_C sont communs aux trois voies. Leur construction
coûte O(m log m), les trois balayages O(m), la mémoire O(m). Cette borne
inclut la préparation si la largeur est choisie par une règle de coût
constant. Chercher une largeur par tests répétés ou reconstruire plusieurs
grilles doit être compté. Pour plusieurs grilles qui se recouvrent,
prendre le maximum des minorants est sûr ; les additionner ne l’est pas.
Le modèle de comparaison prépare chaque voie séparément ; le C++ partage
désormais effectivement le tri et Q_C entre les trois voies.

Sous u16, avec M=65535, on a |d_i|≤2M, |Δ|≤6M² et Q_C≤48M⁴.
Ainsi 16Q_C et 9Δ² sont inférieurs à 2^74 : i128 suffit pour ces produits,
avec élargissement avant multiplication et soustractions signées.
Le modèle Python utilise des entiers arbitraires ; il ne teste pas un
port i128, ses conversions ou ses éventuels filtres flottants.

## 3. Ce qui est conservé et ce qui reste candidat

Les crédits d’A ne comptent que des sites d’A privés de l’ancre ; ceux
de B sont dans B ; le cœur reste extérieur à A∪B. La somme est donc un
minorant d’identités distinctes pour chaque paire, même si les suffixes
diffèrent entre deux lignes. Une somme atteignant h_q autorise le rejet.
Sinon la paire reste dans le chemin complet de sa voie.

La sélection par classes de crédit peut représenter les sous-produits
résiduels sans les développer pour les compter. Leur consommation paie
ensuite M_res paires, plus les seeds, covers, q3/q4, census et la tour.
Les propriétaires canoniques des supports restent ceux du générateur.
Le census exact ne doit jamais prendre un minorant pour le compte réel
des intérieurs. Une baisse des crédits peut aussi affaiblir les prétests
aval ; compter cette conséquence, pas seulement les paires initiales.

Un exemple favorable explique l’intérêt du suffixe : deux facteurs de m
sites, même tube par facteur, projections espacées assez pour que chaque
successeur passe la borne q4. Le cœur extérieur peut être vide. Les crédits
avant saturation sont alors m−1−i côté A et j côté B. Pour m≥h, le résidu
de la voie contient exactement h(h+1)/2 paires au lieu de m².
Des points (2i,y_i,z_i), y_i,z_i dans {0,1}, et leur copie translatée assez
loin selon x réalisent ce cas en u16 pour des m bornés. Ce résultat porte
sur un rectangle et ses crédits, pas sur la sortie FULL du nuage entier.

Un tube trop large peut rendre tous les crédits nuls ; un tube trop fin
peut isoler chaque point. Les configurations transverses, notamment les
projections égales, échappent à ce certificat même lorsqu’un autre test
q2 trouverait des témoins. Il faut alors changer de certificat ou traiter
le résidu complet. La somme des m sur tous les rectangles peut elle-même
être grande : aucune borne globale ni clôture de P0 n’en découle.

## 4. Autre architecture issue du même certificat : dominance 3D

Cette alternative est démontrée ici mais **non exécutée par le modèle**.
Choisir un cône simplicial rationnel inclus dans le cône q4 de pente 3/4.
Pour d≠0, prendre l maximisant |d_l|, M_d=|d_l|, ε=sign(d_l), et j,k
les deux autres axes. Avec les vecteurs de base e_j :

$$U=M_d e_j-\varepsilon d_j e_l,\quad V=M_d e_k-\varepsilon d_k e_l,\quad g_1=2d+U,\quad g_2=2d+V,\quad g_3=2d-U-V.$$

U et V sont orthogonaux à d ; leurs normes sont au plus celle de d,
et celle de U+V au plus √2 fois celle de d. Les générateurs ont donc une
pente au plus √2/2<3/4. Ils sont indépendants et leur somme vaut 6d.
Pour G ayant ces colonnes, poser T=sign(det G)adj(G).
Alors z−a est dans le cône si et seulement si Tz≥Ta coordonnée par
coordonnée, avec Tz≠Ta pour exclure le vecteur nul.

Un tri lexicographique descendant, puis un arbre de Fenwick à deux
dimensions sur les deux dernières coordonnées, compte ces dominants.
Requêter avant insertion ; grouper les triples entièrement égaux, mais
ne pas exclure les seules égalités d’une coordonnée. La construction des
listes de coordonnées des nœuds et les requêtes donnent O(m log²m) temps
et O(m log m) mémoire. Les requêtes additionnent des populations disjointes :
la saturation à h est sûre, **sans soustraire des préfixes saturés**.

Avec cette construction, sous u16, |G_ij|≤4M_d, |adj(G)_ij|≤32M_d² et
|(Tp)_i|≤96M_d²·65535<2^57. Le déterminant explicite a une valeur absolue
6M_d·|d|²<2^56. Cette formulation évite racines et transformations flottantes ;
son implantation et ses limites de tailles restent à qualifier.

La dominance ne dépend pas d’une largeur de tube, mais son cône est plus
restreint que les régions de témoins possibles et sa résidence est supérieure.
Elle ne domine donc pas automatiquement l’autre stratégie en coût total.

## 5. Un rejet parent à ne pas réemployer aveuglément

« Aucun site de Z n’est universel sur A×B » signifie que chaque site
échoue pour au moins une paire. Cette paire peut disparaître à la scission.
Cela diffère de « tous les sites échouent pour toutes les paires ».

Fixture u16 : A={(4,100,0),(4,102,0)}, B={(4,0,0)}, Z={(3,101,0)}.
Z est extérieur aux boîtes des facteurs ; le parent est séparé à s12.
Le majorant minimax hmax4_boxes vaut −408 et exclut un témoin universel
du parent. Pour l’enfant a=(4,102,0), pourtant, H=100 et Ξ=10404,
donc 2H²−Ξ=9596>0 : ce même site est un témoin q4 strict.

Le transport des **positifs** universels par restriction est sûr avec
leurs identités et exclusions. Une frontière de recherche parent doit
conserver les régions rejetées pour seule absence d’universalité et les
réouvrir si nécessaire. Les suppressions ponctuelles vraiment universelles
peuvent être héritées. Cette distinction concerne B1 et les parcours
conjoints de P0 ; elle n’implique pas une erreur du code v7 qui refait sa
recherche. Payer réouvertures, extractions d’IDs et éventuelles copies.

Le `NoCredit` de `src/spindle/predicates.hpp`, désormais publié,
est plus fort dans la direction A : il fixe un b₀ et majore H sur tout
A×Z. Il reste donc valide en scindant A/Z avec B constant. Il ne se
transporte pas automatiquement en scindant B si b₀ disparaît. La fixture
ci-dessus vise le minimax v7 ; elle ne réfute pas ce nouveau prédicat.

## 6. Portée du modèle et suite utile

Le [modèle exact](p0_tube_probe.py) et ses
[commandes et empreintes](P0_TUBES_CHECKS.json) comparent les crédits à des
juges directs sur petits cas, puis les rejets aux témoins réels des paires.
Les boucles exhaustives servent seulement de juges. Le contrôle q2 après
repli est une filtration de paires diamétrales, pas une tour FULL exécutée.
Les mutations vérifient notamment le rang sans marge et le double crédit.
Les compteurs du modèle ne sont pas des chronométrages C++.

Normal et `-O` : 90 essais de voies sur 30 configurations séparées,
45 comparaisons de permutations, 11 880 identités créditées confrontées
à 95 040 tests géométriques aux coins. Les 30 comparaisons finales q2
emploient un census exhaustif borné. Cinq mutants physiques sont rejetés :
rang sans marge, sortie vide sur indécision, somme de grilles identiques,
auto-témoin et négatif parent réutilisé. Une fixture isotrope conserve
12 vrais témoins W4 manqués par les tubes. Ce résultat négatif est maintenu.
Le sélecteur par classes et la dominance ne sont pas implémentés par ce modèle.

Raccord séparé à la famille de rails de l’auditeur complémentaire :
A={(x,600j,0):0≤x≤150,0≤j≤8}, B=A+(64800,0,0), q4, h=8.
Le modèle, avec son width_factor=4 inchangé, produit neuf cellules de
Q_C=0 par facteur. Ses 2 718 crédits égalent les formules min(8,150−x)
et min(8,x−64800). L’agrégation de leurs classes donne **2 916** paires
résiduelles sur 1 846 881, avec 5 418 comparaisons du balayage.
Ce contrôle exécute le modèle et compare à la formule prouvée ; il ne
développe pas le produit, ne fait pas de census géométrique exhaustif
à 2 718 sites et ne mesure pas de temps C++. Sa commande est conservée
avec les autres contrôles. Le contre-exemple des pools fixes appartient
à l’autre audit ; il n’est pas recopié ni attribué à ce modèle.

```bash
python3 -B morsehgp3D_v8/audits/p0_tube_probe.py --selftest
python3 -B -O morsehgp3D_v8/audits/p0_tube_probe.py --selftest
```

Les comparaisons C++ et leur qualification sont maintenant suivies dans
les documents du constructeur. La dominance reste une alternative non
exécutée ici ; le coût aval et le partage sur toute la WSPD restent ouverts.
Les contre-fixtures et preuves sources sont conservées. GCP non utilisé.
