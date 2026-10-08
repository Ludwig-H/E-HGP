# T2-d-B3 : contre-extraction statistique, 8 octobre 2026

**Les trois leviers échouent à la règle temporelle prédéclarée.** Le contrôle
A/A est dans sa fenêtre. Cette note ne réadmet pas l'identité ni les codes
processus : provenance, archives et preuves brutes d'identité sont traitées
séparément par evidence_1704. Aucun résumé d'identité n'est une preuve ici.

Source du protocole : pilote original `6a798b31…` au commit `545ed987e` ;
base avant R1 `8a0716e74`. Lecture Python de 300 JSONL publics, 2 400 lignes
FULL, 2 100 chaudes ; aucune exécution native/cloud ni lecture de payload.
Un hash de manifeste compact ferme noms, tailles et SHA des 300 journaux ;
pas de duplication des journaux ni d'un inventaire historique volumineux.

## Estimand et règle conservés

Cinq trames × six bras × dix processus, huit passes par processus. Exclure
la passe 0 ; prendre la médiane des sept suivantes. Pour chaque tour t,
calculer le rapport des **médianes des processus appariés** bras/avant.
GM = exp(moyenne des dix logarithmes). Bootstrap sur les dix tours, 10 000
tirages, graine 20261008, état aléatoire continu dans l'ordre des cinq trames
et sept comparaisons du pilote. IC95 aux indices 250 et 9749 triés.

La borne haute doit être **strictement <1 sur chaque trame** pour chaque
levier ; le veto A/A porte sur sa GM hors [0,985 ; 1,015], pas sur son IC.
Aucun nuage ni tour retiré, aucun changement de seuil, pas de moyenne qui
masquerait une trame défavorable. FULLN employait des médianes poolées :
cette autre convention ne doit pas remplacer celle du présent juge.

| Trame | Lot : GM [IC95] | Clés : GM [IC95] | Balayage : borne haute |
| --- | --- | --- | ---: |
| ng00 | 0,980181 [0,976761 ; 0,983450] | 0,983692 [0,980734 ; 0,987259] | 1,000937 |
| ng01 | 1,005941 [0,980747 ; 1,046561] | 0,979883 [0,976283 ; 0,983097] | 1,000225 |
| ng02 | 1,011498 [1,006428 ; 1,017223] | 1,009397 [1,006341 ; 1,012667] | 0,999376 |
| 02/001606 | 0,994709 [0,991132 ; 0,999181] | 0,996281 [0,994694 ; 0,997648] | 1,001474 |
| 08/001176 | 0,993509 [0,990408 ; 0,997447] | 0,988556 [0,986465 ; 0,990840] | 0,998513 |

A/A : GM 1,000277 / 0,998693 / 0,999946 / 0,999771 / 1,000781 ; plus grand
écart absolu à 1 = **0,130729 %**, dans ±1,5 %. Le lot échoue sur ng01/ng02,
les clés sur ng02, le balayage sur ng00/ng01/02-001606. Arrondir la borne
balayage ng01 à « 1,000 » masque l'échec strict.

Sur ng01, la médiane des dix médianes du lot vaut 65,045839 ms, contre
66,097462 ms avant, mais la GM appariée dépasse 1. Les tours t03 et t06 ont
les rapports 1,037456 et 1,184673 ; les huit autres sont sous 1. Ces valeurs
restent dans le calcul. Comparer seulement les deux médianes agrégées serait
changer l'estimand et donner ici la mauvaise conclusion.

## Décomposition utile au développeur

Les différences d'étages sont calculées **passe par passe**, puis sommées
sur 70 paires chaudes et divisées par 70. Ce sont des moyennes descriptives,
pas la statistique du juge. P+C+G+queue+reste donne exactement le mur avant
arrondi. Transferts ⊂ C, ouverture et tables ⊂ G : ne pas les ajouter deux fois.

Clés contre avant, ng02 : C **+0,585571**, G **−3,222596**, queue **+2,680904**,
FULL **+0,019098 ms** en moyennes. Le juge, sur médianes des processus, donne
cependant +0,940 % ; ces deux statistiques sont différentes et conservées.
Sur 08/001176 : C +0,831867, G −6,668556, queue +4,008854, FULL −1,874450 ms.
Un G plus court ne se transfère donc pas intégralement au mur FULL.

Le contraste informatif **clés après transfert** conserve le transport et
la mémoire des clés dans les deux bras. Il échoue aussi sur ng02 :
GM **1,009047**, IC95 **[1,006009 ; 1,011864]**. Moyennes appariées :
C +0,072591, G −2,446369, queue +3,139630, FULL +0,748328 ms.
Le coût des transferts seuls n'explique donc pas tout le problème observé.
Les compteurs ne prouvent pas la cause interne : avancer la fin de G peut
allonger mécaniquement la queue sans augmenter le travail de la forêt.

Première suite utile : examiner l'ordonnancement et les fins publiées sur
ng02 dans les deux bras de cette ablation, puis rejuger FULL avec les mêmes
portes. Aucun gain futur n'est déduit de G ou du seul coût d'un lookup.
Les autres campagnes informatives (grande trame, K10, G séquentiel/profil)
ne sont pas ajoutées à la cohorte de décision.

## Rejeu

`python -S [-O] check.py --raw <t2d_b3/journaux/k5> --repo <repo>`

Normal et optimisé : mêmes résultats ; aucun import du pilote. Sources Git
et journaux rehachés, cohortes/rangs/configurations vérifiés. `results.json`
garde toutes les médianes de processus, 35 GM/IC et les sommes de différences
appariées. Admission complète distincte ; cette note conclut seulement sur
la règle statistique calculable depuis ces journaux.
