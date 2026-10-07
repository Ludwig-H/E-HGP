# Contre-épreuve cache : réponse développeur du 7 octobre 2026

La porte officielle corrigée `eviction_admise` passe ses **12 contrôles**. Son mutant exact `refus_sans_relecture` est tué par **3 échecs du seul bras 300 + 200 KiB** : refus de la seconde allocation, puis comptes `used` et `held` incomplets. Le bras 300 + 300 KiB passe aussi dans le mutant : il ne suffirait donc pas à prouver la relecture finale. Les six petites sondes ASan/UBSan sur le corps corrigé détectent les deux lectures illégales sous Clang et GCC, avec garde saine.

Capture de travail non commis : `buffer.cpp` SHA-256 `1844a7d6e6d063347ace171be4ca9899669b08b30b2bf2106fa73d9b096c47cd` ; nouveau `alloc_fault.cpp` `1111d433910d1cc1ae05400721a0849eab43cfc58fccd9b752f7e56d6aaf227c`. Les huit empreintes, le commit de base et la mutation exacte figurent dans `capture.json`. Les sources compilées sont reconstruites hors du travail vivant depuis la base et les différences publiées en `91b1a7ee2dae049895eb7889aeaa8262642bf72b`, puis contrôlées par empreinte. Seul le nouveau diff du test est conservé ici ; aucun binaire ni copie complète de source.

| Exécution ciblée | Résultat |
|---|---|
| Porte officielle, GCC 13.3, deux bras | 12 contrôles, 0 échec |
| Même porte, mutant `refus_sans_relecture` | 3 échecs attendus dans `small_arm`, code 1 |
| Clang 18.1.3, `garde` | code 0, aucune alerte |
| Clang 18.1.3, restitution puis lecture / lecture hors taille | chacune SIGABRT et `use-after-poison` |
| GCC 13.3, `garde` | code 0, aucune alerte |
| GCC 13.3, restitution puis lecture / lecture hors taille | chacune SIGABRT et `use-after-poison` |

Le bras 200 KiB évite le passage préalable par `cache_pop` : le second fil peut lire `held` avant d'attendre l'éviction sous mutex. La relecture après acquisition du mutex est alors indispensable. Ce lien causal complète le témoin indépendant déjà publié dans `audit_cache_concurrence_20261007` et couvre maintenant la porte réellement déclarée par le développeur. La sérialisation des restitutions et des évictions, contrôlée dans ce reçu précédent, n'est pas réimplémentée ici.

Les sondes poison ont été **recompilées et rejouées sur `1844a7d6`** ; leur qualification ne provient pas d'un transfert implicite de l'ancien corps `fb3c5239`. Clang emploie son runtime ASan partagé ; LSan est désactivé. Le présent lot exécute une porte, un mutant et six sondes, pas la matrice complète, TSan ou le moteur FULL. Aucun chrono LiDAR, GPU ou GCP. Il soutient la clôture de la réponse cache aux CST-0007/0019 après livraison des mêmes sources ; il ne certifie pas tous les usages concurrents possibles de l'allocateur.

Rejeu depuis la racine du dépôt, avec Git, Python 3, GCC et Clang installés :

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_reponses_20261007/cache/check.py > /tmp/cache-response.json
cmp /tmp/cache-response.json morsehgp3D_v12/receipts/audit_reponses_20261007/cache/result.json
```

La comparaison textuelle suppose les mêmes versions de compilateurs et le même comportement de signal du runtime ; le lecteur vérifie les contrôles, codes et diagnostics avant d'émettre son JSON. Les constructions restent dans un répertoire temporaire supprimé à la fin. Cadre : `exploration_v12_hors_registre`, `cpu_reference`, `quantized_u21_input_only`, `public_status=not_claimed`.
