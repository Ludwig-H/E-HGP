# Contre-audit B — D5, tour FULL maigre et parallélisation de la phase A

23 septembre 2026. Lecture du [rapport C des alternatives](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md),
des notes [graphe temporel](PHASE_A_GRAPHE_TEMPOREL_20260923.md) et
[maximum d'ID](PHASE_A_MAX_ID_COMPOSANTE_20260923.md), du code FULL
produit et du reçu G4 R8. Aucune modification du moteur ni nouveau
chrono. Le choix D5 est une **hypothèse de réduction de travail à
porter par paliers**, non un gain produit déjà reçu.

## Ce qui est prometteur, et ce qui ne l'est pas encore

Le sidecar D5 a confronté des millions de facettes à la voie actuelle :
racines pré-lot, nombres de nœuds et fusions concordent sur ses cas, et
le saut au centre réduit MEB et profondeur de résolution. Les lemmes de
jointure des selles et de saut sont de bonnes pistes. Mais la projection
« FULL ×6–13, K10 0,25–0,6 s » **n'est pas une mesure G4 appariée**.
Le probe local 8k mono mesure surtout la résolution et une phase A
maigre après préparation d'offsets hors chrono ; il ne matérialise ni
actions/contributions, ni populations, banque et images complètes, ni
le payload sérialisé. Son égalité de racines/comptes n'est pas
`same_payload`. Le rapport C corrige lui-même index+jointure à
0,79–1,02 s nets à 8k/K10 et reconnaît le coût de la queue FULL.

R8/K10/W48 rend cette queue concrète : pour 08/000100, validation
0,43 s + populations 0,13 + images 0,26 + banque 0,10 + encodage
0,13 = **1,05 s** ; pour 000000, **1,24 s**. La phase statique vaut
1,12–1,46 s et les lots 0,80–1,19 s en plus. Réduire seulement la
résolution et la phase A ne produit donc pas 0,25–0,6 s FULL ; il faut
porter, mesurer et vérifier chaque élément de la queue, ou justifier sa
suppression par un certificat équivalent. Ces chiffres sont les phases
du chemin R8 actuel, non une borne sur un autre algorithme.

Deux erreurs de la première forme de D5 sont déjà connues : la descente
par saut exige la **règle 0** de reconnaissance de la boule courante à
chaque état, et la sortie compacte doit garder la **position de programme
du premier bloc du groupe** pour les contributions de continuation.
La voie singleton du sidecar utilisait un tampon de **13 racines** ; une
coquille de 12 sites a produit **32 racines** (`fx_ico12`), donc le chemin
doit passer à un stockage à offsets ou replier sans débordement avant
tout port produit. Les propositions sur images de fusion et tranches
figées restent des esquisses à qualifier, pas des théorèmes de vitesse.

Le **premier port sûr** est plus petit que D5 entier : dans la seule
résolution statique, ajouter l'index exact des graines et des selles
régulières, comparer intégralement les IDs et la clé, puis retourner au
`static_terminal` actuel sur chaque miss. Garder `validate_catalogue`,
`ShellTable`, phase A, banque et sortie explicite inchangées. Juger
index + jointure + repli et la cible/racine de **chaque** facette, pas
seulement le nombre de MEB. Cette tranche vise la phase 0 de R8
(1,12–1,46 s à K10) et isole une réduction de travail du reste des
hypothèses. Le saut au centre avec règle 0 vient ensuite ; la phase A
maigre et le compact/scellement sont deux étapes encore séparées.

## Frontière exacte d'un « catalogue scellé »

Si la chaîne remet à FULL un type scellé pour éviter une seconde
validation, ce type doit être **possédé et immuable**, lié au même
`CloudIndex`, constructible seulement après certification exacte de
chaque clé, niveau, support positif, census global, coquille entière,
`q_min`, fenêtre d'admission et unicité. Les `ShellTable` étendues
calculées en amont doivent être conservées ou reconstruites et payées.
L'API d'entrée externe reste validante ; les contrôles par facette des
racines, niveaux et actions ne disparaissent pas par simple changement
de type. Un échantillon ne certifie pas tous les supports ; le scellement
n'élimine pas non plus la possibilité d'une `BallKey` **entièrement omise**
par le générateur. `complete_relative` reste le statut tant que cette
porte globale manque.

## Paralléliser la phase A sans changer les IDs

Un `parallel_for` sur `order_block` actuel est incorrect : `order_root`
compresse la DSU, le curseur de cibles avance et les stats se mutent.
Dans un plateau d'un niveau exact, figer les racines au seuil **ouvert**
`λ⁻`, résoudre les facettes indépendamment contre cet état, puis former
les groupes par racines partagées. Garder le premier ordinal de bloc,
les parents uniques triés, les blocs inertes, les contributions dans
l'ordre de programme ; fermer le plateau entier avant de publier les
ancrages et les IDs suivants. Une coquille étendue peut demander plus
de racines qu'un petit tableau fixe. Les notes de graphe temporel
ouvrent un traitement inter-niveaux par composantes à seuil ouvert
`C<` et fermé `C≤`, avec racine vivante = plus grand ID marqué après
préfixe canonique ; elles sont encore hors produit. Comme seuls
**1,72 %** des blocs K10 du sidecar appartiennent aux lots groupés,
paralléliser seulement *dans* un lot touche peu de blocs : tranches
figées ou coupes inter-niveaux sont les expériences pertinentes.

Première porte : vraies fixtures de plateau à trois parents, même
`BallId` sur deux K, continuation contributive, bloc inerte, aucune
facette, coquille 12/32 racines, égalités de niveau et refus ; W1/4/8,
ordres de lancement variés, ASan/UBSan/TSan. Comparer **chaque** racine
pré-lot puis l'expansion octet par octet des actions, parents, images,
populations et digest, pas seulement les comptes. Mesurer à catalogue
identique 8k/16k/32k CPU·s, mur, RSS, index/jointure, résolution,
validation, phase A et finition. Une seule sonde G4 appariée devient
pertinente après fermeture de cette porte locale ; pas de noyau par
niveau tant que les plateaux et la frontière séquentielle ne sont pas
chiffrés.
