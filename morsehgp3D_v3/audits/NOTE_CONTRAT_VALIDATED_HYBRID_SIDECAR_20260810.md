# Contrat minimal de `ValidatedHybridSidecar`

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_certificate_builder`,
`profile=quantized_u16_input_only`, `mode=implementation_blueprint`,
`public_status=not_claimed`.

## Décision proposée à Claude

Le sidecar doit être un objet opaque construit **après** le tri canonique et
propriétaire d'un snapshot immuable des points et du catalogue. Un booléen
`source_complete_for_order`, même accompagné d'un digest, reste une assertion
hostile; le digest lie des données, il ne prouve pas leur complétude.

Le fold hybride reçoit uniquement `const ValidatedHybridSidecar&`. L'ancienne
API `(points,point_count,Catalogue)` peut rester un harnais explicitement
relatif/non autoritaire, jamais ouvrir le fast public.

## Payload minimal

```cpp
enum class PrincipalState : uint8_t {
  kUnknown,
  kPrincipalCertified,
  kNonPrincipalCertified
};

enum class CarrierClosure : uint8_t { kUnknown, kCertified };

struct SmallSupport {
  uint8_t size;
  std::array<PointId, 4> ids;
};

struct RemovalEvidence {
  PointId removed;
  SmallSupport witness;
  enum class Relation : uint8_t { kStrict, kEqual } relation;
};

struct GeneratorCertificate {
  ExactBallKey exact_ball;
  Digest256 members_digest;
  SmallSupport canonical_support;
  uint8_t q_min;
  PrincipalState principal;
  std::array<RemovalEvidence, 4> evidence;
  uint8_t evidence_count;
};
```

`ExactBallKey` contient le centre rationnel et le rayon carré rationnel réduits.
Le `HybridBallKey` live est un index de centre à deux étages, pas une clé de
boule autonome : des boules concentriques de rayons différents doivent rester
acceptées.

```cpp
class ValidatedHybridSidecar {
 private:
  std::vector<P3> points_;
  Catalogue catalogue_;
  Digest256 points_digest_;
  Digest256 catalogue_digest_;
  Digest256 source_receipt_digest_;
  Digest256 sidecar_digest_;
  std::vector<GeneratorCertificate> generators_;
  std::vector<GeneratorId> activation_order_;
  std::vector<uint32_t> activation_rank_;
  std::vector<uint32_t> batch_offsets_;
  std::vector<CarrierClosure> closure_by_order_;
  UniqueExactBallIndex ball_index_;
  // constructeur privé; factory amie seulement
};
```

Posséder les entrées par déplacement ferme la fenêtre TOCTOU d'un catalogue
muté après validation.

## Reçu de source

Le reçu de source est lui aussi opaque et construit par le producteur exact.
Il porte version de schéma, contrat et SHA du producteur, digests points et
catalogue, profil, borne de rang, statut terminal et preuve de complétion.

À ce snapshot, `smax>=n` ne suffit pas à créer ce reçu. Pour la v0, la fermeture
reste `kUnknown` tant qu'une porte permanente n'a pas reçu l'achèvement de
l'énumération exhaustive des supports et son absence de censure. Les
catalogues anchored/partiels restent également `kUnknown`.

Le bit principal est une propriété locale et peut néanmoins être certifié dans
un catalogue partiel. C'est la fermeture carrier inconnue qui interdit de
l'utiliser pour omettre des requêtes autoritatives; le fallback relatif reste
permis.

## Factory post-tri

La factory exécute atomiquement :

1. prise de possession et recalcul de tous les digests;
2. validation des tranches du pool, sans chevauchement, trou ni reste;
3. membres/supports triés, uniques, dans le nuage et support inclus;
4. `den>0`, sphère exacte, miniboule des membres et saturation fermée complète;
5. support canonique géométrique et `q_min` recalculés;
6. rejet de deux handles pour la même boule exacte;
7. ordre final canonique et lots construits par `sphere_cmp_beta` exact;
8. pour chaque support `u`, miniboule de `M sans u` : strict pour tous les `u`
   donne principal; égalité exacte avec support alternatif excluant `u` donne
   non-principal; calcul incomplet donne unknown, jamais un faux bit;
9. dérivation de la fermeture par ordre depuis le reçu opaque;
10. construction de l'index injectif et du digest final seulement après succès.

Les témoins positifs/négatifs sont indexés par PointId supprimé, pas par la
position avant permutation.

## Décision du fold

- `kUnknown` ou non-principal : fallback exact relatif;
- fast principal : principal certifié, fermeture carrier certifiée et lot solo,
  jusqu'à réception séparée du certificat ex æquo;
- réduction `q_min>k+1` : mêmes exigences;
- absence ou invalidité du certificat : élargir `query_mask`, jamais inventer
  une complétude ni publier un transcript autoritatif.

Chaque lookup rend zéro ou un handle, vérifie inclusion du carrier, niveau
strict et cohérence d'activation. Le comportement live « premier candidat du
bucket » doit être supprimé.

## Points d'insertion

- `order_k_flats.hpp` : construire le reçu seulement après le tri/réindexation
  final; réutiliser census exact et support canonique déjà présents;
- `anchored_catalogue.hpp` : produire seulement un reçu partiel/unknown;
- `saturated_pipeline.cpp` : recevoir un bundle catalogue+reçu, construire le
  sidecar avant le fold et supprimer l'inférence par `smax>=n`;
- `saturated_fold_hybrid.hpp` : remplacer l'API brute, le bool `principal` et
  l'index premier-candidat par les champs validés.

## Fixtures et mutants

- deux handles de la boule singleton : refus `duplicate_exact_ball`;
- points colinéaires `0,1,2`, boule portée par `0,2`, membres déclarés `{0,2}` :
  saturation incomplète refusée;
- deux boules concentriques de rayons distincts : toutes deux acceptées;
- carré cocirculaire : support alternatif, non-principal certifié;
- diamètre avec point supplémentaire sur la coquille : principal certifié;
- certificats construits avant permutation finale : mutant tué;
- carrier supprimé mais reçu conservé : fermeture refusée;
- `den=0`, support faux, digest obsolète, pool chevauché : refus avant index;
- budget de certificat nul : état unknown, fallback relatif;
- oubli d'un `u`, `<` changé en `<=`, témoin indexé par position : mutants;
- niveaux rationnels distincts arrondis au même double : deux lots;
- mutation du catalogue après validation : impossible avec l'objet propriétaire.

Le code possède déjà les briques positives : census exact, support canonique,
comparaison rationnelle des niveaux et logique locale de suppression de `u`.
Le verrou restant est surtout une frontière de confiance typée.

Snapshot : `HEAD=origin/main=37139de2329c32797815db3fa73130a2e80aeda3`.
Les fichiers `order_k_flats.hpp`, `anchored_catalogue.hpp`,
`saturated_fold_hybrid.hpp` et `saturated_pipeline.cpp` sont inchangés depuis
le précédent audit; aucun sidecar n'est encore implémenté.

