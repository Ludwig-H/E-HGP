# Préflight de la tranche24

Avant gel des sources, le20 septembre2026 :

- Compilation GCC13.3 Release et Clang18 ASan/UBSan, avertissements stricts.
- Première porte avant ajout des identités de compteurs :4 709 contrôles PASS.
  Après ajout des identités :11 702 contrôles PASS dans les deux builds.
  Les vérifications de tri des vues avant normalisation ont ensuite été
  ajoutées ; seules les captures closes font autorité sur la version finale.
- Sondes Release manuelles :32/far/cap/adversarial/K10,
  32 000/cap/K5 et256/adversarial/K5 PASS. Temps de ces préflights non promus.
- Préflight lecteur : trois portes, six sondes32 et quatre sondes grandes
  ou adverses sous Python−O PASS. Inventaire prévu :151 sources.
- Une demande initiale de compilation de la nouvelle cible sonde avant
  régénération CMake a produit « No rule to make target ». Configuration
  relancée puis compilation réussie ; aucun test ne s'était exécuté.
- `python tools/check_docs.py` :598 documents actifs validés à ce point.
  `python tools/check_implementation_status.py` :20 phases validées,
  registre formel inchangé.

Aucune capture de test en échec ni refus géométrique n'est dissimulé par
ces préflights. Les sorties détaillées des qualifications finales sont
conservées dans leurs propres répertoires, avec fermeture des hashes.
Les contrelectures ont corrigé deux déclarations, pas des décisions
géométriques : coût des tris de coquilles et périmètre des compteurs de
rejet des boîtes. GCP non utilisé.

Contrôle d'index final : `git diff --cached --check` ne signale qu'une
ligne blanche terminale dans `mutants/run_mutants.py`, déjà épinglé par
les captures de mutations. Elle est conservée pour ne pas changer le
harnais après sa qualification ; le reste du delta passe ce contrôle.
