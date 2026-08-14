# Risques, réfutations et décisions go/no-go

## Verdict initial

| Proposition | Évaluation actuelle |
|---|---|
| Support normalisé seul + proportions sans décodeur point-wise atteint le SOTA | probabilité faible |
| Support + composante HGP complète incidence-aware est une hypothèse cohérente | oui, gain et coût non démontrés |
| HGP apporte un signal utile à un backbone local fort | plausible, non démontré |
| HSA ou QC-HSA est le meilleur opérateur sur HGP | ouvert, preuve 3D absente |
| Alimenter HSA impose une laminarisation destructrice, donc la voie $K\geq2$ est bloquée | non, l'arbre est déjà laminaire sur les facettes ; ce qui reste est un coût à mesurer |
| Un modèle hybride HGP + local peut être compétitif | crédible mais à haut risque |
| SemanticKITTI seul suffit pour ICML/NeurIPS | improbable |
| Instance doit être travaillée maintenant | non, phase fermée |
| Le descripteur de nœud est le levier décisif | non, le plus faible des trois d'après les ablations publiées |
| « HGP bat les contrôles sur l'oracle de partition » suffit à justifier le programme | non, porte de réfutation seulement |

Cette appréciation est volontairement sévère : le papier HSA n'a aucune expérience 3D dense, et le papier HGP sur SemanticKITTI utilise la sémantique de vérité terrain pour une tâche de regroupement. Aucun des deux ne constitue une preuve directe du modèle proposé.

## R1 — La hiérarchie de densité encode le capteur, pas la sémantique

### Mécanisme

La densité d'un LiDAR décroît avec la portée et dépend de l'angle, de l'occultation et de la surface. HGP peut séparer le même objet à longue distance et fusionner le sol proche, même si son modèle de densité est mathématiquement fondé.

Une observation plus lointaine n'est pas une homothétie du nuage métrique : l'échantillonnage angulaire s'amincit, des retours disparaissent et les occultations changent. L'invariance d'échelle du descripteur ne réfute donc pas ce risque.

La correction range-aware n'est pas non plus acquise, et un signal contraire publié l'interdit explicitement : l'ablation d'ALPINE (Sautier et al., 3DV 2026) montre qu'un seuil proportionnel à la portée, à la manière de LESS, donne $75{,}9$ PQ contre $76{,}3$ pour le seuil constant par classe, malgré l'optimisation de son coefficient à l'échelle du jeu de données. Une correction de portée naïve dégrade donc leur clustering. Le transfert n'est pas immédiat — leur seuil est un rayon de liaison et non un niveau de densité $K$-NN — mais la correction doit être mesurée, et non supposée, avec ce résultat négatif cité.

### Test de réfutation

- comparer statistiques HGP et pureté par distance ;
- déplacer/rééchantillonner des patches ou objets à plusieurs portées ;
- thinning aléatoire et structuré ;
- comparer métrique brute, correction range-aware et hiérarchie géométrique témoin ;
- mesurer la stabilité des ancêtres, niveaux et prédictions.

### No-go

Si la variation intra-objet due à la portée est du même ordre que la séparation interclasse et qu'une correction simple ne la réduit pas, ne pas défendre HGP comme hiérarchie sémantique universelle. Pivoter vers une hiérarchie conditionnée par le capteur ou vers HGP comme régulariseur secondaire.

## R2 — Le canal complet est ambigu, coûteux ou mal sérialisé

La convexification du support ne réfute pas `support + objet HGP complet`. Le second canal peut conserver les incidences et la non-convexité que le premier perd. Le risque réel est de nommer « polyèdre » plusieurs objets incompatibles : le $K$-polyèdre discret $V_v$, le carrier des facettes $C_v^{F}$, le carrier des cofaces $C_v^{Q}$ ou l'union témoin $W_v$, composante exacte du niveau de multicoverture.

### Mécanisme

Le support de $C_v^{F}$ est exactement celui de ses sommets, mais le carrier complet conserve encore ses cellules et incidences. Cette redondance concerne les données source/PL qui conservent ces sommets, jamais le support de $W_v$, qui n'est en général pas celui des observations. Confondre ces identités invaliderait l'interprétation des ablations. Un graphe $\Gamma_K^{\mathrm{elem}}$ seul perd les incidences point--facette ; une liste de cellules sans cofaces perd les connexions marquées ; un certificat seulement $H_0$ ne reconstruit aucun de ces carriers.

La fonction radiale extérieure reste une compression distincte. Elle remplit les lacunes si la forme n'est pas étoilée et ne doit jamais remplacer silencieusement le canal complet.

### Test de réfutation

- fixer `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only` ;
- sérialiser et rejouer `cut_policy`, `cut_level`, `cut_side` et les `deltas` sans ambiguïté aux événements de même niveau ;
- mêmes points, support et marques, mais incidences différentes ;
- carrier des facettes contre carrier des cofaces contre $W_v$ ;
- objet complet seul contre support + objet complet ;
- points seuls, accès à $\Gamma_K^{\mathrm{elem}}$ avec tokens précalculés, sac des mêmes tokens sans messages et incidence complète ; ces contrôles restreignent l'accès calculatoire et ne prétendent pas effacer une information reconstruisible depuis les tokens ;
- deux exports sparse équivalents, puis un mutant `h0_only` qui doit être refusé ;
- appariement strict de RAM, VRAM, paramètres, prétraitement et latence ;
- pour `witness_union`, $N_W$ ventilé par requêtes, patches et échantillons, $\varepsilon_W$, temps et mémoire ;
- rayon extérieur seulement comme ablation lossy.

### No-go

Refuser le résultat si le payload ne permet pas de reconstruire le carrier annoncé, si son contenu dépend de l'ordre des records, si sa coupe change au round-trip ou si une équivalence seulement $H_0$ est présentée comme géométrique. `witness_approx` sans $\varepsilon_W$ est également refusé. Retirer le raccourci de support source si `complexe seul` l'égale ; ne jamais conclure de cette redondance à une identité avec le support de `witness_union`. Retirer la branche complète si points/Deep Sets de même budget l'égalent avec un meilleur coût. Une branche valide mais dominée reste un résultat négatif, pas une réfutation mathématique de l'idée.

## R3 — La normalisation supprime une information sémantique utile

### Mécanisme

La taille, la hauteur et la portée aident à distinguer voiture/camion, personne, poteau/tronc ou végétation/terrain. Un rayon maximal est en outre contaminé par un outlier.

### Test de réfutation

- probe classe à partir de `log(R)`, dimensions et position seules ;
- support brut contre normalisé, puis normalisé + side channels ;
- centre moyen contre médiane géométrique ;
- rayon max, RMS, q95 et q99 ;
- analyse par classe thing et par portée.

### No-go

Si réinjecter l'échelle produit un gain clair, le claim « la normalisation tue la question d'échelle » est abandonné. Le bon claim devient une factorisation forme normalisée / échelle métrique.

## R4 — La normalisation casse la cohérence parent–enfant

### Mécanisme

Le support d'une union est le max des supports seulement dans un repère commun. Deux enfants renormalisés indépendamment ne décrivent ni leur écart ni leur échelle relative. Pour le canal complet, fusionner des listes sans dédupliquer les identifiants peut dupliquer points et cellules ; fusionner seulement les embeddings peut perdre les cofaces nouvelles qui réalisent la fusion.

### Test de réfutation

- reconstruire le support parent direct à partir des enfants ;
- comparer sans géométrie relative, avec déplacement relatif, puis déplacement + ratio d'échelle ;
- rejouer l'union canonique des tables de points, facettes, cofaces et incidences dans plusieurs ordres ;
- ajouter séparément les cellules du lot de fusion et vérifier les carriers reconstruits ;
- mesurer erreur de reconstruction et mIoU ;
- conserver une fixture de deux enfants identiques déplacés différemment.

### No-go

Si les transformations relatives sont nécessaires, elles deviennent contractuelles. Une architecture de seuls vecteurs normalisés indépendants est éliminée. Aucun résumé de taille fixe n'est déclaré lossless sans théorème de composition sur la classe considérée.

## R5 — Les proportions ne localisent pas les classes

### Mécanisme

Un cluster peut et doit représenter exactement son mélange par un vecteur de proportions. Toutefois, ce vecteur est invariant à toute permutation des labels entre les points du cluster : il conserve les masses mais pas leur localisation. Une condensation sans features ni décodeur point-wise peut donc perdre définitivement les frontières et les petites structures.

### Test de réfutation

- erreur des proportions prédites, entropie des cibles et cohérence massique parent–enfants ;
- capacité d'un décodeur point-wise à relocaliser les classes depuis une même proportion de cluster ;
- baseline majoritaire uniquement comme contrôle artificiel d'une sortie dure cluster-constante ;
- borne supérieure relaxée par classe par union de tokens, uniquement comme diagnostic de localisation ;
- détail par classes rares, portée et frontières ;
- comparaison points feuilles, micro-voxels et clusters terminaux ;
- décodeur point-fin avec et sans skip.

### No-go

Si les proportions sont bien estimées mais que le décodeur ne relocalise pas les classes, garder les points comme feuilles ou renforcer le chemin local. Le vote majoritaire ne constitue ni le modèle proposé ni un plafond mIoU. Ce no-go n'élimine pas la hiérarchie ou ses distributions comme contexte.

## R6 — HGP n'est pas meilleur qu'un arbre simple

### Mécanisme

Un octree, une hiérarchie de voxels, RSL ou des superpoints peuvent suffire. Le backbone peut exploiter seulement la connectivité globale, pas les niveaux de l'estimateur $K$-NN/HGP.

### Test de réfutation

Même backbone, descripteur, opérateur, nombre de nœuds, budget et seeds ; échanger seulement l'arbre. Ajouter arbre aléatoire contrôlé et permutation comme null tests.

### No-go

Suspendre le claim HGP si le gain contre le meilleur contrôle est inférieur à environ +0,5 mIoU et si l'intervalle apparié à 95 % recouvre zéro. Continuer seulement si HGP gagne sur un autre axe pré-déclaré, par exemple robustesse ou coût.

## R7 — HSA et QC-HSA n'apportent rien au-delà du pooling

### Mécanisme

La hiérarchie peut être utile tandis que l'attention à blocs est trop contrainte ou trop difficile à optimiser. Les théorèmes KL portent sur l'approximation d'une attention donnée, pas sur la justesse des labels. `QC-HSA` peut aussi payer un coût $\mathcal{O}(N\log N)$ sans gain utile.

### Test de réfutation

Sur le même arbre et les mêmes features, comparer pooling+MLP, message passing, HSA, `QC-HSA`, Sequoia et attention locale supplémentaire. Appareiller paramètres, profondeur et seeds. Sur petits scans, mesurer aussi le reverse-KL à une même attention plate $P$ gelée, avec mêmes scores et masque, puis vérifier la solution contre une optimisation dense. Les KL de modèles entraînés séparément ne sont pas causalement comparables.

### No-go

Si aucun opérateur hiérarchique n'améliore précision, robustesse ou Pareto système, retirer l'attention du claim principal. Si `QC-HSA` domine seulement en KL mais pas en segmentation, sa proposition reste une propriété d'approximation en appendice. La relaxation peut aussi généraliser moins bien en supprimant les égalités entre lignes qui agissent comme régularisation. Ne pas introduire des tokens internes tout en continuant à invoquer les résultats HSA/QC-HSA.

## R8 — La contrainte de blocs propage les erreurs d'arbre

### Mécanisme

Tous les couples de feuilles entre deux branches partagent une interaction structurée. Une mauvaise séparation ou le chaining peut donc contaminer de nombreux points.

### Test de réfutation

- performance selon pureté de l'ancêtre ;
- arbres volontairement perturbés ;
- gate résiduel, têtes locales et arêtes de frontière ;
- cartes d'attention et influence des branches ;
- calibration de l'incertitude aux frontières.

### No-go

Si les voies de correction doivent devenir aussi coûteuses qu'une attention locale/plate complète, ne plus revendiquer un bénéfice structurel ou d'efficacité HSA.

## R9 — La structure n'est pas un arbre propre

### Mécanisme

Ce risque ne disparaît pas, mais il change de nature, et la formulation antérieure était fausse. Pour $K>1$, les composantes facettées induisent bien des unions de points qui se chevauchent, et HSA suppose bien une partition imbriquée ; il n'en résulte pourtant aucun blocage. Le manuscrit de thèse, Partie II, § 9.1, « Et lorsqu'on impose une partition stricte des données ? », page 96, énonce que « pour $K \geq 2$, l'objet naturel n'est pas une partition de $X$, mais un recouvrement de $X$ (ou bien une partition des $(K-1)$-simplexes) ». L'arbre de fusion est donc déjà laminaire, non sur les points mais sur $F_{K}$, l'ensemble des $(K-1)$-simplexes effectivement construits — les sommets du graphe dual, c'est-à-dire les simplexes de Gabriel dans la version standard — dont il forme une partition à chaque niveau. Le recouvrement n'apparaît que dans la projection vers les points, un point appartenant à plusieurs facettes.

La conséquence architecturale est directe : si les feuilles sont les facettes et non les points, l'hypothèse de HSA est satisfaite sans aucun bricolage, puisque son lemme de sous-structure optimale porte sur une partition des feuilles, ce qui est exactement le cas ici. Rien n'est détruit au niveau de l'arbre.

La partition de l'unité que ce document réclamait sans l'avoir existe déjà, et elle est celle du manuscrit. À chaque facette $\tau$ est associé un score local positif $S_{\tau} = \sum_{\sigma \supset \tau, |\sigma| = K+1} \psi(\rho(\sigma))$, où $\rho(\sigma)$ est le rayon de naissance du $K$-simplexe $\sigma$ dans la filtration et $\psi(t) = 1/t^{p}$ ; $\psi$ peut être toute fonction de poids décroissante, le choix uniforme $\psi = 1$ restant possible, mais $1/t^{p}$ « reflète plus exactement la densité locale ». Chaque point normalise par $T_{x} = \sum_{\tau \ni x} S_{\tau}$, avec la convention $1/T_{x} = 0$ lorsque $T_{x} = 0$. En posant $w_{x\tau} = S_{\tau}/T_{x}$, on obtient $w_{x\tau} \geq 0$ et $\sum_{\tau \ni x} w_{x\tau} = 1$ : « lorsqu'un point appartient à au moins une face, il distribue une masse totale égale à 1 entre les faces qui le contiennent ». C'est l'instanciation explicite de l'application $w_{iv}$ dont l'absence était présentée ici comme un obstacle.

La conservation de la masse en découle en une ligne. Pour toute antichaîne, en posant $w_{x \to v} = \sum_{\tau \in v} w_{x\tau}$, on a $\sum_{v} w_{x \to v} = 1$. Il n'y a donc pas de double comptage, et le canal de masse additif — la CDF projetée — redevient exact dès que chaque point est pondéré par $w_{x \to v}$ : le comptage brut $n_{v}$ doit être remplacé partout par cette masse pondérée. La masse d'un nœud suit la même définition, $m_{\tau} = S_{\tau} \sum_{x \in \tau} 1/T_{x}$, et le manuscrit précise que « c'est ce poids $m_{\tau}$, et non le simple comptage des faces, qui est utilisé par le seuil `min_cluster_size` dans l'arbre condensé ».

La conversion en partition stricte est la Proposition 7 du manuscrit : $V_{x}(c) = \sum_{\tau \ni x, \ell(\tau) = c} S_{\tau}/T_{x}$, puis $\mathrm{label}(x) \in \arg\max_{c} V_{x}(c)$. Elle garantit une partition disjointe, avec une classe $-1$ pour les points non classés, sous une règle déterministe de départage des égalités. Pour $K = 1$, les faces sont les points eux-mêmes, le vote est trivial et restitue exactement le single-linkage. Pour un réseau, il suffit de remplacer l'argmax par la combinaison convexe $p(x) = \sum_{\tau \ni x} w_{x\tau} p_{\tau}$, où $p_{\tau}$ est la distribution prédite sur la facette : c'est la Proposition 7 avant durcissement, donc différentiable, et chaque point conserve une prédiction propre puisque les poids dépendent de lui. L'argmax redevient la version d'inférence si une partition stricte est exigée.

Le programme n'est donc plus bloqué : ce qui était écrit ici comme une condition d'existence est devenu un coût à mesurer, sur trois axes. Premièrement, le coût du passage aux facettes comme feuilles : les facettes sont plus nombreuses que les points, donc prendre les facettes comme feuilles augmente la taille de l'arbre, et profondeur, degré et nombre de feuilles sont à mesurer avant d'en faire la baseline. Deuxièmement, la perte au durcissement : le passage à la partition stricte perd de l'information, mais cette perte est mesurable par la marge $V_{x}^{(1)} - V_{x}^{(2)}$ entre les deux premiers clusters, et la fraction de points à vote contesté est le coût exact de la laminarisation. Troisièmement, à $K = 1$, HGP est le single-linkage : la configuration la plus simple n'apporte aucune nouveauté structurelle, et ne peut donc pas porter seule le claim.

T6 — l'attention directement sur le DAG de recouvrement, sans passer par les facettes comme feuilles — n'est plus une condition d'existence mais une extension. Il reste le seul endroit où un budget de nouveauté d'opérateur serait bien placé, mais le programme n'est plus bloqué sans lui.

### Test de réfutation

- mesurer la fraction de points à vote contesté et la distribution complète de la marge $V_{x}^{(1)} - V_{x}^{(2)}$, et rapporter cette fraction comme diagnostic du coût de la laminarisation ;
- comparer nombre de feuilles, profondeur et degré avec les facettes comme feuilles contre les points comme feuilles, sur les mêmes scans, avant de fixer la baseline ;
- vérifier numériquement $\sum_{\tau \ni x} w_{x\tau} = 1$, puis $\sum_{v} w_{x \to v} = 1$ sur toute antichaîne, y compris sur les points où $T_{x} = 0$ ;
- vérifier que la masse pondérée remplace bien le comptage brut $n_{v}$ partout, et que $m_{\tau}$ pilote effectivement `min_cluster_size` dans l'arbre condensé ;
- validation de laminarité sur $F_{K}$ et comptage des multi-appartenances point--facette ;
- comparer, à budget égal, le vote pondéré durci et la combinaison convexe différentiable ;
- contrôle $K = 1$ obligatoire, où la voie doit restituer exactement le single-linkage ;
- comparer arbre natif sur facettes et projection laminaire auditée ; le modèle multi-arbre/DAG reste une extension.

### No-go

Si la fraction de points à vote contesté est élevée et que le passage aux facettes comme feuilles fait exploser la taille de l'arbre, la voie $K \geq 2$ n'est pas exploitable avec cet opérateur : revenir à $K = 1$ en assumant qu'il n'y a alors aucune nouveauté structurelle par rapport au single-linkage, ou ouvrir T6 comme extension avec ses propres preuves. Refuser tout résultat dont la projection dépend de l'ordre d'itération, dont la règle de départage des égalités n'est ni déterministe ni déclarée, ou qui ne vérifie pas $\sum_{\tau \ni x} w_{x\tau} = 1$ sur un domaine déclaré. Ne présenter aucun gain $K \geq 2$ sans rapporter en regard la fraction de points contestés et la taille de l'arbre.

## R10 — Le coût théorique ne devient pas un gain GPU

### Mécanisme

Les traversées par profondeur, kernels sparse et batches irréguliers peuvent être plus lents que PTv3 ou FlashAttention. Une chaîne HGP augmente le nombre de lancements ; un nœud multifurqué augmente le coût quadratique local.

### Test de réfutation

- profiler construction, sérialisation, transfert, kernels et reprojection ;
- P50/P95 sur scans réels ;
- distribution des degrés/profondeurs ;
- comparaison avec PTv3, octree et pooling ;
- scaling en nombre de points et de nœuds ;
- latence réseau seule et end-to-end.

### No-go

Ne pas revendiquer « linéaire » ou « GPU-friendly » si le débit et la mémoire ne le montrent pas. Un gain accuracy substantiel peut rester publiable, mais le claim d'efficacité est retiré.

## R11 — Surapprentissage à la séquence 08

### Mécanisme

Une seule séquence sert de validation standard et ses frames sont temporellement corrélées. Une large recherche d'hyperparamètres peut produire un gain non généralisable.

### Test de réfutation

- hypothèses et matrice pré-enregistrées ;
- seeds appariées ;
- bootstrap de blocs temporels avec agrégation des matrices de confusion avant recalcul du mIoU ;
- validation auxiliaire par séquence pendant les phases exploratoires ;
- second dataset avant le claim général ;
- test caché utilisé après gel seulement.

### No-go

Un gain sur 08 qui disparaît entre seeds, blocs temporels ou second dataset n'est pas une contribution générale.

## R12 — La nouveauté est insuffisante pour ICML/NeurIPS

### Mécanisme

SPT, SP2T, EZ-SP, SPCNet, Sequoia, OctFormer et SSTNet occupent déjà l'espace hiérarchie + attention/proxies/superpoints. Les réseaux simpliciaux, cellulaires, Hodge et incidence-aware occupent déjà l'encodage de complexes. LitePT formalise le motif convolutions précoces puis attention tardive. Fast Multipole Attention, H-Transformer, MRA et HKT occupent l'attention hiérarchique/multi-résolution. Fonction support, ECT/WECT et projections KL sont classiques.

### Test de réfutation

Avant rédaction, auditer l'encodeur du carrier contre les réseaux simpliciaux/cellulaires et `QC-HSA` contre les attentions multi-échelles. Identifier une proposition générale testable parmi T3--T6 : stabilité filtrée et range-aware, composition sparse certifiée, raffinement piloté par le carrier ou opérateur sur recouvrements. Tester sur au moins deux datasets/capteurs.

### No-go

Si le seul résultat est `réseau simplicial existant + HGP + HSA` avec un petit gain SemanticKITTI, viser une venue 3D/appliquée ou publier une étude négative solide, pas sur-vendre une contribution ML générale.

## R13 — Le payload complet n'existe pas encore dans la voie v3

### Mécanisme

Le cadre v3 courant reste `public_status=not_claimed`, son audit live est antérieur au `HEAD`, et sa voie produit ne persiste pas un payload composante-local complet de facettes, cofaces et incidences. Une forêt réduite et une union de points ne suffisent pas à reconstruire le carrier complet. L'oracle Gamma exhaustif borné ne peut pas devenir l'architecture d'entraînement.

### Test de réfutation

- spécifier les trois axes exacts : `payload_kind=marked_incidence`, `carrier_kind` parmi `source_points`, `facet_pl`, `coface_pl`, `witness_union`, et `authority` parmi `incidence_complete`, `pl_complete`, `witness_exact`, `witness_approx`, `h0_only` ;
- pour `witness_approx`, borner $\varepsilon_W$ et compter $N_W$ par requêtes, patches et échantillons ;
- produire un export composante-local avec identifiants partagés, marques et hash canonique ;
- comparer deux producteurs ou deux présentations sparse certifiées du même objet ;
- mesurer taille, temps de construction et réutilisation des cellules entre ancêtres ;
- vérifier qu'aucune arène n'est dimensionnée par le complexe de Čech ambiant.

### No-go

Ne pas entraîner ni publier la branche complète si son entrée provient d'une reconstruction heuristique non déclarée depuis la forêt. Si aucun exporteur sparse certifié n'est viable, limiter l'étude à un oracle borné ou revenir à un canal de points explicitement approximatif.

## R14 — Les structures filiformes sont sous-segmentées par la connexité d'ordre supérieur

### Mécanisme

C'est le risque le plus spécifique du dossier, et il faut le dire : il oppose l'avantage revendiqué de HGP au profil exact des classes qui décident de la métrique visée. Il est écrit dans le manuscrit lui-même, § 9.3, sur le jeu `birch2` : HDBSCAN à $k=100$ obtient un ARI de $0{,}996$ et classe $99{,}7\,\%$ des points, tandis que HGP-Clusterer à $k=84$ obtient un ARI de $0{,}441$ et classe $83{,}9\,\%$ des points. La cause avancée par le manuscrit est explicite : « les clusters sont essentiellement filiformes et sont donc mieux identifiés avec de simples graphes ».

Le mécanisme est structurel et non anecdotique. La connexité d'ordre $K$ exige que $K$ points soient simultanément proches. Le long d'une structure filiforme ou d'une surface mince échantillonnée de façon éparse, cette condition n'est satisfaite qu'à un rayon nettement plus grand que celui qui suffirait à une connexité par arêtes. L'objet fin naît donc tard dans la filtration, et à ce niveau tardif ses voisines l'ont déjà rejoint. Le résultat observable est une sous-segmentation des objets fins, et non une fragmentation : HGP achète sa résistance au chaînage en retardant la naissance des objets minces.

Or la marge de progression du mIoU SemanticKITTI porte exactement sur ces classes-là — `pole`, `traffic-sign`, `bicycle`, `person`, `bicyclist`, `motorcyclist`, `fence` — les classes volumiques plafonnant déjà très haut. Le manuscrit suggère une atténuation, observée sur `birch2` : changer d'estimateur, $\hat{\rho}=1/r^{2}$. Une atténuation testable ne dispense pas de mesurer d'abord l'ampleur du problème. Le développement complet de ce point figure dans [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md).

### Test de réfutation

- mIoU-oracle stratifié par dimension intrinsèque estimée du nœud, à partir des valeurs propres de la covariance de ses points, en séparant régimes linéaire, planaire et volumique ;
- même oracle stratifié par classe fine contre classe volumique, avec les classes rares rapportées séparément ;
- comparer la portée des niveaux de naissance des objets fins contre HDBSCAN au même $K$, sur les mêmes points et à taux de compression apparié ;
- mesurer, pour chaque objet fin de la vérité terrain, le niveau auquel il naît et le niveau auquel il est absorbé par un voisin ;
- rejouer l'ensemble avec l'estimateur $\hat{\rho}=1/r^{2}$ et rapporter séparément l'effet de ce changement d'estimateur ;
- croiser avec la portée, la sous-segmentation attendue et l'amincissement angulaire étant corrélés.

### No-go

Si la sous-segmentation des classes fines est du même ordre que le gain obtenu sur les classes volumiques, HGP ne peut pas être défendu comme hiérarchie pour une métrique moyennée par classe, et le claim doit être retiré ou restreint à un régime déclaré. Si l'atténuation par changement d'estimateur ne réduit pas l'écart sans détruire le gain sur le reste, il faut soit publier ce constat comme résultat négatif, soit changer de métrique cible en l'annonçant, soit conditionner la filtration au capteur. Ne présenter aucun gain agrégé sans la ventilation fin/volumique qui montre d'où il vient.

## R15 — Le goulot n'est pas la partition

### Mécanisme

L'hypothèse implicite du programme est « meilleure partition, donc meilleure segmentation ». La littérature superpoint publie déjà l'oracle qui la teste, et son verdict lui est défavorable. SPG (CVPR 2018), table 5, S3DIS 6-fold : l'oracle « Perfect » atteint $88{,}2$ mIoU et $92{,}7$ mAcc pour un modèle à $62{,}1$. SPT (ICCV 2023) écrit que « the performance of SPT is more than 20 points below the oracle, suggesting that the partition does not strongly limit its performance », soit un oracle supérieur ou égal à $89$ pour un modèle à $68{,}9$ sur Area 5. SuperCluster (3DV 2024) constate que « the high performance of this oracle ($93{,}4$ PQ) indicates that very little precision is lost by working with superpoints », son second oracle de clustering restant à $83{,}6$ PQ.

Ces méthodes laissent donc déjà environ vingt points d'oracle non convertis. Améliorer le plafond d'une partition qui n'est pas saturée ne peut pas produire de gain, et un diagnostic d'oracle favorable à HGP est une porte de réfutation, jamais une porte de promotion : le perdre tue le programme, le gagner ne prouve presque rien.

Les ablations publiées sur cette famille exacte de modèles confirment la même hiérarchie des priorités. SPT : retirer toutes les features handcrafted de nœud coûte $-0{,}7$ mIoU sur S3DIS 6-fold, $-4{,}1$ sur KITTI-360 et $-1{,}4$ sur DALES ; retirer l'encodage d'adjacence coûte $-6{,}3$, $-5{,}4$ et $-3{,}0$ ; passer à un seul niveau de partition coûte $-8{,}4$, $-5{,}1$ et $-0{,}9$. EZ-SP (ICRA 2026) va plus loin : remplacer les features handcrafted par un petit réseau appris change le résultat de $\pm0{,}1$ mIoU. Le descripteur de nœud est le levier le plus faible des trois ; l'adjacence et le nombre de niveaux dominent. Ce constat borne directement l'enjeu du débat de [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md), et l'ordre de mesure qui en découle est celui de [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md).

### Test de réfutation

- mesurer l'écart entre le modèle entraîné et l'oracle de sa propre partition, à budget de régions apparié, sur le même dataset et les mêmes seeds ;
- rapporter en parallèle l'oracle de coupes à niveau fixé, convention de la littérature superpoint, pour permettre la comparaison directe avec les chiffres publiés ;
- reproduire les trois ablations de SPT sur la partition HGP — descripteur de nœud, encodage d'adjacence, nombre de niveaux — et vérifier si leur ordre de grandeur relatif se retrouve ;
- comparer descripteur handcrafted et descripteur appris de même budget, comme contrôle du constat EZ-SP ;
- rapporter la courbe oracle contre nombre de régions pour HGP et pour chaque contrôle, plutôt qu'un point unique.

### No-go

Si l'écart entre le modèle et l'oracle de sa propre partition dépasse dix points, la valeur de HGP ne peut plus être revendiquée sur la qualité des régions : améliorer un plafond déjà non atteint ne peut pas payer. Elle doit alors venir d'ailleurs — niveaux de densité interprétables, théorie de récupérabilité, recouvrement pour $K\geq2$ — et le papier doit le dire ainsi plutôt que « notre arbre est meilleur ». Si l'écart au propre oracle reste au contraire faible, c'est cette mesure, et non le gain de plafond, qui devient l'argument à défendre.

## Tableau de décision global

| Porte | Go | Revise | Stop/Pivot |
|---|---|---|---|
| G0 contrat | forêt déterministe et carrier marqué reconstructible, sans fuite | projection laminaire et complétude relative documentées | payload `h0_only` présenté comme complexe complet |
| G1 structure | HGP bat les contrôles ou apporte robustesse claire | points feuilles, correction range-aware | aucune valeur contre arbres simples |
| G2 descripteur | objet complet incidence-aware informatif à budget égal | support shortcut retiré ou carrier changé | objet complet dominé, ambigu ou non reconstructible |
| G3 opérateur | HSA ou QC-HSA gagne en qualité ou Pareto | agrégateur simple | opérateurs hiérarchiques dominés partout |
| G4 validation | gain apparié, multi-seeds, classes/distance expliquées | retravailler frontières/recette | effet non reproductible |
| G5 système | coût complet soutenable et honnête | claim précision seulement | ni précision ni coût compétitif |
| G6 généralité | second dataset confirme le mécanisme | contribution SemanticKITTI bornée | surapprentissage dataset |

## Pivots scientifiquement valables

- **HGP utile, HSA inutile** : publier une étude de priors hiérarchiques avec agrégateur simple.
- **Objet marqué utile, support source redondant** : retirer le shortcut si le payload source/PL conserve déjà les sommets ; ne pas transférer ce constat au support propre de `witness_union`.
- **Support utile, complexe dominé** : garder le résumé convexe et documenter le résultat négatif incidence-aware.
- **Union témoin trop coûteuse** : tester le carrier PL déclaré, sans lui transférer l'identité avec $L_K(a)$.
- **HGP brut sensible à la portée** : développer une hiérarchie ou métrique range-aware et en analyser la stabilité.
- **Arbre trop contraignant** : passer à un mélange de plusieurs arbres ou un DAG, en assumant une nouvelle théorie.
- **Accuracy neutre, robustesse positive** : recentrer sur thinning, longue distance et changement de capteur.
- **Aucun effet sémantique** : arrêter avant la phase instance ; ne pas chercher à sauver le projet avec un post-traitement panoptique.
