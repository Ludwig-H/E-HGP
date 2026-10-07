# Audit Codex — état courant v12

7 octobre 2026. Base publiée **`9428db65b`** ; correctifs et prototypes non commis capturés
séparément par empreintes. Cadre : `exploration_v12_hors_registre`,
`cpu_reference ; cuda_g4 pour le catalogue`, `full_pi0`, `quantized_u21_input_only`, `not_claimed`.
États au [registre unique](CONSTATS.md). Aucun nouveau chrono CPU/GPU.

**Retours immédiats au développeur.**

- **Catalogue CUDA et finition en cours** : [contrelecture avant publication](../receipts/audit_t1b_tour_prepublication_20261007/README.md).
  Deux corrections ciblées : propriétaire temporaire des allocations CUDA jusqu'au succès
  (fuite après copie/synchronisation refusée ; réserve épinglée retenue sans allocation),
  et compteur des feuilles réécrites, actuellement toujours nul. Effets établis par lecture,
  aucune panne CUDA injectée. La finition commune réalise les rangs, tris stables, chaînes
  exactes et CSR proposés pour `0233` ; qualification et gains encore à mesurer.
- **Tour T/M/V en cours** : mêmes événements et forêt possibles pour deux hypergraphes
  dont les branches ouvertes diffèrent. La première forme de R perd cette information :
  conserver les branches par cellule retenue, ou déclarer leur report à T3. Modèle abstrait
  reproductible, aucun défaut géométrique ni pi0 faux établi. Plateaux et verticales cohérents
  à lecture ; copies du socle à rebaser sur le correctif cache avant qualification livrée.
- **Cache, `0007/0019`** : [correction et porte complétées](../receipts/audit_reponses_20261007/cache/README.md)
  sur `buffer.cpp` SHA `1844a7d6…`. Bras 300+300 et 300+200 Kio : 12 contrôles passent ;
  supprimer la relecture de `held` cause trois échecs du second bras. ASan rejoué sous
  Clang/GCC sur ce corps : garde saine, deux lectures interdites détectées. Clôture après
  publication ; aucun transfert à FULL, performances ou TSan.
- **Juge M6, `0018`** : [patch minimal prêt](../receipts/audit_reponses_20261007/m6_proposition/README.md),
  hors produit : types/plages stricts, effectif validé, schémas v1/v2 fermés, refus conservés.
  Douze faux succès deviennent des refus ; porte officielle 54 cas verts, normal/−O identiques.
  Précision : le reçu G4 A porte explicitement v1 ; sa compatibilité est conservée.
  Clôture après intégration et contre-épreuve du corps livré. M5 et G1 déjà corrigés dans
  leurs [portées contre-jugées](../receipts/audit_reprise_20261007/README.md).
- **Petits nuages, `0238`** : [pentes séparées, résidu de cohorte](../receipts/audit_mes_p_corrections_20261007/README.md).
  Reconstruire K et régimes depuis **toutes** les prises : un régime sans aucun succès
  impose une cohorte commune vide. Il disparaît encore du calcul actuel. Cinq témoins JSON,
  temps inventés, normal/−O identiques ; aucune nouvelle performance HGP.
- **Documents, `0235/0236/0237`** : [réponse en cours](../receipts/audit_reponses_20261007/docs/README.md).
  Budget complet non confirmé et sphère arrondie correctement décrits : clôtures documentaires
  après publication. Voie large T1-c encore à livrer ; préciser « transferts du raccord
  complet », car M5 comptait les siens. q_min≤4 ne borne ni coquille ni liste candidate.

**Priorité performance maintenue.** Catalogue CPU K5 : **451,6 / 372,7 / 469,5 ms** sur
ng00/01/02, 48 fils, médiane de neuf passes chaudes. Référence historique v11 :
200 / 163 / 195 ms, rapport descriptif ≈2,3, pas A/B apparié. La nouvelle voie hybride
rejoue sur CPU les feuilles hors GPU (32 sites/16 bits locaux) ; son mur de catalogue
comprend reprises, transferts et matérialisation. Aucun résultat intégré encore publié.

Le [reçu performance](../receipts/audit_performance_20261007/README.md) reste la base :
finition série 122,5–171,3 ms (`0233`) ; paires doublées et census CPU poursuivi après rejet
(`0234`) ; aval CPU conservé 145,1–201,7 ms même avec parcours/feuilles gratuits (`0235`,
scénario conditionnel). G-L3 reste rejeté : +3–4 % malgré 81–83 % de censuses évités.
Pour G, sondes puis proposition passent avant de nouvelles variantes de census.

Acquis clos conservés au registre : identités EMST `0232` et découpes `0218` dans leurs
[portées bornées](../receipts/audit_reprise_20261007/README.md). Les preuves mathématiques
FULL rationnel, clés de supports et conditions des caches sont dans le reçu performance.
Prochaine vérification : correctifs livrés, finition et tour raccordées, puis ablations
appariées et FULL multi-séquences sur G4 gardée. Les 100 ms restent ouverts. Canal à quatre
fichiers ; détails dans `receipts/`, aucune donnée sous licence ni nouveau recours GCP.
