# Audits courants de la v11

6 octobre 2026. Cadre : `exploration_v11_hors_registre / cpu_reference /
quantized_u21_input_only / not_claimed`. Six notes actives maintenues en
place ; preuves et propositions détaillées dans les reçus immuables.

**Supports MST : correction prioritaire encore ouverte en 0cc9cbec4.**
Garder toutes les boules `merge` conserve les cycles d'un plateau : trois
arêtes au lieu de deux sur le triangle équilatéral. Le différentiel CLI
reproduit ce filtre et ne doit jamais épurer la sortie native à contrôler.
Le lecteur doit vérifier les versions fichier/manifeste, les unions utiles
et la connexion des branches. Le [paquet proposé](../receipts/audit_supports_mst_followup_20261006/README.md)
contient le sélecteur C++, le différentiel indépendant et les gardes du
lecteur. Modèles Python conformes ; intégration et qualification native G4
restent à effectuer. La [réponse aux questions R1/R2 du développeur](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#réponse-à-la-section-r--sélection-de-kruskal-et-contrôles-sans-q_b)
précise pourquoi il faut parfois plusieurs boules au même plateau et
quels contrôles conservent le format réduit sans réénumérer Q_b.
Les attentes erronées coquille25 et `balls <= cells` sont corrigées.

**Arbres de points : refuser l'entrée dans un bloc déjà fusionné.**
Le lecteur admet une chronologie impossible, même avec `exact=True` ; la
même borne de durée de vie manque dans `head::detail::check_shape`.
La [note moteur](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md#arbres-de-points--contrôler-la-fin-de-vie-du-bloc)
précise le témoin, la correction proposée et sa portée. Ce constat ne
démontre pas que le constructeur natif produit un tel arbre.

**Comparatif : conserver les IoU avant arrondi pour choisir la population.**
Un objet avec IoU `10001/20001 > 1/2` devient `0,5000` et peut faire
exclure sa scène. La [note mathématique](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md#comparatif--ne-pas-arrondir-avant-la-décision-de-population)
fournit le correctif du banc et les cas au seuil. L'effet sur les résultats
réels déjà publiés reste à déterminer ; aucun classement révisé annoncé.

**GPU : variante par paires retirée en d4228f5e5.** La suppression clôt
ses défauts propres de concurrence et de qualification ; leurs anciens
patches ne sont plus à appliquer. Coop3 confirme le recul de la régression
de la voie un fil sur sa source `9eee2ed4b` :
[preuve et limites des pièces conservées](../receipts/audit_g4_coop3_20261006/complement/README.md).
La [réponse à la proposition de feuille cohérente](../receipts/audit_coherent_leaf_design_20261006/README.md)
reste disponible. Aucun gain de ce futur noyau n'est acquis.

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
