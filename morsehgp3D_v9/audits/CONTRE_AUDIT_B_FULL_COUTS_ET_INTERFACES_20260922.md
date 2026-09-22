# Contre-audit B — FULL : le tri n'est pas le seul coût aval

22 septembre 2026. Lecture indépendante du constructeur
[`full_ball_tower.hpp` v7](../../morsehgp3D_v7/src/forest/full_ball_tower.hpp),
de son [`ShellTable`](../../morsehgp3D_v7/src/forest/local_plateau.hpp),
du [certificat de couverture](../../morsehgp3D_v7/src/forest/full_coverage_certificate.hpp)
et des [mesures statiques 8k/16k/32k](../../morsehgp3D_v7/docs/RESOLUTION_STATIQUE_CPU_20260911.md).
Les mesures sont **v7/u16**, pas une qualification v9/u18 1 mm ou G4.
Le commit v9 `d2700314` arrivé pendant cette contrelecture porte le
constructeur FULL : les limites `u≤12`, `shell_mask:u16`, `contains_[2^u]`,
double appel de `visit_block` en mode statique et recherche d'intrus
depuis la racine **y sont toujours présentes**. La chaîne v9 et ses tests
font l'objet du contrôle distinct ci-dessous.

## Ce que reçoit réellement le constructeur

Le constructeur v7 suppose un **catalogue de boules globales exact et
complet**, avec clé, niveau exact, intérieur et coquille globaux. Il ne
découvre pas ces boules. Il trie les BallKeys et les niveaux, valide les
populations, puis programme chaque boule sur ses ordres utiles
`K∈[p+q_min−1,min(Kmax,p+u)]`. Pour chaque ordre, il ferme les lots de
même niveau chronologiquement. Le DSU des lots et les verticales ne sont
donc pas interchangeables avec un simple tri parallèle de faces : il faut
également trouver les **parents exacts** de chaque composante locale et
conserver les contributions de couverture. La complétude du catalogue
est une précondition de ce raisonnement, pas une conséquence du constructeur.

| Poste v7 | Travail apparent et portée | Verrou pour v9 |
| --- | --- | --- |
| Catalogue, niveaux, programmes | Tris `O(B log B)` et jusqu'à `K·B` occurrences programmées ; coût sensible au nombre `B` de boules. | `B`, octets des clés u18, intérieur/coquille et sorties doivent être mesurés sur LiDAR. |
| Quotient d'une coquille de taille `u` | `contains_[2^u]` dès la préparation ; `rank(K)` alloue deux tableaux `2^u`, balaie les masques et conserve chaque sommet réduit. | v7 **refuse `u>12`**. La note sur le quotient sphérique propose une cible `O(u²)`, non un port de ce code. |
| Représentants et parents | Une requête par représentant, puis MEB, recherche de clé, recherche d'intrus dans l'index global et descente jusqu'au terminal. | Une recherche d'intrus peut visiter `O(n)` nœuds ; la terminaison lexicographique rayon/coquille ne borne pas ce total sous-quadratiquement. |
| Lots et verticales du constructeur | Fermeture ordonnée, tris locaux de parents et DSU ; historique union-find monotone. | Pas de carré caché évident **hors volume d'entrée/sortie** ; mesurer les grands lots et la mémoire, sans attribuer ici le coût des lecteurs. |
| Lectures du certificat final | `full_coverage_at` balaie tous les nœuds et contributions à **chaque requête** ; une racine historique peut parcourir une chaîne. | `Q=Θ(N)` lectures peuvent coûter `Θ(N²)` ; c'est un coût de **lecteur**, non du constructeur si le contrat ne demande pas ces lectures. |

Sources précises : `local_plateau.hpp:132–176,190–216` pour les masques ;
`full_ball_tower.hpp:458–524` pour l'admission et les programmes,
`:543–570` pour l'intrus, `:751–839` pour les résolutions statiques,
`:934–974` pour `visit_block`/`prepare_block`, `:1020–1078` pour les lots ;
`full_coverage_certificate.hpp:268–306` pour les lecteurs. Les numéros
de ligne décrivent la v7 lue, pas des garanties d'API future.

## Deux obstacles que le port d'un seul noyau ne retire pas

**Grande coquille et format.** Le plafond `u≤12` est vérifié dans
`validate_catalogue`, et l'expansion v7 a la même limite. Le format
`FullCoverageRef.shell_mask` est `u16` et la banque refuse `u>16`.
Même si la [construction compacte des composantes](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
était correcte et rapide, il faudrait changer **ensemble** validation,
émission des représentants, contribution datée et lecture de couverture.
Un masque `u16` décalé au-delà de 15 n'est pas un raccourci acceptable.
Le lemme « `u≥2t−1` implique contribution de coquille vide » ne supprime
ni les représentants, ni les parents, ni les clés du catalogue.

**Résolution répétée et résidence.** En mode statique, v7 exécute
`visit_block` une première fois dans `prepare_static_order` pour extraire
et trier **toutes les demandes de l'ordre K**, puis à nouveau dans
`prepare_block` pendant la fermeture des lots ; une coquille non régulière
recalcule alors `rank(K)`. Seule la résolution géométrique des **clés
uniques** est parallélisée ; extraction, tri, scatter des cibles et
fermeture restent essentiellement ordonnés. Les tableaux `requests`,
`groups`, `seeds`, `static_targets` résident côté hôte même avec le
callback batch externe. Le port GPU des seules MEB ne retire pas ces
tableaux ni la deuxième visite.

Le batch externe constitue en outre une **frontière de confiance** :
`prepare_external_batch` vérifie ordinal, domaine de la `BallId`, fenêtre
de K et antériorité du niveau, mais ne recalcule pas que la cible est
réellement la terminale géométrique du représentant. Dans la gate v7,
le mutant `wrong_admissible` est réfuté parce que le **callback de test**
rejoue `scalar.static_terminal`, non parce que le Builder en déduit la
géométrie de ces quatre contrôles structurels. Ce n'est pas un défaut du
contrat v7, qui suppose le backend correct ; pour un backend GPU v9,
prévoir une qualification différentielle CPU sur cas dégénérés et grands
lots, ou un certificat terminal vérifiable avant publication de la forêt.

Sur l'uniforme u16 historique à `n=32 000`, K1..10/s8, le reçu statique
compte **45 208 799 demandes** hors K1, **22 503 000 clés uniques**,
**18 244 853 MEB** et **1 600 773 173 tests de supports** ; les
capacités statiques retenues culminent à **1 235 849 528 octets** et
le RSS du processus à **9 109 204 KiB**. Ces nombres ne sont ni un pic
VRAM ni des mesures v9 ; ils montrent que « trier instantanément les
faces » n'élimine pas le travail de résolution et de résidence. Dans
**cette seule famille uniforme**, MEB/supports/demandes croissent avec
exposants locaux proches de 1,05–1,08 aux doublements 8k/16k/32k :
cela ne promet pas la même croissance sur une trame LiDAR à 1 mm.

## Raccord réel v9 `d2700314` : portée des portes

`src/chain/tower_chain.cpp` construit déjà un catalogue canonique de
présentations q2/q3/q4, recalcule chaque clé avec les formules de la tour,
recompte exactement l'intérieur et la coquille de **chaque boule émise**,
vérifie `q_min` sur une coquille étendue, puis appelle FULL. Une coquille
de taille >12 produit un statut `unsupported_degeneracy` explicite ; la
trame sans sol 08/000000 rapportée dans `PASSATION.md` n'a pas exercé ce
refus (`max_shell=5`). Son premier essai 1 mm K5/W8 vaut environ **131 s
mur**, dont **117 s q3/q4** et **11 s FULL**, mais il est **exploratoire,
sans reçu** et ne constitue pas une preuve de croissance ou de contrat.

Le recoupement certifie la **cohérence des clés présentes** ; il ne peut
pas constater qu'une BallKey entière manque au générateur. Le juge T2
compare un inventaire exhaustif à `n≤14`, ce qui donne une porte bornée
indispensable, pas une complétude démontrée au-delà. Surtout, dans
`tests/chain/chain_census_tower_gate.cpp`, le chemin jugé pose
`run_tower=false`, puis invoque `build_full_ball_tower` **directement**
sur le catalogue obtenu. Il ne teste donc pas le chemin public
`run_tower_chain(..., run_tower=true)`, ni le digest et la propriété du
résultat FULL. Une porte directe doit comparer la tour publiée par cet
appel public à l'oracle, en W1/W4 et s8/s10/s12, puis tuer une mutation
qui supprime une boule q4 unique : le recoupement seul ne doit pas être
présenté comme une preuve de non-omission.

Les 20 CTests du premier commit comprennent surtout les portes héritées
de la tour et ce juge borné ; les 24 unités de compilation du générateur
portées en v9 n'ont **pas encore** leurs portes dédiées. Les bornes
arithmétiques u18 de la tour et les extrêmes du raccord restent à
requalifier, indépendamment des reçus v8. Le statut public demeure
`not_claimed`.

**Rejeu indépendant ciblé.** Un worktree temporaire détaché au commit
`d2700314`, sans modifier le `main` partagé, a compilé en Release avec
Boost obligatoire ; `ctest -L gate --parallel 4` a passé **20/20** portes.
Le même commit compilé sous Clang 18 ASan/UBSan en `RelWithDebInfo`
a passé **20/20** portes en un second rejeu ; aucune nouvelle borne
arithmétique u18 n'est déduite de ces cas bornés.
Le probe public a aussi produit une tour K1..5 sur les **dix premiers
sites** du fichier sans sol 08/000000 à 1 mm : W1/s8, W4/s8, W1/s10 et
W1/s12 donnent le même digest `f26d0dd2bcc3f4d0` et les mêmes 55
boules, avec zéro q4 sur ce préfixe. Ce diagnostic vérifie un petit
chemin `run_tower=true`, **pas** l'oracle, la complétude q4, le contrat
de trame entière, la croissance ni un temps représentatif. Un essai
`--no-tower` sur les mêmes dix sites retourne lui aussi le statut
`complete_relative`, mais zéro ordre et digest nul : la porte de lecture
des reçus doit contrôler ces champs, pas le seul statut.

Deux précautions pour les prochains reçus : `run_tower_chain` rend aussi
`status=kComplete` avec `run_tower=false` (catalogue seul, zéro ordre et
digest nul) ; un lecteur de contrat doit exiger `run_tower=true`, tous
les ordres `K=1..Kmax_effective` et la sortie FULL correspondante. Le
probe accepte `--grid=<libellé>` sans contrôler l'origine ni le pas des
coordonnées ; le mot `1mm` dans son JSON n'est pas un certificat de
préparation. Lier le hash des octets d'entrée et le manifeste du masque
à la ligne de qualification, et mesurer lecture/préparation séparément.
Le parseur du probe accepte en outre `K=4294967297` : la conversion
ultérieure en `unsigned` le tronque à `K=1`, l'appel public réussit et
le JSON annonce `options.K=1` (reproduit sur un diagnostic à deux sites).
La bibliothèque valide K **après** conversion : corriger le lanceur
par un entier vérifié avant tout rétrécissement, puis ajouter un gate
de refus CLI. Ce n'est pas une erreur de géométrie du cœur.

La chaîne matérialise en outre chaque présentation dans un vecteur de
worker, puis dans un second vecteur `all` avant tri global ; elle alloue
`balls(unique)` et peut en copier le catalogue si demandé. Il n'y a
encore ni streaming borné, ni pression de retour entre q3/q4 et cette
fusion. Ce n'est pas une borne quadratique prouvée, mais un poste
d'octets et de temps incontournable pour le contrat multi-million :
publier les pics simultanés et les copies, pas seulement la capacité
finale `BallData`.

## Interface et expériences recommandées

Le port FULL doit recevoir par BallKey **une seule identité canonique**,
son niveau, `q_min`, l'intérieur et la coquille **globaux**. Pour chaque
ordre utile, la couche locale doit fournir un représentant exact de
chaque composante stricte et sa contribution, non toutes les facettes ni
`reduced_members`. Les IDs d'une grande coquille et les références de
contribution demandent un format possédé non borné par le masque `u16`.
Les incidences/supports utiles aux contrôles différentiels peuvent vivre
dans un flux séparé du chemin industriel, à condition de prouver la
couverture de toutes les BallKeys et de vérifier l'oracle sur petits nuages.

Avant une route GPU, instrumenter **par ordre et par lot** : `B`,
histogramme de `u`, appels au quotient, représentants émis `R`, clés
uniques `U`, requêtes d'intrus, nœuds visités, longueurs maximales des
descentes, MEB/tests exacts, cibles par semis, octets des quatre tableaux
statiques, octets du certificat et temps CPU/mur de chaque sous-phase.
Séparer coût constructeur, sérialisation éventuelle et lectures du
certificat. Une expérience croisée utile garde une quantité de sorties
comparable tout en passant de coquilles `u=12` à `13,16,18,19` **après
port** ; la première taille est comparable à l'oracle v7, les suivantes
ne le sont plus exhaustivement. Tester aussi une chaîne de successeurs
longue avec de nombreuses requêtes de lecture, en la comptant uniquement
au poste lecteur.

Fixture de lecteur sans confusion possible : à K1, n racines sont n
singletons. Interroger `full_coverage_at` sur chacune écrit seulement
`n` IDs au total, mais alloue et parcourt `n` nœuds **n fois**, soit
`Θ(n²)` travail hors constructeur. Une série de coupes sur une longue
chaîne de successeurs isole pareillement le coût des racines historiques.

Une voie plausible est de préparer les représentants et résoudre leurs
clés **par fenêtres de lots de même niveau**, avec semis/catalogue
immuables globaux, puis de fermer les lots dans l'ordre exact. Elle
réduirait la résidence des demandes ; elle ne gagne du temps que si la
deuxième construction du quotient est évitée **sans** copier tous les
représentants de la tour. Mesurer le compromis temps/octets et démontrer
l'identité des parents, contributions et verticales avant de la porter.
La parallélisation massive peut distribuer les résolutions indépendantes
`U`, mais ni un `O(n)` par intrus non maîtrisé ni un catalogue incomplet
ne sont réparés par davantage de workers.
