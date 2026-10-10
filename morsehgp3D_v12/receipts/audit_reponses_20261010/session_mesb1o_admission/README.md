# MES-B L1o : admission de la session close

10 octobre 2026. Session `v12.20261010.mesb1o`, source
`ae8f8107cc5a13356da89addf90808b5aaad9d67`. Audit des sources, métadonnées et
journaux uniquement : aucun moteur, compilation, accès cloud, coordonnées ou
identifiants de points lus ou exécutés. Cadre v12 hors registre, catalogue
CUDA G4 et tour CPU, FULL u21, `public_status=not_claimed`.

**Deux commandes closes avec code 0 ; 19 cas lancés, 23 FULL complètes,
dont 8 passes chaudes.** Quatorze processus réussissent et cinq refusent
avec code 2 déclaré et `resource_exhausted/memory_budget`. Aucun cas absent,
non joué ou coupé ; aucun journal manquant, flux tronqué ou stderr non vide.
Les 31 passes demandées ne sont donc pas présentées comme 31 succès.

La commande d'identité contient trois FULL : deux appareil sous budget 8 Gio,
puis une CPU. La commande L1 contient 17 cas, 20 FULL complètes, budget
appareil 88 Gio ; les deux commandes partagent 48 fils et budget hôte 160 Gio.
Le détail des 19 cas, codes, passes et empreintes des journaux figure dans
`results.json`. Les temps et identités sont étudiés séparément dans
[`mesb1o_temps`](../mesb1o_temps/README.md).

Le préflight déclarait une fenêtre utile estimée de 2 200 s et 3 800 s de
délais cumulés, avec avertissement de dépassement possible. Les métadonnées
retournées donnent 900 s pour la première commande, puis **2 241 s effectives
au lieu de 2 900 s demandées** pour la seconde. Elles finissent en 107,677 s
et 743,816 s ; groupes fermés et aucune troncature. L'avertissement préalable
n'a donc provoqué ici aucun cas manquant. Ces durées de commandes incluent
préparation et autres coûts ; ce ne sont pas les murs FULL.

L'archive de **44 101 octets**, SHA-256
`14b581cbcd7a52895ce65319f19e227bb5310bc54bffec069bd480f12a42ab5c`,
contient **82 fichiers manifestés**, tous vérifiés sans omission. Worker,
commandes et DONE valent 0 ; résultats vérifiés, erreurs et avertissements
de clôture vides. Le reçu certifie l'arrêt ciblé, code 0, de `RUNNING` à
`TERMINATED`. Cet audit relit cette certification archivée ; il n'ajoute pas
une observation cloud indépendante.

Les **375 fichiers sélectionnés** sont exactement ceux du Git source sur
src/bench/tests/cmake/CMake, pilote MES-B, lecteurs et worker. Les **138
fichiers natifs src/** sont aussi identiques au produit B3-K `2aaed1847`.
Release, u21 et CUDA ON sont retrouvés dans les deux rapports de compilation.
L'empreinte initiale de la sonde est identique aux deux lancements :
`d35a809d9e2fbb31bfb6ee66589dd554900128ce1319b0dd54e3de662374b9db`.
Pas de binaire ELF retourné, pas d'empreinte ELF finale archivée et aucune
nouvelle porte CTest ou campagne de mutants dans ce plan. GPU déclaré
RTX PRO 6000 Blackwell Server Edition, 48 fils hôte ; GPU sans processus aux
extrémités déclarées, sans preuve de solitude pendant tout l'intervalle.

Les 38 fichiers natifs JSONL/stderr publiés par
`fbd5923a8a892e1dbab8117f0e62d04f1b432d2a` sont identiques à l'archive ; les
valeurs numériques des deux rapports sont identiques. Le lecteur FULL du
paquet rejoue chaque rapport depuis les bruts, avec les options du plan et
les effectifs du manifeste de données. Les codes de chaque processus natif
sont **déclarés par le pilote épinglé**, sans capture indépendante de retour
supplémentaire. Les deux codes de commandes sont raccordés à `commands.tsv`
et `meta.txt`. Aucun payload de nuage n'est relu : seuls ses noms, tailles,
empreintes et effectifs déclarés sont utilisés.

## Les quatre refus avant toute FULL ne localisent pas la panne

Le [README développeur au commit fbd5923a8](../../g4_mesb1o_20261010/README.md)
attribue les grands refus à la mémoire de l'hôte pendant la tour. Le présent
reçu épingle cette version du README dans `capture.json`. **L'archive ne
prouve ni le budget hôte fautif, ni le stade tour**, pour les quatre cas :

| Cas K5 | Sites | FULL complètes | Maximum nvidia-smi échantillonné |
| --- | ---: | ---: | ---: |
| NIBIO 12 sans sol | 7 793 680 | 0 | 47 407 Mio |
| NIBIO 12 entière | 7 825 857 | 0 | 47 409 Mio |
| Boreas 50 trames sans sol | 7 857 268 | 0 | 47 135 Mio |
| Boreas 50 trames entières | 10 766 998 | 0 | 47 331 Mio |

Chacun publie exactement `open ok` avec budget appareil séparé, puis
`exit resource_exhausted/memory_budget`, et un stderr vide. Aucun champ
ne nomme le budget fautif, la demande refusée, l'usage à cet instant ou le
stade atteint. `open` confirme l'ouverture CUDA, pas la fin du Catalogue.
Une réserve peut échouer avant une allocation physique ; un maximum
nvidia-smi inférieur à 88 Gio ne suffit pas à exclure le budget appareil.
« Mémoire hôte pendant la tour » reste donc une **inférence à diagnostiquer**,
pas une localisation mesurée. Aucun nouveau défaut moteur n'en est déduit.

TU Wien sans sol reste distinct : **5 199 758 sites, première FULL réussie,
seconde passe refusée**, cas suivi par `CST-0243`. Le préfixe complet de ce
cas ne transforme pas les quatre refus sans FULL ci-dessus en échecs au
même stade. Aucune instrumentation diagnostique n'a été appliquée à la
sonde empaquetée.

## Rejeu

```
python3 -B -S check.py --repo DEPOT --session SESSION --snapshot CAPTURE_EXTERNE
python3 -B -S -O check.py --repo DEPOT --session SESSION --snapshot CAPTURE_EXTERNE
```

Relectures normale et optimisée identiques. Le snapshot externe ne conserve
que manifeste de métadonnées, rapports et bruts textuels ; aucun payload.
`capture.json` épingle les entrées, `results.json` les conclusions admises,
`SHA256SUMS` ferme ce reçu. Les propositions de diagnostic et les observations
cloud du coordinateur sont des preuves distinctes.
