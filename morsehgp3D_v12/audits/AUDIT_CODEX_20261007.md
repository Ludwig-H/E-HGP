# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`4981b09cd`** ; prototypes non publiés épinglés dans les reçus.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. États et clôtures au [registre unique](CONSTATS.md).

**Avant intégration et campagne.**

- **Juges, `0018`** : propositions strictes [CUDA](../receipts/audit_reponses_20261007/cuda_juge_proposition/README.md)
  (49 contrôles, sortie CPU réelle admise), [MES-P](../receipts/audit_reponses_20261007/mes_p_admission/README.md)
  (26 anomalies refusées, cinq portes conservées) et [D6](../receipts/audit_reponses_20261007/d6_admission_proposition/README.md)
  encore à intégrer. `device_open` modifié reste à qualifier sur GPU ; D6 requiert référence u21 et sorties complètes.
- **T2-c** : [résumé altéré adopté malgré 120 journaux inchangés](../receipts/audit_reponses_20261007/t2c_pilote_admission/README.md).
  [Correctif proposé](../receipts/audit_reponses_20261007/t2c_pilote_proposition/README.md) : 21 prises, trois jugements,
  cinq auto-tests, deux formats CPU réels ; [30 journaux locaux admis](../receipts/audit_reponses_20261007/t2c_pilote_essai_local/README.md),
  campagne toujours refusée pour quotas. Intégration attendue. Identité finale seulement ; garde auto-test de `rapport` hors patch.
- **G** : [couverture corrigée et erratum de ma proposition](../receipts/audit_reponses_20261007/g_juge_integration/README.md) ;
  résidu `exit.order=False/0.0`. [Hash Gc](../receipts/audit_reponses_20261007/gc_empreinte/README.md) : travail hors digest,
  comparaisons entre fils conservées ; identité de résolution encore liée à la politique.
- **CUDA** : [compteurs corrigés](../receipts/audit_reponses_20261007/cuda_compteurs_reponse/README.md), nettoyages conservés.
  Voie hybride ≤32 sites/16 bits locaux, sinon reprise CPU exacte.
  [B″ local](../receipts/audit_reponses_20261007/cuda_b3_portes_locales/README.md) : 15 mutants (14 par code, un par signal),
  ASan/UBSan 44/44, TSan 12/12 ; **CUDA et cache actif désactivés**, neuf groupes appareil plus inventaire.
  Ces campagnes ne requalifient ni le GPU ni le poison des blocs inactifs. Temps du raccord attendus.
- **Table S* / finition** : [raccord à faire](../receipts/audit_reponses_20261007/gc_support_fusion/README.md), 16 octets
  par entrée Gc contre 4 en finition. Conversion hôte proposée sans transfert supplémentaire : +12 octets par boule
  au final, +16 au pic local avant arrondis du cache ; coût dans C.
- **T/M/V/R** : [branches ouvertes](../receipts/audit_registre_branches_20261007/README.md) implémentées, admission des
  offsets CSR à corriger. [Traces u21](../receipts/audit_tmv_traces_20261007/README.md) : 651 tests réussis + un sauté,
  15 mutants, neuf cas de chaîne ; arbre vérifié. Pas de contre-qualification native. Patch de livraison antérieur à R ;
  préserver le juge G à la fusion. Dédupliquer les racines avant recherche historique conserve ant(b), coût du tri à mesurer.

**Temps disponibles.** [G4 H](../receipts/audit_reponses_20261007/session_h_mesures/README.md), étape **G CPU, 48 fils**,
ng00/01/02 : **80,13 / 63,24 / 75,70 ms K5** ; **633,63 / 449,31 / 517,49 ms K10**.
Un processus par cas, neuf passes chaudes K5, deux K10. Catalogue CPU K5 : 451,6 / 372,7 / 469,5 ms ;
v11 historique 200 / 163 / 195 ms, comparaison descriptive. **Aucun temps intégré catalogue GPU ou FULL v12 publié.**
Trois trames d'une seule séquence ; les 100 ms restent ouverts.

**Leviers.** [Gc](../receipts/audit_reponses_20261007/gc_index_borne/README.md) : recherche exacte logarithmique sous
collisions, gros seau encore séquentiel ; G-L5 reste une dichotomie par requête. File G-L7 : 3 123 requêtes conformes.
[Hash par cellule](../receipts/audit_reponses_20261007/gc_hash_cellule/README.md) : 5 000 égalités, coût conditionnel ;
W1/W8 seul ne détecte pas le mutant déterministe, le rejeu indépendant du travail le peut. Chronos disjoints,
reconstruction par appel. Les [anciens rapports locaux](../receipts/audit_reponses_20261007/gc_rapports/README.md)
utilisent des minima avec passe 0 ; ils ne mesurent pas des médianes chaudes G4.
[Protocole proposé](../receipts/audit_reponses_20261007/gc_plan_mesure/README.md) : dix tours pour équilibrer les positions ;
mesurer C+G puis FULL, car la table S* est construite hors G.

Pistes et limites : [jointure G-L5 complète](../receipts/audit_reponses_20261007/g_l5_proposition/README.md),
[census/supports](../receipts/audit_g_pistes_20261007/README.md),
[finition et feuilles CPU](../receipts/audit_performance_20261007/README.md),
[budget conditionnel H](../receipts/audit_reponses_20261007/t2c_tables/README.md),
[petits nuages H](../receipts/audit_reponses_20261007/session_h_mes_p/README.md),
[préparation D6 hors chrono](../receipts/audit_d6_preparation_20261007/README.md).

Quatre fichiers actifs, 73 constats ; clôtures `0007/0019`, `0235/0236`, `0238` conservées. Capacités 256/64 de `0237`
encore ouvertes. Aucun GCP ni donnée sous licence dans cet audit. Natifs limités aux formats CPU synthétiques :
deux points catalogue, puis deux sondes G à huit points ; aucun benchmark.
