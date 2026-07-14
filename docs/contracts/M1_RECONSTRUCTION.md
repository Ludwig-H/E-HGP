# Contrat candidat M.1 — reconstruction de `full_pi0`

> [!WARNING]
> **Statut scientifique : `proof_obligation`.** Ce document fige un énoncé à démontrer; il ne contient pas sa preuve et ne promeut pas `full_pi0` au statut `exact`. Un accord exhaustif avec l'oracle est nécessaire à la phase 12, mais ne remplace pas la preuve.

| propriété | valeur normative |
|---|---|
| identifiant du contrat | `M1-reconstruction-v1` |
| phase d'introduction | 0 — gel du contrat mathématique |
| profil concerné | `full_pi0` |
| backends concernés | `reference_cpu`, `cuda_g4` |
| modes concernés | `certified`, `budgeted` |
| statut courant | `proof_obligation` |
| phase de fermeture visée | 12 — preuve et implémentation `full_pi0` générique |

La [spécification générale](../SPECIFICATION_MORSEHGP3D.md) demeure l'autorité sur l'objet calculé et le [registre des preuves](../math/STATUT_PREUVES_ET_HEURISTIQUES.md) sur le statut scientifique. Le présent document rend leur obligation M.1 directement testable. Toute modification de ses hypothèses, de sa conclusion ou de la règle de lot exige un nouvel identifiant de contrat.

## 1. Objet et exclusions

M.1 vise la reconstruction de toutes les composantes des multicovertures, y compris les composantes temporairement représentées par une facette isolée, leurs absorptions ultérieures et les applications entre ordres.

Le contrat ne porte pas sur :

- la seule découverte locale des centres critiques;
- la seule validité d'une descente d'attache prise isolément;
- la réduction de Gabriel du profil `hgp_reduced`;
- une condensation aval en partition de points;
- les shells dégénérés, plateaux ou doublons avant la fermeture des extensions correspondantes;
- une approximation statistique de la hiérarchie.

À $k=1$, `hgp_reduced` et `full_pi0` coïncident par convention et la forêt doit aussi satisfaire l'oracle EMST. Pour $k\geq2$, la preuve du profil réduit ne vaut pas preuve de `full_pi0`. En particulier, un flot de Gabriel exact peut omettre la généalogie d'une facette isolée sans contredire le théorème du profil réduit.

## 2. Notation

Soit $X=\left\lbrace x_1,\ldots,x_n\right\rbrace\subset\mathbb{R}^{3}$ un ensemble fini non vide de points distincts. On fixe

$$K_{\mathrm{eff}}=\min(K_{\max},n),\qquad s_{\max}=\min(K_{\mathrm{eff}}+1,n).$$

Pour $y\in\mathbb{R}^{3}$, $D_k(y)$ est la $k$-ième distance carrée de $y$ à $X$, avec multiplicité. Les sous-niveaux fermé et strict sont

$$L_k(a)=\left\lbrace y:D_k(y)\leq a\right\rbrace,\qquad L_k^{<}(a)=\left\lbrace y:D_k(y)<a\right\rbrace.$$

La tour complète recherchée est

$$\mathcal{H}_X(k,a)=\pi_0\bigl(L_k(a)\bigr),\qquad 1\leq k\leq K_{\mathrm{eff}}.$$

Pour un centre critique $c$ de niveau carré exact $a$, on note

$$I(c,a)=X\cap B^{\circ}(c,\sqrt{a}),\qquad U(c,a)=X\cap\partial B(c,\sqrt{a}),\qquad s(c,a)=\lvert I(c,a)\rvert+\lvert U(c,a)\rvert.$$

À l'ordre $k$, l'indice local est $\mu_k(c)=s(c,a)-k$ et la multiplicité locale de Reani–Bobrowski est

$$\Delta_{\mu}(c)=\binom{\lvert U(c,a)\rvert-1}{\mu}.$$

Pour un événement d'indice un, $s=k+1$, on pose $S=I\cup U$. Ses germes stricts, appelés **bras**, sont indexés par le shell complet :

$$F_u=S\setminus\lbrace u\rbrace,\qquad u\in U.$$

Il existe donc $\lvert U\rvert$ bras, même si $\Delta_1=\lvert U\rvert-1$. La multiplicité est un rang local; elle n'est pas le nombre de fusions globales à créer.

## 3. Données de reconstruction

Une reconstruction M.1 consomme les objets canoniques suivants.

### 3.1 Événements critiques

Chaque `CriticalEvent` fournit au minimum :

- le rang fermé $s$;
- les identifiants complets de $I$ et $U$;
- le support minimal canonique;
- le centre homogène exact et le niveau carré exact;
- les signes barycentriques;
- l'indice utilisé par chaque ordre concerné et la multiplicité locale correspondante;
- la classe de dégénérescence et le statut de tous les prédicats décisifs.

Deux émissions représentent le même événement si et seulement si leur clé canonique complète est la même. Une approximation du centre, un identifiant de thread ou un ordre d'arrivée ne participe jamais à cette identité.

### 3.2 Attaches

Pour chaque événement d'indice un à l'ordre $k$ et chaque $u\in U$, il existe un `Attachment` distinct identifié sémantiquement par `(event_id, order, removed_shell_id)`. Il relie le bras $F_u$ à une composante de $L_k^{<}(a)$.

Une attache certifiée contient ou référence :

- la facette initiale $F_u$;
- un témoin que le chemin quitte immédiatement le centre dans le germe associé à $u$;
- une suite rejouable de segments restant dans $L_k^{<}(a)$ après leur point initial;
- les facettes et niveaux exacts de chaque étape;
- la composante ou racine pré-lot terminale;
- les prédicats de décroissance et leur statut exact.

Deux bras peuvent légitimement avoir la même cible. Une attache n'est jamais décidée par proximité flottante, par voisinage approché ou en consultant l'état déjà muté du lot courant.

### 3.3 Lots de niveaux égaux

Un `EqualLevelBatch` est défini par le couple `(order, squared_level_exact)`. Il contient **tous** les minima, événements d'indice un, bras, activations et incidences du profil à ce niveau exact. Deux fractions égales appartiennent au même lot après comparaison exacte, même si leurs représentations initiales diffèrent. Deux niveaux distincts n'appartiennent jamais au même lot, quelle que soit leur proximité flottante.

Le lot enregistre ou engage de manière rejouable :

- les identifiants complets des événements;
- l'état des composantes strictement avant le niveau;
- les cibles pré-lot des bras;
- l'hypergraphe simultané à contracter;
- les naissances, prolongements et multifusions résultants;
- l'état fermé post-lot;
- les ancres verticales posées après cet état fermé.

## 4. Hypothèses du candidat M.1

Les hypothèses suivantes sont cumulatives. Une hypothèse non établie interdit d'invoquer la conclusion.

### H1 — sémantique exacte de l'entrée

- $1\leq K_{\max}\leq10$ et $K_{\mathrm{eff}}=\min(K_{\max},n)$;
- les coordonnées IEEE-754 finies sont interprétées comme dyadiques exacts;
- les points sont distincts;
- tous les niveaux publics sont des rayons carrés exacts;
- aucun prédicat combinatoire décisif ne reste `unknown`.

### H2 — domaine générique certifié

Le périmètre utile satisfait la position générale de la spécification : shell critique utile de taille au plus quatre, support minimal unique et affinement indépendant, centre dans son intérieur relatif avec signes barycentriques non nuls, absence de point extérieur sur la frontière d'une miniball Gabriel utile, et absence de plateau dans les descentes employées.

`RelevantGP` est certifié sur tout le périmètre requis. L'absence de dégénérescence parmi les seuls événements acceptés ne suffit pas.

### H3 — catalogue complet

Le catalogue contient exactement tous les événements critiques de rang $1\leq s\leq s_{\max}$ et aucun faux événement. Le shell global est terminé avant le filtrage par rang. Pour chaque ordre $k$ :

- tous les minima de rang $k$ sont présents;
- tous les événements d'indice un de rang $k+1$ sont présents lorsque ce rang existe;
- niveaux, rangs, supports, shells et clés canoniques sont exacts;
- toutes les cellules enfants et incidences croisées utilisées pour conclure à la complétude sont fermées.

Pour $k=n$, le minimum terminal de niveau $\beta(X)$ est conservé et aucun événement impossible de rang $n+1$ n'est inventé.

### H4 — données locales complètes

Pour chaque centre critique et chaque ordre utilisé, l'indice et la multiplicité locale sont exacts. À l'indice un, les $\lvert U\rvert$ bras $F_u$ sont tous émis, y compris lorsque plusieurs bras auront la même attache globale.

Si les bras d'un centre isolé atteignent $q$ composantes pré-lot distinctes, ce centre peut tuer $q-1$ classes de $H_0$, avec

$$1\leq q\leq\lvert U\rvert,\qquad 0\leq q-1\leq\Delta_1=\lvert U\rvert-1.$$

Pour un lot contenant plusieurs centres, les décès se calculent après contraction de l'hypergraphe entier; ils ne sont pas la somme aveugle des $q-1$ centre par centre.

### H5 — attaches strictes complètes

Pour tout ordre $k$, tout événement d'indice un de niveau $a$ et tout $u\in U$, l'attache du bras $F_u$ à sa composante de $L_k^{<}(a)$ existe, est rejouable et a été vérifiée. L'ensemble des `Attachment` est en bijection avec l'ensemble des triples attendus `(event_id, order, removed_shell_id)`.

Le chemin initial identifie le bon germe local; la suite reste dans le sous-niveau strict; la cible désigne une composante de l'état pré-lot. Une racine atteinte par plusieurs chemins doit être indépendante des choix canoniques autorisés.

### H6 — simultanéité exacte

Pour chaque ordre et chaque niveau exact, l'état strictement antérieur est figé avant toute union. Tous les événements du niveau sont insérés dans un hypergraphe commun, puis ses composantes sont contractées en une seule opération sémantique. Les unions physiques, la publication des nœuds et les ancres verticales n'interviennent qu'après détermination de cette contraction.

Un lot ne dépend ni de l'ordre des événements, ni de leur shard, ni du pivot choisi pour représenter une hyperarête, ni de l'ordonnancement des threads. Une sérialisation binaire éventuelle de la forêt doit marquer ses nœuds auxiliaires et permettre leur contraction vers l'unique multifusion canonique.

Toute incidence entre une naissance et un événement distinct du même niveau doit provenir de l'objet simultané certifié; elle ne peut pas être créée en traitant arbitrairement l'un avant l'autre. La preuve de suffisance de cette règle fait partie des obligations ouvertes.

### H7 — morphismes verticaux complets

Pour chaque $1\leq k<K_{\mathrm{eff}}$ et chaque niveau, les données candidates associent à toute composante source une cible unique accompagnée d'un témoin de l'inclusion $L_{k+1}(a)\subseteq L_k(a)$. Les ancres de rang $2\leq s\leq K_{\mathrm{eff}}$ sont posées vers l'état fermé post-lot de l'ordre inférieur, puis propagées sur les forêts.

Les applications candidates sont totales sur le profil `full_pi0`, leurs cibles sont indépendantes du représentant choisi et chaque étape de propagation est rejouable. La commutation globale des carrés n'est pas supposée dans H7 : elle appartient à la conclusion de M.1 et doit aussi être recherchée comme falsificateur. Une application partielle ne satisfait pas H7.

### H8 — sortie rejouable et sans troncature

La forêt conserve les naissances, multifusions et niveaux exacts. Le `coverage_log` et les labels nécessaires permettent de reconstruire chaque coupe, y compris des unions de points qui se recouvrent pour $k\geq2$. Aucun budget, overflow, événement non résolu, attache manquante ou erreur numérique n'est masqué.

## 5. Énoncé candidat à démontrer

> **M.1 — reconstruction complète, version candidate 1.** Sous H1–H8, l'insertion des minima, l'attache de tous les bras d'indice un dans les sous-niveaux stricts, la contraction simultanée de chaque niveau exact et la propagation des ancres verticales produisent une tour de forêts dont les coupes représentent naturellement $\mathcal{H}_X(k,a)$ pour tout $1\leq k\leq K_{\mathrm{eff}}$ et tout $a\geq0$.

Plus précisément, l'énoncé à prouver exige, pour chaque $k$ et $a$, une bijection canonique

$$\Phi_{k,a}:\mathrm{Cut}(T_k,a)\xrightarrow{\sim}\pi_0\bigl(L_k(a)\bigr).$$

Ces bijections doivent préserver les naissances, les absorptions, les multifusions et les unions d'observations recouvrantes. Pour $a\leq b$, elles doivent aussi être naturelles pour les applications horizontales et verticales :

$$\Phi_{k,b}\circ\widehat{h}_{k,a,b}=h_{k,a,b}\circ\Phi_{k,a},\qquad \Phi_{k,a}\circ\widehat{v}_{k,a}=v_{k,a}\circ\Phi_{k+1,a}.$$

La conclusion comprend donc simultanément :

1. aucune composante de $L_k(a)$ n'est omise;
2. aucune composante ou connexion inexistante n'est inventée;
3. chaque changement de $H_0$ est représenté au bon niveau exact;
4. un lot égal produit une multifusion canonique, pas une généalogie binaire dépendante de l'ordre;
5. la tour commute en ordre et en échelle.

Cette section est un **énoncé**, pas une démonstration. Tant que les obligations de la section 8 ne sont pas fermées et relues, les symboles $\Phi_{k,a}$ désignent les isomorphismes candidats que l'implémentation et l'oracle doivent tenter de falsifier.

## 6. Algorithme sémantique couvert par l'énoncé

Pour un ordre $k$ et un niveau exact $a$, la reconstruction candidate suit cette transaction indivisible :

1. figer les composantes représentant $L_k^{<}(a)$;
2. introduire toutes les naissances de rang $k$ au niveau $a$;
3. charger tous les événements d'indice un de rang $k+1$ au même niveau;
4. résoudre chacun de leurs $\lvert U\rvert$ bras vers une racine pré-lot;
5. construire l'hypergraphe simultané complet, incluant toute incidence certifiée du niveau fermé;
6. contracter ses composantes sans muter l'état pendant leur découverte;
7. publier les naissances survivantes, les prolongements et les multifusions canoniques;
8. appliquer les unions et enregistrer la couverture;
9. poser les ancres verticales vers l'état post-lot;
10. vérifier les carrés de naturalité affectés.

Cette transaction est descriptive : l'obligation ouverte O4 doit encore démontrer qu'elle reproduit le passage topologique du sous-niveau strict au sous-niveau fermé lorsque plusieurs centres partagent $a$. Une implémentation parallèle peut employer toute représentation interne qui produit exactement la même transaction canonique.

## 7. Liens avec les champs de certificat

Les noms ci-dessous sont des champs logiques normatifs; leur encodage est figé par les schémas de phase 0.

| condition | objet ou champ probant | règle pour une invocation de M.1 |
|---|---|---|
| version du contrat | `RunCertificate.reconstruction_contract_id` | vaut exactement `M1-reconstruction-v1` |
| base scientifique | `RunCertificate.proof_basis` | vaut `m1_conditional_contract` avant preuve et ne peut valoir `full_pi0_proved` qu'après fermeture relue de M.1 |
| sémantique de l'entrée | `RunCertificate.input_semantics` | coordonnées, unités, $K_{\max}$, $K_{\mathrm{eff}}$ et politique de doublons conformes à H1 |
| domaine générique | `RunCertificate.general_position_status` | statut certifié, jamais inféré d'une tolérance |
| fermeture `RelevantGP` | `RunCertificate.relevant_gp_complete` | vrai sur tout le périmètre utile |
| enfants canoniques | `RunCertificate.canonical_children_complete` | vrai pour tous les parents requis |
| incidences croisées | `RunCertificate.active_cross_incidences_complete` | vrai avant de déclarer le catalogue complet |
| événements locaux | `CriticalEvent.closed_rank`, `interior_ids`, `shell_ids`, `minimal_support_ids`, `center_witness_homogeneous`, `squared_level_exact`, `barycentric_signs`, `degeneracy_class`, `predicate_status` | complets, canoniques et exacts |
| indice et multiplicité | `CriticalEvent.birth_order`, `saddle_order` et `morse_roles[]` | chaque rôle donne `order`, `morse_index`, `local_multiplicity` et `arm_count`; $\mu=s-k$ et $\Delta_{\mu}=\binom{\lvert U\rvert-1}{\mu}$ sont rejouables |
| catalogue | `RunCertificate.catalog_complete_by_rank[1..s_max]` | vrai à chaque rang requis par le profil |
| bras et chemins | `Attachment` | un enregistrement exact par triple attendu, cible pré-lot et chemin strict rejouable |
| complétude des attaches | `RunCertificate.attachments_complete_by_order[1..Keff]` | vrai à chaque ordre; aucun bras omis ou indécis |
| simultanéité | `EqualLevelBatch` | niveau exact, événements complets, état pré-lot, contraction et état post-lot rejouables |
| fermeture des lots | `RunCertificate.batches_complete_by_order[1..Keff]` | vrai à chaque ordre et invariant sous permutation |
| tour verticale | `VerticalMap` et `RunCertificate.vertical_maps_complete` | données candidates totales, cibles uniques et témoins rejouables; la naturalité est une conclusion auditée séparément |
| restitution des coupes | `MergeForest`, `coverage_log` | toutes les coupes du profil complet sont rejouables |
| limites et arrêts | `RunCertificate.partial_guarantees`, `fallback_counts`, `budgets_and_stop_reason` | aucune limite incompatible avec la conclusion n'est dissimulée |

Les compteurs ne remplacent pas les booléens de fermeture. Inversement, un booléen vrai sans artefact rejouable est invalide.

### 7.1 Règle d'agrégation des attaches

Pour un ordre $k$, `attachments_complete_by_order[k]` vaut vrai si et seulement si :

1. le catalogue du rang $k+1$ est complet;
2. l'ensemble attendu des triples `(event_id, order, removed_shell_id)` est calculé depuis les shells complets;
3. chaque triple possède exactement un `Attachment` accepté;
4. chaque chemin passe son replay exact et cible une composante pré-lot;
5. aucune cible ne reste ambiguë ou partielle.

### 7.2 Règle d'agrégation des lots

Pour un ordre $k$, `batches_complete_by_order[k]` vaut vrai si et seulement si chaque événement utile est affecté à exactement un lot d'égalité algébrique, tous les runs susceptibles de contenir ce niveau ont été fusionnés, la contraction simultanée est terminée, et son résultat canonique est sérialisé.

## 8. Obligations de preuve ouvertes

Chaque obligation reste `proof_obligation` jusqu'à une preuve lisible, liée dans `proof_basis`, et des tests de falsification correspondants.

| identifiant | obligation à fermer |
|---|---|
| O1 — changements de $H_0$ | déduire rigoureusement de la théorie locale que, dans le domaine H2, aucun changement global de $\pi_0$ ne survient hors des minima et événements d'indice un catalogués |
| O2 — complétude des germes | montrer que les $\lvert U\rvert$ facettes $F_u$ représentent tous et seulement les germes stricts pertinents d'un événement d'indice un |
| O3 — local vers global | montrer qu'un chemin d'attache certifié identifie exactement la composante globale du germe dans $L_k^{<}(a)$ et que toutes les attaches nécessaires sont couvertes |
| O4 — niveaux simultanés | prouver que la contraction d'un unique hypergraphe par niveau représente le passage de $L_k^{<}(a)$ à $L_k(a)$ pour plusieurs centres, y compris les interactions naissance–selle, sans ordre auxiliaire |
| O5 — multiplicité | relier la multiplicité locale $\Delta_{\mu}$ au nombre de bras et démontrer la comptabilité globale des décès $H_0$ sans addition abusive entre événements incidents |
| O6 — exhaustivité temporelle | montrer que les coupes de la forêt sont constantes entre niveaux critiques et exactes aux niveaux fermés, y compris les cas limites $k=1$ et $k=n$ |
| O7 — verticalité | prouver l'existence, l'unicité et la propagation des cibles verticales complètes, puis la commutation de tous les carrés ordre–échelle |
| O8 — domaine | démontrer que les hypothèses de H2 excluent précisément les phénomènes locaux non traités utilisés dans O1–O7, sans généralisation silencieuse aux plateaux |
| O9 — représentation finie | prouver que `MergeForest`, `coverage_log`, lots et ancres suffisent à rejouer toutes les composantes, notamment les composantes recouvrantes pour $k\geq2$ |

Les preuves externes déjà enregistrées — correspondance K-polyèdres–$\pi_0$, caractérisation locale des centres, indice et multiplicité — peuvent être citées. Le passage simultané local–global et la naturalité complète ne sont pas considérés comme acquis par simple citation de ces résultats.

## 9. Contre-exemples et falsificateurs attendus

Les familles suivantes doivent casser une reconstruction affaiblie. Elles ne sont pas annoncées comme des contre-exemples à M.1; elles sont des recherches prioritaires de contre-exemples au candidat ou à ses implémentations.

| identifiant | construction minimale recherchée | erreur qu'elle doit révéler |
|---|---|---|
| CE-M1-01 | deux centres distincts au même niveau exact partageant au moins une composante pré-lot | traitement séquentiel, binarisation ou dépendance à l'ordre |
| CE-M1-02 | shell $\lvert U\rvert=3$ dont les trois bras atteignent trois composantes | confusion entre deux décès locaux et deux bras; perte d'une fusion triple |
| CE-M1-03 | shell $\lvert U\rvert=4$ dont les quatre bras atteignent quatre composantes | troncature des bras ou binarisation d'une fusion quadruple |
| CE-M1-04 | événement avec plusieurs bras déjà dans la même composante, puis cas avec tous les bras déjà reliés | interprétation de $\Delta_1$ comme nombre obligatoire de décès globaux |
| CE-M1-05 | facette isolée persistante puis absorbée par une coface non-Gabriel | promotion indue du flot de Gabriel réduit en reconstruction `full_pi0` |
| CE-M1-06 | niveaux rationnels égaux issus de supports différents, suivis de niveaux strictement voisins | groupement par représentation ou par epsilon au lieu de l'égalité exacte |
| CE-M1-07 | naissance et selle distinctes partageant un niveau, avec ordonnancements inversés | incidence circulaire créée depuis un état déjà muté |
| CE-M1-08 | cas terminal $k=n$ | oubli de l'unique naissance $X$ ou invention d'une selle de rang $n+1$ |
| CE-M1-09 | deux composantes d'ordre $k\geq2$ partageant des identifiants d'observations | conversion abusive de la couverture en partition |
| CE-M1-10 | lot commun à deux ordres où l'ancre ne cible correctement que l'état post-lot | flèche verticale prématurée ou carré non commutatif |
| CE-M1-11 | shell de plus de quatre points ou plateau de successeurs | publication `exact` hors du domaine H2 |
| CE-M1-12 | suppression volontaire d'un événement, d'un bras ou d'une attache | booléen de complétude vrai malgré un artefact manquant |

Chaque désaccord avec l'oracle doit être minimisé par suppression de points et réduction des coordonnées. S'il contredit l'énoncé candidat plutôt qu'une hypothèse ou l'implémentation, le cas devient une fixture permanente, l'optimisation est suspendue et le registre des preuves est mis à jour avant toute poursuite.

## 10. Conditions de statut public

Le statut scientifique du contrat et le statut d'un run sont distincts.

### 10.1 Avant la fermeture de M.1

- `hgp_reduced` peut publier `public_status=exact` sur son propre périmètre si ses théorèmes et certificats sont satisfaits; il n'invoque pas M.1 et utilise `proof_basis=reduced_manuscript_theorem_5`.
- `full_pi0` ne peut pas publier `public_status=exact`, même si H1–H8 passent expérimentalement.
- un run `full_pi0` complet au regard de ses artefacts reste `public_status=conditional` avec `reconstruction_contract_id=M1-reconstruction-v1` et `proof_basis=m1_conditional_contract`;
- `forest_semantics=exact` est interdit; `forest_semantics=partial_refinement` n'est permis que si sa garantie unilatérale est établie indépendamment, sinon le champ est absent;
- `require_exact=true` doit échouer explicitement pour `full_pi0`;
- un budget atteint produit `budget_exhausted`; une dégénérescence hors H2 produit `unsupported_degeneracy`; un prédicat indécidable produit `numeric_failure`.

### 10.2 Après une éventuelle fermeture en phase 12

`full_pi0` ne pourra publier `public_status=exact` que si toutes les conditions suivantes sont réunies :

1. la preuve relue couvre exactement la version de contrat déclarée;
2. `proof_basis=full_pi0_proved` identifie la base scientifique promue et liée à cette preuve;
3. H1–H8 sont certifiées pour le run;
4. tous les champs de complétude de la section 7 sont vrais sur leur périmètre entier;
5. le replay des artefacts ne révèle ni incohérence ni objet manquant;
6. le mode est compatible avec une fermeture complète et aucun budget n'a tronqué la sortie;
7. l'oracle exhaustif et les tests de permutation des lots ne révèlent aucune différence.

Une bonne qualité de clustering, une concordance moyenne, une stabilité empirique ou un grand nombre de graines sans défaut ne ferme aucune de ces conditions.

## 11. Critères de falsification et de promotion

Le candidat M.1 est falsifié si une entrée satisfaisant réellement H1–H8 produit une coupe ou une application verticale non isomorphe à l'oracle exact. Il n'est pas falsifié par un cas qui viole explicitement H2, par un catalogue incomplet signalé, ou par une attache dont le certificat échoue.

La promotion de `proof_obligation` exige conjointement :

- une preuve couvrant O1–O9;
- une revue explicite de chaque usage d'un théorème externe;
- les fixtures CE-M1-01 à CE-M1-12 ou une justification documentée de leur impossibilité;
- l'égalité avec $\Gamma_k$ à chaque intervalle critique sur le domaine exhaustif;
- l'invariance sous toute permutation des événements d'un lot;
- la commutation de toutes les applications verticales testables;
- une mise à jour cohérente du registre des preuves et du champ `proof_basis`.

Jusqu'à cette promotion, ce document sert de contrat d'interface entre catalogue, attaches, lots, forêts, verticalité et certificat, sans constituer un résultat mathématique acquis.
