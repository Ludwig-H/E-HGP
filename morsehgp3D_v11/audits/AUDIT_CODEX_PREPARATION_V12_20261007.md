# Audit Codex — référence v11 pour le suivi de la v12

7 octobre 2026. Rôle : auditeur, sur demande de l'utilisateur. Source : `33c2ae3c8` ; moteur gelé `ac081a06f`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**Dossier de fond et preuves :** [contre-audit indépendant](../receipts/audit_independant_v12_20261007/README.md),
[registre de 19 constats](../receipts/audit_independant_v12_20261007/findings.json).

**Avis.** Aucun nouveau résultat FULL faux établi. Les mathématiques de FULL, les garanties de l'arbre de points,
les supports publiés, la sélection plate et la géométrie de rendu gardent des contrats distincts. Le gel v11 reste
une source différentielle à requalifier ; les 100 ms ne sont pas tenues et la voie GPU mesurée n'est pas le CLI.

**Constats à transmettre au développeur :**

- Sept juges de leviers poursuivent jusqu'à une décision d'adoption malgré un banc déclaré non conforme.
  Reproduction sur entrées synthétiques, normal/−O. Aucune adoption historique erronée établie.
- Cache : 262 145 octets utiles provoquent une allocation de 286 720 octets ; la marge n'est pas dans `used`.
  Sous Clang/ASan, le prétraitement retire l'empoisonnement explicite des blocs du cache.
- Tri des cohortes : sur 512 sites alignés, une seule cohorte utile provoque huit brouillons en W8. Le pic augmente
  de 572 320 octets ; sous budget serré, W1 réussit et W8 dense refuse. Les succès gardent les mêmes octets FULL.
- Corriger le bilan de preuve : **3 303 tentatives, 3 285 sorties avec SHA**, sans désaccord parmi les sorties
  comparables. Les 18 refus connus de `claudegpu2` ne sont pas des vidages.
- Distinguer processus résident, processus neuf réchauffé et périmètre du chrono ; fermer la réutilisation d'un
  binaire `b_cuda` sans lien vérifié à ses sources.
- Un nœud long peut être suivi comme chaîne par entrelacement, sans identité stable de nœud ; le Kruskal
  hypergraphique publié peut avoir un cycle d'incidence ; son squelette ne suffit pas à reconstruire les populations.

La [revue mathématique](../receipts/audit_independant_v12_20261007/math/README.md) répond aux huit questions polyèdre
et aux sections T/V/X/Y restées ouvertes. La [revue native](../receipts/audit_independant_v12_20261007/native/README.md)
relit num/catalogue/index/forêts/concurrence. Le [rapport de preuve](../receipts/audit_independant_v12_20261007/evidence/RAPPORT.md)
recalcule mesures, qualifications et comparaison synthétique à HDBSCAN.

**Vérification propre :** [commandes, résultats et limites](../receipts/audit_independant_v12_20261007/verification/README.md).
Release u21 : **920 portes passées, zéro échec, une sentinelle LiDAR sautée faute de données** ; 1 080 coupes
mathématiques supplémentaires. Les fichiers produit restent identiques au snapshot.
Les résultats de cette campagne ne remplacent ni une matrice G4 au pin final ni les campagnes LiDAR multi-séquences.
Aucun moteur modifié ; GCP non utilisé.
