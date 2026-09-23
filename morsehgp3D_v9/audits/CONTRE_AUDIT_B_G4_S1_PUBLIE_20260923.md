# Préflight historique du paquet G4 S1, avant les sessions

23 septembre 2026. Relecture B des commits **`1c9c1e5d7`** (sonde v2)
et **`7565451fc`** (protocole et garde brute), sans session GCP lancée
par B. La sonde vise le **filtre témoin q3/q4** sur les rectangles WSPD
et leurs paires, pas la chaîne HGP/FULL ni le contrat 1 s.

Le problème de couture du WIP est fermé dans le paquet publié : le
bench v2 et les quatre scripts GPU sont des blobs Git du même snapshot.
Le `collect("7565451fc")` strict rend
`protocol_source=commit`, valide **six cas**, et rejette un ancien bench
sans `--inject=pair_mask`. Le protocole a passé localement **10/10
selftests** en Python normal et **10/10** sous `-O` (faux cloud/faux
probe) ; cela vérifie les refus, le cycle de vie et la réception,
**pas** une compilation/exécution CUDA positive. Le préflight réel
en G4 doit obtenir le code 0, puis tuer exactement une paire mutée
avec code 1 et visites inchangées avant toute campagne.
Une reconstruction locale indépendante, CUDA désactivé, a passé le
CTest hôte `mhgp9_gpu_witness_filter_port` du commit publié sur 2 000
sites (**1/1, 18,04 s**) ; il inclut désormais les fixtures de la
garde. Observation locale non archivée comme reçu, sans couverture
device.

Le lecteur suit les codes 0/1/3 du bench v2 sans faux accord vu dans
cette contrelecture ; le code 2 d'entrée refusée ou un JSON malformé
ne peut devenir `complete`. Une exception de la sonde rend code 3
sans JSON et sera classée `probe_failed`, non une mesure `gpu_fault`
structurée : refus sûr, diagnostic moins précis. Les comptes de
population sont auto-déclarés par la sonde ; le mutant teste le
comparateur, pas indépendamment le mapping rang→paire. Les masques
device sont comparés **tous** au CPU par le même binaire.

La garde `FilterInput` publiée refuse maintenant les domaines hors
u18, les partitions d'enfants incohérentes et les boîtes manquant
leurs vrais points ; les fixtures A de faux rejet q4 sont dans la
porte hôte. Le producteur normal utilise `prepare_cloud` avec sites
uniques. Deux réserves restent pour une API brute autonome et le
massif : l'unicité des XYZ n'est pas vérifiée, et le scan de tous les
rangs sous chaque nœud coûte `3Σ|plage(v)|` **avant** les événements
GPU. Une [certification linéaire et possédée](CONTRE_AUDIT_B_VALIDATION_INDEX_GPU_S1_20260923.md)
est proposée ; ces réserves ne bloquent pas à elles seules la première
sonde ~40k issue du producteur certifié.

Pour minimiser le coût SPOT, un **plan à un seul cas**
08/000000/K5/s8/W48 est le premier essai suffisant pour la porte S1.
Le plan par défaut a six cas et peut consacrer jusqu'à 1 500 s utiles,
600 s par cas. Il saute les cinq suivants si le premier est invalide
ou divergent, **mais pas** si ses masques sont exacts avec un temps
GPU `>100 ms` ; dans ce cas, le résultat est un succès d'exactitude
et un échec de la cible de débit, pas un contrat acquis. L'arrêt ciblé
sur la génération certifiée et les deux gardes VM sont en place ; un
fichier de génération malformé peut empêcher l'arrêt immédiat par le
`finally`, laissant les gardes comme recours. Aucun stop non ciblé
ne doit être forcé.

Les chronos à lire séparément sont le premier passage et le meilleur
passage chaud `gpu.total_ms` (transferts device de la passe compris),
le mur du probe, CPU cache ON/OFF, index/front, validation et RSS/HBM.
`gpu.total_ms` exclut la validation, l'index/front, les allocations,
la production et consommation des requêtes, le catalogue et FULL.
Même si la porte de 100 ms est franchie, seule une chaîne complète
appariée sur **trames entières brutes de plusieurs séquences** pourra
qualifier la tour K1..5/K1..10 <1 s, puis 100 ms. À cette lecture :
**aucune mesure G4 GPU ni contrat FULL acquis à ce préflight**.
Depuis, le [contre-audit des deux sessions](CONTRE_AUDIT_B_G4_GPU_S1_SESSIONS_20260923.md)
valide six cas de filtre GPU, dont 08/000000/K5 en 63,801 ms, sans
qualifier la chaîne ni la tour FULL.
