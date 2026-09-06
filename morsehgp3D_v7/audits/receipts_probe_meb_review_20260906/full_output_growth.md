# Croissance de la sortie FULL : le raccord aux minima

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Pour chaque K fixé au moins égal à 2, des nuages réguliers de dimension ambiante trois peuvent posséder un nombre quadratique de minima FULL distincts.** Il s'agit bien de feuilles demandées par la sortie explicite, pas seulement de candidats Delaunay ou d'un catalogue intermédiaire. La famille asymptotique exige une précision croissante ; les témoins u16 ci-dessous sont finis. K=1 est différent : n feuilles et au plus n−1 vraies multifusions.

Ce complément indépendant étend à tout K fixé≥2 la preuve à K2 préparée en parallèle par le constructeur, avec une famille parabolique distincte et des ancres intérieures explicites. Les [jalons de performance](../../docs/CONTRAT_PERFORMANCE.md) restent ceux du dossier principal.

## Pourquoi une boule strictement Gabriel donne une feuille

Le manuscrit définit les sommets de Gamma comme les facettes de cardinal K, isolées incluses : définitions 21–22, PDF 84–85, pages imprimées 58–59. Sa définition 28, PDF 113, page imprimée 87, porte la condition de miniball Gabriel. Le [raccord FULL principal](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md#11-hiérarchie-complète--minima-puis-fusions) établit explicitement la naissance correspondante.

Soit F de cardinal K dont la miniball B, de rayon r, ne contient aucun point étranger, même sur sa coquille. Toute extension F∪{z} de même rayon aurait, par unicité de la miniball de F, exactement B ; or z est strictement extérieur. Toutes ses cofaces ont donc un rayon strictement supérieur. À r, F est un sommet isolé de Gamma et constitue une vraie feuille FULL. Deux labels F distincts restent deux feuilles, même s'ils partagent des points.

À K=1, les arêtes Gabriel ont cardinal 2 : elles sont des connexions candidates, pas des feuilles. Le manuscrit distingue déjà MST et Gabriel, définition 3 et fait 1, PDF 41–42, pages imprimées 15–16. Une forêt issue de n feuilles dont chaque fusion possède au moins deux parents contient au plus n−1 nœuds de fusion. Une borne quadratique du graphe Gabriel ne devient donc pas une borne de sortie FULL à K=1.

## Construction rationnelle directe en dimension trois

Choisir m≥1, h=1/(4m²), t_i=ih pour 1≤i≤m et ε=mh=1/(4m). Définir deux courts arcs paraboliques par :

$$A(t)=\left(1-\frac{t^2}{2},t,0\right),\qquad B(s)=\left(\frac{s^2}{2},0,s\right).$$

La puissance de z pour la boule de diamètre A(t)B(s) est π(z)=(z−A(t))·(z−B(s)). Pour un autre point A(u), le développement exact donne :

$$\pi(A(u))=\frac{(u-t)^2}{2}+\frac{(u^2-t^2)(u^2+s^2)}{4}\geq |u-t|\left(\frac{|u-t|}{2}-\varepsilon^3\right)>0.$$

En effet, |u−t|≥h, |u+t|≤2ε, u²+s²≤2ε² et h/2>ε³. La même identité, en échangeant les paramètres, vaut pour un autre B(v). **Les m² paires croisées sont donc toutes strictement Gabriel**, sans utiliser une borne de Delaunay. Le milieu des deux sites est une combinaison strictement positive 1/2,1/2 : chaque boule est réellement leur miniball, avec support essentiel q2.

À K=2, on obtient ainsi m² feuilles pour N=2m points. Pour tout K≥2 fixé, ajouter K−2 points distincts communs à l'intérieur de toutes ces boules. L'intersection intérieure est non vide, puisque c=(1/2,0,0) vérifie :

$$\pi(c)=-\frac{(1-t^2)(1-s^2)}{4}<0.$$

Les fixtures choisissent explicitement les ancres c_l=(1/2,l/(100K),0), 1≤l≤K−2. Avec η=l/(100K)<1/100 et t,s≤1/4, leur puissance est :

$$\pi(c_l)=-\frac{(1-t^2)(1-s^2)}{4}+\eta(\eta-t)\leq-\frac{225}{1024}+\frac{1}{10000}<0.$$

Chaque label F_ij={A_i,B_j}∪{c_1,…,c_{K−2}} possède donc exactement sa paire comme support, toutes les ancres dans l'intérieur et tous les autres sites strictement dehors. Il est strictement Gabriel de cardinal K, donc une feuille FULL. Les m² labels sont distincts et N=2m+K−2 :

$$\#\text{feuilles FULL}\geq m^2=\frac{(N-K+2)^2}{4}.$$

Le recouvrement des ancres entre toutes les feuilles ne permet aucune identification de composantes : aucune coface n'est encore disponible au niveau de naissance de la feuille considérée.

## Régularité et précision

Pour chaque m fini, toutes les inégalités intérieur/extérieur utilisées sont strictes et en nombre fini. Elles survivent donc dans un voisinage ouvert de la configuration. Une perturbation générique suffisamment petite évite simultanément les dégénérescences algébriques de supports et de coquilles. Les points rationnels sont denses : elle peut être choisie rationnelle. Les paires restent les supports positifs des mêmes labels et les m² feuilles persistent. Cela établit l'existence dans le domaine régulier ; aucune perturbation numérique produite n'est prétendue certifiée ici.

Les coordonnées explicites non perturbées ont des sous-ensembles coplanaires et des ancres alignées. Le script certifie les boules nommées, **pas la régularité globale de ces nuages**, ni l'acceptation d'un producteur FULL sur tous leurs autres appels géométriques.

Une réalisation entière de la famille sans ancres s'obtient en multipliant par 32m⁴ : A_i=(32m⁴−i²,8m²i,0), B_j=(j²,0,8m²j). La plage nécessaire croît avec m. Le profil u16 possède un univers fini ; on ne lui attribue pas une limite littérale N→∞. Une promesse universelle sous-quadratique dans le modèle géométrique à précision variable est exclue ; un contrat borné u16 demande toujours ses propres caps et mesures.

## Raccord fini à la fixture v7 des arcs liés

Le [gate v7](../../tests/linked_arcs_gate.cpp) possède déjà des littéraux u16 et des contrôles de supports positifs q3/q4, de boules strictement vides et de clés distinctes. Leur rôle de minima relève respectivement de FULL K3/K4. Ce raccord utilise la positivité et la vacuité stricte, et non la seule appartenance à Delaunay. L'antériorité de cette construction est documentée dans la [note locale historique](../../../morsehgp3D_v5/audits/STRATEGIE_SOUS_QUADRATIQUE_Q3_Q4_20260830.md), qui cite Edelsbrunner–Pach ; aucun résultat d'exécution v5/v6 n'est hérité.

La nouvelle lecture entière des coordonnées du gate vérifie aussi toutes les puissances étrangères des paires croisées, indépendamment de ses verdicts q3/q4 :

| Paramètre du gate | N | Paires croisées, donc feuilles K2 | Puissances étrangères vérifiées | Plus petite puissance |
| --- | ---: | ---: | ---: | ---: |
| 2 | 6 | 9 | 36 | 2 877 505 |
| 4 | 10 | 25 | 200 | 718 129 |
| 8 | 18 | 81 | 1 296 | 178 009 |
| 16 | 34 | 289 | 9 248 | 29 464 |

Toutes sont strictement positives. Ces comptes donnent des feuilles mathématiques nommées, sans qualification de la régularité globale du nuage ni nouveau passage du moteur FULL.

## Vérification et conséquence pour la campagne

Le [script](full_output_growth.py) utilise uniquement `Fraction`, les entiers Python et les littéraux du gate v7 épinglé. Il vérifie m=2/4/8/16, K=2/3/4/10 : 1 360 labels, 34 720 puissances étrangères et 3 740 puissances d'ancres, avec le support, les puissances sous deux formes et la minoration uniforme. L'unicité de la miniball prouve les inégalités strictes de toutes les cofaces sans les énumérer. Les [reçus normal](full_output_growth_normal.json) et [optimisé](full_output_growth_optimized.json) passent code 0 et sont identiques, SHA-256 `d37a08eccb11e34381d812d2916444f10d2e23cd873da516f287e06b84f66488`. Les deux commandes sont bornées à 60 secondes, épinglées sur CPU1 ; aucun C++, benchmark ou Gamma exhaustif n'est lancé.

Cette borne concerne l'énumération explicite des feuilles. Elle n'interdit pas toute représentation implicite, dont le contrat et le coût d'expansion demanderaient une autre analyse. Aucune performance, verticale, masse ou qualification industrielle n'est acquise par cette preuve. GCP non utilisé.
