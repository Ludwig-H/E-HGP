# Audit indépendant v11 — état des fondations

2026-10-02 08:30:02 UTC. Réponses/documents jusqu'à `f2ebb7a08` ; code WIP figé
par empreintes dans les reçus avant les sondes. `phase=exploration_v11_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u18_input_only`, `public_status=not_claimed`.
Aucun moteur FULL v11 ou GCP exécuté par cet audit. Le checkout évolue ; aucune
qualification transférée aux fichiers modifiés. Deux notes actives : celle-ci
et [la réponse aux cinq verrous](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

**Les corrections numériques et du minuteur sont reconnues.** Reste une erreur
d'exécution encore classée « mutant tué » dans le snapshot du lanceur, et la
contre-lecture mathématique du moteur à intégrer. Les autres fondations avancent.

## Corrections reconnues

| Point | État actuel et portée |
| --- | --- |
| F3/F4 | Exposition par expression, quotient par inverse couvert ; filtre limité aux clés positives, zéro en exact. Doctrine corrigée dans `f2ebb7a08`. Les bornes de chaque expression C++ restent à construire. [Recoupe et preuve](../receipts/audit_independant_20261002/numerical_followup/README.md). |
| Minuteur | StageTimer remplacé par Stopwatch, destructeur trivial sans nom ni Ledger. Les deux contre-cas ont zéro allocation ; publication explicite sous guarded rend le refus mémoire et permet la reprise. GCC normal sur nouvelles copies ; pas intégration globale. [Rejeu du correctif](../receipts/audit_independant_20261002/core_stopwatch_followup/README.md). |
| Filtre style G4 | Les deux portes style et style_opt sont sélectionnées ; défaut initial résolu. [Recoupe](../receipts/audit_independant_20261002/gates_review/README.md). |
| Sorties et budget | §7 fixe capacités/scratch/ancien+nouveau, durée de vie Session, domaines/offsets et rangs exacts. Manifeste final pour sorties multiples ; visibilité atomique aux lecteurs/crashes explicitement non promise. Réponse reconnue, implémentation à qualifier. |

Les [anciens témoins F3](../receipts/audit_independant_20261002/floating_bounds/PUBLICATION.md)
et [StageTimer](../receipts/audit_independant_20261002/core_review/README.md)
restent des preuves closes de leurs versions, sans entretenir ces réserves.

## Lanceur : progrès réel, un cas résiduel précis

Le programme absent est désormais INVALIDE/code3 ; nom exact, issue structurée,
rapports et refus des causes inconnues sont ajoutés. La réserve initiale est retirée.
Mais un script existant/exécutable dont l'interpréteur `#!` est absent passe
le précontrôle puis échoue à exec ; la copie `a7f4b342…` écrit encore TUE/signal,
campagne code0. Aucun programme n'a été exécuté ni signal reçu.
Distinguer l'erreur d'exécution **après** le précontrôle d'un signal réel,
sans énumérer les interpréteurs possibles ; elle doit devenir INVALIDE.
[Deux fixtures normal/−O et limites](../receipts/audit_independant_20261002/gates_followup/README.md).
Ce contrôle ne déclare pas les mutants core réels non causaux.

## Premiers contrôles positifs, ancrés à leurs copies

- [Core intégré](../receipts/audit_independant_20261002/integration_review/README.md) :
  49 fichiers, GCC Release B18, 70 portes rapides passent ; une sentinelle LiDAR
  sautée, campagne longue de mutants exclue. Huit fichiers avaient déjà évolué
  à la recoupe : ce build ne qualifie pas leur nouvelle version.
- [Budget/Buffer](../receipts/audit_independant_20261002/core_review/README.md) :
  compte partagé survivant au propriétaire, déplacement entre budgets et refus
  contrôlés. allocate détruit l'ancien contenu avant remplacement ; un résultat
  transactionnel exige temporaire puis swap. Le préflight admit ajouté ensuite
  n'est pas une réservation concurrente, et n'hérite pas de ces tests.
- [Oracle](../receipts/audit_independant_20261002/reference_review/README.md) :
  1 732 gardes normal/−O identiques, dont 616 coupes analytiques et 760 verticales.
  Aucun défaut mathématique dans ces cas. A/B partagent numérotation/parents ;
  dumps partagent catalogue. Le troisième juge réduit ce risque sur les droites,
  sans devenir un troisième oracle général 3D. Neuf sources ont ensuite changé.

## Port et suite mathématique

Ancien MR1 Pool R2 fermé dans final5, MR1b distinct déclaré équivalent.
Source `865f5e6`, série jusqu'à `210b9fc`, comptes final5 et nouvelles portes FP
restent distincts ; aucune qualification v11 héritée.
[Recoupe conservée](../receipts/audit_independant_20261002/provenance_review/README.md).

La [réponse moteur](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) ferme Q1–Q5 :
représentants couvrants, classe de descente, filtre signé, limites d'Euler,
attaches symétriques et première livraison sans maturité imposée.
Ses témoins restent distincts des tests du moteur ; **Euler seul n'est pas
un certificat universel de complétude**.

Le jalon déclaré est la trame résidente. Multi-millions et u32 demeurent des
étapes ultérieures, avec [dimensionnement par phases](../../morsehgp3D_v10/audits/AUDIT_MASSIF_LIDAR_20260930.md).
Compiler 21/24 ne qualifie pas ces profils ; la tour pondérée est explicitement
refusée tant que sa sémantique n'est pas écrite. Nos contrôles de doublons de
l'oracle ne lèvent pas ce refus natif. Préserver continuum des centres, toutes
les incidences et plateaux, puis mesurer présence/projection/compatibilité.
