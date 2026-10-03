# Verrous mathématiques v11 — état courant de la reprise

3 octobre 2026. Contrelecture du moteur **c40f40798** et des mathématiques
courantes ; les réponses initiales Q1–Q5, acceptées le2octobre, sont déjà
portées dans [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) et leurs
[preuves historiques](../receipts/audit_independant_20261002/math_locks_review/README.md).
Cette note remplace mon ancien suivi ; les reçus ne sont pas réécrits.
L’état de qualification et les temps sont dans [l’audit de reprise](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).

## Invariants favorables du moteur actuel

| Objet | Ce que le code doit préserver et préserve dans les chemins relus |
|---|---|
| Population | I strict et U complète ; centre de la MEB interrogé sur l’index global, pas seulement sa feuille d’origine. Saturation autorisée seulement comme certificat de saut, jamais comme population complète. |
| Support | Support MEB local distinct de S* global ; une presentation q4 peut globalement avoir qmin2/3. Minimum d’U obligatoire seulement pour qmin4. |
| MEB | Positivité du support ET inclusion de toute la partie ; premier support arité/lex alors canonique local. Diamètre par distances exactes ; aucune norme carrée de cross en i64. |
| Cellules | Morceaux locaux→classes globales par surjection ; couverture exhaustive des représentants, puis déduplication des racines avant unions. |
| Descente | Rayon strictement décroissant ; terminal dépendant de la politique mais classe correcte à partir de la date initiale. Pas de racine DSU/NodeIdx dans le mémo. |
| Mémo | Clé tuple complet, cardinal, contexte de domaine ; dates initiale et terminale distinctes. Hit de suffixe exige niveau initial strictement inférieur au précédent. Validité fermée a≥λ, ouverte a>λ. |
| Plateaux | Toutes les incidences strictes résolues avant publication ; multifusion n-aire atomique ; continuation sans faux nœud. |
| Verticales | Naissances relevées à leur coupe fermée ; toutes les images d’enfants contrôlées. Balayage croissant et parents immuables ; pas d’anticipation d’un plateau. |
| Numérique | Bornes sur tous les intermédiaires avant annulation ; certificat de puissance distinct d’orientation et limité à son propriétaire. Le hash ne décide jamais l’identité sans égalité exacte. |

Les simplifications déjà implémentées — classification sans traces, singleton,
MEB au premier support/diamètre, q4 différé, mémo avant MEB, balayage et
réemploi vertical — ne sont plus des recommandations nouvelles.
Les ports présents sont jugés par leur propre campagne c40 : 4073/4073 portes et
326 mutants tués. Cette qualification ne découle pas des captures historiques.

## Coupe sûre maintenant portée dans le catalogue

Pour un préfixe S de q sites, chaque dominateur certifié strict dans la boîte
Q est intérieur à toute boule passant par S dont le centre appartient à Q.
L’union D(S) est un minorant sans doublons de p. Toute extension S′⊇S
conserve D(S)⊆D(S′), tandis que θq=K+1−q décroît avec q.

Ainsi d>θq permet de rejeter S avant les droites J2 ; d=θq permet de
traiter S, puis interdit toutes ses extensions en tant que supports
canoniques admis. Une présentation non canonique de qmin inférieur ne
constitue pas une sortie perdue : la boule est visitée par son S*. La coupe n’exige aucune condition d’acuité ou de positivité du préfixe. Les contacts J2 restent testés sur la fermeture, l’owner demi-ouvert
reste séparé, et I/U des boules réellement admises restent entières.
Le traitement arité/lex des supports admis reste identique.

Fixture exacte K5 : A=(0,0,0), B=(4,4,0), C=(4,0,4), D=(0,4,4),
e=(2,1,1), f=(3,1,1), g=(2,2,1), Q=[2,3)×[1,3)×[1,3).
D(A)∪D(B)∪D(C) contient exactement e,f,g. ABC est strict,
c=(8/3,4/3,4/3), β=32/3, p3 : admissible car3+3=6.
ABCD est strict, c=(2,2,2), β12, p3 : inadmissible car3+4>6.
Les ports ef75 appliquent désormais G3 avant les droites et la garde
descendante après le traitement propre du préfixe. Le test de paires garde
sa place. `prefixes` compense les appels évités : compte logique, pas CPU.
Ce témoin local ne mesure pas le gain du générateur LiDAR entier.

## Union directe de racines maintenant portée

Après find(seed) et touch(root), chaque trace possède déjà une racine
valide et touchée. Si first est aussi la racine courante, fusionner ces
deux racines, raccorder leurs listes et rendre leur minimum conserve le
même plateau. Après chaque fusion, **mettre à jour first** : sa racine
antérieure peut devenir enfant d’un indice plus petit. Une simple
suppression des find de unite sans cet invariant serait fausse.

Les chemins de parent compressés peuvent différer, mais classes, racine
minimale, liste des anciens top, enfants triés, compteurs unions/touches
et nœuds publiés doivent rester identiques. Garder les gardes de capacité
et les additions contrôlées ; tester aussi racines déjà fusionnées, graines
dupliquées et plusieurs cellules de même niveau. La réduction d’appels
constatée ne fournit pas à elle seule une réduction de temps FULL.

## PopulationLookup, ordres concurrents et filtres

Si une partie F=I(b)∪U(b) a cardinal k≤K, elle contient S* de b. Toute boule
contenant F doit contenir S*, dont la MEB est b ; b contient F, donc MEB(F)=b.
Puis p<k et t=m : le pas est terminal avec graine(b,k) et dates égales.
L’égalité entière du tuple après hash/tag rend le hit exact, pas probabiliste.
Une population complète est nécessaire : ce lemme ne s’étend pas à un census
saturé ni à une simple coquille. Singleton traité séparément, dates de la
descente entière conservées et décroissance testée avant un retour terminal.

Les clés des tables sont immuables ; leur CAS ne publie qu’un BallIdx dont
la ligne est déjà construite. Les lectures ordinaires suivent la barrière
Pool. Les ordres possèdent leurs DSU ; toutes les résolutions d’un plateau
précèdent sa publication et toutes les forêts précèdent les verticales.
Lecture statique favorable, complétée par la porte TSan21 passée dans auditfix3.

BirthRuns compose tête/queue/maximum et la rupture par une non-naissance
à travers les blocs. Le regroupement garde l’ordre original des rangs.
Le préchargement de jobs futurs est après tests de bornes et ne décide rien.
Les signes natifs census restent les signes des deux sommes i128 certifiées,
avec contrôle de l’ordre des bornes et repli Wide hors certificat.

Le tri F3/F4 a E=6 pour chaque quotient et c=1−2⁻⁴⁰≤(1−2⁻⁵²)¹³.
Les niveaux de catalogue restent normaux et finis ; hors bande la décision
est celle des rationnels, dedans le comparateur exact support/ordinal tranche.
L’ordre total est donc conservé. La preuve aux arrondis n’est pas une porte
native FENV : les portes des quatre modes, modes différents entre préparation
et comparaison, FTZ/DAZ, niveaux égaux non réduits et proches sont maintenant
intégrées et passées en Release18/21/24, ASan24, TSan21 et poison21
dans auditfix3. Elles ne font pas partie du supplément ASan18.

Les deux réserves d’admission/refus sont corrigées dans le code intégré :
les contextes empruntés sont validés avant tout hit, y compris l’appel direct ;
les census possédés sont majorés par `min(count,W−S)`, avec S≤W.
Les nouveaux oracles de propriétaires, sous-ensembles de workers et budgets
serrés passent dans les sept configurations natives jouées par auditfix3 ;
les quatre mutants sont tués par réponse erronée/refus, sans signal ni délai. Aucun mauvais résultat
FULL sur contexte valide n’avait été établi par les deux réserves initiales.

Les identités de ledger restent exactes, mais **`steps` inchangé est conditionnel**.
Avec mémo, une seconde résolution pouvait compter zéro pas ; la table de
populations prioritaire compte un terminal même sur la répétition. Ne pas
annoncer travail identique avec/sans mémo. `region_line_*`, `population_hits`
et compensations de préfixes doivent distinguer travail physique et logique.

## Partage possible des faces régulières

Pour une cellule régulière m=q≤4, p+q=h≤K+1 et les q faces
F_u=I∪(U\{u}) ont h−1≤12 sites. Leur union peut avoir13sites à K12,
mais aucune MEB de cette union n’est nécessaire.

Après les lookups du mémo, préparer les points de I∪U une fois. Un candidat
S strict est calculé une fois. Il certifie la MEB de F_u exactement si
u∉S et tous ses points extérieurs dans I∪U sont contenus dans{u}.
Un extérieur dans I ou deux extérieurs différents l’interdisent à toutes
les faces. L’ordre arité/lex global restreint à une face conserve son premier
support strict contenant et les coefficients exacts de sa présentation.

Garder le diamètre q2 exact propre à chaque face et le mémo avant MEB ;
partager seulement les q3/q4 nécessaires aux faces non résolues. Les dates,
semis, populations et remontées à la composante restent séparés. Les lanes
et lots gardent leur mémoire admise et positions déterministes. Deux
petits modèles Fraction conservent les quatre MEB et leurs supports.
Sur le cas q3, centres stricts5→5 et tests42→47 ; sur le cas q4, centres
q3/q4 stricts21→15 et tests80→82. Les distances partagées40→15 n’en font
pas un chrono. La table de populations remplace déjà beaucoup de terminaux ; ce levier reste expérimental : comparer l’arithmétique
réellement payée, pas seulement le nombre de présentations.

Autre piste distincte : le mémo actuel publie seulement la partie d’origine.
Un staging de suffixes limité et payé pourrait mémoriser certains tuples
réellement visités après succès, avec leurs propres dates. Interdire toute
publication partielle, clé par boule seule, tableau de chemin non borné ou
NodeIdx vivant ; mesurer collisions/évictions et coût réel avant le port.

## FULL → points : expérience close et décision proposée

L'utilisateur remet l'agent côté auditeur et demande les synthétiques et `Zoltan/`.
L'objet de la thèse, définition8 p21, est $E_C(r)=X\cap\delta_r(C)$, pour
une composante C de L_k(r²). Les deux premières parties du manuscrit ont été
relues : le traitement des points frontière impose les couvertures complètes.
La non-percolation du modèle et le vote pondéré après sélection ne prouvent
pas une hiérarchie laminaire de points.

**Règle testée :** garder les couvertures FULL de cardinal≥m, puis prendre
leur fermeture d'équivalence, avec singletons inactifs pour compléter la
partition. Toutes les incidences sont suivies, y compris les continuations
sans nouvelle fusion ; chaque plateau fermé est publié en entier. m est un
seuil de transmission, distinct de `min_cluster_size`. Aucun label ne décide
la construction.

La règle est laminaire en rayon, équivariante et stable à **1ε en rayon** pour
des IDs appariés, à k/m fixés, par le transport P5 des couvertures à r+ε.
Cela ne donne ni borne additive uniforme en rayon carré, ni stabilité d'IoU,
ni garantie sous suppression de points. Des seuils constants ou croissants
avec k conservent le raffinement vertical aux mêmes coupes.

Pour w(i,j), première co-couverture qualifiée, elle rend
$u(i,j)=\min_{\pi:i\leadsto j}\max_{(a,b)\in\pi}w(a,b)$ : la plus grande
ultramétrique dominée par w, donc les réunions les plus tardives compatibles
avec toutes ces échéances. **Cet optimum n'est pas une optimalité statistique.**
Aux mêmes coupes, unir les partitions des k retenus redonne le plus petit k,
les intersecter le plus grand. À des dates différentes, les branches peuvent
encore se croiser. La synthèse de plusieurs k en une seule hiérarchie reste ouverte.

### Ce que les exemples établissent

[Campagne et preuves closes](../receipts/full_points_20261003/README.md) :
**12/12 synthétiques +5/5 scènes Zoltan entières**, k=2/3/5/10,
seuils {3,k+1,20} préenregistrés, **68 fits sklearn HDBSCAN officiel1.7.2**.
Même XYZ u21/grille1mm et IDs pour toutes les méthodes. 40cas/11203contrôles
natifs contre A/B avant la campagne synthétique, puis portes rapides
14cas/4105contrôles au même ELF pour les deux sessions réelles. Moteur **c40**
figé ; sonde **a12f7f54**, reprise depuis25c312ac par restauration de cet ELF,
sans compilation ni qualification du nouveau moteur b872. Tous les arrêts G4
sont ciblés et certifiés. Le délai1050s pendant l'analyse de Zoltan04 est
conservé comme censure ; 04/05 sont ensuite terminés entiers au même binaire.

Score : meilleur IoU parmi les nœuds actifs, puis moyenne par objet suivi.
C'est un diagnostic oracle avec labels, pas une partition automatique.
Les singletons inactifs sont exclus ; HDBSCAN a ses feuilles actives à0.
Les cinq Zoltan viennent de quatre trames d'une seule séquence ; 01/04 sont
la même trame sans/avec sol. Les void participent à la géométrie/transmission
mais sont exclus du dénominateur IoU ; aucun brut KITTI dans le dépôt.
À k=5 :

| Exemples | Core | Premières attaches / LCA | Fermeture m3 | Fermeture m6 | Fermeture m20 | HDBSCAN |
|---|---:|---:|---:|---:|---:|---:|
| Synthétiques, 96 objets | 0,741753 | 0,845813 | 0,817622 | 0,812322 | 0,811243 | 0,774573 |
| Zoltan, 15 objets | 0,576728 | 0,645993 | 0,611804 | 0,636648 | 0,599193 | 0,602980 |

m3 contre HDBSCAN : **63 gains/16 pertes/17 égalités** en synthétique,
**8/3/4** sur Zoltan ; contre premières attaches/LCA : **20/60/16**, **3/9/3**.
Sur les vélos en rang, m3 perd nettement face à LCA/HDBSCAN ; contre la façade,
il dépasse HDBSCAN ; sur le piéton, il l'égale. Aucune supériorité uniforme.
À k10 sur Zoltan, m11=0,616858 dépasse LCA0,613788 et HDBSCAN0,567363,
avec12/0/3 face à HDBSCAN. m11 est exactement le quotient d'ordre11, tandis
que le comparateur préenregistré a `min_samples=10` : cette observation ne
prouve pas un avantage à paramètre effectif identique.
[Synthèse complète](../receipts/full_points_20261003/results/comparison.json),
[scores par objet](../receipts/full_points_20261003/results/objects.csv).
La relecture indépendante recoupe17712gardes et les2553comparaisons.

Les [contre-exemples exacts](../receipts/full_points_20261003/equilateral/README.md)
restent nécessaires malgré les moyennes :

- Deux triangles **exactement équilatéraux dans un plan de R³** : k2/m3
  conserve ABC/DEF, contrairement aux premières attaches irréversibles,
  même avec LCA de tous les ex æquo. La fixture entière pont2000 légèrement
  non équilatérale pouvait masquer ce défaut.
- Sur cinq points, les couvertures {0,1,2} et {0,3,4} réunissent les cinq
  points à β100/9, avant la réunion FULL à β36. Un bloc peut traverser
  plusieurs composantes FULL. Toutes les co-couvertures et la transitivité
  imposent cette percolation ; l'éviter exige de différer une échéance.
- k2/m2 donne la liaison simple à distance2r pour les partitions complétées
  et hauteurs de réunion, mais pas les dates d'entrée. m3 supprime ce pont
  dans les triangles, mais peut aussi éliminer deux paires légitimes isolées.

[Frontières perturbées](../receipts/full_points_20261003/check_jitter_boundary.py) :
8263gardes, dont1344bornes qualifiées sans violation sur32perturbations±1mm
par axe des triangles à échelle1000. ABC/DEF restent conservés ici ; LCA
première couverture k2 change ses branches32/32 fois et IoU31/32. Core
respecte sa borne propre2ε malgré les changements d'IoU. Sur le synthétique
bridge9331, 2048bornes qualifiées passent ; m20 perd légèrement un score
à k5 et un à k10. Aucune stabilité générale d'IoU n'en découle.

### Raccord exact plus direct pour le développeur

Deux identités : **m≤k n'a aucun effet** ; **(k,m=k+1)=(k+1,m=k+1)** pour
blocs actifs, entrées et réunions. Γ_k le prouve par les cofaces des arêtes
d'un arbre couvrant d'un composant à au moins k+1sites ; l'inclusion FULL
inverse donne l'autre sens. Pour m>k+1 l'identité est fausse : 0,2,4 entre
à β1 pour k1/m3, contre β4 pour k3/m3.

Pour k≥2, sites unitaires distincts et m≤k, fermer les populations **complètes
I∪U** des seules boules fortes `p+qmin≤k≤p+|U|` donne exactement le quotient
de points, entrées comprises, **sans calculer les propriétaires FULL**.
Chaque arête Γ_k partage k−1≥1sites ; fermer ses k-parties suffit. Si la MEB
d'une k-partie n'est pas forte, ses points se relient par des k-parties de
rayon strictement inférieur ayant un intérieur commun non vide. Induction
sur leurs niveaux. Cela ne construit pas FULL et ne réhabilite pas le foldv4/E5.

Pour m=k+1, les forts d'ordre k+1 sont **déjà les faibles de Cat_k FULL**,
`p+qmin≤k+1≤p+|U|`. À rayon positif qmin≥2, donc p≤k−1 : ils survivent
au census et aux listes K-certifiées, avec coquille entière. Aucun nouveau
catalogue ni FULL_(k+1) nécessaire. Le
[supplément exact indépendant](../receipts/full_points_20261003/check_strong_projection.py)
passe **1348gardes** en normal/−O sur dix fixtures, dont46égalités de sets
faibles/forts et568coupes/entrées d'ordre suivant. K1/m≤1 reste séparé ;
m>k+1 requiert un traitement supplémentaire des composantes FULL.

Un extracteur peut donc balayer les niveaux rationnels, fermer les populations
admises en DSU et publier les plateaux ; conserver les entrées et IDs canoniques,
les incidences complètes et les contrôles de capacité. Il doit être confronté
à l'export actuel et à l'oracle borné avant tout gain annoncé. Ce lemme réduit
les dépendances du quotient ; il ne borne pas le coût global du catalogue.
Aucun port natif, gain de temps, contrat100ms ou GPU acquis par cet audit.

### Variante de frontière et choix final

[Premières couvertures qualifiées puis LCA](../receipts/full_points_20261003/check_qualified_first.py),
3274gardes bornées normal/−O : attendre cardinal≥m avant d'attacher le point
retrouve les triangles et garde deux branches sur la fixture frontière.
Mais déplacer le point commun (6,2) en (6−δ,2), δ→0+, fait passer sa hauteur
de réunion avec la branche gauche de6 à10/3 : **saut de8/3** pour perturbation
arbitrairement petite. Ce candidat perd donc la continuité/borne1ε ; la
masse exclusivement affectée peut tomber sous m. Pour m≤k, il coïncide avec
LCA ordinaire déjà mesuré. Aucun gain multi-k ou statistique général établi.

**Recommandation :** garder FULL et ses incidences comme référence du modèle,
qualifier le raccord direct forts/faibles pour disposer d'un quotient rapide
et exact, et conserver premières attaches/LCA comme comparateur de pertinence.
La règle finale doit expliciter l'arbitrage entre frontières partagées,
percolation, stabilité et persistance ; aucune des règles testées ne gagne
sur tous ces axes. La synthèse multi-k reste une décision mathématique à traiter,
plutôt que de sélectionner un seuil après lecture des objets.
