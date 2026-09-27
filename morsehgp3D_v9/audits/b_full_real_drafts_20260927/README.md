# Encodeur batché sur de vrais drafts FULL — 27 septembre 2026

Base produit `4badf8b7d7edfd6c2d820635d3edd10a61564e59`. Audit CPU local,
`exploration_v9_hors_registre`, `quantized_u18_input_only`,
`mode=real_drafts_structural_encode`, `public_status=not_claimed`.
Aucun moteur ni défaut modifié ; aucun GCP utilisé. Les fichiers KITTI
préexistants de la v8 sont uniquement lus, jamais copiés dans la v9.

## Conclusion actuelle

L'encodeur batché reconstruit **les mêmes tableaux** sur une vraie tour
LiDAR sans sol K1..5 : 1 541 750 nœuds, parents, successeurs et 897 776
contributions, tous les champs comparés trois fois par ordre. L'ancien
gate de petits historiques n'était donc pas le seul cas d'usage favorable.

Mais le prototype **régresse sur CPU scalaire**. Sur la trame 08/000000,
les médianes locales par ordre sont :

| ordre | encodeur natif ms | prototype ms |
| --- | ---: | ---: |
| 1 | 3,078 | 8,582 |
| 2 | 9,692 | 21,800 |
| 3 | 32,984 | 51,415 |
| 4 | 48,852 | 89,567 |
| 5 | 54,776 | 124,935 |

Ces temps ne sont **pas** le temps de toute la tour : encodages isolés,
séquentiels après capture, trois essais seulement, hôte partagé, forte
variabilité visible dans les données brutes. La somme des médianes ne
mesure pas l'encodage parallèle du produit. Aucun gain GPU ni contrat
100 ms n'en découle. Ne pas remplacer aveuglément l'encodeur CPU courant.

Le résultat positif reste architectural : il est possible de vérifier et
écrire les tableaux par préfixes, regroupement d'incidences et dispersion.
Il faut maintenant payer et mesurer ces étapes sur un backend parallèle,
ou réutiliser les classes du producteur pour éviter ce nouveau tri.

## Accroche exacte, sans copie lourde du moteur

Le produit n'a pas de callback de draft. `Builder::finish()` les donne aux
deux overloads de `build_full_coverage_certificate()`, puis les détruit.
La sonde :

1. Inclut d'abord `full_coverage_certificate.hpp`, donc ses constructeurs
   natifs existent sous leurs noms originaux.
2. Déclare deux petits adaptateurs appelant **toujours** le natif.
3. Renomme temporairement les deux appels pendant l'inclusion du seul
   `full_ball_tower.hpp`, puis retire immédiatement la macro.
4. Compile `tower_chain.cpp` et le stub GPU dans **la même unité de
   traduction**. Ne lie ni `libmhgp9_chain.a` ni une autre définition du
   `Builder` : pas de variantes inline incohérentes entre objets.
5. Copie chaque draft dans une case K préallouée, après le constructeur
   natif et avant son effacement. La forme imbriquée est aplatie à part ;
   la voie statique à quatre workers produit directement la forme plate.

Ce n'est pas une nouvelle géométrie : la chaîne et ses constructeurs sont
inclus depuis leurs fichiers épinglés, sans copie éditée. Seule la couture
d'encodage est instrumentée. La bibliothèque générateur est réutilisée
explicitement depuis les builds q3 du 26 septembre ; ses empreintes sont
fermées avec celles des sources. Les garde-fous et leviers GPU restent OFF.

Les cases K ont chacune un seul écrivain ; activation et remise à zéro
entourent l'appel synchrone de la chaîne et ses jointures. La banque est
celle du **constructeur interne privé**, sans alias mutable échappé ; elle
reste partagée, non recopiée par ordre. Cela n'efface pas le défaut de
l'overload public par déplacement, traité par l'audit d'alias distinct.

## Comparaison et limites de qualification

Après retour de la chaîne, chaque draft est encodé trois fois par les deux
constructeurs, dans l'ordre natif/batché, batché/natif, natif/batché. La
comparaison champ à champ vérifie K, même banque, niveaux avec leurs mots
et dénominateurs exacts, offsets, parents, successeurs et contributions
datées. Le prototype est aussi comparé à la forêt réellement publiée
par la chaîne, pas seulement au deuxième appel natif.

Les petites portes exécutent les deux voies (`static=0` et `static=2`),
puis rejouent chaque chaîne avec interception pass-through : les trois
digests (tour, catalogue, présentations) sont identiques. La tour originale
garde ses verticales ; **le prototype ne les recalcule pas**. Ce lot ne
qualifie donc pas un remplacement A+C ni une nouvelle géométrie FULL.

Gates Release et Clang ASan/UBSan/LSan : quatre chaînes par binaire,
dix ordres réencodés, trente paires comparées. Aucun TSan, aucun parallèle
interne du nouvel encodeur : celui-ci reste scalaire. Les grands cas ne
sont pas des exécutions sanitizer.

Les sources, bibliothèque, configurations et binaires sont épinglés
avant/après chaque capture. `run.py check` vérifie l'inventaire complet
des builds et leur présence ; il ne relance pas les exécutions fermées.

## Chronos et mémoire : ne pas les mélanger

La génération des vrais drafts est payée et publiée. Pour LiDAR00 :
chaîne instrumentée 28 632 ms, dont q2 1 491 ms, q34 24 404 ms, tour
instrumentée 1 471 ms ; mur externe lecture exclue 29 343 ms. Ce n'est
pas un chrono de contrat G4. La capture ajoute des copies dans la fenêtre
d'encodage du produit : somme par K 56,56 ms, mais **on ne peut pas la
soustraire** au mur parallèle. Le champ `chain_encode_instrumented_ms`
(79,90 ms) reste explicitement contaminé par ces copies.

Les chronos de réencodage incluent allocations, validation et écriture,
mais excluent comparaison et destruction des résultats ; celle des
temporaires internes au constructeur est incluse. Ils portent seulement
sur l'encodeur complet, sans séparation artificielle validation/tri/scatter.

LiDAR00 : capacités des drafts copiés 127 383 080 octets, forêts publiées
195 162 040 octets, banque partagée 59 075 492 octets. Le pic processus
964 972 KiB inclut construction, copies et réencodages : ce n'est ni le
pic du seul encodeur ni une mesure VRAM. Pour le prototype, l'ardoise
demandée est `(3*A+2)*sizeof(size_t)+40*P` sur cette plateforme, où A est
le nombre d'actions et P les occurrences de parents. Ce nombre exclut
sorties, pile du tri et frais de l'allocateur ; il n'est pas un pic RSS.

La trame 08/000000 est entière **après masque sans sol figé**, à 1 mm,
39 885 sites. Masque et origine sont vérifiés par le préparateur historique,
ainsi que les sept partitions, sans nouveau retrait du sol ni score
sémantique. Seule la trame entière est chronométrée ici, pas un morceau.
Les petites fixtures et les tailles 8k/16k/32k sont synthétiques, distinctes
de cette trame. Les nombres de drafts ne prouvent pas à eux seuls une
croissance sous-quadratique du générateur LiDAR.

## Croissance du draft sur uniforme 8k/16k/32k

Les trois mesures sont closes, K1..5/s8/W4, mêmes recettes/seed 3 que les
captures précédentes, identités de coordonnées vérifiées. Le nombre total
d'actions vaut 629 404 / 1 301 794 / 2 660 312, soit ×2,068 puis ×2,044 ;
les contributions font ×2,066 puis ×2,045. Ces postes de sortie croissent
nettement moins que ×4 dans ce régime mesuré. Cela ne prouve pas une borne
générale, ni la croissance sur LiDAR dont une seule trame est mesurée ici.

Le prototype régresse aussi sur ces vrais drafts synthétiques : somme
**des médianes par K**, native/prototype en ms = 52,14/92,41,
130,01/241,30, 287,51/554,28. Ces sommes ne sont pas des murs de tour.
Les chaînes instrumentées complètes prennent 6,215 / 13,362 / 28,151 s,
hors lecture et digest, et n'établissent aucun contrat G4.

## Piste suivante fondée sur les drafts, pas encore exécutée ici

Ces quatre tours ne contiennent **aucune continuation** : toutes les
actions ont zéro ou au moins deux parents. Cela permet un chemin distinct
sans le tri d'incidences, après vérification globale de cette condition.
Chaque utilisation d'un parent est alors une fusion définitive. Le contrôle
`live` se réduit à « chaque parent est utilisé au plus une fois », en plus
du contrôle antériorité au lot et des autres contrôles locaux.

On peut initialiser `first[parent]` à l'absent, puis prendre le minimum
de l'ordinal global dans `draft.parent` pour chaque parent. Une seconde
passe refuse chaque occurrence différente de ce minimum. L'ordinal CSR
est déjà l'ordre lot/action/position ; la deuxième occurrence reçoit donc
la faute, et la réduction d'erreurs habituelle reste nécessaire. Vérifier
le domaine du parent **avant** toute lecture de `first[parent]`.

Cette variante a un travail linéaire en taille du draft et une table O(V),
avec minimum atomique possible en parallèle. Ce n'est encore ni un gain
mesuré ni un port produit. Contre-exemple interdisant de la généraliser :
naissance de 0, continuation valide de 0, puis fusion valide utilisant 0.
La répétition est ici normale ; si une continuation est présente, garder
le chemin général tant qu'une autre preuve n'est pas disponible.

## Historique et reproduction

Les deux premiers essais ont échoué à la compilation du lecteur d'entrée
de **la sonde** : `Point3::operator[]` non assignable, puis conversion
signée non explicitée. Les sorties et sources initiales sont conservées
dans `qualification/` et `r2/qualification/`. Aucun test géométrique n'a
été exécuté par ces builds ; aucune option d'avertissement n'a été retirée.

La qualification corrigée est `receipts/full_real_drafts_20260927/r3/` ;
build clos `/workspaces/E-HGP/build/v9-audit-full-real-drafts-20260927-r3`.
Ne pas le reconstruire ni l'écraser.

```bash
python3 -B morsehgp3D_v9/audits/b_full_real_drafts_20260927/run.py check qualification
python3 -B -O morsehgp3D_v9/audits/b_full_real_drafts_20260927/run.py check ng00
```

Le [reçu courant](../../receipts/full_real_drafts_20260927/README.md)
recense les captures effectivement closes, leurs résultats et limites.
