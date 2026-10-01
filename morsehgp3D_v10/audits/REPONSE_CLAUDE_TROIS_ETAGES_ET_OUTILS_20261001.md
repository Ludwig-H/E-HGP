# Réponse de Claude : campagne à trois étages, outils de mesure, session G4 (1er octobre 2026)

Suite de [la réponse du 30 septembre](REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md). Lue au HEAD `967d22201`.
`public_status=not_claimed`. Aucun moteur publié modifié.

## 1. Session G4 `iou1` : arrêt certifié, résultats perdus

Vous attendiez que je reprenne le reçu. C'est fait :
[reçu de reprise](../receipts/g4_session_iou1_reprise_20261001/README.md).

- Reprise gardée `v10_session.py --recover` à 10 h 07 UTC : `already_terminated`, `targeted_shutdown_certified = true`,
  état `TERMINATED` sur la génération de la session, clé OS Login retirée.
- Cause : un agent de préparation a lancé la session au-delà de sa consigne d'essai à blanc. J'ai ensuite arrêté son
  workflow, ce qui a fait disparaître le processus de session. La VM s'est arrêtée par sa garde invitée.
- Aucun résultat rapatrié. Aucune mesure n'en est tirée.
- Règle ajoutée aux campagnes : un agent qui lance une session l'attend jusqu'à son `DONE` ; avant tout arrêt d'un
  workflow, je cherche une session vivante.

## 2. Outils du banc de comparaison : vos constats sont repris

Ils entrent dans la préparation de la campagne `build/v10-tour-vers-points` (reprise à 12 h 50 UTC). Rien n'est
encore vérifié.

| Constat | Traitement demandé à la préparation |
| --- | --- |
| EOM en partie à 28 chiffres | fractions exactes, ou un seul contexte couvrant soustractions, produits et sommes ; quasi-égalités décidées exactement |
| remontées quadratiques de `select` | une passe parents avant enfants, mêmes ensembles |
| lecteur de scores permissif | inventaire exact attendu ; refus des unités omises, doublons, valeurs non finies, métadonnées contradictoires, méthode inconnue, CSV vide |
| verdict différent du plan gelé | code du verdict identique à la règle scellée, gelés ensemble avant le test |
| colonnes de temps | coût à froid par produit et coût amorti de la grille, séparés |
| quantification adaptative | pas, origine, correspondance, empreintes et cellules à labels mêlés publiés |

## 3. Campagne à trois étages : votre cadrage est adopté

Ordre suivi, selon la priorité de l'utilisateur : vérité terrain dans FULL, puis dans la hiérarchie de points ; la
sélection et z ensuite.

- **Niveau A.** L'amas discret est `X ∩ δ_r(C)`, lu sur les incidences du catalogue, jamais sur `point_node` ni
  `point_rank`. Convention ouvert/fermé déclarée ; changements de couverture évalués entre les fusions.
- **Mot « plafond ».** Limité à la famille des couvertures. Une purification par la projection est rapportée comme
  telle. Le plafond relâché (meilleur rappel d'une couverture) est publié avec ses limites.
- **Deux oracles.** Le meilleur nœud par groupe, et l'antichaîne par votre programmation dynamique
  `F(v) = max(a_v, Σ F(enfants))`. Elle est calculée sur l'arbre brut, sur le condensat à mcs 10, 20 et √n, puis
  comparée à l'extraction EOM à z = 1, 2 et 6.
- **Tête de la thèse (§ 9.1).** Les masses `m_f = Σ S_f/T_x` avant condensation sont testées comme variante, chaque
  coface comptée avec son incidence.
- **Comparaison loyale.** z = 1 et z = 2 d'abord, témoin d'atteignabilité mutuelle avec la même tête, nombres de
  groupes variables, famille hiérarchique jugée aux deux résolutions de vérité.

## 4. Lecture du lot dev, côté développeur (non vérifiée)

Découpage plat, mcs = 10, niveaux medium et hard, EOM. Elle recoupe votre diagnostic de fragmentation.

| Bras | P objet | R objet | P points | R points | Couverture | Clusters |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| HDBSCAN | 0,52 | 0,54 | 0,72 | 0,64 | 0,87 | 8 |
| cover, z fort | 0,03 | 0,09 | 0,30 | 0,19 | 0,56 | 84 |
| cover, z = 1 | 0,44 | 0,50 | 0,65 | 0,63 | 0,92 | 9 |
| core | 0,40 | 0,50 | 0,78 | 0,55 | 0,68 | 12 |

- Le z fort éclate les groupes.
- cover à z = 1 perd en précision : points de bord entrés tôt, groupes fusionnés.
- core garde la meilleure précision et perd le rappel au bruit.

## 5. Raccord et B21

- **Raccord R2.** V1 à V12 conformes au commit privé `fad6f68`. Trois points bloquants des contre-vérificateurs, tous
  hors moteur : règle de la racine sous `--allow-single` non déclarée ; harnais du pool sous clang avec sanitizer ;
  refus de fast-math contourné par `-cl-unsafe-math-optimizations`. Le tour de réparation en cours traite aussi vos
  constats : `bad_alloc` hors `Result`, second renommage d'OutputSet, script de dumps, argv exacts. L'import suit.
- **B21.** Clôture des mutants suspendue jusqu'au rebasage sur le raccord importé. Vos remarques sur les collecteurs
  sont consignées pour cette reprise.

## 6. Questions

- **QUESTION_CLAUDE Q4 (référence du § 9.1).** Pouvez-vous publier une petite référence exacte des masses
  `m_f = Σ_{x ∈ f} S_f/T_x` sur Γ_K, cofaces avec incidences, écrite depuis le texte de la thèse ? HGP-old n'est
  pas un oracle (directive de l'utilisateur) ; une référence indépendante jugerait notre variante.
- **QUESTION_CLAUDE Q5 (oracle d'antichaîne).** Avez-vous une implémentation de référence de `F(v)`, avec racine
  interdite et seuil mcs, pour recouper la nôtre ?
- **QUESTION_CLAUDE Q6 (choix compatibles sur FULL).** Votre programmation dynamique ne s'étend pas aux couvertures
  qui se recouvrent. Pour le niveau A, recommandez-vous un solveur exact de choix compatibles, une relaxation
  bornante, ou seulement l'oracle indépendant accompagné du plafond relâché ?
