# Audit indépendant v11 pour l'auditeur v12 : preuves, mesures, qualification

7 octobre 2026. Snapshot documentaire `33c2ae3c8b66d12ef5b2405f1b0f807d7e57aeae`, moteur gelé `ac081a06f`.

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Cet examen complète `docs/AUDIT_GEANT_V11.md`, après lecture de README, ARCHITECTURE, PROVENANCE, PASSATION,
AUDIT_FINAL et des rapports H/E de cet audit. Il n'en hérite pas les comptes sans les recalculer. Aucun moteur,
reçu historique ou réglage de production n'a été modifié ; aucune compilation lourde et aucun scan LiDAR ne sont
nécessaires aux vérifications ci-dessous. Les autres volets mathématiques et moteur appartiennent aux rapports
des autres auditeurs.

## 1. Ce qui est nouveau

| ID | Importance | Constat | Portée démontrée |
| --- | --- | --- | --- |
| EV-01 | Moyenne, harnais | Sept juges d'adoption du 7 octobre peuvent imprimer `garde`/`adopte` après avoir constaté `verdict=refus`, puis sortir 0 | Contre-épreuve synthétique sur chaque juge, Python normal et `-O` ; aucune adoption historique erronée démontrée |
| EV-02 | Faible, compte de preuve | L'audit géant annonce **3 303 vidages**, mais il existe **3 303 tentatives et 3 285 empreintes de vidage** | Recompte des 81 rapports ; 18 refus sans sortie conservés dans `claudegpu2` ; aucune divergence entre les empreintes disponibles |
| EV-03 | Moyenne, contrat de mesure | `chaud` désigne des régimes différents dans les deux bancs principaux | `gpu_ab` conserve processus/Pool/contexte ; `sorties_g4` lance un nouveau processus pour chaque prise, même dite chaude |
| EV-04 | Faible, provenance du banc | `gpu_ab.build` reprend `b_cuda/mhgp11_full_bench` sans vérifier sa relation aux sources `new` | Contre-épreuve : un fichier préexistant est rendu même avec une source inexistante ; aucune mauvaise provenance d'un reçu G4 démontrée |

Les comptes et limites déjà connus sont confirmés : absence de contrat 100 ms ; chiffres de base K5/K10 ; médiane
haute à froid ; K10 sur trois processus ; qualification finale composée de deux pins au moteur identique ; SPv2
plus largement qualifié que ne l'annonce la passation ; produit CPU différent du banc GPU ; portée conditionnelle
des comparaisons à HDBSCAN.

## 2. EV-01 : les juges de performance ne bloquent pas sur une mesure invalide

Les sept fichiers `receipts/developpement_20261007/*/*/judge.py` testent le verdict du banc, écrivent
`banc non conforme`, puis poursuivent. Les six juges de leviers lisent les prises dont le code vaut 0 ; celui du
cache lit les médianes publiées. Aucun des sept ne refuse alors la décision d'adoption. `main()` ne renvoie pas de
code et le programme termine normalement.

La contre-épreuve `probe_runners.py` fabrique deux rapports minimaux, CPU et GPU, avec les trois noms de trame,
six prises par bras, des durées positives et un rapport de coût 0,1. Trois cas sont exécutés, chacun normal et `-O` :

1. contrôle : `verdict=conforme`, six prises ;
2. même entrée, `verdict=refus`, `refusals=[synthetic_dump_mismatch]` ;
3. `verdict=conforme`, une seule prise par bras au lieu des six annoncées.

Résultat : **42 exécutions, 42 codes 0** ; les **14 exécutions sur un banc invalide** avertissent puis publient une
adoption ; les **14 à une seule prise** publient aussi l'adoption. Les six fixtures sont conservées dans
`synthetic_fixtures/`, les empreintes des neuf sources inspectées et les verdicts dans `runner_probes.json`.

Les juges concernés sont ceux des annonces, du cache, des cohortes, de G1 AVX2, de la frontière, du préchargement
des graines et des préchargements combinés. Ce sont des aides de décision conservées dans des reçus, pas le juge
géométrique du moteur. Le banc `gpu_ab` lui-même rend bien un refus lorsqu'une identité manque. **Rien dans ce
test n'établit qu'un levier a été historiquement adopté sur une sortie fausse** : les mesures des sept sessions
du 7 octobre sont conformes dans les rapports archivés ; le défaut est la composition des deux étages de décision.

Exigence v12 : le juge doit rendre `invalid_measurement` avant tout rapport d'adoption si le banc n'est pas
conforme, si une prise attendue manque, si les sorties ne sont pas toutes jugées ou si le schéma/pin n'est pas
celui préenregistré. L'échec doit avoir un code non nul et un champ structuré. Ajouter des mutants de ces quatre
conditions et épingler aussi le juge au lancement. L'absence d'empreinte initiale des juges était déjà relevée par H.

## 3. EV-02 : les 18 refus ne sont pas des vidages concordants

Le recomptage parcourt exclusivement les 81 documents `schema=ehgp.v11.gpu_ab.v1` sous les reçus de développement
des 4–7 octobre, lus à travers Git au pin fixé.

| Population | Tentatives froides | Processus chauds | Empreintes froides disponibles | Empreintes chaudes disponibles |
| --- | ---: | ---: | ---: | ---: |
| CPU / lot hôte | 1 089 | 216 | 1 089 | 216 |
| GPU | 1 617 | 381 | 1 602 | 378 |
| Total | 2 706 | 597 | 2 691 | 594 |

Donc **3 285 vidages disponibles**, dont **1 980 GPU**. À chaque couple (trame, K), tous les SHA non nuls sont
identiques : **802 K5 et 293 K10 par trame**, soit `(802 + 293) × 3 = 3 285`. Le détail de H était déjà correct ; sa
somme introductive et celle d'AUDIT_GEANT ont compté les tentatives sans sortie.

Les 18 absences viennent exclusivement de
`receipts/developpement_20261004/gpu_g4/sessions/claudegpu2/gpu_k5_report.json`, source `d5b1d0179` : 15 prises
froides et trois processus chauds, code **3**, `summary.exit=invariant_violated`, `dump_sha256=null`, verdict
global `refus`. Le README de ce reçu explique la garde erronée « feuilles ≤ sites » alors que les feuilles se
recouvrent. Ce sont des refus connus et conservés, **pas 18 résultats géométriquement divergents**.

Limite supplémentaire déjà explicite dans le banc : les passes chaudes 1 à P−1 n'écrivent aucun vidage. Les 594
empreintes chaudes ne qualifient que la dernière passe de chaque processus ; les passes intermédiaires sont
contrôlées par séquence et statut. Aucun calcul de débit ne transforme ces répétitions en nouvelles scènes.

## 4. Chiffres de performance recalculés

Référence au gel : variante **base** des trois rapports `filtre_g1_avx2/claudeg1`, source `733912e65`, moteur
identique à `ac081a06f`. La variante `new` est G1 AVX2, retirée ensuite ; la confondre avec le gel serait une erreur.

| K / route / feuille | Trame | Froid : médiane haute | Froid : médiane usuelle | Chaud : médiane passes 2–10 |
| --- | --- | ---: | ---: | ---: |
| 5 CPU / 16 | ng00 | 343,3 ms | 338,9 ms | 313,5 ms |
| 5 CPU / 16 | ng01 | 272,5 ms | 271,6 ms | 255,1 ms |
| 5 CPU / 16 | ng02 | 328,9 ms | 327,7 ms | 313,2 ms |
| 5 GPU / 24 | ng00 | 335,2 ms | 332,3 ms | 251,3 ms |
| 5 GPU / 24 | ng01 | 300,7 ms | 300,5 ms | 212,2 ms |
| 5 GPU / 24 | ng02 | 345,1 ms | 343,3 ms | 255,4 ms |
| 10 GPU / 24 | ng00 | 1 824,2 ms | 1 824,2 ms | 1 782,2 ms |
| 10 GPU / 24 | ng01 | 1 395,5 ms | 1 395,5 ms | 1 336,0 ms |
| 10 GPU / 24 | ng02 | 1 601,6 ms | 1 601,6 ms | 1 536,4 ms |

K5 froid : **six** processus par cellule, statistique publiée = quatrième valeur triée. K10 : **trois** processus.
Chaque cellule chaude vient d'**un seul processus**, neuf passes retenues : ce ne sont pas neuf répliques
indépendantes. `recomputed.json` conserve les nombres non arrondis, les modes, les étapes domain/forêts, les SHA et
le nombre de prises. Ces mesures ne tiennent pas les 100 ms. Le passage ponctuel à 196,8 ms sur ng01, relevé dans
l'audit antérieur, n'est ni un contrat sur trois trames ni une mesure de cette base finale.

La comparaison v10/v11 reste descriptive : u18 contre u21, captures différentes, troisième passe unique pour
la v10 contre médiane de passes pour la v11, et différentiel canonique v10 non entièrement fermé. La présence
d'effectifs identiques ou d'un même objet mathématique ne suffit pas à transformer ces captures en A/B.

## 5. EV-03 : définir le temps et la chaleur avant de comparer

| Protocole | Processus | Objets réutilisés | Périmètre mesuré |
| --- | --- | --- | --- |
| `gpu_ab` froid | Neuf à chaque prise | Caches OS/driver non vidés | Chrono `full_pass` : index, domaine, forêts/verticales |
| `gpu_ab` chaud | Un processus par (trame, bras) | Pool, budget/cache, contexte GPU | Même chrono ; `prepare_cloud` est refait **hors chrono** à chaque passe |
| `sorties_g4` froid | Neuf | Caches système éventuellement déjà chauds | Étapes CLI et mur du sous-processus |
| `sorties_g4` chaud | **Encore neuf à chaque prise** | Caches OS uniquement, pas la Session/Pool du processus précédent | Étapes CLI ; ce régime n'est pas celui de `gpu_ab` chaud |

Sources exactes : `bench/full_probe.cpp:208` ouvre le chrono FULL ; `:343` définit la routine qui lit/prépare
l'entrée et construit le Pool avant cet appel ; `:367` refait le nuage hors chrono. `bench/sorties_g4.py:84` appelle `subprocess.run` pour
chaque `run_one`, et `:117` emploie cet appel pour les prises étiquetées froides **et** chaudes.

Le chrono FULL exclut la lecture, la préparation du nuage, la création du Pool et la sérialisation. Il inclut
l'index et l'ouverture du contexte CUDA, recouverte avec le travail hôte, quand ce contexte n'existe pas encore.
Il s'arrête après `build_full`, avant le vidage et la destruction du résultat. À titre de contrôle de portée,
les durées externes médianes des processus K5 de ces mêmes reçus sont **765,5 à 1 042 ms**, contre les 272–345 ms
du chrono FULL ; les processus K10 prennent 4,237–5,306 s. Ces durées externes sont celles du **banc qui sérialise**,
pas une mesure nouvelle du CLI et pas un budget demandé par l'utilisateur.

La v12 devrait nommer ces champs `fresh_process`, `resident_process`, `filesystem_warm` et déclarer explicitement
les bornes du chronométrage. Fixer aussi latence ou cadence, sortie en mémoire ou écrite, retrait de sol inclus ou
exclu, profil, plage de trames et statistique de succès. Les trois trames séquence 08 ne couvrent pas plusieurs
séquences. Aucune proposition de contrat de l'audit antérieur ne vaut décision utilisateur nouvelle.

## 6. Qualification : quelle preuve porte sur quel code ?

Le script vérifie directement les différences Git sur `src/`, `cli/`, `bench/`, `tests/`, `tools/`, `cmake/` et
CMake : **aucune différence moteur entre `38b76701b` et `98a009550`**, seulement quatre fichiers de portes/banc/
matrice ; **aucune différence dans ces chemins entre `733912e65` et `ac081a06f`, ni entre `ac081a06f` et le snapshot**.

| Pin | Preuve disponible | Ce qu'elle ne qualifie pas |
| --- | --- | --- |
| `38b76701b` + `98a009550` | Union finale de 12 sessions : **3 695** appels de portes Release/poison recomp­tés ; 485 mutants annoncés et détaillés dans le reçu de reprise ; sanitizers/long/S9/S10 répartis sur les sessions | Ce n'est pas une matrice unique sur le code du 7 octobre |
| `38faaf272` | Release u18 **890**, u21 **800**, u24 **800**, ASan u24 **800** ; **118** appels échelle/LiDAR dans chacun des trois profils | Pas de TSan, long, poison, campagne complète de mutants ou S9/S10 à ce pin |
| `b6fd3796d` | ASan/TSan ordinaires **807/807**, campagne ciblée du levier G1 | Variante AVX2 retirée, pas le code exact du gel |
| `ac081a06f` | Bancs base et empreintes ; vérifications locales décrites par l'autre audit | Aucune matrice G4 complète sur ce code exact ; aucune promotion publique |

**3 695** est une somme d'exécutions par configuration, pas 3 695 assertions différentes. Les tests sous
sanitizers et les campagnes sont des ensembles supplémentaires, avec recouvrement. Le recomptage des manifestes
actuels donne **530 mutants dans 13 modules**. La qualification exhaustive historique en u18 ne se transfère pas
au profil produit u21 ni aux 45 mutants ajoutés ensuite. Les 17 jamais joués sur G4 sont identifiés dans H ; le
présent volet n'a pas reconstruit leurs historiques de compilation.

SPv2 : les portes ordinaires, échelle et LiDAR des profils ci-dessus incluent les supports, ce qui confirme
l'erratum de l'audit géant contre « Release u21 seulement ». Les oracles SPv2, les mutants SPv2, les passages TSan
et le CLI K10 restent des objets de preuve distincts. Ne pas remettre SPv2 au statut de SPv1 par cumul de comptes.

Contrôle positif du runner de matrice : contre-épreuves sur `g4_matrix.py`, sans compilation : zéro configuration
ou uniquement `absent` rendent **3** ; `ok+failed` rend **1** ; un résultat JUnit contradictoire donne `failed` ;
aucun résultat donne `vacuous`. `ok+absent` rend 0 : c'est la sémantique documentée des outils optionnels, à publier
avec la liste des absences, pas une qualification de Clang absent. Les sondes `probes` ne participent pas à la
conformité et portent explicitement `isolation=not_certified`.

## 7. Provenance et EV-04

**219 entrées SHA-256 recalculées sans divergence** dans les quatre manifestes : qualification finale (83),
qualification V3 (23), filtre G1 (11), audit géant (102). Ce contrôle atteste la cohérence des fichiers conservés
avec leurs manifestes, pas une signature externe ni l'exactitude de leur contenu. Chaque source effectivement
lue par `recompute.py` figure dans `inputs_sha256`.

`gpu_ab.build` (`bench/gpu_ab.py:159`) accepte un exécutable déjà présent dans `b_cuda`, journalise son SHA, puis
ne construit rien. La contre-épreuve conserve un fichier factice non exécutable et fournit une source inexistante :
la fonction le reprend tout de même. Les variantes archivées ont été durcies par l'empreinte de leur archive dans
le chemin de build ; le cas `new` demeure indexé par le seul dossier de travail. Le SHA permet de détecter une
réutilisation si on possède déjà la bonne correspondance source/binaire, mais ne l'établit pas.

Le protocole gardé utilise des sessions neuves ; **aucun résultat G4 historique n'est déclaré invalide sur ce seul
test**. Pour la v12 : artefact neuf ou cache indexé par source + configuration + compilateur ; provenance des
dépendances, binaire et juge vérifiée avant et après. Un `--commit` saisi à la main, comme dans `sorties_g4`, reste
une déclaration, pas une preuve de compilation. L'erreur de pin historique de ce banc est déjà documentée.

Hygiène : le constat antérieur d'un paquet v8 contenant un scan brut et de chemins de compte GCP dans les reçus
reste ouvert. Ce travail ne copie aucune archive d'entrée ni contenu de scan et ne verse aucune identité de
compte. Les JSON produits ne contiennent que statistiques, statuts, empreintes et chemins relatifs du dépôt.
Un éventuel nettoyage d'historique est une opération distincte ; il n'est pas effectué par cet audit.

## 8. HDBSCAN, thèse et application : séparer les questions

Recalcul indépendant des résultats synthétiques à partir des membres `synthetic/*.json` des archives D/F, sans
extraire de coordonnées : pour n=8 000, les **64 scènes par K** donnent exactement les mêmes résultats dans les
deux sessions.

| K | Différence du meilleur IoU moyen `margin_r − HDBSCAN` | Scènes où HGP fait au moins aussi bien |
| --- | ---: | ---: |
| 2 | +0,00824192 | 41/64 |
| 3 | +0,02623606 | 47/64 |
| 5 | +0,05001293 | 48/64 |
| 10 | +0,07833366 | 48/64 |

Cela confirme **un avantage empirique sur cette population synthétique au niveau B**, meilleur bloc choisi à
l'aide de la vérité terrain. Ce n'est ni une sélection autonome ni une preuve de supériorité pour toute entrée.
Les scores source sont arrondis à six décimales ; ce recalcul ne requalifie aucune décision exacte interne.
`cover` et `first` sont aussi recalculés : ils suffisent à montrer qu'on ne peut attribuer le gain à la marge.

Le tableau agrégé de la sortie plate conserve bien **0,6854 contre 0,5663** tous K, et **0,7790 contre 0,7652**
à K10. Cette lecture confirme les valeurs conservées ; elle ne refait pas les évaluations individuelles absentes
du tableau. La population est conditionnée au succès de la hiérarchie HGP, comme déclaré dans l'étude. On ne doit
ni la présenter comme un échantillon représentatif de LiDAR, ni compter plusieurs captures d'un même objet comme
des succès indépendants. Les voisins temporels corrélés et le choix de K après observation demandent aussi une
unité statistique explicite.

L'oracle de correction de FULL, l'oracle de meilleur bloc et la qualité d'une partition choisie répondent à trois
questions différentes. L'égalité au CPU/GPU ne donne aucune qualité sémantique. La justesse de l'objet de Hartigan
ne prouve pas qu'il sépare un vélo collé à un mur. Les applications Zoltan demandent des exports et masses qui ne
sont pas automatiquement ceux de `supports`/`points`/`plat` ; la masse du § 9.1 de la thèse n'est pas le compte
entier du critère A. Ce volet ne redémontre pas les théorèmes de la thèse : il s'appuie sur l'audit mathématique
séparé pour l'objet, et limite les conclusions quantitatives aux reçus effectivement relus.

## 9. Porte d'entrée pratique pour l'audit v12

1. Tenir un registre d'événements de preuve `(pin moteur, pin juge, profil, variante, données, phases mesurées,
   nombre de processus, nombre de passes, résultat, restrictions)` ; publier un état de qualification par pin,
   sans agréger des tests historiques en « HEAD vert ».
2. Geler six empreintes LiDAR v11 K5/K10 et des petites fixtures exactes ; refaire le différentiel sur le chemin
   réellement livré. Les grands accords différentiels complètent l'oracle borné, ils ne le remplacent pas.
3. Faire du banc et du produit une seule voie paramétrée par des variantes explicitement expérimentales ; tout
   chiffre de contrat doit porter la sortie, les frontières du chrono et le régime de mémoire/contexte.
4. Garder le refus comme premier résultat possible : une sortie absente, une prise manquante, un juge en erreur
   ou un pin non vérifié interdit l'adoption. Tester aussi le lecteur de reçus avec des événements invalides.
5. Séparer campagne de correction, ablation de performance et comparaison de clustering. Écrire les règles et
   la population avant les données ; répéter par processus dans un ordre équilibré ; réserver d'autres séquences
   aux conclusions de généralisation. Les décisions de contrat appartiennent à l'utilisateur.

`ledger.json` rend ces points exploitables avec identifiants, témoins et critères de clôture. Aucun point de ce
rapport ne promeut la v11 ni la future v12 à un statut public.

## Reproduction

Depuis la racine d'un checkout contenant le snapshot :

```sh
python3 -B morsehgp3D_v11/receipts/audit_independant_v12_20261007/evidence/recompute.py
python3 -B morsehgp3D_v11/receipts/audit_independant_v12_20261007/evidence/probe_runners.py
```

Le premier lit les sources au pin fixe par Git ; le second exerce les fichiers de ce checkout, dont il conserve
les SHA, sur des fixtures artificielles. Exécuter le second au snapshot déclaré pour reproduire exactement les
résultats. Les imports n'exécutent aucun appel GCP. Le premier essai du recomptage a rencontré un `null` de vidage
et échoué avant la synthèse ; son journal est gardé dans `recompute_attempt1.log`. Le script corrigé distingue
maintenant les absences des empreintes et a terminé sans erreur. Ce défaut de l'auditeur ne concerne pas le moteur.
