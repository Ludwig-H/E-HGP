# Q3/q4 : scission de rectangles sur LiDAR sans sol (shadow, 23 septembre 2026)

**Verdict provisoire : aucun gain moteur démontré.** Une scission exacte d'un facteur des rectangles résiduels de masse ≥16 ferme 15,5 % des paires résiduelles à K5 et 13,4 % à K10 **avant** les tests par paire, mais ajoute beaucoup de visites de l'index des témoins. Toutes ces paires auraient été rejetées par le filtre exact par paire ; la scission ne sauve donc aucun cover, atlas ni candidat aval. Ne pas porter telle quelle.

Entrée réelle : SemanticKITTI 08/000000, masque sans sol figé, grille 1 mm, 39 885 sites, SHA-256 `0baa4de14c95838ef7bd18d5a98551ca513ed830ec1eeee84f649fa97c95abaf`. Copie disponible dans `morsehgp3D_v8/receipts/lidar_ground_20260921/release/ground_fq64xq_6/scene_00_grid/full.u32le`. Même front WSPD `MidpointSamples`, s8, masque 6, filtre de rectangle `Affine` que la chaîne v9 ; chaque rectangle encore ouvert de masse ≥16 est scindé une fois par le facteur de plus grande taille, et ses deux enfants sont filtrés exactement. La source autonome est `q34_block_shadow_20260923.cpp` (SHA-256 `2b11061210e4e94a8a27a39b1169f94fc3c039f38aec87f20e46e929248d3647`). Bibliothèque locale `build/v9-dev/libmhgp9_gen.a` SHA-256 `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a`, sources générateur non modifiées depuis `cc4664e5`. Depuis la worktree v9, compiler avec `g++ -std=c++20 -O3 -pthread -I morsehgp3D_v9/src/gen /workspaces/E-HGP/morsehgp3D_v9/audits/q34_block_shadow_20260923.cpp build/v9-dev/libmhgp9_gen.a -o /tmp/mhgp9_block_shadow` puis appeler le binaire avec le chemin de l'entrée ci-dessus et `5` ou `10`.

| Mesure | K5 | K10 |
|---|---:|---:|
| Rectangles du front | 3 133 819 | 4 782 714 |
| Rectangles ouverts après filtre | 1 128 166 | 2 034 440 |
| Paires résiduelles | 23 686 751 | 30 777 213 |
| Rectangles ouverts de masse ≥16 | 47 043 | 72 329 |
| Masse de ces rectangles | 21 725 441 | 26 870 605 |
| Paires fermées par une scission | 3 678 290 | 4 124 203 |
| Recherches de témoin ajoutées | 94 086 | 144 658 |
| Visites de nœud Z ajoutées | 40 855 197 | 62 541 576 |

Le certificat enfant est le même `filter_q34_witnesses` exact que le parent : seuls les témoins strictement intérieurs universels au sous-produit comptent ; q3 et q4 gardent des seuils séparés. Le découpage partitionne les paires sans hypothèse d'alignement ou d'ordre des IDs. **Aucun bénéfice temporel n'est inféré de ces nombres** : le cache témoin par paire déjà actif dans R7b rejette une grande partie des 21,64 M (K5) / 26,27 M (K10) paires après expansion, et ces recherches supplémentaires peuvent coûter plus qu'elles n'économisent. Une évaluation utile doit apparier les tests de cache/DFS réellement évités, le coût du certificat de bloc et l'ordonnancement, puis vérifier le flux de sorties inchangé sur plusieurs trames et permutations. Une antichaîne de témoins déjà apprise pour la même ancre peut être retestée sur la boîte B sans nouveau DFS Z ; cette variante est distincte et n'est pas qualifiée par le tableau.

Sondage complémentaire K5, toujours hors produit : après le premier `b` de chaque ligne `a×B` des gros rectangles, une antichaîne obtenue par recherche exacte sur ce premier couple et retestée sur B, puis au plus ses deux enfants, fermerait 2 536 849 + 2 059 537 paires pour 1 916 956 + 3 355 988 tests de nœuds cachés. Ces comptes **supposent cette antichaîne disponible** ; le sondage l'a fabriquée par 710 760 recherches sur le premier couple, coût 35 841 478 visites Z, sans simuler l'état partagé réel du cache entre rectangles. Sur un échantillon déterministe d'un gros rectangle sur 32, 156 716 des 777 997 paires seraient sautées ; leur traitement de référence consomme 373 641 tests du cache et 70 562 visites Z, contre 188 232 tests de boîte. L'effet net sur la chaîne reste ouvert. Aucun K10 apparié ni chrono moteur pour cette variante.
