# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`136e07628`** ; prototypes et correctifs non publiés épinglés par
hashes séparés. Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. États au [registre unique](CONSTATS.md).

**À traiter pendant l’intégration.**

- **Avant adoption CUDA, `0018`** : [juge permissif confirmé](../receipts/audit_reponses_20261007/cuda_juge_prepublication/README.md).
  Étapes absentes, processus dupliqués, comptes manquants ou durées négatives encore « adoptés » ;
  diagnostics des premières passes perdus par le pilote. Dix contre-rapports, normal/−O identiques.
  Corriger le schéma, les cohortes et les critères de mutants avant de conclure la campagne.
- **Profils D6, `0207/0018`** : `9b2747eff` compare les binaires séparés ; [contre-audit du pilote](../receipts/audit_d6_20261007/README.md).
  Référence u21 absente et sorties tronquées encore admises ; correctifs du plan et du
  [lecteur strict proposés](../receipts/audit_reponses_20261007/d6_admission_proposition/README.md), non intégrés.
  ×8/×2048 conserve la géométrie avec niveaux ×64/×4194304, sans précision nouvelle.
  Égalité des comptes seule insuffisante. [Préparation hors chrono](../receipts/audit_d6_preparation_20261007/README.md),
  records de 16→32 octets : mesurer la latence intégrée avant le choix D6 à 3 %.
- **Juge G, `0018`** : les ordres manquants sont désormais refusés ; [contre-rejeu et erratum](../receipts/audit_reponses_20261007/g_juge_integration/README.md).
  Ma proposition initiale omettait la ligne finale `exit` ; le développeur l’a corrigée avant intégration.
  Résidu de type limité : `exit.order=False/0.0` encore admis ; patch d’une ligne contre-jugé.
  Pas d’erreur géométrique trouvée dans T1/T3/census/NUM-GARDE lus.
- **Catalogue CUDA** : [compteurs corrigés dans le prototype actif `66c41ed`](../receipts/audit_reponses_20261007/cuda_compteurs_reponse/README.md),
  réécritures appareil/hôte séparées et sommées ; nettoyages mémoire conservés. Lecture statique,
  comparaison des diagnostics sur GPU réel encore attendue. GPU ≤32 sites/16 bits locaux, sinon reprise CPU exacte.
- **Tour T/M/V et R** : [branches ouvertes désormais matérialisées dans le prototype](../receipts/audit_registre_branches_20261007/README.md).
  Lecture mathématique/concurrence favorable ; admission à compléter des offsets CSR (patch prêt).
  Temporaire `4 P_R` et sortie `4 A` coexistent ; aucune borne `A≤naissances−1`.
  Qualification du nouveau R en cours chez le développeur ; préserver le juge G durci au merge.

**Clôtures vérifiées au registre.**

- **Livraison `28cf75cd1` confirmée** : [M6, G et MES-P](../receipts/audit_reponses_20261007/livraison_28cf75/README.md).
  `null` refusé par M6 (71 cas), couverture G corrigée (15 témoins), **`0238` clos** pour séparation/cohorte,
  y compris régimes entièrement échoués. Rejeux normal/−O identiques ; `0018` reste ouvert.
- **Cache, `0007/0019` clos** : `7b7d025b3`, [12 contrôles, mutant causal et poison Clang/GCC](../receipts/audit_reponses_20261007/cache/README.md).
  Corps courant `5c885dbf…` [équivalent au testé](../receipts/audit_reponses_20261007/cache_equivalence/README.md).
  Portée cache, sans transfert à FULL, aux performances ou à TSan.
- **Documents, `0235/0236` clos** : [corrections confirmées](../receipts/audit_reponses_20261007/docs/README.md), livrées en `5682d00f5`,
  hashes identiques à la capture. Portée documentaire seulement. **`0237` reste ouvert
  techniquement** : T1-c doit lever les capacités 256/64 ; q_min≤4 ne les borne pas.
  Préciser « transferts du raccord complet », M5 comptant déjà les siens.

**Performance.** [Session G4 H contre-lue](../receipts/audit_reponses_20261007/session_h_mesures/README.md), étape **G sur CPU** à 48 fils :
**80,13 / 63,24 / 75,70 ms à K5** ; **633,63 / 449,31 / 517,49 ms à K10**, ng00/01/02.
Un processus par cas, neuf passes chaudes K5 et seulement deux K10. Catalogue CPU K5 inchangé :
451,6 / 372,7 / 469,5 ms ; historique v11 200 / 163 / 195 ms, comparaison descriptive, pas A/B apparié.
Aucun temps intégré catalogue GPU ou FULL v12 publié ; les étapes isolées ne s'additionnent pas en preuve FULL.
Les [petits nuages H](../receipts/audit_reponses_20261007/session_h_mes_p/README.md) confirment les tableaux sur
123 scènes réelles communes ; leur ajustement ne mesure pas le coût d'ordonnancement du pool.

[Tables et résidence](../receipts/audit_reponses_20261007/t2c_tables/README.md) : tables à 3 ms et résolution divisée
par deux laisseraient encore 40,16 / 32,56 / 38,07 ms, reste inchangé. Préparation/validation/libérations à traiter.
Le prototype Gc inclut désormais les tables dans `resolve_ns` ; comparer des frontières homogènes.
Réutiliser les buffers entre trames ne dispense pas de reconstruire les tables d'une nouvelle trame.

[G-L5 : jointure exacte proposée](../receipts/audit_reponses_20261007/g_l5_proposition/README.md), sans produit
cartésien sous collisions ; 180 modèles, 7 560 réponses. Compteurs/reprise des échecs et coexistence avec la table
actuelle explicités. La baisse du temps total reste à juger par MES-G3.

[Pistes complémentaires G](../receipts/audit_g_pistes_20261007/README.md) : espaces census inutiles à K1,
stockage borné avec refus de coquille différé, tests évitables sur support certifié. Propositions, gains non mesurés.

La [finition commune en cours](../receipts/audit_t1b_tour_prepublication_20261007/finition/README.md) répond à `0233` : rangs,
tris stables, chaînes exactes, CSR et table. Qualification et gains restent à mesurer.
Le [diagnostic performance](../receipts/audit_performance_20261007/README.md) maintient les priorités : finition série
122,5–171,3 ms ; travail CPU de feuille répété (`0234`) ; G-L3 rejeté, sondes puis proposition
à traiter dans G. La prochaine campagne doit inclure reprises, transferts, matérialisation,
coexistences mémoire, ablations appariées et FULL multi-séquences. Les 100 ms restent ouverts.

Canal à quatre fichiers, 73 constats ; [historique conservé](../receipts/audit_clotures_20261007/README.md).
Aucun jeu sous licence, nouvelle mesure moteur ou utilisation de GCP par cet audit.
