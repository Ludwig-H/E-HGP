# Design — l'étage paire à la taille contractuelle : le consommateur exact manquant

Statut : design normatif, implémentation à venir. Aucun claim, aucune porte
ouverte ou fermée. Incrément **B1** de la feuille de route du 7 août 2026,
successeur de l'ancien intitulé « R3 : paires device-tuilées » dont ce document
corrige le périmètre.

## 1. Le constat qui corrige l'intitulé

« R3 » a longtemps été écrit comme « porter l'étage paire en tuiles device ».
La relecture du code dit que ce n'est pas le travail restant, parce que la
partie device **existe déjà et ferme déjà l'échelle** — et parce qu'elle est
**architecturalement interdite** de produire ce dont le produit a besoin.

Deux objets distincts, deux binaires distincts, aucune unité de compilation
partagée :

| | frontière device tuilée | session paire ancrée (P8l) |
|---|---|---|
| symbole | `gpu::MortonYao48DeviceTiledPairFrontier` | `hierarchy::ExactSparseAnchoredPairSession` |
| consommateur | qualification hors ligne | `direct_morse_product_runner.cpp:4884` |
| ce qu'il produit | une **partition de couverture syntaxique** de l'espace des paires | la **classification exacte** de chaque candidat |
| à 50 000 points | ferme la couverture, `unresolved_pair_mass=0` | ne termine pas |

La frontière device ne peut pas devenir le catalogue par simple branchement :
son enveloppe scellée **rejette** un lot qui aurait évalué le rang diamétral,
publié un catalogue scientifique ou rapatrié ses candidats
(`morsehgp3d/src/gpu/morton_yao48_device_tiled_pair_frontier.cpp:485-495` et
`:1058-1068` — `exact_diametral_rank_evaluated`,
`scientific_pair_catalog_published`, `candidate_device_to_host_performed`
doivent tous être faux). C'est une propriété voulue, pas un oubli : le
composant compte de l'espace de paires, il ne décide pas de science.

Le travail restant est donc : **construire le consommateur exact qui manque
derrière la frontière device, et le raccorder à la façade par la couture
neutre qui existe déjà.**

## 2. Les mesures qui dimensionnent le problème

Toutes sur G4, toutes déjà archivées.

**Le chemin produit, à 50 000 points et $K=5$** (rang fermé 6), profil sans
budget, délai coopératif 300 s
([escalier R2-d](../validation/phase15_r2d_device_tile_certified_escalier_k5_g4_67facb1.json),
cellule `point_count=50000`) :

| grandeur | valeur |
|---|---:|
| univers de paires dirigées | 2 500 000 000 |
| paires dirigées élaguées authentifiées | 140 167 566 |
| candidats admis (tous `above_rank`) | 4 219 369 |
| couverture atteinte en 299,929 s | **5,7755 %** |
| visites de nœuds de classification | 290 573 814 |
| prédicats exacts groupés | 574 183 630 |
| candidats vivants simultanés | **1** |
| `directed_coverage_certified` | `false` |

Extrapolation linéaire sur la fraction couverte : **≈ 5 190 s**, soit environ
5 200 fois le contrat d'une seconde. Le moteur n'est pas lent par opération —
il exécute près de $2\cdot10^6$ prédicats exacts par seconde sur **un** cœur —
il en exécute beaucoup trop, et strictement en séquence
(`maximum_live_candidates = 1`).

**La frontière device, même nuage, même rang fermé 6, à chaud**
([plancher produit](../validation/phase15_product_floor_diag_g4_68f656b/full50k.json)) :

| grandeur | valeur |
|---|---:|
| construction LBVH (chaud) | 18,209 ms |
| launcher GPU | **1 004,989 ms** |
| records candidats | 4 500 332 |
| régions élaguées | 6 476 068 |
| masse de paires élaguée certifiée | 1 245 474 668 sur 1 249 975 000, soit **99,64 %** |
| visites de nœuds physiques | 22 697 584 |
| visites par record produit | **5,04** |
| `coverage_partition_complete` / `unresolved_pair_mass` | `true` / 0 |

Les deux autres postes du même artefact — recertification hôte 5,389 s et copie
de sortie 1,008 s pour 2,30 Go — appartiennent au **harnais de qualification**,
pas au chemin produit : ils rejouent ce que le produit n'aurait pas à rejouer.

Conclusion de dimensionnement : après B1, l'étage paire coûte de l'ordre de la
seconde au lieu de l'heure et demie. Cela débloque la mesure de l'étage higher
à la taille contractuelle. Cela ne tient **ni** le contrat 1 s **ni** le
contrat 100 ms à soi seul, et le dire autrement serait faux.

## 3. La couture neutre existe

`hierarchy::ExactDirectPairTerminalAuthority`
(`morsehgp3d/include/morsehgp3d/hierarchy/exact_direct_pair_terminal_authority.hpp:100`)
est déjà l'autorité paire consommée par le pont de source du réducteur terminal
(`direct_morse_terminal_reducer_source_bridge.hpp:219` et `:256`). Elle est
neutre quant au producteur : la façade la vérifie sans savoir d'où elle vient.
B1 n'a donc pas à inventer d'interface aval — il a à **remplir celle-ci depuis
un producteur device**, exactement comme le chemin tuile-certifié a rempli la
session ancrée du higher sans changer le contrat de la chaîne.

C'est la même inversion de flux que R1 : aujourd'hui le producteur hôte fait
tout le travail et le device ne sert qu'à la validation croisée ; demain le
payload device est le candidat et l'hôte valide par invariants.

## 4. Les six incréments, dans l'ordre

**B1-a — le classifieur device de rang diamétral, qualifié à l'échelle.**
Le classifieur existe et couvre les rangs fermés 2 à 11, mais sa qualification
native s'arrête à $n=257$ et rang 3. Il faut la porter à 50 000 points et rang
11 ($K=10$), et publier la ventilation par catégorie. Sans cela, tout le reste
repose sur un composant non qualifié à la taille visée.
*Validation* : clôture exacte de la partition de couverture, et différentiel
contre la session hôte P8l sur un nuage assez petit pour que celle-ci termine.

**B1-b — le pont D2H du catalogue candidat.**
La frontière device retient ses records en VRAM et son enveloppe scellée
interdit `candidate_device_to_host_performed`. Le consommateur exact a besoin
d'un **autre** genre de lot, déclaré comme tel, dont l'enveloppe autorise le
rapatriement et l'exige borné par les compteurs des contrôles de tuile — c'est
le pendant exact de R2-f côté paire, et le défaut que R2-f a trouvé (staging
possédé par un transitoire au lieu du lease) est le premier à ne pas refaire.
*Validation* : les segments rapatriés portent le fait
`record_segments_host_readable`, le drainage échoue fermé sans lui, et la
somme des comptes rapatriés égale les comptes des contrôles.

**B1-c — le producteur d'autorité paire depuis le payload device.**
Le producteur actuel de `ExactDirectPairTerminalAuthority` est qualifié à
$n=16$ en mode `serial_device_reference`. Il faut un producteur qui synthétise
l'autorité depuis le lot device : records canoniques, ordre, arités, bornes
d'index, comptes d'audit delta, clôture BigInt de la partition de masse.
*Validation* : les invariants 1–5 du régime tuile-certifié, transposés aux
paires ; aucun appel au générateur hôte ; le certificat déclare sa base de
vérification.

**B1-d — la septième surcharge de façade.**
La façade compose aujourd'hui l'autorité paire et le certificat de chaîne
ancrée du higher séparément. Il manque la surcharge qui compose l'autorité
paire **device** avec le certificat higher tuile-certifié dans un seul objet
scientifique.
*Validation* : égalité de tous les champs scientifiques avec la composition
hôte sur une fixture que les deux chemins ferment.

**B1-e — l'option `--pair-backend` du runner.**
Le runner n'a aucune option de backend paire : `--higher-backend` existe,
son pendant paire n'existe pas. Elle doit arriver **en dernier**, avec les
mêmes rejets typés que son homologue higher, et le défaut doit rester le
chemin hôte tant que le différentiel B1-f n'est pas vert.

**B1-f — le différentiel device ≡ P8l.**
Aucun différentiel n'existe entre les deux chemins. Il est la condition de
bascule du défaut, et il doit porter sur ce qui décrit le **résultat** —
événements acceptés, diagnostics d'extra-shell, masses, comptabilité des
records émis — et non sur les compteurs de travail, qui diffèrent par
construction entre bases de vérification, comme R1-a l'a établi pour le higher.

## 5. Ce que B1 ne fait pas

B1 ne touche pas à l'étage higher, ne change aucun statut, n'ouvre ni ne ferme
de phase, et ne prétend aucun SLO. Il rend mesurable ce qui ne l'est pas :
aujourd'hui, à 50 000 points, l'étage higher **n'est jamais atteint** parce que
l'étage paire consomme tout le délai. Après B1, le premier chiffre honnête sur
l'étage higher à la taille contractuelle devient possible — et c'est tout ce
que B1 promet.

Deux bornes structurelles scellées doivent être vérifiées avant la première
tentative à 50 000 points, comme l'exige la directive sans budget du 7 août :
la fermeture de descente de facette à 1 048 576 nœuds, et le plafond résident
interactif de 50 000 points.
