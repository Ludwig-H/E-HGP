# L2b — journal FULL et surcoût de diagnostic (capture WIP)

Capture stable du **5 octobre 2026, 18:03:24 UTC**, worktree développeur `build/v11-impl-l2`,
base `ab0f1ba520657fbc1b08f5f9359203558b1c2472`. Dix fichiers WIP capturés et hachés avant/après;
quatre dépendances inchangées sont épinglées au commit de base. Aucun build/test natif ni GCP.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**Constat utile sur le chemin public.** `api/compute.cpp:219` transmet toujours `&seen`, même lorsque
`compute_supports(..., diagnostics=nullptr)` est appelé par `compute`. `order_tree` reçoit donc un pointeur
non nul, transmet `&seen.log`, et `build_order_full` appelle `registers_of(log)` (`order_tree.cpp:136`).
Cette empreinte FNV sert au diagnostic de porte, puis son résultat n’est pas remis au demandeur absent.

Pour C cellules et G graines journalisées, le parcours ajoute exactement **2C+G mots**, soit **16C+8G**
itérations FNV (xor/multiplication), sur le pilote. Il est compris dans le temps `tree`; le chrono reste le
coût réel payé. Aucun temps, gain ou nombre de graines LiDAR n’a été mesuré ni déduit ici. Correction ciblée :
transmettre `diagnostics ? &seen : nullptr` à `supports_parts`; la copie finale du diagnostic est déjà
conditionnelle. Cela retire le parcours lorsqu’aucun diagnostic n’est demandé et conserve celui des portes.

**Ownership et concurrence : lecture favorable, qualification en attente.** Le journal appartient à
`build_order_full` et ne cible que l’ordre k. Sa tâche de publication unique écrit via `cell/regular_cell`;
les tâches de résolution et de verticales n’écrivent pas dans ce journal. `parallel_for` attend tous les
workers, y compris sur refus, avant la fermeture/lecture du journal. Les trois tampons du journal sont admis
et possédés; les autres ordres et les verticales de k sont rendus avant le rattachement. Le domaine est
emprunté pendant la construction et déplacé uniquement au succès. Aucun autre défaut important établi dans
ce delta. Cela ne qualifie pas le brouillon ni les octets écrits après la capture.

`source_manifest.json` fournit les SHA; `review.json` donne les sites causaux et la portée; `closure.json`
distingue les octets capturés et l’état final du développeur. `sources/` conserve la capture; `diffs/` son delta
au commit de base. Les portes d’identité par voies, le journal égal et TSan restent à jouer dans la tranche;
leur absence pendant le WIP n’est pas un défaut et ne bloque pas la publication courante.
