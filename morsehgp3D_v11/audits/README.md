# Audits courants de la v11

5 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves détaillées dans les reçus immuables.

**Finm : raccorder le mutant CLI à une porte qui l'atteint.**
`sp_masque_16379` modifie `order_params`, devenu inactif pour
`supports` depuis L2b. Conserver la mutation et la jouer sur
`mhgp11_cli_points`, qui utilise encore ce paramétrage ; le moteur
et l'oracle supports restent inchangés. La campagne détecte
461 mutants ; celui-ci survit, et les 23 API ne sont pas jugés après
le refus de leur témoin u18 sur les hashes ci-dessous.
[Reçu et périmètre](../receipts/audit_g4_finm_20261005/README.md),
[raccord proposé](../receipts/audit_g4_finm_20261005/cli_mutant/README.md).

**Fina2 : corriger l'attendu multiprofil de la porte L2b.** Sur
**38b76701b**, les six portes `api_supports_route` d'échelle et LiDAR
échouent en u18/u24 et passent en u21/poison. Leur déclaration CMake
grave les mêmes hashes de fichier et de manifeste issus d'u21 pour tous
les profils, alors que ces deux objets portent les bits : cet attendu doit dépendre du
profil. La sortie précise des échecs manque dans l'archive ; aucun
défaut produit n'en est déduit. Cette porte sert aussi de référence au
mutant `voie_supports_order_tree`, dans la campagne u18.
Finm confirme ce mécanisme sur le témoin u18 à 8 000 points : les
voies et l'appel public concordent, mais la ligne aux SHA u21 est refusée.
[Constat et reprise ciblée](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#fina2--attendu-de-qualification-l2b-à-corriger-par-profil).

**Fins : les lots sanitizer d’échelle et LiDAR sont terminés.** Au
même pin **38b76701b**, **80/80** portes TSan u21 passent, dont les
six identités L2b. ASan/UBSan u24 termine **74/80** portes avec succès.
Pour les six échecs, les sondes déclarent les voies conformes, puis
le juge refuse leurs hashes u24 contre ses attendus u21. Aucun diagnostic sanitizer
dans les journaux examinés. Aucune porte manquante ni coupure.
Points, sortie plate et `roots_cost` passent
sur les trois tailles et trois trames dans les deux configurations.
Les contrôles sanitizer courts sont dans la session B suivante.
[Reçu vérifié et périmètre](../receipts/audit_g4_fins_20261005/README.md).

**Priorité : qualifier l'assemblage final sur G4.** Supports, arbre K
seul, points et tête plate sont publiés. S10 arrive en **076d9142b**,
sa garde numérique en **510dae50e**, son témoin exact en **38b76701b**.
La session gardée `v11.20261005.claudefina2` est close sur ce dernier
commit, avec `data_complet` et arrêt ciblé certifié. Les quatre
configurations ordinaires restent partielles à l'échéance ; leurs
portes S10 (unitaires, CLI, échelle et LiDAR) passent toutes. Les
échecs L2b ci-dessus doivent être traités avant qualification globale.
[Reçu vérifié et périmètre](../receipts/audit_g4_fina2_20261005/README.md).

Les plans suivants conservent les mutants, les onze lots sanitizer et
les **huit différentiels complets S9/S10**. Les deux sessions
Python épinglées sont adoptées par le développeur, successives au même
commit final : l'une qualifie l'arbre de points, l'autre toutes les
étiquettes plates, le bruit et les identifiants canoniques. Le lecteur
de fichiers seul ne remplace pas ces comparaisons.
[Plan S9](../receipts/audit_s9_qualification_scope_20261005/README.md),
[plan S10](../receipts/audit_s10_differential_plan_20261005/README.md).
Le plan B de la chaîne finale complète explicitement les onze lots S
avec les portes sanitizer courtes, dont la frontière S10 et le refus S9.

**S10 : constat numérique fermé dans les sources.** La garde `2^100`
précède les conversions signées et renvoie les grandes racines de l'API
abstraite au calcul exact. Les deux sources correctives sont identiques
à la capture relue ; sommes et marges tiennent en `i128`. Le test
renforcé détecte la suppression de la garde et vérifie aussi la racine
exacte `R=2^127−1`, ses groupes et l'égalité EOM. La condensation,
les réciproques z=1/2/3 et la propagation des refus sont relues sans
nouveau défaut important établi. Les portes ordinaires passent dans
fina2 ; les portes d’échelle et LiDAR passent aussi sous sanitizers
dans fins. Le complément court et le différentiel complet restent à qualifier.
[Correction et portée](../receipts/audit_s10_root_guard_20261005/README.md),
[preuve initiale](../receipts/audit_s10_wip_20261005/math/README.md).

**S9 : refus du tri corrigé en 3d47eaa93.** Le tri par tas s'arrête au
premier refus, sans changement de relation d'ordre. Les sources
correspondent à la capture favorable ; le modèle vérifie arrêt, bornes
et permutation. La porte native du refus passe dans les quatre profils
ordinaires de fina2 ; le refus sous sanitizers et le différentiel
complet restent leurs propres portes.
[Correction et preuve](../receipts/audit_s9_sort_fix_20261005/README.md).

**L2b : raccord livré, qualification à terminer.** La mesure G4 appariée
avait imposé de raccorder le journal des graines à la voie concurrente
FULL pour `supports`. L2b est publiée en **0810962ac**. Le correctif
**311ef5e3c** supprime le hachage diagnostic de tout le journal quand il
n'est pas demandé ; les trois fichiers correspondent à la capture
relue. L'identité des voies à W1/W4 passe en u21/poison dans fina2.
Les six identités passent aussi sous TSan u21 dans fins ; les attendus
u18/u24 ci-dessus restent à corriger.
[Décision mesurée](../receipts/audit_l2_decision_20261005/README.md),
[correction et limites](../receipts/audit_l2b_followup_20261005/README.md).

**Acquis G4 antérieurs, avec leurs sources propres.**

- **A2, b319efc84 :** u18/u21/u24/poison conformes,
  **914/824/824/825 tests**, aucun résultat manquant, arrêt ciblé certifié.
  Les portes longues et le complément supports W48 sur ng00/ng02 sont
  conformes dans leurs sessions dédiées.
  [Reçu A2](../receipts/audit_g4_a2_20261005/README.md),
  [complément W48](../receipts/audit_s7_20261005/README.md).
- **B :** ASan/UBSan u24 **774/824**, TSan u21 **759/824** ; aucun échec
  individuel terminé, autres portes coupées à l'échéance.
  [Périmètre exact](../receipts/audit_g4_b_20261005/README.md).
- **S, d26328fe2 :** cinq lots TSan d'échelle et LiDAR **60/60** ; autres
  lots partiels. ASan garde trois refus `roots_cost`, avec les entrées
  synthétiques absentes du paquet. Les six fichiers requis sont présents
  et conformes dans `data_complet`, utilisé pour la reprise. Cette source
  précède L2b et S10.
  [Résultats et reprise](../receipts/audit_g4_s_20261005/README.md).

**Contrat de temps encore ouvert.** La référence FULL qualifiée
K5/u21/W48 donne **412 / 352 / 381 ms** sur trois trames sans sol de la
séquence08. Le banc F qualifie export FULL natif et projection Python
exacte. Les **100 ms** et la qualification de la nouvelle chaîne native
complète restent à acquérir. Les résultats GPU gardent leur périmètre
propre ; ils ne qualifient pas ces ajouts.

**Notes de travail et dialogue.**

- [Mathématiques : supports, hiérarchies et sélection](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : qualification, performances et intégration](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse courante du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md).
- [Question du développeur sur les 100 ms](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
- [Audit indépendant maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).
- [Contrat de la hiérarchie de points](../docs/HIERARCHIE_POINTS.md).

L'[audit général du 5 octobre](../receipts/audit_geant_20261005/README.md)
et les deux notes détaillées conservent les preuves des tranches
précédentes, leurs corrections et leurs limites. Les échanges clos sont
[archivés avec leurs empreintes](../receipts/audit_dialogues_20261004/README.md),
[contrat des supports compris](../receipts/audit_supports_contract_20261005/notes_before/README.md).
Aucun nouveau build, test natif ni GCP lancé par les auditeurs. Une seule
session G4 gardée à la fois ; arrêt ciblé certifié après chaque session.
