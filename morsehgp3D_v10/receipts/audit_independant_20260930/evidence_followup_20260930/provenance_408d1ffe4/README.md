# Provenance de la reprise — 30 septembre 2026

Lecture de la note au commit `408d1ffe4d90a7ee6393716c6ddc8319b2b77daa`.
Les sept sauvegardes R2 sont présentes : bancs, entrées CLI, faits mathématiques,
oracles, pool, SiteTree et tête. Leurs 726 payloads déclarés sont intacts et
identiques aux originaux dans `/tmp/mhgp10-r2`. Les quatre fichiers supplémentaires
et le journal sont hachés dans ce reçu, sans changer les manifestes d'origine.
Les patchs complets décrivent chacun l'état final depuis la base `56020cab6` ;
les séries n'ont pas toutes la même convention de départ.

Le journal contient trois acceptations avec réserves (bancs, oracles, pool),
un refus SiteTree suivi d'une réparation sans résultat final, trois vérifications
échouées (CLI, tête, faits mathématiques) et une intégration échouée. La cause
« quota » est annoncée dans la note, sans être inscrite dans les événements
`failed` du journal. Le refus SiteTree concerne la preuve du chemin de repli
réellement exécuté ; cette lecture ne démontre aucun nouveau défaut numérique.

Les deux logs de `build/v10-giant-audit` nomment 13 portes distinctes réussies :
11 hors oracles en 44,29 s ; catalogue 162,96 s et tour 182,81 s en parallèle.
Le build a compilé le moteur et les CLI u18, ainsi que les deux helpers autonomes
RankIndex et grid32. Les sources pool, tête, SiteTree, générateur et CLI restent
celles de `777406b82`. Ces treize portes ne qualifient donc pas R2. Elles ne
raccordent pas non plus les primitives u32 au moteur.

Le cache CMake pointe vers le checkout partagé. Les hashes de sources et binaires
observés maintenant sont préservés, mais ne remplacent pas une clôture avant et
après la compilation historique. Le reçu conserve les logs lus, sans relancer
les portes ou une compilation.

Point à traiter au raccord : la garde de nombre de boules du patch CLI R2 ne
protège pas les cardinalités des nœuds et du CSR de la forêt Kruskal. Aucun des
sept patchs R2 ne corrige ces casts ou la réserve `nb+nj` en u32. Le coordinateur
traite ce complément séparément. Les huit fichiers communs aux patchs, dont
CMake partagé par les sept groupes, sont listés dans le reçu.

Aucune note courante ni source produit modifiée. Aucun GPU ou GCP utilisé.
