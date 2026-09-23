# Erratum au reçu des arêtes sans sortie

23 septembre 2026, relevé par le contre-audit B
[`CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md`](../../audits/CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md).
Les fichiers bruts et leurs empreintes ne changent pas ; seules deux
lectures du README sont rectifiées.

- La part des arêtes sans sortie n'est pas « 91 % ». Sur
  `harness/s00_k5_w8_base_instrumented.out`, les trois classes mortes font
  1 387,3 G cycles contre 47,5 G pour les arêtes qui émettent et 184,3 G pour
  le filtre de témoins de paire : **96,7 %** des cycles classés par arête,
  **85,7 %** en comptant le filtre (94,4 % et 87,5 % à K10). Parts du
  tableau sur le total filtre compris : 11,4 / 8,6 / 18,8 / 58,2 / 2,9 %.
  Compteurs `rdtsc` non sérialisés, préemption comprise : ordres de
  grandeur, pas des temps exacts.
- Le cover moyen d'une arête morte est de 1 704 sites toutes classes
  mortes confondues ; 1 453 est la moyenne des seules arêtes mixtes mortes.

Les autres chiffres (gains CPU du prototype, condensés) sont inchangés ; le
reçu G4 R3 porte depuis l'ablation appariée sur la tour complète.
