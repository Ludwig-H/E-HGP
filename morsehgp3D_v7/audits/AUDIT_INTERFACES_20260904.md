# Audit des interfaces v7 — état vérifié le 4 septembre 2026

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

**Les 26 scènes CLI et les six rejets structurels d’archives passent sur le binaire mono et le lecteur épinglés, y compris pour `normalized_horizontal_h0_candidate`.** La porte indépendante avec `--require-product-rejections`, exécutée sous `python3 -O`, rend `0`. Le correctif de nettoyage sous panne d’allocation est requalifié séparément dans le [retour courant au constructeur](RETOUR_ARCHIVE_COURANT.md) ; la présente porte est rejouée sur le nouveau binaire.

## Sources et résultat reproductible

Lecture préalable des parties I, pages PDF 35–76, et II, pages PDF 77–134, de `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, puis des documents d'entrée v7. Le § 9.1 impose de distinguer la hiérarchie sur les facettes, sa projection sur les points et les poids d'incidence ; un contrôle d'archive ne confond pas ces objets.

[Le reçu courant](receipts_20260904/interfaces_delta_current.json) porte les résultats du delta d’archive intégré. Le CLI et le lecteur proviennent de la copie de sources dédiée sous `audits/.work_mono/`. Le [reçu d’exécution](receipts_20260904/interfaces_delta_validation.json) conserve la commande exacte et les hashes avant/après du binaire, du lecteur, de la contre-fixture et d’`archive.hpp` : ils sont stables. Le [reçu de construction du delta](receipts_20260904/mono_current.json) raccorde le nouveau CLI aux sources compilées.

| Élément exécuté | SHA-256 |
| --- | --- |
| Binaire v7 | `c7da95a3a83c1e31fdfd95db852fed86f43208e6b1b051dfb36e78baf45e5175` |
| Lecteur d'archive | `73f686a31c0ac6cf39f26f938e26c67bfc5fa112deb4c99eadb3656b54d610f5` |

Exécution indépendante terminée à `2026-09-04T21:57:39.521493+00:00`, code 0, sous Python optimisé. Les 26 scènes et six corruptions sont qualifiées sur le CLI reconstruit après le delta mono, sans réattribuer les résultats d’un ancien binaire. Le changement AxisBounds intégré ensuite possède une [revue distincte](CENSUS_AXIS_COURANT.md) ; ce CLI n’est pas présenté comme sa compilation.

## Contrôles acquis

Les 26 invocations comprennent **4 succès et 22 refus de code `2`**. Les refus n'émettent aucun payload sur stdout, ne publient aucune archive finale et ne laissent aucun répertoire provisoire dans les scènes exercées.

- L'entrée réelle conserve ses identités et l'ordre physique des points dans `input.u16`.
- Les digests des trois forêts testées sont identiques entre stockage classique à un thread et CSR à quatre threads, y compris avec `s=9223372036854775807`.
- La scène E5 à cinq points avec `--complete-incidences` produit des archives `normalized_horizontal_h0_candidate` en classique/un thread et CSR/quatre threads. Les octets des trois forêts sont identiques, donc leurs facettes, partitions, niveaux, parents, `born` et sorties aussi. Les digests normalisés diffèrent de ceux de la route `verified_events_only`, ce qui exclut une comparaison vacue.
- Une destination existante est refusée sans modification de son manifeste.
- `--require-exact`, le mélange entrée réelle/génération synthétique, un budget inférieur à un candidat, les options invalides et les mutants dans le binaire produit sont refusés.
- Les identités ou positions dupliquées, dépassements u32/u16, entiers signés hors contrat, coordonnées décimales ou exponentielles, champs manquants ou surnuméraires, NUL, lignes trop longues et entrée à un seul point sont refusés.

La structure normalisée est rejouée par le juge indépendant avec un état avant lot. Un parent doit être déjà matérialisé, une clé `born` doit être nouvelle et une composante ne peut être consommée deux fois au même lot. La scène K=2 contient dix facettes, six deltas, cinq références de parents et dix références `born` : les deux rôles sont effectivement exercés.

La porte vérifie aussi trois incohérences structurelles pour chacun des deux objets, en recalculant les hashes après mutation : six cas au total. Les mutations normalisées portent sur K=2 et exercent donc réellement la réduction des parents et les premières matérialisations.

| Mutation | Lecteur v7, `verified_events_only` K=1 | Lecteur v7, normalisé K=2 | Juge indépendant |
| --- | --- | --- | --- |
| Partition finale remplacée par des singletons malgré les deltas de fusion | Rejet | Rejet | Deux rejets |
| Même numéro de lot attribué à plusieurs niveaux exacts distincts | Rejet | Rejet | Deux rejets |
| Sortie d'un delta remplacée par une clé connue non canonique | Rejet | Rejet | Deux rejets |

Le lecteur reconstruit les composantes uniquement depuis les deltas exportés. Il contrôle les parents dans l'état avant lot, les sorties canoniques, la consommation unique des références, les premières matérialisations, la correspondance lot/niveau et la partition finale. Il ne réutilise ni les cofaces sources ni le DSU du producteur.

[La contre-fixture permanente](contre_fixture_archive_20260904.py) utilise un rejeu par ensembles, indépendant du DSU du lecteur v7. Son champ `product_reader_rejected` désigne la fonction `tests/archive_gate.py::validate_archive`, une porte de test du dépôt. Les six valeurs sont `true` dans le reçu courant. Les contrôles restent effectifs sous Python optimisé ; aucune porte de cette contre-fixture n'utilise `assert`.

## Portée de l'export et prochaines qualifications

**Objet scientifique.** Deux archives positives portent `verified_events_only` et deux portent `normalized_horizontal_h0_candidate`. Leur structure et leur identité entre les stockages testés sont qualifiées sur E5. Le format déclare `public_status=not_claimed`, `require_exact=false` et `vertical_maps=none`. Le rejeu structurel ne certifie ni les niveaux géométriques ni la complétude de Gamma ; ces propriétés restent du ressort des campagnes mathématiques propres à la route normalisée.

**Lecture de coupes et rendu pondéré.** L'archive donne des facettes et des deltas. Elle ne fournit pas les multiplicités par coface nécessaires aux scores du § 9.1. Dans la route normalisée, `born` désigne la première matérialisation dans le sous-flot ; il ne doit pas être interprété comme la naissance géométrique de la facette. Un consommateur ayant besoin de cette naissance peut la recalculer depuis l'entrée canonique et la facette, ou demander son export explicite. Un export de rendu pondéré constitue un contrat distinct.

**Exploitation et ressources.** Les refus de cette porte se produisent avant le premier callback de forêt. La [porte de nettoyage](RETOUR_ARCHIVE_COURANT.md) couvre séparément les refus tardifs après callbacks ; les deux portées restent distinguées. Le format n'annonce pas de reprise après interruption ni de garantie après coupure électrique. Le budget `partial_named_payload_proxy_v1` borne des tampons déclarés et ne constitue pas un plafond RSS global : lecture de l'entrée, calcul et export doivent rester mesurés ensemble dans une campagne d'échelle.

**Provenance des campagnes.** Le manifeste lie l'entrée, l'ordre maximal effectif, la sémantique et les hashes. Pour reproduire une campagne et ses coûts, un reçu externe doit aussi conserver commande, options demandées/effectives, identité du binaire, configuration et mesures brutes. Le reçu courant fournit les preuves de ce sous-audit ; il n'attribue aucune performance industrielle aux cinq points utilisés.

La [classification du banc d'incidences](CAMPAGNE_INCIDENCES_COURANTE.md) est vérifiée séparément sur sa correction courante : sept tests en Python normal et optimisé, plus un refus réel K=2. Une observation terminée avec refus conserve zéro succès moteur ; les motifs inconnus et invariants restent invalides.

## Reproduction

Depuis la racine, après préparation d'un binaire v7 :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/contre_fixture_archive_20260904.py --binary build/v7/mhgp7 --validator morsehgp3D_v7/tests/archive_gate.py --work-dir morsehgp3D_v7/audits/.work_boundary_current --require-product-rejections
```

Le résultat exigé est `0`, avec 26 scènes conformes et six rejets par chacun des deux lecteurs. L'exécution de cet audit a utilisé les copies figées des deux arguments ; les hashes ci-dessus identifient précisément les octets concernés.

Toutes les écritures de ce sous-audit sont dans `morsehgp3D_v7/audits/`. Aucun code produit, CMake, registre ou fichier hors de ce dossier n'a été modifié. GCP non utilisé.
