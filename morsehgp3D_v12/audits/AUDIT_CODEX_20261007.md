# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`9c5809919`** ; prototypes et correctifs non publiés épinglés par
hashes séparés. Cadre : `exploration_v12_hors_registre`, `cpu_reference ; cuda_g4 pour le catalogue`,
`full_pi0`, `quantized_u21_input_only`, `not_claimed`. États au [registre unique](CONSTATS.md).

**À traiter pendant l’intégration.**

- **Juge M6, `0018`** : `e50114adf` corrige les indices/types et schémas, mais un rapport
  JSON `null` rend encore `mes_m6_ok`, zéro prise et zéro ligne. [Contre-épreuve de la livraison](../receipts/audit_reponses_20261007/m6_integration/README.md).
  Refuser tout non-dictionnaire, y compris `null` : cette garde figurait déjà dans le
  [patch proposé](../receipts/audit_reponses_20261007/m6_proposition/README.md). Les 68 cas officiels et le reçu historique v1 passent.
- **Juge multi-fils G livré en `99fa83246`, `0018`** : [sortie tronquée acceptée](../receipts/audit_t2g_prepublication_20261007/README.md),
  k1 seul pour K5, avec exactement la ligne attendue par CTest. Exiger tous les ordres,
  k1 lié aux sites, types stricts, empreinte complète et régimes distincts. [Patch proposé](../receipts/audit_reponses_20261007/g_juge_proposition/README.md),
  15 comparaisons JSON normal/−O ; empreinte historique complète encore à capturer.
  Aucune erreur géométrique trouvée dans T1/T3/census/NUM-GARDE lus.
- **Catalogue CUDA** : [nettoyage des erreurs corrigé dans le prototype](../receipts/audit_reponses_20261007/cuda_integration/README.md),
  corps `5e215fe2…`, lecture statique seulement. Le compteur des feuilles réécrites reste
  nul malgré leur rejeu. Voie hybride : GPU ≤32 sites/16 bits locaux, sinon reprise CPU exacte.
- **Tour T/M/V et R** : la première forme du registre perd les branches ouvertes des
  hyperarêtes. [Collecte dans T/M proposée](../receipts/audit_t1b_tour_prepublication_20261007/branches/README.md), sans barrière
  par plateau ni nouvelle descente G ; 69 modèles abstraits conformes. Six événements
  peuvent demander 27 branches : budgéter la vraie sortie. Report à T3 possible s’il est
  déclaré ; aucun défaut de pi0 établi. Rebaser les copies du socle avant qualification.
- **Petits nuages, `0238`** : le lecteur livré en `59d604b87` sépare les pentes, mais oublie
  les régimes entièrement en échec dans la cohorte commune. [Patch de cinq lignes prêt](../receipts/audit_reponses_20261007/mes_p_proposition/README.md) :
  cinq contre-témoins et quatre cas officiels passent, normal/−O. Hors produit ; temps inventés.

**Clôtures vérifiées au registre.**

- **Cache, `0007/0019` clos** : livré en `7b7d025b3`, [porte complétée](../receipts/audit_reponses_20261007/cache/README.md), 12 contrôles ; mutant de
  relecture tué par le bras 300+200 Kio. ASan Clang/GCC détecte les deux lectures interdites.
  Le corps courant `5c885dbf…` est [équivalent à `1844a7d6…`](../receipts/audit_reponses_20261007/cache_equivalence/README.md), simple remplacement
  d’un alias. Pas de nouveau rejeu ni transfert à FULL, aux performances ou à TSan.
- **Documents, `0235/0236` clos** : [corrections confirmées](../receipts/audit_reponses_20261007/docs/README.md), livrées en `5682d00f5`,
  hashes identiques à la capture. Portée documentaire seulement. **`0237` reste ouvert
  techniquement** : T1-c doit lever les capacités 256/64 ; q_min≤4 ne les borne pas.
  Préciser « transferts du raccord complet », M5 comptant déjà les siens.

**Performance.** Catalogue CPU G4 K5 inchangé : **451,6 / 372,7 / 469,5 ms**
sur ng00/01/02, 48 fils, médiane de neuf passes chaudes ; historique v11 200 / 163 / 195 ms,
rapport descriptif ≈2,3, pas A/B apparié. [Nouveaux temps locaux de G](../receipts/audit_reponses_20261007/cuda_integration/README.md) sur ng00 :
K5 ≈2,83 s à un fil, 1,05 s à trois ; K10 ≈10,1 s à trois. G seul, codespace partagé,
sans index/catalogue ni T/M/V : ne pas les additionner aux mesures G4. Aucun temps intégré
catalogue GPU ou FULL v12 publié.

La [finition commune en cours](../receipts/audit_t1b_tour_prepublication_20261007/finition/README.md) répond à `0233` : rangs,
tris stables, chaînes exactes, CSR et table. Qualification et gains restent à mesurer.
Le [diagnostic performance](../receipts/audit_performance_20261007/README.md) maintient les priorités : finition série
122,5–171,3 ms ; travail CPU de feuille répété (`0234`) ; G-L3 rejeté, sondes puis proposition
à traiter dans G. La prochaine campagne doit inclure reprises, transferts, matérialisation,
coexistences mémoire, ablations appariées et FULL multi-séquences. Les 100 ms restent ouverts.

Canal à quatre fichiers ; [anciennes cellules déplacées et liens conservés](../receipts/audit_clotures_20261007/README.md),
73 constats maintenus. Preuves et propositions dans `receipts/`. Aucun jeu sous licence,
aucune nouvelle mesure moteur ni utilisation de GCP par cet audit.
