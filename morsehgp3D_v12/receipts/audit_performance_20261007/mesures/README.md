# Relecture indépendante des temps F2 / E — 7 octobre 2026

**Conclusion pour le constructeur : porter seulement le parcours et les feuilles sur GPU ne suffit pas si l'aval CPU
de F2 reste inchangé.** Sur ng00 / ng01 / ng02 à K5, feuille 24, assemblage + table coûtent déjà **154,0 / 122,5 /
171,3 ms**. Les accélérations de la fin du catalogue et les sondes de la résolution sont prioritaires ; le saut G-L3
doit rester rejeté dans sa forme mesurée. Aucun temps FULL v12 ni contrat de 100 ms n'est acquis.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference ; cuda_g4 pour le catalogue contractuel à mesurer
objet=full_pi0
quantification=quantized_u21_input_only
public_status=not_claimed
```

Base lue : `58d384721678d11ef8ccd86c76cca182f41a716c`. Aucun produit modifié, aucun nouveau banc, aucune commande GCP.
Ce reçu est une **relecture arithmétique et une inspection des frontières de chrono** ; il ne rejoue ni les moteurs,
ni les oracles géométriques, ni le protocole complet d'admission des sessions.

## Reproduction et portée des preuves

Depuis la racine du dépôt :

```sh
python3 -S morsehgp3D_v12/receipts/audit_performance_20261007/mesures/relecture.py --check
python3 -S -O morsehgp3D_v12/receipts/audit_performance_20261007/mesures/relecture.py --check
```

Sur un checkout plus récent dont les documents ont changé, relire les mêmes octets historiques sans créer de worktree :

```sh
python3 -S morsehgp3D_v12/receipts/audit_performance_20261007/mesures/relecture.py --git-pin --check
python3 -S -O morsehgp3D_v12/receipts/audit_performance_20261007/mesures/relecture.py --git-pin 58d384721 --check
```

`--git-pin` sans valeur prend `pins.base_commit` ; avec valeur, cette révision est résolue en commit. Chaque entrée est
alors lue par `git show COMMIT:chemin` et doit toujours correspondre à son SHA-256 enregistré. L'option ne modifie ni
le checkout ni les empreintes et ne transfère aucune qualification au code actuel.

[relecture.py](relecture.py) relit les JSON bruts, vérifie les 48 SHA-256 de [pins.json](pins.json), refuse les clés JSON
dupliquées et constantes non finies, contrôle passes / régimes / travail / empreintes stables, puis reproduit exactement
[resultats.json](resultats.json). Sept sources qui définissent les chronos sont aussi rapprochées des manifestes des
instantanés réellement joués en F2 et E. La vérification ne dépend pas de `assert`. Les résultats sérialisés sont
identiques en Python normal et `-O`.

- **F2** : 12 cas, 120 passes brutes ; une invocation par cas, dix passes, neuf chaudes.
- **G-L3** : 15 journaux (cinq processus par trame), ratios puis bootstrap à 10 000 tirages reproduits depuis les
  temps imprimés ; conformité de forêt déclarée dans ces journaux, sans recalcul géométrique indépendant.
- **M7** : quatre profils, parts pondérées par les cycles, pas moyenne des pourcentages par ordre.
- **MES-E** : 11 prises dans le rapport machine du pilote ; pas de nouvelle exécution ni certification des découpes.

Les fichiers historiques ne sont pas recopiés. Le lecteur refuse leur modification ultérieure ; il faut alors relire
l'historique épinglé, et non actualiser silencieusement leurs empreintes.

## F2 : catalogue CPU, budget et parties séquentielles

Sources : [session F2](../../g4_t1f_20261007/README.md), ses douze `stdout` épinglés, et
[sonde](../../../bench/catalogue_probe.cpp). G4, 48 fils, sites ng00 / ng01 / ng02 = 39 885 / 35 551 / 45 845,
trois trames de **la même séquence 08**, u21. Toutes les valeurs ci-dessous sont en millisecondes. Les médianes portent
sur les passes 2 à 10 d'un seul processus ; leur maximum n'est pas un maximum sur plusieurs processus ou séquences.

| Cas | mur médian | mur max | assemblage + table | reste si parcours + feuilles gratuits | non ventilé |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 K5 feuille 16 | 464,8 | 466,0 | 155,0 | 184,2 | 15,8 |
| ng00 K5 feuille 24 | 451,6 | 455,0 | 154,0 | 180,6 | 13,1 |
| ng00 K10 feuille 24 | 1 750,2 | 1 762,7 | 727,1 | 848,4 | 62,1 |
| ng01 K5 feuille 16 | 373,5 | 374,9 | 122,5 | 147,2 | 13,6 |
| ng01 K5 feuille 24 | 372,7 | 373,2 | 122,5 | 145,1 | 11,0 |
| ng01 K10 feuille 24 | 1 379,8 | 1 387,1 | 560,6 | 647,4 | 40,6 |
| ng02 K5 feuille 16 | 467,7 | 472,6 | 172,7 | 206,0 | 16,7 |
| ng02 K5 feuille 24 | 469,5 | 472,1 | 171,3 | 201,7 | 13,6 |
| ng02 K10 feuille 24 | 1 699,4 | 1 704,9 | 722,7 | 844,7 | 57,9 |
| uniforme 8 000 K5 | 157,9 | 158,3 | 61,0 | 72,5 | 5,3 |
| uniforme 16 000 K5 | 326,7 | 328,8 | 135,4 | 159,5 | 12,4 |
| uniforme 32 000 K5 | 671,2 | 674,1 | 290,4 | 340,4 | 26,4 |

Chaque somme ou soustraction est calculée **dans chaque passe, puis médianée**. Ce n'est pas la somme de médianes de
prises distinctes. Le reste conditionnel vaut `wall - traversal - count - fill` ; le non ventilé soustrait en plus
`levels`, `sort`, `assemble`, `table`. Dans le code, `traversal` retranche explicitement les deux sous-chronos de
feuilles ; les sept chronos sont disjoints. Le reste est donc un diagnostic valide de cette exécution CPU. **Ce n'est
pas une mesure GPU ni une borne absolue sur une autre architecture** : le coût de l'aval peut changer avec disposition,
résidence et parallélisme. C'est l'obstacle conditionnel précis à une migration limitée au front et aux feuilles.

Le chrono `wall_ns` englobe seulement `build_catalogue`, jusqu'au catalogue matérialisé avec niveaux exacts, populations
et table de supports. Il exclut lecture, `prepare_cloud`, création du pool, SHA-256, export et destruction du catalogue
rendu. La première passe dite « froid » reste donc **un premier appel catalogue**, pas le démarrage froid du produit.
La sonde ne construit aucune forêt, parent ou verticale. Zéro des 108 passes chaudes du catalogue est sous 100 ms.
Le GPU contractuel et le FULL demeurent non mesurés.

Inspection de [assemble.cpp](../../../src/catalogue/assemble.cpp) et [table.cpp](../../../src/catalogue/table.cpp) :

- Le calcul des niveaux et la copie finale sont déjà parallèles ; tout `assemble_ns` n'est pas séquentiel.
- Le calcul des rangs denses parcourt les boules en série ; un autre parcours séquentiel écrit les niveaux distincts
  et préfixes des incidences. `build_table` ne reçoit aucun pool : comptage, préfixes, placement et tris de lignes sont
  séquentiels.
- La mise à plat des enregistrements précède les sous-chronos ; allocations et libérations ne sont pas toutes
  ventilées. Le résidu à K5 vaut encore 11–17 ms. Son attribution détaillée demande une instrumentation distincte :
  le lecteur ne le rebaptise pas « allocations » sans mesure.

La légère différence README / logs, jusqu'à 1 ms suivant l'arrondi, ne change aucun diagnostic. Les chiffres bruts
priment, notamment ng02 K5 feuille 24 : **469,481 ms**.

## Comparaison v11 : descriptive, avec frontières explicites

La référence historique CPU v11 publiée dans [MESURE.md](../../../docs/MESURE.md) donne `domain` = 200 / 163 / 195 ms
à K5, feuille 16. Rapport aux médianes F2, **à feuille 16 identique** : 2,324 / 2,291 / 2,399. À feuille 24 en v12 :
2,258 / 2,287 / 2,408. « Environ 2,3 fois » décrit donc correctement l'ordre de grandeur ; ce n'est ni un effet causal
apparié, ni un intervalle statistique, ni une régression FULL mesurée dans F2.

La frontière v11 `domain` dans [full_probe.cpp](../../../../morsehgp3D_v11/bench/full_probe.cpp) commence **après l'index**
et englobe `prepare_full_domain`, donc catalogue **et table support → boule**. Cette table est déjà construite en
parallèle dans [full_domain.cpp](../../../../morsehgp3D_v11/src/tower/full_domain.cpp). Les deux colonnes comparées sont
ainsi proches fonctionnellement et excluent les forêts, mais leurs architectures et préparations diffèrent.
Les numérateurs et dénominateurs viennent de sessions et prises distinctes ; la campagne F2 n'exécute pas un témoin
v11 entre chaque bras. Comparer F2 aux **314 / 255 / 313 ms FULL v11** mélangerait en plus deux objets chronométrés.

## E : pourquoi le saut reste rejeté, où agir ensuite

Source : [session E](../../g4_t2e_20261007/README.md). Les sorties G-L3 impriment, par ordre, le minimum de trois passes
entrelacées ; le lecteur additionne les temps des ordres 2..5 par processus, forme le ratio saut / base, puis rejoue
le bootstrap des **cinq processus**, graine 20261007. Les minima de passes brutes ne sont pas publiés séparément :
leur calcul en amont n'est pas reconstitué ici.

| Trame | ratio géométrique | IC 95 % | censuses saturés évités | verdict performance |
| --- | ---: | --- | ---: | --- |
| ng00 K5 | 1,032290 | [1,030102 ; 1,035545] | 81,124 % | rejet reproduit |
| ng01 K5 | 1,040797 | [1,038878 ; 1,042590] | 81,531 % | rejet reproduit |
| ng02 K5 | 1,034902 | [1,033061 ; 1,036492] | 82,499 % | rejet reproduit |

La forêt identique déclarée dans les quinze journaux ne transforme pas une économie de compteurs en gain de temps.
Sur ng00 ordre 5 : 219 779 tentatives, 125 275 sauts certifiés, 3 267 267 tests exacts ; les tentatives infructueuses
doivent encore faire le census. Conserver la voie sans saut ; toute politique plus sélective serait un **nouveau bras**
à déclarer et mesurer, sans réinterprétation de ce rejet.

M7 classe les premiers postes à K5 : sondes **38,62 / 39,91 / 42,93 %**, proposition + T1 **26,02 / 25,52 / 25,87 %**,
censuses saturés + complets **18,62 / 17,06 / 13,83 %**. À K10 ng00, sondes et proposition totalisent **66,53 %**.
Ces parts sont celles d'une résolution à **un fil instrumentée**. L'instrumentation donne 1,752 / 1,316 / 1,567 s à K5,
soit **20,4 / 20,8 / 22,4 %** de plus que la réplique non instrumentée imprimée dans les mêmes journaux ; K10 : +16,3 %.
Ne pas présenter ces secondes comme une régression sur la campagne D, ni comme une prédiction à 48 fils.

MES-E est un autre périmètre : **v11 gelée CPU 48 fils, un processus neuf par cas**, mur de `Popen` à `wait4`, incluant
lecture et sérialisation FULL vers `/dev/null`. Les 11 cas rendent zéro ; ce ne sont pas des latences chaudes FULL en
mémoire de la v12. À K5, IGN 1→8 M : 16,042→155,250 s, 11,73→75,00 Gio ; ETH3D : 16,073→203,038 s,
9,36→53,87 Gio. Exposants descriptifs des extrémités : **1,092 / 1,220**, sans intervalle ni borne générale ; forêt
ETH3D ×**28,217** pour environ ×8 sites. À K10, seul 1 M est joué (84,101 s / 40,42 Gio IGN ; 48,562 s / 26,33 Gio
ETH3D). La « rupture vers 4 M » est une extrapolation mémoire, **pas un refus effectivement mesuré**.

## Campagne d'ablation proposée, à déclarer avant exécution

| Priorité | Bras et mesure | Invariants à conserver |
| --- | --- | --- |
| 1 | A = catalogue CPU F2 ; B = assemblage parallèle seul ; C = table parallèle seule ; D = B+C. Chrono catalogue complet, détails + résidu publiés. | Mêmes boules, populations, rangs et digest ; rangs denses par comparaison exacte des voisins, y compris aux frontières de blocs ; préfixes d'incidences u64 ; table canonique identique. |
| 2 | Catalogue appareil complet : parcours, comptage **et écriture**, niveaux, tri, assemblage, table et matérialisation requise. Comparer au bras produit retenu dans le même protocole. | Pas de transfert ni de synchronisation cachés ; résidence hôte/appareil déclarée ; replis exacts et cas de feuilles larges facturés. |
| 3 | Résolution : G-L5 sondes groupées / jointure triée, puis amélioration des propositions, chacun seul et combinaison ; comparer au bras sans saut G-L3. | Chaque requête, chaîne et certificat garde son sens ; résolution exhaustive, traces canoniques et forêt identique, niveaux exacts. Mesurer préparation, tri des requêtes et dispersion des réponses. |
| 4 | FULL résident dès disponibilité, puis 1/2/4/8 M avec lots bornés et suivi du coût forêt. | Ordres 1..K et verticales réellement produits ; sortie totale et mémoire bornée, aucun préfixe présenté comme scène entière. |

Pour les premières ablations : ng00–02 K5/K10, feuilles 16 et 24 fixées à l'avance, au moins cinq processus par bras,
dix passes, ordre entrelacé ; comparer les médianes des passes 2..10 **par processus** et les ratios appariés.
Rejouer le témoin v11 dans la même campagne si une comparaison v11 est publiée. Ajouter les trois uniformes pour
détecter un transfert du coût vers les populations volumineuses. Les trois trames historiques sont le diagnostic de
départ ; le contrat final exige les plusieurs séquences retenues au plan, médiane et maximum, petits et grands nuages.
Ne retenir un gain qu'avec le juge prévu par MESURE et les sorties exactes ; ne pas choisir après coup la meilleure
feuille pour chaque bras sans une campagne de confirmation.

Une fois le FULL disponible, **son chrono propre** juge les 100 ms. Aucun total de cette note n'additionne les mesures
isolées des GPU A/C, du catalogue F2 et de la résolution M7 pour fabriquer un temps intégré.

### Actualité distincte : session G, lecture documentaire seulement

Après le pin de cet audit, `5c5fc7109` publie [MES-P](../../g4_t0g_20261007/README.md). Son README a été lu via ce commit,
sans relire ici ses 318 prises : il s'agit de la **v11 gelée à 48 fils**, pas du produit v12. Il signale la forêt à
92–99 % sur les réseaux entiers, 8,436 s à 10 000 sites K5 et expiration K10 ; les sphères de 3 000 / 10 000 sites
sont refusées. Ces familles doivent entrer dans les futures ablations ; aucune cause commune exacte avec ETH3D n'est
établie par la seule part de temps « forêt ». Ni coût CPU à un fil ni seuil CPU/GPU v12 n'en découlent. Cette actualité
est hors des 48 entrées et de la reproduction F2/E ci-dessus.
