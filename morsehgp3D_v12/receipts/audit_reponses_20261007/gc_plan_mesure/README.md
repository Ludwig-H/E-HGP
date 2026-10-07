# T2-c : ordre des prises et gain net à mesurer

7 octobre 2026, prototype `repo2`, base publiée `0377684ec`. Exploration CPU u21, FULL pi0,
`public_status=not_claimed`. Épingles de lecture dans `capture.json`. Aucun chrono nouveau.

## Dix tours pour équilibrer les positions

Le plan prépare cinq bras et huit tours, dans l'ordre tournant du pilote, inversé un tour sur deux.
En huit tours, `apres` n'est jamais premier : ses positions 1, 2, 3, 4 apparaissent chacune deux fois.
`avant` occupe les positions 0, 1, 4 deux fois et 2, 3 une fois. Le bras A/A a les mêmes effectifs par position
que `avant` ; sa neutralité ne suffit donc pas à exclure un effet de position propre à `apres`.

Avec **dix tours**, le même algorithme place chaque bras deux fois à chacune des cinq positions. Proposition
simple avant campagne : demander dix tours et conserver les paires complètes. Cela ne prouve ni l'équilibre
des effets du prédécesseur, ni l'absence de dérive temporelle ; ne pas appeler ce cycle un carré de Williams.

`check.py` vérifie les permutations et un contre-exemple purement algébrique : cinq bras identiques, coût
sans unité 1,1 uniquement en première position, 1 ailleurs. En huit tours, le rapport géométrique A/A vaut 1
mais `apres/avant` vaut environ 0,97645. En dix tours, tous valent 1. Ce témoin démontre un biais possible de
l'estimateur ponctuel, **pas une adoption indue par l'intervalle bootstrap**, ni un biais mesuré sur G4.
Rejeu normal/−O identique ; aucune donnée de nuage.

## Mesurer aussi le coût déplacé vers le catalogue

Le nouveau `SupportEntry` contigu a 16 octets, contre quatre pour l'ancien indice de boule. Il économise des
indirections pendant G, mais ajoute **12 octets bruts par boule** à cette table : 15 680 352 octets pour
1 306 696 boules sur ng00 K5. Cache, alignements, copies temporaires et mémoire appareil restent à compter.
Le coût de construction et de remplissage de cette table appartient au catalogue ; il précède `resolve_tower`.

Le pilote mesure et juge explicitement G. Ses prises reconstruisent le catalogue avant les passes G, hors
de `wall_ns`. Un gain de G est donc un résultat valable sur cet étage, mais n'établit pas le gain net par
trame de cette modification du catalogue. Ajouter une mesure appariée **catalogue + G**, puis FULL intégré,
avec préparation et pic mémoire, avant d'en déduire une baisse de latence totale. Ne pas additionner des
médianes historiques de sessions différentes. La non-infériorité sur les autres étages de `MESURE.md` § 5
reste distincte du critère `REGLE_T2C` annoncé ici.

L'A/A est explicitement diagnostique dans cette règle : son absence de veto n'est pas qualifiée comme un
écart d'implémentation. Les trois cas décisifs appartiennent à la seule séquence 08 ; ce microbanc ne ferme
pas le contrat FULL multi-séquences ou multi-millions. Les minima locaux antérieurs ne sont pas les médianes
chaudes de ce futur pilote : voir [leur contre-lecture](../gc_rapports/README.md).

Reproduction : `python3 -B -S check.py`, puis `python3 -O -B -S check.py`, depuis ce dossier. Ce reçu propose
un ajustement de protocole ; aucune source du développeur n'est modifiée, aucune campagne n'est lancée.
