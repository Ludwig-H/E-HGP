# FULL A : manifeste natif possédé et rejeu indépendant

27 septembre 2026. Prototype d'audit isolé, profil u18/grille 1 mm,
`exploration_v9_hors_registre`, `cpu_reference`, `not_claimed`.
GCP non utilisé. Aucun moteur ni défaut produit modifié.

## Ce que cette porte établit

Qualification locale **R2 close : PASS** — 17 commandes, Release GCC et
Clang ASan/UBSan/LSan, 1 140 dépendances épinglées ; lecteurs normal/`-O`
identiques. [Résumé fermé](receipts/r2/summary.json),
[capture et recettes](receipts/r2/capture.json).

Le [ticket](../b_full_construction_parallel_20260927/NEXT_MANIFEST.md)
demandait une entrée fiable avant le futur constructeur événementiel.
Le manifeste conserve **tous** les blocs actifs de chaque K, y compris
les blocs muets, les tranches de cibles, les contributions exactes, les
rangs globaux et la représentation rationnelle du premier bloc de chaque
plateau. Un rejeu chronologique indépendant rend les mêmes tableaux A.

Il ne s'agit pas encore d'un constructeur massif, d'une forêt couvrante
parallèle, d'un remplacement de B/C, ni d'une nouvelle géométrie. Aucun
gain, coût sous-quadratique en nombre de points, chrono LiDAR ou contrat
FULL/G4 n'est acquis par ce dossier. Les catalogues sont ceux de petites
fixtures rationnelles, **pas** des catalogues produits par le générateur
LiDAR/WSPD. Les blocs et cibles qui en sont capturés sont, eux, réellement
calculés par le Builder natif validant ces catalogues.

## Attribution et frontière d'observation

[instrument.py](instrument.py) vérifie le SHA256 du header natif
`full_ball_tower.hpp` :
`124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0`.
Il extrait uniquement la classe, renomme classe et constructeur (deux
substitutions), puis ajoute trois observations (trois substitutions).
Les marqueurs et cardinalités sont vérifiés exactement ; le résultat,
le patch et leurs hashes sont conservés dans [instrument.json](instrument.json).
Une seule unité de traduction inclut d'abord le header natif inchangé,
puis la classe `AObservedBuilder` distincte. Aucune définition alternative
de `Builder`, aucun accès privé forcé, aucune archive `libchain` liée.

Les trois observations copient :

1. Après `order_prepare_lean`, programmes, cibles, comptes, masques,
   intérieurs, niveaux/rangs et domaine K1 ;
2. Les racines par occurrence **avant** tri/dédoublonnage ;
3. À la sortie réussie de A, draft encore ball-tagged, ancres, successeurs,
   naissances, rangs et onze compteurs sémantiques.

Un slot possédé par K, un écrivain ; aucune vue ne survit au hook. Le
pointeur d'observation global est fixé avant les threads, remis à zéro
après leurs jointures. Ce harnais est **non réentrant** : ne pas imbriquer
ou appeler concurremment deux captures. L'admission globale vient du
succès de tout l'appel et de tous ses slots, jamais du seul succès A d'un K.
Une panne B après tous les A et un réemploi échoué d'une capture admise
sont explicitement testés ; aucun manifeste n'est alors admis.

`static4`, hash et tri sont observés. `static1` n'entre pas dans cette
voie native lean et reste un témoin complet non observé. K1 est capturé
au sein d'une tour Kmax>1 ; aucune capture de Kmax=1 n'est revendiquée.
Le bras pass-through de la copie, le natif static4 et le natif static1
sont comparés champ à champ : banques, nœuds, parents, successeurs,
contributions datées, verticales, ordre et statut/motif. Les traces de
phase 0 (cibles et premiers ordinals de classes) sont également comparées.

## Format et juge

[manifest.hpp](manifest.hpp) convertit BallId en indice de bloc actif
par une table triée. `representative_begin` et `target` sont en u64 ;
CSR complet, blocs et lots sont validés avant toute indirection de cible.
Une cible K≥2 appartient au même programme et à un rang strictement
antérieur. K1 vise le **rang dans le domaine d'IDs ordonné**, pas un ID
brut ni le rang géométrique de `CloudIndex::upos`.

Le champ `run=level_run[ball]+1` conserve les trous du catalogue global.
Le zéro est réservé aux sites de K1. Le zéro initial et ses populations,
`birth_ball=absent` et `runs=0` sont reconstruits explicitement. Le champ
`lot_first_ball` désigne le premier bloc du lot entier, même silencieux ;
le juge conserve sa représentation rationnelle sans normalisation.

Le rejeu suit les historiques sans compression et groupe les racines
pré-lot par une DSU locale. Aucune mutation de successeur du plateau
n'intervient avant la collecte de toutes ses racines. Les parents sont
triés par ID historique et les contributions restent en ordre des blocs.
Ancres, racines par occurrence, successeurs, naissances/rangs, tous les
CSR du draft, références et niveaux représentés sont comparés au natif.
Les tableaux de compression internes ne sont pas des sorties sémantiques.
Le rejeu n'utilise pas la sortie capturée pour prendre ses décisions.

Le manifeste est une structure d'audit publique et possédée, pas un
nouveau `SealedCatalogue` ni un certificat géométrique. `validate_binding`
contrôle séparément sa filiation avec l'entrée native capturée. Les motifs
de refus testés ici sont ceux du harnais, pas une nouvelle priorité de
refus du produit. MEB, census, cibles et complétude du catalogue ne sont
pas requalifiés par l'égalité du rejeu.

## Corpus et mutations

[fixtures.hpp](fixtures.hpp) porte explicitement les conversions du gate
`tests/tower/full_ball_tower_gate.cpp`, SHA256
`c915775562c298a1a926b56ecbe7bbd05555618294c721a70e91020a71e359a7`.
Il réutilise la primitive rationnelle indépendante du juge local pour
énumérer les supports positifs de taille 2 à 4 et les populations des
petits catalogues. Ce producteur borné est un juge, jamais un candidat
algorithmique pour LiDAR.

Paire, triangle, tétraèdre, tétraèdre u18 extrême, carré, ABCZ, ABCZ doublé,
bloc inerte, contacts égaux, deux types de blocs de rayon 25 et coquille
12 sites/32 parents D5 ; Kmax jusqu'à 10. Deux variantes par fixture :
IDs non identitaires et entrée inversée, catalogue inversé et
représentations rationnelles égales distinctes. Slots K relus dans les
deux ordres, hash/tri et static1/static4.

Les cinq mutations causales sont de deux sortes, dans le même binaire :

- Trois **corruptions de manifeste** encore structurellement valide :
  frontière CSR décalée à taille finale constante, bloc muet omis,
  cible K1 remplacée par le rang géométrique ; le lien natif les refuse.
- Deux **branches mutantes du rejeu** : niveau du premier contributeur
  plutôt que du premier bloc ; suppression d'une contribution de
  continuation. Le juge complet des drafts les refuse. La seconde peut
  aussi supprimer un lot devenu vide, donc le premier champ divergent
  observé est `replay.level_representation`, en plus de la perte de refs.

Ce ne sont pas cinq mutants compilés indépendamment. Des refus de CSR,
domaine, cible et rang, ainsi que les deux échecs d'admission globale,
complètent la porte. Les doublons de racines intra-bloc et les partages
de racines entre blocs d'un plateau doivent être réellement non nuls.

## Qualification et reproduction

Dans chaque gate qualifié R2 : 22 variantes, 44 captures, 376 rejeux,
4 176 blocs, 5 416 occurrences de racines, 3 024 incidences de parents,
2 456 contributions, 624 blocs inertes, 32 continuations, 32 parents
maximum, 888 doublons intra-bloc et 1 120 partages de racines au plateau,
708 refus attendus et les cinq mutations causales exercées.
Ces nombres agrègent les deux lectures d'ordre K et les deux modes
hash/tri ; ils ne représentent pas autant de nuages indépendants.

La qualification fraîche Release GCC et Clang ASan/UBSan/LSan est lancée
par [run.py](run.py). Le collecteur de processus est réutilisé explicitement
depuis `gcp-migration/full_probe_session_v7.py`, SHA256
`177b25a0d72150dc331661fdf8da1ccde77ea17fb694d9c6af5b0929755160d8`,
vérifié avant import. Aucun appel cloud de ce helper n'est utilisé.
Les deux inventaires compilateur `-M` sont recueillis et hachés avant
toute compilation ; les dépendances effectives `.o.d`, objets, binaires,
sources, recettes et sorties sont fermés ensuite. Le lecteur LIVE dépend
des fichiers/builds locaux, n'exécute pas le gate et ne recompile rien.

```sh
python3 -B morsehgp3D_v9/audits/b_full_a_manifest_20260927/run.py --capture morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2 --build-prefix /workspaces/E-HGP/build/v9-a-manifest-20260927-r2
python3 -B morsehgp3D_v9/audits/b_full_a_manifest_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B -O morsehgp3D_v9/audits/b_full_a_manifest_20260927/run.py --readback morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B morsehgp3D_v9/audits/b_full_a_manifest_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
python3 -B -O morsehgp3D_v9/audits/b_full_a_manifest_20260927/selftest.py morsehgp3D_v9/audits/b_full_a_manifest_20260927/receipts/r2
```

R1 a réussi ses 17 commandes, puis le lecteur final a refusé son inventaire
préalable d'includes incomplet (options optimisation/sanitizer absentes
du `-M`). Reçus/builds et [runner R1](historical/r1_run.py) conservés ;
**R1 non qualifiée**, voir [l'échec](historical/R1_FAILURE.md).
R2 reprend des scans et builds frais, mêmes sources C++, avec les options
de compilation exactes dans les inventaires. R2 termine avec code 0,
sources/dépendances fermées, mêmes gates dans les deux builds ; lecteurs
normal/`-O` PASS. Les builds R1 et R2 sont désormais épinglés.
Le [selftest](selftest.py) hors géométrie rejette 20 corruptions du lecteur,
notamment build/binaire, commandes, dépendances et faux compteurs de
non-vacuité (stdout repinné pour atteindre le contrôle sémantique).
Il ne qualifie pas vingt motifs de refus distincts ni un nouveau moteur.
Les
préflights antérieurs ont rencontré un include Boost manquant, puis
des erreurs de compilation du harnais et une non-vacuité insuffisante du
mutant de représentation ; corrigés avant la capture fraîche. Ce ne sont
pas des défaillances géométriques du moteur. Le préflight n'est pas une
autorité de qualification et son build ne sera pas utilisé par le lecteur.

## Coûts et suite bornée

La capture copie le domaine et les métadonnées du **catalogue entier dans
chaque K**, puis le draft et l'historique. Le format n'est pas optimisé pour
une grosse campagne. Les compteurs locaux `copy_ms` sont des observations
non publiées par cette porte ; ni temps natifs instrumentés ni durées de
gate ne sont utilisés comme benchmark. Ces copies ne sont pas gratuites
et ne seront pas soustraites d'un mur pour annoncer un coût candidat.
Le juge chronologique sans compression est volontairement borné aux
petites fixtures, sans claim de croissance.

La suite est un **autre prototype** `event_a(manifest)` consommant cette
entrée, avec mêmes sorties avant tout parallélisme. Les vraies captures
LiDAR, V/E/G/P/C, capacités mémoire et coûts complets viendront après ce
raccord. La construction de B/C et l'encodage restent natifs et inchangés.
