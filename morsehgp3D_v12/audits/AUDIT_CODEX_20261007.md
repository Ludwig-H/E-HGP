# Audit Codex — état courant v12

7 octobre 2026. Auditeur du développeur v12. Dernier pin examiné : **`3e6e6a8e7`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue produit`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Les décisions D1–D15 sont enregistrées.

**Dernière relecture : microbanc CUDA M6 et échelle.** [Rapport et témoins](../receipts/audit_suivi_20261007/README.md).
Quatre constats ajoutés au [registre](CONSTATS.md) :

- **CST-0209** : médiane paire erronée dans le code M6 ; l'extrait original renvoie 5 pour 1..10, attendu 5,5.
- **CST-0210** : premier lancement du graphe et de `touch` mélangé aux quantiles résidents. Séparer préparation,
  premier usage et répétitions chaudes avant campagne.
- **CST-0211** : les volumes empiriques sont une prévision mémoire ; la réservation certifiée et le refus
  transactionnel restent à définir pour les grands nuages.
- **CST-0212** : le prototype de forêt réserve le bit haut des opérandes u32 au genre. Ses indices ont 31 bits
  utiles ; écrire leur domaine exact ou changer le codage avant le port.

Le microbanc compile avec CUDA 12.9 pour sm_120 ; cinq refus CLI vérifiés avant tout appel CUDA. Aucun lancement
GPU. Les modèles exacts d'échelle distinguent sites, naissances, événements bruts et nœuds après contraction :
le cube de huit sites a douze naissances à K2 ; une seule multifusion peut résumer un journal arbitrairement grand.

**Contrats numériques encore ouverts** : CST-0201 (portée des certificats), 0202 (identité XYZ indépendante de
Morton tronqué), 0204 (fermeture de boîte à 33 bits), 0205 (profondeur 63 prouvée sur natif u21), 0207 (ratio D6),
0208 (budget du réservoir). [Preuves et réponses](../receipts/audit_contrats_20261007/README.md).
Les lemmes T corrigés et les budgets mixtes ont été contre-lus ; les portes de leur implantation restent distinctes.

**Suite utile** : relire les corrections de M6 avant sa campagne G4, puis les fabriques numériques et le codage
réel de la forêt. Pas de nouveau moteur, profil u32, débit GPU ou contrat de 100 ms qualifié par ce suivi.
GCP non utilisé. Le canal reste limité à son état courant ; rapports détaillés et anciennes notes sont dans les
reçus. Le contrôle `python morsehgp3D_v12/tools/check_constats.py` vérifie structure, tailles et liens.
