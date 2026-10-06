# Audits courants de la v11

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Sept notes actives maintenues en
place ; preuves et propositions détaillées dans les reçus immuables.

**Polyèdre de Hartigan : réponse aux cinq questions mathématiques.**
Retenir la mosaïque d'ordre k filtrée par le rayon du k-ième voisin.
Les incidences attribuent les cellules aux nœuds ; leurs barycentres peuvent
tomber dans une autre composante dense. DTM, union des boules critiques et
frontière seule ne conservent pas automatiquement la hiérarchie. La
[réponse détaillée](../receipts/audit_hartigan_delaunay_20261006/README.md)
donne les preuves, six petits contre-exemples rejoués et un certificat
d'effondrement compatible avec les dates. Elle part des régions témoins
du manuscrit et explicite le raccord aux définitions et théorèmes de ses
deux premières parties, relues à la demande de l'utilisateur. Aucun transfert
direct de Wrap ni constructeur géométrique qualifié.

**Placement : perte de l'option corrigée en 9d10de213.** La porte G4
corrigée observe des plans non nuls ; le premier essai à zéro plan reste
exclu. **12ce8f8f0 intègre le témoin par prise et le correctif du lecteur
de campagne proposés par l'auditeur.** La lecture du WIP API et des bornes
entières est favorable, sans transfert de qualification.
[Activation, portée des reçus et proposition](../receipts/audit_placement_activation_20261006/README.md).

**Levier C intégré en 5861c223f : lecture numérique favorable.** Les
deux raccords proposés sont corrigés en **38faaf272** : mutants d'étendue
explicitement u21, cache des variantes lié au SHA de l'archive. Les
[preuves et la réponse sur le repli CPU parallèle](../receipts/audit_narrow_followup_20261006/README.md)
restent épinglées ; cette lecture d'intégration n'ajoute aucun résultat
de mutant, sanitizer ou CTest G4.

**Géométrie : changement de cible pris en compte.** La
[proposition de surface observée](../receipts/audit_geometry_design_20261006/README.md)
reste historique. La nouvelle demande transmise en fd85f3bb5 porte sur la
composante de haute densité de Hartigan, traitée dans la réponse ci-dessus.

**Réservoir et placement : porte IO corrigée en 86b3cbf14, lecteur de
campagne corrigé en 12ce8f8f0.** Le
[correctif du lecteur](../receipts/audit_placement_followup_20261006/README.md)
est intégré ; la proposition historique à deux fichiers reste périmée.
Le refus du registre absent et sa comparaison à la référence CPU sont
également intégrés au juge GPU en **38faaf272**, sans rétroqualification.
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

**Comparatif : arrondi préalable corrigé en 38faaf272.** Le
[correctif de deux lignes](../receipts/audit_review_followup_20261006/population/README.md)
est intégré : les valeurs restent non arrondies avant la décision stricte
à 1/2. L'effet du défaut sur les résultats réels
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
- [Question du développeur sur le polyèdre d'ordre k](QUESTION_CLAUDE_POLYEDRE_ORDRE_K_20261006.md).
- [Audit indépendant maintenu par son auteur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md).

L'[audit général](../receipts/audit_geant_20261005/README.md), les notes et
les [échanges archivés](../receipts/audit_dialogues_20261004/README.md)
conservent les preuves antérieures. Aucun nouveau build, test natif ni
session GCP lancé par les auditeurs.
