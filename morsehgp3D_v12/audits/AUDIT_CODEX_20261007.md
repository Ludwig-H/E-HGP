# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`b06cd1449`** ; prototypes non publiés épinglés dans les reçus.
Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. États et clôtures au [registre unique](CONSTATS.md).

**Priorités avant intégration et campagne.**

- **Juges, `0018`** : [CUDA admet encore des preuves absentes](../receipts/audit_reponses_20261007/cuda_juge_prepublication/README.md).
  [Correctif proposé](../receipts/audit_reponses_20261007/cuda_juge_proposition/README.md) : 49 contrôles normal/−O,
  vraie sortie CPU acceptée ; extension C++ `device_open` à qualifier sur GPU.
  [MES-P strict proposé](../receipts/audit_reponses_20261007/mes_p_admission/README.md) : 28 cas, 26 anomalies refusées,
  porte 5/5 conservée. Ni ces patches ni le [lecteur D6](../receipts/audit_reponses_20261007/d6_admission_proposition/README.md)
  ne sont intégrés. Référence u21 et sorties complètes requises avant décision D6.
- **T2-c avant G4** : [pilote `fa1b7cd6` contre-éprouvé](../receipts/audit_reponses_20261007/t2c_pilote_admission/README.md).
  Changer le résumé seul fait adopter un gain malgré 120 journaux inchangés ; huit prises testent aussi
  types, passes et configuration. [Correctif proposé](../receipts/audit_reponses_20261007/t2c_pilote_proposition/README.md) :
  21 prises, trois jugements, cinq auto-tests et deux vrais formats admis ; références corrigées, rien d'intégré.
  Identité finale seulement ; garde auto-test en mode `rapport` encore hors patch.
- **G** : [couverture corrigée et erratum](../receipts/audit_reponses_20261007/g_juge_integration/README.md).
  Ma première proposition omettait `exit`, corrigé par le développeur avant livraison ; résidu `exit.order=False/0.0`.
  [Gc `c002345` contre-lu](../receipts/audit_reponses_20261007/gc_empreinte/README.md) : travail hors digest,
  comparaison entre fils conservée. Le hash de résolution reste lié à la politique ; témoin exact de huit points.
- **CUDA** : [compteurs corrigés dans `66c41ed`](../receipts/audit_reponses_20261007/cuda_compteurs_reponse/README.md),
  nettoyages mémoire conservés. Voie hybride ≤32 sites/16 bits locaux, sinon reprise CPU exacte.
  Diagnostics sur appareil réel et temps du raccord complet encore attendus.
- **Table S* / finition** : [raccord à faire](../receipts/audit_reponses_20261007/gc_support_fusion/README.md).
  Gc utilise 16 octets par entrée, la finition CPU/CUDA en produit encore 4. Conversion hôte proposée sans transfert
  supplémentaire : +12 octets par boule au final, +16 au pic local avant arrondis du cache ; coût dans C.
- **T/M/V/R** : [branches ouvertes implémentées](../receipts/audit_registre_branches_20261007/README.md),
  admission des offsets CSR à corriger (patch prêt). [Traces concordantes](../receipts/audit_tmv_traces_20261007/README.md) :
  651 tests réussis + un sauté, 15 mutants, neuf cas de chaîne, arbre source vérifié. Pas de contre-qualification native.
  Le patch de livraison reste antérieur à R ; préserver le juge G actuel à la fusion. Dédupliquer les racines avant
  recherche historique conserve ant(b), mais le tri supplémentaire reste à mesurer.

**Performance et mathématiques.** [G4 H contre-lue](../receipts/audit_reponses_20261007/session_h_mesures/README.md),
étape **G CPU, 48 fils**, ng00/01/02 : **80,13 / 63,24 / 75,70 ms K5** ;
**633,63 / 449,31 / 517,49 ms K10**. Un processus par cas, neuf passes chaudes K5, deux K10.
Catalogue CPU K5 : 451,6 / 372,7 / 469,5 ms ; v11 historique 200 / 163 / 195 ms, comparaison descriptive.
**Aucun temps intégré catalogue GPU ou FULL v12 publié.** Trois trames d'une seule séquence ; les 100 ms restent ouverts.

[Gc actif](../receipts/audit_reponses_20261007/gc_index_borne/README.md) : recherche exacte logarithmique sous collisions,
construction d'un gros seau encore séquentielle ; G-L5 reste une dichotomie par requête. File G-L7 conforme au modèle,
3 123 requêtes. Chronos rétablis **disjoints**, correction du prototype précédent ; reconstruction de l'index par appel.
Les [rapports locaux](../receipts/audit_reponses_20261007/gc_rapports/README.md) prennent des minima, passe 0 comprise :
pas des médianes chaudes G4, ratios appariés non recalculables sans les passes manquantes.

[Protocole Gc proposé](../receipts/audit_reponses_20261007/gc_plan_mesure/README.md) : dix tours équilibrent les positions
entre cinq bras. La construction de la table S* est hors G : mesurer catalogue + G puis FULL pour juger le gain net.
[Budget conditionnel H](../receipts/audit_reponses_20261007/t2c_tables/README.md) : tables à 3 ms et résolution divisée
par deux laisseraient encore 40,16 / 32,56 / 38,07 ms, autres coûts inchangés ; ce n'est pas une prédiction de Gc.

Autres leviers : [jointure G-L5 complète](../receipts/audit_reponses_20261007/g_l5_proposition/README.md),
[census/supports](../receipts/audit_g_pistes_20261007/README.md),
[finition `0233`](../receipts/audit_t1b_tour_prepublication_20261007/finition/README.md),
[travail CPU répété `0234`](../receipts/audit_performance_20261007/README.md).
Les [petits nuages H](../receipts/audit_reponses_20261007/session_h_mes_p/README.md) et la
[préparation D6 hors chrono](../receipts/audit_d6_preparation_20261007/README.md) gardent leurs limites de portée.

Canal à quatre fichiers, 73 constats ; clôtures cache `0007/0019`, documents `0235/0236`, cohorte MES-P `0238` conservées.
Capacités 256/64 de `0237` encore ouvertes. Aucun GCP ni donnée sous licence dans cet audit. Contrôles natifs limités aux formats CPU :
deux points catalogue, puis deux sondes G à huit points synthétiques ; aucun benchmark.
