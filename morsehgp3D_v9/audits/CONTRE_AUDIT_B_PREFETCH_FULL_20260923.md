# Contre-audit B — préfetch géométrique FULL par K

23 septembre 2026. Lecture de
`audits/PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md` (A) contre le moteur v9
`5ab4326c` et le reçu G4 R1. **Verdict : découplage mathématiquement
plausible, sans faille exacte trouvée ; port asynchrone et gain non prouvés.**
La source G4 mesurée est `e28296bb`, antérieure au MEB diamètre et à tout
préfetch. Son entrée est une trame 08 **sans sol**, u18 à 1 mm, CPU sur G4 ;
ni GPU, ni float32 original, ni contrat principal brut ne sont couverts.

## Ce qui peut être préparé indépendamment

Pour un K et une facette triée, le résolveur statique lit index,
`BallData`, `by_key` et les semis des populations complètes `I∪U` propres
à ce K ; il renvoie un `BallId`, non une ancre ni une racine
(`src/tower/forest/full_ball_tower.hpp:577–632`). `visit_block` extrait les
facettes du catalogue/ShellTable, sans lire la forêt (`:924–953`). Dans
`static_terminal`, `before` sert aux validations de niveau **strictement
inférieur**, pas à choisir une branche. Les programmes sont triés par
niveau exact ; le tri des requêtes par `(facette, ordinal)` garde comme
première occurrence le consommateur de plus petit niveau (`:518–529,
:755–790`). Sur un catalogue valide, valider ce minimum permet donc de
réutiliser le même BallId pour les autres occurrences **du même K** ; leur
chronologie doit toujours être contrôlée. Les semis et le rang ne se
transportent pas entre deux K.

La traduction `BallId → anchors[target] → root` attend une ancre déjà
fermée (`:861–867`). Les K et niveaux restent ordonnés, et une multifusion
de niveau égal n'est publiée qu'après résolution/union de **tout** son lot
(`:298–349,:990–1080`). Un préfetch ne peut jamais publier un parent ou
fermer une fraction de ce lot. Le raccourci évoqué par A à Kmax sur une
clé déjà cataloguée est compatible avec le rang validé, mais les recherches
d'intrus sur clés **absentes** demeurent ; il ne retire pas à lui seul le
coût observé à K10.

## Port concret : plus que `current_k` à isoler

Le `Builder` actuel partage aussi `static_targets`, `static_cursor`, `st`,
`programs`, `extra`, `by_key`, `anchors` et les historiques
(`:390–404`). Les trois premiers sont mutables ; les autres ne deviennent
des lectures concurrentes sûres qu'après construction complète et sous une
durée de vie possédée. `FullBallGeometryView` emprunte index et spans
catalogue ; son callback de lot est **synchrone** (`:59–95`). Pour calculer
plusieurs K pendant la fermeture courante, il faut un contexte immuable
**par K**, des requêtes/semis/cibles/curseurs et compteurs privés, une
réduction vérifiée du travail, puis joindre/canceller tous les jobs avant
de libérer le catalogue ou de publier un résultat. Un échec d'un futur K
peut annuler la tour, jamais laisser un préfixe « FULL » publié ; le
travail déjà payé doit rester compté ou marqué inconnu si le backend ne
peut l'attester. Le maintien du même `BallId` et du même ordinal à travers
les fenêtres est une obligation de sortie, pas une optimisation facultative.

## Mémoire : scénario `60R` ≠ plancher incompressible

Sur l'ABI LP64 lue, `FullBallBatchRequest` occupe **56 octets** et
`static_targets` **4 octets par occurrence**. Posons `R=Σ_K` occurrences
de facettes, `U_K` clés de facettes distinctes et `S_K` semis au K considéré.
Garder naïvement **toutes** les requêtes et cibles de tous les K coûte
`60R` : sur le reçu G4 08/000000 K10 (`R=17 389 031`), cela fait
**1 043 341 860 octets logiques**. Ce chiffre d'A est juste **pour ce
scénario**, non une mémoire obligatoire du préfetch. Après résolution
d'une fenêtre, ses requêtes/semis peuvent être relâchés ; conserver
seulement les cibles coûterait `4R` (69 556 124 octets sur cette trame),
hors métadonnées d'ordinals et capacités. Le chemin actuel ajoute, par K,
`8(U_K+1)` octets de groupes et `44S_K` de semis ; le callback externe
peut simultanément ajouter `56U_K` de `pending`, `4U_K` de cibles uniques
et `16U_K` de résultats, avant ses propres buffers
(`:653–679,:755–814`). Il faut mesurer les **octets coexistants** et
libérer les fenêtres achevées, pas additionner des maxima non simultanés.

À 30 M sites, ni `B` (BallKeys) ni `R` ne sont connus : multiplier le
ratio de la trame de 40 k serait infondé. Sans préfetch, le `Builder`
statique actuel a déjà, juste après K1 et avant K2, un plancher de
payload/capacités sur cette ABI d'au moins `576n−172+248B` octets,
soit **17,28 Go + 248B** à 30 M (cache temporel absent en mode statique),
hors allocateur/surcapacités. C'est un plancher de **la représentation
actuelle**, pas une nécessité mathématique : K1 publié pèse déjà `216n`
octets dans ce format et pourrait être réencodé. Le catalogue lui-même
reste global et `BallId=u32` impose le refus si `B>2³²−1` ; aucune borne
LiDAR ou garantie de résidence à 30 M ne découle du reçu G4.

## Portes de décision

G4 R1 mesure, sur 08/000000/K10, FULL 75,90 s temporel contre 34,14 s
statique W48, même condensé ; il ne décompose pas les 34,14 s. Le statique
prépare déjà les facettes, trie/dédoublonne, résout en parallèle, puis
refait `visit_block` pendant la fermeture (`:755–849,:955–964`). Un
lookahead ne peut surtout recouvrir que géométrie future et fermeture
courante ; lancé sur 48 workers déjà occupés, il peut aussi créer
contention CPU/mémoire. **Mesurer avant de promettre un gain** : par K,
temps mur/CPU et octets d'extraction, tri/unique, semis, géométrie,
scatter, second `visit_block`, fermeture/DSU et encodage ; `R_K,U_K,S_K`,
taille maximale de lot, longueur des chaînes, attentes/occupation des
workers, pic RSS simultané et débit mémoire. Comparer sur mêmes entrées
statique W48 actuel, fenêtre unique bornée puis lookahead borné, trois
répétitions, mêmes catalogues, condensés et comptes **par ordre**.
Compléter par oracles de petits nuages à égalités de niveau/coquilles
étendues, relecture des ordinals, mutations de cible/`before`, et fautes
injectées pendant extraction, fenêtre future et fermeture d'un lot :
aucune publication partielle, tous les workers joints, travail payé
conservé. Aucun G4 nouveau n'a été lancé pour cet audit.
