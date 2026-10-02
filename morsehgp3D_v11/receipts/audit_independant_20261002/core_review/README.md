# Core v11 WIP : propriété du budget et destructeur du minuteur

2 octobre 2026. Snapshot **non committé** : HEAD de base `52687f8e532db49c9d7334b49111763a72246e1c`, statut initial `?? src/core/`, 19 fichiers figés **avant** toute exécution. Pas de build CMake, campagne, GCP/GPU ni modification produit. Les sondes compilent uniquement ces copies, en normal et UBSan, dans `/tmp`.

## Constat matériel exécuté : StageTimer peut réallouer et terminer

[ledger.hpp](sources/morsehgp3D_v11/src/core/ledger.hpp#L48) promet que l'étage créé à la construction rend le destructeur sans allocation ni exception ; le nom et le Ledger doivent survivre au minuteur. Le minuteur conserve un `string_view`. [ledger.cpp](sources/morsehgp3D_v11/src/core/ledger.cpp#L19) crée une entrée absente par `map.emplace`; son destructeur rappelle `ledger.time(stage, ns)`.

Deux usages respectent les durées de vie publiées mais cassent la promesse :

| Usage pendant que le minuteur est vivant | Allocations dans le destructeur | Faute mémoire injectée après préparation |
| --- | ---: | --- |
| Nom et Ledger stables, témoin de contrôle | 0 | pas exercée |
| Nom `abc` modifié **sur place** en `dbc`, mémoire/view et propriétaires toujours valides | 1 | `std::terminate` |
| Nom littéral stable `abc`, **même Ledger vivant** réaffecté par `ledger = Ledger{}` | 1 | `std::terminate` |

Le premier cas ajoute une clé dbc, alors qu'abc avait été préparée ; le second recrée le nœud supprimé. `Ledger` admet cette affectation publique et aucun interdit supplémentaire n'a été trouvé dans le header ou l'architecture figée. Aucun accès concurrent au Ledger n'est utilisé.

Le destructeur est implicitement `noexcept` : `bad_alloc` y provoque terminate, avant qu'un `guarded` extérieur puisse la transformer en refus. Le test installe un terminate handler qui écrit une ligne constante et termine avec **code 42**, au lieu de faire abort/core dump. C'est l'observation d'une terminaison attendue du contre-exemple, **pas une porte produit conforme ni un simple mutant**, ni une panne réelle de RAM. L'allocateur remplacé ne vit que dans [probe.cpp](probe.cpp) ; aucune injection dans les sources.

Les six usages par mode donnent les mêmes réponses en normal/UBSan, sans diagnostic UBSan. [RUN_normal.json](RUN_normal.json) et [RUN_ubsan.json](RUN_ubsan.json) conservent commandes, codes, sorties et hashes binaires. Ces preuves montrent un défaut **de contrat API de la copie WIP**, pas sa fréquence dans une chaîne v11 finale encore absente.

**Correction utile tôt :** fermer ensemble l'identité du nom et la stabilité du compteur. Copier le nom seul ne résout pas la réaffectation du Ledger. Soit fournir un handle d'étage stable avec sa durée de vie/génération définie, soit déclarer/protéger l'interdiction de modifier le nom ou de remplacer/déplacer le Ledger pendant les timers. Le chemin terminal doit mettre à jour un compteur existant sans allocation ; définir explicitement ce qu'un reset fait des timers actifs. Ajouter ces deux petits contre-cas aux portes avant de compter sur la destruction pendant un refus mémoire.

Hashes des corps jugés : ledger.hpp `a6ffb4f0c08c00ce262d6fb6132a633842e7db9a79c30f69ee03a3376e686c86`, ledger.cpp `5c6556e38e76e75f42c2064ce3d4b1c94c352c556d30679e60a219c5ac2fccbf`.

## Budget/Buffer : acquis et portée exacte

La propriété `shared_ptr<BudgetAccount>` ferme le risque de résultat survivant au MemoryBudget : le petit contrôle écrit/lit le Buffer après destruction du propriétaire, puis le rend. Le déplacement entre deux budgets rend le bloc remplacé au bon compte et transporte la réservation du bloc reçu. Garde du produit `n*sizeof(T)` avant calcul, hypothèse `size_t` 64 bits statique, alignement de T contrôlé ; aucune anomalie nouvelle exécutée sur ces points.

La réserve CAS vérifie `bytes≤limit` et `cur≤limit−bytes` avant addition ; l'allocation refusée rend sa réservation. Le compte est sûr entre **Buffers distincts** ; les mutations d'un même Buffer exigent une propriété unique. Nous n'avons pas rejoué les 8×5000 boucles de la porte développeur : l'intégration et la concurrence sont examinées séparément par le root. Aucun TSan nouveau ici. `peak` est le maximum des **réservations**, même si operator new échoue ensuite ; ce n'est pas le RSS ni le seul payload des allocations réussies.

`allocate` libère d'abord le contenu précédent. Un refus laisse le Buffer vide : c'est documenté et testé, et le contrôle indépendant le confirme. Une croissance/remplacement transactionnel de résultat doit donc passer par un Buffer temporaire puis swap, en comptant ancien+nouveau simultanément ; ne pas lire cette API destructive comme une garantie forte de remplacement.

Le budget compte les Buffer, pas `map`, comptes shared_ptr ou autres allocations externes. La règle « tout grand tableau en Buffer » doit être tenue par les modules ; limiter/budgeter leurs états locaux et les capacités cumulées par worker reste nécessaire. Les identifiants externes PointId et les indices denses restent des domaines distincts : la sentinelle d'indice n'est pas un motif pour rejeter implicitement toute valeur externe u32 maximale ; aucun consommateur cloud v11 n'est jugé ici.

## Évolution live conservée sans requalification

Pendant ces sondes, buffer.hpp/buffer.cpp, types.hpp, status.hpp et reasons.def ont changé. Les 19 copies compilées demeurent intactes. [SOURCE_AFTER.json](SOURCE_AFTER.json) consigne **stable=false** et les nouveaux hashes ; ledger.hpp/cpp restaient identiques à la lecture de fermeture. Les modifications live ajoutent notamment `MemoryBudget::admit` (préflight sans réservation), explicitent la concurrence et retirent allocate_zero. Il n'y a donc aucune demande de rétablir allocate_zero ni de corriger un budget propriétaire déjà partagé.

`admit` doit être interprété avec son pilotage exclusif d'étage/formule complète : ce n'est pas une réservation contre une autre opération concurrente. Son ajout n'est **pas compilé ou requalifié par ce reçu**. De même, les nouvelles raisons/modules et la réduction des types déclarés ne reçoivent pas les résultats du binaire antérieur par héritage.

[judge.py](judge.py), normal/−O, relit les résultats sans nouvelle exécution native. [dependencies.mk](dependencies.mk), versions, source avant/après et [SHA256SUMS](SHA256SUMS) ferment les copies et preuves. La clôture n'affirme pas que l'ensemble live soit resté stable ni publié.
