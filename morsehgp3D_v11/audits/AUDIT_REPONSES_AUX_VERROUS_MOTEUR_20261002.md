# FULL → points : conseil mathématique courant au développeur

3 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Moteurs c40 puis b872 :
[qualification et temps](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
Les réponses Q1–Q5 sont intégrées dans [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) ;
[leurs preuves closes](../receipts/audit_independant_20261002/math_locks_review/README.md)
ne constituent plus des réserves ouvertes. Cette note garde seulement le conseil
courant ; preuves et campagnes antérieures restent dans receipts/Git.

**Le candidat intérieur en rayon est pertinent, avec deux garanties à préciser.**
La règle retenue par le développeur est désormais H^r_(k+1)=P_1∘Π_(k+1),
à k fixé : fidèle à FULL, laminaire et stable à **3ε en entrées et réunions**.
Elle conserve les triangles équilatéraux exacts et traite l'ambiguïté de frontière
sans les fusions anticipées de la fermeture extérieure. Sa qualification change
cependant la garantie de récupération avant fusion parasite ; son optimalité
prouvée porte sur les profils abstraits. Les deux défauts arithmétiques signalés
ont été corrigés dans le WIP, sans qualification native du nouveau consommateur.
La synthèse multi-k et l'optimalité statistique restent ouvertes.

## État relu et corrections nécessaires

WIP développeur e26b48055, document **966af6de**, consommateur rayon corrigé
**58952a8b**. [Sources, preuves et contre-gardes figées](../receipts/hm_followup_20261003/README.md).
Provenance P_κ retrouvée dans la v10 ; marge en rayon carré Q_1 abandonnée.
Les anciennes réserves sur son plafond de racine sont donc retirées du conseil
actif, avec preuve conservée dans [le reçu précédent](../receipts/hm_review_20261003/README.md).

### Qualification : corriger H5, puis graver la fixture de huit sites

H5 affirme que Π_(k+1) ne fait perdre que de petites structures hors de l'amas
principal. **C'est faux : un point cœur d'un amas déjà qualifié peut entrer
après une fusion parasite.** Le vérificateur du workflow l'a réfuté ; notre
oracle Gram/Γ indépendant le confirme en **126 gardes exactes**, normal/−O.

À k2/m3, sites x=(0,0,0), y=(10,0,0), s2=(20,0,0), s3=(30,0,0),
b1=(44,0,0), b2=(54,0,0), w1=(−20,10,0), w2=(−20,−10,0) :

| Quantité pour x | Rayon |
|---|---:|
| Première couverture α ; temps cœur d₂ | 5 ; 10 |
| Première couverture qualifiée t′ | 10 |
| Fusion FULL de {x,y,s2,s3} avec {b1,b2}, F | 12 |
| Naissance du rival qualifié {x,w1,w2} | 25/2 |
| Rencontre avec sa première lignée | √250 |
| Entrée H^r_3 | √250−5/2 ≈13,311 |

Ainsi α+d₂/2=10≤F et x est cœur à F, mais H^r_3 l'exclut encore.
Le rival qualifié naît **après F** ; la qualification agit aussi sur les
ambiguïtés futures. Ce témoin se traduit en coordonnées positives u18, sans
changer sa géométrie. [Vérificateur autonome](../receipts/hm_followup_20261003/check_qualified_delay.py).

La garantie correcte est **e≤t′+d_k/2** et la récupération est assurée si
**t′+d_k/2≤F**, avec t′≤d_(k+1). Le retard reste borné ; la couche frontière
potentiellement perdue s'épaissit. Conserver α, t′, D et e séparément rend ce
coût visible, sans labels. Ce constat ne réfute ni la fidélité ni la stabilité.

### Optimalité : annoncer le cadre intrinsèque, sans transfert géométrique

Le théorème C du workflow est favorable, mais son cadre exact est :
**tous profils abstraits**, règle locale au profil fixé, équivariante par
isomorphisme de profils, entrée immédiate des profils sans rival, constante
Lipschitz intrinsèque c. Il établit c≥3 ; avec monotonie, P_1 est la plus
précoce de constante intrinsèque 3. La preuve et ses entrelacements ont été
relus ; sa propre remarque 3 exclut le transfert automatique de la borne
inférieure aux seuls nuages géométriquement réalisables.

**La borne supérieure géométrique 3ε est prouvée ; sa minimalité géométrique
reste ouverte.** Aucune optimalité parmi toutes les règles décorées ni au sens
statistique n'en découle. Π_(k+1) est un choix de modèle : elle préserve les
triangles, mais supprime les structures de k sites voulues dans d'autres cibles
v10. Aucun seuil d'effectif ne résout seul cette tension.

La preuve 3ε pour les réunions utilise les vraies identités d'entrelacement
φψ=Up(2ε), leur commutation aux remontées et le transport des couvertures
qualifiées. Pour les pendaisons finales g_X,g_Y,
m_Y(φg_Xi,g_Yi)≤e_Xi+3ε ; l'ultramétrie par la chaîne
g_Yi→φg_Xi→φg_Xj→g_Yj donne u_Y(i,j)≤u_X(i,j)+3ε, puis symétrie.
Les cartes conservent les IDs couverts par inclusion ; elles ne conservent
pas nécessairement l'égalité des effectifs.

### Comparaisons corrigées ; qualification exacte encore à compléter

Les défauts de l'ancien consommateur sont **levés sur 58952a8b** :

- Les classes de carrés rationnelles certifient l'égalité des radicaux sans
  factorisation ; une somme non nulle non séparée dans le budget émet un refus.
- Le filtre `cmp_level` borne maintenant l'erreur absolue par les trois racines
  et la cible, ce qui corrige notre témoin d'annulation.

[Contrôle des véritables helpers figés](../receipts/hm_followup_20261003/radicals/check_followup.py) :
**1536 gardes**, normal/−O identiques. Les témoins proches à 2^-9000 vérifient
le refus mais sont hors domaine192bits ; le témoin scalaire d'annulation
respecte192bits/u24, sans fixture Cloud génératrice démontrée.

Deux points utiles avant port natif :

1. Deux dates contiennent **trois racines chacune** ; leur ordre exige jusqu'à
   six radicandes. Seules certaines décisions se réduisent à deux contre deux.
   Définir un type de date de points, distinct de `LevelRank`, avec refus
   transactionnel, ordre exact et plateaux fermés communs à FULL.
2. L'oracle rayon relu calcule les racines à120 chiffres, mais ses additions
   et soustractions utilisent le contexte Decimal global de28 chiffres.
   Sur les triangles exacts, cela peut choisir un propriétaire déjà mort à
   l'égalité. La porte compare des ultramétriques double à tolérance1e−9 :
   elle ne qualifie pas tous les propriétaires aux plateaux exacts.
   Le nouveau WIP ajoute égalité, annulation et3000 comparaisons de six
   radicandes : progrès favorable, mais son oracle Decimal200 garde lui aussi
   les additions hors contexte. Mettre tout le calcul dans le même contexte,
   puis graver l'égalité/plateau et comparer explicitement les propriétaires.

À k1, la décision m=k+1 donne m2, tandis que la garantie liaison simple
annoncée est testée à m1. Pour {0,2}, m2 fait entrer les deux sites à1,
contre0 à m1 : réunions non diagonales identiques, calendrier d'entrée différent.
Prévoir m1 à k1, ou préciser que la règle qualifiée vaut pour k≥2.

La session active **claudepts3 consomme f023f6d0**, antérieur au correctif
58952a8b. Ses résultats doivent porter cette empreinte. Pour qualifier le
correctif, rejouer les décisions sur les exports conservés, vérifier les
sorties de points et les plateaux ; si elles sont identiques, réutiliser les
fits HDBSCAN déjà faits avec leur provenance. Pas de transfert implicite.

## Mesures closes : ce qu'elles établissent

Les campagnes pts1/2 testent le consommateur carré **5df63b60**, distinct du
LIVE0da8 et de la variante rayon. [Recoupe indépendante](../receipts/hm_review_20261003/campaign_review.json) :

- pts1 :128 synthétiques/704 objets, grille isotrope adaptative20–273µm ;
  44 noms LiDAR sont **42 entrées distinctes** après SHA(XYZ,labels), soit
  497 instances,53 sauvetages (objet,k),11 pertes et7 sauvetages forts.
- pts2 :72 trames voisines sélectionnées et20 témoins. Voisines :101 sauvetages,
  23 pertes,24 lignes fortes correspondant à9 couples classe/instance répétés.
- Témoins :zéro sauvetage et zéro perte au seuil1/2 ; moyenne H_m−HDBSCAN
  environ−0,00359/−0,00437/−0,00216/+0,000685 à k2/3/5/10.

Les succès ciblés restent acquis ; doublons et voisins corrélés n'ajoutent
pas d'observations indépendantes. Les scores sont des meilleurs nœuds
disponibles, sans partition sélectionnée ni avantage statistique général.
Aucun contrat100ms, GPU ou massif acquis par ces campagnes.

Aux mêmes(k,m), toute pendaison fidèle qualifiée H vérifie
**u_fermeture≤w_qualifiée≤u_H** : à leur réunion les deux sites sont couverts
par la même composante qualifiée. La fermeture demeure une borne inférieure
canonique utile ; son écart avec H ne mesure pas une erreur statistique.

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

## Raccord multi-k : un croisement exact à traiter

Les règles P_1∘Π_(k+1) ne s'emboîtent pas automatiquement entre ordres.
Pour X={(0,0,0),(3,0,0),(6,0,0),(18,0,0),(19,0,0),(20,0,0)}, à rayon7 :

| Ordre et seuil | Blocs de la règle intérieure |
|---|---|
| k2/m3 | {0,3,6} et {18,19,20} |
| k3/m4 | {6,18,19,20} |

Le dernier bloc traverse les deux précédents : leur réunion comme famille
n'est pas laminaire. [Contrôle indépendant](../receipts/hm_followup_20261003/check_vertical.py) :
55 gardes Fraction/Γ, normal/−O ; entrées exactes k2=(3,3,4,1,1,1),
k3=(9,8,7,7,7,7). Ce constat ne réfute pas la construction à k fixé.

Le théorème D v10 requiert **en plus** l'entrée immédiate en couverture non
ambiguë : fidélité + laminarité + verticalité seules ne sont pas impossibles.
Le témoin ci-dessus étend le conflit à l'unicité **parmi les couvertures
qualifiées** si l'on exige cette entrée immédiate. P_1 abandonne déjà cet axiome.

Piste concrète : transporter les attaches d'un ordre supérieur par les cartes
verticales donne des propriétaires compatibles et fidèles aux ordres inférieurs,
mais leur fait perdre leurs propres entrées précoces. Le critère de choix doit
assumer ce coût. Une compatibilité aux mêmes rayons ne suffit d'ailleurs pas
à rendre laminaire l'ensemble de toutes les coupes aux rayons différents.
Conserver ces croisements comme diagnostics avant toute synthèse ; fermer ou
intersecter aveuglément les partitions redonne les ordres extrêmes ou change
la fidélité, sans fournir un critère statistique.
