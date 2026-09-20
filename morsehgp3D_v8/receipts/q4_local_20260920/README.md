# Fragments exacts et balayages q4 locaux — tranche28

20 septembre2026, après66b1551f/A5ff70645. `cpu_reference`, u16,
`public_status=not_claimed`. Une arête fournie, **q4 seulement** ; pas
de générateur global, de tour FULL ou de contrat G4. GCP non utilisé.
Contrat et preuve : [note produit](../../docs/Q4_FRAGMENTS_ET_BALAYAGES_LOCAUX_20260920.md).

## Ce qui est effectivement qualifié

Un atlas immuable partage entre les faces une partition exacte des témoins
en intérieurs uniformes, extérieurs stricts et blocs actifs. Sa construction
utilise le même index global ; les partitions terminales sont achevées
une fois avant partage. Le balayage local conserve la contribution constante
des événements hors cellule et émet une racine frontalière une seule fois.
Profondeur, support canonique et coquille sont comparés aux juges rationnels.
L'entrée précédente et ses valeurs par défaut restent inchangées.

Captures finales r2, toutes closes PASS :

| Capture | Commandes | Contenu |
| --- | ---: | --- |
| [smoke_dcu5kp0z](smoke_dcu5kp0z/COMPLETION.json) | 27 | 7 gates Release,20 mesures32 sites |
| [smoke_r4ju3jzb](smoke_r4ju3jzb/COMPLETION.json) | 27 | 7 gates Clang ASan/UBSan,20 mesures32 sites |
| [scale_ipjudaz2](scale_ipjudaz2/COMPLETION.json) | 39 | 7 gates Release,32 mesures dont24 à8k/16k/32k |
| [regression_gw_4e23g](regression_gw_4e23g/COMPLETION.json) | 1 | 89 CTests Release, aucun ignoré |

Soit72 mesures principales. La gate nouvelle passe11981 contrôles :
76 appels par arête,170 requêtes réutilisant un atlas,85 références,
488 complétions/1146 tests de sites par oracle,68 candidats q4,
coquille30, contacts et frontières non possédées, extrêmes u16,
cinq pannes d'allocation ciblées et quatre appels concurrents.
Ce n'est ni une injection exhaustive des pannes ni une qualification TSan
ou d'un ordonnanceur q4 ; les89 CTests n'ont pas tous été lancés sous sanitizer.

Trois [mutants compilés](mutants/compiled__sv8d6az/COMPLETION.json) sont
tués causalement : retirer les contacts min=0, oublier un intérieur clippé,
émettre une frontière fermée dans chaque cellule. Les vingt
[différentiels contre27](mutants/differential_lnkgkcjz/COMPLETION.json)
reproduisent tout le JSON hors temps de l'ancien chemin collectif.
Lectures normales/−O, contrôles de fermeture et37 corruptions du lecteur
principal complètent les preuves ; la [fermeture finale des dix lectures](readers_fbce52ch/COMPLETION.json)
conserve des résultats normal/−O identiques. Voir aussi les
[mutations géométriques](mutants/README.md) et le
[différentiel final](mutants/DEFAULT_DIFFERENTIAL_R2.md).

Sources177 gelées ; builds finaux épinglés :
`build/v8_q4_local_r2_20260920` et
`build/v8_q4_local_sanitize_r2_20260920`.
Chaque reçu garde commandes, environnement, sources, exécutables,
cache/archive, sorties et empreintes de fermeture.

## Mesures : moins de travail, mais pas de sous-quadratique général

Options fixes : domaine positif, profondeur7,4096 nœuds, budget Z512
intermédiaire, feuilles32, clipping actif. Un thread, affinité CPU0 pour
la campagne scale ; qualification concurrente sur l'hôte. Un essai par
configuration, **pas de gain de temps stable revendiqué**.
La construction terminale n'est pas limitée à512 tests et son travail
est inclus dans les compteurs et le temps du nouveau chemin.

Les deux séries denses contiennent n−2 seeds et n témoins couverts.
Leurs véritables balayages q4 sont exécutés, contrairement aux seuls
minorants de repli des tranches26/27. K5 et K10 donnent ici le même W
et le même nombre de comparaisons, mais pas les mêmes sorties.

| Régime | n | Lectures actives W | Comparaisons de tri | q4 K5 / K10 | Run K10, ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Préfixe dense | 8 000 | 5 310 748 | 10 271 563 | 6 /25 | 460,749 |
| Préfixe dense | 16 000 | 13 798 714 | 20 005 755 | 6 /25 | 1015,504 |
| Préfixe dense | 32 000 | 15 791 402 | 21 069 568 | 6 /25 | 1098,285 |
| Dense permuté | 8 000 | 912 689 | 893 368 | 6 /36 | 65,928 |
| Dense permuté | 16 000 | 3 579 207 | 4 077 292 | 6 /37 | 260,485 |
| Dense permuté | 32 000 | 14 973 733 | 19 803 576 | 8 /30 | 1148,874 |

Le run inclut atlas, génération des seeds, balayages et callback ; il
exclut nuage/index/cover et validation indépendante. Les totaux muraux
correspondants K10 sont480,086/1053,287/1177,130ms pour le préfixe,
109,979/341,196/1228,882ms pour la permutation. Ils ne sont **pas** le
temps d'une tour et ne se transposent pas à50k sur G4.

W préfixe croît×2,598 puis×1,144 ; **W permuté croît×3,922 puis×4,184**,
et ses tris×4,564 puis×4,857. Ce dernier régime conserve le carré dans
les feuilles. À32k/K10 il rejette981463 groupes pour profondeur et émet
seulement30 présentations de15 boules : ici le travail n'est pas imposé
par la taille de la sortie. Tous les autres ratios, même supérieurs à4,
sont exposés par le lecteur, sans sélection des seuls postes favorables.

La préparation est payée : à32k/K10 permuté,157069 tests de blocs,
161982 tests de sites et136866 copies d'IDs de frontière. Les compteurs
de population héritée ne sont pas des lectures de coordonnées. Pic
dynamique propre267672 octets, hors nuage/index/cover et objets fixes,
pas une mesure RSS. Le préfixe a un pic propre300440 octets.
Même son poste tests de blocs K10 fait16333/39111/160196, soit
×2,395/×4,096 : ses bons ratios W ne s'étendent pas à tous les postes.

Sur l'adversaire256, les lectures de la référence couverte sont65024 ;
elles deviennent16320 K5 ou27270 K10. Comparaisons de tri nouvelles :
8784/14119, sorties q4 identiques6/36. Mais32→64 augmente W de plus
de×4 et déclenche le raffinement : les petits ratios ne prouvent pas
une borne asymptotique. Les fonds lointains gardent deux seeds : W=12
(far), W=22 (cap), tests8k/16k/32k, pas un générateur global croissant.

La référence `cover` calcule aussi q3 avant filtrage de ses callbacks :
ses temps ne constituent pas une comparaison q4 pure. Aucun ancien repli
quadratique dense8k/16k/32k n'est exécuté. Le juge grand dense refait le
census rationnel global de chaque boule publiée, ce qui valide les sorties
mais **ne prouve pas à lui seul leur complétude** ; celle-ci est vérifiée
sur les petits oracles indépendants et argumentée dans la note.
La permutation SplitMix64 porte sur la grille entière avant préfixage ;
elle n'est pas celle du prototype A, dont aucun ratio n'est hérité.

## Historique conservé et commandes

Le premier budget Z seul laissait trop d'inconnus terminaux :72,663M
lectures à8k/K10. La finition unique les réduit à5,311M ; voir le
[préflight](PREFLIGHT.md), pas un benchmark apparié de temps.
Un mutant d'intérieur clippé a survécu au premier gel :
`mutants/compiled_0_udmiyg` reste en échec. Une fixture permanente a
renforcé gate/runner ; aucun code moteur ne change entre r1 et r2.
Les smokes r1, le différentiel intermédiaire et leurs lectures restent
historiques. Gate/runner exacts r1 archivés dans `historical_r1/` ; les
builds `v8_q4_local_20260920` et `v8_q4_local_sanitize_20260920` restent
eux aussi épinglés. Ne pas réécrire ces reçus avec les sources r2.

```sh
taskset -c 0 python3 morsehgp3D_v8/bench/run_q4_local_checks.py run --build build/v8_q4_local_r2_20260920 --output morsehgp3D_v8/receipts/q4_local_20260920 --campaign scale
python3 morsehgp3D_v8/bench/run_q4_local_checks.py read morsehgp3D_v8/receipts/q4_local_20260920/scale_ipjudaz2 --check-live
python3 -O morsehgp3D_v8/bench/run_q4_local_checks.py read morsehgp3D_v8/receipts/q4_local_20260920/scale_ipjudaz2 --check-live
```

Suite prioritaire : décisions partagées aussi entre faces et événements
de faible profondeur ; payer les listes de conflits, les doublons et les
coquilles. L'atlas immuable prépare ce travail distribué, sans qualifier
un backend parallèle/GPU. q3 global, WSPD multivoie, catalogue, intérieurs,
FULL et contrats G4 restent ouverts. s8/10/12 n'intervient pas dans cette
primitive sans front WSPD ; cette comparaison reste due au raccord global.
