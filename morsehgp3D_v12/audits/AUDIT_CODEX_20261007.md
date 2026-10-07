# Audit Codex — état courant v12

7 octobre 2026. Auditeur du développeur v12. Dernier pin examiné : **`95247cf4b`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue produit`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Décisions D1–D15 enregistrées.

**Dernière relecture : socle T0 et microbancs M2, M3, M4, M6.**
[Rapport, témoins et limites](../receipts/audit_socle_microbancs_20261007/README.md).
Les états font foi au [registre unique](CONSTATS.md).

- **Trois clôtures** : `CST-0209` (quantiles M6), `0210` (premier usage séparé des répétitions),
  `0212` (domaines d'indices contrôlés dans le microbanc M4 ; produit futur à requalifier).
- **Avant adoption sur G4** : `CST-0018` reste actif. M2 peut adopter malgré l'échec d'identité hôte,
  des JSON périmés ou l'absence de cas décisionnel. Les pilotes M3/M4 acceptent des sorties sans preuve.
- **Trois nouveaux constats** : `0213`, la réplication demandée ne couvre pas la mesure de résolution de M3 ;
  `0214`, M4 réussit sans les sections de preuve des verticales ; `0215`, M2 accepte des vidages invalides
  malgré un checksum correct (profil, coordonnées, K, indices et plages).

Les petites comparaisons exactes de géométrie passent : feuille J3/cohérente contre v11, certification M3,
contraction M4 et verticales présentes, dont une image vers une naissance de l'ordre inférieur. Ces résultats
ne dispensent pas de refuser les entrées ou les preuves manquantes. Les témoins sont synthétiques ; aucun
contenu KITTI n'est ajouté. Aucun lancement GPU ni campagne lourde locale.

**Réponse aux contrats numériques** : les corrections de `0dbf69347` sont contre-lues ; `0201`, `0202`, `0204`,
`0205`, `0207`, `0208` et `0211` passent en cours, avec portes d'implantation encore attendues. Le socle porte
l'arithmétique globale u21/u24 de la v11 et refuse u32. Le cache mémoire reste à corriger (`0007`, `0019`) :
le buffer v12 admet 262 145 octets mais alloue 286 720, puis les conserve hors compte après restitution.

**Suite utile** : rendre les juges stricts sur identité, fraîcheur, complétude et rattachement des résultats ;
valider les vidages avant les noyaux ; répéter la vraie métrique M3 par processus ; intégrer ensuite les portes
du repère local et le budget physique. Nouveaux dépôts `26b53648c` (session G4) et `c55c12871` (données)
encore à relire. Aucun gain G4 ni contrat de 100 ms qualifié ici. GCP non utilisé.

Le canal reste limité à quatre fichiers courants ; détails et anciennes notes dans `receipts/`.
Contrôle avant publication : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
