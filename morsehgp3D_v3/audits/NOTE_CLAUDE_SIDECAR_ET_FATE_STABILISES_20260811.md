# Note de livraison Claude — sidecar durci et différentiel q2 non compensable

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce commit stabilise l'empreinte du delta audité en live par
[`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md),
[`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) et
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md). C'est une note de
livraison, pas un reçu : la réception appartient aux auditeurs, sur cette
empreinte.

## Sidecar (`validated_hybrid_sidecar.hpp` + `sidecar_sha256.hpp`)

- **Forge fraîche refusée à la COMPILATION** : jeton et reçu non
  trivialement copiables, constructeurs privés possédés par
  `SealedSourceProducer`, `static_assert` permanents dans la porte ; le rejeu
  de l'attaque `std::bit_cast` ne compile plus (`__is_trivially_copyable`
  échoue).
- **Doublon non adjacent `[r1,r2,r1]` refusé** : index trié par
  `(centre réduit, sphere_cmp_beta, indice)` — fixture 12 gravée, `[r1,r2]`
  reste acceptée (fixture 3).
- **Surfaces hostiles refusées AVANT toute arithmétique** : grille u16 sur
  chaque coordonnée du nuage ET chaque `Sphere.base`, bornes
  `|num| < 2^90`, `0 < den < 2^73`, pgcd sur magnitudes non signées. Rejeu
  UBSan des trois attaques (coordonnée INT32_MIN, base INT32_MIN,
  `nx = INT128_MIN`) : trois refus propres, zéro UB, code 0.
- **Tie-break canonique COHÉRENT** : le support canonique est reconstruit
  (coquille du census triée par coordonnées puis miniboule) et toute
  déclaration d'un autre tie-break est REFUSÉE — evidence, digest et fold
  consomment un seul support (solution « rejeter » de l'audit état courant,
  défaut 2). Mutant `skip-canonical-check` tué par la fixture du carré
  ({0,1} canonique acceptée, {2,3} refusée).
- **SHA-256 contractuel** : sérialisation canonique little-endian, sections
  taguées, longueurs préfixées, version de schéma ; selftest FIPS (4
  vecteurs dont le million de « a ») EFFECTIF en tête de `build` et rejoué
  par la porte. Le digest final lie la décision complète : ckey de centre,
  digest des membres, support canonique, `q_min`, état principal, TOUTES les
  `RemovalEvidence`, `maximum_order`, fermetures par ordre, et l'identité du
  producteur certifiant.
- **Reçu avec identité de producteur** : contrat (`FLTC`), profil (`U16Q`),
  schéma de tâches, statut terminal, digest de version de code ; la
  fermeture n'est certifiée que pour ce contrat au statut `kOk`. Le
  déplacement INVALIDE la source (fixture : le pointeur retenu ne certifie
  plus, le reçu vivant certifie encore).
- **Périmètre documenté à jamais** : oracle borné `n<=32`, double
  énumération, jamais un chemin public ni un candidat 50 k (trois fichiers).

## Self-join q2 (`pair_selfjoin_probe.cpp`)

- **Code mort de frontière SUPPRIMÉ** (constat de l'audit delta) ; recherche
  de témoins depuis la racine à SORTIE PRÉCOCE dès le dixième témoin.
- **Différentiel non compensable** : ledger de FATE par paire — partition,
  multiplicité un (détectée au marquage), inclusion exacte des non-inertes
  dans les microtuiles par balayage autoritaire. Les masses terrain 400 sont
  identiques au triplet près à la baseline `8a39c53` (57 985 prunées,
  21 815 microtuiles, 27,34 %, 1 846 états) : la suppression du code mort ne
  change pas les sorts, exactement le constat des auditeurs.
- **Fixtures gravées** : `contact` (dot == 0 par boîtes de largeur nulle et
  extrémités dupliquées), `tenth-witness` (exactement neuf témoins stricts),
  `duplicate` (coordonnées dupliquées), `q2-vs-q3-scope` (les coordonnées de
  l'audit ; inertie exacte imposée à dix, sort non imposé — granularité de
  bloc conservatrice ; assertions q3 : témoins strictement hors du cercle
  ×36, ab plus long côté).
- **Six injections à code exact 4** : `skip-half-block`, `drop-rr`,
  `drop-last-microtile` (identité du ledger), `threshold-nine`,
  `count-shell` (sort par paire), et `duplicate-compensated` — fils dupliqué
  plus fils omis de MÊME masse : l'identité agrégée FERME et seul le sort
  par paire mord (la preuve exécutable que le compte agrégé était
  compensable).
- **Frontière de budget gravée à l'état près** : 1 846 états passent,
  1 845 refusent (code 3). Planchers anti-vacuité `--min-pruned-pairs` /
  `--min-states` (violation = code 1).
- **Compteurs complétés** : tests ponctuels des feuilles, pile témoin
  max, octets de l'arbre, états prunés. Le libellé « chaud » reste un
  chrono local de sonde, jamais un `warm_e2e`.

Suite ciblée : 28/28 (22 self-join + 6 sidecar), suite complète v3 verte au
même worktree (224 tests, les 5 rouges du run précédent étaient un usage
fautif du harnais expect-failure pour des codes 0, corrigé par des doubles
`add_test`).

## Ce que cette livraison ne fait pas

Aucune masse 50 k n'est réduite. Les mesures de familles (porte 5) suivent
sur machine inoccupée avec provenance et sorties brutes versionnées. Le
falsificateur d'ancres q3/q4 suit la porte en cinq points de
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md)
et les corrections de
[`REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md`](REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md)
(q2 = total `|P|`, résiduels incomparables, fixture des neuf points axiaux à
graver, banque `always` des projections nulles).

GCP non utilisé pour cette livraison.
