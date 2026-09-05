# Contrelecture de la qualification constructeur FULL lazy

Observation close le 5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Les nouvelles exécutions constructeur sont closes : **14/14 en Release et 14/14 sous ASan/UBSan**, sur les sources lazy gelées. Les commandes, sorties, dépendances et sceaux ont été contre-vérifiés en lecture seule. Aucun moteur, build, CTest ou contrôleur n’a été relancé par l’auditeur. Le [reçu indépendant](constructor_review.json) distingue ces résultats du précontrôle **12/14 en échec**, conservé intégralement. GCP non utilisé.

## Provenance et fermeture

Le reçu privé constructeur s’étend du 5 septembre, 17:25:34,382601 au 17:29:42,322554 UTC. Le contrôleur est `528175a4fae239aa62630c32c27355be34db1092bef7f8cdb98e589022663bb4` ; son inventaire source est `08dda37e9d9cd5d37ecb3fbda3ca2305d95858f68c742836a5559afdffc5e70e`. Les **582 pins** restent identiques avant/après les deux builds, avant les CTests et lors de la contrelecture : 60 fichiers v7, 521 en-têtes Boost et un depfile de précontrôle utilisé uniquement pour identifier les dépendances Boost.

Les principales sources sont capturées littéralement :

| Source | SHA-256 |
| --- | --- |
| Producteur `full_gabriel.hpp` | `13c6cc72ab5065d498827bf89c6bc2a321b5e896c93a60263de52b9d800a2627` |
| Porte lazy | `6c325c8ba63dd8f2182085e1b3c539842ebbf4849322835b0dd585215a8048b6` |
| Porte allocation lazy | `352b9e423dd290d29f531a030a5353bc2796fa8641ed6c83ed81272b9dfb1de3` |
| Porte digest | `673a5749fd9c35abcca610567554ecfed585298c86cbe7f640f25aa24d718c03` |
| Sérialiseur du bench | `671b2dfb51f1385ee7301bd6b03ef64e62c0d768c92534a6f09589726ce9adc3` |

Le sceau privé couvre **189 fichiers**, dont le manifeste, sans se couvrir lui-même. Tous correspondent aux longueurs et hashes déclarés ; les sources, six binaires par mode et objets de compilation encore disponibles concordent également. Les liaisons de compilation sont identiques après build, avant CTest et après CTest. La [capture portable](constructor_review/capture_manifest.json) conserve 138 pièces statiques utiles, environ 1,58 Mo, avec compression déterministe des grands JSON. Elle comprend les sources nouvelles, le contrôleur archivé en texte, les bruts, commandes, configurations et dépendances. Elle ne contient ni ELF, ni objets compilés, ni copie des 521 headers Boost.

À l’observation de cette revue, aucun nouveau paquet lazy public gelé n’était présent sous `receipts/` ; le reçu privé, déjà clos et scellé, est donc préservé ici. Une publication constructeur ultérieure devra être raccordée à ces octets, sans réécrire cette observation.

## Ce qui a réellement été exécuté

| Phase | Compilation | CTest clos | Temps CTest rapporté |
| --- | --- | --- | --- |
| Release | C++20, `-O3 -DNDEBUG`, warnings stricts | 14/14 ; commande code 0, fin 17:26:40,637702 UTC | 0,70 s |
| SAN | `-O1 -g -DNDEBUG`, `-fsanitize=address,undefined`, frame pointer, sans PIE | 14/14 ; commande code 0, fin 17:29:40,743747 UTC | 3,86 s |

Les deux répertoires sont admis absents par le contrôleur avant création. Configuration et build utilisent CPU0, CTest CPU6, sélection explicite de 14 portes, un job, timeout de 60 secondes par test et limite externe de 120 secondes pour CTest. Aucun test désactivé, ignoré ou déclaré inversé n’est observé. Il s’agit de **14 portes sélectionnées parmi 399 enregistrées**, jamais d’un résultat 399/399.

Les 14 commandes de moteur sont reliées à leurs wrappers : **neuf codes 0 et cinq codes 2 attendus** pour les arguments inconnus. L’inventaire, les commandes dans LastTest et les noms JUnit sont exactement ceux prévus. Les stderr du contrôleur sont vides et les sorties complètes LastTest/CTest concordent entre Release et SAN. Le journal vide créé par l’inventaire est conservé séparément ; le véritable LastTest est différent et postérieur à la borne de fraîcheur sur le même système de fichiers.

Le contexte du contrôleur ROOT est observé avec `TracerPid=0`, `Seccomp=0` et sans override. L’environnement SAN transmis est explicite : `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1`, `UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`. ROOT désigne ce contexte d’agent, pas une preuve d’UID système 0. Ces observations ne constituent pas des captures `/proc` de chaque enfant ni une preuve d’exclusivité du CPU.

**Limite JUnit précise :** treize sorties correspondent intégralement au LastTest. Celle de `mhgp7_full_gabriel_lazy_allocation` est tronquée par CTest à exactement 1 024 octets, suivis du marqueur explicite de troncature. Ce préfixe est exact dans les deux modes. Le LastTest et le stdout verbeux conservent les six cellules et le résumé complet : les 434 fautes sont vérifiées sur ces bruts complets. Cette différence de capture n’est pas une anomalie produit et n’est pas masquée par une fausse égalité XML/log.

## Portée des portes nouvelles

Chaque mode lazy parcourt six nuages globalement réguliers, de un à huit points, soit **27 ordres**, **81 appels lazy nominaux** pour cache 0/1/1 000 000 et neuf permutations supplémentaires. Il juge **3 192 coupes**, 114 records de catalogue et l’égalité littérale eager/lazy. Le juge OBig indépendant vérifie le catalogue exhaustif borné, puis la partition des labels minima et la couverture des points issue de toutes les facettes Gamma, sur les deux côtés des niveaux critiques. Ces coupes ne sont pas seulement des comparaisons de nombres de composantes.

Les sentinelles physiques ferment six cas J1, trois cas de seconde étape, deux cas nommés de hit dans le même lot, quatre hits observés, 65 insertions évitées par saturation et 18 ordres terminaux K=n. Les identités de travail distinguent consultations obligatoires des minima, cache, requêtes portail, MEB et remplacements. Le cache nul reste une exécution complète sans insertion. Le cache grand ne sature pas et retrouve les relations causales attendues avec eager. E5 exerce la normalisation d’ancres historiques et les directes sans fusion. Le mode `--rejects` ajoute **127 refus nommés** : plafonds au niveau mesuré, puis zéro/mesure moins un ; conflit de `max_aliases`, ordres invalides, minima ou terminal absents/futurs, niveau terminal discordant. Chaque refus conserve sa politique lazy et vide le résultat public.

La porte d’allocation utilise E5 et J1 à K2, cache 0/1/grand : **six cellules, 18 positifs et 434 fautes persistantes**, toutes atteintes, aucun échappement. Les compteurs par cellule sont 91/93/93 puis 51/53/53 allocations. Préparation, snapshots et vérifications restent hors de la fenêtre de panne ; seule l’API publique reçoit les fautes. Le résultat refusé est vide, les entrées et l’index restent identiques, les compteurs partiels restent bornés et la dernière panne conserve tous les compteurs du succès. Dans les quatre cellules à cache positif, une admission prospective peut survivre à une insertion qui échoue ; elle n’est pas réinterprétée comme une entrée résidente ni comme un simple cache miss.

La porte digest confronte **672 divisions et réductions** à Boost `cpp_int` — 160 combinaisons fixes et 512 cas déterministes — avec six rejets de domaine. Elle compare les limbs normalisés, puis appelle le selftest sémantique interne du sérialiseur. Son autorité est celle du **sérialiseur de bench** : ni catalogue géométrique indépendant, ni nouvelle mesure de digest d’un grand nuage n’en découle.

Les sept anciennes portes sont réexécutées sur les nouveaux binaires : les résumés structurels 68/218, producteur 67 ordres/1 492 coupes et 80 refus, puis 102 pannes d’allocation eager, conservent leurs périmètres propres. Aucun résultat historique F339 n’est transféré.

## Le négatif de préparation reste utile

Le [précontrôle conservé](constructor_review/development/README.md) contient six commandes et le source fautif `3c9b74bf…`. La sélection étendue termine **12/14, code CTest 8** : les deux modes de la fixture `J1_shared_lot` échouent à l’admission de régularité globale. La boule de diamètre CV au niveau 17/2 porte A et B sur sa coquille en plus du support CV.

La condition d’admission globale est textuellement inchangée dans la porte corrigée. Le candidat fautif devient un **négatif permanent**, observé une fois dans chacun des modes de test. Son remplacement régulier exerce deux directes au même niveau 841 et un vrai hit cache. La correction ferme ce cas sans faire passer rétroactivement le précontrôle ni abaisser l’exigence géométrique.

La qualification reste bornée à ces fixtures et plafonds. Elle ne ferme pas le domaine général K9/K10, une autorité indépendante sur tout catalogue, une suite complète, la CLI, la verticale, les masses, le GPU ou une mesure de performance lazy. Les headers système, bibliothèques et l’OS ne sont pas tous scellés avant compilation ; le reçu revendique honnêtement des builds neufs, sans herméticité. La qualification C++ indépendante de l’auditeur reste une preuve séparée.
