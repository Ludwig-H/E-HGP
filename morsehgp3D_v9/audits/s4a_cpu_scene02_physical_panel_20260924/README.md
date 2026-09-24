# Panneau CPU S3/S4a : secteurs physiques de 08/000200 sans sol

**Campagne complète au 24 septembre 2026 : 42/42 sorties de bras validées,
21 paires S3/S4a égales sur sorties et ledger commun.** Les 44 tentatives
comprennent six imports historiques, 35 nouveaux murs sous la garde CPU et
trois tentatives valides sous contention. Parmi les 42 bras sélectionnés,
un garde un mur censuré (`quarter_full/s3`) ; le rejeu accepté de
`full_full/s3` a conservé l'ancien essai contended sans l'utiliser comme
chrono. `SUMMARY.json` publie les effectifs et pentes finis, les ratios de
découpe et les classes temporelles ; `SHA256SUMS` ferme l'inventaire.

Ce panneau vise à mesurer la tour K1..10 à `s8/W8` sur les sept secteurs du capteur
(trame entière, deux moitiés, quatre quarts) et trois densités globales
emboîtées `1/4 ⊂ 1/2 ⊂ 1`. Le masque sans sol est celui de la trame entière.
Les secteurs utilisent les signes des coordonnées float32 d'origine, avant
la grille commune 1 mm/u18 ; les points transmis au moteur conservent leurs
coordonnées de cette grille et leur ordre dans la trame entière. Le rang
`splitmix64(ID_original_grille XOR 7d1c9a5eb3f24680)` choisit d'abord les
densités sur les 45 845 sites retenus, puis chaque secteur est intersecté.

Les deux bras de chaque entrée utilisent le même binaire CPU v20, le même
filtre et les mêmes certificats S3 ; S4a active seulement
`q34_batch_q3=1`. Le binaire SHA-256 `eea3040c…` a été reconstruit depuis le
snapshot Git immuable `7ceadffa…` sous `/tmp`, sans compilation ni édition
du worktree du constructeur. [BUILD_PROVENANCE.json](BUILD_PROVENANCE.json)
garde les commandes, le compilateur, les objets Git et les empreintes.

Le quart physique chaud `x≥0,y<0` possède déjà trois entrées et six bras
S3/S4a au même seed et au même SHA de binaire. Le runner vérifie leurs
octets, commandes, empreintes, sorties et charge hôte avant de les importer ;
les six stdout/stderr sont copiés sans modifier le reçu antérieur. Leur garde
historique vérifiait seulement `load_before/load_after≤8` ; elle n'avait pas
de mesure des cœurs externes et leurs murs ne sont pas des chronos sous la
nouvelle garde. Toute
nouvelle tentative conserve sa propre sortie et son motif d'échec éventuel
dans `ATTEMPTS.jsonl`. Le runner refuse une entrée ou un binaire modifié
avant une mesure ; le lecteur recoupe les sorties, les dix ordres, Euler, les identités du ledger et les
digests tour/catalogue égaux entre les bras. Un nouveau chrono sous garde exige
une charge 1 minute au plus 8 avant la mesure et au plus 2,5
cœur occupé par les autres processus, échantillonné avant puis compté pendant
le cas via `/proc/stat` moins les CPU·s du processus fils ; la charge après
la mesure est diagnostique, car le W8 lui-même peut la faire monter. Une
sortie valide sous contention fournit ses compteurs déterministes et ses
CPU·s sans qualifier son mur. Les pentes du mur interne restent indicatives,
et le résumé expose la classe temporelle de chaque bras. Le résumé calcule
les pentes avec les effectifs réels de chaque couple de nuages.

`MANIFEST.json` et `inputs/` contiennent les 21 payloads régénérables et
leurs IDs originaux, retours bruts et rangs de la trame entière.
Le SHA-256 du binaire est contrôlé avant et après chaque exécution.
`run_panel.py verify` reconstruit les entrées depuis les reçus v8 et relit
les SHA des tentatives et sorties sans exécuter HGP ni exiger que le binaire
temporaire existe encore. `SUMMARY.json` et `SHA256SUMS` ont été créés
après validation et appariement des 42 sorties. Pour relire le panneau avec un
binaire reproduisant le SHA épinglé :

```sh
MHGP9_PANEL_BINARY=<chemin-du-binaire-épinglé> python3 -B \
  morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/run_panel.py run
python3 -B morsehgp3D_v9/audits/s4a_cpu_scene02_physical_panel_20260924/run_panel.py verify
```

`run --wait-for-host=1800` attend au plus 30 minutes cumulées quand la
charge avant le cas dépasse la garde, sans ouvrir de tentative ni écraser
une sortie. Une reprise relit d'abord toutes les tentatives existantes.

`SUMMARY.json` donne au total 18 pentes spatiales avec les effectifs réels
(six par densité) et neuf découpes parent→deux enfants (trois par densité) avec
`R=Σtravail(enfants)/travail(parent)`,
`B=Σ(n_enfant/n_parent)^2` et `R/B`, séparément pour S3 et S4a :
`core_sites`, `expanded_pairs`, CPU·s, puis mur externe uniquement lorsque
les trois bras ont passé la nouvelle garde. Par exemple, à densité pleine,
trame entière→deux moitiés donne `B=0,5075`,
`R_core_sites=0,8989`, `R_expanded_pairs=0,7807` et
`R_CPU=0,9160/0,9085` (S3/S4a). Ces R/B sont des diagnostics de neuf
découpes finies, pas des bornes de croissance.

Les murs sous garde restent descriptifs : `full_full/s3` accepté a pris
88,365 s avec 0,98 cœur externe pendant l'essai, contre 106,703 s et
1,98 cœur externe pour `full_full/s4a`. Les CPU·s de chaîne restent
570,555/575,130 pour les essais sélectionnés ; ces deux exécutions ne sont pas des répétitions
temporellement appariées et leur écart de mur ne mesure pas un effet
causal de S4a. Les six murs importés n'ont que la garde historique de
charge et ne participent pas aux ratios de mur qualifiés.

Les coupes et densités sont des problèmes HGP distincts, utilisés pour un
diagnostic fini de croissance. Un statut `complete_relative` valide la
cohérence du catalogue recoupé, sans prouver l'absence de clés omises.
Ce panneau CPU ne mesure ni GPU/G4, ni la trame brute avec sol, ni plusieurs
séquences et ne qualifie aucun contrat de temps ou de borne asymptotique.
