# Comparaison ciblée 8k : filtres et census q3

21 septembre 2026. Huit mesures du flux global q3/q4, pas de catalogue,
de tour FULL, de GPU ni de contrat G4. Même scan SemanticKITTI 00/000000,
préfixe quantifié u16 de 8 000 sites, K5, s8, masque6, Local28, front samples,
payload digest. Les quatre mesures Scalar précèdent les quatre mesures Boxes.
Un essai par configuration, sans nouvelle compilation ni modification des
206 sources gelées, scripts ou binaires.

Captures closes `passed`, sans essai de benchmark supprimé ou relancé :

- [Scalar : lidar_tgo9mbax](lidar/lidar_tgo9mbax/MANIFEST.json).
- [Boxes : lidar_b7fo1a0i](lidar/lidar_b7fo1a0i/MANIFEST.json).
- [Huit lectures et analyse croisée](BENCH_8K_READBACKS.json) : normal/−O,
  historique/live, tous codes0 ; sorties identiques pour les quatre lectures
  d'une capture. L'analyse réutilise les validateurs, compare aussi les deux
  captures entre elles et vérifie leurs hashes avant/après lecture.

## Temps observés

Temps `pipeline_including_shared_preparation` : préparation cloud/index,
front, filtres, covers, q3/q4 et digest ; hors chargement initial du fichier,
sérialisation finale et libération. Les workers sont des fils logiciels,
pas une preuve d'utilisation d'autant de cœurs physiques.

| Témoins | Census q3 | W1 (s) | W4 (s) | Rapport W1/W4 |
|---|---|---:|---:|---:|
| Pair | Scalar | 21,861 | 7,035 | 3,107 |
| RectanglePair | Scalar | 20,273 | 10,169 | 1,994 |
| Pair | Boxes | 34,380 | 15,824 | 2,173 |
| RectanglePair | Boxes | 25,722 | 9,478 | 2,714 |

**Charge concurrente déclarée** : campagne du parent n8k/16k/32k,
K5/10, backends28/30 et qualifications sur le même hôte. Pas d'affinité
exclusive ni répétitions : ces rapports ne qualifient ni une accélération
stable ni un choix de défaut. Boxes/Scalar vaut ici ×1,573/×2,249 pour
Pair W1/W4, et ×1,269/×0,932 pour RectanglePair. Le résultat négatif ou
mixte est conservé ; moins de tests ne signifie pas automatiquement moins
de temps CPU.

## Travail discret, indépendant de ces fluctuations

Pair et RectanglePair aboutissent aux mêmes 177 415 covers, 16 106 462
sites couverts cumulés, 1 911 457 graines q3 et 2 312 013 graines q4.
Tous les compteurs q4 sont identiques entre les huit mesures. Le census
q3 et son tri de coquille sont les seuls travaux q3 changés par Boxes.

| Travail de sélection | Pair | RectanglePair |
|---|---:|---:|
| Masse entrante du front | 2 285 750 | 2 285 750 |
| Paires explicitement développées | 2 285 750 | 344 856 |
| Visites de témoins, rectangles + paires | 151 391 483 | 61 494 474 |
| Bornes Xi, rectangles + paires | 92 777 738 | 24 652 851 |

La préparation par rectangles est incluse : −84,9 % de paires développées,
−59,4 % de visites et −73,4 % de bornes Xi. Ce gain ne modifie pas le résidu
géométrique final ; il évite surtout de retrouver les mêmes rejets paire
par paire.

Pour les mêmes 1 911 457 boules q3 :

- Scalar paie 278 543 614 tests ponctuels de puissance.
- Boxes paie 72 936 399 bornes préparées, dont 47 765 979 nœuds visités
  et 25 170 420 bornes préparées puis non visitées après saturation.
  Les 5 445 262 tests de feuilles sont inclus dans ces préparations,
  pas un poste à ajouter une seconde fois.
- Les préparations par boule, notamment les trois axes de sommet entier,
  restent payées. Une borne de boîte et une puissance ponctuelle n'ont
  pas le même coût : le rapport des nombres ne constitue pas un speedup.

## Sorties et portée

Les huit sorties ont le même digest et les mêmes populations : 104 670
candidats, soit 93 914 q3 et 10 756 q4 ; 324 808 IDs de coquille émis.
Input FNV64 : `6370949069026829313` ; somme `dfe0baea59c20fd`,
xor `1731f87592ccef41`. C'est une comparaison de digests et de registres,
pas un nouvel oracle exhaustif à 8k ; les portes rationnelles séparées
qualifient les petits cas.

Une taille unique ne mesure pas la croissance 8k/16k/32k. Ces huit essais
ne prouvent donc aucun caractère sous-quadratique. Ils justifient de mesurer
séparément le gain de sélection, le coût des bornes et le nombre de graines
restantes avant toute décision de remplacer le défaut.
