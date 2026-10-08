# B2 : admission des FULL GPU et fermeture des preuves

8 octobre 2026. Source **4171b2653fa5cb903102a3fc44572bcb3330b3e8**, publication
**f33cf8d21**. Relecture Python des primaires locaux et des objets Git seulement ;
aucun moteur, compilation, contrôleur, appel distant ou payload LiDAR lu.

**Le lot B2 et le bras tables sont adoptés selon la règle préannoncée.** Relecture
et census restent rejetés. Les **373 processus / 2 816 FULL** sont admis, dont
**2 100 passes chaudes décisives**. Toutes ces prises utilisent le **catalogue GPU**,
u21, W48, cache de blocs 8 Gio ; G et la forêt s'exécutent sur CPU. La voie
informative `--sequentiel` conserve `--device`. **Aucun FULL entièrement CPU n'est
mesuré dans ce lot** ; `cpu_ns` mesure le travail CPU du processus hybride.

## Temps absolus à K5

Dix processus par bras et trame, huit passes par processus ; la première est
écartée. Chaque cellule « méd. proc. » est la médiane des dix médianes des sept
passes chaudes. « Réunie » rassemble les 70 passes ; ce n'est pas la statistique
d'adoption. Millisecondes ; valeurs exactes en nanosecondes dans `results.json`.

| Trame (sites) | Avant méd. proc. | B2 méd. proc. | B2 réunie | B2 max méd. proc. | B2 max brut |
| --- | ---: | ---: | ---: | ---: | ---: |
| ng00 (39 885) | 87,286083 | **82,629930** | 82,544725 | 82,959035 | 102,807724 |
| ng01 (35 551) | 71,626247 | **67,765654** | 67,857629 | 68,490824 | 70,407252 |
| ng02 (45 845) | 88,074307 | **84,866213** | 84,858730 | 85,425225 | 86,680774 |
| 02/001606 (64 740) | 149,912206 | **147,503467** | 147,640167 | 148,390547 | 150,588907 |
| 08/001176 (67 114) | 175,635714 | **172,658210** | 172,430601 | 175,331125 | 175,861031 |

Une seule des 70 passes ng00 après dépasse 100 ms ; les deux trames moyennes les
dépassent sur toutes leurs passes. Le maximum brut reste un diagnostic distinct
du maximum des médianes processus. Cette campagne ne couvre pas les 37 trames
contractuelles : aucune nouvelle qualification globale « FULL ≤ 100 ms ».

K10 est **informatif**, un processus par bras/trame, cinq passes dont quatre
chaudes : avant → B2, ng00 **548,786924 → 519,572578 ms**, ng01
**406,272195 → 384,933467 ms**, ng02 **465,676897 → 444,925975 ms**. Il n'y a ni
dix répétitions indépendantes, ni identité K10 mesurée ici. La grande trame
08/002119 (99 099 sites), également informative, a trois processus : médiane
des médianes **291,892106 → 286,792528 ms** ; maximum brut après **289,140578 ms**.

## Admission et statistique

Le lecteur ferme la cohorte depuis le plan (W48, P8, dix tours), le manifeste
antérieur épinglé des 37 trames et la source du pilote. Il exige exactement les
373 chemins distincts, les codes entiers 0, les indices/configurations des passes
et les types du lecteur partagé **15437e5f…**, puis recalcule tous les résumés.
Les 25 processus d'identité (deux passes chacun, cinq trames × cinq exécutables)
donnent un FUL1 identique pour chaque trame ; ng00 retrouve `3a2bfb4f…`.
Les 300 processus décisifs donnent 2 400 passes, dont 2 100 chaudes ; 48 processus
informatifs donnent 366 passes, dont 318 chaudes.

Le bootstrap est recalculé depuis les médianes des **bruts**, avec les 10 000
tirages et la graine/ordre du protocole. Adoption : borne haute strictement < 1
sur chacune des cinq trames. A/A utilise le même ELF, ses moyennes géométriques
vont de **0,9987724323 à 1,0007065447**, dans [0,985 ; 1,015]. Tous les IC et
verdicts du worker sont reproduits ; seul un ratio **informatif**
`ng00/tables_apres_relecture` diffère de 1 ULP
(`0,9439484718178398` au rejeu, `0,9439484718178397` archivé).
Cette différence exacte est conservée, sans tolérance sur les seuils.

Précision à apporter au README développeur : **seule ng02 fait échouer le bras
census**, avec borne haute **1,000220039156779**. La trame 02/001606 a une borne
**0,9996379771974621**, donc passe malgré son affichage arrondi « 1,000 ».

## Sources, archive et limites

L'archive retournée fait **986 717 octets**, SHA
`b0d3d9c49346d6cdd75002f41ec89c4847f7d2ab062ae1a49c9e00485089f9af` ; ses **456
entrées** sont vérifiées. Les 373 JSONL publiés sont identiques aux primaires.
Le rapport publié conserve les résultats numériques ; ses chemins et ceux des
cinq logs de construction ont été nettoyés. Le lecteur juge le rapport primaire.

Le paquet fournit **363 fichiers de sources exacts Git4171** dans le périmètre
src/bench/tests/cmake/CMakeLists et lecteurs B2 ; l'archive avant donne **359
fichiers exacts Git72f622a55**. Toutes les substitutions des quatre bras sont
reconstruites en mémoire, leurs préimages/postimages hachées. Le bras après a
**132/134 fichiers src identiques** au paquet ; les deux différences sont
uniquement les crochets de test CST-0241 de `pipeline.hpp` et `pipeline_run.cpp`.
Hors `MHGP12_REGION_HOOKS`, leurs corps coïncident après retrait des commentaires
et appels vides. C'est une équivalence statique du chemin déclaré, **pas** une
identité littérale « fichier par fichier », ni une comparaison d'ELF recompilés.
La sonde FULL est identique avant/après (`1068940a…`) ; les cinq configurations
archivées annoncent Release/u21/CUDA ON. A6 n'est pas dans ce chemin mesuré.

Les SHA des six bras sont identiques entre construction et **fin du décisif K5**,
y compris `avant_bis == avant`. Cette fermeture précède les prises informatives :
aucun SHA final supplémentaire après grande/K10/séquentiel n'est archivé. Les
relevés GPU sont vides avant/après décisif et après informations ; ils ne prouvent
pas l'isolation continue. FUL1 est joué séparément des chronos et ne couvre pas
toutes les structures auxiliaires de forêt.

Les quatre commandes ont code 0 : socle **141,165 s**, pilote **688,504 s**,
mutants **342,227 s**, profil **66,376 s**. Le journal socle compte **733 Passed,
0 Skipped, 0 Failed**. Les portes mutants num/index/tour passent ; ce reçu ne
reconstitue pas leurs diagnostics individuels. Worker/DONE0, aucun message
d'erreur, arrêt ciblé certifié0 en une tentative, garde intacte et réserve libérée.

Le pic actif du budget exclut les blocs hôtes inactifs du cache ; RSS est le
maximum cumulatif du processus et n'inclut pas la VRAM. Les fenêtres G/forêt se
recouvrent : aucune somme de leurs médianes n'est présentée comme un mur intégré.

## Rejeu

```sh
python check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.t2db2
python -O check.py --repo /workspaces/E-HGP --session /workspaces/.ehgp-sessions/v12.20261008.t2db2
sha256sum -c SHA256SUMS
```

Sources importées uniquement pour leurs lecteurs ; aucun `main` de pilote appelé.
Les copies de JSONL sont temporaires ; aucune archive, donnée ni source entière
n'est dupliquée dans ce reçu. `capture.json` est une liste blanche des métadonnées.
