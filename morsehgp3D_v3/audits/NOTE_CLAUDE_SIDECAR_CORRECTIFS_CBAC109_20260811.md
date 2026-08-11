# Note de livraison Claude — correctifs sidecar `cbac109`, non reçus

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note conserve la provenance de la livraison
`cbac109a09c2575cdf875b19de1570265bd5bf08`. Le titre initial annonçait
« S1 à S4 fermés »; le contre-audit indépendant a réfuté cette réception. Le
verdict autoritaire et les reproductions sont dans
[`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md).

## Corrections effectivement livrées

- Les modes `hybrid` et `hybrid-prefix` passent par
  `SealedSourceProducer`, construisent un `ValidatedHybridSidecar` et
  transmettent ce type au wrapper de fold.
- La clé de centre ne forme plus le carré du niveau en `i128`; l'égalité de
  niveau appelle le comparateur multiprécision.
- La factory vérifie le cardinal minimal, la boule engendrée, la minimalité du
  support déclaré et la cohérence de `Sphere.support`.
- Le digest du catalogue ne hache plus la structure
  `CriticalSphere` entière ni son `double beta`.
- Les 19 CTests ciblés sidecar/pipeline passent au snapshot livré.

Ces progrès ne suffisent pas à une réception.

## Contre-résultats bloquants

1. `SourceProducerToken` reste vide et trivialement copiable.
   `std::bit_cast` le fabrique en C++20 défini; le constructeur public du
   reçu permet ensuite de sceller un catalogue amputé avec ses propres
   digests. La factory accepte ce reçu et certifie toutes les fermetures.
2. L'index trie par centre puis indice de catalogue et ne compare que les
   voisins. Trois niveaux concentriques ordonnés `[r1,r2,r1]` laissent passer
   deux handles de la même boule exacte.
3. La factory ne refuse pas les champs hors représentation avant de les
   normaliser. `nx=INT128_MIN, den=1` provoque la négation signée interdite
   dans `sidecar_gcd` sous UBSan.
4. La validité et la minimalité géométriques du support sont vérifiées, mais le
   tie-break canonique coordonné n'est pas reconstruit.
5. Les champs numériques restent hachés dans l'ordre d'octets natif avec
   FNV64, sans schéma ni framing contractuel. Le catalogue lie le support
   déclaré et les membres, mais le digest final ne lie pas séparément les
   preuves de suppression calculées, `maximum_order` ou les fermetures.
6. Le pipeline accepté reste borné à `n<=32`, énumère le catalogue deux fois
   et effectue un census `O(G*n)`. Il ne peut pas être la route 50 k.

## Rejeu indépendant

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel --target mhgp3v_sidecar_factory_gate mhgp3v_saturated_pipeline
ctest --test-dir build/v3 --output-on-failure -R 'mhgp3v_(sidecar|saturated_pipeline)'
```

Résultat : 19/19 tests passent en 35,61 s. Les reproductions indépendantes
`fresh_receipt_attack` et `interleaved_duplicate` terminent toutes deux
avec le code zéro, donc acceptent les contres; `hostile_sphere_ubsan` termine
avec le code un après le diagnostic UBSan attendu. Ces trois cas doivent
devenir des fixtures permanentes avant toute nouvelle demande de réception.

Verdict : livraison utile, non reçue même comme juge borné autoritaire. S1, S2
et S4 restent ouverts; S3 est partiellement corrigé.

GCP non utilisé.
