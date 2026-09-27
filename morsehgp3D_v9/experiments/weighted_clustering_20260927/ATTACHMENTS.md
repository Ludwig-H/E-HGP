# Attaches exactes des facettes pondérées dans FULL

Adaptateur distinct, 27 septembre 2026. Le catalogue Gabriel fournit les
cofaces et les poids ; **son graphe brut n'est pas utilisé comme hiérarchie**.
Chaque facette de F=∂C est résolue dans la véritable tour FULL native, dès le
rayon de sa propre miniboule, y compris les rattachements silencieux.
Aucun fichier moteur, ancien adaptateur, build ou reçu épinglé n'est modifié.

## Interface

Le nouveau binaire accepte les mêmes options que le précédent :
`--input XYZ.u32le --k K [--workers W] [--verify-coverage]`, plus `--gate`.
Le constructeur hérité conserve le nom d'artefact `native_weighted_export`
**dans un autre répertoire de build** ; le schéma permet de les distinguer.

Schéma `mhgp9_weighted_full_attachment_export_v1` :

- `weighted` : objet V1 inchangé, avec `native`, catalogue complet et cofaces.
- `anchors` : pour K≥2, exactement une entrée par boule du catalogue : nœud
  FULL fermé à sa date, ou `null` si la boule n'est pas active à l'ordre K.
  Une ancre silencieuse est conservée même sans contribution de couverture.
- `attachments` : une entrée par facette distincte de F, tri lexicographique.
  `vertices` contient les IDs d'entrée triés ; `beta` est le rayon **carré de
  la MEB originale de cette facette**, pas le niveau d'une coface incidente.
  `terminal_ball`, `anchor_node` désignent l'arrêt du résolveur ; `node` est
  cette ancre normalisée par les successeurs FULL à la coupe **fermée beta**.
  `descending_steps` et `same_radius_steps` comptent les échanges effectués.
- `validation` : égalité de tous les champs sémantiques des deux tours et
  deux condensés calculés à chaque appel ; admission de tous les slots
  capturés, et disponibilité des ancres.
- `stats` : facettes, ancres disponibles/absentes, MEB et recherches d'intrus,
  visites de nœuds, tests ponctuels, plages entièrement intérieures, échanges,
  durées additionnelles de FULL observé, comparaison et résolution.

K1 : la capture historique ne couvre pas une tour de Kmax=1. `anchors=[]`,
`anchors_available=false`, `all_capture_slots_admitted=false` le déclarent.
Les deux forêts sont néanmoins comparées. Chaque facette singleton est liée
directement à sa feuille native, beta=0, `terminal_ball=null` ; son identité
PointId est vérifiée par la couverture exacte de cette feuille à zéro.

## Source des ancres, sans accès privé forcé

L'adaptateur réutilise **intact**
`audits/b_full_a_manifest_20260927/native_a.hpp`, classe distincte
`AObservedBuilder`, et `capture.hpp`. Il n'introduit ni `#define private public`,
ni nouvelle définition du Builder natif. Le header natif conserve le SHA256
`124d3e9b52b1e6155e5a2ffd2a32cda7e5b403dda21155da2949b9ca8c90c0d0` ;
`instrument.py --check` a été relu et exécuté avec succès : les deux renommages
et les trois observations sont exactement ceux de la qualification historique.
Le hash de `native_a.hpp` est
`e06121208f7c4a7d2b5e04c1ecdf32cd8dcb73456e1c21e6ac770583a54293e4`.

La chaîne native construit d'abord sa tour et conserve son catalogue. Un index
déterministe neuf reconvertit ses rangs géométriques ; le même catalogue est
ensuite fourni au Builder observé **non scellé**, qui paie la validation
complète. Au moins deux workers sont nécessaires à sa voie observée, même si
la chaîne native est demandée avec W1. Aucun callback ne remplace une décision.

Après retour global réussi, tous les ordres doivent avoir exactement une
entrée et une sortie de capture, avec les mêmes IDs spatiaux. L'objet observé
est comparé au natif champ par champ : ordres, niveaux représentés, nœuds,
parents, successeurs, références de contributions et dates, banques et
verticales. Les chronos, compteurs de travail et motifs de provenance ne sont
pas des champs sémantiques à égaliser. Les digests ne remplacent pas cette
comparaison. Chaque ancre active doit déjà être une racine fermée à sa date.
La capture et la forêt observée sont ensuite détruites ; seule la table des
ancres de K est gardée. Capture non réentrante : aucun export concurrent dans
le même processus avec ce pointeur global historique.

Le port local `same_tower` est explicitement dérivé du juge existant
`b_full_a_real_20260927/probe.cpp`. Le reindex et le mode non scellé suivent
également ce précédent, mais la nouvelle utilisation est jugée séparément.

## Résolveur à la naissance d'une facette

La MEB exacte utilise `anchor_meb`, supports≤4, sur au plus K=10 sites. Si sa
clé est une boule du catalogue avec ancre active à K, le résolveur s'arrête.
Sinon il trouve un site non sélectionné strictement intérieur par l'index et
les bornes exactes `AxisBounds`, remplace le premier sommet du support retenu,
retrie, puis recalcule la MEB. C'est un port déclaré des décisions de
`Builder::intruder_work` et de sa descente de résolution ; le cache et les
raccourcis de graines ne sont pas requis pour cette référence.

Chaque échange reste dans la même composante à la coupe originale : l'union
des deux facettes successives est contenue dans la boule courante, donc sa
MEB ne dépasse pas cette coupe. Le rayon ne croît jamais ; à rayon constant,
la clé doit rester identique et le nombre de sites sélectionnés sur la
coquille doit diminuer d'une unité. Ces invariants sont contrôlés exactement.
Le nombre fini de facettes possibles et cette décroissance lexicographique
assurent l'arrêt sans plafond de recherche. Ce n'est **pas** une énumération
de toutes les facettes de X, ni une nouvelle borne générale sur le nombre
d'échanges ou les visites d'index.

À l'arrêt, toutes les K-facettes contenues dans la boule terminale appartiennent
à son bloc fermé. Son ancre est donc valide pour la facette résolue. Suivre
ensuite les successeurs de niveau≤beta originale donne la bonne composante à
la naissance. Contrairement au consommateur natif d'un événement ultérieur,
l'adaptateur admet l'égalité à cette coupe ; imposer la comparaison stricte
du `before` natif retarderait certaines naissances.

Il ne faut pas chercher l'ancre seulement dans les contributions : pour le
carré (0,0,0),(2,0,0),(2,2,0),(0,2,0), K2, la diagonale a sa MEB à beta=2,
sans intrus. Les arêtes strictes couvrent déjà toute la coquille, donc la boule
centrale n'ajoute aucun point ; son ancre existe néanmoins et fusionne quatre
branches. C'est un contre-exemple exact au raccourci « terminal Gabriel donc
contribution de couverture non vide ».

E5 est le second gate : A=(0,0,7), B=(0,9,6), C=(1,4,0), D=(0,0,1),
E=(4,1,2). La facette AC naît à beta=33/2 et s'attache à un segment plus
ancien après une descente par intrus, avant la coface Gabriel ABC de niveau
83886/3563. Aucun point nouveau ni nœud FULL nouveau ne signale cette attache.

## Construction, reçus et limites

`build_attachments.py --build REPERTOIRE_NEUF --jobs 2` charge le constructeur
`build_native.py` figé comme bibliothèque, avec son `ROOT` et `V9` inchangés.
Seul `HERE` est dirigé vers un nouveau répertoire privé frère `-adapter`
contenant l'unité d'entrée. Aucun objet ancien n'est réutilisé.

Une macro extérieure `main` ne convient pas au V1, qui en définit puis retire
déjà une pour son propre include. Le wrapper produit donc une copie privée
d'inclusion avec **une substitution comptée** de son propre `int main` vers
`mhgp9_frozen_weighted_export_main`. Toutes les autres lignes restent intactes.
Le SHA du V1 original est vérifié avant copie. Les chemins d'inclusion sont
explicités par `CPLUS_INCLUDE_PATH`, sans héritage caché ; les dépendances
effectives du compilateur les épinglent. Le reçu du frère `-adapter` conserve
substitution, hashes générés, environnement et pins avant/après. Le reçu du
build compile et ferme toutes les dépendances ; les deux reçus sont nécessaires.

Premier build : `build/v9-weighted-attachments-native-20260927-r1`.
Binaire SHA256
`c7afdae542074ee90bb8c1ba5a7dfe679ef9851f91bf32bbcbc067b15853576e`.
Reçu privé des contrôles :
`build/v9-weighted-attachment-checks-20260927-r1/receipt.json`, SHA256
`e6fdae5581e7e7320ee95daeef5d5ab1f0e9541266b695d896b5085a83746db9`.
Gate : 6 cas carré/E5 K1/2/3, 39 facettes, une descente exercée. Tests CLI
normal/−O : trois tests chacun, huit refus, normalisation et identité W1/W2.
L'oracle Fraction indépendant à toutes les coupes relève du protocole parent.

La qualification ASan/UBSan précédente porte sur l'exporteur V1 **sans ces
attaches**, pas sur ce nouvel adaptateur. Ses essais restent distincts :
Clang r1 échec de lien (nom du driver résolu en `clang`), GCC r2 build réussi
mais LSan refusé sous ptrace, checks r3 réussis avec `detect_leaks=0` et
**aucune qualification LSan**. Ne pas transférer ces tests aux nouvelles lignes.

Deux FULL, copie de capture, reindex, résolution, comparaisons et sérialisation
sont des coûts de référence explicites, pas une baseline de vitesse native
ni un chrono FULL comparable. Aucun GCP, aucun gain ou contrat massif acquis.
