# Mesures CPU G4 publiées le 4 octobre — contrelecture des liaisons

Publication `61da03749344a6acc4fea2b9eee875606cc857e8` ; exécutions
distinctes : `claudeab8` au pin `54c167bb6466e790f5259b44870725f3e02ed43f`,
`claudediag1` au pin `e49ea46907e3c4bb9e0942e2205cf490748136e4`.
Les corrections ultérieures des lecteurs, de l'ordre A/B et des timings
publiées en `d5b1d0179`/`61da03749` ne sont pas attribuées à ces sessions.
Cadre : CPU de référence, Release u21, mode 16379, `not_claimed` ; aucune
mesure GPU. Les coordonnées et IDs des trois trames entières sans sol sont
identiques entre les deux sessions, par tailles et SHA.

Qualification q3/R : **acquise pour cette source CPU et ces portes**.
`claudeab8` est `completed` : 673/673 portes passent, sans skip ; trois
mutants num et quatre catalogue sont tués par code de juge. Le code natif
`src/` et CMake du pin 54c est identique à `9b9244a00`. Aucun ASan/UBSan
ni TSan supplémentaire n'est joué dans cette session. `claudediag1` reste
`failed_remote` : 676/678 portes passent, seules `mhgp11_style` et
`mhgp11_style_opt` échouent (fonction de 110 lignes) ; TSan ciblé 8/8 et sept
mutants catalogue passent. Ce refus global est conservé.

Les deux archives de résultats et paquets sources existants ont été
rehachés. Les manifestes des résultats couvrent exactement 285 et 235
payloads, tous vérifiés ; les rapports publiés égalent leurs originaux
archivés. Quatre blobs primaires de chaque paquet correspondent au Git
exécuté. Les anciens bras transportés sont liés par leurs SHA d'archives
et binaires enregistrés ; leurs archives ne sont pas recopiées ici. Les
arrêts ciblés sont certifiés, sur la cible attendue et les mêmes générations
de début/fin de chaque session. `ARCHIVE_BINDINGS.json` contient cette
contrelecture ; aucune opération cloud n'a été faite.

| Mesure diagnostic, ELF `967b2a54d52c…` | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| K5, feuille16, W48 libre, médiane native (5 prises), ms | 408,36 | 296,06 | 359,70 |
| K5, feuille16, W24 épinglé 0–23 (5 prises), ms | 574,91 | 442,59 | 527,67 |
| K10, feuille16, W48 (3 prises), ms | 3284,69 | 2506,39 | 2738,83 |
| K10, feuille24, W48 (3 prises), ms | 2506,13 | 1822,46 | 2064,50 |

Ajouter une ligne diagnostic datée au tableau ancien 412/352/381 ms, plutôt
que remplacer sa provenance ou calculer un gain entre captures non
appariées. À W1, les paires source figées montrent un gain q3 de 1,0–1,3 %
du mur ; R et les compteurs R1 sont proches du bras A/A. Une seule paire
par trame reste descriptive. À W48, cinq paires donnent une p bilatérale
minimale de 0,0625 ; aucun gain statistique n'est acquis. Les médianes des
phases ne doivent pas être additionnées pour fabriquer une médiane FULL.

Deux précisions utiles au README producteur :

- L'identité canonique concerne **126 prises A/B** (54 + 72). Les **48
  prises supplémentaires** K10/W24/W48 attestent `ok`/code0 et leurs temps,
  mais ne portent aucun SHA de dump : le lecteur ne prouve pas leur égalité
  de sortie. Limiter la phrase « toutes les prises, dumps identiques » à
  l'inventaire A/B.
- W24 épinglé contre W48 libre change simultanément le nombre de workers
  et l'affinité. Le rapport observé ne démontre pas isolément l'effet de
  l'hyperthreading ni que le moteur attend la mémoire. Conserver le résultat
  descriptif et séparer ces facteurs dans une éventuelle ablation future.

Les temps natifs excluent lecture/dump selon le banc ; `seconds` est le
temps du processus et ne doit pas s'y substituer. Aucun contrat 100 ms,
plusieurs séquences, u18/u24, GPU ou sortie plate n'est acquis ici.

`review_measures.py` relit les douze objets Git publiés par SHA, rejoue le
`check.py` exact en Python standard dans un temporaire, vérifie inventaires
et métadonnées, puis dérive les chiffres. Normal et optimisé rendent la
même sortie ; le lecteur producteur passe ses 176 contrôles. Les grands
rapports JSON restent dans Git, les archives dans leurs captures d'origine.
La capsule est légère et exige seulement un dépôt contenant le commit
épinglé pour son rejeu ; elle ne contient aucune coordonnée KITTI.

```sh
python3 -B -S review_measures.py --repo /chemin/vers/E-HGP
python3 -B -O -S review_measures.py --repo /chemin/vers/E-HGP
sha256sum -c SHA256SUMS
```

Le ledger inventorie tous les payloads hors lui-même et `SHA256SUMS` racine.
`SHA256SUMS` inclut le ledger et exclut uniquement son propre chemin racine.
