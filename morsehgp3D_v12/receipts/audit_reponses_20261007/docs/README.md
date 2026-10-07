# Réponses documentaires 0235–0237 — 7 octobre 2026

**Les corrections lèvent les affirmations documentaires de 0235 et 0236. La couverture des grandes feuilles reste à
livrer : 0237 peut passer en cours, avec clarification acquise et capacité produit ouverte.** Aucun budget, objet
géométrique ni moteur n'est requalifié par cette lecture.

Capture de modifications **non commises**, sur `91b1a7ee2dae049895eb7889aeaa8262642bf72b` : `docs/PLAN.md`,
`docs/MESURE.md`, nouvel `receipts/g4_t0g_20261007/ERRATUM_20261007.md`. [capture.json](capture.json) donne SHA-256,
tailles, base et lignes du registre consultées ; [documents.diff](documents.diff) conserve les changements exacts,
dont l'erratum. Sources et HEAD relus identiques après capture ; le diff passe `git apply --check` au commit de base.
Ni document vivant, ni registre, ni reçu historique modifié par cet audit. Aucun test numérique ou moteur rejoué.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

| Constat | Réponse observée | Avis et limite |
| --- | --- | --- |
| `CST-0235` | PLAN retire explicitement « budget C confirmé » : 35–45 ms restent non confirmées ; à aval CPU inchangé, parcours/feuilles gratuits laissent 145–202 ms K5. T1-b doit mesurer la fin du catalogue. | **Clôture documentaire possible après publication**. Les valeurs restent conditionnelles à l'aval conservé ; elles ne bornent pas un autre moteur. Aucun catalogue GPU intégré ni FULL nouveau n'est revendiqué. |
| `CST-0236` | L'erratum nomme une quasi-sphère après arrondi entier, cite les déterminants non nuls des cinq points des trois graines et distingue liste candidate, coquille et cardinal total. MESURE reprend « quasi-sphère arrondie ». | **Clôture documentaire possible après publication**. Les refus v11 mesurés restent acquis ; leur causalité n'est plus attribuée à la cosphéricité globale. Les contre-exemples précédents sont cités, sans nouveau calcul ici. |
| `CST-0237` | PLAN ouvre T1-c ; MESURE indique « non couvert par T1 ». Sont explicités : feuilles 256, coquilles 64, `m` sur un octet, masques 64 bits ; `q_min≤4` ne borne ni coquille ni candidats. | **Clarification documentaire acquise ; capacité en cours**. Un protocole et une cible ne suppriment aucun plafond du code. Aucun grand nuage v12 n'est nouvellement exécuté. |

Le protocole T1-c reprend les distinctions nécessaires : diagnostic sur générateur épinglé (largeur, profondeur,
histogrammes de coquilles), quasi-sphère / sphère entière exacte / réseau séparés, puis ventilation préparation /
résolution / noyau / contraction. Le futur contrat doit traiter masques et CSR extensibles, budgets, listes larges et
canonisation par boule ; `LEM-T7` reste conditionnel à son adoption. Petits témoins contre l'oracle, frontières 32/33,
64/65, 255/256/257, plateaux et mesures G4 avec sorties complètes sont prévus. Aucun écrêtage, jitter ou préfixe n'est
présenté comme résolution du problème.

L'erratum rappelle correctement que `forest_ns` englobe aussi préparation et résolution : les 92–99 % observés sur
le réseau ne prouvent pas que le seul noyau union-find soit responsable. Il conserve le reçu et ses empreintes
historiques ; l'erratum est explicitement un ajout séparé. Son paragraphe `CST-0238` signale une correction de lecteur,
**hors de cette contrelecture** : aucune clôture de 0238 n'est déduite ici de la seule annonce.

Une formulation de PLAN mérite précision avant publication : « les transferts ne sont pas mesurés » doit viser
**les transferts et synchronisations du raccord catalogue complet**. Le microbanc M5 comptait déjà ses transferts.
Cette nuance ne rétablit aucun budget acquis ; elle évite de nier une portion déjà mesurée. Même réserve pour les
« environ 16 ms » : deux briques isolées, jamais un temps intégré nouveau.

Ces avis portent uniquement sur les octets capturés. La clôture du registre devra citer le commit qui les publie,
après vérification que le texte retenu correspond à cette capture ou à sa précision sur les transferts.
