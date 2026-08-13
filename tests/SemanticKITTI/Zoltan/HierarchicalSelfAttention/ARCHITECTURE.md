# Architecture proposée

## Principe

L'architecture de départ est hybride : une représentation locale forte protège les détails de surface et les frontières, tandis que la hiérarchie HGP apporte du contexte à plus grande échelle. Cette décision évite de demander à la fonction support de reconstruire toute l'information du nuage et suit le signal négatif du papier HSA : remplacer indiscriminément toutes les couches basses d'un modèle plat peut dégrader fortement les résultats.

Nom de travail : **HGP-Hybrid Transformer**. Ce nom n'est pas un claim de nouveauté définitif.

## Contrat d'entrée de la hiérarchie

On suppose que chaque scan fournit :

- les points dans l'ordre original, avec `xyz` et rémission ;
- une forêt enracinée laminaire ;
- `parent`, `first_child`/`children`, profondeur et ordre topologique ;
- le mapping point–feuille et, si possible, des intervalles contigus de feuilles par nœud ;
- cardinalité, centre, rayon et statistiques HGP par nœud ;
- niveau de naissance, niveau de mort et persistance/stabilité lorsqu'ils sont définis ;
- l'ordre HGP $K$, la distance géométrique $d_{\mathrm{geo}}$, les paramètres de reconstruction et la version du reconstructeur ;
- un identifiant déterministe du scan et de la hiérarchie.

Les labels SemanticKITTI ne participent jamais à la construction. Une hiérarchie par classe prédite serait une expérience aval différente et n'est pas admise dans l'encodeur sémantique principal.

HSA a besoin d'un arbre. Si les objets HGP d'ordre supérieur ont des appartenances ponctuelles chevauchantes, le producteur doit fournir soit une projection laminaire auditée, soit une structure DAG explicitement prise en charge. Dupliquer silencieusement les points ou arbitrer l'appartenance par ordre d'itération rendrait le résultat non interprétable.

## Représentation des feuilles

Deux granularités seront comparées :

1. **points comme feuilles**, solution de référence scientifique, sans plafond de pureté dû à la tokenisation ;
2. **micro-voxels ou micro-clusters comme feuilles**, seulement si les courbes majoritaire et optimiste mIoU–compression valident cette réduction.

Le premier prototype doit partir d'une recette SemanticKITTI réellement reproductible et épinglable, par exemple MinkUNet/Cylinder3D ou une recette publique SphereFormer auditée. PTv3 reste le porteur Transformer ambitieux, mais son dépôt officiel ne fournit pas à ce jour une recette SemanticKITTI complète avec config, poids et résultat reproductible ; il ne doit donc pas bloquer WP0. Un second backbone fort devra vérifier que l'effet HGP n'est pas propre au porteur choisi. Le backbone reçoit exactement les mêmes entrées et la même recette dans toutes les comparaisons appariées.

Pour chaque feuille $i$, le backbone produit $f_i^{0}\in\mathbb{R}^{d}$. Les projections Q/K/V et les normalisations suivent la définition de la baseline HSA testée. Les coordonnées absolues ou cylindriques ne sont pas supprimées : le repère ego et la gravité sont sémantiquement utiles.

## Descripteur des nœuds

### Canal de support

Pour un nœud non dégénéré $v$, $s_v(u)=\max_{x\in C_v}\left\langle u,\frac{x-c_v}{R_v}\right\rangle$ est échantillonné sur une grille déterministe de la sphère. Comparer séparément des grilles Fibonacci de cardinalité exacte 20/42/80/162 et des grilles de sommets d'icosphères de cardinalité réelle 12/42/162/642. Chaque direction est normalisée ; si des largeurs sont utilisées, chaque paire $u,-u$ est explicitement présente. La construction, la cardinalité après ajout des antipodes et le rayon de couverture mesuré sont enregistrés. La sélection finale dépend d'une courbe erreur–mémoire–latence.

Le support maximal est stable vis-à-vis de petites perturbations de l'enveloppe convexe au sens de Hausdorff, mais statistiquement fragile à un point aberrant. Il ne reçoit des gradients que par les points extrêmes des directions. Il reste donc un canal parmi plusieurs.

### Canal robuste directionnel

Pour chaque direction, calculer une petite pile de quantiles des projections normalisées $\left\langle u,\frac{x-c_v}{R_v}\right\rangle$, par exemple `q50`, `q90`, `q95`, `q99` et `max`. Les quantiles ne sont pas des fonctions support convexes au sens strict ; ils sont utilisés comme features de distribution. Une variante log-mean-exp multi-température doit être testée pour distribuer les gradients, sans la présenter comme robuste aux outliers.

### Convention dégénérée

Si $R_v=0$, fixer supports et quantiles normalisés à zéro et activer un masque `degenerate`. Le canal de taille utilise $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$ accompagné du même masque. Le plancher métrique est fixé dans la configuration ; il sert à la stabilité numérique et rompt l'invariance d'échelle exacte du cas dégénéré. Pour un parent de rayon nul, le déplacement et le ratio d'échelle sont fixés à zéro et masqués.

### Canaux non normalisés

Le modèle reçoit séparément :

- $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$, le masque dégénéré et les dimensions métriques du nœud ;
- `log(1 + n_v)`, masse et estimation de densité ;
- centre en coordonnées cartésiennes et `(range, azimuth, elevation)` ;
- hauteur minimale, maximale et moyenne ;
- valeurs propres normalisées de covariance et dimension intrinsèque estimée ;
- moyenne, dispersion et quantiles de rémission ;
- naissance, mort, persistance/stabilité et ordre HGP $K$ ;
- profondeur, fraction de masse dans le parent et rapports de masse entre enfants ;
- drapeaux singleton, rayon nul, faible dimension et troncature/condensation.

Le dataset standard ne fournit pas explicitement l'identifiant d'anneau laser. Aucun canal `ring` ne doit être inventé ; un indice reconstruit depuis l'élévation doit être déclaré comme feature dérivée.

### Géométrie relative parent–enfant

Une normalisation indépendante rend deux formes semblables identiques même si leur position dans le parent diffère. Pour une arête $p\rightarrow v$, l'embedding doit au minimum inclure $\Delta c_{pv}=\frac{c_v-c_p}{R_p}$ et $r_{pv}=\frac{R_v}{R_p}$, avec la convention masquée ci-dessus pour $R_p=0$. Sur une grille de directions globales commune, l'identité exacte à tester est $s_p(u)=\max_{v\in\mathrm{children}(p)}\left[\left\langle u,\frac{c_v-c_p}{R_p}\right\rangle+\frac{R_v}{R_p}s_v(u)\right]$. Si un repère local tourné est utilisé, sa rotation et le rééchantillonnage des directions deviennent aussi contractuels.

S'ajoutent la différence de niveau HGP, le ratio de cardinalité et l'orientation principale relative. Un MLP partagé par domaine du parent produit alors **un embedding $\epsilon_p(v)$ par enfant**. Dans la variante fidèle, l'énergie positionnelle entre frères $v,w$ utilise le produit scalaire $\epsilon_p(v)^{\top}\epsilon_p(w)$ attendu par HSA. Un MLP pairwise arbitraire ou l'injection d'un état appris aux nœuds internes définit une variante hors théorème. Un attribut de recouvrement n'est permis que dans une future variante DAG/multi-appartenance, pas dans la forêt laminaire HSA.

## Bloc hiérarchique de référence

### Passe bottom-up

Chaque nœud agrège les statistiques suffisantes Q/K/V de ses feuilles descendantes. Son descripteur géométrique produit les embeddings par enfant décrits ci-dessus ; il n'est pas transformé silencieusement en token interne. Les réductions sont groupées par profondeur et concaténées entre scans d'un batch.

La concaténation entre scans est **block-diagonal et masquée**. Un éventuel dummy root ne sert qu'à l'ordonnancement des kernels et sa famille est exclue du calcul d'attention : aucun scan ne peut influencer un autre. Une forêt à l'intérieur d'un scan reçoit un root synthétique propre au scan ; l'interaction entre ses composantes racines est un choix de modèle explicite et ablaté. Test obligatoire : la sortie d'un scan reste bit-identique, ou égale dans la tolérance numérique déclarée, quels que soient les autres scans du batch.

### Interaction entre enfants

Pour chaque famille, HSA calcule une interaction entre sous-arbres frères avec des coefficients partagés par blocs. Le coût structurel de l'algorithme et de l'énergie HSA spécifiques est mieux décrit par $\mathcal{O}\left(\sum_{v} d_v^{2}\right)$, où $d_v$ est le nombre d'enfants, que par le seul slogan linéaire. La borne $\mathcal{O}(M b^{2})$ n'est favorable que si le degré maximal $b$ reste borné et si le nombre de familles $M$ est linéaire. Ces expressions omettent dimension des têtes, projections, descripteurs, transferts et batching ; elles ne s'étendent pas automatiquement aux scores pairwise libres.

Les nœuds de degré élevé, les chaînes profondes et les arbres déséquilibrés sont mesurés. La binarisation ou le rééquilibrage ne sont pas gratuits : ils introduisent une structure artificielle et doivent être ablatés contre l'arbre natif.

### Opérateur expérimental : QC-HSA

HSA partage un coefficient sur tout rectangle entre deux branches sœurs, donc aussi entre plusieurs feuilles requêtes de la même branche. `QC-HSA` conserve au contraire chaque feuille requête. Pour une feuille $i$, elle agrège les clés/valeurs des sous-arbres frères rencontrés sur le chemin vers la racine et calcule un poids propre à $i$ pour chacun de ces groupes.

Les clés et valeurs moyennes des nœuds sont construites bottom-up. Pour un score bilinéaire et un biais HGP constant dans le groupe cible, chaque sortie s'obtient sans matrice dense par un Softmax sur score moyen + log-cardinalité. La [proposition candidate](THEOREM_PROGRAM.md) en fait la projection reverse-KL optimale sur ces contraintes conditionnées par la feuille, montre que sa famille contient celle de HSA et donne l'exactitude des scores constants sur chaque couple de branches de fusion.

Son coût structurel est $C_T=\sum_i|\Pi_T(i)|$, soit $\mathcal{O}(N\log N)$ sur un arbre équilibré de degré borné mais $\mathcal{O}(N^2)$ au pire. Le prototype doit donc rapporter simultanément $C_T$, $\sum_v d_v^2$, la profondeur, le degré et la latence. `QC-HSA` n'est retenue que si sa fidélité point-wise compense ce surcoût face à HSA.

### Passe top-down

Le contexte agrégé redescend vers chaque feuille. Pour la segmentation, la sortie hiérarchique $g_i$ est fusionnée avec la feature locale par un gate résiduel : $z_i=f_i^{0}+\alpha_i\,W_g g_i$, avec $\alpha_i\in[0,1]$ prédit à partir des features locales et hiérarchiques.

Le gate sert de voie de secours lorsque l'arbre traverse une frontière sémantique. Une variante concaténation + MLP est un contrôle nécessaire ; le gate ne doit pas devenir une source de gain non isolée.

## Placement des blocs

La configuration initiale contient :

- un encodeur local bas niveau ;
- un premier bloc HGP aux features intermédiaires ;
- éventuellement un second bloc HGP à plus grande portée ;
- un décodeur local haute résolution ;
- une tête sémantique point-wise.

Tester zéro, un, deux et quatre blocs, ainsi que leur placement tôt/tard. La recommandation initiale est tardive : la géométrie locale doit être apprise avant d'imposer les blocs d'attention hiérarchiques.

## Frontières et adjacency spatiale

Les interactions HSA entre branches sont contraintes par l'arbre. Une erreur de branche lie alors tous les couples concernés. Trois mécanismes de correction sont autorisés, chacun ablaté :

- skip local du backbone ;
- petites arêtes $k_{\mathrm{local}}$-NN ou de frontière entre feuilles de branches voisines ;
- une fraction de têtes locales non hiérarchiques.

Le modèle final ne doit pas cacher un Transformer plat complet sous le nom HSA. Le budget et la portée des voies de correction seront rapportés.

## Sortie et pertes sémantiques

La tête produit 19 logits pour chaque point original. Le chemin feuille–point doit conserver l'ordre exact des points du fichier `.bin`.

Le protocole minimal réutilise l'objectif exact de la baseline reproduite. Les ajouts possibles, testés seulement ensuite, sont :

- loss auxiliaire aux feuilles ou à certains nœuds purs ;
- cohérence parent–enfant pondérée par la pureté prédite ;
- loss de frontière ;
- calibration ou pondération des classes rares.

Ces pertes ne sont ouvertes qu'après preuve de l'effet de l'arbre avec une recette identique. Une supervision majoritaire forcée aux nœuds impurs est exclue au départ.

## Baselines architecturales appariées

Avec exactement le même backbone et le même budget de dimension :

1. aucun module global ;
2. pooling global ou par voxel ;
3. bottom-up/top-down par `mean/max + MLP` ;
4. message passing parent–enfant/frères ;
5. HSA avec arbre aléatoire contrôlé ;
6. HSA avec octree/voxel tree ;
7. HSA avec HGP $K=1$, qui doit être identique au single-linkage et sert de fixture de cohérence ;
8. HSA avec HGP $K=2,3$, RSL/HDBSCAN et les autres arbres de contrôle ;
9. `QC-HSA` avec HGP, puis avec les arbres de contrôle retenus ;
10. attention plate seulement sur sous-échantillon, comme contrôle de qualité et de coût.

Cette matrice empêche d'attribuer à HGP un gain provenant seulement d'un chemin global supplémentaire.

## Ordre HGP et multi-hiérarchie

Commencer par $K=1,2,3$. Le cas $K=1$ doit reproduire exactement le single-linkage et devient une fixture permanente ; il n'est pas compté deux fois dans les comparaisons. L'étude HGP existante sur SemanticKITTI, faite avec masques sémantiques de vérité terrain et pour le regroupement d'instances, observait $K=2$ supérieur à $K=1$ et $K=3$ ; ce résultat ne prouve rien pour la classification sémantique, mais justifie $K=2$ comme candidat initial plutôt qu'une croissance aveugle de $K$.

Une fusion multi-$K$ ou plusieurs arbres par tête ne sera testée qu'après établissement d'un gain pour un $K$ unique. Sinon, elle multiplierait les hypothèses et le coût sans identifier la source du résultat.

## Interface conservée pour l'instance, sans tête active

Le modèle sauvegarde :

- logits et features finales par point ;
- mapping stable point–feuille–ancêtres ;
- topologie et attributs HGP ;
- scores de pureté/incertitude éventuels par nœud.

Cela permettra plus tard de comparer ALPINE, une coupe HGP fixe et une coupe HGP apprise avec les mêmes prédictions sémantiques. Aucun objectif PQ n'entre dans la décision de la phase sémantique.
