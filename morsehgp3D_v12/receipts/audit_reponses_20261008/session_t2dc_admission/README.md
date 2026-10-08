# Session T2-d-C : admission des bruts et recalcul

Les bruts admettent **le lot C et ses quatre leviers**, avec A/A valide et mutant tué par deux empreintes erronées
sous code 0. Python normal et `-O` donnent la même sortie. Aucun moteur n'a été relancé. La
[provenance indépendante](../session_t2dc_provenance/README.md) ferme la session : avant `902041f66`, après
`02b735d6b`, ancien pool identique, arrêt certifié. Le pool `5b3362bbd` n'appartient pas à cette mesure.

Le lecteur relie **393 journaux de sondes** à leurs lignes archivées et à leurs commandes : chemins des bras,
fichiers d'entrée par nom, options, voie, profil u21, K, W48, passes, cohortes exactes et ordre tournant des bras.
Il admet 2 888 passes catalogue et 216 FULL, toutes complètes. La cohorte décisive compte 210 processus, 2 100
passes dont **1 890 chaudes** (passes 2..10) : 10 processus et 90 chaudes par trame/bras. Aucun processus étranger,
indice booléen, code booléen ou mutant hors commande observé. Les
[failles synthétiques du juge](../t2dc_integration/README.md) ne changent donc pas l'admission de cette capture.

Les portes archivées déclarent la vraie voie appareil sur neuf témoins (**217 contrôles**) et les refus sous
budget serré sur l'appareil (**14 appels, 15 contrôles**). Leurs textes ont été rapprochés des journaux ; les
observations de processus GPU avant/après sont vides et marquées réussies. Neuf cas d'identité catalogue CPU/GPU,
18 identités de bras et neuf cas FUL1 avant/après sont relus, avec comptes et grand livre cohérents. Les codes des
sondes sont les entiers enregistrés par `Session.run` dans le rapport ; les journaux ne contiennent pas un second
code de sortie externe indépendant. Aucun binaire n'est reconstruit ou exécuté par cet audit.

Le recalcul par produits des rapports et bootstrap d'indices retrouve le calcul du juge par logarithmes. Deux
bornes ng00 diffèrent d'un dernier bit flottant, **1,11e−16**, sans effet sur l'affichage ou les décisions. Pour
le lot, moyennes géométriques après/avant : **0,78583 / 0,79756 / 0,73688** ; bornes hautes des IC 95 % :
**0,79079 / 0,80178 / 0,73981**. Les bornes sont jugées avant arrondi. L'A/A observe
**0,99225 / 1,00370 / 1,00294**, dans la fenêtre fixée ±1,5 %. Cela ne constitue pas une garantie générale de
précision statistique à 1,5 %.

## Temps admis

Médianes des passes chaudes réunies, en ms ; ce tableau descriptif est distinct de la statistique de rapports
appariés par tour qui décide l'adoption. L'ordre des nombres est ng00 / ng01 / ng02.

| objet et régime | avant | après |
| --- | --- | --- |
| catalogue K5, 10 processus × 9 chaudes | 34,292 / 29,863 / 36,521 | **26,896 / 23,763 / 26,848** |
| catalogue K10, 3 × 4 chaudes, informatif | 135,867 / 111,984 / 145,316 | **102,740 / 84,036 / 98,207** |
| FULL K5, 3 × 9 chaudes, informatif | 160,162 / 128,006 / 164,872 | **153,861 / 122,664 / 155,638** |

Le FULL de ces trois trames demeure au-dessus de 100 ms. Il conserve des empreintes identiques à la session K ;
le pilote ne mesure pas un FULL à K10 dans cette session. Sur **37 trames, catalogue K5 seulement**, un processus
de six passes par bras et trame (cinq chaudes) : médiane des 37 médianes **51,483 → 40,750 ms**, maximum de ces
médianes **89,994 → 71,739 ms**. Noms et nombres de sites sont rapprochés des métadonnées déjà publiées ; aucun
XYZ, ID ou tar de données n'a été ouvert. Ce n'est ni une nouvelle mesure FULL sur 37 trames, ni leur maximum
instantané, ni une campagne répétée entre plusieurs processus par trame.

Les informations cache 4 Gio, trois processus × neuf chaudes, donnent après sans/avec cache :
**26,562 → 25,636 / 23,805 → 22,875 / 26,704 → 25,705 ms**. Aucun critère d'adoption supplémentaire n'en est déduit.

## Diagnostic utile au développeur

Les médianes de la partition « transferts » tombent de **9,059 / 7,897 / 10,291 ms** à
**3,264 / 2,942 / 3,472 ms** ; la publication passe de **4,036 / 3,194 / 3,986 ms** à
**1,662 / 1,372 / 1,630 ms**. Ce sont des partitions du mur nettes de consommation, **pas des mesures isolées du
DMA**. La nouvelle étape sorties vaut **2,602 / 2,223 / 2,710 ms** ; l'ancienne sonde ne la distingue pas.
Ces médianes ne s'additionnent pas pour expliquer une médiane totale.

Les feuilles restent proches de **10,990 / 8,900 / 10,197 ms** après C. Le gain de C seul ne suffit donc pas à
fermer FULL ; G/T/M/V/R restent à travailler. Les ablations soutiennent séparément double tampon, anticipation
et fenêtres sur leur périmètre déclaré. Le bras « flux » tranche sur ng00/ng01 sans chaîne réparée ; sur ng02,
flux et réparation sélective sont mêlés. Le bras à clés entières conserve la réparation sélective, donc ne
représente pas l'ancien repli complet. Source SHA matériel différente avant/après, empreintes hors mur ; aucun
effet temporel causal propre à SHA ni comparaison v11 n'est affirmé ici.

Les gardes FULL complètent le lecteur livré sur les types code/indice, sites/empreinte K, inclusion des temps,
pics successifs et plancher des entrées. Les sondes utilisent ici le **budget partagé** : `pic_octets` compte le
budget hôte/appareil, `pic_appareil_octets=0` est contractuel, et ce pic n'est pas une mesure de RSS.

## Rejeu sans moteur

```sh
python check.py DEPOT RETOURNE > /tmp/t2dc-normal.json
python -O check.py DEPOT RETOURNE > /tmp/t2dc-opt.json
cmp /tmp/t2dc-normal.json /tmp/t2dc-opt.json
cmp /tmp/t2dc-normal.json results.json
```

`RETOURNE` est l'extraction de métadonnées contenant `report.json`, `logs/`, `tableaux_t2dc.md`, fournie par le
reçu de provenance. Ses fichiers sont vérifiés par un inventaire SHA agrégé. Les scripts Git et le manifeste de
37 noms sont épinglés dans `capture.json`. Les bruts ne sont pas dupliqués dans ce reçu ; aucune identité de
compte ni donnée sous licence n'y est copiée.
