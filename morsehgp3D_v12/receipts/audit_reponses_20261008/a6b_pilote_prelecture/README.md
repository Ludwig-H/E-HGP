# A6b — raccord du pilote avant campagne

8 octobre 2026, Codex. Lecture d'une **copie non commise**, doublement hachée à 15:25:23 UTC, sur `150392f99` ; pilote encore identique sous `dbdc71753`. Aucun moteur, compilation, GPU, contrôleur ou jeu de coordonnées consulté. Ce reçu ne juge aucune future campagne.

Le pilote `mes_t2d_a6b/pilote_t2d_a6b.py` (`80a59ed7…`) conserve la fermeture du pilote A6 corrigé en `6497ed3b5`, déjà relu dans [a6_retrait_qualification](../a6_retrait_qualification/README.md). Sur ses 31 fonctions, 28 AST sont identiques après neutralisation des seuls noms A6/A6b et des docstrings. `verifier_cohorte` ne change qu'un message d'erreur ; `tableaux` change ses étiquettes ; l'autotest ajoute les deux nouveaux seuils. Les huit cas publics passent, code 0, sans stderr, sous Python normal et `-O`.

La cohorte est reconstruite depuis la commande et le manifeste : GPU/u21/K5/W48, mêmes 21 grandes trames (>60 000 sites) annoncées et mêmes rotations, exactement trois bras et tous les tours demandés ; ng00–02, K10, W1, uniformes et les 37 trames sont également exigés dans l'identité. Les nombres effectifs du futur manifeste et la provenance du paquet restent à contre-vérifier lors de sa clôture. Les succès exigent un code entier 0, la configuration exacte et le rejeu strict LF des journaux hachés. Une prise manquante ou échouée demeure un refus ; les empreintes ELF initiales/finales sont exigées, A/A doit employer le même binaire. Le mode CPU `--essai` conserve son verdict forcé `essai` et ne qualifie pas des temps CPU.

La nouvelle référence déclarée est **R1 `47feedc96`**, pas l'ancien socle A6. Les arbres `src/` de 47 et 150 sont identiques ; les deux sondes déclarent le même défaut de cache de 8 Gio. Le pilote vérifie le SHA fourni de l'archive avant, mais ce SHA seul ne démontre pas sa correspondance à Git 47 : la comparaison extérieure paquet→Git reste nécessaire. Les résultats de l'ancienne campagne A6 ne deviennent pas ceux d'A6b.

Règle déclarée à 14:47 UTC, conservée ici sans modification : borne supérieure du bootstrap à 95 % **<0,95** pour les grandes trames et **<1,01** pour chacune de ng00–02 ; veto si la moyenne géométrique A/A sort de [0,985 ; 1,015]. Les grandes trames donnent une moyenne de logarithmes par tour sur les secondes visites ; les ng donnent le rapport des médianes chaudes par processus. Le bootstrap rééchantillonne les tours, 10 000 tirages, graine 20261008. Le seuil grandes porte donc sur cet agrégat, pas sur chaque trame. Les étroites fluctuations A/A historiques motivent un seuil ; elles ne garantissent pas la précision d'une campagne future.

Limites de preuve : les huit autotests appellent le juge synthétique sans rejeu de journaux (`verifier=False`) ; ils vérifient les décisions statistiques, pas une exécution native, une cohorte future ou une puissance statistique. La fermeture héritée est établie par comparaison de source et par les preuves précédentes, sans nouveau banc. Aucun défaut nouveau du juge n'a été trouvé dans cette lecture ciblée.

Rejeu léger, chemins vers le dépôt Git et la copie extérieure (aucune sonde appelée) :

```sh
python -B -S check.py /workspaces/E-HGP /workspaces/.ehgp-auditors/evidence-snapshots/a6b_pilote_prelecture_20261008
```

`check.py` vérifie les empreintes avant/après, la comparaison AST, le socle `src/`, les défauts de cache, puis les autotests publics normal/`-O`. Les sources ne sont pas dupliquées dans ce reçu. `capture.json` distingue le pin Git des fichiers non commis ; une livraison ultérieure demande son propre raccord d'empreintes.
