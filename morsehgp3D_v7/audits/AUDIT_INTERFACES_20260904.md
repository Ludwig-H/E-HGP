# Interfaces et publication d’archive — état consolidé le 5 septembre 2026

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

**Les 26 scènes CLI et les six rejets structurels d’archives passent sur le CLI C de la campagne du 4 septembre, incluant AxisBounds, et le lecteur épinglés, y compris pour `normalized_horizontal_h0_candidate`.** La porte indépendante avec `--require-product-rejections`, exécutée sous `python3 -O`, rend `0`. Le correctif de nettoyage sous panne d’allocation est requalifié séparément dans le [retour courant au constructeur](AUDIT_INTERFACES_20260904.md#nettoyage-a1-et-publication) ; la présente porte est rejouée sur le CLI courant.

## Sources et résultat reproductible

Lecture préalable des parties I, pages PDF 35–76, et II, pages PDF 77–134, de `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, puis des documents d'entrée v7. Le § 9.1 impose de distinguer la hiérarchie sur les facettes, sa projection sur les points et les poids d'incidence ; un contrôle d'archive ne confond pas ces objets.

[Le reçu courant](receipts_iteration3/interfaces_current.json) porte les résultats du CLI C. Le CLI et le lecteur proviennent de la copie de sources dédiée sous `audits/.work_iteration3/`. Le [reçu d’exécution](receipts_iteration3/interfaces_execution.json) conserve la commande exacte et les hashes avant/après du binaire, du lecteur et de la contre-fixture : ils sont stables. Le [reçu de construction indépendant](receipts_iteration3/axis_execution.json) raccorde ce CLI aux sources compilées, incluant AxisBounds ; la copie est stable et aucun changement des sources suivies n’est relevé pendant cette construction.

| Élément exécuté | SHA-256 |
| --- | --- |
| Binaire v7, CLI C | `25c9bf8e4ef3cded5647a22f16d81af7a1e778196ad3bff73884a7f58da985f2` |
| Lecteur d'archive | `73f686a31c0ac6cf39f26f938e26c67bfc5fa112deb4c99eadb3656b54d610f5` |

Exécution indépendante terminée à `2026-09-04T22:45:03.482898+00:00`, code 0, sous Python optimisé. Les 26 scènes et six corruptions sont qualifiées sur ce CLI reconstruit avec AxisBounds. La [revue propre à AxisBounds](CENSUS_AXIS_COURANT.md) complète ces contrôles d’interface.

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

**Lecture de coupes et rendu pondéré.** L'archive donne des facettes et des deltas. Elle ne fournit pas les multiplicités par coface nécessaires aux scores du § 9.1. Dans la route normalisée, `born` désigne la première matérialisation dans le sous-flot ; il ne doit pas être interprété comme la naissance géométrique de la facette. Un consommateur ayant besoin de cette naissance peut la recalculer depuis l'entrée canonique et la facette, ou demander son export explicite. Le [contrat courant de masses et vote](CONTRAT_MASSES_VOTE_COURANT.md) précise désormais ce supplément, les univers de facettes/cofaces et le raccord à `build_render`. Le callback reçoit encore les événements ; le CLI les ignore actuellement. La conservation de H0 ne conserve pas leurs multiplicités.

**Exploitation et ressources.** Les refus de cette porte se produisent avant le premier callback de forêt. La [porte de nettoyage](AUDIT_INTERFACES_20260904.md#nettoyage-a1-et-publication) couvre séparément les refus tardifs après callbacks ; les deux portées restent distinguées. Le format n'annonce pas de reprise après interruption ni de garantie après coupure électrique. Le budget `partial_named_payload_proxy_v1` borne des tampons déclarés et ne constitue pas un plafond RSS global : lecture de l'entrée, calcul et export doivent rester mesurés ensemble dans une campagne d'échelle.

**Provenance des campagnes.** Le manifeste lie l'entrée, l'ordre maximal effectif, la sémantique et les hashes. Pour reproduire une campagne et ses coûts, un reçu externe doit aussi conserver commande, options demandées/effectives, identité du binaire, configuration et mesures brutes. Le reçu courant fournit les preuves de ce sous-audit ; il n'attribue aucune performance industrielle aux cinq points utilisés.

La [classification du banc d'incidences](AUDIT_QUALIFICATION_20260905.md#harnais-et-classification-des-campagnes) est vérifiée séparément sur sa correction courante : sept tests en Python normal et optimisé, plus un refus réel K=2. Une observation terminée avec refus conserve zéro succès moteur ; les motifs inconnus et invariants restent invalides.

La [correction du harnais de sonde CI](AUDIT_QUALIFICATION_20260905.md#harnais-et-classification-des-campagnes) passe séparément ses 23 scènes en Python normal et sous `-O`, avec `LD_LIBRARY_PATH` hérité simulé. Le rejet brut de code 2 est conservé et les inventaires nominaux utilisent un environnement nettoyé. Il s’agit d’un résultat local sur faux binaires de porte ; aucun nouveau succès global GitHub n’est revendiqué.

## Reproduction

Depuis la racine, après préparation d'un binaire v7 :

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/contre_fixture_archive_20260904.py --binary build/v7/mhgp7 --validator morsehgp3D_v7/tests/archive_gate.py --work-dir morsehgp3D_v7/audits/.work_boundary_current --require-product-rejections
```

Le résultat exigé est `0`, avec 26 scènes conformes et six rejets par chacun des deux lecteurs. L'exécution de cet audit a utilisé les copies figées des deux arguments ; les hashes ci-dessus identifient précisément les octets concernés.

Toutes les écritures de ce sous-audit sont dans `morsehgp3D_v7/audits/`. Aucun code produit, CMake, registre ou fichier hors de ce dossier n'a été modifié. GCP non utilisé.

## Nettoyage A1 et publication

A1 est fermé sur `archive.hpp` SHA-256 `cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430`, inchangé dans D et le delta E relu. Le [reçu ciblé](receipts_20260904/archive_delta_current.json) conserve la compilation et quatre CTests réussis ; la [suite D indépendante](receipts_20260905/release/summary.json) confirme ensuite ses portes intégrées.

La [probe indépendante](archive_cleanup_probe.cpp) rend 0 avec et sans allocations disponibles, sans résidu. Les injections exercent dix fichiers de forêt, neuf fautes de construction, une après création du répertoire, dix de commit et 24 allocations refusées. Le refus tardif atteint les callbacks K1/K2 (`callback_mask=6`) avant invalidation du payload et nettoyage. La synchronisation du parent après publication reste distincte du succès du renommage. Le seul patch de la porte copiée déplace son temporaire sous `audits/` ; il est conservé au reçu.

### Propriété et chemins sans exception

| Emplacement dans `archive.hpp` | Opération et précondition |
| --- | --- |
| `read_u16_text`, lignes 30–31 | Le deleter ferme le `FILE*` d'entrée en lecture à tout retour ; aucun provisoire à supprimer. |
| `StagingDirectory::~StagingDirectory`, lignes 84–87 | Destructeur explicitement `noexcept` : nettoie uniquement le nom créé et non publié, puis ferme une seule fois les deux descripteurs encore possédés. |
| `StagingDirectory::create`, lignes 89–107 | Ouvre d'abord le parent ; crée le provisoire par `/proc/self/fd/<parent>/…` ; copie immédiatement son nom dans un tableau fixe ; ouvre son fd sans allocation C++ intermédiaire. Le membre est déjà construit : son destructeur fonctionne même si la construction de `ForestArchive` échoue. |
| `StagingDirectory::cleanup`, lignes 122–145 | Essaie les douze noms constants par `unlinkat`, accepte `ENOENT`, compare les identités device/inode avant la suppression du nom du répertoire. Sans fd de provisoire, seul le répertoire encore vide du chemin constructeur est supprimé. |
| `StagingDirectory::warn`, lignes 147–173 | Diagnostic dans 192 octets de pile ; conversions entières `to_chars` ; au plus trois appels à `write`, sans exception C++ ni allocation dynamique. |
| `StagingDirectory::publish`, lignes 114–119 | `renameat2(..., RENAME_NOREPLACE)` est le point de publication. Le booléen `published_` est positionné immédiatement ; l'échec ultérieur de `fsync(parent)` devient un booléen, pas un abandon. |
| `File` constructeur/destructeur, lignes 182–190 | `openat` crée un nom fermé sous le fd du provisoire ; si `fdopen` échoue, ferme le fd. Le destructeur ferme seulement le flux qu'il possède encore. Le fichier déjà créé reste couvert par la liste de nettoyage. |
| `File::finish`, lignes 211–216 | Vérifie `fflush`/`fsync` ; transfère le pointeur et remet le membre à null avant `fclose`, évitant une seconde fermeture même si celle-ci échoue. |
| `ForestArchive::~ForestArchive`, ligne 239 | Destructeur par défaut ; le membre `directory_`, déclaré avant les autres membres, est détruit en dernier. Aucun parcours récursif du système de fichiers n'est ajouté à cette destruction. |
| `WriteGuard::~WriteGuard`, ligne 329 | Affecte uniquement `failed=true` après un échec avant fin d'écriture. Les opérations suivantes refusent l'archive abandonnée (`331–333`). |
| `commit` et `parent_sync_confirmed`, lignes 314–324 | Les fichiers et le répertoire sont synchronisés avant publication ; après celle-ci, seules les affectations de statut restent. L'indicateur de synchronisation parent reste distinct de la publication. |

Les objets `File` locaux sont détruits avant l'archive et son descripteur de répertoire. Les chaînes et vecteurs membres libèrent leur mémoire avant le nettoyage ; ce chemin ne reconstruit pas de chemin `std::filesystem`, ne parcourt pas de `directory_iterator` et n'appelle pas `remove_all`.

### Inventaire fermé et sûreté du commit

Le nettoyeur essaie `input.u16`, `forest_K1.bin` à `forest_K10.bin`, puis `manifest.json`, indépendamment de `entries_` et de `next_k_`. Cela couvre aussi un fichier créé avant l'échec de son enregistrement au manifeste. Les dix noms de forêt sont bornés par le contrôle d'ordre `1..10` (`260–269`). Aucune destination finale ne figure dans la liste.

Un échec d'écriture ou de commit avant renommage empoisonne l'archive. Les contrôles d'ordre complet, de sémantique et de digest restent présents (`290–301`). Après renommage réussi, `published_` interdit au destructeur de retirer l'archive ; l'échec du `fsync` du parent ne produit pas un refus contredisant une archive déjà visible. La porte API confirme ce comportement ainsi que l'impossibilité de publier l'ancien préfixe après un suffixe échoué.

La porte de nettoyage confirme un refus persistant d'allocation, le nettoyage des dix fichiers d'ordre, les échecs de construction et de commit, un refus après callbacks K1/K2 et une erreur OS de suppression. La probe indépendante inchangée revient avec code 0 dans ses deux bras. Ces résultats portent sur la copie figée ; l'unique adaptation de la porte constructeur concerne son répertoire temporaire, comme indiqué dans le reçu.

### Limites explicites

La création par descripteur dépend ici de Linux et d'un `/proc/self/fd` accessible. Le nombre de fichiers, les tampons et le nombre d'appels du diagnostic sont bornés ; le temps et le succès des appels système ne le sont pas. Un diagnostic de dernier recours est best effort si stderr lui-même est inutilisable.

La comparaison device/inode évite de supprimer une entrée déjà remplacée lors du contrôle. Elle ne constitue pas une transaction face à un tiers modifiant continuellement le même nom entre le contrôle et `unlinkat` : le provisoire suppose une propriété exclusive des écritures. Une entrée inattendue n'autorise pas une suppression récursive élargie.

Les erreurs de `close` ne sont pas reprises aveuglément sur Linux ; les erreurs de suppression restent signalées sans être appelées succès. Le test OS démontre précisément un résidu attendu et son diagnostic, puis retire ce résidu de fixture. Aucun résidu ne reste à la fin de la qualification. La correction n'ajoute ni checkpoint ni garantie après coupure électrique.
