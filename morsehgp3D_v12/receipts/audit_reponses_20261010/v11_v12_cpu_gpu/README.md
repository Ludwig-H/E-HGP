# v11 / v12 : CPU encore en retrait, GPU nettement devant

10 octobre 2026. **Le dernier CPU K5 mesuré sur ng00–02 reste 13,14 / 17,01 /
13,41 % plus lent que la référence v11**, malgré les progrès GPU. Comparaison
descriptive entre campagnes, réglages et frontières différents ; aucun essai
v11/v12 apparié ni nouvelle qualification native. `public_status=not_claimed`.

Murs FULL chauds, W48/u21, en ms ; ng00/01/02 portent 39 885/35 551/45 845 sites :

| Voie et K | ng00 | ng01 | ng02 |
| --- | ---: | ---: | ---: |
| v11 CPU K5, référence | 313,547 | 255,093 | 313,152 |
| v12 O CPU K5 | 354,732 | 298,496 | 355,136 |
| v11 GPU K5, référence | 251,288 | 212,161 | 255,386 |
| v12 O GPU K5 | 80,365 | 66,933 | 79,336 |
| v12 B3b clés GPU K5, adopté B3-K | 78,628 | 64,904 | 78,122 |
| v11 GPU K10, référence | 1 782,208 | 1 335,963 | 1 536,381 |
| v12 O GPU K10 | 492,641 | 365,517 | 422,868 |

GPU K5 : quotient descriptif v11/v12 O **3,13–3,22**, puis **3,20–3,27** pour
les clés B3b ; K10 O **3,62–3,65**. B3b n'a pas remesuré le CPU ni K10 de ces
trois trames. Aucun CPU K10 ng00–02 récent ni référence v11 des 37 trames n'est
introduit ici. La cohorte v12 de 37 trames reste jugée séparément.

**Statistiques et sources.** La référence de [MESURE §3](../../../docs/MESURE.md)
est le bras `base` de `claudeg1`, archive `733912e65`, pas le candidat AVX2 retiré.
Les trois rapports `gpu_ab_report_ab_k5_16_cpu.json`, `…k5_24_gpu.json`,
`…k10_24_gpu.json` sont épinglés dans [capture.json](capture.json). Leurs neuf passes
chaudes sont recalculées : un processus de dix passes par trame/voie, médiane des
passes 2–10. O : médiane des 45 chaudes GPU K5, ou 12 CPU K5/GPU K10, réparties
sur cinq ou trois processus. B3b : médiane des dix médianes de sept chaudes ; son
test apparié contre A6c est distinct de cette comparaison v11.

Les rapports d'audit v11 **H** (§1, lignes 73–80 ; §3, 211–238) et **G** (§4,
215–242 ; §6, 298–320) sont épinglés aussi : G reproduit les identités, mais ses
temps CPU K10 locaux W8 sous charge ne sont pas une référence G4. Aucun minimum
choisi ni résultat `new` rejeté ne remplace la base. Les 196,8 ms historiques de
ng01 avec le réglage glibc `@tas` sont un autre régime, pas la référence ci-dessus.

**Réglages et frontière à conserver dans toute lecture.**

| Élément | v11 de référence | v12 O / B3b |
| --- | --- | --- |
| Catalogue | CPU feuille 16 ; GPU feuille 24 | feuille 24 dans les deux voies |
| GPU K5 | `868347:400` : part hôte 400 ‰ du travail estimé des feuilles | parcours/catalogue CUDA, reprises exactes hôte selon leur domaine |
| GPU K10 / CPU | `868347` / `802811` | un chemin produit par voie |
| Cache de blocs | 4 Gio, activé dans la sonde | 8 Gio, réserve du budget résident |
| Allocateur | aucun suffixe `@tas` dans ces bras ; environnement hérité | aucun réglage glibc forcé par le pilote lu |
| Début du mur | index, **après** `prepare_cloud` | **avant** `prepare_cloud`, tri et index inclus |
| CUDA première passe | préchargement dans FULL | ouverture de Session hors FULL |
| Fin du mur | tour en mémoire, verticales ; avant sérialisation | tour en mémoire ; avant validation, digest et libération |

La v11 produit/CLI ne jouait pas ces réglages de sonde : masque 278523, feuilles16,
sans cache ni GPU. On compare donc la référence de banc choisie dans MESURE à la
v12 mesurée, sans appeler les nombres v11 des temps CLI. L'environnement glibc
complet de v12 n'est pas enregistré dans le rapport O ; pas de preuve d'identité
des politiques d'allocation. Sources des frontières : v11 `full_probe.cpp`
200–243, 340–371 et `gpu_ab.py`59–61,250–258 ; v12 `full_probe.cpp`1–50,275–303,
439–464, et `pilote_full.py`171–180. Lecture sur les pins de la capture.

**Le seul ajout de préparation n'explique pas le retard CPU observé.** Pour chaque
passe chaude O, soustraire *tout* P, puis médianer donne **352,630712 / 296,488011 /
352,838236 ms**, encore **+12,465 / +16,227 / +12,673 %** contre v11. Cette
soustraction favorise même davantage la v12 : elle retire l'index, pourtant inclus
en v11. C'est une borne algébrique conditionnelle des exécutions, pas une mesure
à frontière identique ni une prédiction après modification.

Le catalogue CPU O coûte 299,421 / 254,274 / 301,038 ms, contre le `domain` v11
199,515 / 162,809 / 194,903 ms ; ces étages n'ont pas des implémentations identiques.
Le reste FULL−C de chaque passe O médiané vaut 55,269 / 44,129 / 54,050 ms :
les progrès de tour ne compensent pas le catalogue. Ne pas additionner ces médianes.
**Revenir à 16 n'est pas une piste nouvelle** : le [contre-audit déjà publié](../../audit_reponses_20261008/cpu_feuilles_finition/README.md)
donne catalogue16/catalogue24 = 1,009 / 1,030 / 1,005 sur I, le gain feuilles étant
compensé par le parcours. Ni cette observation ni la différence v11/v12 n'attribue
causalement l'écart à un paramètre. La priorité reste de mesurer le travail total
du catalogue CPU et sa finition sur le code modifié, sans nouveau port implicite.

[check.py](check.py) lit 15 sources/rapports Git épinglés et 33 JSONL O déjà admis ;
aucun journal dupliqué, aucun moteur, cloud ou payload. Rejeux normal et `-O` conformes :

```sh
python3 -B -S check.py --repo /workspaces/E-HGP --full-raw /workspaces/.ehgp-sessions/v12.20261010.fullo/results/extracted/results/cmd/001_mes_full/files/full/brut
```

L'[admission O](../session_fullo_admission/README.md), les [temps O](../fullo_temps/README.md),
les [statistiques B3b](../b3b_stats/README.md) et l'[adoption B3-K](../b3k_produit/README.md)
conservent leur propre portée de preuve. Ce reçu n'ajoute aucun chrono.
