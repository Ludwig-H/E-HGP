# Note de Claude — S1 à S4 fermés : le sidecar redevient candidat à la réception comme juge borné

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Réponse d'implémentation à
[`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md).
Chaque défaut bloquant est fermé par le mécanisme demandé ; S5 reste une
limite déclarée (juge borné, jamais la route 50 k) et S6/S7 sont actés.

## S1 — le reçu n'est plus forgeable, le fold consomme la frontière

- `HybridSourceReceipt` n'a plus de constructeur public : il exige un
  `SourceProducerToken` privé, détenu par amitié par le seul
  `SealedSourceProducer` (`prototype/sealed_source.hpp`) — le producteur
  TERMINAL qui énumère lui-même (`flat_catalogue`) et scelle le reçu sur SES
  sorties. La forge fraîche sur table amputée est refusée à la compilation ;
  le réemploi reste refusé par les digests (fixture 6, réécrite sur le
  producteur scellé).
- Le pipeline ne fabrique plus de reçu : modes `hybrid`/`hybrid-prefix`
  passent par le producteur scellé, puis la factory, puis
  `build_saturated_fold_hybrid_validated(const ValidatedHybridSidecar&)`
  (`prototype/hybrid_fold_validated.hpp`) — le fold autoritaire reçoit le
  sidecar typé, consomme SES points/catalogue possédés et SES
  `principal_flags()` (le moteur ne recalcule plus les certificats), et
  refuse atomiquement sans fermeture certifiée. L'API brute reste le harnais
  des juges. Le producteur parallèle est refusé explicitement (« le
  producteur scellé v0 est séquentiel »).

## S2 — plus aucun carré hors borne

`ExactBallKey` ne porte plus que le centre rationnel réduit (numérateurs
~90 bits, sûrs en i128) ; l'égalité de NIVEAU est déléguée à
`sphere_cmp_beta`, déjà multiprécision (`BigInt<6>`). L'unicité des handles
compare centre réduit ET niveau exact — les concentriques restent acceptées.
Fixture 11 : la réfutation u16 aux numérateurs géants
`(32767,32767,0),(57863,57862,0),(7672,7673,0),(60104,30135,1)` passe par le
producteur scellé et la factory sans déborder.

## S3 — support propre recalculé, pas recopié

Trois vérifications remplacent la recopie : le CARDINAL déclaré égale celui
du support minimal recalculé indépendamment (q_min recalculé) ; le support
déclaré ENGENDRE exactement la boule déclarée (`miniball_of` + égalité de
boule exacte) ; il est MINIMAL (aucun sous-ensemble propre n'engendre).
L'égalité d'identifiants n'est PAS exigée — deux supports minimaux distincts
d'une boule cosphérique sont tous deux valides, et l'exiger refusait les
vrais catalogues (attrapé par les portes pipeline pendant la correction).
Le champ interne `Sphere.support` doit être cohérent avec `n_support`.
Fixtures : `redundant_cospherical_support` (carré déclaré q=4, refusé par le
cardinal), `sphere_support_field_mismatch`, support non générateur `{0,2}`.

## S4 — sérialisation canonique champ par champ

Le digest ne hache plus l'image mémoire : base (i64), `nx/ny/nz/den` en
deux moitiés 64 bits chacun, champ `support`, rang, offsets, supports,
membres — sans padding ABI ni projection `double beta`. FNV reste déclaré
v0 (liaison, pas preuve) ; le SHA-256 contractuel reste une porte.

## S5 à S7 — actés

- S5 : la factory reste un JUGE BORNÉ (census O(G·n), miniboules par
  générateur) — déclaré, jamais présenté comme la route 50 k. Le chemin
  produit devra consommer des certificats streamés et rejouables.
- S6 : la portée du prune est corrigée dans les documents courants (fait par
  l'audit lui-même) ; le prédicat du code étiquette déjà « branche beta<Q
  vide », jamais « no_support ».
- S7 : « q=2 admissible » est retiré ; aucune lane n'est admise, les masses
  count-only sont un refus de la route combinadique. La prochaine
  proposition de source sera output-sensitive avec son propre préflight
  avant tout portage CUDA.

Tests : porte factory 11 fixtures + 3 mutants tués ; 19/19 sidecar+pipeline ;
région des portes complète verte.

GCP non utilisé.
