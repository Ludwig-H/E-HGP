# Arbres des amas K‑NN : ce que dit la littérature statistique, et ce qu'on peut en tirer pour la tour v10

```text
phase=exploration_v10_hors_registre   backend=cpu_reference   profile=quantized_u18_input_only
mode=lecture_critique_litterature      public_status=not_claimed
GCP non utilisé. Aucun fichier du dépôt modifié ; aucune graine test ni test_v10b générée.
```

## 0. Méthode

**Réponse courte.**
- La littérature justifie que la tour calcule le bon objet : l'arbre exact de l'estimateur K‑NN, réduit à ses points d'échantillon. Cet arbre est consistant quand K/log n → ∞ et K/n → 0.
- Elle ne justifie ni nos K ≤ 10, ni un choix de z, ni l'EOM comme sélection.
- Sur les coquilles (dev), le résultat de l'EOM dépend de z de façon monotone. À K = 10, seul z ∈ [3 ; 4] retrouve les 8 coquilles.

**Étiquettes utilisées dans tout le rapport.**
- [V] : énoncé lu dans le texte intégral. Les PDF et leurs extraits texte sont dans `/workspaces/E-HGP/build/v10-persist/audit_hier/litterature_statistique/pdf/`.
- [A] : résumé ou notice seulement.
- [S] : rapporté par une source secondaire lue, que je nomme.
- [D] : dérivation de ma part, non publiée.
- [E] : mesure de cet audit, sur dev.
- [R] : résultat interne au dépôt, non revu.

**Sources introuvables en texte intégral.**
- Campello et al. 2013 et 2015 : leurs définitions sont vérifiées via la thèse de Moulavi (2014), Malzer–Baum (2020), McInnes–Healy (2017), la documentation hdbscan et la thèse (§ 4.4.4).
- Müller–Sawitzki 1991, Polonik 1995, Loftsgaarden–Quesenberry 1965, Devroye–Wagner 1977, Hartigan–Mohanty 1992, ToMATo 2013 et Sheehy 2012 : résumé seulement.

**Notations.** n points ; d la dimension ambiante ; m la dimension intrinsèque ; d_K(y) la distance au K‑ième point le plus proche, multiplicités comprises, boules fermées ; α_K(x) le rayon d'entrée par première couverture ; λ = r^(−z).

---

## 1. L_K(r), multicouverture, distance à la mesure

**1.1 L'identité [D, triviale].**
- L_K(r) = {y : |B̄(y,r) ∩ X| ≥ K} = {d_K ≤ r} = {f_K ≥ K/(n v_d r^d)}, avec f_K = K/(n v_d d_K^d).
- L'identité vaut pour tout exposant utilisé dans f_K, puisque f_K est une fonction monotone de d_K.
- Conséquence :
  - les ensembles, leurs composantes et leurs inclusions (l'arbre) ne dépendent ni de d ni de l'échelle de λ ;
  - seules les hauteurs en dépendent, et avec elles tout ce qui est calculé sur les hauteurs : EOM, seuils de persistance, métriques entre arbres.

**1.2 L'estimateur K‑NN et ses conventions.**
- Loftsgaarden & Quesenberry (1965), Ann. Math. Statist. 36(3):1049–1051 [A]. Selon Dasgupta–Kpotufe, ils le montrent consistant quand f est continue.
- Dasgupta & Kpotufe (2014, NIPS 27), déf. 1 : f_k = k/(n v_d r_k^d) [V].
- Les conventions varient :
  - Chaudhuri et al. (2014) comptent le point lui‑même, comme la tour et HDBSCAN (Moulavi 2014, déf. 4.3.3) [V] ;
  - Kpotufe–von Luxburg (2011) l'excluent [V].
- La constante (k, k−1 ou k−2 pour un estimateur sans biais sous approximation poissonnienne [D]) n'affecte pas les ensembles de niveau.
- Chaudhuri et al. (2014, § 3) nomment λ = k/(n v_d r^d) la « densité empirique correspondante ». Ils préfèrent pourtant r [V] : « r is directly observed rather than inferred… [on] a low-dimensional submanifold… the inferred λ is misleading ».

**1.3 La multicouverture.**
- Sheehy (2012), CCCG 309–314 [A] : la subdivision barycentrique du complexe de Čech, filtrée par le cardinal des sommets, capture exactement la topologie des régions k‑couvertes, pour tout k.
- Edelsbrunner & Osang (SoCG 2018 ; DCG 65:1296–1313, 2021) [A]. Ils construisent un pavage rhomboïde dans R^(d+1), dont les tranches entières sont les mosaïques de Delaunay d'ordre k. Ils calculent la persistance à k fixe (filtration en rayon) et à r fixe (en « zigzag »).
- Corbet, Kerber, Lesnick & Osang (arXiv 2103.07823v3, copie locale) [V] : Cov_{r,k} = {b : ‖b−a‖ ≤ r pour au moins k sites}. C'est exactement L_k(r) en rayon non carré.
- Reani & Bobrowski (arXiv 2403.12792) [V] : les sous‑niveaux de d^(k) sont les couvertures k‑fold ; ils en donnent la théorie de Morse.

**1.4 La distance à la mesure (DTM).**
- Chazal, Cohen‑Steiner & Mérigot (2011), FoCM 11(6):733–751 [V]. Ils définissent la pseudo‑distance δ_{μ,m}(x) = inf{r : μ(B(x,r)) > m}.
  - Pour la mesure empirique, δ vaut d_k avec k le plus petit entier > m·n. La multicouverture est donc faite des sous‑niveaux de δ.
  - δ est 1‑lipschitzienne, mais μ ↦ δ_{μ,m} n'est continue en aucun sens raisonnable (exemple à deux Dirac).
  - La DTM vaut d²_{μ,m0} = (1/m0) ∫_0^{m0} δ²_{μ,m} dm. En empirique, c'est la moyenne des carrés des distances aux k0 plus proches voisins.
  - Théorème 3.5 : ‖d_{μ,m0} − d_{μ',m0}‖∞ ≤ m0^(−1/2) W₂(μ, μ').
  - § 5 : avantages sur l'estimateur K‑NN : définie sans densité (mesures portées par une variété), stable en Wasserstein, semi‑concave.
- Guibas, Mérigot & Morozov (2013), DCG 49:22–45 [V], prop. 1. La DTM au carré est une distance de puissance aux barycentres des k‑parties. Il faut autant de barycentres que de sites de Voronoï d'ordre k.
- Biau et al. (2011), EJS 5:204–237 [A] : estimateur K‑NN pondéré, issu de la DTM.
- Lien élémentaire [D] :
  - on a d_{⌈k/2⌉}²/2 ≤ DTM²_{k/n} ≤ d_k² ;
  - d'où Cov_{r,k} ⊆ {DTM ≤ r} ⊆ Cov_{√2·r, ⌈k/2⌉}.

**1.5 Stabilité.**
- Blumberg & Lesnick (2024), FoCM 24(2):385–427 [V], théorème 1.6(i) : d_I(M(X), M(Y)) ≤ d_Pr(ν_X, ν_Y).
  - La bifiltration de multicouverture, normalisée en masse k/n, est donc stable en distance de Prohorov, c'est‑à‑dire robuste aux points aberrants.
  - Le théorème 3.1 étend le résultat à des mesures quelconques, donc aux multiplicités.
  - L'entrelacement décale à la fois le rayon et la masse. Une tranche à K fixe n'hérite donc d'aucune stabilité [D].
- Blaser, Brun, Gardaa & Salbu (arXiv 2405.01214v4) [V] définissent la « core bifiltration », inspirée de HDBSCAN. Leur théorème 3.7 donne :
  - Core^β_{r,k} ⊆ Cov_{(1+1/β)r,k} ;
  - Cov_{r,k} ⊆ Core^β_{max(1,2β)r,k} ;
  - un produit des facteurs minimal, égal à 3, pour β = 1/2.
- Or β = 1/2 correspond à la liaison simple robuste avec α = 1, c'est‑à‑dire HDBSCAN [D]. C'est la version publiée du lemme C5 du dépôt (`docs/conception/CLUSTER_v2.md`, lignes 176–190) [R].
- Un facteur 3 en rayon, soit ln 3 ≈ 1,10 en log r, dépasse la durée de vie de beaucoup d'amas. HDBSCAN et la tour peuvent donc diverger sur toute structure moins persistante. L'entrelacement ne dit pas lequel des deux est meilleur.

---

## 2. Consistance de l'arbre des amas

**2.1 Hartigan.**
- Hartigan (1975, *Clustering Algorithms*) définit les amas de forte densité [S, via Eldridge et al. 2015].
- Hartigan (1981), JASA 76(374):388–394 [V] :
  - la liaison moyenne et la liaison complète sont « hopelessly inconsistent » ;
  - la consistance de la liaison simple dépend de la percolation ;
  - la liaison simple est consistante en dimension 1 mais pas en dimension ≥ 2 [S, Chaudhuri et al. 2014] ;
  - il introduit la consistance fractionnaire, et prouve un résultat plus faible quand la vallée est assez basse.
- Penrose (1995), JMVA 53:94–109 : consistance fractionnaire dès que le ratio inf f / sup‑min sur les chemins dépasse 1 [S].
- Définition de Chaudhuri et al. (2014, déf. 2.3) [V]. Pour A et A′, composantes distinctes de {f ≥ λ}, les plus petits amas qui contiennent A∩X_n et A′∩X_n doivent être disjoints avec probabilité tendant vers 1.
- Hartigan ne parle pas des points de densité inférieure à λ.

**2.2 L'estimateur plug‑in exact (c'est la tour).**
- Chaudhuri et al. (2014), annexe A, lemme A.1 [V] :
  - hypothèses : sup|f_n − f| ≤ ε_n et Ξ − ξ > 2ε_n ;
  - conclusion : A et A′ sont dans deux composantes disjointes de C_{f_n}(Ξ − ε_n) ;
  - ils ajoutent : « computing the level sets of f_n is usually not an easy task ». Wong & Lane (1983) ne l'approchent que pour le K‑NN, sans preuve [S].
- Rolle–Scoccola (2024) qualifient aussi le plug‑in de « not computationally‑tractable » [V].
- Eldridge, Belkin & Wang (COLT 2015) [V], théorème 5 : d(C_f, C_f̃) ≤ ‖f − f̃‖∞ en distorsion de fusion, hauteurs en unités de densité.
- Kim et al. (NIPS 2016), lemme 1 [V] : d∞ = d_M pour des densités continues.
- Devroye & Wagner (1977), Ann. Statist. 5(3):536–540 [A] : sup|f_n − f| → 0 p.s. si f est uniformément continue sur R^d, k/n → 0 et k/log n → ∞.
- **Corollaire [D]** (non énoncé tel quel dans ces articles) :
  - dans ce régime, l'arbre C∩X de la tour à l'ordre K(n) est consistant au sens de Hartigan ;
  - sa distorsion de fusion est majorée par sup|f_K − f| ;
  - aucun paramètre α ni aucun argument de percolation n'est nécessaire.
- **Réserves.**
  - La continuité uniforme exclut les densités à bord franc et les mesures singulières (surfaces, filaments), c'est‑à‑dire nos familles.
  - Pour ces cas, la littérature propose trois cadres :
    - la densité lissée à largeur fixe (Rinaldo & Wasserman 2010) ;
    - la multicouverture à masse K/n fixe (Blumberg–Lesnick) ;
    - les versions sur variété, avec dimension intrinsèque (Balakrishnan et al. 2013 ; Jiang 2017).
  - La littérature statistique lue déclare ce calcul difficile. La géométrie algorithmique (Edelsbrunner–Osang, Corbet et al.) le fait, comme composantes connexes des tranches horizontales, sans que les deux littératures se croisent dans ce que j'ai lu. Toute revendication de nouveauté est à vérifier.

**2.3 Approximations par graphe.**

*Chaudhuri & Dasgupta (NIPS 23, 2010) ; Chaudhuri, Dasgupta, Kpotufe & von Luxburg (IEEE TIT 60(12):7900–7912, 2014) [V].*
- Algorithme 1 : sommets r_k(x_i) ≤ r, arêtes de longueur ≤ αr. La liaison simple correspond à (α = 1, k = 2).
- Théorème 3.3. Hypothèses :
  - √2 ≤ α ≤ 2 ;
  - k ≥ C·(d log n/ε²)·log²(1/δ) ;
  - A et A′ (σ,ε)‑séparés, avec λ = inf f sur A_σ ∪ A′_σ ;
  - n ≥ k/(v_d (σ/2)^d λ)·(1+ε/2).

  Conclusion : séparation et connexité au rayon r(λ).
- Consistance avec k_n = d log n/ε_n², ε_n → 0 et k_n/n → 0.
- Pour α = 1, le lemme 4.6 impose un k exponentiel en d. Les auteurs écrivent : « open problem… whether (α = 1, k ∼ d log n) yields consistency ».
- Borne inférieure par Fano (théorème 6.1), optimale en σ, λ et ε.
- Le graphe K‑NN (algorithme 2) exige en plus k ≳ Λ/λ, et le lemme 5.3 montre que c'est nécessaire.
- **Correspondances [D].**
  - HDBSCAN(min_samples = k) est l'algorithme 1 avec α = 1, c'est‑à‑dire le cas non prouvé.
  - `alpha` de sklearn est le α de Chaudhuri et al. (doc 1.9.1 : « distance scaling parameter as used in robust single linkage » [V]).
  - Le banc utilise `algorithm='kd_tree'`. La sonde du dépôt montre que `alpha` y a bien la sémantique de liaison simple robuste [R, CLUSTER_v2 § 4.3].
  - La référence « feuilles, α = 2 » du banc est donc dans le domaine prouvé.

*Autres résultats.*
- Eldridge et al. (2015) [V] :
  - la consistance de Hartigan tolère la sur‑segmentation et l'emboîtement impropre ;
  - minimalité et séparation l'impliquent ;
  - la liaison simple robuste converge en distorsion de fusion (théorème 7 : f c‑lipschitzienne, support compact, nombre fini de composantes).
- Kpotufe & von Luxburg (ICML 2011, 225–232) [V] : graphe K‑NN, erreur ε_k = 11F√(ln(2n/δ)/k), k compris entre environ d ln n et n^(2α/(3α+d)).
- Balakrishnan et al. (NIPS 2013) [V] : sur une variété, les vitesses dépendent de m et non de la dimension ambiante ; le rayon est relié au niveau par v_m r^m λ.
- Rinaldo & Wasserman (2010), Ann. Statist. 38(5) [V] : on cible la densité lissée p_h à h fixe, ce qui admet des mesures singulières ; sup|p_h − p̂_h| = O(√(log n/n)) sans dépendance en d.
- Wang, Lu & Rinaldo (JMLR 20(170), 2019) [V] :
  - DBSCAN à h fixe, en balayant k, estime l'arbre ;
  - vitesse (log n/n)^(α/(2α+d)), minimax aux logarithmes près ;
  - ils ne distinguent pas cœurs et bords, « no impact on the rates ».
- Jiang (ICML 2017) [V] :
  - DBSCAN, distance de Hausdorff, vitesse Õ(n^(−1/(2β+D))) ;
  - sur une variété, Õ(n^(−1/(2β+d·max(1,β)))), avec f_k défini à partir de la dimension intrinsèque ;
  - réglage adaptatif.
- Sriperumbudur & Steinwart (AISTATS 2012) et Steinwart (2015, Ann. Statist. 43(5)) [V] : estimation adaptative du premier niveau de scission ρ* et de ses composantes.

**2.4 Ce que la théorie exige de K.**
- K → ∞ avec K/log n → ∞, en pratique K ≳ C·d·log n.
- K/n → 0.
- Pour la densité ponctuelle, l'optimum est K ~ n^(2β/(2β+d)) (Dasgupta–Kpotufe).
- La séparation impose une borne supérieure : r(λ) ≤ σ/2, soit K ≲ n v_d (σ/2)^d λ. C'est le terme de biais.
- **Nos K ≤ 10 sont hors de ce régime.** Pour d = 3, d log n vaut 22,8 (n = 2 000) et 27,0 (n = 8 000) [D].
- À K fixe, seule vaut la consistance fractionnaire (Hartigan, Penrose) ; le chapitre 7 de la thèse relève de ce régime.

---

## 3. Appartenance des points : cœurs ou dilatation

- **Cœurs.** Aucun théorème ne parle du rappel des points de densité inférieure au niveau.
  - Kpotufe–von Luxburg (théorème 1a) : seuls les points de A, de densité ≥ λ, doivent être connexes au niveau λ − 2ε_k.
  - DBSCAN* retire les points de bord, un choix délibéré [V, Moulavi 2014 § 4.3, même équipe que Campello] : « more consistent with a statistical interpretation… border objects do not technically belong to the level set as their estimated density is below the threshold ».
- **Dilatation.** Ce sont des estimateurs classiques.
  - Wang, Lu & Rinaldo, éq. (5) : L̂(λ) = ∪ B(X_j, h), sur les X_j dont la densité estimée dépasse le seuil. C'est « l'estimateur de Devroye–Wise » (SIAM J. Appl. Math. 38(3), 1980) [S].
  - Cuevas, Febrero & Fraiman (Can. J. Stat. 28(2), 2000) combinent un noyau et un graphe à rayon fixe [S, Rinaldo & Wasserman ; Rolle–Scoccola].
- **L'entrée cover de v10 [D].**
  - Par définition, {x : α_K(x) ≤ r} = X ∩ (L_K(r) ⊕ B̄(0,r)). C'est une dilatation de l'ensemble de niveau continu par son propre rayon.
  - Le proxy de densité K/(n v_d α_K^d) est compris entre f_K et 2^d·f_K.
  - Esquisse, à vérifier : dans le régime K → ∞, le rayon de dilatation tend vers 0, et la consistance de Hartigan se transmet par continuité uniforme.
  - À K fixe, l'effet est du premier ordre. Au bord d'un support, une boule centrée en x perd jusqu'à la moitié de la masse. Sur une structure plus mince que r, tout point est un point de bord.
  - Mesure sur les arbres de coquilles, dev, n = 8 000 [E] : α_K/d_K a une médiane de 0,72 (K = 10) et 0,70 (K = 8), avec un minimum de 0,50, la borne. C'est cohérent avec le gain cover de +0,02 à +0,04 [R].
- **Affectation de tous les points.**
  - Azzalini & Torelli (2007), Stat. Comput. 17:71–80 [A], et le paquet pdfCluster (Azzalini & Menardi, JSS 57(11), 2014) [V]. Une seconde étape classe les points restants par log‑ratio de densités, par blocs séquentiels. Ils notent que ces points sont « inevitably in the outskirts of the cluster cores ». Le remplissage b(ρ) de v10 en est une version fruste.
  - Chacón (2015), Stat. Sci. 30(4):518–532 [V]. Le clustering modal partitionne tout l'espace (bassins du flot de gradient). La perte est P(C △ D). Sa consistance n'est prouvée qu'en dimension 1 (théorème 4.1), ce que confirme Steinwart (2015).

---

## 4. Élagage et sélection

**4.1 Élagage.**
- Par la masse :
  - McInnes & Healy (ICDMW 2017, 33–42) [V] : mcs suit le « runt pruning » de Stuetzle (J. Classif. 20:25–47, 2003) ;
  - Hartigan & Mohanty (1992), J. Classif. 9:63–70 [A] : le test du runt, qui mesure la significativité par la taille.
- Stuetzle & Nugent (JCGS 19(2):397–418, 2010) [V] :
  - excès de masse E(N) = ∫ (p − λ(N)) dx sur le nœud ;
  - estimateur Ẽ(N) = (1/n) Σ 1{x_i ∈ N}·(1 − λ(N)/p̂(x_i)), c'est‑à‑dire des poids d'importance 1/p̂ ;
  - l'excès de masse du runt est le plus petit des deux enfants ;
  - le seuil se prend à la première rupture, de façon subjective ;
  - pour l'estimateur au plus proche voisin, Ẽ se réduit à la taille.
- Chaudhuri et al. (2014, § 7) : « relying on size can be misleading ».
- Par saillance :
  - Kpotufe–von Luxburg reconnectent les composantes connexes à λ − ε̃. Avec ε̃ ≥ 3ε_k, toutes les fausses branches sont supprimées. En pratique, ε̃ = F/√k.
  - Chaudhuri et al. 2014 (théorème 7.5) : un terme relatif C_δ√(d log n/k) plus un ε̃ absolu, par exemple ε̃ = Θ(1/√k).
- Kim et al. (NIPS 2016) [V] : intervalle de confiance bootstrap sur d∞, avec un noyau à h fixe ; ils élaguent les feuilles plus courtes que 2·t̂_α.
- ToMATo (Chazal, Guibas, Oudot & Skraba, J. ACM 60(6):41, 2013) [A] : fusion guidée par la persistance, avec un seuil τ.
- Rolle–Scoccola (§ 6) [V] : aplatissement par écart de proéminence ; théorème 84 : si ε < gapsize/16, les amas sont 3ε‑entrelacés.

**4.2 L'excès de masse (EOM).**
- Origines :
  - Hartigan (1987), JASA 82(397):267–270 [S, Stuetzle & Nugent] ;
  - Müller & Sawitzki (1991), JASA 86(415):738–746 [A] ;
  - Polonik (1995), Ann. Statist. 23(3):855–881 [A] : E(λ) = sup_C (F(C) − λ|C|) ; si f existe, les maximiseurs sont les ensembles de niveau.
  - Dans ces trois travaux, λ est en unités de densité.
- Campello et al. :
  - version continue : E(C) = ∫ (f − λ_min) dx, et l'excès relatif E_R tronqué à λ_max ;
  - version empirique : S(C) = Σ (λ_max(x) − λ_min), « where the density value λ is simply set to 1/ε » [V, Malzer–Baum ; McInnes–Healy ; doc hdbscan] ;
  - sélection par FOSC (DMKD 27(3):344–371, 2013), racine exclue.
- **Non‑invariance [D, fondée sur les textes].**
  - S n'estime pas E_R : une somme sur l'échantillon estime ∫ g·f dx et non ∫ g dx. C'est ce que corrige Stuetzle–Nugent.
  - 1/ε n'est pas une densité, qui varie comme ε^(−d). McInnes–Healy l'appellent pourtant « an efficient estimate of the local density ».
  - Un changement d'échelle monotone g(λ) laisse l'arbre inchangé mais change S. Rolle–Scoccola : « Hartigan consistency is agnostic to the choice of parameterization ».
  - McInnes–Healy, éq. (3) : σ = ∫ ŝ(t)/t² dt. Pour λ = t^(−z), cela devient z ∫ ŝ(t) t^(−z−1) dt.
  - Quand z → 0 (après normalisation), on obtient la persistance en log r. Quand z → ∞, le poids se concentre aux petits rayons, ce qui donne les feuilles.
- Moulavi (2014), thèse (Alberta), § 4.4.5 [V] :
  - l'EOM relatif « goes to infinity » si quelques points sont très proches ;
  - il propose Σ ((λ_max − λ_min)/(λ_max + λ_min))^d, avec d la dimension des données.
- Biais documentés :
  - doc hdbscan [V] : l'EOM a « a tendency to pick one or two large clusters and then a number of small extra clusters » ; la sélection par feuilles donne « many small homogeneous clusters » ;
  - Malzer & Baum (MFI 2020, 223–228) [V] : avec un mcs petit, l'EOM produit des micro‑amas dans les zones denses.
  - Le biais de l'EOM change donc de sens selon l'échelle.

**4.3 Quelle paramétrisation est canonique ?**
- **Densité.** C'est l'unité des fonctionnelles d'excès de masse (Müller–Sawitzki, Polonik) et des métriques d'arbre (Eldridge et al., Kim et al., Wang et al.). Sur une variété, c'est la densité par rapport au volume intrinsèque, soit λ ∝ r^(−m) (Jiang, Balakrishnan et al.).
- **Rayon r.** C'est le choix de Chaudhuri et al.
- **Contenu de probabilité.** Rinaldo, Singh, Nugent & Wasserman (JMLR 13:905–948, 2012), § 3.2 [V], indexent l'arbre par la masse au‑dessus du niveau, ce qui le rend invariant à l'échelle de λ.
- **1/ε de HDBSCAN.** Une convention, sans justification trouvée dans les textes lus.
- **Aucun article trouvé ne fonde l'exposant de la stabilité EOM ni n'étudie la dépendance de la sélection à cet exposant.**

**4.4 Bruit.**
- Moore & Yackel (1976) [S, Dasgupta–Kpotufe] : √k(f_k − f)/f → N(0,1).
- Bornes uniformes :
  - Dasgupta–Kpotufe, lemmes 3 et 4 [V] : erreur relative ≲ C_{δ,n}/√k, avec C_{δ,n} = 16 log(2/δ)√(d log n) ;
  - Jiang : même forme sur variété ;
  - ces bornes sont vides à nos K : √(3 ln 8000/10) ≈ 1,6, avant même la constante.
- En log r, sous approximation poissonnienne [D], sd(ln d_K) = √ψ′(K−1)/m. Cela donne 0,80/m à K = 3, 0,53/m à K = 5 et 0,34/m à K = 10.
- Ce bruit est ponctuel. Il ne calibre pas la persistance d'un amas de plusieurs centaines de points, qui agrège les points. Seul le bootstrap de Kim et al. calibre la persistance d'un amas ; il n'est pas testé ici.

**4.5 Mini‑expérience [E] : fonctionnelles de sélection sur les coquilles.**
- Arbres existants `bench/diag_shells/tree_k{8,10}` (dev, 8 × 1 000 points, entrée cover), en Python pur, aucun binaire relancé. Script : `…/litterature_statistique/sel_variants.py`, sortie `sel_variants_shells.txt`.
- La condensation reproduit le diagnostic : 3 amas à ẑ = 2,1 pour K = 10.
- mcs = 89, EOM racine exclue, pas de remplissage.

| Sélection | K = 10 | K = 8 |
|---|---|---|
| EOM échelle log r (limite z → 0) | 3 | 3 |
| EOM z = 1 | 3 | 3 |
| EOM z = 2 | 3 | **8** |
| EOM z = ẑ = 2,1 | 3 | **8** |
| EOM z = 3 et z = 4 | **8** | **8** |
| EOM z = 8 | 15 (ARI_s 0,83) | 15 (ARI_s 0,87) |
| Feuilles | 31 | 36 |
| Masse de Lebesgue (poids 1/f, Stuetzle–Nugent), m = 1 ; 2,1 ; 3 | 3 ; 3 ; **8** | 3 ; **8** ; **8** |
| EOM bornée (Moulavi), d = 1 ; 2,1 ; 3 | 3 dans les trois cas | 3 dans les trois cas |
| Contenu de probabilité (Rinaldo et al.) | 30, proche des feuilles | 36, proche des feuilles |

- Lecture :
  - l'effet de z est monotone et va vers les feuilles quand z croît ;
  - la bonne fenêtre existe mais dépend de K ;
  - aucune variante de la littérature ne fait mieux que l'EOM classique avec un z bien choisi ;
  - à z = 2,1, la sélection passe de 8 amas (K = 8) à 3 (K = 10). C'est une instabilité en K du même type que la proposition 45 de Rolle–Scoccola.

---

## 5. Choix de K et axe multiparamètre

- **Rolle & Scoccola (JMLR 25(258):1–74, 2024 ; lu en arXiv v4) [V].**
  - Degree‑Rips : la liaison simple robuste en est la tranche à k fixe, le plug‑in la tranche à rayon fixe.
  - Proposition 44 : la liaison simple robuste avec κ ≥ 2 fixé est discontinue pour la distance GHP.
  - Proposition 45 : passer de κ à κ′ peut changer arbitrairement le résultat.
  - Résultat A : d_CI(DR(M), DR(N)) ≤ 2·d_GHP(M, N).
  - Résultat B : les tranches linéaires de pente négative (λ‑link) sont stables.
  - Résultat C : ces tranches sont consistantes pour toute densité continue à support compact.
  - « γ‑link » désigne dans le texte la tranche le long d'une courbe γ générale.
- Blumberg–Lesnick (§ 1.5) : la multicouverture est stable en Prohorov ; les bifiltrations en degré ne le sont que faiblement, avec une constante 3 optimale.
- Carlsson & Zomorodian (DCG 42(1):71–93, 2009) et Lesnick & Wright (arXiv 1512.00180) [S].
- Neto, Sander, Campello & Nascimento (ICDM 2017, arXiv 1709.04545) [V] : on peut calculer toutes les hiérarchies HDBSCAN* pour une plage de mpts ; « certain data clusters may reveal themselves at different values of mpts ».
- K adaptatif :
  - Dasgupta–Kpotufe et Jiang : réglage adaptatif de k et de la dimension ;
  - Steinwart 2015 : largeur adaptative ;
  - Rinaldo et al. 2012 : choix de la largeur par instabilité.
- Aucun de ces travaux ne donne de règle prouvée pour choisir K scène par scène dans le régime de petits K fixes.

---

## 6. HGP‑old au regard de la littérature (lu comme spécification)

**6.1 Constats de lecture du code.**
- **Échelle λ = r^(−expZ).**
  - Code : `clustering.py:23` et `_cython.pyx:483`, λ = 1/(r+ε), où le poids r vaut rayon^expZ (`hypergraph.py:119–128`). Valeur par défaut expZ = 2 (`core.py:51`).
  - Carnets :
    - expZ = 3 (`HGP-clusterer Colab.ipynb:14202`, K = 5) ;
    - expZ = 8 (`tests/OliveOil/HGP-Clusterer_OliveOil.ipynb:161`, 8 variables) ;
    - expZ = 4 (`tests/Maya/HGP-Clusterer_Maya.ipynb:169`) ;
    - expZ = 2 (`tests/MarieBenchmark/HGP_clusterer_MarieBenchmark.ipynb:765`) ;
    - expZ = 1 dans SemanticKITTI (`…_final.py:586`), mais avec une coupe de type DBSCAN et non l'EOM (`:1151`).
  - C'est ψ(t) = t^(−p) du § 9.1 de la thèse.
  - En revanche, la comparaison SIPU (§ 9.2.5) est faite avec ψ = 1/t, c'est‑à‑dire z = 1.
- **Masses des faces.**
  - S_τ = Σ r_σ^(−expZ) (`_cython.pyx` ≈ 836–870 ; `hypergraph.py:252–260`) ;
  - T_x à `core.py:208`, m_τ à `core.py:214–215`.
- **Entrée des faces.** Une face ne rejoint le graphe qu'à sa première coface, un (K+1)‑simplexe (arêtes : `hypergraph.py:269–273`).
  - Un point entre donc vers min ρ(σ) sur les σ ∋ x de taille K+1, soit ≈ α_{K+1}(x) [D].
  - C'est un ordre plus haut que l'entrée α_K de v10‑b.
  - Les candidats viennent de la mosaïque de Delaunay d'ordre K (`hypergraph.py:70–77`), avec min_samples = K+1 par défaut (`core.py:147–148`). Cela repose sur la proposition 6 et le théorème 5, faux en général (E5) [R].
- **Forêt.** Un arbre condensé par composante connexe (`core.py:233–265`). Les racines sont sélectionnables (`clustering.py:212`), avec λ_mort = 0 pour la racine (`_cython.pyx:635`).
- **Étiquettes.** Vote argmax Σ S_τ (`core.py:320`), puis remplissage optionnel au plus proche voisin (`core.py:366–380`).
- Ces choix sont des heuristiques. Aucun résultat statistique vérifié ne s'y rapporte.

**6.2 Mécanismes plausibles de ses bons résultats (hypothèses, non testées).**
- **H1.** Hors de la comparaison SIPU, ses carnets font tourner l'EOM à z = p ≥ 2 : c'est l'EOM « en unités de densité » des textes fondateurs. Cela l'éloigne du biais « parent » de HDBSCAN à z = 1, comme sur les coquilles [E]. Cela n'explique pas SIPU, fait à z = 1.
- **H2.** L'appartenance par faces relève de la sémantique des amas discrets, qui corrige le déficit de bord (§ 3). Le gain est mesuré dans v10 [R].
- **H3.** L'entrée à l'ordre K+1 est plus prudente. Elle limite la prolifération de petites feuilles que v10 attribue à l'entrée cover.
- **H4.** Les masses m_τ ∝ S_τ déplacent la masse de chaque point vers ses faces les plus denses, un effet analogue à une hausse de z.
- **H5.** La racine par composante est sélectionnable, comme `allow_single_cluster` ; FOSC l'exclut.
- **H6.** La restriction de Gabriel peut manquer des fusions, ce qui retarde les scissions et favorise les enfants.

---

## 7. Conséquences pour la tour v10

**Ce qui est justifié.**
1. **L'objet.** La tour est l'arbre plug‑in exact de l'estimateur K‑NN.
   - Il est indépendant de d et de l'échelle de λ.
   - Il est consistant (Hartigan, distorsion de fusion) si K/log n → ∞, K/n → 0 et f est uniformément continue (Devroye–Wagner, lemme A.1, Eldridge et al. th. 5 ; corollaire [D]).
   - Le paramètre α disparaît.
   - La liaison simple robuste n'en est qu'une approximation, entrelacée avec un facteur 3 à α = 1 (Blaser et al.) et prouvée seulement pour α ∈ [√2 ; 2] avec k ≳ d log n.
2. **Garder les applications verticales.** Seule la bifiltration a un théorème de stabilité (Blumberg–Lesnick). Les tranches à K fixe sont instables (Rolle–Scoccola ; ici, coquilles à K = 8 contre K = 10).
3. **L'entrée cover.** C'est un estimateur par dilatation classique (Devroye–Wise, Wang et al.), asymptotiquement équivalent aux cœurs. Son gain est un effet de bord à K fixe. Le choix inverse de DBSCAN* est une définition, pas une nécessité statistique.
4. **Traiter z comme un hyperparamètre de sélection** choisi sur dev, et publier la fenêtre de z où la sélection reste stable. La non‑invariance de l'EOM est démontrable. Un résultat à z ≠ 1 n'est pas « l'EOM de HDBSCAN ».
5. **Garder α ∈ {1 ; 2} côté sklearn.** α = 2 est la référence prouvée, α = 1 le cas ouvert.

**Ce qui n'est pas justifié.**
1. **Aucune garantie quantitative à nos K** (K ≤ 10 < d log n ≈ 23 à 27) : toutes les bornes sont vides. L'optimum K = 3 est un constat empirique du régime à K fixe (percolation, chapitre 7 de la thèse, asymptotique et en partie conjectural).
2. **ẑ n'est pas l'échelle canonique de l'EOM.** La densité intrinsèque vaut pour les ensembles de niveau, pas pour l'EOM. Les coquilles réfutent ẑ comme règle à K = 10.
   - z = p (thèse § 9.1, HGP‑old) n'est pas mieux fondé.
   - Pour des scènes de dimension mixte, un ẑ global n'a aucun appui.
3. **Des seuils de persistance en 1/√K** calibrent le bruit ponctuel, pas celui des amas.
4. **« HDBSCAN ne peut pas battre la tour » n'est pas un théorème.** La littérature fait de la tour l'objet cible, pas un meilleur estimateur à n et K finis. Le banc compare aussi des têtes différentes (EOM à z contre feuilles à α = 2). La version testable : même tête sur la tour et sur les hiérarchies MR₁ et MR₂ du dépôt.
5. **Le « paradoxe du choix de K »** (thèse § 4.4.5) s'explique par le terme de biais : K ≲ n v_d (σ/2)^d λ. Il n'est pas propre à la connexité de HDBSCAN, et la tête v10 en cœurs se dégradait aussi quand K augmentait.
6. **Les masses et le vote du § 9.1** n'ont aucun analogue statistique vérifié.

**Ce qui reste ouvert.**
1. **Une sélection fondée.** Pistes testables sur les arbres exportés, sans recalculer la tour :
   - un z local, c'est‑à‑dire l'EOM en densité intrinsèque par amas ;
   - l'écart de proéminence sur des tranches obliques (r, K) à la Rolle–Scoccola. ToMATo par écart à K fixe a été mesuré à 0,55–0,59 [R] ;
   - le bootstrap de Kim et al. ;
   - la persistance à travers K via les verticales, motivée par Blumberg–Lesnick mais sans théorème de sélection.
   - L'EOM bornée de Moulavi échoue sur les coquilles, et l'indexation par contenu de probabilité se comporte comme les feuilles [E].
2. **Familles allongées.** La théorie demande un K petit (σ face à r_K) et l'échelle intrinsèque (m ≈ 1). Elle suggère un K ou un z locaux, sans garantie.
3. **Le régime K/n fixe** (multicouverture en masse, DTM) : une cible stable en Prohorov, sans hypothèse de densité, pertinente pour les surfaces LiDAR. Non exploré.
4. **Priorité et nouveauté** du calcul exact du plug‑in K‑NN en 3D : à vérifier par une recherche dédiée, avec Edelsbrunner–Osang, Corbet et al. et Blaser et al.

**Fichiers de travail**, tous dans `/workspaces/E-HGP/build/v10-persist/audit_hier/litterature_statistique/` :
- `sel_variants.py` : le script de la mini‑expérience ;
- `sel_variants_shells.txt` : sa sortie ;
- `pdf/` : les articles téléchargés et leurs extraits `.txt`, plus le rendu de la page 93 de la thèse de Moulavi (`moulavi_p93-093.png`) ;
- `these.txt`, `corbet.txt`, `edelsbrunner_osang.txt`, `hauseux_ans.txt` : extraits texte de la thèse et des PDF locaux.