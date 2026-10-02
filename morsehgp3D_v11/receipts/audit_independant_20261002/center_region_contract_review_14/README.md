# CentreRegion — contrat mathématique et raccord catalogue WIP

**Conclusion : pas de défaut mathématique trouvé dans les copies lues.**
La première capture fixe 20 fichiers LIVE WIP, avec 15 versions Git257
séparées ; elle précède la lecture. Une capture tardive distincte fixe neuf
fichiers du raccord catalogue et de ses juges, avec sept versions Git257.
Les listes, horaires et SHA sont dans [SOURCE_BEFORE.json](SOURCE_BEFORE.json)
et [LATE_BEFORE.json](LATE_BEFORE.json) ; le contrôle après lecture figure
dans `SOURCE_AFTER.json`. Ni la qualification MEB meb1, ni les références R2
annoncées dans la provenance ne qualifient ce nouveau port. Aucun code produit,
build, natif ou cloud n'a été exécuté ou modifié par cette revue.

[CenterRegion](source_wip/morsehgp3D_v11/src/num/center_region.hpp) possède ses
bornes et sa factory exige `0≤lo_i<hi_i≤M=2^B`. Il représente la **fermeture**
de la boîte des centres ; hi=M est permis, contrairement à un Point ou à
num::Box. Un contact sur hi doit survivre au filtre, puis la propriété de la
boîte demi-ouverte doit être décidée séparément. Aucun Point de coordonnée M
n'est construit pour ces tests. Des points égaux donnent une bissectrice
égale à tout l'espace ; un triplet aligné est explicitement `degenerate`, pas
une réponse géométrique `disjoint`. Les contrats concernent les profils
entiers 18/21/24, sans epsilon ni transfert au profil float32 ou à un futur u32.

La preuve du [SAT natif](source_wip/morsehgp3D_v11/src/num/center_region.cpp)
est complète pour ce domaine. Posons `u=a−b`, `v=a−c`,
`A=||a||²−||b||²`, `B=||a||²−||c||²`, et `m=(lo+hi)/2`.
Les deux différences de distances au carré sont `A−2u·z` et `B−2v·z`.
Avec `δ=2(z−m)`, `h=hi−lo` et `p=(A−u·(lo+hi),B−v·(lo+hi))`, leur image
est `p−Σ δ_j(u_j,v_j)`, où `−h_j≤δ_j≤h_j`. En rang deux, h_j>0 donne un
zonogone plein. Les normales `(v_k,−u_k)` de ses générateurs non nuls sont
les normales de tous ses côtés ; elles suffisent donc à décider l'appartenance
de l'origine. On obtient exactement
`|v_k p_0−u_k p_1|≤Σ h_j |v_k u_j−u_k v_j|`.
Il n'y a pas de facteur deux manquant et l'égalité conserve les contacts.
Pour la paire seule, l'extremum de la différence affine est atteint à un coin,
donc le test `lower≤0≤upper` est exact.

Les budgets incluent les produits **et les sommes partielles**. Avec les
Point certifiés `0≤coord<M`, on a `|u_i|,|v_i|<M`, `|A|,|B|<3M²`,
`|p_i|<9M²` et `|u_i v_j−u_j v_i|<2M²`. Le membre gauche est <18M³ ; le
droit est <4M³ puisque son terme j=k est nul. `Int<2B+4>` est i64 pour
18/21/24 ; `Int<3B+5>` est i64 en18 et i128 en21/24. Le code élargit avant
les multiplications cubiques, conserve les signes et n'introduit pas de N².
La validation de la région précède toute soustraction de bornes ; les bornes
arbitraires i64 ne sont jamais utilisées dans ces expressions avant refus.

Une garde extrême utile pour la qualification est fournie par
`a=(0,0,0), b=(m,m,0), c=(m,0,m), Q=[0,1]^3`, m=`2^B−1` : un membre gauche
vaut `2m³−2m²`, soit `18446708889358434300` en21 et
`9444730713939644514300` en24. Il exige réellement un produit élargi avant
le calcul. La fixture actuelle à axes simples `(m,0,0),(0,m,0)` a un membre
gauche <m³<2^63 en21 ; elle n'atteste donc pas un dépassement d'i64 en21.
La nouvelle fixture couvre un intermédiaire hors
i64, utile pour une porte de budget ou sanitizer ; elle ne démontre pas la
mort géométrique d'un mutant, qui pourrait encore retourner `disjoint`
correctement par hasard après débordement. C'est une proposition de garde ciblée, pas un
défaut de l'implémentation qui élargit correctement.

Le raccord tardif [leaf.cpp](source_late_wip/morsehgp3D_v11/src/catalogue/leaf.cpp)
a le SHA `9714e2203dd94733fccaa91eac2e82a4d00d8b2c13f4ec176b2be89004e82d89`,
identique à la copie de l'autre auditeur à19:12:24 UTC. Les bits de dominance
stricts correspondent exactement à une bissectrice disjointe de la fermeture
(lignes30–44,49–64). Chaque nouvelle paire puis chaque nouvelle face du
préfixe est testée ; les anciennes ont déjà été traitées. Tout centre d'une
présentation qui appartient à la boîte est équidistant de ses sites : il
appartient donc à ces plans et à ces droites. Leur rejet `disjoint` ne peut
supprimer cette présentation. Le rejet supplémentaire `degenerate` est sûr
**dans ce DFS q3/q4** : trois points alignés ne peuvent appartenir à un
support q3/q4 affinement indépendant ; les boules q2 restent explorées
séparément. Il ne faut pas exporter cette interprétation vers une requête
géométrique générique qui voudrait distinguer tous les lieux dégénérés.

L'émission q3 exige encore un triangle aigu, mais l'échec de cette émission
ne gouverne pas la récursion (lignes80–86,155–186). Une face obtuse peut donc
prolonger un q4 positif ; les tests de droite n'imposent aucun angle. La région
est construite depuis la boîte propriétaire (193–196), alors que l'émission
garde le test de propriété `center_in_box`. Le nouveau rejet ne tronque ni I/U
ni la canonicalisation S* ; celles-ci restent dans leur chemin antérieur.
Cette argumentation garantit la nécessité géométrique des filtres lus,
pas une qualification native du raccord, de ses deux passes ou de la tour FULL.

[check.py](check.py) est autonome : aucune importation de références produit.
Il compare la formule SAT dérivée à la main à une droite rationnelle explicite
intersectée avec les six faces. Ses 864 comparaisons de droites et432 paires
sur trois profils couvrent permutations, contacts, rang dégénéré et extrêmes.
La fixture `(7,4,2),(7,0,1),(7,3,4)` montre que les trois plans peuvent
rencontrer séparément `[0,2]^3` alors que la droite commune la manque ; la
fixture `(2,2,1),(1,2,2),(0,0,1)` conserve le contact `(1,1,1)` de `[0,1]^3`.
Les lectures normales et `-O` passent avec des sorties identiques.

Le juge développeur tardivement copié utilise Gauss2×3 puis un intervalle
paramétrique Fraction, distinct du SAT natif. Il n'a pas été exécuté ici.
Sa dépendance de selftest annoncée peut encore évoluer dans ce chantier WIP ;
aucune qualification n'est tirée d'une simple déclaration de provenance.
Relire `python3 check.py`, `python3 -O check.py`, puis
`sha256sum -c SHA256SUMS`. Le manifeste inclut tout le reçu, à l'exception
du fichier SHA256SUMS à sa racine.
