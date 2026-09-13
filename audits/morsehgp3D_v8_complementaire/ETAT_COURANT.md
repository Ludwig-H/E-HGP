# État de l'audit complémentaire v8

13 septembre 2026, quatrième passe après `65ac5ee6` ; code axial et
partage entre voies en construction, distincts des captures R3.
Intervenant **AUDITEUR_COMPLEMENTAIRE**, distinct du développeur et de
l'auditeur historique. Cadre : `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Suites actives transmises au développeur

| Priorité | Constat vérifié et aide concrète |
| --- | --- |
| P0, orientation | Une [rotation isométrique u16](P0_AXES_ET_ROTATIONS.md) fait passer le filtre axial de 6 483 670 à 256 millions de candidates à n32k/h10. Son repli est compact ; une famille de directions déclarée peut retrouver les colonnes, recherche générale ouverte. |
| P0, addition | Les [colonnes exactes](P0_SOMME_TEMOINS_AXIAUX.md) d'une même ancre ont des témoins disjoints. Leur somme peut renforcer le filtre actuel par maximum ; fixture minimale exécutée, borne de résidu calculée. |
| P0, autres pistes | Les [nappes 2D](P0_NAPPES_2D.md), l'[ordre des témoins](P0_ORDRE_TEMOINS.md) et les [groupes recouvrants](P0_GROUPES_RECOUVRANTS.md) restent des apports bornés à comparer, sans qualification aval ou FULL. |

Les constats et propositions sont dans le
[journal partagé](../COORDINATION_MORSEHGP3D_V8.md). **P0 reste ouverte** :
les gains bornés sur une famille ne paient pas encore les résidus généraux,
le census, q3/q4 complets ou la tour FULL. Le partage des préparations
entre voies actuellement développé n'hérite pas des qualifications R3.

## Corrections et partage nouvellement contre-vérifiés

L'[affectation après échec d'allocation](P0_PLAN_ASSIGNMENT.md) est
corrigée dans le snapshot décrit par [BATCH_EXCEPTION_REVIEW](BATCH_EXCEPTION_REVIEW.json) :
20 pannes injectées, cible conservée, 24 refus d'accès après déplacement.
Le partage Tubes passe 195 lots / 585 voies, puis 3 051 voies du juge
indépendant : crédits et résidus égaux aux appels isolés, préparation
comptée une fois. Normal et rejeu −O sur les sources embarquées concordent.
Aucun nouveau défaut relevé dans ces octets ; leurs hashes délimitent l'avis.

L'ancien P2 `sheet` inactive est clos sur le lecteur single `0ebf0c55…` :
une vraie capture q3/Kmax1 avec seuil et résidu nuls passe normal/−O.
Le [reçu apparié](PAIRED_PROVENANCE_CHECKS.json) conserve cette fermeture
séparément des deux points ensuite corrigés dans les lecteurs appariés :
`f4148e22…` refuse le mélange de builds et conserve leur identité dans
les résumés ; `212efc6e…` rejette le marqueur de checksum axial erroné.
Les contrôles positifs et négatifs passent en normal/−O sur les captures
conservées. Les [trois constats de protocole](P0_RECUS_APPARIES.md) sont
donc clos sur ces pins, sans déclaration de campagne réelle contaminée.

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
sont fermés sur ce runner. Le cas de voie inactive du lecteur a été
fermé lors de la quatrième passe ci-dessus.

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

- [Filtre axial](AXIS_Q2_IDENTITY_CHECKS.json) : O2 et UBSan, 131 plans,
  24 143 paires, 874 128 tests du census ; quatre mutants C++ rejetés.
- [Rotations](AXIS_ROTATION_CHECKS.json) : 27 configurations × trois plans
  par mode, neuf configurations avec census ; 3 709 380 tests ponctuels,
  18 grandes configurations sans développement des candidates.

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
python3 -B -O audits/morsehgp3D_v8_complementaire/batch_exception_checks.py --replay audits/morsehgp3D_v8_complementaire/BATCH_EXCEPTION_REVIEW.json
python3 -B -O audits/morsehgp3D_v8_complementaire/axis_rotation_checks.py --selftest --replay audits/morsehgp3D_v8_complementaire/AXIS_ROTATION_CHECKS.json
```

Ces reçus embarquent les octets nécessaires à leurs replays. Les notes
antérieures conservent leurs propres commandes et pins : certaines gates
refusent donc légitimement le worktree courant. Une correction nécessite
une nouvelle qualification, jamais la réécriture d'un reçu clos.

Les sujets résolus sont retirés de la liste active ; les preuves restent
dans leurs notes et reçus nommés. Aucun fichier constructeur ou de l'autre
auditeur n'est repris dans nos commits. La porte documentaire générale
exclut ces audits : validation explicite de nos Markdown en complément.
GCP non utilisé.
