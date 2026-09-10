# Primitive privée : batch de témoins WSPD

Statut : `phase=exploration_v7_hors_registre`, `backend=stub-hote`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. GCP non utilisé par cet agent. **Compilation et lien NVCC locaux réussis ; aucune exécution GPU, aucun contrat 50k acquis.** Aucun raccord du générateur ni modification de son chemin scalaire actif.

## Résultat retenu

`run_r2/receipt.json` conserve 16 commandes, les sources/dépendances copiées avant compilation, leurs pins avant/après, les sorties brutes et les hashes des binaires locaux. O2 et ASan/UBSan/LeakSanitizer exécutent chacun le même gate stub : **74 313 checks, 73 935 lignes comparées au scalaire, 37 rejets, zéro échec**. Les comparaisons portent sur les trois comptes, les visites DFS et les évaluations de coins, pas seulement sur les décisions de mort.

Les mutants `no-mask` et `reverse-push` produisent respectivement **6 456** et **3 290** différences causales et sont rejetés (code 1), dans les deux builds. Les arguments inconnus donnent exactement 2. Les terminaux WSPD effectivement produits pour s=8,10,12 apportent respectivement 178,185,192 requêtes supplémentaires ; ce n'est ni une comparaison de performances de ces s, ni une qualification de leur générateur.

La compilation et le lien CUDA du gate passent sous NVCC 12.9.86, `sm_120`, O3/NDEBUG, `-fmad=false`, `--expt-relaxed-constexpr` et les options host `-Wall -Wextra -Wpedantic -Werror -pthread`, via le helper strict épinglé. Stderr vide. Le binaire CUDA n'est **jamais exécuté** par le recorder. Les temps locaux de compilation et du stub ne mesurent pas le device.

## Interface et unité de travail

`src/spindle/witness_batch.hpp` est la copie figée du schéma commun fourni par ROOT (SHA `66f31ead8b358dbbb09274b1a0e4fbfcc4777604827527f5c0b2220c1602cd52`). La route `gpu::WitnessRouteContext` expose `prepare(const CloudIndex&, size_t capacity)`, `run(span<const WitnessBatchRequest>, const u64 h[3], u64 generation, vector<WitnessBatchResult>*)`, `close()`, `ready()`, `index_digest()` et `costs()`.

Une requête contient deux NodeRef de sous-arbres disjoints, un masque q2/q3/q4, l'activation des coins et un request_id. **Un thread traite un rectangle entier**, avec CoreBall q3/q4 calculées sur CPU et les mêmes tests entiers, exclusions A∪B, ordre DFS (droite avant gauche), masques hérités et écrêtages que le scalaire. Aucun développement des paires de points, catalogue Gamma, tableau global rectangle×nœud ni structure de candidats supplémentaire.

Le contexte possède une copie immuable de son index host/device. Les NodeRef de l'appel désignent **cet index**, dont `index_digest()` permet au futur raccord de contrôler l'association ; l'API n'accepte pas un deuxième CloudIndex arbitraire. Le raccord doit donc lier explicitement le front à ce digest lors de sa préparation. La mutation/destruction de l'index d'origine ne modifie pas un contexte préparé.

Les request_id doivent être strictement croissants dans chaque appel (y compris aux frontières de chunks), et la génération doit être strictement croissante pendant **toute la durée de vie de l'objet contexte**, même après close/prepare. Une nouvelle génération autorise le début d'un nouveau front avec request_id remis à zéro. Un appel vide consomme sa génération et vide les anciennes sorties sans lancer de kernel. Un échec invalide le contexte et ne publie aucun préfixe, même si un premier chunk avait réussi.

## Domaine et coût déclarés

L'index réutilise le wire existant : coordonnées u16, NodeRef i32, préfixes de multiplicité u32 strictement croissants. La route refuse avant exécution les formes/boîtes/partitions/prefixes incohérents ; la limite u32 de masse provient du wire existant, pas d'un nouveau plafond de candidats. Les tests couvrent les poids dupliqués, les boîtes plates, les extrêmes 0/65535, toutes les valeurs de masque et les seuils nuls, ordinaires et u64 maximaux. Sous u16, H tient sur i64 et Xi/H² sont calculés en i128. La pile de 64 entrées est celle de l'index Morton 48 bits ; une profondeur non traitable produit un statut d'échec transactionnel, jamais une sortie partielle.

Pour m positions uniques, le **payload** de l'index vaut `38*m - 24` octets de chaque côté. La mémoire physique des vecteurs hôtes peut être supérieure à leur taille logique ; `host_index_bytes` déclare ces octets utiles, **pas un pic RSS ni les capacités des vecteurs**. Les buffers device par lot de capacité B occupent 136*B octets. Les miroirs host du lot occupent le même payload ; la sortie transactionnelle privée coûte O(R) résultats pour R requêtes de l'appel, avant publication. Pour conserver O(B) mémoire auxiliaire de résultats, le futur contrôleur appelle la route avec R≤B.

Par ligne effectivement lancée : 72 octets de requête H2D, 64 octets de sentinelles H2D et 64 octets D2H, soit **200 octets explicitement copiés**. Aucun faux async : transfert, lancement, synchronisation et retour sont synchrones. Les arguments de lancement/runtime et le trafic interne du GPU ne figurent pas dans ces octets de memcpy. Les coûts cumulés séparent préparation/validation de l'index, CoreBall, allocation, copies, kernel+synchronisation, validation et libération ; création du runtime/contexte doit encore être incluse dans un futur chronométrage froid externe.

Le travail reste O(m + somme des nœuds/coins visités par les requêtes), mémoire O(m+B+R). **Cette parallélisation ne prouve aucune borne universellement sous-quadratique** et ne change pas le volume de sorties du générateur. Elle n'édite aucun ledger de masse : les certificats rectangle, h_a/h_b, scissions et pair_mass restent la responsabilité du futur contrôleur CPU. Un prochain gate de raccord devra juger l'ordre, la propriété des rectangles, l'absence de pertes/doublons et le ledger, pas seulement cette primitive.

## Rejets et échecs conservés

Les 37 rejets incluent indices/préfixes invalides, masque/NodeRef/plages invalides, IDs dupliqués, générations rejouées, sortie ou seuil nul, 9 points d'échec d'allocation test-only, sentinelle non écrite, ID/génération corrompus au deuxième chunk, refus de pile injecté et index vide. Allocation/libération vérifiées : aucun buffer restant après erreur. Un singleton valide avec appel vide est accepté.

`run_r1/` est **failed** : O2 et SAN passaient, mais le gate NVCC déclenchait `-Wmaybe-uninitialized` dans le scalaire actif `CoreBall balls[3];`. La seule copie privée supplémentaire `src/spindle/witness_count.hpp` remplace cette ligne par `CoreBall balls[3]{};` dans `run_r2`, sans changer les décisions. L'original actif SHA `f20970aa2183f6c0904d640ae5fe072894b6e0b4f7f9440277fbed77e8245803` n'a pas été édité. Les fautes de développement antérieures (signature WSPD, nom de fabrique et initialiseur vide ambigu) ont été corrigées avant la capture retenue ; l'ancien exécutable exploratoire n'est pas sa preuve.

Vérification locale : `python3 build/v7_wspd_device_20260910_r2/verify.py build/v7_wspd_device_20260910_r2/run_r2` ; identique avec `python3 -O`. Les fichiers de la variante sont figés ; ROOT pourra les reprendre pour une session SPOT **séparée** et un gate device avant toute intégration.

Pins retenus : kernel `1e42848c47fdee2cfde043f35343e92eb67fada0d45b8d441cf1760cde3347b4`, route `af208b68c2b2b93ebd13b1fa4064fd3bfd468f4838cd2df8522946dcb5df34c8`, scalaire privé `baa83b05909014b20b01092ab2b7c4e485f0f71e7e7098408dd45ea4ae0d3655`, reçu `5dbbdc628c3cb5fda2cc9adea94b4b3a814b78be2c28199658e8ac2888ff2b1e`.
