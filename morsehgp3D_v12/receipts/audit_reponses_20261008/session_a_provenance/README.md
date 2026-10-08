# Session A : retour complet, expiration d'une porte distincte du pilote

La session `v12.20261008.t2da` est récupérée et arrêtée, mais **n'est pas globalement réussie** : `failed_remote`, worker1 et `DONE=3`. Le pilote de mesure termine code0 ; la commande LiDAR est la seule commande en échec, sur expiration externe. Aucun incident du contrôleur n'est enregistré. Arrêt ciblé certifié code0, une tentative, `RUNNING → TERMINATED`, garde intacte et réserve libérée. Aucun moteur, compilation ou appel distant exécuté par l'audit.

| Commande | Code / statut | Mur du processus | Preuve primaire |
|---|---|---:|---|
|0 socle_ctest|0 / ok|141,465 s|719/719 tests `Passed`, aucun skip|
|1 t2da_pilote|0 / ok|297,856 s|rapport,61 JSONL et constructions récupérés|
|2 lidar_ctest|124 / timeout|180,003 s|6/7 tests `Passed` ; `mhgp12_tower_chain_m0` démarré sans résultat final|
|3 mutants_tour|0 / ok|96,721 s|un CTest `mhgp12_mutants_tower` passé|

Le journal des mutants ne détaille pas ici chaque exécution mutante : le CTest passé n'est pas transformé en nouvelle preuve individuelle de causalité. Les durées de ce tableau sont des murs de commandes, **pas des latences FULL**. La porte LiDAR inachevée n'est ni une réfutation géométrique ni une réussite. Réciproquement, elle n'efface pas les journaux du pilote ; leur admission et leur jugement restent séparés.

## Sources effectives : archive et plan priment sur le commentaire902

L'enveloppe et le paquet sont au commit **`5f5c0c83fcb7df842996f584e3870bfbf3017d89`**. Le bras **avant est réellement27eca**, déjà équipé du catalogue C et du pool :

| Bras | Archive hachée | Sources Git vérifiées |
|---|---|---|
|avant|7 144 003 octets, SHA `929c37449f727d38e5c6df325a9bb254d9eb7c0ca4f27d7991772df37a25110c`|351 fichiers exactement `27eca166b9b678bd97c9469e7071823097fc0c0b`|
|après|paquet SHA `8cd6e5facefc6a1ef0ffbaa2a4593f2e3ca573f669dcc5d36ef6bcecaec1e8d5`|356 fichiers exactement `5f5c0c83fcb7df842996f584e3870bfbf3017d89`|

Périmètre complet `src/`, `bench/`, `tests/`, `cmake/`, `CMakeLists.txt` ; pilote vérifié séparément, SHA `ea9023cf…`. L'argument `--avant-archive`, son SHA dans le plan et dans le rapport, puis les octets de l'archive concordent. Le nom livré est `v12_src_27eca166b.tar.gz`. Sonde FULL avant `201119a7…`, après `e3a54104…`. Empreintes binaires déclarées identiques à l'ouverture et à la fermeture du pilote ; aucun ELF récupéré ou réexécuté par l'audit.

**Correction de notre interprétation antérieure.** Le [reçu de comparaison](../t2d_a_comparaison/README.md) affirmait trop fortement que `BRAS=(avant,apres)` imposait902. Le pilote prend l'archive fournie en CLI ;902 n'est ici qu'une ancienne mention du docstring. Le risque « gain C et pool inclus depuis902 » **ne s'applique pas à cette campagne** : le catalogue est identique entre27 et5f, et les corps du pool sont identiques après retrait des seules lignes de commentaires. Les autres changements produit sont dans la tour A. Le reçu historique reste inchangé ; ses exemples de rapports sont synthétiques.

La proposition de comparaison **A/S dans le même binaire** reste pertinente : `apres_sequentiel` est présent dans les identités, pas dans la cohorte de temps décisive. Elle isolerait la route courante tout en gardant le binaire commun. Elle ne mesurerait pas le seul chevauchement, car ouverture, allocations, raccord et ordonnancement changent également. Aucun résultat existant n'est rebaptisé sous cette proposition.

## Fermeture de l'archive et limites

Archive de résultats **440 058 octets**, SHA `ec185db3b8d89217844b85873c393422981dfd0b2e4be1f40cf7c498fbff127e` ; ses **128 entrées de manifeste** sont vérifiées. Plan SHA `a94ec8d84cab8767fa716e71dc0b12325e3cd2c80ae7b12b7484a977fc0bd659`, lancement07:26:35 UTC. Le pilote déclare `adopte` ; ce reçu vérifie cette déclaration et sa provenance, **pas encore sa conformité statistique**.

Les métadonnées déclarent les trois ng00–02 et l'archive37 trames. Aucun XYZ, ID ou tar de scènes n'a été ouvert. L'archive27 lue est une **archive de sources**, conservée hors Git. Rapport,61 JSONL, quatre journaux de construction et six flux de portes sont copiés hors Git :73 fichiers,1 896 581 octets. Aucune identité ni commande privée n'est copiée dans le reçu.

Le lecteur réutilise les vérificateurs d'archive et de sources déjà publiés pour MES-C. Rejeux normal/−O concordants, sources et fichiers de session épinglés avant/après. [capture.json](capture.json) porte les hashes et résultats publics ; [SHA256SUMS](SHA256SUMS) ferme le reçu.

```sh
python -B check.py --repo DEPOT --session SESSION --before-sources ARCHIVE27 > /tmp/a-normal.txt
python -B -O check.py --repo DEPOT --session SESSION --before-sources ARCHIVE27 > /tmp/a-opt.txt
cmp /tmp/a-normal.txt /tmp/a-opt.txt
```
