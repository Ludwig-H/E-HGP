# Risques, réfutations et décisions go/no-go

## Verdict initial

| Proposition | Évaluation actuelle |
|---|---|
| Support normalisé seul + proportions sans décodeur point-wise atteint le SOTA | probabilité faible |
| Support + composante HGP complète incidence-aware est une hypothèse cohérente | oui, gain et coût non démontrés |
| HGP apporte un signal utile à un backbone local fort | plausible, non démontré |
| HSA ou QC-HSA est le meilleur opérateur sur HGP | ouvert, preuve 3D absente |
| Un modèle hybride HGP + local peut être compétitif | crédible mais à haut risque |
| SemanticKITTI seul suffit pour ICML/NeurIPS | improbable |
| Instance doit être travaillée maintenant | non, phase fermée |

Cette appréciation est volontairement sévère : le papier HSA n'a aucune expérience 3D dense, et le papier HGP sur SemanticKITTI utilise la sémantique de vérité terrain pour une tâche de regroupement. Aucun des deux ne constitue une preuve directe du modèle proposé.

## R1 — La hiérarchie de densité encode le capteur, pas la sémantique

### Mécanisme

La densité d'un LiDAR décroît avec la portée et dépend de l'angle, de l'occultation et de la surface. HGP peut séparer le même objet à longue distance et fusionner le sol proche, même si son modèle de densité est mathématiquement fondé.

Une observation plus lointaine n'est pas une homothétie du nuage métrique : l'échantillonnage angulaire s'amincit, des retours disparaissent et les occultations changent. L'invariance d'échelle du descripteur ne réfute donc pas ce risque.

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

Pour $K>1$, les composantes facettées peuvent induire des unions de points qui se chevauchent. HSA standard suppose une partition imbriquée.

La projection par « partition de l'unité » n'est pas encore une méthode définie. Le chemin de référence reste donc $K=1$ ou une laminarisation déterministe auditée tant qu'une application $w_{iv}$, son domaine et sa règle de construction ne sont pas spécifiés.

### Test de réfutation

- validation de laminarité ;
- comptage des multi-appartenances ;
- comparaison arbre natif disponible et projection laminaire auditée ; le modèle multi-arbre/DAG n'entre qu'après définition de $w_{iv}$ ;
- conservation des masses et absence de double comptage.

### No-go

Refuser tout résultat dont la projection dépend de l'ordre d'itération, n'est pas reproductible ou ne vérifie pas $\sum_v w_{iv}=1$ sur un domaine déclaré. Tant que ces poids ne sont pas définis, aucune propriété de conservation ou de stochasticité n'est revendiquée pour le DAG. Si une projection perd l'avantage HGP, le modèle HSA standard n'est pas adapté ; ouvrir une nouvelle voie DAG avec ses propres preuves ou revenir à $K=1$.

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
