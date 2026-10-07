# Audit Codex — contrats et premiers ports v12

7 octobre 2026. Note vivante, rôle d'auditeur. Cadre : `phase=exploration_v12_hors_registre`,
`backend=cpu_reference ; cuda_g4 pour le catalogue produit`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.

**Relecture achevée des premiers contrats.** [Rapport et témoins](../receipts/audit_contrats_20261007/README.md),
sur le contrat numérique `e264de6f2`, avec prise en compte de sa révision `8865e32c1`, des deux notes de l'autre
auditeur et de la réponse aux lemmes publiée sur `a0e31abfe`. D1–D15 sont enregistrées ; leurs choix ne sont pas
redemandés. La base reste le [contre-audit v11](../../morsehgp3D_v11/receipts/audit_independant_v12_20261007/README.md).

Six nouveaux constats au [registre](CONSTATS.md), à traiter avant les ports concernés :

1. **CST-0201 — certificats** : ils lisent le domaine. Un q3 d'étendue 20 bits certifié pour son seul support
   déborde i128 avec un site pourtant admis par la nouvelle garde. L'intermédiaire déborde, pas le résultat final.
2. **CST-0202 — identité XYZ** : la v11 fusionne les clés Morton égales. Une clé tronquée transforme trois positions
   distinctes en deux sites si ce groupeur est porté directement ; séparer identité exacte et clé de localité.
3. **CST-0204 — boîtes** : `hi=max+1` peut valoir `2^32` ; leur fermeture exige alors un repère de 33 bits.
4. **CST-0205 — profondeur** : 48 points synthétiques, K5, feuille 24, atteignent 63 niveaux sur le natif v11.
   Remplacer la borne 38 de l'architecture par la preuve `3B` et compter la racine dans les capacités.
5. **CST-0207 — D6** : comparer original et translation dans un binaire ne mesure pas le coût u32 contre u21.
6. **CST-0208 — réservoir** : distance préparatoire de G1 à `2s+4` bits ; i64 garanti jusqu'à s29, pas s30.

Accord indépendant sur les six lemmes T sous leurs hypothèses corrigées. Le correctif T1 et sa porte du carré ont
été relus et rejoués, mutant tué. Les modèles exacts ajoutent 720 permutations de plateau, 23 040 requêtes
d'historique et 36 quotients locaux. Ils complètent les lignes CST-0101 à 0113 sans les dupliquer ni les fermer.
T6 doit garder l'image inférieure qui est elle-même une naissance ; le coût cubique de T7 ne couvre pas toutes les
intersections de masques. La révision numérique corrige la portée des gardes et la formulation de la translation ;
les six points ci-dessus restent ouverts. Réponses détaillées et relecture des nouvelles formules au reçu.

Les scripts passent en Python normal et `-O`. Deux petits catalogues v11 ont été exécutés ; aucun moteur v12,
GPU, profil u32 ou temps de tour nouvellement qualifié. GCP non utilisé. Le contrôle structurel
`python morsehgp3D_v12/tools/check_constats.py` ne certifie ni les preuves ni les clôtures.
