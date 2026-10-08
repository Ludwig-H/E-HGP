# TMVR livré : équivalence repo6 et portée de l’intégration

Contre-lecture du 8 octobre 2026, commit
`7398aed7d8dd5595c880aa8a8677ecf05a8f2a32`, sans nouvelle exécution native.
`backend=cpu_reference`, `quantification=quantized_u21_input_only`,
`objet=full_pi0`, `public_status=not_claimed`.

## Sources réellement livrées

Le prototype repo6 est resté à l’empreinte
`1b5d1fb38c2c2cbcbfaac86525eeaaeeab0f044afb1a4aa490a6f249d5b73279`,
382 fichiers, base `a5e0dbc77`, patch `b3c78ae3`. Comparaison avec les **objets
Git du commit livré**, et non avec des fichiers vivants : **381 fichiers
identiques sur 382**. Seul `docs/PLAN.md` diffère : annonce de livraison T2-b,
bilan local et ajout des résultats J ; aucun changement exécutable.

Le périmètre est celui de `COPIED` du lanceur de mutants : `CMakeLists.txt`,
`cmake`, `src`, `cli`, `bench`, `tests`, `tools`, `reference`, `docs` ; ni build,
ni reçus, ni `microbancs`, ni dépendances externes. Les 128 fichiers de `src`,
40 de `bench`, 167 de `tests` et toute la configuration sélectionnée sont
identiques. Les 30 chemins code/configuration/tests touchés par le commit
correspondent exactement au prototype ; ses quatre autres chemins sont deux
documents et deux reçus développeur. Listes et hashes agrégés dans
`capture.json`, recalculables par `check.py`. Cette égalité rattache les preuves
repo6 au code livré ; elle ne reconstruit pas la provenance d’une compilation.

## Preuves de correction et sélection intégrée

Les preuves du [jalon repo6 u21](../../audit_tmv_repo6_u21_20261008/README.md)
s’appliquent donc à ces mêmes corps : MES-M0, neuf cas × deux modes avec les
entrées de référence, identité à l’octet u21 ; chaîne native CPU catalogue
T1 → G-c → T/M/V/R → FUL1, neuf empreintes **sémantiques** FULL égales à la v11,
1 contre 8 fils. Les octets de la chaîne T1 ne sont pas annoncés égaux à la v11 :
les présentations rationnelles peuvent différer. JUG-EMST porte sur l’ordre un.
Les [27 mutants repo6](../../audit_tmv_repo6_mutants_20261008/README.md), dont
naissances canoniques, branches ouvertes et admission des offsets R, sont
également rattachés à des sources identiques. Les traces ne sont pas rejouées
nativement ici et les données/vidages ne sont pas relus par ce reçu.

Le journal d’intégration nommé `build_v12_u21.ctest_tmv.log` précise l’annonce
« 717/717 » : **717 sélectionnés, 716 Passed, une sentinelle LiDAR Skipped,
zéro échec**, sur 748 portes enregistrées. Les 26 ajouts par rapport aux
691 sélections de repo6 sont tous `mhgp12_reference_diff_v10*` ; aucune porte
repo6 perdue. La configuration u21 n’enregistre pas les différentiels v11 du
catalogue/de G. Les 16 portes rapides nouvelles de TMVR sont passées : onze
groupes natifs et leur inventaire, oracle normal/−O, forme de niveau normal/−O.
MES-M0 et la chaîne LiDAR sont des portes longues/conditionnelles distinctes ;
le total 717 ne remplace pas leurs campagnes repo6.

`integration.py` relit quatre journaux nommés et épinglés (configuration,
construction, CTest intégré, CTest repo6). Il ne prend pas le `CTestTestfile`
vivant, déjà régénéré pour de nouvelles portes FULL. Cache et `LastTest.log`
ne sont que des observations ponctuelles conservées dans `capture.json` :
Release/u21, CUDA OFF, tous modules, sources main ; dumps MES-M0 et JUG-EMST
non configurés. La commande CTest extérieure et son code de sortie n’ont pas
été retrouvés ; le zéro échec vient du bilan primaire, sans inventer ce code.
La preuve forte est l’identité des sources Git avec repo6 qualifié. Ces journaux
corroborent les portes intégrées, sans certification rétroactive du binaire
actuel d’un build réutilisé.

## Clôtures circonscrites et limites

- **CST-0105** : `forest.hpp:178–182` déclare `rang(leaf) ≤ r` ;
  `vertical_images.cpp:18` refuse `tower_query_domain`. La porte passée
  `mhgp12_tower_forest_verticales`, `forest_unit.cpp:275–280`, vérifie le refus
  naissance de rang 1/coupe 0, puis deux succès aux coupes 1 et 2. `requetes`
  compare aussi les coupes au parcours des parents sur 60 forêts construites.
  Cela permet la clôture du défaut documentaire et de sa porte u21.
- **CST-0107** : `verticales`, lignes 249–251, exige la permutation canonique
  `{1,0,3,2}`, différente de l’ordre des boules. `catalogue` contrôle le raccord
  réel ; `temoins`/`hypergraphes` contrôlent les fusions M. Le mutant
  `naissances_par_cle` et les neuf chaînes renforcent cette clôture u21.
- **CST-0022/0211/0212** : registre R et représentation sont maintenant livrés ;
  les 685 contrôles du pic sur 160 étapes et le mutant d’admission sont acquis.
  Cela ne ferme pas les autres contrats de 0022, ni une qualification globale
  sous budget fini, ni de nouveaux profils numériques.
- **CST-0239** : correction du comparateur livrée, mais aucune qualification
  native `_GLIBCXX_DEBUG` déduite des traces Release.
- **CST-0240 reste ouvert sur le fond** : `forest.hpp:187–189` et
  `forest_validate.cpp:8–9` annoncent la non-lecture des événements/CSR de
  survivants. Le certificat des historiques altérés et sa mutation ne sont
  pas intégrés. Les portes de forêts construites ne prouvent pas leur rejet.

L’intégration u24/u32, les sanitizers sur cette composition, DEBUG et la chaîne
FULL GPU/G4 ne sont pas qualifiés ici. La batterie repo6 large a été arrêtée,
pas conclue en échec produit. La sonde `tower_chain.cpp` reste une chaîne de
correction CPU, une passe et des temps par étage : aucun mur FULL résident,
aucun contrat 100 ms ni nouveau temps GPU ne découle de ce reçu.

## Relecture sans moteur

```sh
python -B check.py --repo DEPOT --prototype REPO6/morsehgp3D_v12 --integration-root SCRATCHPAD
python -B -O check.py --repo DEPOT --prototype REPO6/morsehgp3D_v12 --integration-root SCRATCHPAD
```

Rejeux normal/−O identiques ; sources et quatre journaux stables avant/après.
Sans `--integration-root`, seules les sources sont relues. Aucun build,
benchmark, donnée LiDAR ou appel cloud n’est lancé.
