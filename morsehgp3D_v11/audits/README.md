# Audits courants de la v11

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves détaillées dans les reçus immuables.

**Deux corrections de qualification à livrer.**

- **Attendus supports multiprofil.** Les six portes `api_supports_route`
  d’échelle/LiDAR imposent des hashes u21 aux profils u18/u24. Finm
  confirme ce défaut sur le témoin u18 à 8k ; fins le confirme sur les
  six cas u24 : les voies et l’appel public concordent, puis le juge
  refuse seulement les deux hashes. Conserver ces comparaisons exactes
  et adapter les attendus au profil. Aucun défaut moteur établi.
  [Constat et correction proposée](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#fina2--attendu-de-qualification-l2b-à-corriger-par-profil),
  [six preuves u24](../receipts/audit_g4_fins_20261005/supports_route/README.md).
- **Mutant CLI devenu inactif pour sa porte.** `sp_masque_16379`
  modifie `order_params`, que supports n’appelle plus depuis L2b.
  Conserver la mutation et la raccorder à `mhgp11_cli_points`, qui
  atteint cette fonction ; adapter son nom et sa note. Le moteur et
  l’oracle supports restent inchangés. Le nouveau raccord reste à juger.
  [Preuve et raccord proposé](../receipts/audit_g4_finm_20261005/cli_mutant/README.md).

**Qualification G4 finale, source 38b76701b.** Les sessions closes
ci-dessous ont des sources vérifiées et un arrêt ciblé certifié.

| Lot | Résultat vérifié | Portée encore ouverte |
| --- | --- | --- |
| [Fina2](../receipts/audit_g4_fina2_20261005/README.md) | Portes S9/S10 prioritaires PASS en u18/u21/u24/poison | Six échecs supports par profil u18/u24 ; 41 occurrences sans résultat à l’échéance |
| [Finm](../receipts/audit_g4_finm_20261005/README.md) | 461 mutants détectés sur 462 jugés, dont tous les head/points/num | Un survivant CLI expliqué ci-dessus ; 23 API non jugés après refus du témoin |
| [Fins](../receipts/audit_g4_fins_20261005/README.md) | TSan u21 80/80 ; ASan/UBSan u24 74/80 ; mêmes 80 portes d’échelle/LiDAR terminées | Six refus du juge aux hashes u21 ; aucun diagnostic sanitizer trouvé |
| [Finb](../receipts/audit_g4_finb_20261005/README.md) | 783/783 portes courtes sous chacun des deux profils sanitizer | Différentiels longs distincts |
| [Finl](../receipts/audit_g4_finl_20261005/README.md) | 28/28 portes longues hors mutants : onze K10 et dix-sept références FULL | Bilan brut 35/41 : six campagnes mutants u21 sans résultat après échéance ; aucune identité longue manquante |
| [P10](../receipts/audit_g4_finp10_20261005/README.md) | 4/4 différentiels de la sortie plate : synthétiques et trois trames entières | Sur le même arbre de points natif ; son contrôle propre est dans P9 |
| [P9](../receipts/audit_g4_finp9_20261005/README.md) | 4/4 différentiels de l’arbre de points : sites, dates, plateaux et parents | Sur le même catalogue et la même tour FULL exportés |

**Reprise ciblée préparée.** Les 41 absences ordinaires sont onze
noms CLI −O, répartis 8/11/11/11 entre les quatre profils. La matrice
proposée conserve leurs profils, poison et délais, et sélectionne
exactement ces occurrences. Elle nécessite l’intégration d’un seul
JSON sous `tools/` au commit corrigé poussé avant exécution. Les
six supports_route u18/u24 et les deux campagnes mutants API/CLI
restent des reprises séparées après correction. Aucun rejeu des
28 portes longues fonctionnelles déjà terminées n’est demandé.
[Plan, intégration et contrôle indépendant](../receipts/proposition_reprise_41_20261005/README.md).

**Les huit différentiels complets S10/S9 passent.** P10 compare la
sortie plate native à la tête Python sur le même arbre de points :
partitions, bruit et étiquettes canoniques, avec EOM z=1/2/3 et feuilles.
P9 compare séparément l’arbre natif à la construction Python. Chaque
lot couvre ses cas synthétiques et les trois trames sans sol entières,
au même pin final en Release u21.
[Plan S10](../receipts/audit_s10_differential_plan_20261005/README.md),
[plan S9](../receipts/audit_s9_qualification_scope_20261005/README.md).

**Corrections du moteur confirmées.**

- **S10**, garde publiée en **510dae50e**, témoin exact en **38b76701b** :
  les racines au-delà de `2^100` passent au calcul exact avant toute
  conversion signée. La frontière `R=2^127−1`, ses groupes et son
  égalité EOM passent dans les quatre profils ordinaires puis sous
  ASan/UBSan u24 et TSan u21. [Preuve et correction](../receipts/audit_s10_root_guard_20261005/README.md),
  [constat initial](../receipts/audit_s10_wip_20261005/math/README.md).
- **S9**, **3d47eaa93** : le tri s’arrête au premier refus. La porte
  ciblée passe dans les quatre profils ordinaires puis sous les deux
  sanitizers. [Correction et preuve](../receipts/audit_s9_sort_fix_20261005/README.md).
- **L2b**, **0810962ac**, diagnostic corrigé en **311ef5e3c** : supports
  utilise la voie FULL choisie sur G4 ; le journal n’est plus haché
  lorsque le diagnostic n’est pas demandé. Les six identités de voies
  passent notamment sous TSan u21. [Décision mesurée](../receipts/audit_l2_decision_20261005/README.md),
  [raccord et limites](../receipts/audit_l2b_followup_20261005/README.md).

**Portée du banc final à corriger.** Il mesure FULL et supports/L2b,
qui utilisent tous deux la voie FULL. Son ancien verdict
`build_order_par_defaut` ne peut donc plus choisir entre FULL et
l’arbre K seul : désactiver cette décision historique. Le plan passe
aussi l’ancien commit `b319efc84` comme simple étiquette, alors que la
session est épinglée à `38b76701b`. Conserver les mesures pour leur
périmètre réel et rectifier leur attribution. Points et plat ne sont
pas mesurés par ce banc.
[Preuve de source et correction ciblée](../receipts/audit_finmesure_scope_20261005/README.md).

**Le contrat de 100 ms reste ouvert.** La référence FULL qualifiée
K5/u21/W48 est à 352–412 ms sur trois trames sans sol de la séquence08.
La qualification de la nouvelle chaîne native complète reste à
terminer. Les acquis GPU gardent leur périmètre propre. Les reçus
[A2 antérieur](../receipts/audit_g4_a2_20261005/README.md),
[W48](../receipts/audit_s7_20261005/README.md),
[B antérieur](../receipts/audit_g4_b_20261005/README.md) et
[S antérieur](../receipts/audit_g4_s_20261005/README.md) conservent
leurs sources et limites ; ils ne sont pas transférés au pin final.

**Notes de travail et dialogue.**

- [Mathématiques : supports, hiérarchies et sélection](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md).
- [Question du développeur sur les 100 ms](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
- [Audit indépendant maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).
- [Contrat de la hiérarchie de points](../docs/HIERARCHIE_POINTS.md).

L’[audit général du 5 octobre](../receipts/audit_geant_20261005/README.md)
et les deux notes détaillées conservent les preuves des tranches
précédentes. Les échanges clos sont
[archivés avec leurs empreintes](../receipts/audit_dialogues_20261004/README.md),
[contrat des supports compris](../receipts/audit_supports_contract_20261005/notes_before/README.md).
Aucun nouveau build, test natif ni GCP lancé par les auditeurs.
