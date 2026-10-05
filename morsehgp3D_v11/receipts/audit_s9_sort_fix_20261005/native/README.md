# S9 : correction du refus du tri

Capture WIP du 5 octobre 2026 à 17:10:40 UTC sur HEAD `451301787c7e02b8f5e1a4c08064275b6af6de1e`. Quatre fichiers capturés ; SHA et statut final dans `source_manifest.json` et `closure.json`. Aucune source développeur modifiée, aucune ancienne capsule retouchée.

**Correction source favorable.** Le tri par tas rend chaque `Outcome` refusé avant d’utiliser sa réponse ou de comparer à nouveau. `date_less` et `entry_order` propagent le refus. Aucun changement de relation, aucune allocation supplémentaire. Le tri réussi garde le départage « date exacte, puis SiteIdx ».

Port Python borné : 6 011 ordres finis et 133 456 positions du premier refus injecté. À chaque refus : arrêt exact, même Outcome, permutation conservée, lectures/écritures instrumentées dans les bornes. Les témoins de 17 éléments aux comparaisons 4/9/30 s’arrêtent immédiatement. Rejeu : `python3 -S -B heap_sort_port.py` et `python3 -O -S -B heap_sort_port.py` ; commandes, SHA et sorties identiques dans `heap_replay.json`.

La porte native `sort_refusal` est raccordée (`fast`) : 48 tailles 17..64, trois positions de refus, raison/permutation et ordre réussi, 528 contrôles. Ajouter `CHECK_EQ(calls,fail_at)` graverait la propriété centrale sans étendre le scénario ; suggestion non bloquante.

**Limite : aucune qualification native nouvelle.** Les rapports lus décrivent le commit antérieur au correctif. Le modèle ne lance aucun C++, ASan, budget natif ni publication. Les effets globaux mémoire/publication sont établis par lecture du raccord ; les portes G4 restent attendues. `review.json` conserve cette distinction. L’ancien P1 est corrigé dans cette capture source.
