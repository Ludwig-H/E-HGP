# Préparer une fois le catalogue et lier les semis aux workers

11 septembre 2026, sources publiées sur **ac3e8b9f**. Réponse aux deux
questions du constructeur : retirer les reconstructions redondantes après
validation, et éviter de relire tous les semis à chaque fenêtre parallèle.
Preuves conditionnelles, lecture du C++ et petit modèle exact indépendant ;
aucune intégration ni mesure de gain dans ce paquet.

## 1. Retourner la préparation déjà validée

Le doublon existe dans les sources, pas seulement dans une estimation :

| Étape lue | Travail qui sera refait |
| --- | --- |
| Builder `validate_catalogue`, lignes 442–525, SHA83f1c78e | Profil/identités, contrôles locaux des boules, `by_key`, tri exact stable, programmes par K et ShellTable des extras |
| Sonde `bench.cpp`, lignes 26–30, SHAf702e9dd | Le Builder temporaire et ses métadonnées sont détruits après validation |
| Atlas `prepare`, lignes 160–201, SHA9fc118a2 | Nouveaux tris par clé puis niveau, programmes et ShellTable |
| Flux ordonné, lignes 103–105, SHAfb9f0c0c | Nouveau `by_key` destiné à Geometry |

Le premier raccord sûr est une fabrique renvoyant un **catalogue préparé**,
avec ses métadonnées sous un propriétaire commun. Builder, Atlas et Geometry
consomment ses vues. Extraire cette préparation de Builder évite de garder
ses futures arènes de sortie et ses états de calendrier pour leur seule
validation. Les contrôles géométriques existants peuvent tous rester actifs
une fois dans cette fabrique.

Le plan partagé porte au minimum l’identité de l’index et du stockage BallData,
son ordre original, le profil, K demandé/effectif, `by_key`, l’ordre exact
stable, les admissions et programmes, et les tables locales supplémentaires.
Les rangs et les vues utiles se dérivent de cet ordre une seule fois. Une
permutation inverse de `by_key` peut aussi fournir un rang entier de clé.
La construction, les allocations et les comparaisons exactes initiales
restent comptées ; les réemplois ne sont pas de nouvelles préparations.

Quatre autorités doivent rester distinctes :

- **Validation locale** : domaines, PointId uniques, clés canoniques uniques,
  formes bornées, indices I/U distincts, puissances, support positif donnant
  le même niveau exact, arité minimale et fenêtre de rang.
- **Complétude productrice** : toutes les boules/points nécessaires ont été
  couverts par la chaîne admise de génération et census. La validation des
  enregistrements reçus n’établit pas cette propriété. `census_balls` seul
  ne certifie pas la complétude de ses candidats d’entrée.
- **Liaison à l’owner** : les métadonnées concernent exactement les données
  vivantes et immuables consommées. Ni cardinal égal ni pointeur/taille seuls
  ne certifient l’absence d’une mutation par alias.
- **Exactitude des dérivés** : rangs, programmes et masques correspondent aux
  données liées. La validation structurelle d’une forêt ne les remplace pas.

Une route interne peut consommer un résultat de fabrique fermé aux mutations,
avec construction privée et durée de vie portée par les vues. Un simple
`span<const>` ou `shared_ptr<const>` n’exclut pas un alias mutable préexistant.
L’API empruntée conserve son contrat explicite d’index/catalogue immuables ;
l’entrée extérieure garde ses contrôles. Retirer une vérification géométrique
ultérieure demande de relier sa postcondition à celle du producteur, sur le
même owner et la même génération. Le partage des métadonnées ne nécessite
pas d’attendre cette seconde preuve et n’introduit aucune option de confiance.

Deux détails de représentation sont obligatoires. Les ShellTable trient la
coquille par **PointId** ; Atlas traduit leurs représentants vers les bits
BallData originaux, mais garde `contribution_shell` dans l’ordre PointId.
Réutiliser la table est permis ; réutiliser indistinctement ses masques ne
l’est pas. Leurs permutations doivent suivre le propriétaire. De plus,
`same_exact_level` accepte des fractions brutes équivalentes : garder l’ordre
stable des clés à égalité et les premiers représentants historiques. Le
premier représentant brut global d’un niveau ne remplace pas celui du lot de K requis
par l’[export historique](../receipts_historical_export_20260911/README.md).

## 2. Prouver une fois l’ordre exact, puis comparer des rangs

Soient les représentants $d_1<\cdots<d_m$, vérifiés par comparaison exacte,
et les rangs $r(B)$ tels que $d_{r(B)}=\lambda_B$ pour **chaque** BallId admis.
Alors $\lambda_A<\lambda_B\iff r(A)<r(B)$, et les égalités se correspondent.
Le test actuel de programme est donc exactement remplacé par :

```cpp
previous_rank < rank ||
(previous_rank == rank && previous_key < key)
```

Cette substitution s’applique à `Atlas::validate_shape`, lignes 142–143,
après ses contrôles des représentants stricts (108–112) et de chaque
correspondance exacte rang/niveau (119–120). Elle retire les comparaisons
rationnelles répétées des programmes. Les vérifications d’admission, unicité
et couverture des cellules (133–148) restent nécessaires : le même nombre
de lignes avec un doublon compensant une omission ne suffit pas.

Un rang de clé peut remplacer le second comparateur si `by_key` a lui-même
été certifié comme permutation strictement triée de toutes les clés uniques.
Cela conserve l’ordre par `(niveau,clé)` et ses représentants bruts. Aucun tri
flottant ni ordre par identifiant de stockage ne remplace cette liaison.
Le [modèle rationnel](model.py) confronte l’ordre direct en fractions exactes
à ces rangs et conserve les contre-fixtures de liaison, plateau et omission.

## 3. Lier les semis une fois, sans reparcourir S à chaque fenêtre

L’adapter T2, SHA993786f3, compare à chaque appel le nombre de semis puis
chacune des paires `(facette complète[10],BallId)` à `Context::seeds(K)`.
Sa table propriétaire est déjà reconstruite et triée depuis les populations
complètes I∪U. Pour J_K fenêtres, ce raccord répète J_K·S_K comparaisons.

Faire une liaison exhaustive une fois peut produire un objet opaque
`BoundOrderSeeds`, attaché à l’owner immuable, sa génération et K. Les
résolutions suivantes consomment cet objet et sa vue détenue ; elles ne
reçoivent plus un nouveau span arbitraire à accepter en O(1). Une entrée
extérieure proposant d’autres semis doit repasser par la liaison complète.
Ne pas remplacer l’égalité exacte par une simple égalité de digest.

Chaque BallId donne un semis complet au seul ordre K=|I|+|U|. Si B est le
nombre de boules, $\sum_{K=2}^{K_{\max}}S_K\leq B$. La comparaison de liaison
sur toute la tour devient donc O(B), plus O(ΣJ_K) contrôles des objets liés.
La construction/tri initiale des tables, leurs transferts, les recherches
par facette et les MEB restent payés séparément. Ce résultat ne qualifie
aucun débit GPU et ne transforme pas les semis en preuve de complétude.

**Le Context actuel n’est pas réentrant.** Son backend, ses buffers, son
compteur batch et son état d’échec sont mutables. Le raccord parallèle doit
séparer plan/index/semis en lecture seule, scratch et état backend par worker,
puis orchestration des résultats. L’identité d’une tâche doit inclure son
owner/génération, K, fenêtre et domaine de requêtes ; les compteurs locaux de
workers ne deviennent pas implicitement des identifiants globaux.

Grouper les facettes complètes dans chaque fenêtre, résoudre chaque groupe
indépendamment de φ/DSU, puis disperser les résultats vers des slots distincts.
Consommer dans l’ordre source pour le réducteur mono. Borner **ensemble**
fenêtres en vol et fenêtres terminées mais bloquées derrière une fenêtre
ancienne ; sinon le retard d’un worker peut réintroduire un tampon global.
Les droits d’écriture des slots, l’admission des résultats, les refus et la
fusion exacte du travail payé restent à qualifier dans le raccord.

## 4. La terminale seule ne certifie pas un nouveau seuil de consommateur

La liaison des semis ne permet pas de supprimer la garde géométrique initiale.
`Geometry::static_terminal` vérifie dès sa première MEB que son niveau est
strictement inférieur au seuil `before`, puis descend à niveaux non croissants.
Une terminale mémorisée peut être beaucoup plus basse que cette MEB initiale.

Contre-fixture u16, sur une droite de l’espace : les positions sont 0, 2, 3, 4
(avec y=z=0), K=2 et F={0,4}. Ici les nombres désignent des **coordonnées**,
pas des indices géométriques à passer tels quels au wire. La boule initiale
est centrée en 2, de rayon carré 4, avec deux points intérieurs : son ordre
minimal est 3, donc elle n’est pas une terminale K2. Remplacer le premier
support 0 par l’intrus 2 donne F′={2,4}, centrée en 3, de rayon carré 1.
Elle a un point intérieur et son intervalle d’admission est [2,3] : c’est
une terminale K2.

Un ancien seuil 5 admet cette résolution. Le nouveau seuil 2 vérifie bien
1<2 pour la terminale, mais 4≥2 pour la MEB initiale : l’API doit refuser
cette nouvelle requête. Le [modèle exact](model.py) vérifie les puissances,
les cardinaux et ce refus. Ce témoin concerne une **requête à seuil fourni** ;
il n’est pas présenté comme une occurrence FULL authentique ni comme un
bogue actuellement exécuté par le producteur.

Le mémo sûr garde `(owner,génération,K,F complète,T,before_validé)`. Réemployer
T lorsque `before_nouveau >= before_validé` est suffisant : toutes les gardes
précédentes restent strictes et le chemin géométrique est inchangé. En dessous,
faire un **miss avec repli**, car la requête peut être valide sans que ce
certificat le démontre. Le seuil 9/2 illustre ce cas : la résolution fraîche est valide, mais le
seuil mémorisé 5 ne suffit pas au réemploi direct. Une variante plus précise
garde le niveau initial
certifié λ₀ (rayon carré) et autorise le réemploi dès `λ₀ < before_nouveau`. Le WireResult
courant ne transporte pas cette donnée : ne pas inventer cette autorité.
Une provenance authentique du représentant peut aussi porter la garde initiale,
mais elle doit être explicitement liée à la requête.

Pour dédupliquer un groupe, choisir son vrai minimum de niveau de consommateur.
Le premier ordinal convient seulement si sa monotonie de niveau est déjà
établie. Conserver ensuite les gardes par consommateur lors du scatter.
La complétude des clés, l’identité de l’owner et K restent dans le contrat.

## 5. Portes de raccord et statut

Le constructeur dispose d’un delta vérifiable : conserver les rejets locaux
existants ; comparer préparation ancienne/partagée, terminales et FULL ;
exercer niveaux bruts équivalents, shell réellement permutée et plateaux ;
refuser un owner/index/K différent de même taille ; muter rang, clé à égalité,
programme et traduction des masques. Compter préparation et liaison une fois,
consommations de vues et travail réel des workers séparément. Les anciens
contrats de temps incluent encore les opérations qui seront retirées.

La [voie ordonnée publiée](../../receipts/ordered_streaming_20260911/README.md)
est contre-lue, lecteurs normal/−O PASS : quinze captures dont un refus LSan
conservé, triplet 8k/16k/32k clos. Les retris de hubs sont supprimés ; les
mesures sous charge ne prouvent aucun speedup universel. Le draft dense
b2a472db est favorable en lecture et reste non compilé dans ce paquet.
Les résultats du présent modèle ne lui sont pas attribués.

Le modèle conserve six variantes de catalogues, 84 comparaisons de signes,
18 programmes et cinq mutants causaux. Il accepte une représentation
rationnelle équivalente et vérifie le repli positif à 9/2. Les captures
normal/−O et le lecteur sont séparés de toute exécution du C++ constructeur.

```bash
python3 -B morsehgp3D_v7/audits/receipts_prepared_catalogue_20260911/model.py --selftest
python3 -B -O morsehgp3D_v7/audits/receipts_prepared_catalogue_20260911/model.py --selftest
python3 -B -O morsehgp3D_v7/audits/receipts_prepared_catalogue_20260911/verify.py
```

`public_status=not_claimed`. GCP non utilisé.
