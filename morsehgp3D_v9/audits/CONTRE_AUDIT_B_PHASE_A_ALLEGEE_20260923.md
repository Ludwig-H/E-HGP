# Contre-audit B — phase A allégée, preuve locale et prochaine mesure

23 septembre 2026. Relecture en lecture seule du port `aa29245fb`
(publié aussi en `96a053805`, restauré à l'identique par
`293aa6d7b`) et du
[reçu local](../receipts/tower_phaseA_lean_local_20260923/README.md).
Il s'agit de la tour CPU sur **une** trame 08/000000 sans sol,
39 885 sites, grille 1 mm/u18, s8/W8, K5 ou K10. Ni G4, ni trame brute,
ni profil float32, ni borne de croissance ne sont mesurés ici.

## Verdict technique

La carte `BallId→level_run` est correcte **si** `by_level` est trié
par niveau exact et si chacune de ses ruptures est décidée exactement.
Le code réutilise le filtre double seulement pour les écarts
certifiés, puis replie à `same_exact_level`. Les comptes par chunk
sont préfixés avant la seconde passe ; à la frontière, la rupture
`(j−1,j)` est évaluée par le chunk de `j`, sans doublon ni trou trouvé.
Les comparaisons de la phase A par run remplacent alors les tests
entiers répétés sans modifier l'ordre des plateaux. La voie statique
intégrée de la phase 0 utilise aussi les runs ; le résolveur batch
externe conserve, lui, ses comparaisons exactes. Le singleton ne
construit d'action que si elle est publiée et garde le tampon de
racines. Cela enlève une allocation d'action pour un lot inerte,
**pas toute allocation possible** : `lot_blocks.resize` ou le premier
`roots.push_back` peuvent encore allouer. Aucun défaut fonctionnel
nouveau n'a été trouvé à la lecture du diff ; neuf portes ciblées
ont passé sur un build frais d'audit, non archivé comme nouveau reçu.

Le membre `level_run` coûte **4 octets par boule du catalogue** et
reste résident pendant la construction de la banque et l'encodage
des forêts, après son dernier usage. Sur un catalogue R10 existant
de 5 512 670 boules, cela représente **22,1 Mo** à libérer avant
`finish()` sans toucher aux sorties ; vérifier d'abord son dernier
lecteur et mesurer la crête RSS. Le vecteur temporaire `approx` vit
désormais jusqu'à la construction des programmes, puis sort de portée
avant `finish()`. Tester explicitement plateaux égaux/quasi égaux,
frontières de chunk, quatre modes d'arrondi et W1/W4/W48 avant
d'attribuer une équivalence universelle au nouveau raccourci ; les
portes courantes ne constituent pas cette matrice complète.

## Ce que mesure réellement le reçu

Les **13 empreintes** du dossier passent. Les cinq paires de sorties
base/nouveau ont mêmes entrée et options déclarées, statut,
catalogue, digest de tour, compteurs par ordre et compteurs de travail
de la tour. Les nombres du tableau sont bien ceux des JSON : la
phase A de l'ordre maximal passe de `753/784/835` à `495/584/534 ms`
à K5, et de `3679/3119` à `2134/1965 ms` à K10. Les temps de tour
K10 sont `14,766/15,858` contre `13,340/13,323 s` ; à K5 les trois
paires de tour sont mixtes. Callgrind donne `540,7→301,3 M`
instructions exclusives de la phase A sur **une autre coupe de 4k**.
La hausse apparente de validation K5 reste inexpliquée dans le reçu ;
l'hôte partagé était fortement chargé. On ne peut convertir ces
mesures en gain G4, encore moins en tour sous une seconde.

Le reçu n'épingle **ni la commande exacte, ni le SHA complet des deux
binaires, ni le SHA des octets de l'entrée brute, ni le log des
`152/152` portes**. Les JSON sont intègres, mais le lien externe
entre ces sorties, les binaires et cette assertion de tests demeure
plus faible que celui d'une réception G4. De plus, la base
`3dfedcae` et `aa29245f` diffèrent aussi sur deux sources GPU ; les
leviers GPU sont désactivés dans les cinq paires, ce qui rend leur
effet improbable ici sans faire de l'ablation un diff source
littéralement limité à un fichier. Les compteurs amont de cache et
l'occupation des workers q3/q4 varient entre répétitions, malgré les
compteurs de travail et condensés de tour égaux : ne pas interpréter
leurs chronos isolés comme une causalité fine.

**Suite utile :** fournir commande, SHA complets et log de portes ;
mesurer sur G4 avec le même paquet ON/OFF, plusieurs trames brutes
et sans sol, K5/K10, s8/10/12, W1/W24/W48, ainsi que crête mémoire,
temps de validation, phase A par ordre et temps total de la tour.
Comparer catalogue et payload FULL clé par clé ou par porte exacte,
pas seulement le digest. Cette optimisation réduit du travail réel
de la tour ; elle ne réduit ni le nombre de formes q3/q4 ni la masse
des paires WSPD, qui restent les autres verrous du contrat.
