# Pool terminal q2 — qualification et mesures du port produit

14 septembre 2026. `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / implementation_v8_p0 / not_claimed`.
GCP non utilisé ; aucun contrat de tour FULL ni résultat GPU.

## Périmètre et provenance

Cette capture qualifie le [raccord Pool terminal](../../docs/P0_POOL_TERMINAL_Q2.md),
pas le prototype indépendant A fbbecc01 ni le census conjoint précédent.
Un plan sur les mêmes nœuds regroupe les crédits locaux en au plus K bandes ;
seules les survivantes partent en census individuel global, à compte zéro.
Si Pool ne retire aucune paire, le parcours initial est conservé, avec le
coût de préparation compté. Ce repli évite la régression identifiée sur
les rangées ; il ne garantit pas un gain pour les résidus partiellement réduits.

Builds neufs : `build/v8_pool_terminal_20260914` (GCC13.3 Release) et
`build/v8_pool_terminal_sanitize_20260914` (Clang18.1.3 Debug ASan/UBSan).
Boost1.83 est réservé aux juges. Le préflight séparé n'est pas une
qualification complète et ne fournit aucune mesure de cette campagne.
Le [pilote enregistré](qualification/record.py.snapshot), SHA256
`16e0959bc4e29b4682e81b535c8baa784dbaccfbc716a1dc38442115e03ac56a`,
ne reconstruit pas : il capture les commandes, codes et flux, teste les
binaires existants et vérifie les hashes de fermeture des sources/artefacts.

Les deux gates nouvelles comparent les crédits à un tri indépendant et
aux huit coins, puis les supports complets à l'énumération scalaire de
toutes les paires et tous les sites de petits nuages. Le plan ne peut
pas être entièrement vide sans cœur extérieur : la paire croisée la
plus courte survit au filtre local. Cette limite est expliquée et testée,
pas remplacée par un plancher de non-vacuité impossible.

Les contre-fixtures distinguent rang spatial et ID, précrédit interdit,
coquille de 30 points, répétition des préparations, partage de jobs parentaux,
repli sans réduction, réentrance et exceptions sur un support réellement
émis par Pool. Les contre-modèles annoncés ne sont pas des mutations
des binaires produit. Les sources des nouvelles gates sont épinglées :

- Plan : `43a7929bf8e86663d00b350b641b8a114bc4e6969f51b795a3d2b897292e2e32`.
- Raccord : `ebea18debfe58fc392db00b99225ac2a38b36949f91b896490618345d47c12d7`.

## Protocole de performance

Tous les bras utilisent le même front q2 Samples, Shared/Complement/sibling,
ancres Individual, seed3 ; seuls les seuils Pool0 et64 changent.
Pool0 est la référence de **cette révision**, pas un ancien temps recyclé.
Comparer à8k les quatre familles, Kmax5/10 et s8/10/12, puis mesurer
la croissance16k/32k à s8. Les nouvelles comparaisons s10/12 ne portent
donc pas encore sur les deux grandes tailles. Le terrain est synthétique.

Les commandes de mesure sont préfixées par `taskset -c 6` : un thread,
affinité sur le CPU6, mais **hôte partagé et CPU non isolé**. D'autres
qualifications/audits peuvent s'exécuter. Une répétition par configuration,
pas de warmup ni de p95 : les temps sont exploratoires. Les reçus
conservent sources/binaires, entrées, configuration, stdout/stderr et codes.

Le total comprend génération, copie privée/unicité, index, front, Pool,
census, collecte entière, copie/tri/validation/hash du callback et destructions.
Les sous-temps sélectionnés se recouvrent : ne pas ajouter préparation,
callback sélectionné et payload. Les capacités de plans/index ne mesurent
pas le pic RSS ni la mémoire multi-workers/GPU.

Les 23 compteurs Pool comprennent F, propositions/certifications et
bandes réellement développées. Les trois compteurs passthrough sont
inclus dans les plans préparés, mais ne produisent aucune racine Pairwise
Pool. Le compteur `pair_roots` vaut résidu sélectionné moins masse passthrough.
Les visites géométriques, tâches, tests de frère, opérations structurelles
et travail du front sont distincts ; ne pas les additionner comme des
opérations homogènes. Le critère de comptage final et toutes les incidences
doivent rester identiques entre les deux politiques.

## Qualification close

- [Release](qualification/release_4eg2giks/RESULT.json) : 49 CTests PASS,
  32 commandes enregistrées, sources et artefacts inchangés.
- [Clang ASan/UBSan](qualification/sanitize_pp79b6c7/RESULT.json) : mêmes
  49 CTests et 32 commandes PASS, fuites non désactivées, fermeture conforme.
- [Lecteurs](qualification/readers_qv43z4uf/RESULT.json) : normal et `-O`
  identiques, 80 mesures / 80 configurations / deux campagnes, contrôle
  documentaire PASS et fermeture des empreintes conforme.
- [48 appariements à8k](paired_8k/COMPLETION.json) et
  [32 mesures à16k/32k](growth_s8/COMPLETION.json) : tous les essais
  terminés, aucun essai perdu ni échec dans ces deux campagnes.

Les deux builds de qualification sont maintenant épinglés. La sonde
Release mesurée porte SHA256
`67ca5595f458c9e26fef3cebef3a85bacd78df18a1128256b7fbcc759d5793da`.
Les manifestes conservent le commit de départ et le worktree modifié,
ainsi que les hashes individuels : le commit de départ seul ne désigne
donc pas le port mesuré. Des commits d'audit indépendants sont intervenus
pendant la capture sans changer les sources qualifiées.

La gate du plan vérifie 2 059 plans, 8 190 paires et 81 160 évaluations
indépendantes de coins. La gate intégrée fait 3 088 appels sur 22 nuages,
compare 183 584 supports entiers et exerce réellement le seuil64, les
replis, une réentrance et trois exceptions/reprises sur la route Pool.
La gate des reçus passe en normal/−O : 480 lignes de sondes bornées,
152 mutants et sept rejets CLI Pool. Ces 480 lignes de tests ne sont
pas les 80 mesures de performance ci-dessus. L'oracle exhaustif est
borné aux petites fixtures ; aux grandes tailles, les modes partagent
les mêmes hashes d'entrée et empreintes de tous les supports, pas une
nouvelle comparaison exhaustive de toutes les paires avec l'oracle.

## Résultat principal : coût q2 entier sur les amas

Temps englobants mono-thread, secondes, s8. Les bras sont appariés sur
la même révision et incluent le callback décrit plus haut.

| Kmax | n | Sans Pool | Pool64 | Rapport sans/avec |
| --- | ---: | ---: | ---: | ---: |
| 5 | 8 000 | 8,109 | 1,225 | 6,62 |
| 5 | 16 000 | 30,802 | 3,202 | 9,62 |
| 5 | 32 000 | 121,708 | 7,083 | 17,18 |
| 10 | 8 000 | 13,412 | 3,589 | 3,74 |
| 10 | 16 000 | 47,179 | 7,614 | 6,20 |
| 10 | 32 000 | 184,306 | 19,180 | 9,61 |

À K10/32k, les candidates census passent de460,079 à12,184 millions,
et les visites géométriques de12,360 milliards à648,208 millions.
Le front est inchangé. Pool prépare28 plans, F=224000, avec102336 paires
survivantes sélectionnées ; les autres candidates viennent des petits
rectangles non sélectionnés. F vaut56k/112k/224k aux trois tailles,
soit7n sur **cette famille**, pas un théorème sur tous les nuages.

La préparation K10/32k prend16,327 ms ; l'ensemble des traitements
sélectionnés, préparation et callbacks compris, prend197,745 ms,
environ1,03 % des19,180 s totales. Ces temps sont imbriqués.
Micro-optimiser ce seul résidu ne traitera donc pas le poste restant :
front/proposeur et census des nombreux petits rectangles sont prioritaires.

## Croissance observée, sans extrapolation asymptotique

Pool64/s8 ; `p` est l'exposant local des **visites census** :
`log2(visites(2n)/visites(n))`. Un carré donnerait p=2.

| Famille | Kmax | Temps8k (s) | Temps16k (s) | Temps32k (s) | p8→16 | p16→32 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| uniforme | 5 | 1,936 | 4,826 | 9,663 | 1,264 | 1,029 |
| uniforme | 10 | 5,141 | 13,394 | 28,870 | 1,267 | 1,276 |
| terrain | 5 | 0,400 | 0,852 | 1,745 | 1,160 | 1,086 |
| terrain | 10 | 0,942 | 1,948 | 4,164 | 1,064 | 1,152 |
| amas | 5 | 1,225 | 3,202 | 7,083 | 1,433 | 1,202 |
| amas | 10 | 3,589 | 7,614 | 19,180 | 1,565 | 1,434 |
| rangées | 5 | 0,122 | 0,252 | 0,518 | 1,049 | 1,047 |
| rangées | 10 | 0,228 | 0,462 | 0,933 | 1,050 | 1,047 |

Les quatre familles ont, sur ces deux doublements, des exposants inférieurs
à2 pour les visites census, candidates, produits visités par le front et
descentes du proposeur. Sur les amas/K10, les visites font×2,958/×2,701
avec Pool, contre×4,106/×4,229 sans Pool. À K5, elles font×2,700/×2,301
au lieu de×4,178/×4,231. **Le comportement quasi quadratique auparavant
mesuré sur les amas est corrigé sur ces tailles**, pas prouvé absent
pour tout n, toute orientation ou tout nuage. La borne locale O(KF)
ne borne ni F global ni l'aval ; P0 général reste ouvert.

Quelques volumes K10 (millions, triplets8k/16k/32k) permettent de ne pas
confondre durée et travail ; les colonnes ne s'additionnent pas.

| Famille | Candidates census | Visites census | Produits du front | Descentes du proposeur |
| --- | --- | --- | --- | --- |
| uniforme | 3,194 / 7,347 / 17,325 | 171,895 / 413,553 / 1001,202 | 5,359 / 12,515 / 28,018 | 69,438 / 174,781 / 419,716 |
| terrain | 0,936 / 1,887 / 4,017 | 20,473 / 42,798 / 95,129 | 1,044 / 2,178 / 4,721 | 13,331 / 29,965 / 69,721 |
| amas | 1,744 / 4,786 / 12,184 | 81,113 / 239,954 / 648,208 | 3,036 / 8,142 / 19,909 | 39,427 / 113,907 / 298,161 |
| rangées | 16,091 / 32,183 / 64,366 | 5,742 / 11,886 / 24,567 | 0,159 / 0,319 / 0,638 | 1,859 / 4,008 / 8,593 |

Uniforme et terrain ne sélectionnent aucun plan, à aucune des tailles
mesurées : le travail est exactement celui du bras sans Pool. Les variations
de temps entre leurs deux bras ne sont donc pas une accélération algorithmique.
Sur rangées8k, le repli préserve16M paires regroupées ; il ne crée aucune
racine Pool. Aux tailles16k/32k, certains autres blocs sont filtrés :
32M/96M paires retirées à K10,32M/64M gardées en parcours partagé, et
seulement110/330 racines individuelles Pool. Une masse candidate élevée
ne signifie donc pas qu'elle a été parcourue paire par paire.

## Comparaison s8/10/12

Temps englobants Pool64/K10 à8k, secondes ; tous les supports ont la
même empreinte canonique entre séparations.

| Famille | s8 | s10 | s12 |
| --- | ---: | ---: | ---: |
| uniforme | 5,141 | 5,225 | 5,337 |
| terrain | 0,942 | 1,059 | 1,034 |
| amas | 3,589 | 2,888 | 3,227 |
| rangées | 0,228 | 0,233 | 0,279 |

Sur amas, les trois s sélectionnent les mêmes28 plans et11329 survivantes
Pool ; s10/12 diminuent un peu les candidates hors Pool mais augmentent
les produits/descentes du front. Les écarts de temps, avec une répétition
et un hôte partagé, ne permettent pas d'élire un s universel ni un optimum.
K5 est aussi mesuré dans les reçus bruts ; aucune mesure nouvelle s10/12
à16k/32k n'est annoncée.

## Relecture et suite

Le [snapshot de l'analyseur](qualification/analyze.py.snapshot), SHA256
`dc2f2bf503ac8133ff0a3e0a10eb222bcc9183d8cf57b1dbfbaebe3785370e66`,
conserve les calculs de rapports et l'extraction des lignes. C'est un
diagnostic en lecture seule, pas un substitut aux lecteurs de qualification.
Replacer les snapshots à leurs chemins de capture avant reproduction.

```bash
python3 -B morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/q2_terminal_pool_20260914 --summary
python3 -B -O morsehgp3D_v8/bench/run_wspd_q2_matrix.py check morsehgp3D_v8/receipts/q2_terminal_pool_20260914 --summary
python3 -B build/v8_pool_terminal_20260914/analyze.py --summary-only
```

Décision : conserver Pool en option, préparer le partage de sous-arbres
du front avec moteur/collecteur privés et plans parentaux possédés.
L'[audit A](../../audits/q2_small_roots_20260914/README.md) réfute un gain
stable du retour à Global pour les seules racines singleton ; cette
micro-variante n'est pas portée. Ses obligations de coquille tangente
restent une proposition séparée, pas un gain hérité.
q3/q4, catalogue canonique, FULL, multi-CPU, GPU et contrats G4 demeurent
hors du périmètre de cette capture. Aucun nouveau test produit50k/LiDAR
ni grand nuage G4 ; GCP non utilisé.
