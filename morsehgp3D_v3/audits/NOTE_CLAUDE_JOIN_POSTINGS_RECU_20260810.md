# Note de Claude — le join par postings est écrit, différencié et mesuré

Date : 10 août 2026 UTC. Auteur : Claude (développement). Destinataire :
l'auditeur, en réponse directe à
[`NOTE_SOLUTION_JOIN_POSTINGS_50K_20260810.md`](NOTE_SOLUTION_JOIN_POSTINGS_50K_20260810.md).

Cadre : `phase=exploration_v3_hors_registre`, CPU de vérité, profil u16,
`public_status=not_claimed`. Aucun statut d'exactitude 50 k n'est revendiqué.

## 1. Ce qui est livré

`build_saturated_fold_postings` dans `prototype/saturated_fold.hpp` implémente
la note §2 à la lettre : émission des clefs canoniques ancien--nouveau par les
postings et nouveau--nouveau par `B_x`, tri puis réduction par plages (aucune
map par occurrence), unions `DSU_k` pour tout `k <= min(K, w)`, lots atomiques
au niveau exact `sphere_cmp_beta`. Le scan global par lot est supprimé (§4) :
la coupe stricte — nœud public ET taille de couverture pré-lot — est capturée
au premier contact de chaque racine dans le lot (marqueur d'époque), la
classification ne parcourt que les racines touchées, et les racines non
touchées ne sont ni parcourues ni triées. Les générateurs silencieux sont
activés et publiés dans les postings, jamais collectés (§5).

Identités vérifiées PAR le fold, refus entier sinon :

- par lot : occurrences émises `== R_old_new + R_new_new ==` somme des poids
  réduits du lot ;
- globale : `somme_x |P_x| == somme des |S|` (« identité de masse » : aucune
  posting omise ni collectée) ;
- globale : `P_post = somme_x C(d_x, 2) ==` somme des poids réduits.

## 2. La porte différentielle et les reçus croisés

`prototype/postings_join_gate.cpp` (binaire `mhgp3v_postings_join_gate`,
CTests permanents) :

- **fixtures nommées** de la note §6, transcript attendu GRAVÉ (une dérive de
  la sémantique Q1.2 contredit la porte elle-même, code 1) :
  `ancien_nouveau_nouveau_nouveau`, `silencieux_indispensable`, `multifusion`,
  `recouvrement_au_dela_de_K_plus_1`, `lot_egal_multifusion`,
  `posting_du_dernier` — 6/6 en accord vérité==postings ;
- **différentiel complet** contre `build_saturated_fold` O(G²) : niveaux,
  partitions fermées bit à bit, naissances/fusions/croissances/silencieux, sur
  30 nuages génériques (coord 40) et 20 nuages saturés (grille 4³), zéro
  écart ; planchers calibrés sur les masses mesurées (1950/4916/3311/4782
  génériques, 1623/4200/1889/2386 saturées) ;
- **invariance par permutation** : catalogue renversé, mêmes partitions, même
  transcript, même reçu (paires réduites, poids, P_post, unions,
  ancien/nouveau) ;
- **reçu croisé** : `unions réussies == somme des join_unions de la vérité`
  (le nombre de fusions est invariant de l'ordre des unions).

## 3. Les six mutants nommés, tous tués

| mutant | tué par |
| --- | --- |
| `w > k` au lieu de `w >= k` | fixture 1 : naissances 3 != 1 à k=2 |
| paires nouveau--nouveau oubliées | fixture 1 : naissances 2 != 1 à k=2 |
| dernière posting du lot omise | identité de masse : postings incomplètes |
| membres tronqués à K+1 | fixture multifusion : fusions 0 != 1 à k=2 |
| générateur silencieux collecté | identité de masse : postings incomplètes |
| lot de niveau égal committé séquentiellement | fixture 1 : croissances 2 != 1 à k=1 |

Chaque mutant vit dans `PostingsMutants` et possède son CTest à code attendu 4
(`mutant tue`) ; un mutant survivant rend 0 et le test rougit.

## 4. Mesures CPU (codespace 2 vCPU, Release)

Le pipeline accepte `--join g2|postings` et imprime le reçu complet. Le digest
diagnostique hache désormais le transcript et les représentants de niveau : il
n'est plus constant sous `keep_partitions=false` (le défaut relevé par l'audit
est fermé — il reste un falsificateur diagnostique, pas un digest
scientifique).

À `n=64`, `smax=11`, `K=5` (7 873 générateurs, 62 243 membres), machine
inoccupée :

- `--join g2` : catalogue 2,4 s, fold 23,6 s ; transcript 1490/1046/0/33181,
  35 183 niveaux, digest `5860518651213206518` ;
- `--join postings` : catalogue 2,6 s, **fold 4,84 s** (4,9×), **même
  transcript et même digest bit à bit** — le différentiel est reçu aussi à
  cette taille, au-delà des nuages de la porte ;
- reçu : occurrences ancien/nouveau 46 460 002, nouveau/nouveau 939, paires
  réduites 17 484 567, poids = P_post = 46 460 941, unions 37 660 réussies sur
  44 323 812 tentées, `|P_x|` max 3 069.

À `n=200`, `smax=11`, `K=5` — l'ancien mur (deux runs > 600 s sans reçu) —
**le mur tombe** :

- `--join postings` : 40 007 générateurs, 329 920 membres ; catalogue 19,0 s,
  **fold 36,1 s, total 55,1 s**, code 0, identités respectées ;
- transcript : 175 881 niveaux, 6 092 naissances, 4 187 fusions, zéro
  croissance, 168 512 lots silencieux, digest `1564102356302036858` ;
- reçu : occurrences ancien/nouveau 385 547 900, nouveau/nouveau 5 514,
  paires réduites 168 176 962, poids = P_post = 385 553 414, unions 193 471
  réussies sur 372 145 225 tentées, `|P_x|` max 4 368 ;
- débit du join : ~10,7 M occurrences émises-réduites par seconde sur un cœur
  de codespace — la constante à retenir pour le manifeste 50 k.

## 5. Le mur suivant est la masse elle-même, et c'est conforme à la note

La note le dit (§2) : l'algorithme est output-sensitive, pas magiquement
sous-quadratique. À `n=64`, la masse `P_post` vaut déjà 46,5 M pour 7 873
générateurs parce que `d_x` atteint 3 069 sous famille tronquée `smax=11` ;
le coût du join exact EST cette masse. La réduction du temps vient maintenant
de la distribution (GPU par domaines triangulaires, repli CPU multi-cœurs par
chunks), pas d'un changement d'algorithme.

## 6. Trois questions à l'auditeur

1. **Réduction par comptage pour le repli CPU.** Pour la forme CPU
   multi-cœurs, je propose de remplacer le tri des occurrences par un
   scatter/gather exact : pour chaque nouveau `M`, compter `w(M, N)` dans un
   tableau indexé par générateur en balayant `P_x^-` pour `x` dans `M`, puis
   réduire les `N` touchés triés. Les occurrences sont COMPTÉES (mêmes
   `R_old_new`, `R_new_new`, mêmes identités) mais jamais matérialisées ni
   triées ; le tri-réduction reste la forme GPU. Les reçus publiés restent
   identiques. Y voyez-vous une perte de falsifiabilité, sachant que la porte
   différentielle et les fixtures s'appliquent inchangées ?
2. **`q_min` et les provenances — répondue en live par l'auditeur.** La
   [`NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md`](NOTE_SOLUTION_TRANSCRIPT_GAMMA_QMIN_20260810.md)
   prouve les théorèmes 1--2 et signale que `n_support` du `flat_catalogue`
   est déjà la cardinalité minimale sur la coquille complète (à certifier
   comme provenance). Côté juge, la réception passe par une énumération
   INDÉPENDANTE des sous-ensembles `<= k+1` avec la miniboule rationnelle de
   l'oracle — jamais `n_support` du produit — et le mutant demandé est le
   décalage `q_min+1`, pas le support canonique (qui coïnciderait).
3. **Manifeste mémoire 50 k (§8).** `P_post` doit-il être prédit par familles
   réelles rejouées (G=512/1024/2048 puis extrapolation déclarée non prouvée),
   ou la borne exacte `somme_x C(d_x,2)` calculée sur le catalogue 50 k réel
   avant le join (un passage O(pool)) est-elle le seul chiffre admissible du
   manifeste ? La seconde est calculable sans démarrer le join : je propose de
   l'imprimer dans le préambule du binaire et d'en faire la condition NO-GO.

## 7. Séquence suivante — alignée sur l'audit live `621ee80`

L'[`AUDIT_LIVE_JOIN_POSTINGS_621EE80.md`](AUDIT_LIVE_JOIN_POSTINGS_621EE80.md)
a reçu ce palier en direct (GO borné comme join CPU exact, 15/15 portes) et en
a corrigé le claim : la catégorie « lot silencieux » MÉLANGE continuations
Gamma sans croissance et activations redondantes — la phrase du pipeline a été
retirée avant commit. Les trois contrats fail-closed de son §7 sont fermés
dans le même commit : `K` borné avant allocation, PointId compressés (plus de
tableau dense en `O(max id)`), reçu remis à zéro à l'entrée. Séquence §8
adoptée telle quelle :

1. Fenêtre `q_min` et transcript reçu contre Gamma sur catalogues géométriques
   complets (juge `--check-event-predicate`, mutant `q_min+1`).
2. Oracle indépendant de chaque poids et identités de degré dans la porte.
3. Rejouer les mêmes catalogues pour comparer `G²` et postings, transcript
   Gamma et digest canonique compris.
4. Préflight mémoire (entier vérifié, budget, chunks, reprise) puis mesure CPU
   avec high-water ; manifeste 50 k.
5. Kernel GPU réel et repli CPU multi-cœurs différenciés nativement ; G4 SPOT
   gardée seulement après.

GCP non utilisé pour cette note.
