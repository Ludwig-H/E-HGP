# Audits courants de la v11

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves détaillées dans les reçus immuables.

**Corrections publiées en be05bfad8 et 98a009550, à qualifier sur G4.**

- **API multiprofil :** références fichier/manifeste par profil,
  journal conservé, six références u21 inchangées et six références
  u24 concordantes avec les reçus G4. La sonde reste identique à 38b76701b.
- **Mutant CLI :** mutation de `order_params` raccordée à la porte
  points qui l’atteint ; 28 individus conservés.
- **Matrice :** exigence LiDAR retirée du seul lot court ; portes et
  planchers conservés dans les lots d’échelle. Le refus impossible
  est corrigé, les sélections couvrent les reprises nécessaires.

[Relecture indépendante des corrections publiées](../receipts/audit_gates_fix_closed_20261005/README.md).
Les campagnes corrigées et les occurrences auparavant manquantes
restent à exécuter ; aucun PASS historique n’est changé rétroactivement.

**Chaîne G4 close, source 38b76701b.** Les sessions closes
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
| [Mesure finale](../receipts/audit_g4_finmesure_20261005/README.md) | 52 appels concordants ; deux contrôles supports W48 conformes | Les 18 prises chaudes K5/W48 dépassent toutes 100 ms pour l’étage `tree` ; points/plat non mesurés |

**Reprise G4 en cours sur 98a009550.** Le plan adopté comprend
quatre sessions successives : R1 lots courts des quatre profils ;
R2 huit lots d’échelle couvrant les 41 absences et les identités
supports ; R3 mutants, dont API/CLI ; R4 quatre lots ASan/UBSan u24.
Le préflight R1 est lu ; aucun résultat de reprise n’est encore acquis.
L’arrêt ciblé certifié est exigé entre sessions. Les 28 portes longues
fonctionnelles déjà terminées ne sont pas rejouées. La
[proposition antérieure limitée aux 41 absences](../receipts/proposition_reprise_41_20261005/README.md)
reste archivée ; le plan adopté utilise la matrice corrigée publiée.

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

**Décision du banc corrigée en be05bfad8.** Il mesure FULL et supports/L2b,
qui utilisent tous deux la voie FULL. Son ancien verdict
`build_order_par_defaut` ne peut donc plus choisir entre FULL et
l’arbre K seul : il est désormais remplacé par `sans_objet_post_l2b`.
Le plan de cette ancienne capture passe aussi le commit `b319efc84` comme simple étiquette, alors que la
session est épinglée à `38b76701b`. Conserver les mesures pour leur
périmètre réel et rectifier leur attribution. Points et plat ne sont
pas mesurés par ce banc.
[Preuve de source et correction ciblée](../receipts/audit_finmesure_scope_20261005/README.md).

**Le contrat de 100 ms reste ouvert au pin final.** Les 18 prises
chaudes K5/W48 dépassent toutes 100 ms pour l’étage `tree` : forêts
1..5 et verticales, hors catalogue, rattachement et sorties. Réduire
cet étage reste nécessaire. La qualification attend les reprises
ciblées ci-dessus. Les reçus antérieurs et les acquis GPU conservent
leurs sources et leur périmètre ; aucun transfert au pin final.

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
