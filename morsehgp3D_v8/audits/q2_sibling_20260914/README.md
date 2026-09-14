# Census q2 : le frère peut fournir le complément de témoins

14 septembre 2026, auditeur A. `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
Prototype d’audit uniquement ; sources de référence f7edd646. GCP non utilisé.

## Résultat utile au constructeur

Les [30 appels clos](SUMMARY.json) couvrent huit campagnes, huit entrées
physiques et 24 configurations, à Kmax=10, s=8, Samples/Shared et masque
q2 identiques. Les trois variantes rendent les mêmes supports, intérieurs
et coquilles ; front, candidates et collecte restent identiques. Les
répétitions en ordre inverse conservent tous les compteurs discrets.

Le certificat est un raccourci local sûr. Les résultats invitent à le
garder comme option expérimentale : sur les trois scans LiDAR à 8k, il
retire 0,39–0,84 % des visites Z, sans gain temporel stable. À 50k,
le retrait atteint 2,61 %, pour des temps toujours proches de 13,7 s.
Le seuil restant est mathématiquement plus fort, mais son supplément
de travail évité est très faible dans les régimes mesurés.

Scan 000000, avec lecture de l’entrée hors du temps intégré ci-dessous :

| Sites | Temps intégré baseline / autonome / restant, s | Visites Z baseline / autonome / restant, millions |
| --- | ---: | ---: |
| 8k | 1,531–1,538 / 1,538–1,543 / 1,537–1,572 | 59,227 / 58,993 / 58,948 |
| 16k | 3,547 / 3,738 / 3,842 | 135,806 / 134,441 / 134,310 |
| 32k | 7,949 / 7,892 / 7,755 | 280,997 / 277,280 / 276,861 |
| 50k | 13,711 / 13,675 / 13,673 | 579,839 / 565,479 / 564,685 |

À 8k sur le premier scan, seulement 32 911 propositions autonomes sur
2,24 millions d’entrées enfant passent la garde de population (1,47 %).
Le seuil restant évalue 65 441 bornes : ses 15 197 rejets, contre 1 529
pour l’autonome, évitent peu de parcours supplémentaires. En comptant
les bornes Z **et** celles du frère, le total est même légèrement supérieur
au mode autonome. Ces opérations n’ont pas toutes le même coût ; le
temps intégré reste le juge de leur combinaison.

Le régime des huit amas conserve une croissance problématique :

| Sites | Temps intégré baseline / autonome / restant, s | Visites Z baseline / autonome / restant, millions |
| --- | ---: | ---: |
| 8k | 15,780–16,327 / 15,267–15,574 / 15,104–15,314 | 973,179 / 922,280 / 921,645 |
| 16k | 66,062 / 62,727 / 78,712 | 4 199,341 / 3 959,987 / 3 957,918 |

À 8k amas, les deux ordres de mesure donnent 3,3–4,6 % de temps retiré
par l’autonome et 4,3–6,2 % par le restant. Les visites passent cependant
encore par un facteur 4,29–4,32 lorsque n double. Les 4,23/4,60 millions
de bornes du frère à 8k et 16,44/17,73 millions à 16k sont réellement payés.
Le total des bornes Z+frère baisse de 4,56 % à 8k et de 5,07 % à 16k.
Le choix du seuil restant change très peu ce dernier total.

Les temps viennent d’une machine partagée ; seuls les pilotes 8k sont
répétés en ordre inverse. L’essai restant à 16k amas, 78,712 s, est
conservé sans attribuer son écart au seul changement de seuil. Ces
observations ne donnent ni intervalle de confiance ni borne asymptotique.
Le coût dominant reste le census et ses subdivisions ; ce certificat
ne résout pas P0 et n’établit aucun contrat FULL ou G4.

## Question et portée

La [tranche constructeur](../../docs/P0_FRONT_ET_CENSUS_Q2.md) propose,
au partage B→C/S, de tester le frère S comme ensemble de Kmax témoins
stricts communs aux paires a×C. Ce certificat autonome rejette immédiatement
C ; sinon le census reprend son compte et son curseur inchangés.

Le complément ci-dessous permet de tester aussi des frères plus petits
que Kmax, sans liste d’exclusion ni modification du préfixe consommé.
Il s’applique au moment précis où le census partagé divise B. Il reste
distinct des blocs frères du chemin du proposeur étudiés par l’
[auditeur B](../PROPAGATION_TEMOINS_20260914.md).

## Pourquoi le seuil Kmax−c est sûr ici

Soit W l’ensemble des témoins stricts déjà crédités avant de diviser B,
avec compte c<Kmax. Chaque bloc crédité est strictement intérieur pour
toutes les paires a×B ; les crédits hérités des ancêtres le sont aussi
par restriction. Pour tout z∈W et tout b∈B, H(a,b,z)>0.

Si z appartenait à B, choisir b=z donnerait H(a,z,z)=0. Ainsi W∩B est
vide. Puisque le frère S est un sous-ensemble de B, W∩S est vide aussi.
Si la borne exacte sur les boîtes vérifie Hmin(a,C,S)>0, chaque paire
a×C possède donc au moins c+|S| témoins stricts distincts. Le rejet est
sûr dès que |S|≥Kmax−c.

Le certificat ne modifie jamais le compte ni le curseur : succès signifie
rejet immédiat, échec signifie reprise inchangée. Aucun témoin de S n’est
préchargé dans une continuation qui pourrait le compter une seconde fois.
Cette preuve utilise l’ensemble commun de témoins certifiés par les
blocs du parcours actuel ; un compteur externe, ou la seule égalité de
comptes issus d’ensembles différents, n’offre pas cet invariant.

Cette disjonction est particulière au frère d’un groupe encore intact.
Elle ne permet pas d’additionner arbitrairement les crédits du front à
ceux du census, ni d’exclure tous les sites des facteurs du rôle de témoin.
La collecte complète des intérieurs et de la coquille reste inchangée.

## Ne pas ajouter une garde de préfixe sans utilité

Un frère certifié intérieur ne pourrait pas non plus contenir un site
déjà consommé comme extérieur pour le parent. On pourrait donc songer à
tester son intersection avec le préfixe Z avant de payer la borne. Cette
garde est cependant toujours inactive au split du B global actuel.

En effet, avant chaque split, `prefix_end ≤ B.range.first`. Pour une
boîte B non ponctuelle, les bornes continues vérifient
`Hmin(a,B,B) ≤ 0 < Hmax(a,B,B)` : z=b donne zéro ; sur un axe de largeur
w>0, choisir l’extrémité b la plus éloignée de a puis déplacer z de w/4
vers l’intérieur donne H≥w²/16, en prenant z=b sur les autres axes.
Tout ancêtre Z contenant B reste donc indécis. Le parcours ne peut le
consommer ; arrivé à Z=B, les diagonales égales imposent le split de B,
qui peut aussi survenir plus tôt. Les enfants héritent d’un préfixe situé
avant leur parent, donc avant chacun d’eux et son frère.

Cette preuve dépend du même arbre global pour B/Z et de la règle actuelle
sur les diagonales. Elle ne vaut pas pour les rangs locaux d’Axis. Elle
explique aussi pourquoi la fragmentation peut persister : le parcours
ne consomme aucun site de son propre groupe B avant de le diviser.
Le prototype n’ajoute pas cette garde inutile.

## Implémentation et contrôles

Trois spécialisations comparées : `baseline`, `sibling` au seuil Kmax,
et `sibling_remaining` au seuil Kmax−c. Le test réutilise les constantes
préparées pour l’enfant ; le chemin de référence n’exécute pas les tests
ni les compteurs propres aux frères. Les chronomètres paient le front,
le census, la collecte et le callback complet avec son digest canonique.

L’adaptation est produite par [build_prototype.py](build_prototype.py)
depuis l’archive immuable de l’audit précédent. Le
[diff du census](r2_census.patch) et les sources adaptées sont conservés.
Le [build Release](r2_BUILD.json) et la
[reprise Clang ASan/UBSan avec LeakSanitizer](san_r3_BUILD.json) passent
les trois modes du gate : 467 858 contrôles et 1 255 appels intégrés par
mode, avec comparaison des objets complets. Ces qualifications portent
sur le prototype, pas sur les modifications produit parallèles.

Le [juge scalaire ciblé](sibling_fixture.cpp) passe 108 appels, quatre
permutations B et trois mutants **modèles**. Il emploie aussi un arbre B
local distinct de l’ordre Z pour vérifier les identités ; le gate adapté
exerce le raccord global. Sur a=(1000,0,0), B={0,…,63}×{0}×{0}, K10,
le certificat autonome donne 35 tâches et 109 visites Z, contre 127 et
421 en référence ; 48 paires sont rejetées en deux groupes, dix acceptées.

La fixture distincte a=(0,0,0), B={(999,0,0),(1000,1,0)}, z=(500,2,1),
K2 valide le complément restant en pleine dimension. Le compte vaut un
au split ; le frère singleton de (1000,1,0) fournit H=999. La variante
autonome n’évalue aucune borne de frère, la variante restante certifie
ce rejet ; l’autre paire garde son unique témoin intérieur et sa coquille.

Les échecs restent disponibles : [r1](r1_BUILD.json), helper privé appelé
à tort par le juge, puis [san_r2](san_r2_BUILD.json), LeakSanitizer bloqué
par le traçage du sandbox. Les sources antérieures et leurs archives
sont conservées. La reprise sanitizer autorisée conserve la détection
des fuites. Aucun échec n’est effacé ni requalifié en succès.

Les [contrôles du runner](CHECKS.json) passent en normal/−O : neuf cas
simulés de lancement, `KeyboardInterrupt` ou sortie invalide et trois
mutants de ledger. Ils ne qualifient pas les interruptions par signaux OS.
NaN et les exposants débordants comme `1e999` sont refusés avant insertion
dans le résultat JSON, afin de conserver la tentative et son stdout brut.
Ce correctif ne réécrit pas les reçus ni le runner épinglé de l’audit
précédent, qui n’a produit aucune de ces sorties invalides.

## Entrées et protocole

Les [entrées LiDAR existantes](../lidar08_20260914/README.md) restent
primaires : scans 000000, 000100 et 000200, sans hypothèse d’alignement des
points. Les [entrées amas](INPUTS.json) reproduisent explicitement la
recette constructeur à graine 3 et conservent son ordre d’identifiants.
Le hash constructeur de mots u64 et le hash des octets u16 de la sonde
sont nommés séparément. La génération des fichiers précède les mesures.

Les nouveaux tests du frère sont comptés séparément des visites Z.
`sibling_work.rejected_pairs` est déjà inclus dans le total des rejets
du census, sans double addition. Les contrats de tour FULL, G4 et massif
restent ouverts.

## Reproduction et fermeture

Les sources d’origine et adaptées, commandes, versions de compilateur et
hashes de binaires figurent dans les reçus de build. Le builder reconstruit
depuis l’archive épinglée ; un nouveau nom de capture est obligatoire et
les builds antérieurs ne sont jamais écrasés.

```bash
python3 -B morsehgp3D_v8/audits/q2_sibling_20260914/report.py --check
python3 -B -O morsehgp3D_v8/audits/q2_sibling_20260914/report.py --check
python3 -B morsehgp3D_v8/audits/q2_sibling_20260914/check_reference.py
```

Le rapport valide les huit fermetures, l’appariement des modes et les
répétitions. Le second contrôle vérifie les dix lignes baseline contre
les reçus LiDAR antérieurs et les reçus constructeur des amas, sur les
champs effectivement communs aux sondes. Une première version demandait
à tort des champs de préparation absents de la sonde d’audit ; cet échec
de lecteur et sa source figurent dans `CHECKS.json`, sans changement des
mesures. Les commandes normal/−O et contrôles documentaires finaux sont
conservés dans [VALIDATION.json](VALIDATION.json).
