# Audit Codex — état courant v12

7 octobre 2026. Code éprouvé : **`58d384721`** (catalogue `671072339`). Relecture des
mesures F2/E ; complément mathématique sur le reçu G `5c5fc7109`.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`.
[Rapport et propositions pour le développeur](../receipts/audit_performance_20261007/README.md) ;
états au [registre unique](CONSTATS.md).

**Priorité : corriger la régression, sans changer l'objet.** Le catalogue CPU produit vaut
451,6 / 372,7 / 469,5 ms à K5 sur ng00/01/02, 48 fils, médiane de neuf passes chaudes
par cas. La référence historique v11 vaut 200 / 163 / 195 ms : rapport descriptif ≈2,3,
pas A/B apparié. Ce n'est pas un temps FULL. La voie GPU intégrée reste à mesurer.

- **`0233`, finition remise en série** : scans de rangs/CSR, aplatissement et table. Assemblage+table
  coûtent 122,5–171,3 ms. Conserver l'aval CPU et rendre parcours+feuilles gratuits laisserait
  encore 145,1–201,7 ms : scénario conditionnel, pas prédiction GPU. Preuve et modèle d'un
  scan bloqué à halo, premier représentant rationnel conservé ; table par permutation exacte.
- **`0234`, travail physique masqué par les compteurs logiques** : chaque paire est calculée
  deux fois ; le census CPU continue après le rejet. Prototype hors produit : arrêt effectif,
  mêmes émissions mot à mot et quinze compteurs sur sept feuilles, dont Exact et warp 256.
  Ces tests ne chiffrent pas un gain LiDAR ; la variante exige son ablation G4.
- **`0235`, budget catalogue non confirmé** : M2+M5 ne couvrent ni émission complète ni
  finition intégrée. Réviser le bilan « budget confirmé » et juger le catalogue complet,
  avec préparations, replis, mémoire et coût aval, avant l'adoption produit.
- **Mathématiques** : changement d'empreinte FULL vers les valeurs rationnelles exactes
  justifié ; topologie, centres, PointId et verticales restent comparés. Clés de supports
  par rang lexicographique prouvées ; caches par coquille/population/partie complètes
  justifiés sous propriétaire et contexte précis. Niveau, cardinal ou hash seuls insuffisants.
- **`0236/0237`, dégénérescences** : le générateur `synth_sphere` arrondit ses coordonnées,
  contrairement au diagnostic « tous cosphériques » du reçu G (cinq points réfutent chaque
  cas testé). Distinguer largeur de liste candidate et coquille exacte. L'objectif sans
  refus de largeur reste incompatible avec les capacités actuelles 256/64 ; q_min≤4 ne
  borne ni la coquille ni le travail d'énumération. Protocole d'extension dans le reçu.

**Suite concrète** : finition parallèle/appareil, arrêt CPU et paires uniques, puis clés
préparées si le profil le justifie. G-L3 reste rejeté (+3–4 % malgré 81–83 % de censuses
évités) ; le profil G4 donne priorité aux sondes de table puis à la proposition de boule.
Campagne d'ablations appariées proposée, même objet et périmètre, puis FULL multi-séquences.

**Vérifications** : relecture de 120 passes F2 ; modèle de scan 3 537 comparaisons et quatre
mutants, 13 468 requêtes de table ; 30 381 comparaisons de clés, témoins géométriques exacts,
prototype de feuille, lecteurs normal/−O. Aucun recalcul LiDAR, GPU/GCP ni chrono nouveau.
Ces preuves ne qualifient pas un catalogue corrigé ni les 100 ms.

Les clôtures antérieures restent au registre. Correctifs des juges/données (`2b2113264`)
et EMST (`98ca07556`, `0232` en cours) annoncés par le développeur, sans contre-qualification
supplémentaire dans ce lot. Canal : une note courante ; détails et rejeux dans `receipts/`.
