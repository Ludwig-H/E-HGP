# Design — Tranche 15L : archive durable de run, publication atomique et reprise

Statut : design normatif, implémentation à venir (verrou ③ de l'ordre de la
roadmap). Aucun claim. Périmètre fixé par la roadmap (l. 1974 : « 15L doit
lier le manifeste, la chaîne source terminale et la chaîne de segments 15I
dans un header--footer durable, fournir un chargeur borné et un recertifier
massif réel, puis exercer une reprise après interruption ») et par le TOML
(`eleventh_increment_next_gate`). Directives du 6/8/2026 : tests locaux
légers ; le recertifier massif réel et toute mesure à échelle vont à la G4.

## Découpage en incréments

**15L-a (prochain incrément, host-first)** : codec canonique des segments,
archive header--footer, publication atomique, chargeur borné, reprise après
interruption. **15L-b (G4)** : recertifier massif réel depuis les runs
pair--higher + gate complet 1 M avant 10 000 001 points (obligation déjà
ouverte : passe sanitizer de la suite transaction).

## Constat d'inventaire (vérifié)

- Les segments 15I (`ExactDirectMorseForestBatchSegment`,
  `direct_morse_forest_segment_sink.hpp`) sont des structures mémoire :
  cursors jumeaux, `payload_digest`, `Batch`, six arènes de records
  (`BirthRecord`, `ArmRootBinding`, `SaddleRecord`, `AtomicGroup`,
  `NodeId`, `Node`) — avec clés de facettes, handles de composantes,
  témoins, `ExactCenter3`/`ExactLevel` rationnels et stamps de locator.
  15I déclare explicitement : « no wire codec ».
- Le sceau final (`ExactDirectMorseForestFinalSeal`) porte le point count,
  l'ordre maximal effectif, le digest du nuage canonique, le cursor final,
  le stamp final du locator, les compteurs et les racines finales.
- 15K (`direct_morse_forest_source_wire.hpp`) fournit le précédent canonique
  côté SOURCE : enveloppe fixe 64 octets big-endian, version/type fermés,
  longueur, SHA-256 du payload, identités, texte rationnel canonique sous
  caps individuel et cumulatif, refus avant callback aval. Ses conventions
  (pas ses symboles internes) sont le modèle du codec 15L.

## Format d'archive `morsehgp3d-run.v1`

Fichier unique, big-endian, trois régions :

1. **HEADER (enveloppe fixe 128 octets)** : magic `MH3DRUN1`, version de
   schéma d'archive (1), version de schéma de segment (2), digest du
   manifeste, digest d'identité source, digest du nuage canonique higher,
   point count, ordre maximal demandé, octets réservés à zéro imposés.
   Écrit en PREMIER mais ne certifie rien seul : un header sans footer valide
   est un run interrompu, jamais un run partiel consommable.
2. **FRAMES de segments**, un par segment 15I acquitté, dans l'ordre de la
   chaîne : `frame_length (u64) || segment_index (u64) || payload
   canonique || payload_sha256 (32) || chain_digest_after (32)`. Le payload
   canonique encode le segment champ par champ dans l'ordre de déclaration
   des structs, offsets/indices en u64, optionnels par octet de présence,
   enums par u8 fermé, textes rationnels canoniques (`ExactLevel`,
   `ExactCenter3`) sous caps individuel et cumulatif, booléens par u8∈{0,1}.
   `chain_digest_after` doit égaler le `end_cursor.chain_digest` du segment :
   la chaîne 15I est la chaîne de l'archive, aucun digest parallèle n'est
   inventé.
3. **FOOTER (scellement)** : magic `MH3DEND1`, compte de segments, cursor
   final encodé, digest terminal de la chaîne source (15J/15K), frame du
   `FinalSeal` encodée comme un segment, SHA-256 de l'archive entière depuis
   l'octet 0 jusqu'au début de ce digest, magic répété. Le footer lie donc
   manifeste + chaîne source terminale + chaîne de segments, ce qui est la
   définition du verrou.

## Protocole d'atomicité et de reprise

- **Écriture** : `<final>.tmp.<pid>` dans le répertoire cible ; append
  séquentiel header → frames → footer ; `fsync(fichier)` ; `rename()`
  atomique vers le nom final ; `fsync(répertoire)`. Le writer n'expose
  jamais un chemin final partiellement écrit.
- **Interruption** : à tout instant, le répertoire contient soit aucun
  fichier final, soit un fichier final complet. La routine de reprise
  classe : `absent`, `torn_temp_discarded` (les `.tmp.*` sont supprimés
  fail-closed après réouverture et constat d'absence de footer valide),
  `valid_final` (header + chaque frame + footer + digest global revérifiés).
  Aucune réparation partielle : un temp n'est JAMAIS promu.
- **Anti-rollback** : le footer porte le digest terminal de la chaîne
  source ; un fichier final plus court mais bien formé d'un run antérieur
  est détectable par ce digest contre l'autorité appelante — le chargeur
  l'exige en entrée (`expected_terminal_source_chain_digest`) et refuse
  tout autre.

## Chargeur borné

`load_exact_direct_morse_forest_run_archive(path, expected_identities,
limits, consumer)` : valide le header contre les identités attendues, lit
les frames une à une sous caps (`maximum_frame_byte_count`,
`maximum_segment_count`, caps de records par segment réutilisant
`ExactDirectMorseForestSegmentLimits`, caps de texte rationnel), décode UN
segment à la fois, vérifie `payload_sha256` puis l'égalité de
`chain_digest_after` avec le cursor décodé, livre au consumer synchrone
non-rétenteur (modèle 15J), libère, continue. Mémoire résidente :
O(plus grand frame). Toute corruption, troncature, suffixe, cap dépassé ou
rupture de chaîne échoue fermé AVANT le callback du segment fautif ;
les segments déjà livrés sont annoncés comme préfixe validé, jamais comme
run complet.

Comme pour 15K, le chargé n'est PAS une autorité scientifique : le décodage
alimente le recertifier (15L-b) qui rejoue les autorités exactes ; l'égalité
`operator==` avec les segments résidents est le différentiel de test, pas
une preuve de vérité scientifique.

## Tests (locaux, légers — cible < 1 s comme 15K)

Round-trip sur la fixture courte 15I existante (segments résidents →
archive → chargeur → égalité `operator==` segment par segment et du sceau
final) ; golden header/footer ; corruption de chaque région (header, frame,
sha, chain, footer, digest global) ; troncature à chaque frontière ;
suffixe ; caps à cap-moins-un ; temp non promu après interruption simulée
(écriture stoppée avant footer → reprise = `torn_temp_discarded`) ;
anti-rollback (footer d'un run préfixe refusé sous le digest terminal
attendu). Les mesures RSS, le gate 1 M et le recertifier réel sont hors de
ces tests (G4, 15L-b).
