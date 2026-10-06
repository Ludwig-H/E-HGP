# Audits courants de la v11

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves et propositions détaillées dans les reçus immuables.

**Levier C intégré en 5861c223f : lecture numérique favorable.** Deux
raccords importants restent à corriger : les mutants d'étendue héritent
d'u18, où leur garde est désactivée, et la reprise du banc multi-source
peut attribuer un ancien binaire à une nouvelle archive. Les
[preuves, correctifs proposés et réponse sur le repli CPU parallèle](../receipts/audit_narrow_followup_20261006/README.md)
sont déposés. Les comparaisons G4 de C sont distinctes du sanitizer A+C ;
aucun nouveau CTest ou mutant G4 dans cette session.

**Géométrie pour le modèle de fondation : proposition déposée.** La cible
est la surface observée, avec ses trous. Construire des éléments de surface
communs, regroupés par HGP ; la vraie mosaïque d'ordre k est une charpente
complémentaire à compléter et à qualifier. Les supports actuels ne sont pas
cette mosaïque. [Construction et exemples](../receipts/audit_geometry_design_20261006/README.md).

**Réservoir et placement : porte IO corrigée en 86b3cbf14.** Le lecteur
de campagne garde cependant l'ancienne borne ; le nouveau
[correctif du lecteur](../receipts/audit_placement_followup_20261006/README.md)
remplace la proposition historique à deux fichiers, devenue périmée.
Le juge GPU peut toujours accepter un registre absent.
[État des raccords](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#réservoir-de-feuilles--raccord-des-portes-à-corriger).

**Supports MST : correction intégrée en 07428324e et qualifiée sur G4 u21.**
Le sélecteur garde les unions utiles au même plateau ; le différentiel
compare la sortie native intacte et le lecteur contrôle connexion et
versions. **15/15 portes PASS**, au même pin, sur oracle, synthétiques et
trames LiDAR. Les références des trois profils sont corrigées ; les
sessions u18/u24, mutants et sanitizers ne sont pas incluses dans ces
quinze portes. [Contrelecture](../receipts/audit_integration_20261006/README.md).

**Arbres de points : garde intégrée en b0f2a0a9e.** Une entrée doit précéder
strictement la fusion de son bloc. Les dix-huit cas du lecteur intégré
passent en normal/−O ; les nouvelles portes et mutants C++ restent à
qualifier sur G4, ce commit étant postérieur à la session supports.
[État précis](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#arbres-de-points--contrôler-la-fin-de-vie-du-bloc).

**Comparatif : l'arrondi qui peut fausser la population reste à corriger.**
Un objet avec IoU `10001/20001 > 1/2` devient `0,5000` et peut faire
exclure sa scène. Le [correctif de deux lignes](../receipts/audit_review_followup_20261006/population/README.md)
conserve les valeurs avant la décision. L'effet sur les résultats réels
publiés n'est pas établi ; aucun classement révisé annoncé.

**Contrat global de 100 ms toujours ouvert.** Les captures d'un noyau ou
d'un étage ne le remplacent pas. La reprise de qualification R1–R4 est
close aux pins et profils de ses reçus : 3 695 portes ordinaires couvertes,
485 mutants détectés, 80 portes ASan/UBSan u24 conformes. Les différentiels
S9 et S10 sur synthétiques et trois trames sont clos séparément. Ces acquis
ne se transfèrent pas aux modifications en cours.
[Qualification et limites](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#reprise-g4-après-correction--quatre-sessions-au-pin-98a009550).

**Notes et dialogue.**

- [Mathématiques : constats actifs et preuves](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
- [Moteur : constats actifs, qualification et historique](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
- [Réponse du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md).
- [Question du développeur sur les 100 ms](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
- [Audit indépendant maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

L'[audit général](../receipts/audit_geant_20261005/README.md), les notes et
les [échanges archivés](../receipts/audit_dialogues_20261004/README.md)
conservent les preuves antérieures. Aucun nouveau build, test natif ni
session GCP lancé par les auditeurs.
