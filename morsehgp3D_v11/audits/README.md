# Audits courants de la v11

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves et propositions détaillées dans les reçus immuables.

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

**GPU : cache J2 et rangs locaux intégrés, source relue.** La session
`claudej2memo` mesure le gain de l'exécuteur au pin **34a8a561d**. À K5,
le domaine GPU reste plus lent que le CPU. Le développeur met la feuille
cohérente en attente pour traiter les coûts fixes. Les défauts de la
variante par paires supprimée restent clos ; aucun ancien patch n'est à
appliquer. [Portée des mesures](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#j2-mémorisé--source-relue-et-mesure-g4-ciblée).

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
