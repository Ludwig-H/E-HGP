# Audit mathématique constructif — delta `ac61a77..00cf78c`

**À Claude**  
**Date :** 15 août 2026  
**Dépôt :** `Ludwig-H/E-HGP`, dossier `morsehgp3D_v3/`  
**Head relu :** `00cf78cfaa00e96e823ce1d1816b351fc7674a2c`  
**Dernier audit tiers pris comme base :** `AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md`

> Revue statique des sources, diffs, notes et reçus. Je n’ai pas pu rejouer localement les CTests dans cet environnement. Les nombres d’exécution ci-dessous sont donc reçus comme des résultats rapportés par les commits, tandis que les verdicts de sûreté portent sur les preuves, les invariants et le code relu.

## Verdict

Le mouvement général est bon, et même nettement meilleur que ne le laisserait croire la succession de rétractations. Les cinq rétractations de `32e11e7` ne sont pas cinq régressions : elles montrent que le protocole commence enfin à distinguer une observation, une explication et un théorème. C’est précisément ce qu’il fallait.

Je **reçois positivement** les réparations de sûreté et de comptabilité introduites depuis `eb1b52a` : refus du vrai-vivant hors cap, véritable plafond entier, déclassement de l’invariant circulaire, restauration du mutant `rayon+1`, correction de la comparaison dual-tree et nettoyage de l’index. Je ne vois pas, dans le delta nominal relu, de nouvelle fermeture fausse évidente.

Je ne reçois en revanche **pas encore** les conclusions suivantes sous leur formulation actuelle :

1. « l’ensemble W-vivant est quasi linéaire » ou « il ne devient pas quadratique » sans restriction au régime testé ;
2. « l’instruction est en `O(h)` et non en `O(n)` » comme propriété déterministe ;
3. « l’effet intrinsèque de la séparation est presque invariant en `n` » tant que les deux mesures ne portent pas sur le même univers de `PairId` entièrement décidé ;
4. la qualification complète des crédits q3/q4 et de `h_b`, dont les ledgers restent asymétriques.

Ces réserves n’invalident pas la piste. Elles indiquent exactement comment la rendre publiable et comment éviter de demander à trois pentes locales de résoudre, à elles seules, la théorie asymptotique de la géométrie aléatoire. Les pauvres n’avaient rien demandé.

---

## 1. Contrat mathématique rappelé par les parties I et II de la thèse

La partie I fixe le contrat de sortie : la hiérarchie est l’évolution exacte des composantes connexes d’une filtration géométrique, représentable par un arbre couvrant dans le cas d’ordre deux. La partie II remplace les points par les 
`(K-1)`-simplexes : deux sommets du graphe d’ordre `K` sont adjacents lorsque leur union possède une région de Čech non vide. Les composantes sont ensuite projetées sur les points et peuvent se recouvrir.

Conséquence pour `morsehgp3D_v3` :

- le préfiltre WSPD peut être **fail-open** ;
- une fermeture certifiée doit être exacte ;
- une ancre W-vivante n’est pas encore un support, encore moins une fusion ;
- l’objet final à comparer à l’oracle est la chaîne

```text
ancres W-vivantes
  -> complétions positives et owner
  -> (K-1)-simplexes exacts
  -> événements de fusion exacts
  -> composantes / dendrogramme / payload
```

Les pentes sur le nombre d’ancres sont donc une excellente mesure de broad phase, mais elles ne suffisent pas à recevoir l’algorithme HGP. C’est particulièrement important ici, car le dossier contient déjà la contre-famille `two_lines` : masse universelle quadratique, mais source aiguë q3/q4 vide. Elle montre que le bon levier peut se trouver **après** W-vivant, dans la positivité et l’owner, et non dans un raffinement indéfini des seuls certificats universels.

---

## 2. Ce que je reçois dans les derniers commits

### 2.1. Refus hors cap pour `--vrai-vivant`

La porte

```text
--vrai-vivant && masse_non_decide != 0  => code 3
```

est nécessaire et correcte. Sans elle, les rectangles hors cap alimentent les survivantes sans décider leurs paires : le compte est un minorant du vrai W-vivant et le mou n’a pas le sens annoncé.

### 2.2. Invariant `survivantes >= W-vivantes`

Le commentaire actuel identifie correctement sa circularité. Une fermeture fausse fait disparaître la paire avant le balayage exact ; elle ne peut donc pas violer cet invariant. Le conserver comme garde de comptage est utile, le présenter comme preuve de sûreté ne l’était pas.

### 2.3. Véritable plafond entier

Le remplacement de `floor(sqrt(x))+1` par `ceil(sqrt(x))` est reçu. Le contre-exemple q3 du rayon `+1` est également bon : l’unité est ajoutée au rayon avant élévation au carré et peut donc coûter environ `2R+1`, pas une poussière arithmétique.

### 2.4. Dual-tree

La rétractation du gain `2,2–3,0x` est juste. La vraie optimisation mesurée est la fusion des trois lanes ; le dual-tree est pour l’instant une transformation sémantiquement exacte, mais pas une accélération face à une baseline elle aussi fusionnée. C’est un résultat utile : il évite de complexifier la route de production pour une victoire qui venait du dénominateur.

### 2.5. Cap et facteur `6,4`

La décomposition rapportée, selon laquelle `99,052 %` de la baisse apparente venait de la masse hors cap, explique correctement l’ancien artefact. La correction de `cellule_max`, auparavant mise à jour après le `continue`, est également reçue.

### 2.6. Échantillonneur

La correction de normalisation est juste : les neuf résidus rapportés entre `-1,50` et `+1,52` écarts-types ne présentent pas d’anomalie binomiale visible. La formulation exacte doit toutefois être :

> « aucune anomalie n’est détectée sur ces neuf confrontations ; les écarts sont compatibles avec le modèle binomial ».

Cela ne prouve ni l’indépendance parfaite du générateur ni sa qualité sur toutes les graines. Le sampler peut redevenir un instrument exploratoire, assorti d’un intervalle de Wilson ou de Clopper–Pearson et d’une batterie multi-graines.

### 2.7. Interprétation du mou

Les deux quantités suivantes sont toutes deux légitimes, mais ne répondent pas à la même question :

```text
mu - 1       = surcoût par rapport au plancher V ;
1 - 1 / mu   = fraction des survivantes S qui pourrait encore être retirée.
```

Il ne faut donc pas bannir `mu-1`, seulement le nommer correctement. Pour les cas nuls :

```text
V > 0                  : mu = S / V ;
V = 0 et S > 0         : mu = +inf, fraction retirable = 1 ;
V = 0 et S = 0         : mu = NA, pas 0.
```

Le code imprime encore `0` dans les deux derniers cas ; cela doit être corrigé avant de faire des agrégats.

---

## 3. Résultat théorique utile : pourquoi le W-vivant est linéaire sur Poisson

Les nouvelles pentes peuvent être mieux interprétées qu’avec une simple régression log-log. Il existe ici un calcul exact.

### Proposition 1 — nombre moyen d’ancres vivantes sous Poisson homogène

Soit un processus de Poisson stationnaire d’intensité `lambda` dans `R^d`. Supposons que la région témoin associée à une paire à distance `r` soit une copie homothétique d’une région fixe, de mesure

```text
|W(a,b)| = v r^d.
```

Une paire est dite W-vivante si elle contient strictement moins de `h` autres points dans `W(a,b)`. Pour une suite régulière de fenêtres croissantes `Q_L`, le nombre `V_h(Q_L)` de paires non ordonnées W-vivantes vérifie

```text
E[V_h(Q_L)] / E[|X ∩ Q_L|]
    ->  s_{d-1} h / (2 d v),
```

où `s_{d-1}` est l’aire de la sphère unité de dimension `d-1`. En dimension trois :

```text
E[V_h] / E[n]  ->  2 pi h / (3 v).
```

#### Preuve

Par la formule de Campbell–Mecke, hors terme de bord,

```text
E[V_h] / |Q_L|
 = lambda^2 / 2 * s_{d-1}
   * integral_0^inf r^(d-1) P(Poisson(lambda v r^d) < h) dr.
```

Avec `t = lambda v r^d`, on a

```text
r^(d-1) dr = dt / (d lambda v),
```

et

```text
integral_0^inf P(Poisson(t) < h) dt
 = sum_{k=0}^{h-1} integral_0^inf exp(-t) t^k / k! dt
 = h.
```

La formule suit. Le terme de bord est `o(|Q_L|)` pour des fenêtres régulières.

### Conséquence pratique

Sur la famille `uniform`, la croissance linéaire n’est pas seulement « compatible avec quatre tailles » : elle est **la prédiction théorique exacte en espérance** du régime de Poisson homogène à densité fixe. Il faut donc comparer `V_q/n` à la constante analytique `2 pi h_q/(3 v_q)`, et pas seulement ajuster un exposant.

Cela donne une gate bien plus informative :

```text
uniform : V_q / n doit converger vers la constante de Poisson calculée ;
terrain/scanline : mesurer l’écart à cette constante et l’expliquer ;
eight_clusters : ne pas attendre la constante homogène à travers les vides.
```

### Proposition 2 — coût moyen de la lentille pour une paire vivante

Soit `L(a,b)` la lentille de candidats, avec `W(a,b) subset L(a,b)`, et

```text
R = |L(a,b)| / |W(a,b)|.
```

Pour une paire W-vivante choisie selon la mesure de Palm des paires vivantes du processus homogène :

```text
E[#(X ∩ W) | paire vivante] = (h - 1) / 2,
E[lambda |W| | paire vivante] = (h + 1) / 2,
E[#(X ∩ L) | paire vivante] = (R h + R - 2) / 2.
```

Le premier résultat est même plus précis : le nombre de témoins dans `W` est uniforme sur `{0,...,h-1}` après intégration sur les distances des paires vivantes.

Avec le rapport `R = 10,86` et `h_4 = 8` annoncé dans la note, la prédiction homogène vaut environ

```text
(10,86 * 8 + 10,86 - 2) / 2 = 47,9 candidats,
```

pas `87`. La mesure `uniform = 38,9` est donc beaucoup plus proche du modèle homogène que `eight_clusters = 87,8`. La famille groupée ne « colle exactement » pas au modèle uniforme ; elle met au contraire en évidence l’effet de l’inhomogénéité et des vides entre amas.

Ce calcul fournit une formulation propre :

> L’instruction par lentille est `O(h)` **en espérance sous le modèle de Poisson homogène à densité fixe**, avec une constante explicite. Elle n’est pas `O(h)` au pire cas.

### Contre-exemple déterministe au `O(h)`

La différence `L(a,b) \ W(a,b)` contient un ouvert non vide. On peut y placer `Theta(n)` points distincts, sans placer aucun point dans `W(a,b)`. L’ancre reste W-vivante, mais sa lentille contient `Theta(n)` candidats. Le pire cas de l’instruction reste donc linéaire par ancre.

Plus sévère encore, la contre-famille `two_lines` déjà gravée dans le dépôt produit `Theta(n^2)` ancres universellement vivantes tout en ayant zéro porteur aigu q3/q4. La phrase globale « il ne devient pas quadratique » est donc fausse sans hypothèse de régularité.

### Proposition 3 — condition déterministe suffisante

Une version déterministe reste possible. Supposons que le nuage soit `d`-Ahlfors régulier aux échelles utiles : il existe `c,C>0` tels que, pour toute boule pertinente,

```text
c r^d <= #X ∩ B(x,r) <= C r^d,
```

et que le fuseau `W_q(a,b)` contienne une boule de rayon `kappa_q |ab|`. Si `(a,b)` est W-vivante, la borne inférieure impose

```text
|ab|^d <= (h_q + O(1)) / (c kappa_q^d).
```

La borne supérieure implique alors que chaque point n’a que `O((C/c) h_q)` partenaires W-vivants. Ainsi

```text
|V_q| = O((C/c) h_q n).
```

C’est le bon théorème conditionnel pour expliquer les familles sans grands trous. Il échoue exactement sur les mélanges séparés, les droites croisées et les scènes comportant des vides macroscopiques. Autrement dit, le résultat dit quelque chose de géométrique, au lieu de demander à une pente de faire semblant d’être une hypothèse de régularité.

---

## 4. Corrections P0 encore nécessaires

### P0.1 — reformuler les conclusions d’échelle

Formulation proposée :

> Sur les quatre tailles et les trois familles testées, les neuf exposants locaux de `|V_4|` appartiennent à `[1,068 ; 1,163]`. Aucune dérive vers `2` n’est observée sur cette rampe. Ces données sont compatibles avec une croissance proche de linéaire sur l’intervalle testé ; elles n’établissent ni `o(n^2)`, ni l’absence d’un changement de régime. Sur Poisson homogène à densité fixe, une croissance linéaire en espérance peut être démontrée par Campbell–Mecke.

Il faut ajouter les familles réellement LiDAR déjà disponibles :

- `scanline_single_pass` ;
- `scanline_overlap_multiecho` ;
- et conserver `two_lines` comme contre-régime quadratique explicite.

Les trois familles actuelles ne suffisent pas à conclure sur les anisotropies de balayage, les trous de lignes et les recouvrements multi-passes.

### P0.2 — comparaison `s=6` / `s=8` sur un même univers

La décomposition agrégée du facteur `6,4` est convaincante, mais la phrase « effet intrinsèque presque invariant » demande un protocole apparié.

Solution la plus simple : **interdire toute comparaison de séparations dès que l’une des deux exécutions a `masse_non_decide != 0`**. À `n=32000`, augmenter le cap au-delà du vrai `cellule_max`, ou mieux, subdiviser récursivement tout rectangle trop gros en coupant le plus grand des deux nœuds jusqu’à respecter le cap. Les sous-boîtes d’un couple séparé restent séparées, donc ce raffinement conserve la sûreté et la partition.

Si l’on souhaite conserver un cap dur, construire pour q4 un statut par `PairId` :

```text
D0 | OFFCAP | CLOSED | SURVIVE
```

et publier la matrice de migration entre `s=6` et `s=8`. L’effet des certificats doit être mesuré sur

```text
J = { PairId : D>0, statut_s6 != OFFCAP, statut_s8 != OFFCAP }.
```

Le passage `OFFCAP -> décidé` doit être reporté séparément, jamais mélangé au gain de fermeture.

### P0.3 — exclure les ancres de diamètre nul sans perdre les multiplicités

`PROPOSITION.md` donne le bon domaine : une ancre diamétrale exige `D>0`. Le balayage courant parcourt cependant tous les indices `i<j`, même si leurs coordonnées sont identiques. Pour `D=0`, les prédicats rendent naturellement aucun témoin et la paire devient artificiellement W-vivante.

L’univers propre est

```text
T+ = C(n,2) - sum_x C(m_x,2),
```

où `m_x` est la multiplicité de la position `x`.

Les identifiants dupliqués doivent rester dans les pools de témoins. La bonne architecture est donc :

1. regrouper les `PointId` par coordonnée ;
2. construire la géométrie WSPD sur les positions distinctes ;
3. transporter les multiplicités dans les masses de paires et dans les comptes de témoins ;
4. ne jamais créer d’ancre entre deux identifiants de la même position ;
5. réexpanser les identifiants seulement lorsque le support exact l’exige.

À court terme, si cette agrégation n’est pas encore prête, le probe diagnostique doit refuser explicitement toute position dupliquée plutôt que de publier un compte faux sous un profil qui prétend les supporter.

Fixture minimale : deux `PointId` à la même coordonnée et un troisième point distinct. La paire dégénérée doit être absente des ancres, mais les deux identifiants doivent compter comme deux témoins pour une autre ancre lorsque la géométrie les admet.

### P0.4 — mettre la documentation exécutable en cohérence

`AUDIT_ETAT_COURANT.md` se proclame encore autorité mutable unique au pin `66b4f0c`. Il est désormais périmé. `00cf78c` corrige l’index, mais pas cette contradiction.

À faire :

- remplacer son bandeau par un renvoi explicite au nouvel audit courant ;
- marquer les notes historiques rétractées comme telles dès leur première ligne ;
- retirer des commentaires de `combined_prefilter_probe.cpp` l’ancien récit des « trois à douze sigmas » ;
- remplacer « instruction en `O(h)` » par la formulation probabiliste ci-dessus ;
- remplacer « exact sans `O(n^3)` » par :

> coût `O(n^2 + n|S|)` avec sorties anticipées ; sous-linéaire dans le cube lorsque le résiduel est sous-quadratique, mais cubique au pire cas.

Un commentaire source faux est plus dangereux qu’une note historique fausse : il accompagne directement la prochaine modification, comme un petit conseiller malveillant installé dans l’éditeur.

---

## 5. Portes d’implémentation encore manquantes

### 5.1. Ledger q3/q4 aux feuilles

Le ledger par lane enregistre les identifiants crédités en bloc, mais les incréments q3/q4 effectués aux feuilles ne poussent pas encore leurs `PointId` dans `core_ids[q]`. L’oracle peut donc manquer un double crédit `bulk -> feuille` ou `feuille -> feuille` sur q3/q4.

Correction : à chaque incrément effectif de `hcore[q]`, enregistrer `(PointId, source)` sous oracle, pour les trois lanes et toutes les branches :

```text
source = BULK_BALL | BULK_Q2 | LEAF_CORNER64 | LEAF_BOUNDS.
```

Par lane, un second crédit du même identifiant est une erreur ; un identifiant dans `A ∪ B` est une erreur. Ajouter deux mutants qui omettent respectivement l’effacement du masque q3 et q4 après un crédit en bloc.

### 5.2. Vérification symétrique de `h_a` et `h_b`

`--verifie-jointure` confronte actuellement la route dual-tree à la référence pour le côté `A`, pas pour `B`. Il faut :

- `dual_verifies_A`, `dual_ecarts_A` ;
- `dual_verifies_B`, `dual_ecarts_B` ;
- une porte métamorphique obtenue en échangeant `A` et `B` ;
- une fixture qui tue réellement `drop-B`.

Cette symétrie n’est pas cosmétique : le minorant final est `h_core + h_a + h_b`. Tester seulement la moitié d’une somme est une tradition humaine ancienne, mais elle reste mathématiquement peu convaincante.

### 5.3. Nommer la composition réelle

Le reçu imprime encore seulement `coeur_mode=corner64|bornes`, alors que la route peut être `boule+corner64` ou `boule+bornes`. Il faut imprimer la composition complète et refuser les options `h_a` mutuellement incompatibles au lieu d’appliquer une précédence silencieuse.

Si la boule n’est qu’un chemin `ALL` accéléré, alors

```text
ball+corner64 == corner64
```

sur les décisions finales. Cette égalité appariée doit devenir une porte permanente.

### 5.4. Instrumenter l’instruction réelle, pas seulement sa première lentille

Le compteur actuel mesure, pour les ancres q4 W-vivantes, le nombre de points dans la lentille du troisième sommet. C’est utile, mais ce n’est pas encore le coût de l’instruction complète.

Publier séparément :

```text
sum_lens, p50, p95, p99, max_lens,
positive_owned_seeds,
axial_candidates,
exact_supports,
fusion_events,
payload_bytes.
```

L’algorithme scalable naturel est une requête exacte d’intersection de deux boules sur l’octree Morton :

- `OUT` si la boîte est extérieure à l’une des deux boules ;
- `IN` si elle est intérieure aux deux ;
- descente sinon.

Cela construit un CSR de candidats par arête sans rescanner les `n` points. La positivité aiguë et l’owner doivent être appliqués avant de matérialiser tout le reste. La contre-famille `two_lines` doit alors passer de `Theta(n^2)` ancres W-vivantes à zéro seed positif sans allocation quadratique.

---

## 6. Contre-audit de l’autre auditeur

L’autre audit est solide sur les points importants : biais de cap, baseline dual-tree inéquitable, circularité du vrai-vivant, erreur de normalisation du sampler, contre-exemple du `+1`, ledger q3/q4 et vérification `h_b`. Ses demandes ont conduit à de vraies améliorations.

Trois nuances doivent cependant être conservées :

1. **Trois exposants successifs constituent une gate empirique, pas un théorème asymptotique.** Le dépôt contient lui-même `two_lines`, qui interdit toute conclusion globale non conditionnelle.
2. **« Compatible avec le binomial » ne signifie pas « générateur prouvé sain ».** C’est une absence d’anomalie sur un échantillon de graines.
3. **`mu-1` n’est pas faux en soi.** C’est le surcoût relativement au plancher ; `1-1/mu` est la part retirable relativement au résiduel. Il faut publier les deux avec leurs dénominateurs.

Je renforce en revanche sa demande de protocole cap-aware : toute phrase causale sur l’effet de `s` doit être bloquée tant que l’univers décidé n’est pas commun ou complet.

---

## 7. Ordre de travail recommandé

### P0 — avant toute nouvelle conclusion chiffrée

1. corriger les formulations asymptotiques et le `O(h)` ;
2. rendre `AUDIT_ETAT_COURANT.md` réellement courant ;
3. corriger les cas `V=0` du mou ;
4. rerun `s=6/s=8` avec `masse_non_decide=0` des deux côtés ;
5. exclure `D=0` ou réduire explicitement le profil aux positions uniques.

### P1 — fermer la qualification du préfiltre

6. ledger q3/q4 aux feuilles avec provenance ;
7. vérification séparée et symétrique `h_a/h_b` ;
8. composition des modes imprimée et portes appariées ;
9. campagne `uniform/terrain/scanline/multiecho/eight_clusters/two_lines`, plusieurs graines.

### P2 — reconnecter le broad phase à HGP

10. requête de lentille exacte par octree, groupée par arête ;
11. comptage des seeds positifs/owner puis des supports ;
12. oracle petit `n` sur les **identités** des `(K-1)`-simplexes et des événements de fusion ;
13. comparaison finale des composantes et du payload, en autorisant les recouvrements prévus par la partie II de la thèse.

---

## 8. Tableau de réception

| Élément | Verdict |
|---|---|
| Refus `--vrai-vivant` hors cap | **Reçu** |
| Véritable `ceil_sqrt` | **Reçu** |
| Invariant circulaire correctement étiqueté | **Reçu** |
| Rétractation du facteur `6,4` | **Reçue** |
| Rétractation du gain dual-tree | **Reçue** |
| Sampler réhabilité | **Reçu comme outil exploratoire**, pas comme oracle |
| Pentes `1,068–1,163` | **Reçues comme observations locales** |
| « W-vivant quasi linéaire » sans hypothèse | **Non reçu** ; remplacer par le théorème Poisson/Ahlfors conditionnel |
| « ne devient pas quadratique » | **Réfuté globalement** par `two_lines` |
| Instruction `O(h)` déterministe | **Non reçu** ; vrai en espérance sous Poisson avec constante explicite |
| Effet intrinsèque de `s` invariant | **En attente** d’un univers complet ou apparié |
| Sûreté q3/q4 par identité | **En attente** du ledger aux feuilles |
| Équivalence dual-tree pour `h_b` | **En attente** de la porte symétrique |
| Pipeline exact jusqu’aux fusions HGP | **Toujours ouvert**, comme correctement annoncé par le README |

## Conclusion à Claude

Le préfiltre combiné est désormais beaucoup plus crédible qu’au pin `eb1b52a`. La priorité n’est plus d’inventer un cinquième certificat géométrique marginal : c’est de stabiliser les univers comparés, fermer les ledgers d’identité et exploiter la structure probabiliste du W-vivant.

Le calcul de Campbell–Mecke donne précisément le résultat théorique qu’il manquait : linéarité en espérance sous Poisson homogène et coût moyen de lentille `O(h)` avec constante explicite. En parallèle, `two_lines` rappelle que cette propriété n’est pas universelle et que la positivité aiguë doit intervenir tôt dans la route de production.

La bonne suite est donc : **recevoir proprement le broad phase, puis mesurer la contraction W-vivant -> seeds positifs -> supports -> fusions**. C’est là que le projet rejoint réellement la thèse, au lieu de rester un compteur d’ancres remarquablement sophistiqué.
