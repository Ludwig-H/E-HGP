# Reprise du 22 septembre : domaine u18 et arrêt anticipé de l'atlas

Cadre : `exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u18_input_only`, `public_status=not_claimed`. GCP non utilisé.
Base reprise : `a74e90f2`. Les briques float32 qualifiées et les brouillons
globaux non suivis sont préservés, sans transfert de qualification.

## Ce qui a été repris

Le développeur précédent a raccordé le rejet des graines q3 par l'atlas q4,
une file bornée de plages de rectangles et des chronos par worker. Il a
ensuite élargi le moteur entier à 18 bits pour la grille optionnelle de 1 mm.
Restait notamment une série non commise de 42 portes de test : anciens cas
16 bits conservés et cas 18 bits séparés, avec assertions de non-vacuité.

Les six nouvelles mesures `ground_18bits_20260922/u16_identity` concernent
encore les anciennes entrées à 2 cm (trois trames × K5/K10, huit workers).
Ce sont des contrôles de non-régression, pas une campagne 1 mm.
Les contrats bruts entiers, float32 par défaut et plusieurs séquences restent
ceux d'`AGENTS.md`. Une priorité technique sur la voie grille/sans-sol ne
vaut pas modification de ces contrats.

## Correction numérique

Le stockage `int32_t` peut représenter des coordonnées bien plus larges que
le domaine certifié [0,262143]. Un nuage préparé les refusait déjà, mais
plusieurs fabriques publiques autonomes ne le faisaient pas. Des entrées
forgées pouvaient donc déborder avant de revenir au pipeline validé.

La reprise contrôle points et boîtes aux fabriques concernées, ainsi que
les formes et centres rationnels publics de l'atlas. Les opérations chaudes
internes emploient les points/boîtes déjà certifiés ; les requêtes ponctuelles
`noexcept` gardent leur précondition de domaine explicitement documentée.
Cela ne constitue pas une promesse d'accepter tous les entiers `int32_t`.
Une porte distincte couvre aussi INT32_MIN/MAX, INT64_MIN/MAX et INT128_MIN/MAX.

## Optimisation structurelle : cesser de classer une cellule déjà rejetée

Pour une cellule fermée de centres, un bloc est intérieur seulement si sa
borne supérieure de puissance est **strictement négative** partout.
Les blocs consommés sont disjoints ; leurs populations s'ajoutent au compte
hérité, sans recompter les ancêtres.

Si ce minorant atteint K−1, toutes les boules concernées atteignent le seuil
de rejet q3 (K−1) et dépassent celui de q4 (K−2). On peut donc interrompre la classification du
reste de la cellule. Il ne faut pas inventer une partition complète ni
utiliser ce minorant comme compte exact dans un balayage avec sorties.

La fabrique renvoie exclusivement l'un de ces objets :

- un fragment complet, avec son compte exact et toute sa frontière active ;
- un certificat terminal de profondeur, sans fragment utilisable par le balayage.

L'option `Q4LocalOptions::saturate_deep` reste désactivée par défaut.
Les contacts (puissance nulle) ne contribuent jamais au minorant. Un budget de travail
intermédiaire épuisé conserve les blocs indécis : ce n'est pas un rejet.
La population non visitée est publiée comme telle, jamais comme un nombre
de tests prétendument économisés. Les allocations et copies déjà payées
restent comptées même lorsque le fragment temporaire est jeté.

## Preuves, mesures et suite

Première passe de reprise : 78/79 portes courtes passent. La porte q4 locale
échoue sur le nouveau juge de centre : il comparait la coordonnée x à la
deuxième coordonnée rationnelle, et y à la première. La base A/B du juge et
les équations de Cramer du moteur établissent l'ordre direct. Le test est
corrigé ; l'échec et sa source sont conservés dans
`receipts/u18_resume_20260922/preflight_center_oracle/`.

La première capture complète Release (`release/`) passe 134 tests sur 136
exécutés, avec trois exclusions historiques explicites. Deux lecteurs de
mutations échouent : l'inventaire affine refuse les nouveaux champs u18 ;
le mutant de coût est bien tué par une assertion de préfixe, mais le lecteur
attendait celle d'une fixture ultérieure. Ces échecs ne sont pas des
divergences géométriques du moteur. La capture SAN commencée en parallèle
est interrompue explicitement après ce constat et conserve son signal 2.
Les répertoires `release/` et `sanitize/` restent des essais échoués,
jamais l'autorité d'une qualification réussie.

La reprise R2 reconstruit les sources gelées dans des builds neufs, après
correction de ces seuls juges Python. Elle vérifie les dépendances avant
compilation et à la fermeture, puis les fichiers effectifs aux lectures
LIVE. Ne pas promouvoir le préflight ni les anciens exécutables à cette
qualification. Les lecteurs nécessitent les builds et leurs dépendances
locales ; les reçus seuls ne sont pas une archive d'exécution autonome.

Le diagnostic 8k/16k/32k de l'arrêt anticipé porte sur **une arête fournie**.
Il ne prouve pas la croissance du générateur LiDAR entier. Aucun nouveau
résultat sous-quadratique global, FULL, GPU ou contrat d'une seconde n'est
acquis par cette tranche. Les coupes spatiales appariées, le transport dédié
des statistiques de saturation et les essais G4 restent à qualifier.

L'audit des coûts confirme que l'atlas domine encore le tri : la capture
historique scène0/K10 compte 10,718 milliards de bornes de blocs,
24,196 milliards de tests ponctuels et 18,307 milliards d'IDs copiés,
contre 593 millions de comparaisons de tri. Après mesure de cette option,
les pistes suivantes sont le census q3 dans les fragments exacts déjà
construits, puis le catalogue canonique réel et l'aval de la tour.

## Première référence sans sol entière à 1 mm

`receipts/u18_resume_20260922/ground_1mm_first/` mesure la trame
**08/000000 sans sol entière**, soit **39 885 sites**, pas un préfixe.
K5, s8, huit workers, Local28, rejet q3 par atlas activé ; la nouvelle
option `saturate_deep` demeure **désactivée**. Une seule répétition sur
l'hôte local partagé, sans autre test lourd lancé pendant cette commande.

- Temps mur : **104,63 s** ; CPU cumulé : **812,82 s** ; RSS : 15 124 KiB.
- Flux émis : **691 284 supports q3**, **158 496 supports q4**.
- Atlas : 3,252 milliards de bornes de blocs, 7,316 milliards de tests
  ponctuels et 5,547 milliards d'IDs de frontière copiés ; balayage q4 :
  163,678 millions de comparaisons de tri. Le coût amont domine encore.

La sonde R1 a été construite depuis les mêmes sources C++ que la reprise
R2 ; les échecs R1 décrits ci-dessus concernent ses lecteurs Python, pas
sa compilation. Le reçu v2 ferme sources/binaire/entrée avant/après et
ses lectures LIVE normal/−O passent. La sélection ne comporte aucune paire
W1/W8 ni référence de sorties : elle ne revendique donc aucune nouvelle
identité parallèle. Les champs de sortie sont des digests, pas un oracle
exhaustif de cette trame.

Ce temps comprend lecture et préparation partagée du nuage/index, front,
candidats et collecte du digest. Il exclut la segmentation et préparation
hors ligne déjà capturées séparément, le catalogue, l'aval des hiérarchies
et le GPU. Ni contrat principal brut, ni FULL, ni gain de la saturation,
ni mesure de croissance ne se déduisent de cette ligne unique.
