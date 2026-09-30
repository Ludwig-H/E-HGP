# Essai borné sur les listes réellement atteintes

Ce reçu ne lance aucun générateur ou benchmark. Le témoin R2 à seize sites
est une preuve de géométrie, pas une feuille atteinte : à K5/M16, sa feuille
initiale contient toutes les ancres, donc les moments y donnent zéro crédit.

Le prochain essai doit capturer le **S recadré et cand réellement transmis à
`enumerate_leaf`** (`generator.cpp:619`), avant toute optimisation. Garder
les bornes T6 entières demi-ouvertes, IDs, poids, K/M/max_leaf et hashes de
nuage. Le certificat utilise la fermeture de S. Publier les feuilles larges
refusées et stagnantes ; un arrêt de capture rend le lot diagnostic partiel,
jamais une trame entière qualifiée.

1. Choisir quelques petites entrées figées de plus de M sites, génériques,
   groupées et anisotropes ; traiter tous leurs nœuds/feuilles. Ensuite seulement,
   employer une trame existante lors d'une campagne autorisée. La capture est
   une instrumentation temporaire dans une extraction isolée, pas un export
   de listes ajouté au chemin chronométré final.
2. Mesurer Σ|cand| et les ancres hors **S fermé** avant de préparer les moments.
   Une ancre sur un bord compte comme dedans et ne peut fournir σ<0.
   Si aucune ancre n'est éligible, arrêter cette piste sur ce lot sans faire
   passer les témoins synthétiques pour un gain du générateur.
3. Préparer au plus sept groupes par feuille : groupe complet et retraits
   d'un extrême XYZ, égalités par ID. Une passe accumule W,Σwz,Σw‖z‖² et
   les six extrêmes ; les groupes dérivés se calculent par soustraction.
   Retirer un ID de site implique de retirer son poids réel entier.
   Pas de sélection par tuple ni recherche de sous-populations quadratique.
4. Pour chaque ancre éligible, tester chaque groupe dans une unité commune
   T6 : `σ=max_S [V−W‖a‖²−2(M−Wa)·c]`,
   `R²=max_S ‖a−c‖²`. Maxima fermés calculés séparément aux coins par les
   signes des coefficients. Seul `σ+θR²<0` rejette ; égalité ou R²=0 reste
   indécis. θ3=K−2, θ4=K−3 en feuille unitaire ; θ=K−1 sinon.
   Si θ<0, la voie d'arité correspondante est déjà impossible : la couper
   sémantiquement, sans attribuer ce refus aux moments.
5. Conserver le meilleur crédit de groupes recouvrants : jamais leur somme,
   ni leur addition à Dom sans certificat de disjonction. Appliquer les
   refus uniquement à l'éligibilité comme support q3/q4. Le nuage, q2 et
   le census restent complets ; ne pas initialiser un census avec ce crédit.
6. Recouper chaque nouveau rejet sur les tuples du petit lot initial et
   comparer le catalogue exact complet au baseline, poids/contacts compris.
   Cette vérification exhaustive bornée appartient au diagnostic, pas au
   futur chemin de sélection. Capturer toutes les commandes et refus.

Publier : feuilles, Σ|cand|, ancres éligibles et sur bord, groupes/masses,
préparation, tests de moments, refus q3/q4, refus déjà couverts par Dom et
refus nouveaux ; paires/triplets/quadruplets résiduels, census/émissions,
temps mur total et pic mémoire. Comparer à la même extraction/entrée avec
moments désactivés, indépendamment des statistiques de workers.
Le calcul pairwise de Dom reste nécessaire au chemin q2 et aux crédits
ponctuels : mesurer ses tests/temps et ne pas annoncer sa disparition
par les seuls masques q3/q4. Aucune préparation de groupe par paire.

Le coût supplémentaire visé est O(Σ_Q |cand(Q)|) pour un nombre fixe de
groupes. Cela ne borne ni le nombre de feuilles ni l'énumération résiduelle.
Un gain sur les masques seuls ne clôt pas le coût total et n'autorise aucune
extrapolation 8k/16k/32k ou LiDAR avant les mesures correspondantes.

Largeurs à transporter : pour un Cloud préparé, W≤poids total<2^32.
En u18/T6, ancres, témoins et S recadré ont des coordonnées <2^24 ;
`|S(c)|<9·2^80<2^84`. Les moments et leurs produits peuvent donc être
calculés en i128, avec promotion **avant** multiplication ; i64 ne suffit
pas aux masses globales possibles. Pour un futur u32/T6, la même dérivation
donnerait `<9·2^108<2^112`, sous ces préconditions inchangées ; ce n'est pas
une qualification du port u32, du filtre D ou d'une implémentation native.
Un nuage forgé dont poids/IDs divergent ne bénéficie pas de cette borne.
