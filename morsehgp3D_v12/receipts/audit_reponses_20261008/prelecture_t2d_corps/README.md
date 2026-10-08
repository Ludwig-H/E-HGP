# T2-d-A — contrelecture des premiers corps, avant livraison

Capture du 8 octobre 2026, **prototype en cours**, base déclarée `8dc5d6b16`.
La copie active est `prototype_A/work`, distincte de `prototype_A/repo` restée inchangée lors des premiers relevés.
Les empreintes et seuls extraits nécessaires sont dans `capture.json` ; les octets ont été relus après capture.
Cela fige une observation, pas une version livrée ni une qualification d'exécution. Aucun natif, build ou GCP.

## Refus mémoire : correction encore nécessaire à la capture

Dans `pipeline.cpp:149–162` SHA `a24b6bad…`, le helper `open_session` est `noexcept` et contient deux
`std::make_unique` (`BuildState`, puis `Pipeline`). Si l'un lève `std::bad_alloc`, cette exception quitte une
fonction `noexcept` : terminaison du processus, avant que le `guarded` extérieur de `build_tower:211` puisse
la convertir en `memory_budget` (`core/status.hpp:164–169`). C'est une déduction statique du chemin d'exception,
pas une panne injectée ni un échec observé sur une campagne.

La refonte du helper initial `run_tower` (`dcf1bf3e…`) en `open_session` ne corrige pas ce chemin d'exception.
Correction minimale suggérée : enlever `noexcept` du helper interne `open_session` et de sa déclaration dans
`pipeline.hpp`, conserver la frontière
publique `build_tower noexcept` et son `guarded`. Les propriétaires RAII déroulés avant la garde rendent leurs
tampons. Qualification à ajouter par le développeur : échec de la première puis de la seconde allocation,
refus `memory_budget`, budget libéré et aucune terminaison. Cette vérification n'a pas été exécutée ici.

## Numérotation : double diagnostic déjà corrigé pendant la lecture

Dans la première lecture (`pipeline_run.cpp` SHA `41a303e5…`), `number_order` écrivait
`BuildState::physical[i].births_ns`, puis `charge_stage(kNumber)` rechargeait la tâche entière dans le compteur
par fil ; `close_ledger` additionnait les deux. La numérotation était donc imputée deux fois dans `births_ns`,
puis dans `kernels_ns`, sans modifier le mur englobant ni la géométrie.

**La copie capturée SHA `910057b6…` corrige ce point** : `kNumber` chronomètre séparément l'ouverture du noyau
dans `kernel_ns`, tandis que `charge_stage` retourne sans recharger `kNumber`. `number_order` reste l'unique
auteur du temps de numérotation. Aucun défaut courant de double comptage n'est retenu sur ces octets ; aucun
transfert de qualification runtime n'en découle. La première observation et son empreinte restent dans la capture.

## Fin de G : préciser la frontière avant toute mesure d'overlap

Dans `pipeline_run.cpp:209–225`, la durée G du worker s'arrête à `t1`, puis celui-ci peut jouer les feuilles de T.
Le drapeau de tranche, `g_finished` et enfin `g_end_ns` sont publiés **après** ces feuilles. La fin enregistrée
est donc celle des tâches G avec cette pré-passe optionnelle de T, et non nécessairement celle du dernier calcul G.
Un exemple d'horloge purement illustratif : G termine à 10, ses feuilles à 13 et la tour à 20 ; la queue publiée
vaut 7, tandis que la queue après G seul vaut 10. Ce ne sont pas des mesures.

`tower.hpp:219–224` décrit pourtant `end_ns − g_end_ns` comme la forêt non recouverte par G. Soit nommer et
documenter la vraie frontière G+pré-passe, soit séparer la publication de fin des calculs G de la disponibilité
des feuilles. Les compteurs `g_thread_ns` et `forest_thread_ns` sont des sommes de fenêtres murales de tâches,
pas du temps CPU mesuré. Le futur lecteur doit admettre explicitement ce nouveau schéma : les sommes séquentielles
du lecteur FULL K restent propres à K. Aucun chiffre K n'est invalidé par ce prototype.

## Points favorables et limites de cette lecture

- A2 a un auteur des feuilles par tranche : le worker les finit avant son store-release ; le noyau attend
  l'acquire et les calcule seulement si `kSliceLeaves` manque. Le préchargement du noyau est borné aux feuilles prêtes.
- Le DAG attend `V_naissances(k)` avant `V_fusions(k)`, plus l'historique inférieur ; R attend M et l'historique
  du même ordre. Les événements sont libérés à cette jonction, après leurs deux lecteurs.
- Les compteurs G sont séparés par ordre et fil ; une seule région Pool orchestre les tâches. Une admission
  globale additionne les tables et les besoins T/M/V/R, au lieu de réutiliser les admissions d'étages concurrents.
  Ceci n'est pas une preuve exhaustive de la formule, de tous les interleavings ou des refus sous budgets finis.

Le rapport local SHA `63077656…` compare W1 local bruité et W48 G4 pour annoncer ×60/×8. Ce quotient n'est pas
un facteur de parallélisation mesuré sur matériel identique ; la capacité d'un noyau local de 24 ms à suivre G
sur G4 reste une hypothèse de campagne. Aucun gain, identité FUL1/CSR, refus déterministe, sanitizer ou contrat
FULL nouveau n'est acquis ici. Les critères préparatoires restent dans
[la prélecture T](../prelecture_t2d_t/README.md) ; ce reçu n'ajoute aucun constat contre une livraison existante.
