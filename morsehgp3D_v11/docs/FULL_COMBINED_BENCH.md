# Raccord des options catalogue au banc FULL

Tranche préparée le3octobre2026, après `3dbfd1c32`. Elle n'hérite pas des
chronos catalogue seuls de4b8e04be6 ni d'une qualification native ultérieure.
Le banc reste CPU sur G4, coordonnées entières u21/u24, sorties FULL exactes,
trames sans sol entières et synthétiques entiers. Le contrat200ms reste ouvert.

## Masque et périmètre

`mhgp11_full_bench` accepte maintenant0..127. Les bits précédents restent
inchangés :1cacheJ2,2triindirect,4mémo65536,8descentes régulières Q4096/L48,
avec mémos privés4096 si bit4. Les nouveaux bits sont16front adaptatif,
32assemblage parallèle et64catalogue à une seule passe. Tous sont inactifs
par défaut. La surcharge FULL prépare déjà un Pool, comme l'exige la passe
unique ; un nouveau refus global reste un échec complet, pas une sortie tronquée.

Le masque catalogue rapporté vaut `(FULL & 3) | ((FULL >> 2) & 28)` ; son
interprétation diffère donc des bits mémo/forêt FULL. Le lanceur FULL possède
son domaine d'options ; le lanceur catalogue indépendant conserve0..15.
La nouvelle CLI Python accepte également tout le masque0..127.

La collecte FULLv7 vérifie dix durées domaine disjointes : prefix,count,
replay,fill,sort,level_scan,allocation,assembly,single_pass,compact. Leur
somme est bornée par le mur domaine, sans supposer que tous les coûts sont
instrumentés. Les données de l'arène distinguent nombre de blocs, capacité
utile allouée, métadonnées et nombres d'émissions/incidences compactées.
Elles sont des réservations Buffer, pas la RSS ou la fragmentation système.
La passe unique impose count/fill/replay nuls et la compaction complète ;
la voie historique impose deux passages géométriques et zéro travail d'arène.
Une borne mémoire mesurée inclut leurs coexistences avec les autres buffers.

Le chrono FULL reste index + domaine catalogue/lookup + forêts/verticales.
Cloud, Pool, lecture et sérialisation sont séparés ; préparation de grille,
segmentation du sol et hiérarchie de points sont hors de cette mesure.

## Calendrier comparatif

`bench/full_parallel.py --optimized-catalogue` publie le schéma de campagnev2,
avec27processus neufs K5/W48. Chaque entrée LiDAR passe en u21/u24 ; les
uniformes8k/16k/32k passent en u21. Chaque cellule compare, dans cet ordre :

| Mode FULL | Catalogue | Forêt |
|---|---|---|
|15|front fixe, deux passes, assemblage série|lanes et mémos privés|
|63|front adaptatif, deux passes, assemblage parallèle|identique|
|127|front adaptatif, une passe, assemblage parallèle|identique|

La comparaison63/127 isole l'option de stockage en une passe sur cette source.
La comparaison15/63 cumule front adaptatif et assemblage parallèle. Les
changements numériques communs à la source ne sont pas isolés par ces paires.
Un essai par cellule, ordre fixe : ni médiane ni robustesse temporelle présumée.
Le calendrier historique19essais reste le défaut du même pilote.

Les hashes sémantiques doivent coïncider entre profils, les octets à profil
fixé. Tout le travail payé des forêts doit être identique à options de
mémo/descentes fixées, y compris entre modes15/63/127. Le travail logique
catalogue de la génération garde sa propre qualification ; une passe ne
signifie pas deux fois le même travail réellement exécuté.

Le plan `full_combined_g4.json` garde850+180+530secondes sous la session3600s,
un budget de banc450s et60s par processus. Les omissions restent explicitement
budgétaires ; l'échec d'un mode ne supprime pas ses partenaires. Chaque
intention est persistée avant lancement, le résultat avant décodage, et le
payload courant est intégralement rehaché avant réemploi de son résumé.

Le nouveau collecteur est contretesté avec147tentatives simulées,69corruptions,
18petits décodages exacts, sept calendriers et deux interruptions :938contrôles
normal/−O. Cela qualifie son protocole Python, sans exécution native.
