# Qualification portable du raccord résident — r1

Capture fraîche close, 27 septembre 2026. **50 commandes PASS**, premier
essai sans échec, builds GCC Release et Clang ASan/UBSan/LSan. Les lecteurs
normal et `python3 -O` rejugent commandes, codes, groupes de processus
fermés, sources et binaires. Aucune compilation CUDA ni utilisation GCP.

[Prototype et périmètres](../../../audits/b_q34_filtered_resident_20260927/README.md),
[capture](capture.json), [bilan rejugé](summary.json).

Par build : 85 cas, 255 passages portables, 75 990 requêtes et 29 901
survivantes cumulées ; 276 refus internes. Couverture nouvelle :
masques denses/compacts, alias appelant, origine/K/index, ordre et bases
brutes après rectangles fermés, W1/W4, Qr/Q petits et supérieurs à INT_MAX,
Pool/fallbacks, E nul et E positif à sortie nulle. Les compteurs physiques
sont comparés entre configurations. Les formules >2^32 ne sont pas une
exécution de milliards de paires.

22 refus CLI, trois mutants causaux compilés : fermeture indue, base brute
incorrecte, contrôle d'origine omis. Quatre injections portables simulent
`stack_failure` rectangle ou paire et refusent tout résultat réussi ; il
ne s'agit pas de fautes effectivement injectées sur GPU. La fixture frame
de 12 points est artificielle, P=E=S=63, jamais décrite comme LiDAR.

Ce reçu ne qualifie ni CUDA, ni un gain de performance, ni la tour FULL,
ni une borne sous-quadratique globale. Les unités CUDA sont seulement
épinglées et relues ; la future qualification device aura son propre reçu.

```sh
python3 morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
python3 -O morsehgp3D_v9/audits/b_q34_filtered_resident_20260927/run.py --readback morsehgp3D_v9/receipts/q34_filtered_resident_20260927/r1
```
