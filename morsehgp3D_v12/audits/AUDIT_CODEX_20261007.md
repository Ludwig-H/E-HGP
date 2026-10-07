# Audit Codex — état courant v12

7 octobre 2026. Dernier pin examiné : **`e30000dec`**.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue produit`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. Auditeur du développeur v12.

**Dernière relecture : session G4 B et nouveau microbanc du parcours M5.**
[Rapport, témoins et limites](../receipts/audit_b_m5_20261007/README.md).
Les états font foi au [registre unique](CONSTATS.md).

- **Avant adoption M5 (`CST-0018`)** : le vrai pilote peut adopter malgré un contrôle d'identité en échec,
  des cas/fixtures manquants ou une seule passe v11. Il accepte aussi une médiane GPU déclarée qui contredit
  les durées brutes. Témoins par injections à la frontière des programmes externes ; aucun GPU lancé.
- **Géométrie M5 positive sur les portes ciblées** : 110 requêtes exactes de prédicats, 50 sélections de réservoir,
  neuf parcours hôte contre DFS indépendant et conservation de 77 boules de l'oracle borné. Profondeurs 63/96,
  boîte s33 et refus `wide_leaf` exercés. Collectives CUDA et produit restent à qualifier.
- **Session B recomptée** : résolution M3 réduite de 45,3–45,9 % à K10, mais une seule prise par cas (`0213`).
  M4 tient les seuils par ordre à K5 ; contraction K10 à 3,14–4,09 ms, au-dessus des 3 ms. Les preuves T6 sont
  complètes sur cette campagne ; le défaut générique `0214` reste ouvert.
- **Provenance B incomplète (`0021`)** : cinq exécutables M3/M4 non hachés. Sources et commandes raccordées
  apportent une preuve partielle ; le veto du plan empêche l'adoption définitive, même pour M4 K5.
- **Deux nouveaux constats** : `0222`, nombre de tâches émis nul près de la limite u32 ; `0223`, références M5
  sémantiquement invalides admises par le lecteur. Témoins minuscules, aucune erreur sur trame réelle alléguée.

**Réponse T1 suivie (`0113`)** : README M5 §9 distingue désormais le différentiel à ordre parent identique
et celui du catalogue lorsque Morton change. Le témoin des deux fronts est confirmé nativement. Le lecteur FULL
qui recalcule aussi Kruskal et `cover` reste attendu. Les portes M5 complètent `0204`, `0205`, `0208` sans
clore le port produit. L'admission mémoire avant réservation et conversion reste à faire (`0211`).

La [session A](../receipts/audit_session_t1_20261007/README.md) avait confirmé le choix local J3 r168, noyau seul.
Ses six défauts d'outils (`0216`–`0221`) restent ouverts au pin. Les autres états et clôtures sont conservés.
**Suite utile** : rendre le juge M5 strict avant sa campagne décisive ; hacher les exécutables et répliquer la
résolution M3 ; juger M5 sur G4 avant le port T1. Aucun contrat catalogue/FULL/100 ms acquis ici.
Le durcissement M2/M3/M4 `320db4a12`, déposé pendant cette tranche, reste à contre-lire ; les fichiers
M5 du juge, du lecteur et des noyaux examinés ici y sont inchangés. Pas de clôture anticipée de ces corrections.

Aucun appel GCP/GPU ni test lourd local par cet audit. Canal : quatre fichiers courants, preuves dans `receipts/`.
Contrôle : `python morsehgp3D_v12/tools/check_constats.py` (structure, tailles et liens).
