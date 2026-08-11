# Audit épinglé — sidecar `cbac109` et frontière de source

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Périmètre immuable : commit
`cbac109a09c2575cdf875b19de1570265bd5bf08`.
Le statut du worktree postérieur est exclusivement dans
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

Empreintes du snapshot :

| fichier | SHA-256 |
| --- | --- |
| `prototype/validated_hybrid_sidecar.hpp` | `d4611eea124d80d1c4ff20a16cd73a7a40a6bb13f22e522a30adf5d921fd819c` |
| `prototype/sealed_source.hpp` | `74ee9f04aa87862f33137655ea0a74498970471fbe6253ef6d1c37058c9529fe` |
| `prototype/hybrid_fold_validated.hpp` | `d01dd4f86d6db7e312be681ddefec1d0f7d88c5e4103f4a62483bc4fdeba55a6` |
| `prototype/saturated_pipeline.cpp` | `4989a31bdb5e20fcedc04034b5fc305ec9d6c0f6fc99b3cd3bb4b9abe4488b56` |

## Verdict du snapshot

Le delta corrige plusieurs défauts de `9483b1c`, mais n'est pas reçu comme
frontière de confiance. Quatre contre-résultats indépendants subsistent :
reçu frais forgeable, doublon exact non adjacent accepté, comportement indéfini
sur `INT128_MIN` et digest contractuel incomplet.

Le pipeline hybride est structurellement borné à `n<=32`; même corrigé, il est
un oracle CPU et non une route 50 k.

## Corrections réellement présentes à `cbac109`

- Le fold validé reçoit un `ValidatedHybridSidecar` au lieu de revenir au
  catalogue brut.
- La clé ne carre plus les numérateurs du centre en `i128`; le niveau exact est
  comparé avec la primitive multiprécision existante.
- Le support déclaré est vérifié sur la coquille, comparé au cardinal minimal
  recalculé, testé comme générateur de la boule et refusé lorsqu'un sous-ensemble
  propre engendre déjà la même boule.
- Le digest du catalogue ne lit plus l'image mémoire complète de
  `CriticalSphere`, donc ne dépend plus directement du padding ou du `double`
  diagnostique.

Ces progrès ne ferment pas les quatre portes ci-dessous.

## S1 — reçu frais forgeable

`SourceProducerToken` a un constructeur privé, mais le type du snapshot est
vide et trivialement copiable. C++20 permet de le fabriquer avec
`std::bit_cast`, puis d'appeler le constructeur public de
`HybridSourceReceipt` sur un catalogue amputé et ses digests recalculés. La
factory accepte alors la table et certifie les fermetures.

La reproduction indépendante a terminé avec `sidecar.ok()==true` et
`closure_certified_all_orders()==true`. La correction requise est un reçu à
constructeur privé, non trivialement copiable, produit seulement après la
terminaison de l'énumérateur autoritaire. Un digest empêche une
désynchronisation; il ne prouve pas que l'énumération était complète.

## S2 — doublon `[r1,r2,r1]`

L'index du snapshot trie par `(centre,index_catalogue)`, puis ne compare que
les voisins. Pour le nuage
`{(1,2,0),(3,2,0),(0,2,0),(4,2,0)}`, les records valides « boule intérieure,
boule extérieure, copie de la boule intérieure » sont ordonnés
`[r1,r2,r1]`; les deux copies ne deviennent jamais voisines et la factory les
accepte.

Le tri correct ajoute le niveau exact entre centre et indice, ou emploie une
vraie `BallKey` multiprécision. La fixture positive `[r1,r2]` doit rester
acceptée et `[r1,r2,r1]` doit refuser.

## S3 — `INT128_MIN` atteint le pgcd

Une sphère publique synthétique avec `nx=INT128_MIN, den=1` atteint la
négation non représentable de `INT128_MIN` dans le pgcd avant tout refus.
UBSan signale un overflow signé.

La factory doit borner l'ABI avant toute arithmétique et le pgcd doit prendre
les magnitudes dans un entier non signé. Les points et les champs `base`
doivent eux aussi être validés dans le profil u16 avant `sphere_side`,
`miniball_of` ou la construction de clé; borner seulement les numérateurs et
le dénominateur ne suffit pas.

## S4 — support canonique et digest

Le snapshot vérifie qu'un support déclaré est minimal et engendre la boule,
mais le recopie dans `canonical_support`. Sur une coquille avec plusieurs
supports minimaux, cela ne reconstruit aucun tie-break public indépendant.

FNV-1a 64 bits sur l'ordre natif des octets ne constitue pas le SHA-256
contractuel. Le digest final ne lie pas séparément toutes les
`RemovalEvidence`, `maximum_order` et fermetures. La réparation exige une
sérialisation little-endian champ par champ, taggée, versionnée et un digest
du certificat complet.

Une politique de support doit aussi rester cohérente de bout en bout. Deux
choix exacts sont possibles :

1. le support minimal déclaré reste une provenance autoritaire, ses evidence
   et son fold lui sont explicitement liés, tandis que le support canonique
   est un champ distinct de déduplication;
2. le catalogue possédé est normalisé sur le support canonique, puis evidence,
   digests et fold sont tous recalculés sur ce même snapshot.

Publier un support canonique tout en laissant le consommateur croire qu'il
gouverne des evidence calculées sur un autre support est un contrat ambigu.

## Oracle borné à jamais

Trois conditions composent la borne :

1. la CLI refuse `smax>32`;
2. le reçu de famille complète exige `rank_bound>=point_count`;
3. le pipeline refuse le fold si cette fermeture n'est pas certifiée.

Ainsi le mode validé ne peut accepter que `n<=32`. Il énumère en outre le
catalogue une première fois pour le pipeline puis une seconde fois dans le
producteur scellé, et la factory rescane le nuage pour chaque générateur afin
de vérifier la saturation. Ce coût `O(G*n)` et cette double énumération sont
acceptables pour un oracle borné, jamais pour `warm_e2e` à 50 k.

Le futur producteur streamé doit avoir son propre domaine complet, une
identité count/fill/consume, un statut terminal sans censure et des reçus de
tâches rejouables. Il ne peut hériter l'autorité de la seule condition
`smax>=n`.

## Critère de réception d'un successeur

- construction privée et anti-forge réellement exercée;
- domaine u16 vérifié pour points, bases et représentation avant géométrie;
- index injectif par centre et niveau exact;
- politique unique de support, avec evidence et fold cohérents;
- SHA-256 complet sur sérialisation canonique et vecteurs FIPS;
- tests hostiles sous UBSan et mutants non compensables;
- classification explicite `oracle_cpu_borne_n_le_32`, jamais source 50 k.

GCP non utilisé.
