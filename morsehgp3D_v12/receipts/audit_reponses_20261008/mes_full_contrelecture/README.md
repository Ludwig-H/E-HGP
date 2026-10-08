# Contre-lecture indépendante MES-FULL K — 8 octobre 2026

Lecteur prêt pour les résultats de la session K ; **aucun résultat réel de cette campagne n'est qualifié ici**.
Les résultats étaient encore absents lors de la préparation. Ce reçu complète CST-0018, sans modifier le pilote
livré ni relancer la campagne. Aucun moteur, build, GPU ou contrôleur GCP exécuté.

`capture.json` épingle le plan K `24cdb3e5…`, le pilote livré `a89ceec9…` au commit `c9ac60f20`, et l'émetteur
`815cb33f…` au commit `c40318ebd`. Le manifeste `bundle_manifest.json` a été lu seul dans le tar **non comprimé**,
par accès aux en-têtes puis au membre de 42 084 octets : SHA `a1323fa2…`. Seuls ses 37 noms et comptes sont
conservés. SHA de l'archive déclaré par le préflight séparé, pas recalculé ici. Les comptes ng00–02 proviennent
des tailles XYZ/IDs : 39 885 / 35 551 / 45 845 entrées. Aucune coordonnée, identité de compte ou copie de données
sous licence n'est présente ; ni géométrie ni absence de doublons n'ont été recalculées.

## Admission et périmètre

`reader.py` admet chaque flux JSONL entier : CPU sans `open`, appareil avec un unique `open` réussi, puis
`full(i), liberation(i)` pour chaque passe, puis `exit` réussi. Aucune ligne non vide inconnue, clé répétée,
NaN, racine non objet ou texte non ASCII n'est ignoré. Champs exacts de cet émetteur, entiers u64 sans bool,
statuts, rangs de passe, SHA hexadécimales, profil 21, 48 fils, K, voie, noms et sites attendus sont contrôlés.
Le nom demandé en D est exactement `name[-23:]` ; les 37 noms **et** leurs 37 étiquettes sont uniques.

Les inclusions temporelles imposées sont justifiées par les fenêtres séquentielles du code :
`P+C+G+raccord+TMVR <= wall_ns`, `T+M+V+R <= TMVR` et `g_ns.tables+g_ns.resolution <= G`.
Pour cette dernière, `src/tower/passes.cpp:153–201` chronomètre construction des tables et résolution dans des
fenêtres disjointes, ordre après ordre ; l'appel entier est englobé par G dans la sonde. Les diagnostics C restent des sous-diagnostics : leur somme
ne constitue ni C ni le mur. `g_ns`, `hors_mur_ns`, mémoire et libération sont également validés, sans nouvelle
somme de durées non prouvée.

Chaque pic est au moins `16 * somme(sites des entrées distinctes du processus)`. `InputFiles` conserve trois
Buffer u32 (x/y/z) et un Buffer PointId, enum de base u32 ; `io/input.cpp:125–131` les alloue dans le même budget.
La sonde garde toutes ces entrées dans `frames` jusqu'à la fin de `run`, et passe leurs vues au préparateur.
`MemoryBudget::restart_peak()` reprend `used()` : le Pool ou la remise à zéro du pic ne retirent donc pas les
entrées résidentes. En D, on somme les 37 trames une fois, jamais les 74 passes. C'est un plancher conservateur,
pas une estimation de toute la mémoire de FULL. Ces sources supplémentaires sont épinglées dans la capture.

La couverture est celle du **plan K figé**, pas une règle générique sur tout futur pilote :

| Bras | Processus | Passes | Passes chaudes |
| --- | ---: | ---: | ---: |
| K5 appareil, ng00–02 | 15 | 150 | 135 |
| K10 appareil, ng00–02 | 9 | 45 | 36 |
| K5 CPU, ng00–02 | 9 | 45 | 36 |
| K5 appareil, 37 trames successives | 5 | 370 | 185 |
| Total | 38 | 610 | 392 |

L'ordre tournant demandé par processus est reconstitué. En D, seules les passes du second tour font foi.
La constance FUL1 porte sur toutes les passes de chaque trame/K, et sur les deux voies pour ng K5. Aucune égalité
entre une trame ng et un éventuel alias dans v12set n'est ajoutée sans preuve d'identité d'entrée.

Les statistiques, empreintes et deux contrats sont recalculés depuis les bruts, puis comparés récursivement au
rapport (types compris). Par trame, la médiane porte sur **toutes les passes chaudes réunies**, pas sur les médianes
des processus. La règle écrite reste inchangée : médiane de ces valeurs entre les trames et maximum, parmi les
trames, des médianes maximales par processus, tous deux <= 100 ms. Le **maximum brut** reste diagnostique.
Le témoin positif contient donc délibérément des passes à 200 ms tout en tenant cette règle ; un maximum des
médianes par processus à 100 000 001 ns produit bien « non tenu ».

L'admission de campagne exige un GPU **connu vide** avant et après (chaîne vide, jamais `null`), des informations
d'environnement disponibles, le plan effectif et aucune entrée dans `refus`. En cas de condition manquante ou
de désaccord du rapport, les bruts valides restent signalés comme tels et les statistiques restent calculées ;
le verdict indépendant est « refuse ». « non tenu » désigne une mesure admise qui dépasse la règle de temps.

## Preuves et limites distinctes

Le pilote n'archive pas le code externe de chaque sonde. Sans `--codes`, `refus=[]` implique des retours zéro
**sous l'hypothèse, à établir séparément, que ce rapport provient du pilote épinglé** : son `parse_process`
rejette tout code non nul. Le lecteur ne fabrique aucun champ de code externe. Si des codes sont archivés
ailleurs, `--codes` prend un objet `{nom_du_jsonl: code}` complet et impose des entiers zéro.

L'identité des fichiers reçus, la provenance de construction/exécution sur G4 et la fermeture de la Session
restent des preuves séparées ; le lecteur ne les déduit pas de l'existence des sources Git. Il vérifie les pins
du plan/sources, la stabilité des fichiers pendant sa lecture et rend les SHA du rapport et des 38 bruts.
Les chemins du rapport sont contrôlés comme chaînes ; leur liaison aux substitutions du plan et au paquet
réel reste dans l'audit de provenance. L'égalité des comptes aux métadonnées n'est pas un rehash des entrées.

`pic_octets` désigne `MemoryBudget::peak()`, commun aux Buffer hôte et réservations GPU/épinglées, incluant le
résident déjà présent. Il est prélevé après le mur et avant validation/empreinte : ni RSS, ni pic VRAM séparé,
ni pic du validateur. La sonde ne publie pas de CPU·s/trame. Le mur exclut lecture, validation, empreinte,
libération et initialisation de `PassState`. `open` mesure seulement `CatalogueDevice::open`, après la création
du Pool et du budget de Session. Aucun impact numérique de ces frontières n'est estimé ici.

## Rejeu

Depuis la racine Git contenant les commits épinglés :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 CHEMIN_DU_RECU/check.py
PYTHONDONTWRITEBYTECODE=1 python3 -O CHEMIN_DU_RECU/check.py
```

Sorties identiques au champ `result` de `capture.json` : **53 mutations refusées**, témoin synthétique complet
admis, statistiques identiques à celles du pilote épinglé. Le contrôle utilise ses seules fonctions Python
de lecture/statistique, jamais `run`, `build` ou `main`. C'est une contre-épreuve du lecteur, pas une sortie
native ni une qualification GPU.

La contrelecture indépendante a ajouté les témoins « table hors G », « somme de sous-étapes hors G », pic nul,
pic inférieur à une entrée et pic inférieur à la somme des 37 entrées. Les positifs synthétiques portent désormais
un pic supérieur à ce plancher. Le premier lecteur encore non publié omettait ces deux gardes ; aucun résultat
réel n'avait été admis par cette version.

À réception des résultats, sans lancer de campagne :

```sh
PYTHONDONTWRITEBYTECODE=1 python3 CHEMIN_DU_RECU/reader.py \
  --repo DEPOT_GIT --plan PLAN_K_JSON --results DOSSIER_CONTENANT_RAPPORT_ET_BRUT
```

Un dossier incomplet est refusé. Les reçus d'admission et de livraison antérieurs restent immuables.
