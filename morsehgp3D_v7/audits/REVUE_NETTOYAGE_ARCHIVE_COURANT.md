# Revue du nettoyage d'archive — implémentation courante

**Le chemin de nettoyage sous `std::bad_alloc` est fermé pour le périmètre exercé.** La lecture statique et les injections indépendamment rejouées concordent sur `src/io/archive.hpp`, SHA-256 `cc2243aaa1bdbe63b69f165d65152cf62d7fac32ff6c641343542c247d989430`. [Verdict et résultats](RETOUR_ARCHIVE_COURANT.md), [reçu courant](receipts_20260904/archive_delta_current.json).

Cadre : `exploration_v7_hors_registre` / `cpu_reference` / `quantized_u16_input_only` / `audit_independant_math_and_architecture` / `public_status=not_claimed`. Les lignes ci-dessous désignent cette source épinglée.

## Propriété et chemins sans exception

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

## Inventaire fermé et sûreté du commit

Le nettoyeur essaie `input.u16`, `forest_K1.bin` à `forest_K10.bin`, puis `manifest.json`, indépendamment de `entries_` et de `next_k_`. Cela couvre aussi un fichier créé avant l'échec de son enregistrement au manifeste. Les dix noms de forêt sont bornés par le contrôle d'ordre `1..10` (`260–269`). Aucune destination finale ne figure dans la liste.

Un échec d'écriture ou de commit avant renommage empoisonne l'archive. Les contrôles d'ordre complet, de sémantique et de digest restent présents (`290–301`). Après renommage réussi, `published_` interdit au destructeur de retirer l'archive ; l'échec du `fsync` du parent ne produit pas un refus contredisant une archive déjà visible. La porte API confirme ce comportement ainsi que l'impossibilité de publier l'ancien préfixe après un suffixe échoué.

La porte de nettoyage confirme un refus persistant d'allocation, le nettoyage des dix fichiers d'ordre, les échecs de construction et de commit, un refus après callbacks K1/K2 et une erreur OS de suppression. La probe indépendante inchangée revient avec code 0 dans ses deux bras. Ces résultats portent sur la copie figée ; l'unique adaptation de la porte constructeur concerne son répertoire temporaire, comme indiqué dans le reçu.

## Limites explicites

La création par descripteur dépend ici de Linux et d'un `/proc/self/fd` accessible. Le nombre de fichiers, les tampons et le nombre d'appels du diagnostic sont bornés ; le temps et le succès des appels système ne le sont pas. Un diagnostic de dernier recours est best effort si stderr lui-même est inutilisable.

La comparaison device/inode évite de supprimer une entrée déjà remplacée lors du contrôle. Elle ne constitue pas une transaction face à un tiers modifiant continuellement le même nom entre le contrôle et `unlinkat` : le provisoire suppose une propriété exclusive des écritures. Une entrée inattendue n'autorise pas une suppression récursive élargie.

Les erreurs de `close` ne sont pas reprises aveuglément sur Linux ; les erreurs de suppression restent signalées sans être appelées succès. Le test OS démontre précisément un résidu attendu et son diagnostic, puis retire ce résidu de fixture. Aucun résidu ne reste à la fin de la qualification. La correction n'ajoute ni checkpoint ni garantie après coupure électrique.

Les artefacts courants à conserver sont la [probe indépendante](archive_cleanup_probe.cpp), le [reçu de requalification](receipts_20260904/archive_delta_current.json) et le [verdict courant](RETOUR_ARCHIVE_COURANT.md). Les preuves brutes antérieures conservent leur rôle de reproduction ; elles ne décrivent pas le statut de cette source. GCP non utilisé.
