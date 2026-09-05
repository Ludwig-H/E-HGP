# Reçu natif MEB v2 : qualification et coût local

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans les audits. Cette contrelecture ne lance aucun compilateur, moteur ou benchmark ; elle rejoue des lecteurs de preuves.

**Le raccord local de `NoObserver` est fermé sur le corpus du reçu constructeur v2.** La proposition réduit le nombre de candidats de la matrice P401, mais son coût ne justifie aucune activation générale : le cas de deux points répété ralentit, et les petits lots révèlent un effet d'ordre. La [preuve MEB courante](../MEB_DOUBLE_BUDGET_COURANT.md) garde l'autorité mathématique ; ces résultats qualifient une instanciation privée supplémentaire, sans intégrer le helper dans F.

## Autorités conservées

| Pièce | Autorité et portée |
| --- | --- |
| [Capture et références](qualification/capture_manifest.json) | Sources, reçu de build v2, désassemblage, reçu de mesure, sorties complètes ; octets identiques aux originaux, sans ELF versionné |
| [Vérificateur portable](qualification/verify_native_receipt.py) et [revue de fermeture](qualification/review.json) | Sceaux, commandes, scope, inventaire, captures et comptabilité ; quatre positifs et dix-neuf rejets par mode Python |
| [Lecture du harnais](harness_review.md) | Champs effectivement comparés, Work cumulatif, appels conservés entre les horloges, coûts inclus et limites |
| [Analyse des observations](cost_review.md) | Inventaires, coût des candidats, temps appariés, lots courts et distinction AB/BA |
| [Protocole de suivi conservé](followup_protocol.md.txt) et [contrelecture du plan](followup_review.json) | Réponse déjà préparée aux limites de v2, pas une campagne exécutée |

Le reçu de mesure est `874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208`.

Il est clos de 11:50:13,229548 à 11:50:16,210694 UTC. Le build strict C++20/O2 est distinct, reçu `de6de29f55ab55d8edd64f9e3307d4748688635ca7338c36105555da39e0574f` ; le binaire `56e022c817d2e726eb2e3b135e78e577bbdf344ebd0ff352d64d1121300fd976` est sans sanitizer ni macro de test. Le premier échec de compilation v1 reste conservé : sa collision de macro avec `Metrics::main` ne constitue ni un défaut MEB ni une mesure.

## Comparaisons et unités

Le harnais réutilise les 176 scènes/384 ordres du donneur, ajoute son triangle cumulatif séparé et douze jobs répétés sur les deux ordres de la même paire. Il qualifie 9 347 jobs représentant **9 351 états**, chacun comparé à F et à Trace avant puis après mesure. Les deux passages portent sur les mêmes entrées : ils ne doublent pas la diversité du corpus. Les caps restent choisis depuis le rang F pour cette matrice ; le juge rationnel indépendant antérieur garde son attribution propre.

Les comparaisons hors chrono portent sur booléen, diagnostic, treize statistiques, événements sentinelles, clé entière, support complet et niveau q4 brut. Work est comparé à Trace et reste persistant entre les étapes cumulatives. Les boucles mesurées consomment des captures 64 bits contrôlées : ces hashes ne sont pas un nouveau certificat arithmétique.

| Inventaire du reçu | Nombre |
| --- | ---: |
| Ordinaux combinatoires | 1 507 |
| Appels supérieurs par bras et passage | 58 491 |
| Groupes / lignes de temps | 4 699 / 84 582 |
| Chauffes / passages mesurés | 2 / 7 |
| Entrées MEB, juges et replis imbriqués compris | 1 325 812 |
| Plafond d'entrées | 2 000 000 |

L'intervalle du reçu, environ 2,98 s, comprend qualification, mesures, écritures et fermeture après le préflight. Le sous-processus dure 1,270556 s et la somme des timers 0,691921 s. Le plafond de 120 s démarre après l'admission des sources et anciens reçus ; la durée complète d'invocation du runner n'est pas mesurée. Aucun de ces nombres n'est le temps d'une tour. Le coût des lots entoure aussi les resets, les sentinelles, les wrappers et les captures. CPU6 et un thread sont déclarés et contrôlés par le runner ; l'hôte partagé et la fréquence restent des limites d'interprétation.

## Résultat utile et suite déjà préparée

À P401, les 1 152 cas principaux passent de **67 884 candidats F à 10 722 candidats physiques**, propositions et replis réunis. Il y a 795 réductions, 159 égalités et 198 augmentations. Cette somme inclut succès et refus du corpus fermé ; elle ne compte ni distances, ni puissances, ni copies. Elle ne suffit donc pas à prédire la latence.

Les répétitions du cas n=2 exposent le surcoût local ; les petits lots de la matrice ne permettent pas d'attribuer un gain général. Conserver P0 par défaut est cohérent avec cette observation et le contrat de port. Le [suivi déclaré après v2](followup_review.json) garde tous les ordres, un L551 uniforme, P0/P401, 64 répétitions et cinq paires AB/cinq BA. Sa borne conservatrice est vérifiée : 1 779 072 entrées. Aucune mesure de ce suivi n'est attribuée ici. P0 reste un contrôle d'enveloppe ; ni sa soustraction ni un seuil choisi après observation ne sont autorisés par le plan.

La qualification du futur Builder doit encore établir la durée de vie du budget par ordre, le repli sans proposition et les schémas publics. Un gain de tour devra être mesuré sur cette route intégrée. Aucun Gamma exhaustif ni nouvelle structure globale n'est ajouté par le helper privé. Statut `not_claimed`. GCP non utilisé.

## Rejouer sans moteur

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_meb_native_20260905/qualification/verify_native_receipt.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_meb_native_20260905/qualification/verify_native_receipt.py --self-test
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_meb_native_20260905/cost/review_cost.py
PYTHONDONTWRITEBYTECODE=1 python3 -O morsehgp3D_v7/audits/receipts_meb_native_20260905/cost/review_cost.py
```

Les lecteurs de qualification relisent la capture sans écrire ni appeler le C++. Leurs quatre positifs et dix-neuf rejets passent normalement et sous `-O`. La vérification live ajoute une observation des octets encore présents ; elle n'est pas nécessaire au rejeu portable. Un premier rejet de l'inspecteur exigeait à tort des PID distincts entre deux stages séparés : le PID 80 a été réutilisé. Le correctif vérifie l'unicité dans chaque stage ; le premier échec reste conservé, sans attribution au produit ni nouvelle campagne moteur.

Le lecteur de coût régénère seulement son résumé normal/optimisé et vérifie les 50 groupes détaillés conservés ; les 4 699 groupes restent recalculables depuis le brut complet. Trois corruptions d’audit exercent paire manquante, bras manquant et ratio de médianes non appariées. Ce sont des contrôles du lecteur statistique, pas des mutants de géométrie. Les [contrôles de publication](publication_checks.json) lient ces pièces au manifeste courant.
