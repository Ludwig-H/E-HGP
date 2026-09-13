# État de l'audit complémentaire v8

13 septembre 2026, troisième passe sur la publication `3589a2c9`.
Intervenant **AUDITEUR_COMPLEMENTAIRE**, distinct du développeur et de
l'auditeur historique. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Suites actives transmises au développeur

| Priorité | Constat vérifié et aide concrète |
| --- | --- |
| P0, nappes 2D | Le [prototype de sous-rectangles q2](P0_NAPPES_2D.md) accepte les vraies nappes tronquées n8k/16k/32k. À n32k/h10 : 1 865 300 candidates contre 256 millions, 530 706 descripteurs et 3 585 060 visites de témoins. Certification C++ actuelle, aucun census exécuté à cette taille. |
| P1, exception | L'[affectation de CreditPlan](P0_PLAN_ASSIGNMENT.md) peut mélanger propriétaire et crédits après `bad_alloc`, si l'appelant réutilise la cible. Défaut reproduit ; une copie suivie d'un échange corrige le cas dans un snapshot temporaire. |
| P2, lecteur | Le [contrôle R3](CAMPAIGN_RECEIPT_R3_CHECKS.json) découvre un refus indu pour une nappe avec voie inactive : n8/sheet/dual/q3/Kmax1. Conditionner l'invariant « toutes les paires restent » à un seuil actif. |
| P0, ordre des témoins | La [variante ordonnée](P0_ORDRE_TEMOINS.md) reste à comparer : 188 910 → 82 tâches sur la tige diagonale, mêmes crédits et 36 candidates. Construction et hausse locale des tests sur grille conservées dans le bilan. |
| P0, groupes | Les [groupes recouvrants](P0_GROUPES_RECOUVRANTS.md) peuvent contribuer par capacités sur les IDs : cinq triplets donnent deux crédits, contre un pour une sélection disjointe, sur un vrai support q4 positif. Recherche et raccord restent ouverts. |

Les constats et propositions sont dans le
[journal partagé](../COORDINATION_MORSEHGP3D_V8.md). **P0 reste ouverte** :
les gains bornés sur une famille ne paient pas encore les résidus généraux,
le census, q3/q4 complets ou la tour FULL. Le partage des préparations
entre voies actuellement développé n'hérite pas des qualifications R3.

## Corrections R3 reconnues

Les défauts de copie du propriétaire et d'alias du tampon d'entrée sont
corrigés dans les octets publiés : copie privée avant certification,
opérations de copie/déplacement du propriétaire supprimées. L'autre
auditeur conserve l'autorité de ses contre-fixtures ; notre
[contrelecture technique](P0_PLAN_ASSIGNMENT.md) distingue cette fermeture
du nouveau cas d'exception dans le plan.

Le [runner R3](CAMPAIGN_RECEIPT_R3_CHECKS.json) rejette les JSON contradictoires
et tronqués, garde les sorties brutes et vérifie le binaire jusqu'à la
fermeture. Sept cas indépendants passent en normal/−O, ainsi que les
41 contrôles constructeur dans chaque mode. Nos anciens P1/P2 d'admission
sont fermés sur ce runner. Le cas de voie inactive concerne le lecteur.

Les [quatre captures R3](CAMPAIGN_R3_MEASURES_CHECKS.json) passent aussi
le lecteur normal/−O et le contrôle indépendant des 729 tuples, matrices,
sorties brutes et hashes, soit 513 configurations. La lecture utilise
les blobs exacts de `3589a2c9` : le worktree modifié est refusé à juste
titre, sans transfert de qualification au chantier en cours.

Le [témoin initial](CAMPAIGN_RECEIPT_CHECKS.json) et les
[729 tuples initiaux concordants](CAMPAIGN_INITIAL_CHECKS.json) restent
conservés avec leurs pins ; aucune capture réelle falsifiée n'y est alléguée.
Les trois passes de mesure du constructeur ont des sources distinctes.

## Preuves et contrôles conservés

- [Nappes 2D](SHEET_RECTANGLES_CHECKS.json) : 42 cas par mode normal/−O,
  C++20 strict/UBSan ; 3 533 502 tests du juge sur les petites instances,
  18 constructions grandes par mode, mutant sans certificat réfuté.
- [Rangées transverses](P0_RESIDU_TRANSVERSE.md) : les crédits universels
  parfaits peuvent laisser toutes les paires. L'autre auditeur propose
  des [queues 1D et certificats collectifs](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md) ;
  notre prototype 2D prolonge la piste q2 sans remplacer son travail.
- [Rails](P0_RAILS.md) : pool global insuffisant à n2718, 1 846 881
  candidates contre 2 916 pour DualBlocks ; deux mutants compilés réfutés.
- [Quantificateurs](P0_QUANTIFICATEURS.md),
  [prédicats initiaux](PREDICATES_CHECKS.json),
  [tubes](TUBES_CHECKS.json) et [ordre Dual](DUAL_ORDER_CHECKS.json) :
  preuves, juges et limites propres aux octets épinglés, conservés intacts.

## Rejeu et entretien

```bash
python3 -B audits/morsehgp3D_v8_complementaire/sheet_rectangles_checks.py --selftest
python3 -B audits/morsehgp3D_v8_complementaire/plan_assignment_checks.py
python3 -B audits/morsehgp3D_v8_complementaire/campaign_receipt_r3_checks.py --selftest
```

Les deux premiers extraient leurs sources du commit publié dans des
répertoires temporaires neufs. Le dernier exige son runner, son lecteur
et son binaire R3 épinglés ; des sources modifiées demandent une nouvelle
qualification, jamais la réécriture d'un reçu clos. Les gates historiques
qui exigent leurs anciens hashes peuvent donc refuser le worktree courant.

Les sujets résolus sont retirés de la liste active ; les preuves restent
dans leurs notes et reçus nommés. Aucun fichier constructeur ou de l'autre
auditeur n'est repris dans nos commits. La porte documentaire générale
exclut ces audits : validation explicite de nos Markdown en complément.
GCP non utilisé.
