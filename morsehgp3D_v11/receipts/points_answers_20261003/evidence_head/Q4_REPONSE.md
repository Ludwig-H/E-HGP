# Réponse Q4 — récupération asymptotique : ce qui est prouvé, ce qui manque

3 octobre 2026. Lecture primaire du manuscrit local et des rapports du workflow ; aucun fit, calcul natif, build ou GCP. Cette note ne transfère aucune qualification des anciennes campagnes à la règle en rayon.

**Réponse.** À K et contraste de densité fixés, ni l'égalité du rappel de P₁∘Πₖ₊₁ avec FULL, ni l'infériorité asymptotique de la fermeture ne sont établies. On dispose d'une borne déterministe de perte pour l'ancrage, et d'un encadrement de percolation pour la fermeture. Ces résultats donnent des hypothèses suffisantes explicites, pas les conclusions demandées sans hypothèses.

## 1. Objet exact du chapitre 7

Source primaire : MANUSCRIT_THESE_HAUSEUX.pdf, SHA 579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef.

- Définition 23, imprimée 65 / PDF 91 : Θ est une probabilité de Palm sur X_λ∪{0}.
- Imprimée 66 / PDF 92 : Θ^poly(λ) = P[0∈δ_(1/2)(C_∞)], où C_∞ est la composante non bornée de L_K. Le site peut être couvert sans appartenir à L_K ; remplacer cet événement par 0∈C_∞ perd précisément une partie de la frontière.
- Modèle et proposition 3, imprimée 71 / PDF 97 : deux domaines A,B compacts convexes dans un fond comportant un corridor macroscopique ; densité ρ₁ dans A,B, ρ₀ dans le fond, p≥2.
- Théorème 3, imprimées 71–72 / PDF 97–98 : sous les hypothèses de percolation et de continuité de Θ, la fraction juste avant fusion parasite est R_cc = Θ^cc(ρ λ_c^cc), avec ρ=ρ₁/ρ₀. Ce n'est ni le meilleur IoU d'une instance ni une partition EOM.

Le régime n→∞ conserve K et ρ. Aux rayons pertinents r_n, n(2r_n)^pρ₀ tend vers un seuil constant. Ainsi r_n est de l'ordre de n^(-1/p), tout comme d_K et la première date qualifiée typiques dans l'amas. Le fait que le retard absolu tende vers zéro ne démontre donc pas que retard/r_n tende vers zéro.

## 2. Borne finie utile pour P₁ qualifié

Fixons un nuage fini, un ordre k, m=k+1, une coupe fermée r et une composante C de L_k(r). Notons E_C(r)={x∈X : d(x,C)≤r}, H_C(r) le bloc des sites entrés dont le propriétaire remonte dans C, et t′_m(x) la première couverture qualifiée. Les rayons sont ici non carrés.

Le lemme L6 de l'ancrage v10 (mémo §2.2) s'applique encore aux composantes qualifiées : si une composante couvre x à s, elle rejoint la composante contenant x avant s+d_k(x)/2. Le profil qualifié est un sous-profil croissant par remontée ; son temps de résolution ne dépasse pas celui du profil brut. Par conséquent,

    0 ≤ e_H(x) − t′_m(x) ≤ d_k(x)/2.

À m=k+1, la boule centrée en x au rayon d_(k+1)(x) contient au moins k+1 sites, donc t′_(k+1)(x)≤d_(k+1)(x), site compté parmi les voisins. Ce sont des bornes de rayon, pas de rang ni de niveau carré.

**Condition suffisante de récupération dans la bonne composante :**

    x∈C∩X et t′_(k+1)(x)+d_k(x)/2≤r  ⇒  x∈H_C(r).

Preuve : l'entrée est au plus le membre de gauche. La première lignée qualifiée et la composante contenant x ont déjà fusionné à cette date, par L6. À r le site suit donc C. Une condition plus simple mais plus forte est x∈C∩X et d_(k+1)(x)≤2r/3.

**Attention frontière :** la seule condition x∈E_C(r) ne permet pas de remplacer x∈C∩X. Un site peut être couvert par C et être encore dans une autre composante de L_k. La borne suffisante ci-dessus ne certifie donc pas, à elle seule, tout le rappel Θ^poly.

Pour les points de A, définissons la masse critique majorante

    B_A(C,r) = |(E_C(r)\C)∩X∩A|
               + |{x∈C∩X∩A : t′_(k+1)(x)+d_k(x)/2>r}|.

Alors, par fidélité et la condition suffisante,

    0 ≤ |E_C(r)∩A| − |H_C(r)∩A| ≤ B_A(C,r).

Les deux ensembles comptés sont disjoints. Chacun peut néanmoins compter des sites effectivement récupérés : B_A est une MAJORATION, pas une mesure exacte du défaut. Une version exacte compte les sites de E_C(r) non entrés, ou entrés dans une lignée différente de C.

Cette formule se teste sans changer le moteur : pour chaque coupe avant fusion, publier couvertures, entrées et propriétaire remonté, ainsi que le dénominateur |X∩A|. Le nombre d'instances sauvées et l'IoU maximal ne remplacent pas cette masse.

## 3. Théorème conditionnel de transfert à l'asymptotique

Soient r_n<F_n des coupes avant la fusion parasite FULL, C_n la composante géante de A et N_A=|X_n∩A|. Supposons le cadre et la convergence FULL du théorème 3, et

    B_A(C_n,r_n)/N_A → 0 en probabilité.

Alors le sandwich du §2 donne

    |H_Cn(r_n)∩A|/N_A − |E_Cn(r_n)∩A|/N_A → 0,

donc P₁∘Π_(k+1) récupère la même fraction limite que FULL. Il faut la même hypothèse dans B. Employer r_n<F_n évite de compter les cohortes du plateau de fusion comme « avant » la fusion.

**Ce qui manque aujourd'hui :** une preuve que cette masse critique est o(N_A). Dans le régime de Palm à λ_dense=ρ λ_c^poly fixé, les variables t′/r et d_k/r restent à échelle constante. Aucune affirmation du chapitre 7 ne fait disparaître la masse frontière ni la couche de dates proches de r. Le contre-exemple fini R1 prouve l'insuffisance de l'ancien argument ; il ne réfute PAS, à lui seul, une égalité de probabilités de Palm.

Pour la prouver ou la réfuter, il faut analyser les événements correspondants du processus infini (ou embarquer un témoin local dans un événement de Palm de probabilité positive avec connexion à la composante infinie). Dupliquer ou agrandir une fixture finie hors du modèle i.i.d. à densité constante ne suffit pas.

Une borne de couche de cœur explicite aide au contraste élevé. Avec r=1/2 et d_(k+1) site inclus,

    P[d_(k+1)>2r/3] = exp(−μ) Σ_(j=0)^(k−1) μ^j/j!,
    μ = λ_dense ω_p / 3^p.

Elle contrôle le second terme du §2, sans son événement de composante. À λ_dense fixé, cette borne est constante en n ; lorsque λ_dense→∞ elle tend vers zéro. Elle ne contrôle pas, seule, le premier terme frontière. Une démonstration de récupération 1 à grand contraste demanderait aussi une borne de cette frontière ou une connectivité locale supercritique ; ce serait un régime ρ→∞, distinct de n→∞ à ρ fixé.

Autres hypothèses avant d'appliquer le théorème 3 à « cc=H » : définir mesurablement la pendaison sur le processus localement fini infini, justifier existence/uniqueness d'un bloc géant et la loi de Palm/loi des grands nombres, puis la continuité requise. Pour P₁, le supremum porte sur les événements futurs ; une simple limite de fenêtres finies doit être justifiée. L'équivalence du seuil de percolation H avec celui de FULL n'est pas prouvée par la seule fidélité : elle donne seulement λ_c^H≥λ_c^poly si ces seuils sont définis. Une piste conditionnelle est la finite-energy : sur un fond FULL supercritique, ajouter localement assez de points et un chemin vers la composante infinie produit une densité positive de sites satisfaisant la condition du §2. Il faut formaliser ce lemme pour obtenir l'autre inégalité ; cela ne donnerait toujours pas l'égalité des fractions à contraste fixé.

## 4. Fermeture qualifiée : encadrement exact, aucun ordre de rappel aux seuils

Pour k≥2, m=k+1 fixé, supposons les modèles infinis/uniqueness et les probabilités de Palm définis. Une composante infinie FULL est automatiquement qualifiée. Les inclusions vérifiées T3 du workflow donnent, avec la normalisation r=1/2,

    Θ^poly(λ) ≤ Θ^CL(λ) ≤ Θ^poly(2^p λ),
    λ_c^CL ≤ λ_c^poly ≤ 2^p λ_c^CL.

En effet, une couverture FULL qualifiée tient dans un bloc CL au même rayon ; inversement un bloc CL au rayon r tient dans la couverture d'une composante FULL au rayon 2r. La qualification finie n'altère pas l'admissibilité d'une composante infinie. Cette seconde inclusion est à étendre à la configuration localement finie, pas seulement à un échantillon.

Le théorème 3 compare toutefois des ARGUMENTS DIFFÉRENTS :

    R_CL = Θ^CL(ρ λ_c^CL),   R_poly = Θ^poly(ρ λ_c^poly).

Posons a=ρ λ_c^CL. Les deux fractions sont dans

    [Θ^poly(a), Θ^poly(2^p a)].

Donc, sous les hypothèses précédentes,

    |R_CL − R_poly| ≤ Θ^poly(2^p a) − Θ^poly(a).

Aucune inégalité R_CL≤R_poly ou R_CL≥R_poly ne s'en déduit. Si les seuils étaient égaux, l'inclusion donnerait au contraire R_CL≥R_poly. À grand contraste, si Θ^poly(λ)→1, cet encadrement donne convergence des deux fractions vers1 ; il ne compare pas les vitesses des fractions non récupérées.

Le seuil CL strictement plus petit et la « perte exponentielle » évoqués dans fermeture §4.5 sont respectivement une conjecture et une heuristique. Ils exigent une preuve de renforcement essentiel pour ce modèle continu, puis un contrôle des événements dominants/queues de percolation. La fixture finie 1 contre1/2 est hors du modèle convexe homogène et ne tranche aucun de ces énoncés asymptotiques.

## 5. Conseil concret au développeur

Publier maintenant la borne de retard et le théorème CONDITIONNEL à masse critique négligeable. Retirer « qualification sans coût asymptotique » tant que la masse n'est pas contrôlée. Garder trois sorties séparées sur les mêmes scènes/coupes : FULL couvertures, H blocs intérieurs, CL blocs extérieurs, avec fractions et dates de fusion propres. Un contrôle de croissance sur des échantillons i.i.d. de tailles croissantes peut tester la conjecture ; il ne remplace pas la preuve. Les échelles de grille doivent suivre h_n/r_n→0, avec identifiants/retours et collisions traités explicitement : une grille 1mm fixe dans un domaine borné ne représente pas littéralement n→∞ de points distincts du modèle continu.

Ne pas confondre ces expériences de modèle avec les instances SemanticKITTI sélectionnées, les voisins d'une même séquence, les scores IoU des hiérarchies ou le contrat de temps FULL/G4.

Sources de travail : ancrage_marges/MEMO §2.2 L6 et §4 T3 ; axiomes Proposition I ; sa vérification R1 (corrige qualification) ; fermeture T3 et §4.5 ; vérification fermeture (confirme les inclusions et laisse C1–C2 non vérifiées). Les hashes et les deux extraits primaires du PDF sont joints.
