# Contrelecture mathématique S9 WIP — 5 octobre 2026

Les chemins mathématiques examinés, lorsque les comparaisons exactes sont acceptées, concordent avec l'oracle de définition sur les petits témoins. Cette contre-épreuve ne clôt pas S9 : le défaut du tri natif après refus d'une comparaison, signalé séparément par l'auditeur natif, n'est pas exercé par le modèle Python.

Cadre `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. Capture du worktree développeur `build/v11-impl-l3`, HEAD `53c027fe848b0d890f164eb87ebf347338c58d55` avec modifications S9 non commises. Ensemble de 131 fichiers capturé deux fois pour contrôler la cohérence des octets et empreintes ; cet inventaire n'est pas une revue ligne par ligne des 131 fichiers. Inventaire et octets identiques, HEAD inchangé ; SHA256 avant/après dans `source_manifest.json`. 34 fichiers diffèrent du pin : copies figées dans `overlay/`. Le snapshot comporte déjà les corps de points et ancestor_index, pas seulement des interfaces. Aucun rapport `impl_s9.md`/`verif_s9.md` n'était présent au moment de cette capture. Les évolutions ultérieures du développeur ne sont pas transférées à cette preuve.

Aucun build, appel natif, GCP, campagne S8 ou grande suite Python exécuté. Le C++ est lu ; le harnais traduit des opérations dans Python entier/Fraction et n'exécute pas le produit.

## Contrôles importants du raccord

`src/points/incidences.cpp` reprend les incidences fortes (p+q_min≤K) de tous les sites intérieurs et de coquille ; les sites orphelins de Q sont donc conservés. K1 utilise les feuilles-sites, au rang nul. `qualify.cpp` compte chaque paire naissance/site une fois à sa première incidence, puis prend le (K+1)-ième rang ; une fusion est qualifiée à sa naissance. La contre-épreuve compare chaque qualification directement à la première coupe fermée de Gamma_K couvrant au moins m sites, sans réutiliser ce calcul par incidences.

`hang.cpp` remonte une incidence non qualifiée au premier ancêtre strict qualifié, garde le premier départ de rang minimal et maximise la marge positive en rayon. Les dominances de rang sont sûres par monotonie de sqrt ; les autres choix demandent la comparaison exacte de deux racines contre deux. Le propriétaire est recherché à la coupe fermée de la date. L'index Myers construit ses sauts à partir des profondeurs ; la contre-épreuve compare ses LCA aux remontées simples du véritable arbre de définition.

`settle.cpp:69` certifie le prédicat au plancher et son échec au rang suivant sur TOUT Cat_K, indépendamment des rangs LEVELS exportés. La borne haute min(M, rang du parent−1) est valide puisque t≤Q implique la date≤sqrt(level[M]) et que le propriétaire est vivant à la date. Les lignes 103–107 contrôlent cette vie et les rangs de la marge. K=n, K≥2, est explicitement refusé avant calcul : la qualification K+1 serait impossible, conforme à la frontière auparavant ouverte.

`point_tree.cpp` traite toutes les fusions d'un rang avant les entrées non strictes de ce rang, puis groupe les dates strictes par égalité exacte. Les fusions FULL d'un plateau ne sont pas des fusions imbriquées : les parents de la forêt ont des rangs strictement plus grands. Le modèle vérifie les partitions à chaque plateau contre l'ultramétrique exacte de l'oracle, ainsi que `naissance(bloc) ≤ entrée(site) < mort(bloc)` et l'ordre strict des plateaux. Il utilise un tri Python avec comparateur exact total, sans refus ; il ne reproduit donc pas le changement de comparateur dans le std::sort natif en échec.

## Nouvelle exécution bornée

`check_points.py` : six témoins, singleton K1, plateau K1 et K2, cinq sites F5 K2, huit sites F8 K2, équilatéral K2 ; m=1 à K1, m=3 à K2. Les incidences sont tirées du S1 exact (étage A), les qualifications, dates et propriétaires sont comparés aux coupes fermées exactes indépendantes de `points_oracle_stdlib.reference_radius`. Les partitions de l'arbre sur les points sont comparées à l'ultramétrique de cet oracle.

Résultats : 43 qualifications, 387 LCA et 29 propositions de rang plancher vérifiés. Pour chaque date, TOUTE proposition possible dans l'intervalle de recherche est réparée par le galop/dichotomie traduit vers le rang maximal obtenu indépendamment par scan du catalogue. 28 sites, deux pendaisons retardées (dont une stricte), 15 plateaux et 17 blocs au total. Vie du bloc à chaque entrée et atomicité des plateaux conformes. Pas de comparaison native, pas de test au hasard étendu.

La lecture des nouveaux juges confirme leur portée : `tests/points/points_vs_python.py:compare` compare exactement floor/strict, plateau, parent du bloc et entrée de chaque site à la chaîne Python ; `points_oracle_stdlib.py:compare` compare directement dates/propriétaires et partitions à chaque plateau à la définition, mais pas le rang floor. Le lecteur de fichier sparse, à lui seul, ne démontre ni le rang maximal dans le catalogue non publié ni toute la sémantique de vie des blocs ; aucune de ces limites n'est présentée comme défaut du moteur dans ce rapport.

Quatre invocations closes, code 0, aucun essai du harnais en échec : normal, `-O`, replay normal, replay `-O`. Sorties identiques octet pour octet : SHA256 `b33deed188c98292b1e792305d6a0d53b3e0b35e53d31662b2a6b70273b41d33`. Commandes, codes, stderr exacts et SHA des scripts conservés dans `executions.json`. La première source exécutée est conservée dans `attempts/initial_check_points.py`, identique à la finale.

## Reproduction sans worktree développeur

```sh
python3 replay.py /workspaces/E-HGP
python3 -O replay.py /workspaces/E-HGP
```

Le replay reconstruit le pin par Git, applique les fichiers `overlay/` et vérifie chaque SHA avant d'exécuter le harnais dans un dossier temporaire. Reprendre la capsule sauf `snapshot/` : rapport, manifeste, overlay, deux scripts, quatre JSON et quatre stderr, executions, SHA256 et la première source du harnais. Le résultat reste attaché à ce WIP précis et aux chemins sans refus ; aucune qualification S9 globale ou native nouvelle n'est revendiquée.
