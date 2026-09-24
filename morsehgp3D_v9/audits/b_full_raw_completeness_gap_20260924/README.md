# Trou résiduel entre les épingles CPU et la complétude FULL sur LiDAR brut

Audit B, 24 septembre 2026. Lecture de `origin/main` à `605f39be4` ;
épingles CPU brutes de C publiées au `5102ec2cc` ; **aucun reçu G4 R21**.
Les WIP lus
(`wt-group` `6e5f99263`, `wt-e4` `2ff38b10a`, `wt-pool` `89b977f34`)
changent la tour, son ordonnanceur ou ses portes, pas le générateur q3/q4 ni
les juges ci-dessous. Aucun test lourd, GCP ou modification du moteur pour
cette note.

## Verdict

Les six paires de condensés sur les trois trames **brutes avec sol**, grille
1 mm, K5/K10, établissent une référence CPU reproductible entre moteur et
lots CPU, non une énumération indépendante de toutes les boules admissibles.
Les entrées sont identifiées par les SHA-256 complets dans
[`PINS_RAW.json`](../c_raw_pins_20260924/PINS_RAW.json) : `b00`
`233cc4ea8cac6e0b1155ea845af57b32e5236764bf2e119557aeab5bac76c172`,
`b01` `de45e8dcaf5610cd71a369b613f16914d5713e77cbe1532122ec2e823bc0b4ad`,
`b02` `37a7be399fae909a1291cddfc3ca5b972d3effdfa8dc8cd87a41accd5fa0f5f7`.
Le [reçu C](../c_raw_pins_20260924/README.md) porte 2,75–3,07 M boules
à K5 et 10,7–11,7 M à K10 ; son
[lecteur B](../b_raw_pin_reader_20260924/README.md) vérifie les fichiers,
options, sommes et égalités des deux bras, ainsi qu'Euler seulement jusqu'à
Kmax−2. Ces bras peuvent partager une omission en amont.

Le recensement et la tour contrôlent les **clés présentées** ; l'issue du
produit s'appelle précisément
`complete_relative_to_cross_checked_catalogue`
([`tower_chain.cpp:1403`](../../src/chain/tower_chain.cpp#L1403)). Le
condensé du catalogue encode clé, niveau, arité et **tous** les IDs
d'intérieur/coquille des boules présentes
([`tower_chain.cpp:718`](../../src/chain/tower_chain.cpp#L718)) ; celui de
FULL encode l'objet reconstruit
([`tower_chain.hpp:309`](../../src/chain/tower_chain.hpp#L309)). Une clé
absente des deux bras n'entre dans aucun de ces condensés. Le préflight des
voies q3/q4, lui, réduit temporairement l'identité de coquille à taille,
somme et xor d'empreintes 64 bits
([`wspd_q34.cpp:1202`](../../src/gen/pipeline/wspd_q34.cpp#L1202)) : ce
comparateur n'est pas une comparaison exacte des listes d'IDs. Le condensé
final est plus complet sur les boules présentes, mais reste une épingle.

Les juges q2/q3 réellement indépendants du générateur reconstruisent la
clé, le niveau, l'intérieur et l'**ensemble exact** des IDs de coquille
([`q2_sample_judge.cpp:325`](../../tests/chain/q2_sample_judge.cpp#L325),
[`q3_sample_judge.cpp:618`](../../tests/chain/q3_sample_judge.cpp#L618)).
Ils visent notamment q2 à `p=Kmax−1` et q3 à `p=Kmax−2`, mais les portes
LiDAR enregistrées ne les lancent que sur des coupes **sans sol de 8 000
sites**, non sur les trames brutes de 123–125 k sites
([`CMakeLists.txt:314`](../../CMakeLists.txt#L314)). La porte de clés
absentes couvre nominalement q2–q4, mais seulement des familles synthétiques
2k/8k ; elle réutilise `tower::anchor_meb`, `tower::ball_census` et
`ShellTable`, puis ne compare que la présence de la clé
([`chain_absent_keys_gate.cpp:31`](../../tests/chain/chain_absent_keys_gate.cpp#L31),
[`CMakeLists.txt:365`](../../CMakeLists.txt#L365)). L'oracle exhaustif
q2/q3/q4 et les listes complètes existent sur les petits nuages T2
([`chain_census_tower_gate.cpp:55`](../../tests/chain/chain_census_tower_gate.cpp#L55),
[`chain_census_tower_gate.cpp:120`](../../tests/chain/chain_census_tower_gate.cpp#L120)),
pas sur une trame entière. Les 59 fenêtres LiDAR de 52–140 sites de
[D1](../c_alternatives_20260923/propositions/verifications_adverses.md)
sont un autre contrôle local, pas un census sur le nuage complet.

Cette distinction est causale : pour les boules régulières, Euler ne
contrôle que `K≤Kmax−2` et ne voit pas isolément q2 à `p=Kmax−1` ni q3 à
`p=Kmax−2` ; une omission q4 isolée à `p=Kmax−3` peut être visible, mais
des contributions conjointes peuvent se compenser. L'audit C a effectivement
obtenu, par retraits de clés **déjà émises** à 8k, des tours acceptées dont
le condensé FULL change : 10/68 q2 et 26/67 q3 dans la zone aveugle
([`AUDIT_C_OBJET...:420`](../AUDIT_C_OBJET_ET_RECONSTRUCTION_TOUR_20260923.md#L420)).
Le [contre-exemple exact](../CONTRE_EXEMPLE_EULER_KPLUS2_20260923.md)
montre qu'Euler plus une restriction Kmax+2 clé par clé peuvent eux aussi
laisser une omission **commune**. Aucun de ces résultats ne prouve une
omission du générateur actuel.

## Une porte causale bornée à ajouter

Sur chacun des trois fichiers bruts ci-dessus, fabriquer **depuis les seuls
IDs/coordonnées d'entrée**, jamais depuis les boules émises, une liste
figée de 32 ancres : 16 sites d'une permutation à graine publiée, huit de
huit sites de la queue de densité (plus grande distance au 10e voisin,
départage par ID) et huit sites choisis dans les huit quantiles de distance
au centre entier de la boîte englobante (un minimum de hachage par quantile).
Pour chaque ancre, prendre les 12 plus proches voisins et quatre
partenaires aux rangs 32, 64, 128 et 256 dans l'ordre `(distance², ID)` ;
énumérer les supports contenant l'ancre de tailles 2, 3 et 4. Au plus
`32 × [16 + C(16,2) + C(16,3)] = 22 272` supports par trame, réutilisables
pour K5 et K10. Ce tirage est volontairement stratifié géométriquement,
mais **non conditionné par le catalogue** ; publier IDs, graine et comptes
par strate avant la comparaison. Il ne prétend pas être un échantillon
uniforme de toutes les boules.

Pour chaque support positif, un oracle d'audit distinct calcule centre,
rayon et forme primitive par Gram/Cramer en entiers multiprécision. Un index
spatial propre fait le census sur **tous** les sites de la trame, avec signes
entiers exacts aux feuilles, arrêt possible après 10 intérieurs ; pour une
boule admissible, il collecte néanmoins l'ensemble complet des IDs à
puissance négative ou nulle. Sur une coquille de taille au plus 12, calculer
`q_min` indépendamment par les sous-ensembles positifs de tailles 2–4 ;
une coquille plus grande dans une fenêtre admissible est un échec de domaine,
pas un vert silencieux. Reconstruire et vérifier la bijection ID d'entrée ↔
indice géométrique de la chaîne avant toute comparaison. Exiger pour chaque
clé attendue `p+q_min≤K+1` une unique boule du catalogue avec **même clé,
rayon exact, p, q_min, ensemble entier des IDs intérieurs et de coquille**.
Ne pas utiliser une empreinte somme/xor comme substitut aux IDs.

La porte ne doit être verte que si les six cas ont zéro manquante/écart et
des témoins non vacants publiés : au moins une boule régulière q2 à
`p=K−1`, une q3 à `p=K−2` (dont une longue à arête ≥1 600 mm) et une q4
positive à `p≤K−3` pour **chaque** trame/K. Si le tirage borné manque une
strate, issue `vacuous`, pas `PASS` ; la grille d'ancres doit être figée
avant de juger le produit. Dans des copies du catalogue, retirer une clé
q3 critique et une q4 trouvées par l'oracle, puis substituer un ID de
coquille en gardant sa taille : les trois mutations doivent échouer avec
marqueurs distincts `missing_key` et `shell_ids_diff`. Un retrait q2
critique est un quatrième témoin utile. Le budget oracle est
`O(n log n + B n)` au pire, `B≤22 272` fixé, plus le coût de la chaîne
nécessaire pour obtenir son catalogue ; aucun parcours de tous les couples,
triangles ou tétraèdres du nuage complet. Le census sur le nuage global est
essentiel : un oracle limité à la fenêtre des partenaires accepterait des
boules invalidées par des sites éloignés.

Un PASS serait une recherche adverse indépendante et non vacante sur les
six entrées, **pas** un théorème de complétude des 11 M clés K10, pas une
preuve GPU/G4, et pas une qualification multi-séquence ou float32 brut.
