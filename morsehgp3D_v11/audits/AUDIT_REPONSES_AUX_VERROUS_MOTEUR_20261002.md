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

## FULL → points : couverture qualifiée et test de pertinence

L'utilisateur remet l'agent côté auditeur et demande les synthétiques et `Zoltan/`.
La cible reste la couverture de la thèse, définition8 p21 :
$E_C(r) = X \cap \delta_r(C)$, pour une composante C de L_k(r²).
Les recouvrements sont permis ; core et une seule première attache ne sont pas cet objet.
Les deux premières parties du manuscrit ont été relues ; la non-percolation du modèle
et le vote pondéré après sélection ne prouvent pas une hiérarchie laminaire de points.

**Proposition testée :** fixer un seuil de transmission m, garder les couvertures
ayant au moins m sites distincts, puis prendre leur fermeture d'équivalence, avec
singletons pour les points sans couverture qualifiée. Balayer toutes les incidences,
y compris les continuations sans nouvelle fusion, et publier le plateau fermé entier.
La règle n'utilise aucun label. m est distinct de `min_cluster_size`.

Cette construction est laminaire en rayon et équivariante. Le transport P5 déjà
prouvé pour FULL conserve les mêmes IDs couverts à r+ε : il donne une stabilité
**1ε en rayon** des dates d'entrée et hauteurs de réunion, à k/m et IDs appariés
fixés. Ce n'est ni une borne additive uniforme en rayon carré, ni une garantie
sous suppression de points ou après sélection d'une partition. Les mêmes seuils,
ou m croissant avec k, conservent le raffinement vertical aux mêmes coupes.

Cela produit une famille par ordre, pas encore une synthèse de toute la tour.
Aux mêmes coupes et sous ce raffinement, unir les partitions des k retenus
redonne le plus petit k ; les intersecter redonne le plus grand. L'union des
branches à des dates différentes peut encore se croiser. Une hiérarchie unique
exploitant réellement plusieurs k exige donc un critère supplémentaire explicite.

Pour w(i,j), première co-couverture qualifiée, la fermeture rend
$u(i,j) = \min_{\pi:i\leadsto j} \max_{(a,b)\in\pi} w(a,b)$.
Elle est la plus grande ultramétrique dominée par w : réunions aussi tardives
que possible sous toutes ces échéances. Cette optimalité limitée n'est pas
une preuve de meilleure pertinence statistique.

Les [preuves indépendantes et contre-exemples](../receipts/full_points_20261003/README.md)
établissent déjà les points suivants :

- K2/m2 est la liaison simple à distance2r ; seuil3 conserve les deux triangles
  pour les ponts2000/1998/1700, mais élimine aussi deux paires légitimes isolées.
- Deux couvertures {0,1,2} et {0,3,4}, sur la fixture planaire à cinq points,
  font fusionner les cinq points à β100/9 ; FULL ne fusionne qu'à β36.
  **Un bloc peut donc traverser plusieurs composantes FULL.** La stabilité
  ne supprime pas cette percolation ; toutes les co-couvertures et la
  transitivité l'imposent. Pour l'éviter, au moins une échéance doit être différée.
- La première couverture irréversible, même par LCA de tous les ex æquo,
  perd des triangles. La fixture entière pont2000 est légèrement non équilatérale
  et peut masquer ce défaut ; une nouvelle fixture exactement équilatérale
  dans un plan de R³ le révèle sur un plateau exact.

L'export d'audit et le consommateur sont isolés dans le reçu, sans port dans
`src/points/`. Campagne préenregistrée : douze synthétiques, cinq scènes `Zoltan/`
entières, k=2/3/5/10, seuils distincts {3,k+1,20}, mêmes sites u21/1mm pour toutes
les méthodes et comparaison à `sklearn.cluster.HDBSCAN` officiel. Validation
native/export et résultats G4 encore à obtenir. Le meilleur IoU d'un nœud est
un diagnostic avec labels sur les groupes actifs (singletons inactifs exclus),
pas une partition automatique ni une validation hors
échantillon ; les scènes Zoltan sont des exemples sélectionnés. HDBSCAN conserve
toutes les feuilles actives à0 ; K2/m2=SL/2 compare les réunions, pas les entrées.

Recommandation provisoire : conserver les incidences dynamiques complètes,
nommer la concession de percolation et juger m par les objets conservés/perdus.
Ne pas consacrer cette proposition comme règle finale avant la campagne.
