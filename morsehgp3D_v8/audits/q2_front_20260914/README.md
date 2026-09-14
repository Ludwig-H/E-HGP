# LiDAR : juger ensemble le front et le census q2

14 septembre 2026, auditeur A. `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Sources constructeur capturées séparément dans
[r1_BUILD.json](r1_BUILD.json) et [l’archive](r1_sources.zip), puis
[comparées au commit publié f7edd646](SOURCE_PUBLICATION.json) : les
seize fichiers produit/test sont identiques. Aucun chronométrage rejoué.

## Résultat et portée

Les 15 appels clos couvrent six groupes d’entrées physiques et onze
configurations. Le pilote LiDAR confirme l’intérêt du filtre quand son
coût aval est payé. Sur 8 000 sites du scan 000000, Kmax=10 et s=8, les
deux ordres d’exécution donnent les intervalles observés suivants :

| Front / census | Front + census + collecte/callback, secondes | Paires candidates | Visites de comptage |
| --- | ---: | ---: | ---: |
| Samples / Shared | 1,668–2,128 | 1 956 115 | 59 226 897 |
| Samples / Pairwise | 1,754–2,521 | 1 956 115 | 102 372 640 |
| Pure / Shared | 12,233–16,463 | 31 996 000 | 694 940 756 |
| Pure / Pairwise | 16,299–21,079 | 31 996 000 | 1 559 287 316 |

Les quatre modes donnent le même digest de 160 237 supports, 710 926
IDs intérieurs et 323 968 IDs de coquille. Les différences de compteurs
montrent deux effets distincts : le filtre retire du travail au census,
puis Shared partage une partie du travail des paires restantes. Dans
le mode filtré, ses visites baissent de 42,1 %, contre 4,9–15,6 % pour les
temps observés. Une visite de groupe et une visite de paire ne
coûtent pas la même chose. La collecte paie les mêmes 9 256 963 visites
dans les quatre modes ; elle n’explique pas les différences de comptage.
Samples/Shared est ici 7,3–7,7 fois plus rapide que Pure/Shared, selon
l’ordre de mesure. Les compteurs répétés sont identiques, mais la charge
de la machine varie : ces deux répétitions ne sont pas un intervalle
de confiance. La [synthèse](SUMMARY.json) conserve les agrégats et les pins
des six campagnes brutes.

La montée en taille porte sur Samples/Shared, même scan, même Kmax/s :

| Sites | Temps intégré, secondes | Supports émis | Visites de comptage | Tâches de requête |
| --- | ---: | ---: | ---: | ---: |
| 8 000 | 1,668–2,128 | 160 237 | 59 226 897 | 2 834 581 |
| 16 000 | 3,733 | 326 754 | 135 805 996 | 6 383 361 |
| 32 000 | 7,582 | 660 726 | 280 997 181 | 13 458 384 |
| 50 000 | 13,778 | 1 040 133 | 579 839 306 | 29 917 359 |

À 50k, le total avec lecture, propriétaire, index et destructions est
13,817 s. La collecte/callback vaut 1,188 s, soit 8,6 % du temps intégré.
Les 3,83 millions de démarrages racine, 579,8 millions de visites de
comptage et 123,2 millions de pas du proposeur désignent les coûts à
réduire en premier. Shared augmente aussi le nombre de tâches par rapport
à Pairwise sur le même résidu : leur nombre doit rester mesuré à côté
des visites. Aucune sortie n’est tronquée pour obtenir ces temps.

Sur les deux autres scans isolés à 8k, Samples/Shared termine en 1,386 s
(000100, 156 422 supports) et 1,518 s (000200, 153 796 supports). Ces
contrôles complètent le premier scan sans constituer un appariement des
quatre modes sur toutes les scènes.

À 8k sur 000000, s=10 et s=12 laissent respectivement 1 887 124 et
1 845 352 paires, mais paient 59,17 et 59,64 millions de visites Z,
contre 59,23 millions à s=8. Les descentes du proposeur passent de 16,90
à 19,74 puis 22,10 millions. Les temps observés 1,705 et 1,737 s restent
dans la variabilité du pilote s8. La baisse du résidu n’apporte donc pas
ici de baisse nette du travail total ; conserver s8 comme comparaison
appariée, sans en faire un optimum universel.

Ce résultat concerne le raccord **q2 seul**, sur un cœur logique local.
Le filtre générique n’impose ni points alignés ni colonnes exactes.
L’[audit précédent](../lidar08_20260914/README.md) mesurait un front
q2/q3/q4 sans consommateur : ses temps ne servent pas de dénominateur ici.
Les comptes et temps ci-dessus viennent des mêmes sources, du même
masque q2 et des mêmes octets d’entrée. Ce n’est ni une tour FULL, ni une
mesure G4, ni une preuve de croissance générale.

## Sources, entrées et contrôles

Le raccord lit directement les nœuds B de l’index global immuable.
Le facteur le plus petit fournit les ancres via `spatial_order`, qui
reste distinct des IDs originaux. B et Z partagent leur stockage, mais
pas leur état de parcours. Chaque nouvelle ancre part de `(count=0,Z=0)` ;
après division B, compte et curseur sont transmis ensemble. Les données
collectées sont empruntées uniquement pendant le callback. La coquille
complète, les différentes incidences d’une même clé et les effets partiels
d’un callback en exception restent dans le contrat.

Les [entrées LiDAR](../lidar08_20260914/INPUTS.json) sont réutilisées à
l’identique : grille isotrope 2 cm, sites u16 uniques, prélèvements emboîtés
de priorité déterministe. La préparation transforme l’objet brut ; cette
limite reste explicite. Aucune nouvelle acquisition ni accumulation de
scans ne participe à cette capture.

Le [gate du snapshot](r1_BUILD.json) passe avec `-O2 -DNDEBUG`, C++20 et
les avertissements stricts : 1 255 exécutions intégrées, 26 nuages, 1 749
paires et 28 869 tests ponctuels d’oracle, quatre exceptions de callback.
Les huit mutants annoncés sont des modèles de sorties altérées, pas
des injections dans le moteur. Pas de nouvelle qualification sanitizer.

L’[adaptateur](lidar_q2_probe.cpp) vérifie les clés, les bornes d’IDs,
les doublons, la séparation intérieur/coquille et la présence des ancres.
Son digest canonique trie chaque payload et conserve les incidences ;
copie, tri, vérifications et hash sont réellement payés dans le callback.
Le [contrôle indépendant](PROBE_CHECKS.json) confronte les quatre modes
à un calcul scalaire sur les 32 premiers sites : 496 paires, 15 872 tests
ponctuels, 338 supports retenus. Une concordance de checksums ne remplace
pas l’oracle géométrique qui compare les payloads complets dans le gate.

Le temps `pipeline_total_ms` englobe front, comptage, collecte, callback
et destruction des buffers privés du moteur. `front_and_count_ms` est
le résidu de ce temps après `payload_ms`, pas un temps de census isolé.
`total_ms` ajoute lecture du fichier u16, propriétaire, index, contrôles
finaux et destructions de l’adaptateur. La transformation des retours
float32 précède ces chronomètres. Les capacités rapportées ne sont pas
un pic RSS. La charge ambiante est consignée ; les comparaisons de temps
restent locales à cette machine partagée.

## Prolongement vers deux groupes de requêtes

Les [preuves §9–9.1](../P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
couvrent déjà A×B×Z et la division de A ou de B. La nouvelle question
constructeur demande leur port, pas une nouvelle preuve du curseur.
Par axe, les quatre coins `(a,b)` donnent `C=a+b`, `D=(b−a)²`, puis
`4H=D−(2z−C)²`. Les 24 constantes u32 occupent 96 octets ; les calculs
s’effectuent en i64. Le minimum et le maximum continus proviennent du
point de 2Z respectivement le plus éloigné et le plus proche de C.

La file peut porter seulement trois IDs de nœuds et un compte, avec index
et seuil implicites dans un batch homogène. Les constantes se préparent
après dépilage ; une suspension exige de les conserver ou de payer leur
recalcul. Diviser A/B conserve le préfixe Z et son compte pour des produits
disjoints. Diviser Z en tâches indépendantes exigerait une autre réduction :
additionner leurs comptes hérités compterait plusieurs fois le préfixe.

Dès que A est singleton, le moteur WSPD peut reprendre directement
`shared_task(order[A.first], B_node, count, cursor)`, sans `cover`,
`root_start` ni arbre B local. L’échange avec B singleton exige une
convention d’orientation du support. Aucune exclusion globale de A∪B :
sur A={0,1}, B={10}, le site 1 est intérieur pour (0,10) et sur la
coquille de (1,10). Ce format retire l’énumération initiale obligatoire
des ancres, sans prouver un gain de travail total ou une borne globale.

## Ordre Z : une contre-fixture et un état à exclusion explicite

La demande constructeur sur les subdivisions prématurées possède une
[fixture minimale permanente](order_fixture.py), de pleine dimension :
`a=(0,0,0)`, `B={(100,0,0),(102,1,0)}`, `z=(50,2,1)`, Kmax=1.
Le déterminant affine vaut 100 et le témoin commun z donne H=2495/2597.
Après réflexion exacte `x→102−x`, les distances et les profondeurs sont
inchangées. Le miroir du découpage midpoint et du census DFS donne pourtant
4→8 visites Z, 1→3 tâches et 0→1 division B. Dans le cas réfléchi,
Z atteint B avant z ; l’indécision et les diagonales égales déclenchent
le partage de B. Les deux paires finissent rejetées dans les deux cas.

Les [contrôles normal/−O](ORDER_CHECKS.json) vérifient cette topologie,
les échappements et les comptes contre les puissances rationnelles. Il
s’agit d’un miroir Python, sans mesure de temps ni exécution C++ de cette
fixture. La réflexion reconstruit un index neuf : aucun curseur actif
n’est réordonné. **MidpointSamples trouve déjà ce témoin sur ces quatre
sites**, ce que le juge vérifie ; la fixture explique un mécanisme du
census isolé/front Pure, pas à elle seule les mesures Samples sur amas.

Pour la prochaine expérience, le certificat autonome du frère proposé
par le constructeur dans [son contrat](../../docs/P0_FRONT_ET_CENSUS_Q2.md)
est plus simple : lorsque le frère
fournit déjà Kmax témoins stricts,
rejeter sans ajouter ce crédit au compte hérité. Il ne demande aucun
nouvel état d’exclusion, même si le frère recouvre le préfixe consommé.
L’option suivante répond seulement à la conservation d’un crédit partiel ;
elle ne doit pas retarder la mesure du certificat saturant.

Une extension sûre conserve un seul bloc E certifié strictement intérieur
pour tout A×B. Si son cardinal atteint Kmax, rejeter immédiatement ; sinon
son cardinal est inférieur à Kmax≤10. Le bloc est alors crédité avant
le DFS, mais ses rangs doivent être exclus du compte ultérieur. Avec P
le préfixe de feuilles déjà consommé, le nouvel invariant est
`c = |E| + nombre d’intérieurs dans P privé de E`, saturé au seuil.
Consommer un bloc intérieur Z ajoute `|Z|−|Z∩E|` ; l’intersection de deux
plages de rangs coûte O(1). Les enfants A/B héritent du même E certifié.

L’état devient `(A,B,c,curseur,E)`, avec l’identité de l’index. Dès que
le préfixe couvre E entièrement, E peut disparaître : c est alors un
compte de préfixe ordinaire et le repli existant redevient applicable.
**Avant ce point, passer seulement c et le curseur à l’ancien parcours
serait incorrect.** Ce protocole n’autorise pas à précharger le compte
de l’API actuelle, dont le contrat reste zéro à la racine.

Le modèle teste un E singleton fourni, dans les deux orientations et à
Kmax=1/2/3 ; les décisions restent identiques au census sans crédit.
À Kmax=1, une certification suffit à rejeter le produit sans DFS ; le
coût de trouver E reste absent de cette comparaison. À Kmax=2, le mutant
qui omet l’exclusion rejette à tort un support et échoue dans les deux
orientations. Le coût des vérifications d’exclusion est compté séparément.
Le choix des blocs, leur transfert entre front et census, la collecte
complète et le gain sur données réelles restent à implémenter et mesurer.

## Reproduction

Les anciens builds et jeux préparés restent épinglés. Pour construire
une **nouvelle capture** du worktree courant sous ce dossier, utiliser
un nouveau nom avec `build_snapshot.py --name`. Cela ne rejoue pas les
sources r1 : leur reconstruction part de `r1_sources.zip` et des commandes
exactes enregistrées dans `r1_BUILD.json`.

```bash
python3 -B morsehgp3D_v8/audits/q2_front_20260914/measure.py --validate morsehgp3D_v8/audits/q2_front_20260914/campaign_pilot
python3 -B -O morsehgp3D_v8/audits/q2_front_20260914/measure.py --validate morsehgp3D_v8/audits/q2_front_20260914/campaign_pilot
```

Les données et binaires doivent conserver leurs pins pour valider une
capture. Aucun reçu existant n’est écrasé par le runner.

Le pilote conserve son [runner r1](measure_r1.py), épinglé lors de ses
quatre appels réussis. La contrelecture a ensuite fait corriger un défaut
du harnais : une exception de lancement ou une interruption pouvait faire
perdre la ligne de la tentative courante. Le [runner actuel](measure.py)
la sérialise dans un `finally`, puis propage l’erreur. Les
[sept contrôles normal/−O](RUNNER_GUARDS.json) conservent ces cas et deux
rejets d’autorité ; les reçus du pilote restent inchangés. Cette ancienne
source est une preuve consommée par le lecteur, pas une alternative active.
Les contrôles finaux des six campagnes, de la documentation et du registre
sont conservés dans [VALIDATION.json](VALIDATION.json).
