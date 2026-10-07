# MES-D6 — référence, unicité des prises et portée des chronos

Audit de `9b2747eff364d56215b589c782b1a4e51d59a576`, le 7 octobre 2026. Source du pilote :
`da2fa5c0b5d48ce2aff357a1174b47773076335b1c4e7ff2ff850afac9f6e118`.
Cadre : exploration v12 hors registre, CPU de référence, FULL pi0, u21 ; `not_claimed`.

**Le nouveau pilote compare bien les profils compilés séparément. CST-0207 reste ouvert** : les prises G4 et la
règle de non-infériorité de D6 restent à établir. Le README annonce explicitement une mesure sans adoption ; les
trois tours et leurs minimum/maximum ne sont donc pas présentés ici comme un juge statistique erroné.

## Contre-témoins du plan

`check.py` charge le corps publié par `git show` et appelle réellement `main`, `dilate`, `checks` et `ratios`.
**Seuls `build` et `take` sont remplacés** : pas de compilation ni de sonde native, sorties et durées inventées.
L'entrée de deux sites synthétiques est créée sous `/tmp`, puis détruite. Aucun octet LiDAR ni nouveau chrono HGP.

| Cas | Corps livré | Proposition |
| --- | --- | --- |
| Référence 21/x1 présente | code 0, six prises | inchangé |
| Maximum égal à 2^21, profils 21/24 | u21 filtré, u24 seul, code 0, contrôles conformes, rapport `null` | code 2 |
| Même maximum, seul profil 21 | aucune combinaison, `ZeroDivisionError` | code 2 |
| Profil 21 répété | code 0, trois identifiants de prises répétés | code 2 |
| Cas répété | code 0, six identifiants de prises répétés | code 2 |

Le témoin compte les collisions de noms de prises (donc six ou douze chemins JSONL, catalogue et G compris) ;
l'écrasement des fichiers est déduit de `run(..., raw_path)` qui ouvre en `wb`. Il ne simule pas un échec du produit.
Les doublons ne doivent pas passer pour des processus indépendants
dont la preuve brute est conservée. Si un régime sans référence u21 est souhaité, le déclarer séparément, sans
annoncer la comparaison D6 comme conforme.

`proposition.patch` ajoute les refus des doublons, jobs/délai non positifs, entrée illisible et référence u21
absente. Il ne corrige pas le lecteur JSON ni la provenance. Corps proposé :
`008e756bb5a3a400ad66ecb68b5588cbf6ccac9e9af320ab0d27607aac9c9c19`.
Normal et `-O` donnent exactement `normal.json` ; cinq cas après patch conformes. Aucun fichier produit modifié.
Les gardes jobs/délai et les exceptions d'entrée sont lues, sans témoin dédié dans ces cinq cas.

```sh
python3 -B -S check.py > /tmp/d6-plan-normal.json
python3 -B -S -O check.py > /tmp/d6-plan-optimized.json
cmp /tmp/d6-plan-normal.json /tmp/d6-plan-optimized.json
```

## Points à fermer avant mesure qualifiante

- Le reçu final ne contient ni configuration complète, ni commandes, ni hashes des sources, binaires et entrées.
  Les builds `p21/p24/p32` sont réutilisés ; une modification du checkout entre constructions n'est pas détectée.
  Épingler un checkout propre, relever les sources et binaires avant/après, conserver arguments et journal de
  construction. Une session gardée peut apporter ces éléments, mais le pilote autonome ne les prouve pas.
- `run` jette stderr ; `build` jette stdout et stderr. Conserver les deux flux, le code et le délai dans des
  fichiers distincts, particulièrement pour une construction impossible ou une prise expirée.
- La sonde catalogue chronomètre `build_catalogue`, hors préparation du nuage/pool, empreinte, export et
  destruction. La sonde G prépare aussi index et catalogue avant sa boucle, puis chronomètre `resolve_tower`
  seul. Ce sont deux processus et deux étages, pas une latence FULL ni un chrono GPU.
- Le catalogue émet une empreinte à chaque passe ; **G ne l'émet qu'à la dernière**, après `continue` sur toutes
  les précédentes (`bench/tower_probe.cpp`). La phrase « constante sur ses passes » n'est donc pas prouvée pour G.
  Corriger cette portée ou émettre le digest G après chaque passe, toujours hors chronomètre.

Pas de session G4 ni de nouveau test natif par cet audit. La correction du plan ne clôt ni la conformité du moteur
aux profils élargis, ni les défauts du juge, ni le seuil de 3 % de D6.
