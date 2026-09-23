# Contre-audit B — reçu exploratoire des voies mortes

23 septembre 2026. Reçu développeur inspecté en lecture seule :
`morsehgp3D_v9/receipts/q34_dead_edges_20260923/` dans le worktree
`build/v9-open-worktree`. Ses 25 fichiers annoncés dans `SHA256SUMS`
concordent ; les FNV des huit JSON locaux ont été recoupés avec les
trois vrais fichiers 1 mm sans sol, entiers, de 08/000000, 000100,
000200 (39 885 / 35 551 / 45 845 sites). **GCP non utilisé.** Ni le
harnais instrumenté ni les binaires/compilations/entrées ne sont épinglés
par un manifeste complet de source, options et hôte ; conserver ce reçu
comme *exploratoire*, jamais comme qualification FULL/G4 ou preuve de
sous-quadraticité.

## Gain réel de prototype, portée exacte

Le harnais W8, générateur q3/q4 **seul** sur 08/000000/s8, compare une
copie instrumentée de `e54f727c` à un **prototype à balayage linéaire**
du certificat, une exécution par case. Ses lignes brutes rendent :

| K | CPU q3/q4 sans → avec | Mur W8 sans → avec | Présentations et condensé du harnais |
| --- | ---: | ---: | --- |
| 5 | 592,46 → 204,69 s | 86,906 → 28,445 s | 849 780, `f31e41f3e76e03a9` inchangés |
| 10 | 1 600,09 → 557,35 s | 346,633 → 80,756 s | 4 630 767, `c1437caaf8278f22` inchangés |

Il s'agit d'un **fort signal local**, mais ni d'une paire G4, ni de la
tour complète, ni de la version finale à frontière héritée. Le condensé
du harnais trie seulement `(arité, IDs du support, profondeur, **taille**
de coquille)` ; il ne contient **ni BallKey ni IDs de coquille**. L'égalité
ne certifie donc pas à elle seule l'identité du flux géométrique complet.
La preuve du certificat et les petits oracles sont des contrôles
distincts. Les six JSON de tour locale avec certificat ont des digests
égaux à R1/R2, mais **aucun JSON de tour locale sans certificat n'est dans
ce reçu** : cette concordance n'est pas une ablation FULL appariée.
Les deux JSON `frontier_s01` de la nouvelle version à frontière héritée
sont `run_tower=false`, ordres vides et digest zéro ; ils ne créent pas
de nouvelles tours. Leurs temps sur hôte partagé ne se transfèrent pas
au harnais linéaire ni au G4. Par rapport aux JSON locaux linéaires
de même scène/K/W, les tests uniformes diminuent de 32,14 % à K5 et
41,11 % à K10, mais `q34_ms` baisse de 33,41 % à K5 et **augmente de
2,52 %** à K10. Une réduction de tests ne constitue donc pas encore un
gain temporel stable de la version à frontière héritée.

## Deux moyennes et un pourcentage à rectifier

Dans `harness/s00_k5_w8_base_instrumented.out`, les trois classes
**sans émission** ont `1 731 548` arêtes, `2 949 953 989` sites de
covers et `1 387 347 298 173` cycles. Les classes vivantes ont
`312 064` arêtes, `13 497 418` sites et `47 525 634 334` cycles ;
le filtre de paires prend séparément `184 294 004 026` cycles. Ainsi la
part morte vaut **96,69 % des cycles classés par arête**, ou **85,68 %**
si l'on inclut aussi le filtre ; aucune de ces définitions ne donne les
« 91 % » du README/coordination. À K10 les parts analogues sont 94,35 %
et 87,53 %. Les compteurs `rdtsc` ne sont pas sérialisés et incluent les
aléas de préemption ; les appeler « pourcentage du temps q3/q4 » sans
définir dénominateur et méthode est trop fort.
Le tableau de parts du README additionne en outre **12+9+19+60+3=103 %**,
ce qui exige un dénominateur et un arrondi cohérents avant toute citation.

Le cover moyen d'une arête **morte** est
`2 949 953 989 / 1 731 548 = 1 703,65` sites ; celui d'une arête
vivante, `13 497 418 / 312 064 = 43,25`. La moyenne **globale** est
`2 963 451 407 / 2 043 612 = 1 450,10`. Les « 1 453 sites des
arêtes mortes » du README et « 568 » de `docs/PROVENANCE.md` ne sont
donc pas les moyennes de la classe morte de ce brut. Corriger les
libellés/chiffres avant de les reprendre dans une décision de défaut.
Cette rectification ne retire pas le constat qualitatif : les arêtes
sans sortie concentrent énormément de travail aval.

## Coût de préparation et sous-quadraticité encore ouverte

Les deux sondes `frontier_s01` exposent enfin le travail de la version
à frontière héritée : `dead_loads=cover_builds` et **exactement**
`dead_form_sites=cover_sites−2×cover_builds`. Le chargement de tous les
sites couverts n'a donc pas disparu. Sur 08/000100/K5, il vaut
`1 789 938 144` formes et `6 747 549 084` tests de borne uniforme ;
à K10, `4 150 752 371` formes et `17 737 765 076` tests, pour
`n²=1 263 873 601`. Ces masses représentent respectivement
`1,42n²`/`5,34n²` et `3,28n²`/`14,03n²` **à ce seul n**. Elles ne
prouvent aucun exposant asymptotique, mais montrent qu'un gain de temps
par rejet aval peut coexister avec un travail préparatoire déjà massif.
Le reçu n'a ni coupes capteur appariées 8k/16k/32k, ni plusieurs
séquences, ni s10/s12, ni brut avec sol, ni GPU. Mesurer maintenant
temps **et** formes/frontières/cellules/covers/aval sur ces régimes ; une
simple baisse de `q34_ms` ne clôt pas le P0 sous-quadratique.

La sonde de tour locale avec certificat reste très loin du jalon
temporel sur 08/000000 : `chain_total=57,064 s` à K5 et `156,321 s`
à K10, W8. Elle est sur CPU local et non comparable directement à G4
W48, mais aucun contrat de 1 s ou 100 ms n'est acquis par ce reçu.
La prochaine campagne G4 doit d'abord figer le code/protocole, passer
le préflight natif, puis apparier on/off sur les **mêmes trames entières**
avec hashes, toutes les sorties et RSS ; ne pas extrapoler cette unique
séquence à tout SemanticKITTI.
