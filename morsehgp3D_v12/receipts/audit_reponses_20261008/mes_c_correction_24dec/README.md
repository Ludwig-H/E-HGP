# MES-C : correction livrée du jugement, 24dec7b85

8 octobre 2026. Contre-audit du pilote, sans moteur, CUDA, GCP ni lecture de coordonnées. Aucun temps de campagne n'est admis ici.

**Conclusion.** La livraison `24dec7b855fe321d886acd9d938b2b97e8fe4643` reprend exactement la fonction `verdicts` de la [proposition de cohorte](../mes_c_livraison/README.md), après application au pin `83ed7620d`. La comparaison porte sur l'AST, hors commentaires et positions ; elle ne prétend pas que tout le fichier est identique. L'extraction supplémentaire du verdict global dans `overall` conserve les 108 combinaisons des trois états de C1–C3, du mode essai et de la présence de contrôles.

C3 exige désormais tous les couples nuage difficile/voie CPU ou appareil, une seule fois, à K5/W48. Une absence, un doublon, une autre configuration ou un cas non joué rend la cohorte non évaluée. Une cohorte complète comportant un refus ou une expiration reste un résultat « non tenu ». Hors essai, un critère non évalué ou un contrôle manquant entraîne le refus du jugement global. Ces correctifs permettent de clore ce sous-périmètre de cohorte et de propagation du verdict ; ils ne qualifient pas la campagne.

**Règles conservées.** Les AST de `fit`, `session_values`, `fits_of`, `probe_argv` et `expected` sont inchangés. Les seuils restent 2 ms pour C1 et `241.3e6 / 64740` ns/site pour C2. Le README qualifie maintenant C1 d'ordonnée à l'origine extrapolée et C2 de pente OLS, distingue cette pente du plafond individuel « jamais supérieur » et corrige la tournée à 147 nuages, donc 146 entre deux prises d'une même trame. Les limites de comparaison restent celles du [contrôle statistique](../mes_c_statistique/README.md) ; aucune nouvelle règle d'adoption n'est introduite.

**Exécutions bornées.** La porte officielle passe en Python normal et `−O`, avec les mêmes diagnostics : cohorte 9, verdict 5, droites, critères, difficiles, empreintes et usage. Le runner officiel tue 11/11 mutants. Le contrôle indépendant rejoue également chaque mutation séparément : syntaxe valide, code 1 et diagnostic de la porte, sans `Traceback`. Les onze morts ne représentent pas onze fausses adoptions : certaines portent sur les diagnostics internes. En particulier, le mutant qui retire un contrôle d'empreinte par passe reste signalé par le contrôle global entre prises. Les nouvelles mutations de cohorte, de fils et de propagation « non évalué » sont bien détectées causalement. Détails dans [results.json](results.json).

Les tests emploient uniquement leurs sondes Python fictives en `--essai`. Le seul outil d'environnement appelé est `cmake --version` ; aucune construction ni sonde native. Les fichiers Git sont extraits dans un répertoire temporaire ; aucune source produit n'est modifiée.

**Attribution.** La session MES-C a été lancée sur `83ed7620d`, paquet `86eb5522…`, et non sur `24dec7b85`. Le message du commit correctif annonce explicitement un rejeu hors ligne de ses bruts. Le [reçu de provenance](../session_mes_c_provenance/README.md) porte l'exécution et sa clôture. Les métadonnées de lancement rappelées dans [capture.json](capture.json) ne constituent pas une nouvelle qualification de temps ; l'admission des bruts reste distincte.

Rejeu, à partir d'un dépôt contenant les deux commits :

```sh
python -B check.py --repo /workspaces/E-HGP
```

Le lecteur vérifie les empreintes des neuf blobs Git épinglés, applique la proposition sur une copie, compare les fonctions et seuils, puis exécute les seules portes Python décrites ci-dessus. Le résultat attendu est [results.json](results.json). Les empreintes de fermeture sont dans [SHA256SUMS](SHA256SUMS).
