# Contre-audit du harnais réel et du raccord CUDA par vagues

27 septembre 2026, suite de `70168cc3b`, contrelecture close après publication
du protocole portable `33c1d28d7`, dans le worktree de reprise.
Cadre : `exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, `audit_harnais_et_raccord_S2`, `not_claimed`.
Aucun moteur, source gelée ou reçu modifié par cet audit. GCP non utilisé
par cet auditeur ; les lancements éventuels appartiennent au contrôleur
principal et exigent leurs propres reçus de fermeture.

Cette note complète, sans la réécrire, la
[revue précédente](../b_parallel_and_waves_review_20260927/README.md).
Les sections de conception ne valent pas résultat d'une capture ouverte.

## Harnais CPU réel : ce qui est effectivement comparé

Le nouveau [probe.cpp](../b_q34_waves_real_20260927/probe.cpp) construit
un index et un front partagés, puis transmet **le même vecteur** de
rectangles, dans le même ordre, au consommateur par vagues et au batch
natif. Le front `MidpointSamples` est payé séparément ; chaque bras paie
ensuite son propre filtre rectangle. Pas de nouveau `Plan` historique
ni de tableau P/E caché dans le chemin candidat.

Le candidat passe en premier, le natif en second. Tous les workers sont
W1 dans ce harnais, y compris la préparation de l'arène. Une observation
de cet ordre fixe n'est pas une preuve de gain stable ni une comparaison
W1/W4. L'égalité vérifiée porte sur les rectangles filtrés, chaque
survivante ordonnée avec son masque, P et les rejets par voie ; les visites
ponctuelles n'ont pas à être égales.

Les identités publiées sont `S3=P3−rejets_natifs3=E3−rejets_ponctuels3`
et leur analogue q4. Les unions P/E/S restent séparées de la somme de
leurs voies. `Praw` inclut les produits des rectangles fermés ; il peut
être supérieur à P. Les compteurs de préparation F, classes, bandes et
segments restent présents à côté d'E, afin de ne pas déplacer le coût
hors du bilan.

## Chronos et mémoire : limites de lecture

Le code paie successivement préparation, construction/consommation du
curseur, `finish` avec tri et conversion, puis destruction du curseur et
du propriétaire `Prepared`. Les deux `reset` ont réellement lieu avant
la fin du total candidat : aucun propriétaire partagé supplémentaire
ne retarde cette libération. L'index et le nuage restent partagés pour
le juge natif qui suit.

La sortie S reste volontairement vivante à la fin du chrono opérateur,
dans les deux bras. Sa destruction, celle du natif et celles des entrées
sont payées dans `final_output_input_cleanup` et dans le total du harnais,
pas dans le chrono individuel natif. Le total n'inclut pas l'impression
JSON ni la destruction finale des petits objets de diagnostic.

`candidate_total` contient aussi les lectures RSS, les insertions de
compteurs et un scan diagnostic supplémentaire de S. Le champ
`candidate_observation_overhead` publie cette différence avec la somme
des quatre phases ; ne pas présenter ce total comme quatre phases sans
instrumentation. `candidate_input_to_S2` est un véritable intervalle
depuis la lecture/génération d'entrée, non une somme choisie de sous-temps.

Les lectures VmRSS sont des observations aux frontières, pas des pics
isolés. `ru_maxrss` est monotone sur tout le processus ; le passage natif
se produit alors que la sortie candidate existe encore. Son incrément
ne mesure donc pas sa mémoire propre. Les capacités du curseur et de
`Prepared` ne sont pas non plus le RSS ou le pic complet de préparation.

Deux erreurs de forme ont été signalées avant gel : accès à
`front.product_visits` au lieu de `front.work.product_visits`, et lignes
susceptibles de déclencher `-Wmisleading-indentation`. Il ne s'agissait
pas d'une divergence géométrique constatée dans un test.

## Lecteurs CPU et coupes capteur

[run.py](../b_q34_waves_real_20260927/run.py) requalifie le binaire frais
avec la gate héritée explicitement et trois petits appels du nouveau
harnais par build. [measure.py](../b_q34_waves_real_20260927/measure.py)
refuse toute campagne tant que cette qualification n'est pas close et
relue. Sources, archives, binaires, commandes et entrées sont liés par
leurs empreintes ; ce n'est pas une qualification héritée implicitement.

Le groupe initial contient ng00 entier et uniforme8k/16k/32k. Les six
moitiés/quarts forment un groupe distinct, après décision de poursuivre.
Le helper d'entrée vérifie le masque appliqué à la trame entière, les
plans capteur x=0/y=0, l'origine commune, les correspondances d'IDs et
la partition complète/disjointe des sept morceaux. Il ne qualifie pas
la justesse sémantique du masque de sol.

Les trois lecteurs LIVE ont été exécutés indépendamment en modes normal
et `-O` : six PASS, pour les 21 commandes de qualification, cinq initiales
et sept des coupes. Les sources et reçus n'ont pas été modifiés. Les
[résultats clos](../b_q34_waves_real_20260927/README.md) couvrent donc
bien ces dix mesures, pas seulement les petites fixtures.

Une pente doit utiliser les effectifs réellement lus et les quantités
correspondantes, pas supposer que chaque coupe divise n exactement par
deux. Ici : entier39 885 ; moitiés24 591/15 294 ; quarts11 536/13 055/
8 225/7 069. Le manifeste historique garde trois changements de quadrant
brut→grille, un selon x et deux selon y ; les frontières sont celles de
la grille1mm, pas exactement celles des coordonnées float32 brutes.

Les six exposants locaux `log(Wparent/Wenfant)/log(nparent/nenfant)` ont
été recalculés depuis les sorties complètes. E, S, visites ponctuelles,
front, rectangles, F et temps candidat donnent chacun des exposants
inférieurs à deux dans ces six relations. **Exception explicite :** les
tests de coins Pool donnent2,2297589 entre le quart x<0,y≥0 et sa moitié
x<0 ; F donne1,9081100 dans cette même relation. Le bilan ne permet donc
pas d'écrire « tous les compteurs sont sous-quadratiques ». Les pentes
spatiales mélangent taille et hétérogénéité géométrique ; une seule trame
ne fournit ni borne asymptotique, ni qualification de plusieurs scènes.
Une série uniforme n'est pas une courbe LiDAR. Ni ce test S2, ni une
réduction d'E ne qualifient la tour FULL sous-quadratique.

## Résultat réel et obstacle prioritaire de port

Sur ng00, le candidat passe de47,691s natif à31,048s, pour exactement
2 043 612 survivantes. Il reste une seule observation candidat→natif,
sans inversion d'ordre : pas une estimation statistique de gain stable.
Sur uniforme8k/16k/32k, il régresse au contraire d'environ7–8%. Les
principaux compteurs y croissent sous le quadruplement aux doublements,
ce qui ne supprime ni la régression ni le coût absolu.

Le poste décisif n'est pas le tri : ng00 paie14,133s de préparation,
16,833s de consommation et66,155ms de tri/conversion. Dans
[`Prepared::construct`](../b_q34_arena_waves_20260927/waves.hpp), la
boucle des rectangles et son filtre affine sont **sériels** ; `workers`
est transmis seulement à `Arena::build`. Ainsi, le futur appel GPU
avec préparationW4 ne signifie pas que ces14,133s ont été parallélisées.
Ce coût inclut229 928 699 visites rectangle ; il ne doit pas être remplacé
dans un raisonnement par les anciens temps d'arène seule, ni comparé
directement à des chronos d'un autre hôte.

L'arène ne résume pas davantage la mémoire : `Prepared` retient187,227Mo
sur ng00, dont29,328Mo d'arène. Les3 133 819 rectangles et1 356 059
segments coûtent aussi ; le front original reste vivant séparément.
Sur uniforme32k, `Prepared` atteint350,184Mo pour une arène de0,136Mo.
Le curseur supprime les tableaux temporaires P/E, pas les métadonnées R,
les sorties S, ni le coût de leur production. Il faut raccorder le filtre
rectangle GPU existant et les seuls rectangles utiles, en préservant les
ordinals globaux, avant d'espérer un bénéfice net de chaîne. La sonde
CUDA actuelle est d'abord une qualification du transport/décodage exact,
pas une activation industrielle économisant déjà toute la préparation.

## Nouveau décodeur portable et source CUDA

Le [prototype](../b_q34_cuda_waves_20260927/probe.cpp) transporte un
snapshot privé du propriétaire qualifié. L'unité CUDA voit seulement
une API C++17 de tableaux POD ; la construction native et la conversion
restent en C++20. Chaque segment est une bande ou un fallback entier.
Les crédits décident les voies de chaque paire, pas un masque uniforme6.

La revue a demandé un plancher de **réduction effective par Pool** :
`e.mask!=6` pouvait être satisfait par une voie déjà absente du rectangle
ou par K2. Le gate gelé mesure désormais `e.mask!=rectangle.mask`, ainsi
que des rejets union Pool et des survivantes fallback réelles.

Les lecteurs du reçu
[r2](../../receipts/q34_cuda_waves_20260927/r2/summary.json) ont été
exécutés indépendamment en modes normal et `-O` : deux PASS. Les
31 commandes ferment 85 cas / 340 passages par build, 101 320 requêtes
et 39 868 survivantes ; 179 rejets Pool, 312 réductions de voie,
1 668 survivantes fallback et deux mutants causaux. Ce sont des
compteurs de couverture cumulés, pas ceux d'une trame. Le reçu annonce
explicitement `CUDA_compiled=false` et `GCP_used=false`.

La relecture du `.cu` gelé ne trouve pas de défaut évident d'offset,
masque, course ou publication. Les tableaux par candidate sont bornés
par Q ; E et les ordinals restent u64. Les scans emploient seulement
une vague admissible en `int`. Un échec de décodage ou `stack_failure`
est signalé et interdit toute publication, pas compacté comme un masque.
La fin des kernels précède les copies/lectures hôtes. Les S sont triées
avec leurs endpoints et masques avant conversion native.

**Ce constat n'est pas une compilation CUDA.** La gate réellement
exécutée sur GPU demeure obligatoire. Le snapshot, la préparation CPU,
les copies et le tri CPU ne disparaissent pas du coût du futur raccord.
Le temps `waves_including_count_download` comprend les petits retours
de compteurs/tails ; `survivor_download_allocate` mesure seulement les
retours des survivantes et leur allocation hôte.

## Chemin critique G4 historique : ne pas soustraire la somme des K

La [lecture dédiée](../b_critical_path_20260927/README.md) a été rejugée
par cet audit en modes normal et `-O`, puis confrontée aux emplacements
des chronomètres : deux PASS. Les chiffres publiés sont ceux des premiers
passages des processus 0/3 core ON, pas les sous-temps de passages chauds.

`Builder::finish` encode déjà les différents K en parallèle. Sa fenêtre
mur vaut 48,343 / 37,237 ms, alors que la somme des encodages par K vaut
112,728 / 85,376 ms. Les drafts sont libérés dans chaque chrono par K.
Le gain local du nouvel encodeur W4 ne se soustrait donc ni de cette somme
ni du temps FULL pour prédire un gain de chaîne. Il pourrait améliorer
l'intérieur d'un K, à mesurer après raccord et contention réelles.

S2 complet occupe 101,017 / 100,707 ms dans les 487,264 / 490,861 ms de
q34. Le census tardif reste autour de 103 ms et la tour hors fenêtre
d'encodage autour de 252–256 ms. Ces différences décrivent les intervalles
observés ; elles ne sont pas une borne inférieure universelle. Les coûts
q2 et census anticipé sont recouverts, leurs attentes nulles. Les additionner
une seconde fois serait faux. Cette analyse renforce le port sans tableaux
P/E, mais exclut de lui attribuer seul le facteur neuf manquant aux 100 ms.

## Protocole G4 dédié : examen avant tout lancement

La réutilisation inchangée de `full_probe_session_v7.run_session` est
acceptable comme **cycle de vie**, avec un validateur de snapshot
explicitement remplacé, sans faire passer le nouveau reçu pour FULL.
Le nom historique `full_host` ne décrit que le transport. L'archive
est reproduite depuis les blobs Git du commit, contrôlée membre par
membre, sans liens, doublons ou traversal ; la donnée ng00 possède sa
propre empreinte et son effectif complet.

Le pin du helper historique ne suffit pas à épingler le nouveau protocole :
wrapper, validateur et worker doivent correspondre aux versions commises.
Le worker distingue S2 de FULL, impose une gate CUDA avant la mesure,
et partage un budget utile global de 360 s entre ses commandes. Ce budget
n'est pas un plafond de facturation : lancement, récupération et fermeture
gardée sont des étapes supplémentaires. Le shutdown invité est de
30 minutes ; la garde GCE héritée exige au moins 2 700 s et le worker
actuel attend explicitement 3 600 s.

La contrelecture a repéré avant gel un mauvais nom d'option CMake dans
le worker : la vraie option est `MHGP9_CUDA_WAVES_ENABLE_CUDA=ON`.
L'ancienne écriture aurait construit le stub, puis refusé la gate GPU.
Ont également été demandés : prise en charge du fils immédiatement après
`Popen`, même si l'écriture de son PID échoue ; recette configure/build
liée au binaire exécuté ; manifeste des dépendances retournées réellement
vérifié. Ces corrections sont présentes dans la nouvelle version relue :
`wait_owned` joint le contrôleur sur les exceptions post-lancement,
les recettes sont reconstruites depuis le répertoire distant possédé,
et les dépendances locales sont confrontées au manifeste. Une erreur de
wrapper qui remonte après cette jointure impose néanmoins de contrôler
le reçu et l'état final avant passage de relais ; la jointure seule ne
certifie pas l'arrêt.

Le selftest figé a été relu entièrement puis exécuté indépendamment
en modes normal et `-O` : deux PASS, 26 contrôles positifs, 86 refus et
cinq scénarios de jointure. Il refuse notamment une recette changée même
avec intention cohérente, des dépendances source modifiées puis repinnées,
et des gardes identiques mais non certifiées. Le contrôleur historique
ajoute11 contrôles positifs et22 refus. Ces entrées sont synthétiques ;
aucun vrai sous-processus ni appel GCP n'est exécuté par ces selftests.

Aucun défaut bloquant restant trouvé avant lancement du paquet commis.
La session matérielle éventuelle est extérieure à cette note : aucune
fermeture G4, compilation CUDA ni mesure matérielle n'est déduite du
selftest. La présente contrelecture est close, sans modification du
moteur et sans nouvelle campagne de calcul.
