# Contre-audit A — mesures, plan et provenance de l'ouverture v9

22 septembre 2026. Objet jugé : commit `3595725a` sur `origin/main`, audit de
la v8 fondé sur `12294241`. Lecture indépendante des documents publiés et
de leurs reçus ; aucun nouveau benchmark, aucune session GCP. Statut :
`exploration_v9_hors_registre`, `not_claimed`. Ce texte juge des **énoncés**
et propose des portes utiles au développeur ; il ne requalifie pas les
primitives ni la preuve mathématique de FULL.

## 1. Chiffres de la synthèse : vérifiés, avec leur portée

| Énoncé de la [synthèse](../docs/AUDIT_V8_SYNTHESE.md) § 4 | Contrelecture | Décision pratique |
| --- | --- | --- |
| v7 G4 uniforme 50k : tour K10 418,9 s, constructeur FULL 389,7 s ; K5 33,9 s ; 27,27 M nœuds et 21,47 M boules | Les [résultats v7](../../morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md) donnent 418,873/389,668/33,853 s. C'est une autre entrée et un autre moteur. | Bon témoin de coût aval et de mémoire, pas une projection LiDAR v9. |
| G4 CPU, trois trames brutes entières, K5/W48 : 165,214 / 34,319 / 505,479 s et 4,19 / 11,13 / 1,93 CPU logiques occupés | Les [reçus spatiaux](../../morsehgp3D_v8/receipts/q34_spatial_20260921/README.md) concordent. Ils précèdent la file de plages `5224ff4e`, sont mono-essai et n'incluent ni q2 ni FULL. | Mesurer le même chemin après port de la file avant d'attribuer un gain G4 à celle-ci. |
| Sans sol u16/2 cm, scène 0 : K5 W1 889,5 → 453,3 s ; K5 W8 108,0 s pour 741,5 CPU·s ; K10 W8 323,0 s pour 2 212,1 CPU·s | La [campagne appariée](../../morsehgp3D_v8/receipts/ground_phase1_20260921/README.md) confirme les lignes scène 0 via `BASELINE.only.json`. Les scènes 1 et 2 ont une charge croisée déclarée ; les trois premières lignes du `BASELINE.json` initial ont perdu leurs bruts et ne doivent pas servir. | Garder la scène 0 comme comparaison vérifiable, les autres comme indication de difficulté. |
| Sans sol à 1 mm : 104,63 s et 812,82 CPU·s, scène 0 K5/W8 | Le reçu `u18_resume_20260922/ground_1mm_first` est **hors du commit audité** : seulement présent dans le worktree partagé, une répétition, sans paire W1/W8. | L'étiqueter `non publié à 3595725a` ; ne pas lui donner le statut de campagne v9. |
| Atlas scène 0 K10 : 10,7 G bornes + 24,2 G tests + 18,3 G copies d'IDs | Les compteurs du [reçu sans sol](../../morsehgp3D_v8/receipts/ground_phase1_20260921/README.md) et de son `only_probe_02` les soutiennent. Addition = 53,2 G **opérations hétérogènes**, pas 53,2 G opérations de coût identique. | Publier séparément ces trois masses et leurs temps ; le nombre seul n'est pas un chrono. |
| Facteurs « K5 ×6 à ×36 ; K10 ×17 à ×101 » pour 1 s sur le flux | Recalcul de la [table de modèle](../docs/audit_v8/12_parallelisme_gpu_perf.md) § 4 bis : `370,6/(48×1,25)=6,18` (S1/K5), `852,0/24=35,5` (S2/K5), `1665,2/(48×2,05)=16,9` (S1/K10), `2423/24=100,96` (S2/K10). Les bornes mêlent **scènes, CPU W1 mesuré ou inféré, charge croisée, 24/48 équivalents et vitesse G4 supposée**. | Les imprimer comme *scénarios conditionnels*, jamais comme accélérations mesurées ni comme minorants mathématiques de tout algorithme. Le FULL absent s'ajoute en coût, mais peut partager du travail avec une refonte. |
| « La G4 consomme 0,49 fois les CPU·s locaux » | Le rapport `691,65/1417,5=0,488` est exact pour la trame brute 0. Mais il compare **G4 W48** et **local W4** sous charges et SMT différents ; les 48 fils G4 n'occupent que 4,19 CPU logiques en moyenne. | Ne pas convertir 0,49 en vitesse G4/W1 ni l'appliquer aux trames sans sol ; mesurer W1 et W24/W48 sur le **même binaire et la même entrée**. |
| 132 CTests, 129 verts, trois désactivés | Le [reçu d'ouverture](../receipts/audit_v8_20260922/README.md) confirme : 125 verts dans la première construction et huit mutations vertes dans la seconde, union 129, aucun échec. | Formuler « 129 exécutés et verts, 3 désactivés » ; ce n'est pas 132 juges passés. |

Le budget « 48 CPU·s pour 1 s » n'est qu'un plafond arithmétique si 48 fils
étaient pleinement utiles ; la G4 a 24 cœurs physiques/48 fils SMT et son
efficacité à W48 dépend de la forme des tâches. La table de facteurs prend
déjà des scénarios 24 ou 48 équivalents. Sa conclusion robuste est que **le
flux actuel dépasse fortement le budget**, pas que le gain requis est connu
au facteur près. Le chiffre 93 % d'aval FULL vaut pour le nuage uniforme v7
K10 ; il sert d'alerte, pas d'Amdahl applicable au LiDAR v9.

## 2. Plan : garder la tranche verticale, desserrer la porte de mesure

La [phase V9-1](../docs/PLAN_V9.md) a raison d'exiger tôt **un vrai objet FULL**,
avec q2, catalogue canonique, coquilles, intérieurs, parents et sortie ; un
flux q3/q4 isolé peut accélérer sans rapprocher la tour du contrat. La porte
actuelle exige toutefois les **trois trames sans sol à 1 mm, K5 et K10, W1**
avant d'ouvrir V9-2. À la seule vitesse du flux u16/2 cm, une ligne K10/W1
sur la scène 2 se compte en dizaines de minutes ; catalogue et FULL peuvent
encore accroître temps et mémoire. La porte risque de suspendre les recherches
qui réduisent précisément ce coût.

Proposition : fermer d'abord les **invariants de la chaîne** avec T2 et des
tailles croissantes ; obtenir au moins un reçu FULL sur une trame sans sol
entière à 1 mm ou un échec explicite (délai/mémoire avec compteurs et
frontière du chronomètre). Ce reçu ou cet échec suffit à ouvrir en parallèle
V9-2 sur le poste mesuré. Les six lignes trames × K et leurs répétitions
restent obligatoires **avant toute revendication de performance**. Ainsi le
développeur obtient une vérité de bout en bout sans attendre des heures pour
tester un certificat local de miniball.

La porte de V9-2 « ×10 d'opérations en moins sur le poste dominant » est un
bon signal d'exploration mais insuffisant comme sortie : les trois compteurs
de l'atlas mélangent coûts différents, et un travail peut simplement migrer
vers le census, le catalogue ou l'écriture. Ajouter au même reçu
`temps_total`, CPU·s, mémoire, taille de sortie, travail q2/q3/q4/aval,
`Σ|frontière active|`, travail de construction des certificats, et répétitions
sur trames entières. La réduction du **travail total hors sortie** et sa pente
8k/16k/32k, puis entre trames entières, guident l'hypothèse
sous-quadratique ; elles ne la prouvent pas. Le certificat k-Gabriel local par
miniball mérite cette place prioritaire, sans mosaïque globale de Delaunay ou
de Voronoi d'ordre supérieur.

Enfin, la [phase V9-5](../docs/PLAN_V9.md) reporte la trame brute entière et
les autres séquences. Or le [contrat du 21 septembre](../../morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md)
garde la trame brute entière comme contrat principal et exige plusieurs
scènes ; le [protocole sans sol](../../morsehgp3D_v8/docs/LIDAR_SANS_SOL_PROTOCOLE_20260921.md)
dit explicitement qu'il s'y ajoute. Une directive plus récente, citée dans
la synthèse, met le sans-sol en priorité. La planification peut donc commencer
par sans-sol, mais doit maintenir **deux lignes de qualification distinctes**
(sans sol et brute), avec profil numérique déclaré, avant de dire que le
contrat principal est acquis. Trois trames de la seule séquence 08 ne
qualifient aucune généralisation inter-séquences.

## 3. Audits complémentaires et reçus

Le [rapport de cascade](../../morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md)
et son `RESULTS.json` sont présents à `3595725a`, mais **le C++ de l'appelant,
les répétitions brutes et la capsule de rejeu annoncés ne le sont pas** : ils
renvoient à `MorseHGP_LiDAR_cascade_2026-09-22.zip`, absent du dépôt consulté.
La sonde native [probe.cpp](../../morsehgp3D_v8/audits/lidar_rectangles_20260922/probe.cpp)
ne couvre que la première étude limitée ; elle ne recrée pas toutes les
mesures élargies. Je ne peux pas rapatrier une archive indisponible. Le gain
×2,55–×4,46 du filtrage échantillonné est donc **arithmétiquement lisible dans
le JSON, mais non rejouable depuis Git**. Il conserve les mêmes paires et ne
supprime pas d'atlas ; le seul profil épinglé avant les phases récentes donne
5–8 % environ aux filtres de témoins. Conserver la piste, mais ne pas en faire
le premier pari de baisse globale sans ablation sur l'appel entier à 1 mm.

Le [collectif avant atlas](../../morsehgp3D_v8/audits/CERTIFICATS_COLLECTIFS_AVANT_ATLAS_20260922.md)
a, lui, une [sonde et ses résultats](../../morsehgp3D_v8/audits/collective_edge_20260922/RESULTS.json)
dans Git pour le corpus exact et le microbenchmark. Son bilan LiDAR
« 221 rejets sur 763 arêtes » dépend d'une **extension et de captures hors
dépôt**, sur échantillon stratifié ; q3 réutilise l'atlas, donc les temps q4
évités ne sont pas un gain garanti sur le pipeline. Proposer une ablation
native complète `collectif on/off`, mêmes émissions et compteurs q3/q4,
avant de lui attribuer un gain de mur.

La [politique de reçus v9](../docs/PLAN_V9.md) va dans le bon sens : manifestes
versionnés, données de nuage hors Git, `SHA256SUMS`, lecteur hermétique,
builds non écrasés. La preuve doit garder les **fichiers source de l'appelant
et les répétitions brutes ou une recette déterministe vraiment exécutable** ;
un hash d'une archive absente ne suffit pas. Fixer tôt un schéma explicite
pour `input_sha256`, source/compilateur, commande, statut d'échec, frontières
du chrono, `GPU_executed`, taille d'entrée, sortie et charge de la machine.

J'ai confirmé par la liste des membres que le snapshot versionné
[`gcp_package_r1/snapshot.tar.gz`](../../morsehgp3D_v8/receipts/q34_spatial_20260921/gcp_package_r1/snapshot.tar.gz)
contient trois `RAW.bin` de KITTI et leurs dérivés ; l'archive hôte
[`host_evidence.tar.gz`](../../morsehgp3D_v8/receipts/q34_spatial_20260921/gcp_r1/host_evidence.tar.gz)
contient des sorties OS Login. Le [rapport d'entrée v9](../docs/audit_v8/08_lidar_entrees_g4.md)
les inventorie. Je n'ai ni affiché leurs valeurs personnelles ni évalué les
droits de redistribution. Pour la v9, appliquer immédiatement la règle du
plan : **aucun octet de scan ni sortie OS Login dans les reçus Git**, seulement
empreintes et paramètres, et contrôleur capable d'inspecter les membres des
archives. L'historique v8 demeure une question séparée pour les mainteneurs.

## Suivi au commit `3f0d188f` — reçu publié, qualification R2 encore ouverte

La ligne sans sol 1 mm décrite plus haut est maintenant **versionnée** dans
[`ground_1mm_first`](../../morsehgp3D_v8/receipts/u18_resume_20260922/README.md)
par `3f0d188f`. Son reçu v2 est `passed` en lecture LIVE normale et `-O`
(187 fichiers contrôlés) et son entrée `.u32le` est versionnée en v8, mais
le lecteur dépend encore des builds et dépendances locaux : le reçu ne
constitue pas une archive autonome depuis un clone neuf. La campagne
reste `partial`, avec une répétition, K5/W8 sur une seule trame et aucun
différentiel W1/W8 ; le chiffre de 104,63 s reste **q3/q4 seul**.

Les captures `release_r2` et `sanitize_r2` de la reprise u18 sont en revanche
`status="failed"` dans leurs deux `COMPLETION.json`, même si leurs CTests
ont code 0. Le lecteur `run_u18_resume_checks.py:168–180` cherche des
éléments JUnit `<skipped>`, alors que les vrais `CTEST.xml` marquent les
exclusions par `testcase status="disabled"` **sans** cet enfant : 139 cas,
136 exécutés/3 désactivés en Release, 131/8 sous sanitizers, zéro échec
CTest. Son test `u18_resume_checks_test.py:72–78` fabrique seulement
`status="notrun"` **avec** `<skipped>` et ne juge pas le format réel.
Correctif proposé au constructeur : prendre `status="disabled"` comme
autorité, refuser un désaccord avec tout marqueur `<skipped>` éventuel,
graver un extrait du vrai JUnit, puis refaire une capture **R3** depuis des
sources gelées. Les R2 restent des essais échoués ; la v9 peut porter le
code en le requalifiant, pas hériter d'un PASS inexistant.

La [carte d'héritage v9](../docs/HERITAGE_V7_V8.md) et la
[passation](../PASSATION.md) figent encore la v8 à `a74e90f2` et décrivent
les gardes de fabrique u18, la porte de domaine et `saturate_deep` comme
non commis. Conserver `a74e90f2` pour le **chemin mesuré d'origine**, mais
ajouter `3f0d188f` comme pin distinct des gardes de domaine, du résultat
« fragment exact ou certificat terminal » et de leurs tests. L'option
`saturate_deep=false` du reçu 1 mm ne mesure aucun gain de cette option.
La synthèse et la passation v9 doivent aussi corriger leurs mentions
« non commis/non suivi » ; aucune de ces corrections documentaires ne
promet un résultat FULL ou G4.
