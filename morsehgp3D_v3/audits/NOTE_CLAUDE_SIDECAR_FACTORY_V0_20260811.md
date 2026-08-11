# Note de Claude — ValidatedHybridSidecar v0 : la fermeture des carriers devient une capability construite

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_certificate_builder`,
`profile=quantized_u16_input_only`, `mode=typed_trust_boundary_v0`,
`public_status=not_claimed`.

Implémentation de la porte 2 de l'[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md)
selon le
[`contrat du 10 août`](NOTE_CONTRAT_VALIDATED_HYBRID_SIDECAR_20260810.md).

## Ce qui est livré

`prototype/validated_hybrid_sidecar.hpp` + `prototype/sidecar_factory_gate.cpp` :

- **Objet opaque propriétaire** (points et catalogue par déplacement, fenêtre
  TOCTOU fermée par le type), constructeur privé, factory seule.
- **`ExactBallKey` centre + rayon réduits** : les concentriques de rayons
  distincts restent acceptées (fixture 3) ; l'index de boules est INJECTIF et
  vérifié à la construction — deux handles de la même boule exacte refusent
  (fixture 1).
- **Les dix validations atomiques du contrat** : digests recalculés, pool
  sans chevauchement/trou/reste, membres et supports triés/inclus, `den>0`,
  support SUR la coquille (`sphere_side == 0` par point), **saturation fermée
  complète par census exact du nuage** (fixture 2 : les colinéaires aux
  membres censurés refusent), **miniboule des membres égale à la boule
  déclarée**, ordre canonique et lots par `sphere_cmp_beta`, certificats
  principaux par témoins de suppression, fermeture depuis le reçu, index et
  digest final seulement après succès.
- **Certificats principaux par `RemovalEvidence`** indexés par le PointId
  supprimé : strict pour tous les u → principal (fixture 5) ; égalité exacte
  avec support alternatif excluant u → non-principal (fixture 4, carré
  cocirculaire) ; calcul incomplet → `kUnknown`, jamais un faux bit ; une
  miniboule au-dessus de la boule des membres est un REFUS (contradiction
  arithmétique).
- **`HybridSourceReceipt` opaque** : le producteur scelle digests, borne de
  rang et achèvement d'énumération ; la factory dérive `CarrierClosure`
  UNIQUEMENT d'un reçu lié — la fixture 6 grave le cas décisif : table
  amputée du carrier AB + reçu conservé → digest désynchronisé → fermeture
  `kUnknown`, jamais inventée.
- **Trois mutants tués** par les fixtures prévues : `skip-last-removal`
  (principal des concentriques non certifié), `strict-leq` (le carré
  cocirculaire déclaré principal à tort), `witness-by-position` (témoins 0/1
  au lieu des PointId 2/3 du support externe).
- **Pipeline** : les modes `hybrid`/`hybrid-prefix` n'infèrent plus la
  famille complète de `smax >= n` lu sur place — ils construisent le reçu du
  producteur puis le sidecar, et refusent par la raison de la factory ;
  `prefix-all` reste relatif sans exigence. 57/57 CTests de la région.

## Limites v0, déclarées

- Digest FNV-1a 64 bits : il LIE les octets (mutation, désynchronisation),
  il n'est pas cryptographique — le schéma final exigera le SHA-256 des
  contrats.
- La revalidation par census est O(G·n) dans la factory : correcte et
  bornée pour le harnais ; le producteur exact portera son propre census au
  schéma final.
- Le fold consomme le sidecar au niveau du PIPELINE (refus avant fold) ; le
  passage des `principal_flags()` et de l'index injectif DANS le moteur du
  fold — remplaçant sa recomputation interne et le lookup par seau de
  centre — est l'étape suivante, avec `BallActivation` à coquille variable.
- `q_cert` (plus grande arité positive certifiée, pont H0) n'est pas encore
  porté : le certificat actuel ne prouve que `q_min` et l'état principal.

GCP non utilisé.
