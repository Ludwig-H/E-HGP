# Contrelecture mathématique adverse de l'audit e02

Pin principal : `e02a6c235bc4a706519cdaa15f4b1465a6275eba`. Un petit lot ultérieur `8f68622b2181e2537280ca09275363a46f2f34dd` concerne seulement les catégories des démos ; il est capturé séparément. Cadre `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Aucun natif, build, fit, GCP, workflow, production ou ancienne capsule modifiée.

**Conclusion :** aucune réfutation nouvelle des preuves FULL/H3/B3 de l'audit e02. Les hypothèses déjà explicites sont indispensables ; les tests ne les remplacent pas. Une aide nouvelle est établie ci-dessous : B garde le même plafond L6 que H, avec une marge supplémentaire exactement bornée. Deux gardes prospectives précisent le domaine de la qualification et des réciproques EOM. Ce sont des limites de transfert, pas des bugs imputés au moteur courant.

## 1. Résultat nouveau : B ne dépasse pas le plafond L6 d'H

Conserver les hypothèses finies de H3/B3 : mêmes sites distincts de poids un, k≥2, qualification fixée m≤n, premier point qualifié p=(C,t′) atteint, contacts fermés, racine prolongée. Poser ρ=d_k(x), self compris, D=e−t′. Le critère B est sB=m(H,Q), H=Up(p,e) et Q=(core,ρ).

Choisir y∈C avec ℓ=|y−x|≤t′. Les populations k des boules B̄(y,t′) et B̄(x,ρ) donnent ρ≤t′+ℓ. Tout centre z=x+τ(y−x) est dans L_k au rayon

    min(ρ+τℓ, t′+(1−τ)ℓ).

Le maximum de cette quantité sur [0,1] est au plus

    S=max(t′,(ρ+t′+ℓ)/2)≤t′+ρ/2.

Ce S est ≥ρ : si t′≥ρ c'est immédiat, sinon ρ≤t′+ℓ le donne. Le segment relie donc le premier propriétaire au cœur déjà actif, sans exiger que ce cœur soit qualifié : m(p,Q)≤t′+ρ/2. L6 donne aussi e=t′+D≤t′+ρ/2. Par remontée,

    sB=max(e,m(p,Q))≤t′+ρ/2,
    0≤sB−e≤ρ/2−D.

Ainsi le critère B n'ajoute pas un deuxième plafond pessimiste après celui d'H. Cela ne signifie ni B=e ni un faible retard en pratique.

**Fibre limite sans rival.** x=(0,0), y=(2N,1), z=(2N,−1), N≥1, k2,m3. La MEB du triangle a centre (N+1/(4N),0) et rayon t′=(4N²+1)/(4N) ; ses trois poids sont strictement positifs. À n=k+1, la seule composante qualifiée commence à la MEB totale : e=t′ et D=0. Pour x, ρ=√(4N²+1), sB=ρ. Le rapport

    (sB−e)/(ρ/2)=2−√(1+1/(4N²))

tend vers1. Le supplément B peut donc approcher ρ/2 même sans ambiguïté qualifiée. Cette limite concerne la famille géométrique non bornée ; le profil entier u21 fixé possède un domaine fini, aucune constante optimale u21 n'est revendiquée. N=1,2,5,20 sont rejoués exactement.

**La constante 3 de B3 n'est pas remise en cause.** La preuve exige l'alignement fort m_Y(H_Y,φH_X)≤e_X+3ε, le pont core ≤ρ_X+2ε et le transport du meeting ≤sB_X+ε. La seule stabilité des valeurs e_i/u_ij de pendaisons arbitraires ne suffirait pas : deux ancres de hauteur1 sur des feuilles A/B qui fusionnent à100 ont les mêmes valeurs singleton ; si le cœur est sur A, B peut valoir1 ou100. Ce garde abstrait n'est **pas** un P1, ni un nuage réalisé, ni une réfutation de H3. Il explique pourquoi l'alignement géométrique explicite est nécessaire.

## 2. Deux fibres qui empêchent des transferts illégitimes

**Une naissance n'implique pas population=k.** Croix (±1,0),(0,±1), translatée dans le domaine : k3, boule centrale λ=1, q_min=2, p=0, U4. Chaque triple contient une paire antipodale et a MEB de rayon1 ; aucune trace stricte. La composante naît donc à1, avec quatre sites, sans ancien prédécesseur. T3 donne population=k comme condition suffisante de naissance, pas comme caractérisation. A/B concordent sur cette naissance non régulière ; aucun défaut du code actuel n'est établi.

**Root birth n'est pas un plafond de toutes les dates.** Nuage traduit de (3,4),(5,0),(3,−4),(−5,0), k3 : root birth16 en niveau carré ; les trois premiers sites ont couverture brute16, le dernier25. La qualification m4 commence à25 pour tous. Les dates core incluent80, donc dépassent même les dates qualifiées. Refaire la distinction évite d'utiliser la naissance de la racine dans un majorant couvrant les incidences ou B. Une première assertion de notre lecteur confondait les quatre couvertures brutes avec les quatre couvertures qualifiées ; corrigée et conservée dans `history/`, sans défaut produit déduit.

**L'identité d'ordre est spéciale à m=k+1.** Sur X={0,2,4,6}, k2,m4, les cofaces {0,2,4} et {2,4,6} connectent Γ2 et qualifient ses quatre sites au rayon2. Aucun rival qualifié : e=2. La première couverture d'ordre4 vaut pourtant3, MEB de tous les sites. Il est donc faux de généraliser t′=α_(k+1) à t′=α_m pour m arbitraire. L'audit e02 ne fait pas cette généralisation ; le garde protège une API future qui mêlerait seuil de transmission, mcs et ordre de densité.

## 3. Réciproques EOM : formule vraie, domaine à conserver

Pour e=√t+√M−√q>0, avec t,M,q rationnels ≥0, poser d=t+M−q et Δ=d²−4tM. La formule de l'audit e02 est correcte. Si Δ=0, q=(√t±√M)² ; la branche somme rend e=0 et est exclue. La branche différence donne e=2√min(t,M), avec min(t,M)>0. Ainsi ni un changement de signe ni un cas singulier oublié n'a été trouvé.

Le cube de l'inverse reste dans les classes impaires {t,M,q,tMq}. Les dépendances et zéros ne font que les réduire. Une arithmétique multiquadratique séparée, sans facteurisation et sans la classe privée Rad, vérifie 164 dates positives, dont16 singulières, les égalités, z=1/z=3 et ≤4 classes. Les30 entrées non positives sont explicitement hors contrat, pas passées à l'inverse.

**Garde prospective de maturité.** Avec θ=1/2,

    e=√2+√5−√3, ρ=√7>e,
    sE=(e+ρ)/2>e.

Ces données radicales satisfont les bornes scalaires ρ≤2√t et e−√t≤ρ/2, mais aucune réalisation géométrique de ce profil n'est affirmée. Les quatre générateurs √2,√3,√5,√7 sont indépendants. Une résolution rationnelle dans le corps de degré16 donne **huit** classes non nulles pour 1/sE et pour son cube, et en vérifie les produits exacts. Le helper à trois radicaux ne peut donc pas être appliqué tel quel à une maturité générale Eθ. Les dates A et B/C, qui choisissent entre une date H et un rayon pur, restent dans son domaine. Le texte e02 annonce sa formule conditionnellement à la forme de e : aucune formule actuellement publiée n'est fausse.

## 4. Audit des implications et hypothèses

| Preuve ou implication attaquée | Résultat et hypothèse indispensable |
|---|---|
| M1/M2 | F fini non vide ; MEB unique ; support strict ≤4. Présentation non stricte≠support minimum, rayon seul≠clé de boule globale. |
| T1 | Nerf **fini** de convexes fermés/ouverts ; échanges via k+1 ; intersections strictes testées aussi sur les cofaces. L'extension infinie ne suit pas de cette preuve finie. |
| T2 | Séparabilité de la frontière et **tous** les intérieurs pertinents ; représentant I∪A. Poids nuls d'une présentation ne prouvent pas un support strict. |
| T3/T4 | Racines globales prises avant tout plateau ; un prédécesseur=continuation, zéro=naissance. Pièces locales≠racines globales. U4/k3 réfute seulement l'optimisation hypothétique «birth iff pop=k». |
| T5/T6 | Strict décroissement, mémo ouvert a>λ_b versus fermé a≥λ_b ; terminal éventuellement différent mais même classe à la date initiale. Suffisance constructive conditionnelle au catalogue complet et à **chaque** morceau rencontré ; aucune borne globale de travail. |
| P1/P2/P3 | self compris ; core et cover distincts. Le minimum de la preuve P3 doit porter sur les parties contenant x **et** avec centre dans C. Toutes les populations fortes et leurs propriétaires courants, pas uniquement la naissance ou la première incidence. |
| P4 / plusieurs k | Laminarité fixed-k. Ancien garde six sites 0,3,6,18,19,20 revérifié : à r7, bloc k2={0,3,6}, bloc k3={6,18,19,20} ; P1 qualifié n'enlève pas le croisement. |
| P5/H3 fort | Géométrie appariée, mêmes IDs et mêmes masses fixes ; φψ=Up(2ε), profils qualifiés transportés dans **les deux sens**. Un simple transport unilatéral ou stabilité scalaire ne suffit pas. Insertion revérifiée : {0,2000,4000} → ajout1, e_0 et B_0 changent2000→1000. |
| B3 | Meeting daté conservant la future naissance de LCA ; maximum des deux entrées. Garantie3 conditionnelle aux alignements forts réellement démontrés, pas aux IDs de nœuds ou à l'IoU. Nouveau plafond L6 ci-dessus. |
| Condensation | u_ij≥max(e_i,e_j), s_i≥e_i ; R_i statistique d'ordre de max(u_ij,s_j). Le facteur décrit les blocs admissibles, mais score=cohortes effectivement comptées : ne pas remplacer s_i par R_i sauf domaine A. Poids variables ne sont pas couverts par des poids fixes. |
| Raffinement EOM en z | Même arbre condensé, mêmes cohortes, mêmes racines admissibles, ties exacts parent. Après normalisation à la date du parent, son score décroît et l'optimum descendant croît. Une décision descendante stricte ne peut être remasquée par un ancêtre. C'est un raffinement des clusters sélectionnés, pas nécessairement de la partition totale si tout bruit est agrégé. Aucun transfert à A→C, κ1→κ2 ou à un arbitre forcé au budget. |
| Stabilité de la sortie plate | Dates stables ne garantissent pas une antichaîne stable ; il faut une marge de score **et** correspondance de l'arbre/cohortes condensés. Ni la stabilité de H ni une monotonie de rappel κ ne l'impliquent. |
| Statistique / thèse / infini | Best-node exploratoire≠sélection plate/consistance. Fixer λ=r^−z n'établit pas le modèle d'échantillonnage, la dimension intrinsèque ni l'objectif de Lebesgue. H∞ mesurable et attained, Palm à λ/F fixes et convergence de fenêtres sont des hypothèses distinctes ; aucune n'est ajoutée implicitement. |

## 5. Lot ultérieur 8f : «réussite de hiérarchie» n'est pas une antichaîne

`choisir_bouts.py` déclare bien le diagnostic meilleur-IoU indépendant par objet, les quatre issues et la priorité win/loss/both_fail/both_ok, au même k. Cette lecture ne révèle pas de nouveau score faux. Toutefois même **tous best>1/2** ne garantit pas des nœuds simultanément sélectionnables : six points, vérité A4/B2, nœud B2 imbriqué dans root6. Les best sont (2/3,1), moyenne5/6 ; à mcs2 aucune feuille A singleton n'est admissible et la meilleure antichaîne globale atteint seulement1/2. Le vrai `outcome` extrait par AST classe ce cas «win» de hiérarchie, ce qui est conforme à son contrat. Les démos illustrent donc l'information disponible, pas encore un clustering plat qui en réalise tous les meilleurs scores.

**Contrôle réel complémentaire**, sans fit ni calcul natif : JSON émis par la session close `claudebouts2`, source b72fe8771, bout `b00_001470_velos_43_61`, 250 sites. Le fichier capturé est identique au membre de l'archive close (arrêt TERMINATED certifié). À k5, HGP best=(0,963768;0,710526), HDB=(0,818841;0,482143) ; les deux meilleurs blocs HGP ont133 et83 sites, **intersection0**, donc une antichaîne brute et un appariement un-à-un aux deux objets sont possibles. À k3 les blocs133/57 sont aussi disjoints. À k10, où les deux méthodes échouent, les meilleurs blocs HGP216/54 sont imbriqués (intersection54). Ce contrôle atteste la compatibilité de nœuds retournés d'un cas gagnant ; il n'exécute ni une condensation/mcs, ni EOM, ni une recherche de coupe commune. L'IoU publié n'est pas recalculé à partir de données KITTI.

## Rejeu et fermeture

931 gardes normal/−O concordantes, sur cinq familles limites + quatre membres de la fibre N, insertion, croisement qualifié, réciproques et domaine Eθ. Calculs Gamma/MEB bornés ≤8 sites ; AST arithmétique/dateB/scoring seulement ; aucun import scientifique privé ou fit. A/B actuels concordent sur les huit couples nuage/ordre limites. Les contrôles ne constituent pas une troisième preuve des théorèmes MEB/nerf.

`PYTHONDONTWRITEBYTECODE=1 python3 check.py` et `PYTHONDONTWRITEBYTECODE=1 python3 -O check.py`. Les sorties, un premier échec de lecteur conservé, sources avant/après et commandes sont dans la capsule. Les cinq sources privées sont inchangées entre captures. Le root SHA256SUMS inclut tous les fichiers et exclut seulement lui-même. Aucun reçu clos antérieur n'est retouché.
