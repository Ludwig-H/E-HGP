# Audit du chemin rapide par grille `order_k` — snapshots `08b9ba1` à `4ef89a1`

> [!CAUTION]
> **Verdict : GO pour le lemme local des deux boules, NO-GO pour le parcours exact et pour 50 k.** Le P0 d'état du premier snapshot a été effectivement corrigé et la puissance affine donne un vrai certificat local. Mais le [rapport numérique spécialisé](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md) réfute encore le sur-balayage de la grille, les mesures « ACCORD » ne comparent pas les niveaux, la référence partage les P0 de navigation, les runs G4 à 8 k et 20 k terminent sur un niveau transporté négatif, et aucun résultat 50 k n'était disponible au point de coupe audité.

## 1. Phase, portée et empreintes

Phase annoncée par le header : prototype M3. Backend audité : CPU, prédicats entiers pour les décisions de pinceau, grille uniforme et enveloppes `double` pour la sélection locale. Profils observés : `lidar` et `uniform` synthétiques. Mode : navigation diagnostique directe, sans catalogue, census critique, forêt ni statut public. La porte d'exactitude de cette voie n'est pas satisfaite.

Audit en lecture seule du code produit. Les probes et binaires d'audit sont restés sous `/tmp`. Aucun commit, branche, code produit ou état GCP n'a été modifié par l'auditeur. Les sorties G4 mentionnées plus bas proviennent du journal de développement concurrent et ont seulement été lues.

| objet | SHA-256 ou identité |
|---|---|
| HEAD observé | `5a6cdb1af030a264ce07adddd312be2c458459b4` |
| [`prototype/order_k_bfs.hpp`](../prototype/order_k_bfs.hpp), live audité | `4ef89a194d2adee0e86ddd78cd15caab9af8ec76de8f6d14cca329926f9321a5` |
| snapshot initial grille | `08b9ba1182445d281758e994b4ed7a8e96fa27c4ca3af0cb83021cdbbd62dc2c` |
| snapshot état inter-round corrigé | `d960a7b2cc347e18c031d4f99ed0e99bf1a80b898084180db13baac01a7d8a1c` |
| snapshot union de deux boules | `d4beafa70739a1c11c9fd148d0129730da0913d0bd82a4d78e02ed71d5c10292` |
| oracle rationnel indépendant `/tmp/orderk-grid-audit.awD1l5/independent_oracle.cpp` | `5aed270215598aeb44bc20fce6b0b61648e733afd06afd93400237ad77bae023` |
| probe niveaux `/tmp/audit_fast_reference.cpp` | `7bc65ec84137367ce06896679202bf577763e5f2cbbd74c4d344f66d8bc4c904` |
| probe dégénérescences fast `/tmp/audit_fast_degeneracy.cpp` | `4ebc8a76dac33269f02696ab1098d2ddbdc9a0510e98c021236df572a1219bf4` |
| driver de mesure `fast.cpp` | `74614c90da544ab01e858e351033104f18450cfa5573bd483ee000a025a8152e` |
| [audit numérique du filtre](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md) | `f15d720c46dfafe32afd37c40598b0325e7439a096aa758506317764c9e41a91` |

Le header live était modifié mais non committé. Les verdicts ci-dessous sont donc attachés à son empreinte, pas au HEAD.

## 2. Chronologie : un P0 corrigé, puis une vraie amélioration mathématique

| snapshot | changement | résultat audité |
|---|---|---|
| `08b9ba1...` | première grille, liste `candidates` vidée à chaque doublement | P0 d'état : marques périmées et perte massive de sommets |
| `d960a7b...` | accumulation des candidats et curseur `tested` | correction effective du P0 inter-round ; certification encore par grande boule englobante |
| `d4beafa...` | remplacement par l'union des boules courante et candidate | lemme local valide ; la grille rend encore tout le pavé AABB |
| `4ef89a1...` | filtre `double` par distance à l'intérieur du pavé | moins de candidats, mais conversion AABB indéfinie et preuve d'enveloppe flottante absente |

### 2.1 Le défaut `08b9ba1` est reproduit et corrigé dans les snapshots suivants

Dans `08b9ba1`, `candidates.clear()` était exécuté à chaque tour d'expansion tandis que `mark[z]` restait posé. À la fin de la requête, seuls les candidats du dernier tour étaient démarqués. Tous ceux des tours antérieurs devenaient donc invisibles aux directions et sommets suivants.

Le comparateur sous `/tmp`, sur le tout premier nuage aléatoire `n=12`, plafond 8, rend :

```text
MISMATCH cloud=0 n=12 cap=8 so=0 fo=0 slow=318 fast=2 fallback=16
```

Les diagnostics de développement du même snapshot donnaient également 2 sommets rapides contre 116 680, 258 324, 557 371 et 90 478 selon les cas. Ces « accélérations » étaient exclusivement celles d'un résultat tronqué.

Le delta `d960a7b` déplace `candidates.clear()` hors de la boucle, accumule tous les indices marqués et ne traite que le suffixe nouveau via `tested`. Le live conserve ce schéma et démarque finalement toute la liste accumulée. Aucun état inter-requête périmé n'a été retrouvé sur ce snapshot. Un comparateur fast/slow sur 1 000 nuages simples `n=12` passe après ce correctif, et l'oracle rationnel indépendant passe lui aussi sur 1 000 nuages simples du snapshot `d960a7b`.

## 3. Le certificat local est bon ; sa primitive numérique ne l'est pas encore

Pour un pinceau fixé, la puissance d'un point est affine dans le paramètre. Toute boule intermédiaire est donc incluse dans l'union des deux boules terminales. Une seule collecte exacte de cette union contient tous les concurrents plus proches et leurs ex aequo. Le cap de huit itérations n'est **pas** un finding : il est redondant sous ce certificat, et ne peut pas réparer un sous-balayage.

Le commentaire « aucun point ne change d'état » est toutefois circulaire avant d'avoir trouvé le vrai voisin. La formulation correcte est que tous les points qui changent d'état entre les deux paramètres appartiennent à l'union terminale.

L'[audit numérique dédié](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md) ferme le reste sans duplication ici : conversion hors plage vers `int` avant saturation, sous-balayage d'une coquille u16 à déterminant 1, campagnes positives de la marge sur 200 012 sphères, statut réel du filtre de distance et conception fail-open recommandée. Tant que cette primitive ne rend pas tous les points des deux boules fermées, le lemme ne certifie pas le code.

## 4. La validation « ACCORD » ne compare pas le bon objet

Le driver `fast.cpp` construit deux `std::set<std::vector<i32>>` et compare seulement les coquilles ainsi que le booléen `out_of_domain`. Il ignore `Vertex.level`. Or le rang fermé, l'élagage et la publication dépendent de ce niveau.

La fixture actuelle de témoin coplanaire constant donne, sur `4ef89a1...` :

```text
harness_accord=1 fast_out=1 slow_out=1 fast_level=0 slow_level=0 exact_level=1
```

Ainsi le format même du driver peut écrire `ACCORD=oui` quand fast et slow partagent un niveau faux. Les deux chemins partagent aussi le germe, le transport, la coupe et les prédicats de navigation ; le slow n'est donc pas une autorité indépendante.

Un résultat positif existe et doit être conservé sans être surinterprété : l'oracle sous `/tmp` reconstruit chaque sphère par rationnels `cpp_int`, recalcule exactement coquille et niveau, puis compare la table `(shell, level)`. Sur le live `4ef89a1...`, 2 000 nuages non dégénérés `n=10`, plafond 8 et coordonnées dans `[0,1000]` passent. C'est une bonne qualification du cas simple petit ; elle ne couvre ni la grille u16 extrême, ni les multiplicités, ni les profils à grande échelle.

La porte minimale doit comparer pour chaque sommet : coquille complète, niveau strict, rang fermé, statut de domaine et voisins dans les deux directions. Elle doit avoir une vérité qui n'appelle ni `seed_shell`, ni `in_sphere_side`, ni le transport du sujet.

## 5. Les P0 multiplicité et coupe par rang sont inchangés dans le fast live

Le fast duplique le même parcours et la même transition que le slow après la requête de voisin. Un probe direct du fast `4ef89a1...`, avec census exact `sphere_side` de chaque sommet produit, donne :

```text
coplanar out=1 vertices=1 beyond=0 mismatch=1
cube out=0 vertices=0 beyond=1 mismatch=0
bridge out=0 vertices=7 beyond=0 mismatch=0
constant out=0 vertices=14 beyond=0 mismatch=8
```

Ces lignes recertifient les findings de l'[audit des dégénérescences](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md) sur le chemin rapide courant :

- le germe coplanaire stocke le niveau 0 au lieu de 1 puis tombe sur un niveau négatif ;
- le cube cosphérique est coupé avant navigation parce que sa coquille de taille 8 dépasse le plafond 6 ;
- la fixture pont à coquille 5 reste limitée à la composante de sept sommets sous la coupe par rang fermé ;
- huit des quatorze sommets de la fixture constante perdent des membres permanents de coquille.

Le [rapport sur la coupe par rang](AUDIT_COQUILLES_RANK_CUT_C1548B3.md) reste donc applicable : la connectivité prouvée porte sur le niveau strict, pas sur `shell.size() + level`. La grille accélère la recherche d'un voisin dans la composante parcourue ; elle ne restaure ni les sommets coupés ni les membres constants perdus.

## 6. Les runs G4 live falsifient le claim de montée en charge

Le journal G4 a compilé le header `4ef89a1...` avec `-O3 -march=native`. Le run de contrôle `n=600` annonce 410 184 sommets, 14,00 s contre 78,14 s et `ACCORD=oui`, mais cet accord est seulement celui des ensembles de coquilles décrit ci-dessus.

Quatre runs `n=2000,8000,20000,50000` ont ensuite été lancés **concurremment** par `nohup`, en mode sans référence, plafond direct 11. Au point de coupe du journal, le 9 août 2026 à 03:53:18 UTC après une attente de 120 secondes, les seules sorties disponibles étaient :

| `n` | sommets rendus | temps | candidats/requête | replis exhaustifs | statut driver |
|---:|---:|---:|---:|---:|---|
| 2 000 | 1 477 918 | 76,62 s | 104 | 16 559 | `ok` |
| 8 000 | 703 219 | 65,09 s | 212 | 2 448 | `HORS DOMAINE` |
| 20 000 | 3 376 | 0,29 s | 214 | 1 | `HORS DOMAINE` |
| 50 000 | aucune sortie | aucune mesure | — | — | non obtenu |

Les temps sont contaminés par la concurrence et les deux temps courts correspondent à un abandon précoce, pas à une accélération.

Le libellé `HORS DOMAINE` est techniquement trompeur. Après un germe réussi, la seule affectation `*out_of_domain = true` du fast se trouve dans la branche `level < 0`. Puisque les runs 8 k et 20 k ont déjà rendu un préfixe non vide, `seed_shell` a réussi : ces sorties prouvent un **invariant de transport rompu**, pas un test ni un rejet de `RelevantGP`. Le driver n'exécute aucun validateur de domaine, catalogue ou oracle. Déterminer la transition causale exacte demanderait une trace `(shell, level, direction, tied)` ; le branchement statique suffit en revanche à réfuter l'étiquette actuelle.

Le cas 2 k ne satisfait pas l'objectif dans les conditions observées : 76,62 secondes pour le parcours seul. La concurrence empêche d'en faire une borne de temps isolé, mais il matérialise 739 sommets par point avant tout census de criticité et avant la forêt. Aucun résultat 50 k, encore moins 50 k complet sous une seconde, n'était disponible au point de coupe audité.

## 7. Les diagnostics de performance sous-comptent le travail

Les micro-mesures locales montrent une amélioration réelle du voisinage moyen après `d960a7b`, puis après l'union et le filtre de boule. Elles ne sont pas des reçus isolés : le temps de la même référence `lidar n=200` varie par exemple de 17,51 s à 55,30 s puis 18,10 s entre runs.

| snapshot | `lidar 200` | `lidar 400` | `uniform 300` |
|---|---:|---:|---:|
| `d960a7b` | 15,77 s, 147 cand./req. | 53,54 s, 237 | 11,08 s, 139 |
| `d4beafa` | 17,41 s, 80 | 68,15 s, 110 | 4,18 s, 36 |
| `4ef89a1` | 7,31 s, 34 | 18,08 s, 49 | 4,24 s, 33 |

Le compteur `pencil_candidates` ne compte que les points arrivés jusqu'à `orient != 0` dans `absorb`. Il omet les cellules parcourues, les points rejetés par le test de distance, les recherches dans la coquille, la construction des sphères, les tests du lot final et les opérations de hash. `candidats/requête` n'est donc ni le nombre de points touchés par la grille ni une mesure de coût total.

`level_recomputed` a également changé de sens : son commentaire exige qu'il reste à 1 pour le germe, tandis que le fast l'incrémente à chaque repli exhaustif. Le driver soustrait ensuite 1 et l'affiche comme nombre de replis. Cette réutilisation rend les statistiques slow/fast non comparables et doit être remplacée par un champ dédié.

Enfin, une direction réellement non bornée ne trouve aucun `best`; après 24 dilatations, elle déclenche encore un scan exhaustif des `n` points. Les 16 559 replis du run 2 k montrent que cette branche n'est pas marginale.

## 8. Le fast n'est pas le chemin catalogue et reste global en mémoire

Dans le snapshot audité, `order_k_catalogue` appelle toujours `order_k_vertices`, jamais `order_k_vertices_fast`. Le fast n'est référencé par aucun sujet de l'oracle ni cible CMake : il est exercé par un driver scratch.

Même substitué, le temps mesuré ne comprendrait pas :

- les arités 1 à 3 et leur harvest ;
- le census exact de tous les points pour chaque sphère candidate ;
- la déduplication et le tri exacts du catalogue ;
- la réduction hiérarchique et les forêts Morse HGP 3D ;
- la sérialisation et les reçus de bout en bout.

Le driver passe directement un plafond 11. Le catalogue courant navigue à `s_max + 2`; pour une cible `s_max=11`, son plafond est 13. Les volumes et temps du driver sous-explorent donc même l'intermédiaire prévu par ce catalogue.

L'architecture conserve par ailleurs `seen`, `frontier` et `visited` globaux. Chaque coquille terminée reste au moins dans `seen` **et** dans `visited`, avec `frontier` en high-water temporaire : ce n'est pas le reverse search sans table globale. Son espace est au moins $\Omega(V)$ et une coquille de multiplicité $m$ déclenche toujours $2\binom{m}{3}$ requêtes incidentes avant déduplication. Les 1 477 918 sommets matérialisés à seulement 2 k points confirment que le chemin construit un intermédiaire d'arrangement bien plus gros que la hiérarchie utile.

Conditionnellement, si le ratio observé de 739 sommets par point restait du même ordre, 50 k produirait environ 37 millions de sommets **avant** catalogue et forêts, chacun conservé deux fois au minimum. Ce n'est ni une borne asymptotique ni une prédiction validée ; c'est l'extrapolation qui interdit de revendiquer la faisabilité sans mesurer le high-water mémoire. L'intermédiaire reste contraire à l'invariant v3 d'allégement de `HGP-old`.

## 9. Porte de reprise proposée

1. Appliquer la conception fail-open et les fixtures permanentes du [rapport numérique](AUDIT_FILTRAGE_SPATIAL_NUMERIQUE_ORDER_K_4EF89A1.md).
2. Écrire le lemme de puissance affine comme certificat normatif, puis réduire la boucle d'union à son vrai point fixe démontré.
3. Fournir une borne formelle de sur-enveloppe `double` ou conserver la décision de boule en arithmétique exacte après une broad phase prouvée.
4. Comparer `(shell, level)` à l'oracle rationnel indépendant, y compris limites u16, ex aequo, coplanaires constants et coquilles multiples.
5. Instrumenter toute branche `level < 0` avec la transition complète et remplacer `HORS DOMAINE` par `transport_invariant_failed` tant qu'aucun validateur de domaine n'a parlé.
6. Séparer les compteurs `seed_level_census`, `exhaustive_fallbacks`, `grid_points_touched`, `distance_tests`, `orient_tests` et `exact_side_tests`.
7. Résoudre les P0 de filtration par niveau strict et de transition multiplicitaire avant toute nouvelle mesure de catalogue.
8. Intégrer le fast à un sujet oracle seulement après ces portes, puis mesurer séquentiellement le pipeline complet avec plafond produit, mémoire maximale, hash d'entrée et reçu rejouable.
9. Ne réannoncer 50 k qu'après une sortie 50 k exacte, complète et terminée ; le run sans sortie et les abandons à niveau négatif ne constituent pas une qualification.

GCP non utilisé par l'auditeur.
