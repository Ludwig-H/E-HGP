# B3b : statistiques indépendantes sur A6c

10 octobre 2026, session `v12.20261010.t2db3b` fermée ; candidat
`81b0883d1`, avant `aa6338ee8`. `phase=exploration_v12_hors_registre`,
`objet=full_pi0`, `public_status=not_claimed`.

Les **clés seules et le lot B3 satisfont la règle statistique** sur les cinq
trames ; le balayage seul échoue sur ng02 et 08/001176. Les cinq rapports A/A
sont dans la fenêtre de ±1,5 % ; écart maximal 0,434495 %. La provenance et les
identités sont une admission séparée : voir la
[prélecture](../b3b_prelecture/README.md), notamment le juge v1 qui ne relit pas
les identités. Ce reçu ne transforme pas ses résumés en preuves natives.

Rejeu indépendant des **300 processus**, 2 400 passes dont **2 100 chaudes**.
Six bras, dix tours appariés par trame, huit passes par processus. Comme prévu
avant la mesure : médiane des sept passes chaudes par processus, puis rapport
apparié au bras avant ; moyenne géométrique (GM) et bootstrap de 10 000 tirages,
graine 20261008, flux aléatoire continu sur les cinq trames et sept comparaisons.
L'adoption exige une borne haute **strictement inférieure à 1 sur chaque trame**.
Aucune valeur aberrante retirée, aucun seuil changé.

Les temps ci-dessous sont les médianes des dix médianes de processus. La GM
porte sur les dix rapports appariés ; elle n'est pas le quotient de ces deux
médianes.

| Trame | Sites | Avant, ms | Clés seules, ms | GM clés [IC 95 %] |
| --- | ---: | ---: | ---: | --- |
| ng00 | 39 885 | 79,860479 | 78,627960 | 0,982330 [0,977564 ; 0,986416] |
| ng01 | 35 551 | 66,070014 | 64,904470 | 0,985611 [0,977683 ; 0,998664] |
| ng02 | 45 845 | 79,351291 | 78,122066 | 0,983849 [0,981389 ; 0,985876] |
| 02/001606 | 64 740 | 125,344436 | 122,920248 | 0,979765 [0,977912 ; 0,981819] |
| 08/001176 | 67 114 | 146,744800 | 142,569367 | 0,971484 [0,969015 ; 0,973975] |

Le lot donne respectivement 78,618145 / 65,021582 / 78,056891 / 122,939648 /
143,109132 ms. Ses cinq GM vont de 0,974888 à 0,987088 ; la plus haute borne
supérieure est 0,998378 sur ng01. Les clés seules gagnent 1,44–2,85 % en GM ;
le lot gagne 1,29–2,51 %. Cela ne qualifie ni le contrat sur 37 trames ni un gain
CPU, K10 ou multi-millions.

**Aucun bénéfice additionnel du balayage sur les clés n'est établi.** Les cinq
bornes hautes de `balayage_apres_cles` sont 1,005274 / 1,016379 / 1,002118 /
1,003015 / 1,007351. Le balayage seul échoue à 1,021769 sur ng02 et 1,002211 sur
08/001176. Pour simplifier le produit, les clés seules sont le choix soutenu par
cette comparaison ; le passage du lot n'est pas une preuve séparée du scan.

Les deltas de temps sont aussi conservés par paires dans `results.json`, avec
une décomposition additive exacte. Exemple 08/001176, clés moins avant, moyenne
sur 70 paires de passes : FULL −4,141822 ms, C +0,867328 ms, G −7,355539 ms,
queue +2,341718 ms. La queue est mesurée depuis la fin de G : sa hausse ne prouve
pas une hausse du travail forestier. Le bras `transfert` ralentit la GM sur les
cinq trames ; il conserve toutefois aussi la reconstruction hôte de la voie
découpée et n'isole pas universellement le PCIe. Voir la prélecture.

Le lecteur réemploie le [calcul statistique immuable du premier B3](../../audit_reponses_20261008/t2db3_stats/check.py),
avec son hash, un nouveau manifeste de 300 journaux, les sources de `81b0883d1`
et de nouveaux résultats. Il valide les cohortes, configurations, ordre des
passes, fermeture de chaque JSONL et égalité des partitions de temps. Aucun
moteur, compilateur ou accès GCP ; aucun contenu de nuage lu. La clôture de
session a été observée (`completed`, résultats vérifiés, arrêt ciblé certifié,
code d'arrêt zéro), avec la même archive que celle dont proviennent ces bruts.

```sh
python3 -B check.py --raw /chemin/journaux/k5 --repo /chemin/du/depot
```

Le rejeu normal et sous `-O` doit donner le même résultat. `capture.json` épingle
l'archive et les sources ; `results.json` contient les statistiques complètes.
