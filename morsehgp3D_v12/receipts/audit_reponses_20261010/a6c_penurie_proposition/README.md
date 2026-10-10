# CST-0245 : proposition de portes ON/OFF

10 octobre 2026. Source épinglée `2aaed1847e63ff86550db1fb1ba6e36313aa58bc`.
Proposition de tests seulement, **non appliquée, non compilée, non exécutée**.
Phase exploration_v12_hors_registre ; cpu_reference / cuda_g4 pour le
catalogue ; objet full_pi0 ; quantized_u21_input_only ; public_status=not_claimed.
Ce reçu ne ferme pas CST-0244/0245 et ne qualifie aucun nouveau chrono.

Appliquer d'abord la [proposition CST-0244](../a6c_admission_proposition/proposition.patch),
puis [proposition.patch](proposition.patch), limitée à deux fichiers de tests.
Les trois temps du produit sont appelés via leurs API existantes ; aucun
algorithme de tour, compteur d'allocation produit ou CTest nouveau n'est ajouté.

| Porte existante | Extension proposée |
|---|---|
| `pipeline_unit:admission` | Les 24 petites fixtures actuelles puis une grille K2, chacune OFF et ON ; seuil fixé avant `open_session`. Masque de tous les ordres, N/H positifs ON et nuls OFF. Pic réellement observé sous la borne. Quand l’admission est liante, budget `used_open + admitted` accepté ; un octet de moins refusé **à admit_session**, usage et pic inchangés. Au moins huit cas liants par mode, aucun succès obtenu par simple absence de cas. |
| `pipeline_unit:admission` | ON→OFF et OFF→ON après ouverture : `tower_invariant`, sans réservation ; restauration par un seuil numériquement différent mais équivalent, puis admission/exécution réussies. Retour à l'usage de base après destruction. |
| `pipeline_fault:allocation` | Corps conservé : trois allocations levantes du véritable `build_tower`, refus sous injection, retour des ressources et reprise FUL1. |
| `pipeline_fault:penurie` | Grille 27 sites/K2, OFF/ON via `SessionRun` sous `guarded`, seuil fixé avant ouverture. Chaque allocation sans exception injectée à un fil : **toutes** doivent refuser `memory_budget`. À trois fils, échantillon calibré sur le compteur de cette largeur. Retour au budget de base après chaque tentative, reprise et identité FUL1 avec le témoin public. |

## Pourquoi la fixture change

Le nuage aléatoire actuel de pénurie ne garantit aucune cohorte non triviale
hors ordre 1. Or `forest_births.cpp:130–132` n'alloue les tampons temporaires
`Sphere` et `local` de N que pour `k>=2 && widest>1`. Forcer ON seul ne suffit
donc pas à prouver que le balayage d'injections touche ces allocations.

La grille 3×3×3 de pas 7, K2, fournit 54 paires axiales adjacentes : distance
7, milieux distincts, rayon 7/2, exactement les deux extrémités sur chaque
sphère et aucun autre site dans sa boule. Avant ce rayon, aucune paire de
boules de sites ne s'intersecte ; au niveau commun, ces 54 milieux donnent
une cohorte de naissances K2. `check.py` vérifie ces propriétés par entiers
sur la fixture synthétique, sans moteur. La porte native exigera également
`max_cohort>1` à l'ordre 2, masque ON `0b11` et travaux N/H positifs ; une
fixture qui ne parcourt pas la voie attendue fera donc échouer le test.
A6c engage tous les ordres au pin, pas seulement l'ordre maximal.

Le balayage complet à un fil inclut alors les allocations temporaires de N.
Il vérifie refus et restitution, sans devoir prédire leur numéro d'allocation
ni copier la formule mémoire du produit. Le seuil numérique `total>=100`
de l'ancienne fixture aléatoire est remplacé par les conditions structurelles
ci-dessus et l'injection de chacune des allocations comptées. La porte
publique d'allocations levantes conserve sa fixture aléatoire 70 sites/K4.

Seconde revue statique : `guarded` accepte `Outcome` et `Result<Tower>` ;
les captures restent vivantes jusqu'au retour synchrone, le Pool rejoint ses
ouvriers avant les relevés W3, et `TowerDiagnostics` ne possède que des
compteurs/tableaux fixes. H positif sur K2 est justifié : les naissances non
vides donnent des tâches `kDepth`, tandis que `kHistory` garde sa tâche CSR ;
`run_step` compte les deux (`pipeline_run.cpp:353–359,393–394`). Les allocations
de N, du CSR H, des tables/radix et des espaces census relues propagent leur
refus ; aucun essai d'allocation facultatif avec repli n'a été trouvé dans
ces voies au pin. Sans cache, le balayage W1 garde donc ses refus obligatoires.

Correction de la contre-lecture root : `digest_of` peut rendre la chaîne
`"refus"`. `attempt` valide désormais toute empreinte demandée (64 caractères
hexadécimaux minuscules), après désactivation de l'injection et photographie
des compteurs ; sinon il rend `tower_invariant`. Deux sentinelles égales ne
peuvent plus faire accepter le témoin ou la reprise, y compris dans la porte
publique d'allocations levantes dont le corps reste inchangé.

## Limites et rejeu

Budgets des portes sans cache : chaque allocation neuve atteint l'injecteur,
les tailles sont exactes. Aucun transfert au cache de 8 Gio, à la pénurie
CUDA, aux très grandes cohortes ou à tous les ordonnancements concurrents.
L'échantillon à trois fils admet succès ou refus mémoire, jamais fuite ;
l'exhaustivité et les refus obligatoires portent sur un fil.
La modification augmente le nombre de tentatives ; durée et seuil des huit
cas liants par mode restent à valider nativement, pas à assouplir silencieusement.
Une durée dépassant les délais existants (admission 600 s, pénurie 300 s),
une fixture non liante ou une empreinte refusée sont à distinguer d'un défaut
A6c ; aucune de ces conditions n'a été mesurée par ce reçu.

Relecture statique sans écriture : `python check.py /workspaces/E-HGP`, puis
`python -O check.py /workspaces/E-HGP`. Elle applique les deux patches en
mémoire, contrôle hashes et ancrages, et rejoue la géométrie synthétique.
Elle ne valide ni compilation C++, ni sorties, ni compte d'allocations natif.
L'adoption demandera les portes existantes admission/refus/allocation/pénurie
sur la composition précise, avec statut, logs et sources/binaires archivés.
