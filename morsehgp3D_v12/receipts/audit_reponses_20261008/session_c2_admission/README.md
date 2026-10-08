# MES-C2 : admission et mesures des petits nuages

**Verdict admis : non tenu ; C1, C2 et C3 restent non tenus.** Le lecteur indépendant **55ca24fe** est utilisé
sans modification. Seules ses entrées changent conformément au plan C2 figé : source `27eca166b`, fils **4 et 48**,
mêmes 159 métadonnées et même archive `g4_small`. La correction C3 du pilote est comprise dans ce nouveau pin.
Le régime W1 n'est pas prévu dans cette session ; il n'est ni admis ni transformé en succès.

La [provenance et l'arrêt](../session_c2_provenance/README.md) sont contre-vérifiés séparément. Les 56 processus
prévus sont joués : **48 réussites, 8 refus**, aucune expiration, aucune prise manquante. Le lecteur admet
**3 608 passes complètes et 2 392 chaudes**, sans écart au rapport ni contrôle manquant. Normal et `-O` rendent
les mêmes résultats. Aucun moteur, compilation ou GCP lancé par cet audit ; aucun payload de données ouvert.

Huit Sessions régulières comportent chacune 147 nuages × trois tours : **441 passes, dont 294 chaudes**. Le
premier passage de chaque nuage est écarté ; les deux suivants font foi. Les 147 nuages se répartissent en
132 réels et 15 synthétiques sains. Les 48 processus difficiles couvrent séparément douze cas × deux voies ×
K5/K10, chacun demandant deux passes à W48. Les huit refus sont les quasi-sphères de 3 000 et 10 000 sites,
sur les deux voies et aux deux K, tous `unsupported_degeneracy/wide_leaf`. Aucune passe refusée n'est un temps nul.

## FULL, temps chauds

Pour chaque nuage : médiane de ses **deux passes chaudes**. Les cellules montrent la médiane puis le maximum de
ces médianes sur **147 nuages**, en ms. Le maximum n'est pas celui des passes individuelles.

| K | fils | CPU : médiane / maximum | appareil : médiane / maximum |
| --- | ---: | ---: | ---: |
| 5 | 4 | 45,923 / 1 033,733 | 12,728 / 300,127 |
| 5 | 48 | **28,949 / 202,945** | **10,150 / 77,582** |
| 10 | 4 | 152,565 / 6 058,169 | 48,829 / 3 001,044 |
| 10 | 48 | **53,008 / 949,535** | **24,580 / 486,095** |

Sur les **132 réels seuls**, K5/W48 donne CPU **28,021 / 153,770 ms**, appareil **9,683 / 53,249 ms** ; K10/W48
donne CPU **52,936 / 616,290 ms**, appareil **22,293 / 272,733 ms**. Les autres groupes/configurations sont dans
`results.json`, sans mélange de régression entre familles.

Les critères C1/C2 portent sur la droite des moindres carrés des **réels CPU/K5/W48** : ordonnée à l'origine
**16,450755 ms** (> 2 ms), pente **11,571603 µs/site** (> 3,727217 µs/site). Ce sont des paramètres descriptifs de
régression, pas une décomposition physique fixe/variable ni un plafond par nuage. C3 est complet mais non tenu
à cause des quatre refus K5. Le verdict global passe de **« refusé » pour MES-C à « non tenu » pour MES-C2**
parce que l'expiration et le contrôle manquant disparaissent ; aucun des trois critères ne devient tenu.

Le réseau entier à 10 000 sites, hors cohorte des 147 : K5 **290,377 ms CPU / 232,379 ms appareil** ; K10
**12,746 s / 12,185 s**. Une seule passe chaude par processus difficile : pas de distribution inter-processus.

## Ressources

Les deux budgets sont **séparés**, 64 Gio chacun ; la mémoire épinglée est imputée au budget hôte. Maximum du
pic compté sur les passes chaudes des Sessions régulières :

| voie / K | budget hôte : octets | budget appareil : octets | épinglée : octets |
| --- | ---: | ---: | ---: |
| CPU / 5 | 326 253 916 | 0 | 0 |
| appareil / 5 | 236 832 648 | 373 736 380 | 16 777 216 |
| CPU / 10 | 1 801 373 220 | 0 | 0 |
| appareil / 10 | 1 299 677 074 | 1 974 757 012 | 16 777 216 |

Ces maxima sont identiques à W4/W48 pour chaque ligne. Ils ne sont pas des RSS ni des coûts de nuages isolés :
les entrées des 147 nuages restent résidentes et les capacités peuvent être réutilisées entre trames. Le RSS
publié est un maximum cumulatif du processus ; maximum observé ici **2 055 217 152 octets** (CPU K10/W48).

CPU·s cumulées dans les fenêtres de mesure des **294 passes chaudes** de chaque Session régulière :

| K | CPU W4 → W48 | appareil W4 → W48 |
| --- | ---: | ---: |
| 5 | 139,164 → 234,114 CPU·s | 28,145 → 36,193 CPU·s |
| 10 | 617,934 → 986,822 CPU·s | 216,640 → 281,215 CPU·s |

Ce n'est pas le CPU de tout le processus, qui comprend aussi ouverture, empreintes et travail hors mur.
Les latences diminuent à W48 tandis que le travail CPU mesuré augmente sur cette cohorte ; aucune extrapolation
à un autre nombre de fils n'est faite.

## Comparaison descriptive avec MES-C

Archive de données identique, noms/sites/groupes et ordre des nuages par Session identiques. Six configurations
complètes sont communes ; **882 comparaisons nuage/configuration** retrouvent les mêmes empreintes FUL1, avec K
conservé. Les GPU/K10 de MES-C n'étaient pas complets : aucune division par une mesure absente n'est effectuée.
L'ordre global des configurations diffère puisque W1 a été supprimé du plan MES-C2.

Sur les vingt réels d'au plus 150 sites, médiane des valeurs chaudes MES-C → MES-C2 : CPU/W48 **15,603 →
11,135 ms**, appareil/W48 **8,490 → 4,535 ms** ; CPU/W4 **6,819 → 6,591 ms**, appareil/W4 **4,870 → 4,533 ms**.
La valeur W48 appareil n'est pas inférieure à W4 sur ce sous-groupe, à la précision de cette prise.

Les sources ont changé avec **le pool à équipe et le lot catalogue C** ; les sessions ne sont pas une expérience
appariée. Aucun effet isolé du pool, gain causal d'un levier ou comparaison v11 n'est déduit de ces ratios. Une
Session par configuration, deux observations chaudes par nuage, ne fournit pas un intervalle entre répétitions
de processus. Les ratios par nom et groupes de taille sont publiés comme descriptifs dans `results.json`.

## Rejeu

```sh
python check.py DEPOT RETOUR_MESC2 RAPPORT_MESC > /tmp/mesc2-normal.json
python -O check.py DEPOT RETOUR_MESC2 RAPPORT_MESC > /tmp/mesc2-opt.json
cmp /tmp/mesc2-normal.json /tmp/mesc2-opt.json
cmp /tmp/mesc2-normal.json results.json
```

`RETOUR_MESC2` contient `rapport_c.json`, `brut/`, construction et tableaux, issus de l'extraction metadata du
reçu de provenance. `RAPPORT_MESC` est le rapport historique déjà admis. Inventaires/hashes, lecteur figé,
protocole antérieur et plan C2 sont vérifiés avant lecture. Les sources de chaque run sont chargées au pin Git.
Les codes des sondes sont **inférés conditionnellement au pilote épinglé**, pas inventés comme codes externes.
Bruts non dupliqués ici ; aucune identité de compte ni donnée sous licence dans le reçu.
