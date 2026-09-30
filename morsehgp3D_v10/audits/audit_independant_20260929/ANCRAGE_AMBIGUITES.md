# Frontière, laminarité et précision — décisions mathématiques

30 septembre 2026. Produit 4b7d70422 ; preuves jusqu'à d192637a7. Nouveau contrôle de bande et croisement inter-K, aucun moteur changé/GCP. public_status=not_claimed. [Version longue conservée exactement](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md).

## Cible de la thèse

Parties I/II relues : pages imprimées 1–50 et 53–107, [trace](../../receipts/audit_independant_20260929/lecture_these/receipt.json). Définition 8 p.21, théorème 2 pp.60–61 : C_discret(r)=X∩δ_r(C). Une observation frontière peut couvrir plusieurs composantes sans être core. **Couverture commune ne signifie pas fusion spatiale.** Core reste exact/stable ; il peut différer les observations jusqu'à la connexion parasite.

§9.1 pp.96–97 : contributions aux faces normalisées par observation, masses pour la condensation avant sélection/vote. Remplissage final ne restaure pas une branche perdue pour masse insuffisante. Proposition 7 : partition pour sélection fixée, pas emboîtement de votes à chaque coupe. Poids de boules : preuve d'agrégation ou nouvelle politique déclarée, sans probabilité calibrée.

Core à K fixé est un arbre ; couvertures recouvrantes et coupes multi-K peuvent se croiser. FULL/verticales ne déterminent pas seuls une projection laminaire exclusive. [Obstructions](../../receipts/audit_independant_20260929/historique/base_6206d1d11/TOUR_ET_POINTS.md).

## Bande à K fixé : cible courante et limite inter-K

Le bras 4b7 considère tous les témoins forts jusqu'à (1+η)α_K(x), puis fixe leur LCA aux dates propres. Couverture à l'entrée, laminarité à K fixé et raffinement avec η sont corrects. Cinq sites (0,1,2,6,9) sur une droite, η=1/8, r=3 donnent **K2 : {0,1,2}|{6,9} ; K3 : {0,1,2,6}|{9}**. Les blocs se croisent. La composante K3 descend à gauche K2 ; x=6 est ancré à droite K2, sans ex æquo de première couverture. [Γ et export natif, normal/−O](../../receipts/audit_independant_20260930/cover_band_followup/README.md). Les verticales FULL sont correctes ; les propriétaires ne commutent pas avec elles.

**La cible utilisateur est désormais un seul K fixé.** Le croisement ne la bloque pas. Une combinaison future de plusieurs K exigerait une politique de transport ou de combinaison déclarée. Le raffinement commun est laminaire mais laisse ici 6 et 9 singletons ; le regroupement commun fusionne tout dès β=9, avant la fusion géométrique K2 à 49/4. Inclure K1 dans ce regroupement redonne K1 : tout bloc couvert raffine une composante L1. [Preuve et 800 contrôles](../../receipts/audit_independant_20260930/cover_band_followup/intrinsic_cross_k/README.md). Comparer rappel, masse différée et condensation pour le bras à K fixé.

**Variante éprouvée à K fixé :** supprimer les ancêtres redondants parmi les nœuds sélectionnés, puis LCA. Les témoins de première couverture restent minimaux ; les nouveaux témoins plus tardifs ne peuvent en être descendants. Couverture, laminarité et monotonie avec η sont conservées, entrée jamais plus tardive. [Preuve et prototype exact](../../receipts/audit_independant_20260930/fixed_k_antichain/README.md) : 2 892 bandes abstraites, six exports/507 coupes, incidences internes K3/K5 gardées, normal/−O identiques. Aucun bras développeur modifié.

Triangle K2 (0,0),(8,0),(0,3), η=1/8 : x=(8,0) entre à β=16 au lieu de 73/4, l’ancêtre tardif n’ajoutant pas de lignée concurrente. La masse devient disponible plus tôt ; ici la partition de points peut rester identique, aucun gain de coassociation/qualité acquis. Le prototype garde les antichaînes pour audit ; tri par point et mémoire supplémentaires à mesurer. Bande complète à fermer jusqu’à (1+η)²α² avant décision, malgré la date d’entrée avancée.

## Toutes les incidences, y compris internes

Pour dist(x,C)≤r, témoin critique contenant x, rayon≤r, centre dans C, avec population≥K et p+q_min≤K. [Preuve et contrôles](../audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md). Résoudre le centre à K puis son ancêtre vivant ; population≥K n'assure pas une cellule active à K. Garder I/U complets.

Transporter (x,K,niveau(b),composante_K(centre(b))), résoudre chaque (b,K) une fois, puis dédupliquer par point/coupe. K1 : ajouter les n sites au niveau zéro (p=0, q_min=m=1 ; formule 1D p=K−2 limitée à K≥2). Volume≤n+Kmax·Σ_b|I_b∪U_b| pour ce raccord non pondéré, b désignant les boules positives. Borne relative au catalogue. **Conserver les fusions FULL p+q_min=K+1.**

[Sections 10–11](../audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md#L694) : contact coquille/intérieur modifiant les atomes ; entrée frontière strictement interne K3/K5. Chaque point a une feuille couvrante à K2, pas toutes ses couvertures futures. « Une feuille=une unité » n'est pas une réduction générale.

Conserver la mesure des continuations : leurs durées segmentées se télescopent si sommées correctement ; exclure les ancêtres perd une masse finie, les compter comme nouvelles observations peut doubler la masse. Diagnostic de durée à explorer, sans tête qualifiée.

## Projection dure : contrôle et limite de majorité

À la première couverture α(x), résoudre/dédupliquer **toutes** les composantes à cette date, plateau activé. Unique : attache immédiate, puis ascendance figée. Sinon conflit déclaré, majorité fixe ou LCA, puis propriétaire figé. Exporter l'ancêtre vivant à l'entrée ; singletons auparavant, jamais bloc collectif bruit.

Majorité M_x(C)>θW_x,1/2≤θ<1 : laminaire pour témoins finis, poids positifs fixes, activation puis ascendance. W comprend les futurs. Aucune entrée précoce/robustesse obtenue quand univers/poids sont recalculés.

Cinq sites K2 : (1,1,0),(2,1,0),(0,2,0),(0,0,0),(0,1,1). À β=1/4, {0,1} seule couverture de x0, mais masse normalisée inverse-β de 4/11. À β=2/3, autre branche de poids 6/11 reçoit x0 ; paire{0,1} attend β=5/4. Uniforme échoue aussi ; ancrage unique/K2-LCA η=0 donnent la paire dès 1/4. [Fraction normal/−O et export natif borné](../../receipts/audit_independant_20260930/tower_math/receipt.json). Claude a préenregistré le cas/bras hybride ; aucun gain statistique acquis.

Diagnostic fractionnaire à W fixe : m_C(r)=Σ_xΣ_{i actif dans C}w_i/W_x ; R_x(r)=1−Σ_{i actif}w_i/W_x. Somme masses actives+réserves=n ; W=0 garde une unité en réserve, sans affectation anticipée. Cinq sites à β=1/4 : 15/11+40/11=5. Renormaliser seulement les actifs changerait la règle.

## Bras K2-LCA et marges

α_min(x)=min_{y≠x}||x−y||/2 ; S_η(x) contient toutes les paires incidentes de demi-distance≤(1+η)α_min(x), égalités incluses, même non critiques. Résoudre les milieux à leur propre rayon, LCA J, e(x)=max(naissance(J),max rayon des paires). Attacher à l'ancêtre de J vivant à e(x), puis ascendance. Laminarité ; η accru peut différer le rappel.

Déplacement apparié≤ε : g_xy=||x−y||/2−(1+η)α_min(x) varie d'au plus (2+η)ε. Marge stricte correspondante conserve les candidats ; dates conditionnellement contrôlées à 2ε en rayon. À η=0, g du gagnant vaut0 : gagnant unique préservé si écart premier/deuxième>2ε ; ex æquo distincts. Aucun certificat n'autorise la troncature par ID.

Majorité recalculée : marge de vote et transport nécessaires. q_i=w_i/W_x,γ=M_x(C)/W_x−θ ; γ>Σ_i|q_i'−q_i| conserve conditionnellement le vote transporté, pas l'univers/admission. Une coquille devenue intérieure peut changer l'univers malgré fusion FULL inchangée.

## Ce que garantit le pas h

Arrondi au plus proche : déplacement≤ε=√3·h/2 par rapport aux coordonnées d'entrée. **Mêmes observations/IDs, copies ou poids conservés.** Rayon MEB de chaque partie change d'au plusε ; Γ_K possède des inclusions réciproques au décalageε, FULL est interlacé en rayon. Supports, coquilles, plateaux et projection dure ne restent pas forcément identiques.

Core : |u_X(i,j)−u_Y(i,j)|≤2ε, soit √3·h pour cette quantification. Pour Γ/FULL, le décalage en rayon carré est (√β+ε)² ; pour core, (√β+2ε)². Aucune constante globale h². Déduplication sans copies/poids change l'univers. Pas de garantie EOM ou d'étiquettes physiques.

Même arbre changé d'unité : β_phys=h²β_grille, λ_phys=h^(−z)λ_grille ; stabilités multipliées par h^(−z). Les poids 1/β prennent h^(−2), les poids 1/r prennent h^(−1), les uniformes restent identiques : chaque facteur global s'annule à la normalisation. EOM idéal invariant avec mêmes masses, z, conventions de zéro et scores définis. Requantifier peut changer les décisions. [Contrat h/profil](../AUDIT_MASSIF_LIDAR_20260930.md), [preuve et contrôles bornés](../../receipts/audit_independant_20260930/precision_grille/semantique/README.md).

## Condensation : correction préalable

Le [défaut désormais reproduit](../../receipts/audit_independant_20260930/point_condensation_followup/README.md) porte sur les départs directement attachés : quand la masse restante passe sous min_cluster_size, la branche doit finir à cette cohorte, même sans division géométrique. Dans le vrai cas interne K3/mcs6, sortie 6→5 à β25 : stabilité z1 correcte 6/5 au lieu de 88/65 ; étiquettes inchangées ici. Les arbres API montrent séparément des inversions EOM, racine exclue. Relectures d’archives normal/−O passent ; aucun moteur/sklearn relancé. Corriger les dates exactes simultanées avant d’interpréter l’effet de l’antichaîne sur EOM.

## Prochaine comparaison utile

Avant condensation : couvertures, masses/réserves, ambiguïtés, entrées internes et rappel avant fusion parasite. Après : EOM équitable/scores. Panel uniforme global et panel stratifié diagnostique séparés ; paires/IDs fixés sur le nuage de base, réutilisés sous perturbation. Publier entrées, hauteurs en rayon, marges et fraction certifiable ; maximum observé≠borne exhaustive.

Porte de conception : cinq sites, contact coquille/intérieur, entrées K3/K5 et continuations. Ne pas choisir un bras pour sa seule laminarité ou sa facilité à jeter les incidences. [Sources statistiques](../../receipts/audit_independant_20260929/historique/base_6206d1d11/tower_statistical_sources.md) : petits K fixes et LiDAR corrélés/surfaciques n'héritent pas des théorèmes i.i.d. à K croissant.
