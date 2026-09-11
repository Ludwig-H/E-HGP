# Dialogue actif avec le constructeur

11 septembre 2026. Priorité utilisateur : les objets permettant de paralléliser
**toutes** les étapes coûteuses de la tour. La [coordination](COORDINATION_AUDITEURS.md)
répartit les écritures.

## Réduction mono directe sur les naissances

La [preuve et le modèle](receipts_birth_stream_20260911/README.md) fournissent
une simplification au brouillon ordonné. À `local0(B)`, φ(T) est déjà connu
par antériorité stricte : poser φ(B)=φ(T), sans union. Pour les occurrences
suivantes, Kruskal reçoit directement φ(B)–φ(Tj). Il retient exactement les
arêtes non pivots du Kruskal ordonné sur hubs après contraction, avec mêmes
dates et ordinaux. La coupe au milieu d’un hub entre deux fenêtres est admise.

Le raccord se place dans `flush`, lors de la consommation originale des
requêtes après leur résolution triée. Un indice dense propre à K permet
l’accès O(1) au DSU sur L ; convertir en identités natives à la frontière
`graph_full`. **Ne jamais écrire `find(φ)` dans φ ni dans les extrémités du
certificat.** La table des naissances reste stable pendant l’usage des indices.
L’écriture des marques O(A), les atlas, semis, fenêtres et sorties demeurent.

Ce chemin évite les recompactions, le DSU des A hubs et leur certificat
intermédiaire. Sur les compteurs n8000 scellés, il demanderait 7 342 931
unions testées ; ce chiffre est déduit, sans temps ou RSS mesuré.
La suite utile est le raccord sur les mêmes terminales réelles, puis les
comparaisons φ/marques/coupes/FULL et une paire mono isolée. Les invariants
et pièges d’API sont détaillés dans le paquet, sans nouvelle demande vague.

## Qualifications précédentes closes

Le [vrai flux par fenêtres](../receipts/streaming_graph_20260911/README.md)
est publié et contre-lu : tableaux globaux requests/targets/graphes supprimés,
travail MEB cumulé correctement contrôlé, 114 census et quatre fenêtres par
build dans les captures constructeur. Le résultat négatif mono n8000 est
conservé. Son arrêt avant extension à n16k/n32k évite de répéter une variante
coûteuse avant correction. Cette qualification ne s’étend pas au brouillon
ordonné ni à la contraction directe proposée ici.

La [compatibilité d’export historique](receipts_historical_export_20260911/README.md)
est prête et contre-vérifiée par le constructeur : banque unique, indices,
niveaux bruts et nœuds reconstruits après les horizontales. Les détails des
deux minima restent dans la preuve, plutôt que répétés comme question ouverte.
Le [premier raccord FULL](../receipts/atlas_graph_full_20260911/README.md) et
les preuves de [composition](receipts_composable_msf_20260911/README.md) et de
[décomposition de la tour](receipts_parallel_objects_20260911/README.md)
conservent leurs autorités propres. Les fenêtres indépendantes hors ordre
restent compatibles avec cette architecture, via leur réducteur composable.

## Acquis repris par le développeur

| Point clos | Preuve et portée conservées |
| --- | --- |
| Prototype MEB initial | [K7 et repli exact](receipts_meb_boundary_20260911/README.md) conservés ; cas de base réparé par le second auditeur, suivi du patch pris en charge dans abc960ac. |
| Lots et portes permanentes | Publication 324f6192, [40 CTests actifs](../receipts/full_ball_batch_active_cmake_20260911/README.md) ; lecteur contre-vérifié normal/−O. Les 65 cas callback comprennent 64 rejets et un cas positif sans requête. |
| Consommation cumulative hôte | [Deux correctifs contre-vérifiés](receipts_batch_work_20260911/README.md) ; [T2 par lots](../receipts/gpu_terminal_batch_t2_20260911/README.md) et [annotations HD](../receipts/gpu_terminal_batch_hd_20260911/README.md) relus sur leurs preuves publiées, sans exécution device par notre lecture. |
| Semis et vrai census→tour K10 | [Semis intégré](../docs/SEMIS_APRES_ECHANGE_20260911.md), [qualification K10](../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md) ; ne plus demander le premier essai ni la porte permanente. |
| Journal incrémental et blocs 50k | [Premier raccord essayé puis non retenu](../receipts/incremental_full_trial_20260911/README.md), [qualification du second auditeur](receipts_cache_commit_20260911/README.md) conservée sur ses propres octets. |

Les [empreintes et lectures](ENTRETIEN.json) distinguent les autorités. Archive industrielle et contrats de temps de toute la tour restent ouverts ; aucun nouveau temps n’est déduit des compteurs locaux. GCP non utilisé par cet audit.
