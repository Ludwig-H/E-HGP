# B : périmètre de la campagne active et prochain FULL courant

8 octobre 2026. Préparation vérifiée, **aucun résultat B ni arrêt qualifié dans ce reçu**.
La capture locale précède leur retour. Aucun moteur, build, contrôleur cloud ou payload de scène lu par l'audit.

La session `v12.20261008.t2db` emploie le paquet Git `41d4d828b`, SHA `dc717c43…c843ad5`,
et le plan SHA `74686743…716711c`. L'archive avant **réellement désignée et présente** est
`v12_src_902041f66.tar.gz`, SHA `0f91cda2…67045` : 342 sources identiques à Git902.
Le paquet contient 357 sources identiques à Git41d4, dans `src/bench/tests/cmake/CMakeLists.txt`.
Le pilote et le lecteur hors de ce périmètre sont vérifiés séparément. Les empreintes complètes sont dans
`capture.json` ; relecture normale et −O identique.

Les huit bras mesurés sont construits depuis l'archive avant : 902, son A/A, puis les six substitutions B.
Le paquet courant n'est pas un bras ; il fournit notamment le pilote et les portes. Ainsi G compare B sur
902, **avec le même ancien pool entre bras**. Le FULL informatif est aussi construit depuis 902/902+B,
catalogue CUDA, séquentiel, K5/ng00–02/W48. Ni A ni le nouveau catalogue C ni le pool5b ne sont dans ces
bras. La conformité des dix fichiers B au produit livré ne transfère pas leurs temps au FULL courant.
La base902 n'est pas déduite d'une annotation du script : plan, SHA de l'archive et octets Git concordent.

Le protocole G conserve dix tours de huit bras, dix passes (première exclue), rotation sans inversion et
veto A/A ±1,5 %. Voir la [contrelecture du raccord livré](../t2db_raccord/README.md) pour les quatre
résidus FULL corrigés. Les quatre délais du plan actif sont 900/1500/900/1200 s ; le délai LiDAR est
900 s, contre 180 s dans A. Il reste une limite effective à contrôler au retour, pas une réussite présumée.

Pour isoler ensuite B dans le produit courant, une base avant concrète est `da5c21fc1` (A+C+pool, avant B).
Les **31 préimages et postimages des six bras** lui correspondent exactement ; les dix fichiers du bras
après sont identiques à ceux livrés en41d4. Cette proposition de base n'est pas celle utilisée dans la
campagne en cours. Préannoncer la nouvelle archive et les empreintes, puis mesurer les deux FULL dans
le même régime recouvert, mêmes options/catalogue/budgets et cohortes. Au pin41d4 il faut `--recouvert`
et un lecteur de ce schéma ; le pilote B actuel exige le schéma séquentiel. Toute future bascule du défaut
de `full_probe` impose donc d'expliciter l'option ou le schéma du pilote avant la campagne correspondante.
Conserver G isolé comme diagnostic séparé : son coût CPU séquentiel ne se substitue pas à la fenêtre G
recouverte. Les prises CPU, petites trames, K10 et massifs du nouveau défaut restent à qualifier ; MES-C2
au pin27 et L1r ne les qualifient pas après A. Pas de campagne supplémentaire lancée ici.

Le diagnostic A ci-dessous réutilise les [bruts déjà admis](../session_t2da_admission/README.md), source5f5 :
37 trames, trois processus, seconde visite seule, soit111 chaudes. Calcul par passe, puis médiane des trois
prises par trame, puis médiane/maximum de ces37 médianes. Il ne somme pas des médianes d'étages.

| Quantité, ms | Médiane des37 médianes | Maximum des37 médianes | Trames dont médiane >100 ms |
| --- | ---: | ---: | ---: |
| FULL mesuré A | 160,567405 | 319,740140 | 26 |
| P+C | 46,475373 | 82,401861 | 0 |
| P+C+G | 118,452341 | 250,827322 | 23 |
| FULL−queue, scénario | 120,522644 | 252,393012 | 23 |

Enlever seulement la queue finale, en conservant tous les autres coûts mesurés, ne suffit donc pas pour
23 trames. Le résidu `FULL−queue−(P+C+G)` est conservé : médiane des37 médianes1,246769 ms,
maximum brut6,443147 ms. G est une fenêtre murale avec forêt simultanée, **pas un temps de calcul pur**.
Ce calcul n'est ni une borne d'impossibilité globale ni une prédiction de B : B peut changer le travail,
les allocations, les conflits de ressources et donc cette fenêtre. Les minima/maxima des111 passes et
la statistique par trame sont distingués dans `results.json`.

Rejeu, métadonnées et trois journaux de mesures seulement :

```sh
python3 check.py --repo DEPOT --session SESSION_B \
  --before-sources ARCHIVE_SOURCE_902 --returned-a RETOUR_A
python3 -O check.py --repo DEPOT --session SESSION_B \
  --before-sources ARCHIVE_SOURCE_902 --returned-a RETOUR_A
```

Le lecteur réutilise `session_mes_c_provenance/source_check.py` publié, épinglé par SHA ; les sources sont
lues depuis les objets Git, pas le worktree mobile. Pas de journaux ni archive copiés dans ce reçu.
