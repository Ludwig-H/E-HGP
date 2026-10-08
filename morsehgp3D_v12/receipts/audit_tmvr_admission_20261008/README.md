# TMVR : réponse sur l'admission de R — 8 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`public_status=not_claimed`. Sources du prototype repo5 non intégré, base
`c903774b1`. Lecture statique et métadonnées seulement ; aucun moteur rejoué.
Les reçus historiques des profils restent inchangés et ne qualifient pas ce corps.

Le patch `6f0643ac…` remplace `39622284…`. Ses 31 fichiers s'appliquent sur
une extraction temporaire de sa base et reproduisent exactement repo5.
L'ancien repo3 demeure inchangé. Depuis repo3, onze fichiers du patch diffèrent :
cinq corps internes T/M/V/R et validation, la porte et son manifeste, son
enregistrement CMake, l'architecture et les deux fichiers des raisons.
Les règles géométriques du noyau, de contraction et de verticale sont conservées.

La seconde admission de R couvre maintenant **4 A + 8 Σ(R_k + 1)**, avant
`place_rows`. Les offsets sont inclus même pour un ordre sans cellule retenue.
Le corps `63a86cde…` intègre ainsi le correctif proposé dans
`../audit_registre_branches_20261007/`. A demeure borné par les représentants
relus, et non par le nombre d'événements. Les compteurs logiques sont inchangés.
`admit_stage` cumule les octets par étage dans l'état privé ; ses sept appels
restent sur le pilote séquentiel. La validation admet aussi ses tampons :
maximum des nombres de naissances et de quatre fois les nœuds de l'ordre
inférieur, car les deux tableaux temporaires ne coexistent pas.

La nouvelle porte `mhgp12_tower_forest_admission` joue les quatre étages
séparément sur 40 petits nuages synthétiques, K de 2 à 5, budget illimité
**sans cache**. Elle exige `pic − usage_entrée ≤ octets_admis` et compare le
registre obtenu à `build_forests`. Pour R, travail, tâches et sortie coexistent
avant restitution : le pic couvre la somme exacte des deux admissions.
Le mutant retire uniquement les offsets de la seconde admission. Les
allocations réussissent encore ; c'est l'inégalité de la porte qui échoue.
Cette causalité est donc distincte d'un refus tardif de `Buffer`.

Le rapport isolé du développeur annonce témoin vert et mutant **TUE par code**,
sans signal ni délai ; son hash d'arbre correspond exactement à **repo4**
(`7f466791…`). Le manifeste et les corps de la porte/R sont conservés dans
repo5. Nous n'avons ni rejoué ces binaires ni retrouvé ici leurs sorties
natives détaillées. Ce test ne démontre pas un refus anticipé avec un budget
fini ni le régime du cache : la position de l'admission avant allocation est
contre-lue dans le code. La qualification complète de repo5 reste ouverte.

T7 n'est pas ajouté : l'oracle, les formes et `tower.hpp` restent ceux de
repo3 ; le témoin cercle25 à quatre sites et LEM-T7 restent à intégrer.
Le juge G durci est conservé (`c88be31c…`, raisons `device_unavailable=29`,
`device_fault=30`, puis `tower_query_domain=31`). Le nouveau plan final5
prévoit aussi la chaîne complète aux profils 24/32. Il était seulement en
construction lors de cette lecture : aucun résultat final5 n'est transféré
des campagnes repo3 ou du mutant isolé repo4.

```sh
python3 -B -S check.py --prototype DOSSIER_TMV --git-repository DEPOT
python3 -O -B -S check.py --prototype DOSSIER_TMV --git-repository DEPOT
```

Le lecteur vérifie les pins avant/après, le rapport mutant et l'application
temporaire ; ses sorties doivent égaler `capture.json.result`. Aucun payload,
chemin privé ou identité de compte n'est conservé.
