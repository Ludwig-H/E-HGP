# Préflight avant gel —20 septembre2026

Construction neuve `build/v8_q34_candidates_20260920`, GCC13.3,
C++20 Release et avertissements stricts. Première configuration avec
`BUILD_TESTING=OFF` pendant l'écriture des gates ; elle ne qualifie aucune
suite CTest. La bibliothèque et la sonde q34 compilent.

Les appels de préflight `mhgp8_q34_seed_probe 32 uniform 5` et
`mhgp8_q34_seed_probe 32 coplanar 10` terminent à0 et émettent chacun
une boule q3 et une q4 de profondeur2, checksum9232636534885143249.
Les groupes q4 sont respectivement29 et3 ; le régime coplanaire concerne
les sites lointains, pas le tétraèdre local, réellement non plat.

Ces observations antérieures au gel ne remplacent pas les captures closes
avec empreintes de sources et artefacts, ni le passage des oracles.

Le premier préflight ExactBall autonome passe1807 cas et34 460 contrôles.
Un préflight q34 sous Clang ASan/UBSan passe633 appels avant ajout des
dernières fixtures/planchers (profondeur qui redescend et q_min inférieur).
Les sources de test ont ensuite été gelées et les gates reconstruites :
la version finale compte635 appels/24 590 contrôles, pas633. Aucun échec
géométrique de ce préflight n'a été remplacé par une relance favorable.
