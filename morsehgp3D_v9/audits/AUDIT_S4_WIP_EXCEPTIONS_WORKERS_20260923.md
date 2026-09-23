# S4a WIP : récupérer les pannes mémoire et compter les deux voies

23 septembre 2026 — préflight de source en lecture seule, `exploration_v9_hors_registre`, `public_status=not_claimed`. Snapshot **mutable, non publié** du worktree développeur `build/v9-open-worktree`, HEAD `3605cef99`, `src/gpu/lanes_host.hpp` SHA-256 `225b127fee96124b04ed9fdd21ca3c75d0f4a9b2775a6ff4674cb731951095f3`. Ce point n'est ni un résultat CUDA/G4 ni une reproduction dynamique de panne. Depuis le premier snapshot `d1292fb6…`, le WIP a ajouté un tableau `scratch` au slab ; les fenêtres décrites ci-dessous restent présentes.

`run_lanes_batch_host` dispose déjà d'un relais utile `failure`/`failed`/`turn` et joint les threads en cas d'échec **pendant** le calcul d'un bloc (`lanes_host.hpp:74–91`). Sa fenêtre de capture est cependant trop courte :

- les allocations des slabs par worker (`:55–65`) et les `assign` de début de bloc (`:70–73`) précèdent le `try` ;
- le commit ordonné fait `out.records.insert(...)` sous mutex **après** le `try` (`:93–120`) ; l'insertion peut réallouer ;
- le `worker()` du fil appelant (`:135`) n'est lui-même couvert par aucun garde qui joigne `pool` pendant le déroulement de pile après exception.

Avec plusieurs blocs et `workers>1`, une exception d'allocation dans un thread créé qui traverse son point d'entrée appelle `std::terminate`. Si elle vient du fil appelant avant les `join`, le destructeur de `pool` rencontre des `std::thread` encore joignables et appelle aussi `std::terminate`. Une panne pendant `insert` est particulièrement mal placée : elle arrive avec le mutex pris et avant l'incrément de `committed`, donc les autres workers peuvent attendre ce commit. Le `catch(std::bad_alloc)` de la chaîne (`tower_chain.cpp`, fin de `run_tower_chain`) ne peut pas convertir une terminaison de thread en `kResourceExhausted`. Je n'en déduis aucun défaut d'exactitude géométrique des sorties réussies.

Le cas zéro arête révèle aussi un coût inutile : `blocks=0` mais `threads=1`, donc les slabs complets sont alloués avant que le worker constate `block>=blocks`; la voie CUDA retourne, elle, avant toute allocation de slabs (`filter_runner.cu`, `run_lanes_batch`). Un retour hôte vide après validation supprimerait ce coût et sa fenêtre de panne.

**Correction courte proposée.** Englober toute la fonction worker, allocations, `assign`, calcul, attente et commit compris, dans un `try/catch (...)` qui publie la première exception sous mutex, pose `failed`, réveille `turn`, puis sort. Encadrer aussi l'appel du worker sur le fil appelant par un garde de jointure, et ne relancer `failure` qu'après avoir joint tous les threads. Le commit partiellement modifié n'est jamais publié au-dessus si l'appel échoue : le résultat local est abandonné ; sur succès, conserver la réservation et les tranches déterministes actuelles. Ajouter un retour immédiat pour `edges==0` après la garde d'entrée.

**Porte ciblée avant publication.** Injecter une panne d'allocation à trois points distincts : (1) construction de slab dans un worker créé, (2) `assign` d'un bloc après le premier, (3) croissance de `out.records` au commit. Exiger une fin sans terminaison ni worker bloqué, la jointure, et un échec typé `kResourceExhausted` au niveau chaîne. Faire tourner au moins deux blocs/plusieurs workers et une version un seul worker ; vérifier ensuite que les sorties et le ledger du chemin sans panne sont inchangés. Cette porte serait complémentaire du gate géométrique `lanes_port_gate.cpp`, qui teste les reports de capacité mais pas ces exceptions.

## Ledger `both_edges` quand q3 est différé

Même snapshot mutable ; `src/gen/pipeline/wspd_q34.cpp` SHA-256 `b628bd4bb308e29daefe44ebcf8325417b247e5a6ee646403570c03e9b58cbe8`. Le contrat du générateur définit `q3_edges/q4_edges/both_edges` pour les **voies survivantes** (`wspd_q34.hpp:200–205`) ; la porte globale compare `both_edges` au nombre d'arêtes de masque survivant `6` (`tests/gen/wspd_q34_gate.cpp:246,264`). Pour un survivant S3 de masque `6`, S4a pose `asked[j]=2` (`wspd_q34.cpp:1615–1622`). La phase 3 appelle donc `certified_edge(...,4)` : le moteur compte q4 seul (`:1656–1664`). Si la voie q3 est décidée, S4a ajoute explicitement `both_edges` (`:1698–1704`). Si elle est différée par capacité, elle entre dans `tail` et le moteur appelle plus tard `certified_edge(...,2)` (`:1721–1730`) : ni cet appel ni la phase 3 n'ajoutent `both_edges`. Le compteur sous-estime alors de **une arête par q3 différé avec q4 ouverte**, bien que q3 et q4 aient tous deux été effectivement exécutés. `validate_completion` contrôle les masses q3/q4 mais pas `both_edges` (`:387–410`), donc ne signale pas cet écart.

Le traitement est simple : après `check_lanes_batch`, pour **chaque** survivant S3 certifié avec `asked[j]=2` et q4 encore ouverte, ajouter `both_edges` exactement une fois au ledger de raccord `filter_work`, **avant** la branche `decided`/`tail` ; supprimer l'ajout réservé aux seuls décidés. Garder `q3_edges`, `lanes.edges`, les émissions et le coût de traîne selon le chemin réellement exécuté. Il n'y a pas de double compte : les arêtes S3 différées ont `asked=0` et leur `surviving_edge` compte naturellement les deux voies ; pour les arêtes S3 certifiées, les deux appels séparés portent chacun un masque singleton et n'ajoutent jamais `both_edges` eux-mêmes. Une porte hôte à capacité réduite doit imposer au moins un survivant S3 de masque `6` dont q3 est reportée, puis comparer le multiensemble des présentations, le digest de tour et le ledger sémantique des voies avec S4a désactivé. Un mutant qui omet l'ajout dans la branche `tail` doit faire échouer précisément la comparaison `both_edges` ; exiger ce plancher. Le gate actuel `lanes_port_gate.cpp` contrôle le travail q3 décidé/reporté mais pas la fusion q3/q4 de la chaîne. Ce constat porte sur la comptabilité, pas sur une boule perdue.


**Suivi du WIP à 22 h 59 UTC.** `wspd_q34.cpp` a changé (SHA-256
`5b421436e10916ace68764043c07ec47c556bd81daaf3a7cea196dcfbdd30a5a`).
L'ajout à `both_edges` est maintenant placé avant la bifurcation
`decided`/`tail` (`:1696–1705`) : le sous-comptage repéré ci-dessus est
**corrigé dans le source mutable**. La porte à capacité réduite et son
mutant restent à exécuter avant de qualifier ce changement. Le fichier
`lanes_host.hpp` conserve au même moment son SHA-256 `225b127f…` : les
fenêtres d'exception du premier constat demeurent ouvertes.


## Suivi après le commit S4a local

Le développeur a créé le commit local **`aad7416a5`** (non encore sur
`origin/main` à cette lecture). Il incorpore la correction `both_edges`,
mais le `lanes_host.hpp` commis garde les fenêtres d'exception du premier
constat. Le correctif **mutable** suivant, `lanes_host.hpp` SHA-256
`50144a3ef90a572822af6c5aa9d7b9119fa1ab4f459fe4e8977ea0d6e0835133`,
englobe maintenant allocations, blocs, attente et commit dans le `try` du
worker, pose `failed` et réveille les autres sous mutex, joint tous les
fils avant de relancer la première exception ; `edges==0` revient avant
les slabs. Cette structure ferme les trois fenêtres **à la lecture du
source**, sans reçu d'injection aux trois points ni qualification G4.

La nouvelle porte mutable `tests/gpu/lanes_port_gate.cpp` (SHA-256
`e03cd41c06c3c8ed2ff087be646815198157ca9e4c96f084cda912aaf911addc`)
cherche une panne en fixant `record_capacity=0xffffffff` pour un appel à
un et plusieurs workers. `std::vector<LaneRecord>` tente alors environ
**512 Gio par worker**, puis initialise les enregistrements. Sur un hôte
Linux avec overcommit, la réservation virtuelle peut réussir et
l'initialisation provoquer un OOM du processus ou de l'hôte avant qu'un
`std::bad_alloc` soit livré. Ce test est **non déterministe et coûteux** ;
je ne l'ai pas lancé. Il ne force par ailleurs ni l'`assign` de bloc ni
la croissance de `out.records.insert` sous mutex.

Remplacer cette panne géante par un point d'injection déterministe et borné
aux trois allocations ciblées, y compris après le premier bloc et au
commit ordonné ; chaque essai doit démontrer jointure, réveil, erreur
typée et absence de sortie partielle. Pour la seule panne de slab, un
sous-processus avec limite d'espace d'adressage peut servir de garde
supplémentaire, sans prétendre tester les deux autres fenêtres. Une porte
à capacité réduite qui reporte q3 avec q4 ouvert reste nécessaire pour
qualifier le `both_edges` corrigé : le gate de chaîne du commit ne compare
ni `both_edges` ni le ledger des voies, et son plancher de traîne peut être
satisfait par une arête q3 seule. Ne pas assimiler ce préflight à un
résultat S4a/G4 : aucun reçu R15 n'est présent ici.

**Suivi du WIP à 23 h 22 UTC.** Le gate de chaîne mutable
`tests/chain/chain_batch_q3_gate.cpp` (SHA-256 `8cf7e0e3…`) compare désormais
sept champs du ledger entre S3 seul, S3+S4a jugé et S3+S4a à ardoise réduite,
dont `both_edges`, `cover_builds`, `cover_sites` et les deux voies mortes.
La lacune « aucun contrôle de `both_edges` » ci-dessus est donc **corrigée
dans ce WIP**. Le plancher `tails>0` et le plancher `both>0` portent encore
sur des sommes séparées : ils ne forcent pas la même arête à avoir q3
**différée** et q4 **ouverte**. Une fixture S3 de masque `6` avec report q3
sur cette arête, ou un compteur explicite de l'intersection, puis un mutant
qui omet son crédit `both_edges`, fermeraient la porte ciblée. Comparer aussi
`q4_emitted` à l'ardoise réduite : le gate compare actuellement cette masse
entre S3 et S4a normal, mais pas avec le bras reporté.

Le nouveau `receipts/s4a_q3_lanes_local_20260923/run.sh` (SHA-256
`f34fa558…`) est pour l'instant **un plan de capture**, seul fichier du
dossier à cette lecture. Il projette 08/000000 sans sol, grille 1 mm,
K5/K10/W8, trois modes de chaîne CPU et la comparaison huit anneaux/un
anneau. Il ne rejoue pas les moitiés, quarts ou densités. Il écrit `HEAD`
alors que les sources S4a et les portes sont encore modifiées hors commit,
et ne lie ni empreintes des sources effectives, du binaire et de l'entrée,
ni contrôle des statuts, des digests de catalogue/tour et des ledgers des
JSON produits. Les futurs `SHA256SUMS` des seuls fichiers de sortie ne
remplacent pas ces vérifications. Pour un reçu exploitable, figer ce paquet,
archiver ses identités, comparer les trois sorties et publier les reports ;
ensuite mesurer le même port sur la matrice plein/moitiés/quarts ×
densités déjà archivée. Les statistiques `--file` ne sont pas une porte
LiDAR d'exactitude, et la borne `warp_steps_lower_bound` reste un minorant
par arête des ballots par graine (voir l'audit de couplage S4a).

**Réception v20, WIP `gcp-migration/tower_worker_v9.py` SHA-256
`89983e24…`.** Le validateur accepte maintenant à juste titre une traîne
q3 à ardoise réduite et l'absence de census q3 de feuille quand S4a le
remplace. Il garde toutefois une porte trop lâche dans `validate_lanes`
(`:623–628`) : dès que `batch.deferred>0`, un nombre arbitraire de voies
q3 peut manquer dans `lanes_asked`. Dans le générateur, `asked[j]` est
exactement la voie q3 ouverte d'un certificat S3 **décidé** ; toute voie
q3 non demandée vient donc d'un certificat S3 différé, à raison d'au
plus une par arête. L'invariant gratuit est
`0 ≤ ledger.q3_edges − batch.lanes_asked ≤ batch.deferred`.
La borne gauche existe déjà ; ajouter la borne droite et un mutant avec
une seule arête S3 différée mais deux voies q3 manquantes. Cela renforce
la réception des comptes ; les digests du reçu restent une porte séparée
pour l'objet. Voir aussi la [validation répétée et le budget d'arène](AUDIT_S4A_VALIDATION_ET_ARENE_20260923.md).
