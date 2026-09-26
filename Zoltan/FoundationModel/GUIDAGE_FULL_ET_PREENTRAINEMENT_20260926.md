# Comment FULL peut guider l'apprentissage

26 septembre 2026, suite de l'[audit architectural](AUDIT_V9_ET_ARCHITECTURE_20260926.md) et du [contrat des coupes](CONTRAT_COUPES_ET_MASSES_20260926.md), base fba0d0225. Morse HGP 3D v9 est posé exactement conforme à sa spécification. On cherche ici une expérience qui décide **où sa structure aide le réseau**.

Cadre : `phase=conception_modele_fondation_hors_registre`, `backend=python_reference`, `mode=guidage_et_contre_exemples_bornes`, `public_status=not_claimed`. Les exemples sont synthétiques ; aucun entraînement, aucune étiquette SemanticKITTI, aucun calcul GCP. Les formules de guidage sont des propositions de cette tranche, pas des propriétés déjà qualifiées du tokenizer.

## 1. La décision qui simplifie le premier essai

**Tester FULL comme enseignant avant de rendre son usage obligatoire dans le backbone.** La tour peut produire des relations entre retours et des régions de supervision ; un encodeur PTv3 ordinaire apprend sur ces cibles. À l'inférence, ce bras ne calcule aucune tour. HGP-UNet reste le bras qui consomme FULL à l'inférence. Les deux usages répondent à des questions différentes et peuvent se combiner.

Ce choix ne réduit pas FULL à un vecteur de statistiques : l'enseignant interroge plusieurs coupes et, ensuite, plusieurs K. Il transmet des relations datées. Il permet aussi de commencer avec `coverage_v1` à K1, avant le supplément pondéré nécessaire à FP/PUR à K supérieur.

Le plan minimal croise les deux interventions :

| bras | architecture de l'élève | recette SSL | FULL à l'inférence |
| --- | --- | --- | --- |
| A0G0 | référence PTv3 | recette de référence | non |
| A0G1 | même PTv3 | même recette + guidage FULL | non |
| A1G0 | première substitution FP/PUR vérifiée | recette de référence | oui |
| A1G1 | même substitution que A1G0 | même recette + même guidage | oui |

Commencer par **A0G0/A0G1**. Ne lancer A1 qu'une fois l'export pondéré prêt. Avec une métrique aval S, les effets sont $S_{01}-S_{00}$, $S_{10}-S_{00}$ et l'interaction $S_{11}-S_{10}-S_{01}+S_{00}$. Une absence de gain du pooling ne réfute donc pas l'usage comme enseignant ; une interaction négative peut indiquer que les deux contraintes sont redondantes.

Apparier données, augmentations, encodeur dans chaque paire, graines et protocole. Rapporter deux comparaisons : exposition égale aux scans, puis budget de calcul total égal. Époques et paramètres égaux ne suffisent pas à égaliser FLOPs, lecture du cache et préparation. Garder les mêmes requêtes/cibles FULL dans les bras G1 pour isoler l'effet architectural.

## 2. Ce que l'élève a le droit de voir

Pour le pilote, T est une vue enseignante d'une seule trame, transformée puis quantifiée. V résulte d'une suppression de retours de T. Conserver IDs originaux, même transformation, même grille/origine et masque sol préalable ; ne pas recentrer ou re-quantifier différemment V dans cette comparaison d'inclusion. Dédupliquer les sites selon le contrat, sans perdre leurs retours.

~~~text
trame -> vue T -----------------> FULL(T) -> cibles et poids de perte
            |
            +-> masque -> V ----> encodeur(V) -> predictions
                                  [FULL(V) seulement dans A1]
~~~

**Tout le forward élève dépend de V et de la configuration publique.** Les parents, niveaux, quantiles, degrés, IDs de tokens, matrices P et graphes issus de FULL(T) restent du côté de la perte. Retirer seulement le rayon cible d'une feature ne suffit pas si le biais d'attention ou le choix des voisins l'encode déjà. Dans A1, recalculer FULL(V) ; élaguer la tour enseignante conserve des fusions causées par des retours cachés.

Contrôle causal concret : fixer V et la seed, changer uniquement les retours cachés de T. Les entrées et le forward élève doivent rester identiques ; seules les cibles peuvent changer. L'exemple du §3 montre précisément une paire de telles complétions. Les scripts bornés exercent le contre-exemple, pas un forward PTv3 qui n'existe pas encore ici.

Le masque et les requêtes du premier pilote sont tirés depuis V/les IDs et une seed, indépendamment des réponses FULL(T). Masquer un nœud enseignant entier devient ensuite une **intervention distincte** : sa sélection peut déjà révéler la structure cible. Comparer à des masques géométriques de même volume et nombre de retours. Ne pas envoyer le token ou les statistiques de la région supprimée dans le forward.

## 3. Premier prétexte : prédire des relations K1 à des rayons interrogés

### Cible exacte, prédiction conditionnelle

Pour deux sites distincts i,j retenus dans V, un rayon $0<r\leq H$ et une coupe fermée, poser :

$$Y_T(i,j,r)=\mathbf{1}\lbrace i,j\ \text{sont dans la meme composante K1 de T au rayon }r\rbrace.$$

FULL fournit le label ; le modèle reçoit la requête r et prédit sa probabilité depuis les représentations de V. Les sites sont présents dès zéro à K1 ; on évite ainsi les naissances partielles et le recouvrement de K supérieur dans ce premier essai.

Pour une suppression pure à coordonnées fixes, $L_K(V,r)\subseteq L_K(T,r)$. À K1 cela donne $Y_V(i,j,r)\leq Y_T(i,j,r)$ sur les IDs communs, **pas une égalité**. Exemple : T={0,1,2}, V={0,2}, plongés sur un axe de $\mathbb{R}^{3}$. Les extrémités fusionnent à 1/2 dans T, à 1 dans V. Au rayon 3/4, les réponses sont respectivement 1 et 0.

La même entrée V peut aussi provenir d'un enseignant T'={0,2,4}, de même cardinal que T, dont la réponse est 0. La structure cachée n'est donc pas identifiable en général. La perte apprend une prédiction conditionnelle ; une cible HGP exacte n'autorise pas à promettre une reconstruction exacte depuis l'occultation. Avec deux complétions équiprobables de labels opposés, la meilleure probabilité est 1/2.

### Tête et perte

Une tête symétrique reçoit $(z_i+z_j,\ |z_i-z_j|,\ \log(r/r_0))$, avec $r_0$ une unité fixe déclarée, et produit $\widehat Y_{ijr}$. Elle ne reçoit directement ni coordonnées brutes de la paire ni variables FULL enseignantes : les représentations portent l'information. Les coordonnées restent évidemment nécessaires à l'encodeur 3D ; ce choix n'élimine pas à lui seul le raccourci géométrique.

$$\mathcal L=\mathcal L_{\mathrm{SSL,base}}+\lambda\,\frac{\sum_{(i,j,r)\in E}\omega_{ijr}\,\mathrm{BCE}(\widehat Y_{ijr},Y_T(i,j,r))}{\sum_{(i,j,r)\in E}\omega_{ijr}}.$$

E est un ensemble borné de requêtes, partagé entre bras ; les poids et la loi d'échantillonnage sont déclarés. Utiliser des strates de distance, portée et rayon choisies sans lire Y, avec vrais positifs et négatifs vérifiés après construction. Si une strate est triviale, la rapporter ; ne pas annoncer une compétence depuis des labels constants. Un rééquilibrage ultérieur selon Y doit annoncer le changement de distribution ou employer des poids de correction.

Les relations de connexité décrivent le scan, **pas une égalité de classe sémantique**. Deux voitures séparées peuvent avoir la même classe, et route/voiture partager une composante. Garder la tête auxiliaire et la recette SSL de base ; ne pas imposer que la distance des embeddings soit exactement la distance de fusion. Son utilité se décide par sonde linéaire, faible annotation et transfert, pas par la seule baisse de BCE.

Comparer la prédiction à : distance directe $d(i,j)/2$, statistiques locales et rayon K-NN, puis graphe K1/MST calculé sur V. Distinguer le cas où le contexte visible suffit du cas où un pont caché change la réponse. À K1, HGP et single-linkage donnent le même objet : un gain de ce pilote établit la valeur du guidage hiérarchique, **aucune spécificité d'ordre supérieur**. Celle-ci exige les ablations K1 contre plusieurs K.

Rapporter séparément les requêtes avec $Y_V=1$, où l'inclusion donne déjà la réponse enseignante, et celles avec $Y_V=0$. Ajouter calibration/Brier et violations de monotonie en r aux scores de prédiction. Une tête sigmoïde libre n'est pas monotone par construction ; un contrôle mesuré ne devient pas une garantie.

### Contrôle indispensable : le rayon seul peut résoudre le prétexte

La présence de positifs et de négatifs dans E n'écarte pas une représentation constante : la tête reçoit r et peut apprendre $q_0(r)=\Pr(Y_T=1\mid r)$. Deux groupes de rayons ayant des prévalences 1/10 et 9/10 donnent 90 % de réponses correctes à un prédicteur sans aucune feature de point, alors que le corpus global est équilibré. Ajouter un témoin **encodeur constant + même tête**, puis un témoin de statistiques visibles de trame/portée/distance. Ajuster ces témoins sur l'entraînement seulement. Une diminution de BCE ou une variance non nulle des embeddings ne suffit pas à les dépasser.

Sur le sous-ensemble $Y_V=0$, le problème informatif est la création d'un chemin par les retours cachés. Le prédicteur idéal se décompose en $Y_V+(1-Y_V)q(V,i,j,r)$. Cette identité sert à analyser la perte, sans ajouter FULL(V) au forward A0. Publier la fraction de chaque sous-ensemble, BCE/Brier par strate et l'amélioration par rapport au prior visible. Un Brier skill $1-\mathrm{BS}_{\mathrm{modele}}/\mathrm{BS}_{\mathrm{temoin}}$ n'est défini que si le dénominateur est positif ; un témoin parfait est une strate déjà résolue. L'apport de contexte se mesure sur des paires appariées en distance, rayon et statistiques locales, puis par retrait du contexte distant. Le gain aval reste le critère de décision.

L'échantillonnage des cas $Y_V=0$ peut accélérer l'apprentissage, puisqu'il ne lit que la vue élève ; il change néanmoins la distribution des requêtes. Conserver une évaluation représentative de la loi E déclarée, en plus de ce diagnostic conditionnel. Les scores de calibration ne se calculent pas sur un lot artificiellement équilibré selon les labels sans correction de ses probabilités de sélection. Garder un support de probabilité non nul pour les strates sur lesquelles on revendique une performance.

### Variante constructive : une CDF monotone avec queue censurée

Dans cette variante, la tête reçoit les features de la paire/vue et produit
**une distribution fixe pour cette paire/vue**. Le rayon interrogé choisit
seulement le cumul ; il ne modifie pas la distribution prédite, contrairement
à l'entrée rayon de la tête sigmoïde libre.

Pour des seuils publics $0<b_1<\cdots<b_M=H$, une tête par paire peut sortir une distribution $(p_1,\ldots,p_M,p_{>H})$ via softmax, puis $\widehat Y(b_j)=\sum_{m\leq j}p_m$. Les bins sont $(0,b_1],(b_1,b_2],\ldots,(b_{M-1},H]$ ; la dernière masse représente une fusion au-delà de H. Les réponses sont monotones par construction aux seuils déclarés. Une fusion située dans un bin supervise sa masse ; une non-fusion à H supervise la queue. Un horizon exporté plus court utilise la probabilité de survie au seuil correspondant, jamais une fusion inventée à cet horizon. Une requête à rayon arbitraire demande une règle d'interpolation déclarée et ne devient pas exacte par cette paramétrisation. Comparer cette variante au sigmoid libre avec le même budget ; aucun réseau n'est implémenté ici.

La monotonie en r ne rend pas les **probabilités conditionnelles** ultramétriques entre paires. À V et r fixes, un mélange équiprobable de partitions $\{i,j\}|\{k\}$ et $\{i\}|\{j,k\}$ donne $(p_{ij},p_{jk},p_{ik})=(1/2,1/2,0)$. Il est valide malgré $p_{ik}<\min(p_{ij},p_{jk})$. En revanche, la transitivité dans chaque réalisation impose $p_{ik}\geq p_{ij}+p_{jk}-1$ : $(9/10,9/10,1/10)$ est impossible. Ces inégalités nécessaires ne suffisent pas à certifier une loi jointe de partitions sur un grand ensemble. Ne pas forcer la tête conditionnelle à être une hiérarchie dure ; FULL enseignant demeure exact réalisation par réalisation.

### Horizon fini

Si i,j n'ont pas fusionné avant H, les labels 0 pour $r\leq H$ sont valides. Le rayon de fusion est censuré au-delà de H ; il ne vaut ni H ni l'infini démontré. Préférer les requêtes binaires à une régression de $\log r_{\mathrm{fusion}}$ qui inventerait une valeur. À K supérieur, distinguer absence de naissance, absence de coappartenance et niveau non exporté.

## 4. Correspondances entre vues : la masse commune n'est pas une identité

Supposer deux matrices d'affectation $P^a,P^b$, normalisées sur tous les retours représentés, **réserves comprises**. Aligner leurs lignes sur J, l'intersection des IDs de retour. Choisir une seule mesure positive $\nu$ sur J ; le pilote prend un poids par retour. Deux mesures par site recalculées séparément après décimation peuvent donner des multiplicateurs différents aux mêmes IDs et ne constituent pas cette mesure commune.

Cette jointure est disponible pour des vues issues des mêmes retours et pour les IDs du scan ancre conservés dans un agrégat. Deux acquisitions réelles distinctes n'ont pas ces IDs communs : leur appariement demande pose, visibilité ou correspondances supplémentaires, que FULL ne fournit pas.

$$G=(P^a_J)^\top D_\nu P^b_J,\qquad m^a=(P^a_J)^\top\nu,\qquad m^b=(P^b_J)^\top\nu.$$

On a $G\mathbf{1}=m^a$ et $G^\top\mathbf{1}=m^b$. Pour $m^a_u>0$, $T_{a\leftarrow b}=D_{m^a}^{-1}G$ est un mélange de lignes de somme 1. Les masses sont celles **du domaine commun**, pas celles des tokens complets avant crop. Une ligne de masse commune nulle est masquée ; aucune cible n'est créée par voisin le plus proche pour remplir ce vide. Publier masses communes/totales dans les deux sens et la masse envoyée aux réserves.

Ces identités ne font de T ni une verticale FULL, ni un appariement bijectif, ni une preuve de stabilité. Pour $P^a=P^b=((1,0),(1/2,1/2),(0,1))$ et poids unitaires, on obtient :

$$T_{a\leftarrow a}=\begin{pmatrix}5/6&1/6\\1/6&5/6\end{pmatrix}\ne I.$$

Exiger $z=T_{a\leftarrow a}z$ force ici les deux tokens à avoir les mêmes caractéristiques. Répéter le transport lisse même une vue inchangée. Le durcir par argmax efface au contraire son recouvrement. **Employer G pour agréger des observations communes, pas pour imposer une identité de tokens.**

Pour comparer des descripteurs régionaux, une option plus lisible est de regrouper les features élève et enseignant **avec la même matrice enseignante restreinte à J** :

$$\bar z^s_v=\frac{\sum_{i\in J}\nu_iP^T_{iv}z^s_i}{m^T_v},\qquad \bar z^t_v=\frac{\sum_{i\in J}\nu_iP^T_{iv}z^t_i}{m^T_v},\qquad m^T_v>0.$$

P et les features enseignantes n'ont pas de gradient ; P reste dans la perte. Ceci compare le même support pondéré sans demander que les tokens élève correspondent. Exclure les réserves des cibles régionales, tout en gardant leur masse dans la comptabilité. Une région ayant peu de masse observable peut être écartée par une règle déclarée ; ne pas renormaliser silencieusement le déficit en évidence positive.

**L'accord régional seul admet une représentation constante.** Une cible dense ou un enseignant EMA n'annule pas ce problème par définition. Garder le mécanisme de diversité de la recette SSL de référence et les relations FULL fixes ; mesurer variance/rang des features, distribution des prédictions et sonde linéaire. Ces contrôles diagnostiquent l'effondrement, sans prouver une utilité sémantique.

Le calcul de G coûte $\sum_{i\in J}d_a(i)d_b(i)$ contributions, où d est le nombre de poids non nuls par retour. Ne pas former une matrice dense tokens×tokens. Le regroupement commun des features coûte plutôt des parcours des incidences enseignantes, ce qui justifie de l'essayer d'abord.

## 5. Étendre aux ordres supérieurs sans inventer une identité de nœud

Un point peut appartenir à plusieurs composantes K. Remplacer directement Y par « même node_id » perd donc l'information recherchée. Avec l'export pondéré, une cible possible à chaque coupe est :

$$a_i=\sum_{v\ \mathrm{reel}}P_{iv},\qquad \Gamma_{K,r}(i,j)=\sum_{v\ \mathrm{reel}}P_{iv}P_{jv}.$$

« réel » exclut les réserves. Γ est la masse de deux choix indépendants d'incidence aboutissant au même token ; ce n'est ni une relation d'équivalence, ni la probabilité que les retours aient la même classe. Elle peut avoir une diagonale inférieure à 1 ; ne pas forcer un auto-appariement certain. Évaluer seulement i≠j.

Si l'on veut conditionner sur les incidences représentées, utiliser $\Gamma_{K,r}/(a_i a_j)$ seulement pour $a_i a_j>0$, en conservant $a_i,a_j$ et le masque de supervision. Cette cible conditionnelle est une variante déclarée ; elle ne récupère pas l'information absente. Pour la lecture non conditionnée, une petite valeur peut provenir d'un défaut de couverture et ne doit pas être interprétée automatiquement comme une séparation.

Même à couverture complète, Γ dépend de la **concentration des incidences**. Deux distributions identiques uniformes sur m tokens ont Γ=1/m : passer de un à dix tokens communs fait baisser la cible de 1 à 1/10 sans désaccord des distributions. Déclarer le sens recherché avant la perte : collision de deux tirages pour Γ, ou ressemblance des distributions. Pour cette seconde question, une alternative bornée est $O(i,j)=\sum_v\min(\pi_{iv},\pi_{jv})=1-\|\pi_i-\pi_j\|_1/2$, avec $\pi_i=P_i/a_i$ sur les tokens réels et $a_i>0$. Elle vaut 1 pour des distributions identiques. O ne remplace pas Γ silencieusement et ne mesure pas davantage une identité sémantique. Comparer les deux cibles à couverture et concentration $\sum_v\pi_{iv}^2$ déclarées ; des matrices rationnelles arbitraires ne prouvent pas leur fréquence dans FULL.

Le « profil en K » devient un vecteur de ces lectures pour **les mêmes IDs et rayons**, avec leur couverture, plutôt qu'un « ordre où le nœud disparaît » sans correspondance définie. Entre vues et entre K, garder les branches et leurs mesures autonomes. Les requêtes de paires évitent la matérialisation de $PP^\top$ ; elles ne suppriment pas le coût d'export des incidences.

## 6. Sort des six prétextes proposés

| prétexte | formulation utilisable | ordre proposé |
| --- | --- | --- |
| FM-1 fusion | requêtes K1 de connexité à r ; ensuite lectures pondérées multi-K | premier pilote |
| FM-2 persistance | prédiction de survie/rayon avec naissances, horizon et censure explicites ; ancêtres enseignants invisibles au forward | après FM-1 |
| FM-3 profil K | mêmes paires/retours interrogés dans plusieurs ordres, couverture et réserve publiées | après supplément pondéré |
| FM-4 rétablissement | cible enseignante plus informée, prédiction conditionnelle sur retours visibles ; masques indépendants puis structurels ablatés | variante de FM-1 |
| FM-5 accord inter-vues | mêmes IDs et supports pondérés dans la perte, sans égalité imposée des arbres recalculés | avec la SSL de base |
| FM-6 agrégat temporel | cible de complétion distincte, alignement et visibilité explicites | régime temporel secondaire |

FM-6 utilise de l'information supplémentaire pendant le pré-entraînement même si l'inférence est mono-trame. Le régime primaire de ce dossier reste LiDAR mono-scan sans historique : comparer d'abord des vues de la même trame. FM-6 devra comparer tous ses bras avec **le même accès aux trames et à l'odométrie**. Fenêtres, objets mobiles, bruit de recalage et géométrie cachée rendent l'agrégat différent d'une vérité physique ; l'absence de retour n'est pas un label d'espace vide.

## 7. LiDAR : conserver les informations dont le modèle a besoin

La tour fournit un estimateur exact sur les retours observés. Sur une surface plane idéale à densité surfacique uniforme λ, l'approximation $K\simeq\pi\lambda r_K^2$ donne $K/(n\omega_3r_K^3)\simeq\pi^{3/2}\lambda^{3/2}/(n\omega_3\sqrt{K})$. Ce calcul illustre un effet d'échantillonnage et de dimension ; il ne transforme pas la densité ambiante 3D en densité physique d'objet.

Le témoin « densité seule » doit donc inclure un profil de $\log r_K$, comptes multi-rayons et anisotropie, avec portée/rémission disponibles dans tous les bras concernés. Fixer la convention du voisin propre et des doublons. Un gain sur un unique canal mal normalisé ne suffit pas à prouver la valeur de la tour ; un témoin local fort rend la conclusion plus utile.

Conserver d'abord un encodeur de retours avec connexion fine : coordonnées physiques, rémission et statistiques pondérées simples, puis quelques canaux de forme ablatés sur les **mêmes régions**. Donner une taille physique en entrée permet au réseau de l'utiliser ; cela ne garantit aucune équivariance apprise. Retirer la métrique ou imposer une invariance totale peut confondre des objets de tailles différentes.

Le sans-sol est une variante d'architecture légitime et un régime prioritaire du moteur. Pour la segmentation de tous les retours, comparer : **tour brute** contre **tour non-sol + branche de contexte sol**, avec masque géométrique figé avant les vues et raccord vers tous les IDs. Les retours unknown restent déclarés ; un masque géométrique n'est pas une classe sémantique. Coût du masque, erreurs de retrait et contexte perdu font partie de l'expérience. Ne pas exclure cette solution pour préserver un discours « sans constantes » : Kmax, condensation, choix de coupes et budget comportent déjà des choix.

## 8. Ce qui permettra de décider

1. **Avant apprentissage :** export K1 daté, requêtes positives/négatives et censurées ; mêmes IDs ; aucun accès élève aux objets enseignants ; stabilité des statistiques sous les augmentations déclarées.
2. **Premier apprentissage :** A0G0/A0G1 avec λ=0 comme référence, tête auxiliaire identique pour comptabiliser son coût ; priors rayon seul et statistiques visibles, témoin local, puis cibles brouillées dans des strates distance/portée/rayon conservées. Mesurer le gain conditionnel sur $Y_V=0$ et le gain sur une loi de requêtes représentative. Le brouillage teste l'information des labels, pas une fausse tour à parents invalides.
3. **Mesure aval :** sonde linéaire et peu d'étiquettes, puis transfert de capteur avec réglages gelés ; au moins trois graines, résultats par classe/portée et matrices de confusion agrégées. Ces expériences ne sont pas exécutées dans cette tranche.
4. **Valeur propre des ordres :** K1 seul, plusieurs K avec budget de requêtes total identique, puis K répété avec mêmes têtes/capacité. La prédiction d'un profil K plus gros ne constitue pas à elle seule un gain de fondation.
5. **Choix final :** guidage utile seul → conserver l'enseignant hors ligne ; architecture utile seule → garder la substitution ; complémentarité mesurée → combiner. Une perte prétexte excellente sans amélioration aval justifie de retirer ce prétexte.

Un `GuidanceBundle` peut stocker des tuples (IDs originaux i,j, K, rayon carré exact, côté, cible, couverture, horizon, poids), avec digest de T, politique de masque et loi des requêtes. Il ne contient pas nécessairement le FULL complet. Son volume dépend du budget explicite de requêtes ; le pré-calcul FULL et le coût de l'adaptateur restent payés. Ce format est une proposition, pas un exporteur déjà implémenté.

## 9. Appuis et portée des vérifications

[Sonata](https://arxiv.org/html/2503.16429v1) motive le contrôle des raccourcis transmis par les opérateurs spatiaux et l'évaluation par sonde linéaire. [DOS](https://arxiv.org/html/2512.11465v1) fournit un précédent LiDAR de distillation sur points observables avec un traitement de la diversité des prototypes. Ces travaux n'établissent ni le gain de HGP ni la validité des nouvelles pertes ; ils servent à choisir des contrôles. Les formules de connexité ci-dessus viennent de Morse HGP à K1, et celles de masse du contrat du dossier.

[Topological Autoencoders](https://proceedings.mlr.press/v119/moor20a.html) fournit un antécédent de régularisation latente par connexité multi-échelle, sans établir de résultat LiDAR pour ce projet. [GroupContrast](https://openaccess.thecvf.com/content/CVPR2024/html/Wang_GroupContrast_Semantic-aware_Self-supervised_Representation_Learning_for_3D_Understanding_CVPR_2024_paper.html) motive la séparation entre discrimination géométrique et similitude sémantique. Pour le LiDAR temporel, [BEVContrast](https://arxiv.org/abs/2310.17281) et [TARL](https://www.ipb.uni-bonn.de/pdfs/nunes2023cvpr.pdf) sont des comparateurs de regroupement régional ; leur accès aux scans et poses doit être apparié dans FM-6. Ces précédents complètent le contrôle local et K1/MST, qui reste le témoin exact du premier prétexte.

La [référence de contrôle des cibles](reference/verify_guidance_controls.py) ajoute quatre contre-exemples rationnels : prior en rayon, concentration de Γ, marges de partitions et CDF censurée. Elle ne teste ni une tête entraînée ni la réalisabilité HGP de ses matrices abstraites. Les identités probabilistes sont des déductions du présent audit, pas des résultats importés de ces publications.

Les [reçus de cette tranche](receipts/guidance_20260926/README.md) séparent les contre-exemples géométriques K1 des identités sur matrices rationnelles. Ils ne qualifient ni l'export v9, ni un modèle entraîné, ni la réalisabilité HGP de chaque matrice abstraite. Le [contrat des coupes](CONTRAT_COUPES_ET_MASSES_20260926.md) et ses propres reçus restent l'autorité de la tranche précédente.
