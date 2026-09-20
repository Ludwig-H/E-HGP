# q4 : fenêtre exacte de faible profondeur — tranche30

20 septembre2026. `cpu_reference`, entrée u16, `public_status=not_claimed`.
Une arête fournie, **q4 seulement**, un thread dans les mesures : ni un
générateur global ni une tour FULL. GCP non utilisé.
[Preuve, API et limites](../../docs/Q4_FENETRE_DE_FAIBLE_PROFONDEUR_20260920.md).

## Ce qui est calculé

La sélection partagée29 reste identique. Pour chaque seed retenue,30
remplace le tri de toutes ses racines par deux petits tas : premières
entrées et dernières sorties. Après décompte exact des constantes
intérieures, ils définissent une fenêtre **fermée** contenant tous les
centres de profondeur admissible. Un second scan conserve tous les
contacts des deux bornes et compte les contributions intérieures fixes.
Seuls les événements strictement entre les bornes sont triés : au plus
2H−2 IDs, avec H=K−2−constantes intérieures. Une fenêtre ponctuelle reste
valide ; les grandes coquilles ne sont ni tronquées ni perturbées.

Le compte repart de zéro sur les sites retenus29. Le balayage retire les
sorties avant d'observer la profondeur stricte, puis ajoute les entrées.
Le certificat29 rétablit ensuite profondeur et coquille du cover ; la
positivité et l'arête propriétaire assurent leur validité globale.
Les six buffers privés sont réutilisés entre seeds. Les anciens chemins
et leur défaut restent inchangés ; aucune équipe q4 n'est ajoutée.

## Chronologie des preuves : échec initial conservé et reprise distincte

Le premier gel porte189 sources. Ces trois captures sont closes PASS,
avec sources et artefacts inchangés entre leur ouverture et leur fermeture :

| Capture initiale | Commandes | Contenu |
| --- | ---: | --- |
| [smoke_t17k5a2a](smoke_t17k5a2a/COMPLETION.json) | 19 | 9 gates Release,10 mesures à32 sites |
| [smoke_7e8pmosr](smoke_7e8pmosr/COMPLETION.json) | 19 | 9 gates Clang ASan/UBSan,10 mesures à32 sites |
| [scale_5ph5gh2r](scale_5ph5gh2r/COMPLETION.json) | 41 | 9 gates Release,32 mesures dont24 à8k/16k/32k |

Ce sont **52 mesures initiales**, pas une clôture réussie de toute la
régression. [regression_9a8cwnny](regression_9a8cwnny/COMPLETION.json)
reste **FAILED** :90/91 CTests passent, CTest sort avec code8.
Le test `mhgp8_wspd_q2_dynamic_receipts_gate_optimized` annonce un mutant
`worker_digest` survivant. Sa corruption forçait le champ d'un worker à0,
alors que ce champ pouvait déjà valoir0 : le test n'avait alors rien
corrompu. C'est un défaut du test Python de mutation, pas un résultat
q4 incorrect ni une raison d'ignorer la régression en échec.

La première [clôture de lecture](readers_fvjcjci_/COMPLETION.json) reste
également FAILED : trois lectures normales passent, puis le lecteur
refuse correctement la régression échouée. Aucune lecture optimisée ni
auto-épreuve de corruption n'est exécutée dans cette clôture. Son départ
prématuré ne transforme pas ce refus attendu en défaut du lecteur.

La [correction ciblée](preflight/WORKER_DIGEST_FIX.md) bascule désormais
un bit du digest, y compris lorsque sa valeur initiale est0 ; trois
contrôles déterministes exercent0,1 et2^64−1. Elle est limitée au test
Python : le produit C++, la gate6206 et les sondes q4 ne changent pas.
L'ancienne source et les échecs sont conservés. Les quatre captures
suivantes qualifient séparément le gel189 corrigé, toutes closes PASS
avec contrôles de fermeture sans erreur :

| Autorité de reprise | Commandes | Contenu |
| --- | ---: | --- |
| [smoke_ohuhz56s](smoke_ohuhz56s/COMPLETION.json) | 19 | 9 gates Release,10 mesures à32 sites |
| [smoke_radsfh8d](smoke_radsfh8d/COMPLETION.json) | 19 | 9 gates Clang ASan/UBSan,10 mesures à32 sites |
| [scale_qv0aggg3](scale_qv0aggg3/COMPLETION.json) | 41 | 9 gates Release,32 mesures dont24 à8k/16k/32k |
| [regression_2xx82lty](regression_2xx82lty/COMPLETION.json) | 1 | 91/91 CTests Release, aucun ignoré |

Ce sont **52 nouvelles observations**, distinctes des52 initiales,
pas52 tests supplémentaires de configurations différentes. Les anciens
reçus ne sont ni réécrits ni rebaptisés pour obtenir cette réussite.
La [fermeture des dix lectures](readers_h76zdu5c/COMPLETION.json) du
nouvel instantané passe : sorties normales/`-O` identiques,46 corruptions
de reçus refusées,189 sources,90 entrées et77 artefacts inchangés avant/après.
Les91 CTests sont exécutés en Release ; la campagne sanitizer porte sur
les neuf gates et dix sondes indiquées, pas sur l'ensemble des91 CTests.

Les erreurs de compilation initiales de la fixture de gate, puis le
chemin d'objet erroné du premier lancement de mutations, sont consignés
dans [PREFLIGHT.md](PREFLIGHT.md). Aucun mutant n'avait été exécuté lors
de cette erreur de lancement. Les builds utilisés sont
`build/v8_q4_window_20260920` et `build/v8_q4_window_sanitize_20260920` ;
leurs binaires ne sont pas réutilisés pour développer la tranche suivante.

## Contrôles géométriques et compatibilité

La nouvelle gate passe **6206 contrôles**, en Release et ASan/UBSan :
173 familles rationnelles,185 appels par seed,50 appels par arête,
2185 complétions et6322 tests de sites,95 candidats q4. Elle vérifie
352 groupes de racines,35 fenêtres ponctuelles dont4 avec constantes
intérieures et11 avec émission ;18 rejets par constantes,6 fenêtres
disjointes et2 rejets par contribution fixe sont réellement exercés.
Les compteurs sont contrôlés contre toutes les racines rationnelles,
pas seulement contre un digest de sortie.

Les cas comprennent les deux bornes infinies, des groupes mêlant entrées
et sorties, les coquilles30, les extrêmes u16, K maximal sans allocation
proportionnelle àK, les seeds retirées, les permutations et les dix arêtes
d'une petite fixture. Quatre allocations fautives, un callback levant,
le retrait des propriétaires pendant le callback et quatre appels
concurrents contrôlent les durées de vie. Ce n'est pas une campagne TSan
ni une qualification d'un ordonnanceur q4.

Les huit coordonnées du centre isolé de l'auditeur sont portées
explicitement puis vérifiées par **notre** oracle rationnel : centre
(33,33,27), rayon carré27, profondeur0, support0123 et coquille8 àK3.
Aucun code ou résultat de qualification de l'auditeur n'est hérité.

Trois [mutations compilées](mutants/README_MUTANTS.md) sont tuées par
une différence géométrique de boules/profondeurs/supports/coquilles,
avant les contrôles de compteurs : jeter L=U, oublier les intérieurs
fixes, oublier la coquille constante. Le premier cas causal est une
coquille30 avec son centre ajouté : profondeur1, et non0. Ce sont trois
fautes ciblées jugées par une gate, pas trois oracles indépendants.
Vingt [différentiels du chemin29](mutants/README_DIFFERENTIAL.md),
soit40 commandes, retrouvent tous ses champs JSON hors chronométrages.
Ils contrôlent la compatibilité, pas le gain de30. Les autorités finales
distinctes sont [compiled_89foraim](mutants/compiled_89foraim/COMPLETION.json)
et [differential_3gm_bctv](mutants/differential_3gm_bctv/COMPLETION.json),
closes PASS après correction du test Python. Leurs huit lectures
normales/`-O`, historiques/live, passent ; les JSON `*_REPRISE_READBACK`
sont liés depuis les deux notes. Les premières captures et lectures
restent intactes avec leurs anciens pins, sans transfert de qualification
automatique au nouvel instantané.

## Mesures initiales appariées : une arête, pas la tour

Le tableau suivant provient exclusivement de
[scale_5ph5gh2r](scale_5ph5gh2r/COMPLETION.json). Un thread, CPU0,
un essai par configuration sur hôte partagé : **aucun gain stable de
chronométrage n'est déduit de cette seule série**. La référence29 est
réellement exécutée. Les deux runs comprennent sélection, génération
des seeds, balayages et callback ; préparation commune nuage/index/cover,
validation indépendante et libération sont distinguées.

Chaque mesure compare les enregistrements complets normalisés
(support, clé, profondeur, coquille), pas seulement leurs hashes.
Le juge refait un census rationnel global par boule publiée distincte :
validation des émissions, pas preuve autonome de complétude sur le grand
dense. Celle-ci repose sur les certificats et petits oracles séparés.

K10, mêmes sorties entre29 et30. « Comparaisons30 » additionne **tas +
tri des tas + fenêtre + tri interne + regroupement** ; la référence
additionne son tri et son regroupement. Ne compter que le tri interne
masquerait l'essentiel du nouveau travail.

| Régime | n | Premier scan | Second scan ajouté | Comparaisons30 /29 | Run30 /29, ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Préfixe | 8 000 | 749 955 | 57 222 | 1 026 859 /9 373 577 | 30,751 /232,778 |
| Préfixe | 16 000 | 1 046 528 | 113 664 | 1 621 616 /13 434 740 | 48,876 /336,331 |
| Préfixe | 32 000 | 1 540 080 | 255 852 | 2 826 644 /19 959 115 | 85,861 /499,488 |
| Permuté | 8 000 | 165 648 | 42 840 | 352 569 /1 836 679 | 14,271 /52,933 |
| Permuté | 16 000 | 499 848 | 96 288 | 884 540 /5 932 798 | 36,485 /168,228 |
| Permuté | 32 000 | 1 575 024 | 254 968 | 2 446 344 /20 458 496 | 84,302 /577,742 |

Aux doublements, les lectures totales30 font×1,437/×1,548 au préfixe,
×2,859/×3,070 à la permutation ; les comparaisons font×1,579/×1,743
et×2,509/×2,766. Cela décrit ces régimes, pas une borne générale.
Les premiers scans restent exactement ceux de29 :30 ne supprime pas
le produit du nombre de seeds par le nombre de sites retenus.

La [comparaison de reprise](SCALE_REPRISE_COMPARISON.json) retrouve les
32 lignes intégralement identiques hors chronométrages, avec les mêmes
binaires ; seul le test Python change parmi189 sources. Les observations
temporelles restent distinctes : à32k permuté/K10, la reprise
`scale_qv0aggg3` mesure84,221ms contre565,259ms pour les runs, et
100,808ms contre581,846ms préparation comprise. Elles ne remplacent pas
sélectivement les temps du tableau initial.

Dans la capture initiale à32k permuté/K10, préparation commune+run
vaut99,749ms contre593,190ms.
Le run seul84,302ms **ne satisfait donc pas le contrat100ms de tour** :
ni50k points, ni toutes les arêtes, ni q2/q3, ni FULL, ni G4 ne sont mesurés.
Les capacités privées du sweep passent de16416 à320octets dans ce cas,
mais le pic dynamique propre total reste identique :1861533octets,
dominé par la sélection partagée29. Ce n'est pas le RSS ; nuage/index,
objets fixes et stockage du consommateur ont leur portée séparée.

## Contre-régimes et travail restant

- **AdversaireK10 : le carré du premier scan subsiste.** n32/64/128/256,
  r=n et S=n−2, donnent960/3968/16128/65024 lectures, soit
  ×4,133/×4,065/×4,032. Toujours36 sorties, donc le carré n'est pas
  imposé par leur nombre. Les scans ajoutés valent960/2944/6912/14336.
  Les comparaisons totales30 sont4326/13578/39636/119932, nettement
  moindres que29 mais ne font pas disparaître le premier scan.
  À256, run3,493ms contre17,824ms pour29 : amélioration locale,
  pas clôture de P0.
- AdversaireK5 : le premier doublement960→3968 reste supérieur à×4,
  puis les couches29 réduisent r ; ne pas ne publier que les derniers
  doublements favorables.
- **Far : aucun besoin de fenêtre sélective.** Douze lectures initiales
  deviennent24 ; K10 paie25 comparaisons au lieu13 et les buffers du
  sweep128octets au lieu64. La préparation globale domine ; les petites
  durées murales ne prouvent pas un gain général.
- Cap32k/K10 :2298 lectures initiales deviennent4596 ; les comparaisons
  baissent30277→8179 et le run17,303→15,111ms. Le coût de préparation
  des couches reste présent ; ce test ne décide pas si30 est préférable
  aux anciennes partitions par blocs28.

Par seed, avec t=min(K−2,r), le produit coûte
O(r log(1+t)+t log(1+t)), hors travail supplémentaire du consommateur.
Les grandes coquilles sont parcourues réellement. ÀK fixé, cela est
linéaire en r, mais **S seeds conservent O(Sr)** ; r et S peuvent être
proportionnels àn. La sélection29 coûte encore O(m log(1+m)+Km) par
arête. Aucune borne générale sous-quadratique n'est acquise.

La prochaine réduction devrait éviter de relire tous les témoins pour
chaque seed, par des structures partagées exactes ; conserver les chaînes
convexes29 est une piste, pas un port qualifié. WSPD multivoie, q3 global,
catalogue/intérieurs, FULL, GPU/G4 et dizaines de millions restent ouverts.
s8/10/12 se compare au futur raccord global, pas à cette primitive sans s.
