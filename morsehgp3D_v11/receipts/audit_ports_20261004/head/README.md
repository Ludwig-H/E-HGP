# Recoupe E1 — 4 octobre 2026

Source figée : `723cf6e436f6cb6f524501cbd9a51bf0e17fe6fe`. La capture initialement appelée LIVE WIP est attribuée à ce commit par les huit égalités de blobs Git. Les huit fichiers relus restent identiques à la clôture ; le HEAD LIVE a avancé indépendamment à `49831e9e9fa2e2e44afc3b8b9affd7723302e11b`.

`points_flat_summary.py:230–234` raccorde le verdict H_L2 à `h_l2_claim` : p de Holm < 0,05 **et** IC basse > −0,02. Les deux anciens témoins (Holm ≈ 0,0301, IC basse −0,021 ou −0,020) sont désormais refusés. Le contrôle positif et les autres primaires positives restent acceptés. Aucun résultat P08 ni bootstrap réel n’est rejugé : cinq distributions sont injectées dans l’AST exact du décideur.

`points_flat_gate.py:38,152–154,272–274` ajoute EOM z=2 à la boucle de comparaison avec l’oracle indépendant. F4b attend trois groupes en z=2 contre deux en z=1, sur les mêmes neuf sites, k=2 et mcs=3. L’attendu est donc distinct de z=1 ; son exactitude géométrique n’est pas rejouée ici. La porte G4 mise à jour reste à jouer avant S3a/S3b/S6/S7, comme la documentation le demande. `tests/tower/tests.cmake:72` enregistre le selftest stdlib des règles de revendication.

`SORTIE_PLATE.md:113–117,161–175` corrige les portées : l’ancienne affirmation « toute EOM, à tout z » est retirée ; la tête reste Python après l’export C++ ; une contribution T−A non significative n’est pas dite nulle. Une précision demeure utile à la ligne 165 : l’`oracle_m05(cond,first,ids,truth)` des campagnes est une antichaîne optimale supervisée sur l’arbre condensé (`points_flat_campaign.py:98–129`), distincte de l’oracle indépendant de correction de la porte. Formulation conseillée : « z=2 était mesuré dans les campagnes, sans comparaison à l’oracle indépendant de correction de la porte ».

Validation : selftest source, 10 contrôles ; sonde autonome AST et source, 94 contrôles. Normal/−O produisent des sorties identiques. Les runs sont Python stdlib avec `-S` : aucun import produit/NumPy, fit, moteur natif, build ou GCP. Le premier contrôle d’audit échoue sur une virgule omise dans son littéral documentaire ; script et sorties sont conservés sous `first_attempt/`, puis le seul littéral est corrigé. Ce n’est pas un échec du produit.

Rejeu portable : `python3 -B -S check_head_followup.py`, puis `python3 -B -O -S check_head_followup.py`. `COMMANDS.json` nomme les commandes et sorties ; `BILAN.json` distingue correction source et qualification. Ce reçu n’hérite aucune qualification G4 antérieure sur un autre pin.

L’inventaire `LEDGER.json` couvre tous les payloads, sauf lui-même et le `SHA256SUMS` racine. Ce dernier couvre les payloads et le ledger ; seul son propre fichier est exclu.
