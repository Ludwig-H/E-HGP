# Audit indépendant v11 — état des fondations

2026-10-02 10:05:06 UTC. Socle publié `5c5457a53`, documents jusqu'à `92c5af705` ;
ports cloud/IO WIP figés avant lecture. `phase=exploration_v11_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u18_input_only`, `public_status=not_claimed`.
Cette tranche : lecture statique et petits contrôles autonomes du protocole et
de formules, sans build/test natif ni GCP. Deux notes actives : celle-ci et
[les verrous mathématiques et la robustesse frontière](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Oracle désormais séparé ; comptage cloud cohérent.** Les corrections précédentes
sont reconnues. Préciser l'interruption/isolation des sondes de la matrice G4 et
protéger l'absorption d'un morceau vide dans IO. Aucun contrat FULL acquis.

## Corrections reconnues

| Sujet | État et portée |
| --- | --- |
| F3/F4 | Exposition par expression, réutilisations, inverse et clés positives corrigés. Les domaines et bornes des expressions C++ restent à construire. [Preuve corrigée](../receipts/audit_independant_20261002/numerical_followup/README.md). F6 précise désormais conversion des feuilles et domaine du seuil dans `92c5af705` ; correction documentaire reconnue, portes des expressions réelles à jouer. |
| Minuteur | Stopwatch à destructeur trivial, publication explicite sous guarded. Refus mémoire et reprise contrôlés sur copies, sans intégration globale héritée. [Rejeu](../receipts/audit_independant_20261002/core_stopwatch_followup/README.md). |
| Lanceur mutants | Programme absent, puis script exécutable à interpréteur absent : INVALIDE/code3. Le wrapper publié ferme le deuxième cas ; vrai lecteur/CTest sur fixture, configuration et build doublés. Retirer la réserve du snapshot antérieur. [Chemin exact et limites](../receipts/audit_independant_20261002/runner_session_review_2/README.md). Ce contrôle ne qualifie pas les campagnes de mutants core réels. |
| Contrats | §7 précise allocations simultanées, durée de vie Session, domaines et sorties par manifeste final. Doctrine reconnue ; chaque port reste à qualifier. |

Les anciens [F3](../receipts/audit_independant_20261002/floating_bounds/PUBLICATION.md)
et [StageTimer](../receipts/audit_independant_20261002/core_review/README.md) restent
clos à leur source. Le défaut d'arrêt simulé par une ligne imprimée, décrit par
[l'autre auditeur](AUDIT_OUVERTURE_ET_REPRISE_V10_20261002.md), est corrigé à `92c5af705`
par lecture du vrai statut du processus ; nouveau code relu, porte native G4 attendue.

## G4 : cycle de vie relu, mesures à isoler et interruption à distinguer

Pins, verrou v10/v11 commun et fermeture ciblée n'ont pas de nouveau défaut
établi dans cette lecture ; ni GCE ni OS Login réels exécutés.
Deux contrôles du vrai outil, avec doubles explicités, montrent :

- Steps.run marque ok quand le parent sort 0 malgré un descendant encore vivant.
  La fermeture finale du worker couvre le groupe, mais la prochaine sonde n'est
  pas garantie seule. Fermer l'étape et sa descendance avant toute mesure suivante.
- SIGTERM pendant une sonde après les configurations vertes apparaît dans signals,
  mais code0/conforming=true subsiste. Le contrat annonce une interruption code1.
  Garder la validité des portes terminées et distinguer l'interruption globale
  dans le résumé et le code ; les sondes ne sont pas des portes de conformité.

[Sources, contre-cas et limites](../receipts/audit_independant_20261002/runner_session_review_2/README.md).
Ces cas ne déclarent ni fuite de VM ni ancienne porte mathématique fausse.

## Oracle : réserve structurelle levée, catalogue à juger séparément

A, B et juge ont leurs propres numérotation, parents et coupes. Le troisième
attendu par intervalles ne dépend que de Fraction et des fenêtres ordonnées ;
il reste collinéaire. Aucun défaut FULL nouveau trouvé dans ces deltas.
[Quatorze sources stables, lecture et contrôles autonomes](../receipts/audit_independant_20261002/reference_separation_review_2/README.md).
L'égalité FULL seule ne certifie pas les boules inertes ni leurs incidences I/U ;
prévoir le juge catalogue indépendant avant son raccord natif.

Les deux triangles historiques sont une approximation entière h=1732, pas deux
équilatéraux exacts ; leurs niveaux légèrement distincts sont déjà attendus.
Garder en plus le plateau exact et l'attendu idéal du manuscrit. Les anciennes
[1 732 gardes](../receipts/audit_independant_20261002/reference_review/README.md) et
[70 portes GCC](../receipts/audit_independant_20261002/integration_review/README.md)
appartiennent aux copies initiales, sans transfert à la nouvelle livraison.

## Cloud : pic exact de préparation, entrées vivantes à ajouter

Tri stable, regroupement, multiplicités, CSR u64 et PointId maximal cohérents
à la lecture. Le résultat possède ses Buffer ; pas d'alias d'entrée après retour.
Préciser que les spans restent inchangés pendant l'appel, ou copier les données
avant certification : les coordonnées sont validées puis relues après le tri.
[Lecture et dimensionnement](../receipts/audit_independant_20261002/cloud_contract_review_2/README.md).

Pour n retours/s sites : pic propre max(2Rn+H, Rn+4n+24s+8), R=16 en B18/21,
R=32 en B24. Ajouter les autres réservations vivantes. Avec quatre entrées Buffer
16n et s=n : B18/21=max(48n+H,60n+8), B24=80n+81920.
À 30 M retours uniques : environ 1,8/2,4 Go décimaux, **préparation seulement**,
hors index/catalogue/FULL. Ce sont des formules, pas un benchmark/RSS ni un contrat massif.
Morton large seul ne qualifie pas le moteur u32 ni la tour pondérée.

## IO WIP : morceau vide à traiter avant son pointeur

La copie `fbf1a80e…` appelle memcpy avec source nulle et longueur zéro lorsque
`update(std::string_view{})` suit `update("a")`. Ajouter le retour réussi immédiat
pour l'entrée vide ; compteur et digest inchangés. [Source normative et cas G4 proposé](../receipts/audit_independant_20261002/io_contract_review_2/README.md).
Constat par préconditions, aucun crash/sanitizer natif observé. Longueur SHA-256
bornée avant somme, digest sur copie et entier texte strict : points positifs
sur les mêmes copies ; formats et sortie transactionnelle à venir.

## Suite

Les [cinq décisions moteur](REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md) sont adoptées.
La [nouvelle preuve frontière](../receipts/audit_independant_20261002/boundary_stability_review_2/README.md)
sépare stabilité FULL en rayon et discontinuité possible du premier cover/LCA.
Conserver les retours et le repère physique ; la projection et ses masses se
jugent séparément. Les paliers de précision et le [massif](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md)
restent au plan, après le jalon trame. Aucun temps FULL/G4 acquis par cet audit.
