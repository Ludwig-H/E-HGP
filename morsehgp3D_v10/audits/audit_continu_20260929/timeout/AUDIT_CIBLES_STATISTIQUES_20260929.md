# Audit : références oracle, ARI et portée des têtes multi-K

29 septembre 2026. Lecture des sources et des rapports, sans nouveau calcul de
clustering, sans modification du moteur et sans requalification des scores.
Les résultats empiriques des rapports ne sont pas rejetés ; certaines
interprétations mathématiques doivent être restreintes.

## 1. Le « plafond Bayes » n'est pas une borne d'ARI

Dans `receipts/bench_dev_alloc_20260929/ceiling_alloc.py:23–34`, pour chaque classe
de vérité terrain, les paramètres sont estimés sur les points à évaluer :

```python
X = P[L == g]
mu = X.mean(axis=0)
C = np.cov(X.T) + 1e-9 * np.eye(3)
```

La décision MAP utilise ensuite ces moyennes, covariances et effectifs estimés.
Il s'agit donc d'une **référence supervisée de diagnostic**, pas du classifieur
bayésien construit avec les paramètres exacts du générateur. L'accès aux labels
est explicite et légitime pour ce diagnostic, mais ne doit pas être attribué aux
têtes testées.

Même avec les vrais paramètres, le choix MAP point par point maximise la
probabilité de classification correcte sous la perte 0–1. L'ARI compare des
paires et corrige par les effectifs de la partition ; `ARI_s` traite en plus
les points déclarés bruit comme des singletons. Ce n'est pas cette perte 0–1.
On ne peut donc pas déduire que le score ARI d'une partition MAP borne celui de
toute méthode de clustering. Le nombre 0,895 reste un score mesuré de référence,
pas un plafond démontré.

**Correction conseillée :** employer « référence MAP avec paramètres estimés sur
les labels », conserver le score et les écarts, et retirer les formulations de
borne ou de fraction d'un optimum accessible.

## 2. Le « Morse de la vraie densité » est un surrogate discret

`receipts/bench_dev_alloc_20260929/morse_ceiling.py:28–47` réestime les mêmes
paramètres sur les classes de l'échantillon. Aux lignes 60–74, le calcul est :

```python
_, nbr = tree.query(X, k=k)  # k = 20, puis 40
best = np.lexsort((nbr, -logf[nbr]), axis=1)[:, 0]
parent = nbr[np.arange(len(X)), best]
# suivi des parents jusqu'aux racines
lab = groups[np.argmax(logp[:, root], axis=0)]
```

C'est un graphe discret de montée sur 20 ou 40 voisins, évalué avec une densité
ajustée sur la vérité terrain. Ce n'est ni l'intégration du flot du gradient de
la densité génératrice exacte ni une preuve que tous ses bassins continus sont
retrouvés. Deux racines discrètes peuvent recevoir la même classe MAP : cette
dernière étape utilise encore les composantes latentes et n'est pas une simple
numérotation distincte des bassins du graphe.

La comparaison 20/40 est un contrôle de sensibilité utile ; elle ne prouve pas
la convergence vers les bassins continus. Le score 0,847 doit rester associé
exactement à ce surrogate.

**Correction conseillée :** « référence de montée discrète sur densité ajustée
avec les labels, voisinages 20/40, racines étiquetées par composante MAP ».
Pour un véritable diagnostic utilisant la densité génératrice, lire directement
les paramètres connus du générateur et publier séparément les approximations
numériques du flot. Ce chantier n'est pas nécessaire pour conserver les mesures
de clustering existantes.

## 3. L'écart 0,048 n'est pas une impossibilité démontrée

`receipts/bench_dev_alloc_20260929/README.md`, section « Décomposition », affirme
que Bayes − Morse = 0,048 ne peut être récupéré par aucune méthode fondée sur les
niveaux de densité. `PASSATION.md` reprend cette affirmation.

Elle ne suit pas du code : les deux références ci-dessus sont des procédures
précises évaluées par ARI_s, pas des optima sur des classes de méthodes. Une
hiérarchie de composantes de sur-niveaux ne prescrit pas à elle seule l'affectation
de toute la masse située sous les cols. Choisir un remplissage, une abstention ou
une autre règle sur cette masse peut changer l'ARI sans changer l'arbre des
sur-niveaux. Le rapport reconnaît d'ailleurs que la tour dépasse la référence
Morse dans certaines cellules (`unbalanced` et `heteroscedastic`).

**Correction conseillée :** conserver la décomposition arithmétique comme
diagnostic comparant trois procédures ; retirer « hors de portée de toute
méthode de densité ». Identifier les modes et identifier les composantes latentes
d'un mélange sont deux objectifs distincts, mais leur écart d'ARI n'est pas une
constante universelle ni une borne issue des seuls arbres.

## 4. Multi-K : appariement empirique, stabilité et sélection sont distincts

`audits/tete_multik_20260929/ANTICHAINE_ORDRES_MELES_20260929.md:120–126`
construit des inclusions à 10 % près ; les minima d'inclusion observés sont 0,957
et 0,933, pas 1. Aux lignes 217–219, un point contesté est attribué après la
sélection à un des amas choisis. Cela peut produire une partition plate valable,
mais ne fournit pas une preuve de partitions emboîtées quand on fait varier un
niveau ou le réglage de sélection.

Même une antichaîne pour l'inclusion **exacte** n'assure pas la disjonction :
les ensembles `{1,2}` et `{2,3}` sont incomparables et se recouvrent. L'inclusion
approchée n'est en outre pas transitive. Les verticales exactes des composantes
spatiales de FULL ne transforment pas automatiquement ces amas condensés,
appariés à leurs propres niveaux, en une seule famille laminaire de points.

Deux constructions sûres pour l'emboîtement, à distinguer des gains empiriques :

- conserver un arbre de référence de points et utiliser les autres K seulement
  pour scorer, contracter ou sélectionner ses branches ;
- choisir une trajectoire monotone dans FULL, rayon croissant et K décroissant,
  et suivre ses inclusions exactes. Les régions croissent alors par inclusion.

Dans les deux cas, chaque point doit avoir une attache fixée une fois pour toutes,
puis suivre les parents. S'il n'est pas encore entré, il reste singleton pour
obtenir une partition de **tous** les points. Une nouvelle affectation dépendant
de chaque coupe peut casser cet emboîtement. Ces principes suffisent à la
laminarité structurelle ; ils ne prouvent ni meilleure qualité, ni consistance
statistique, ni robustesse de la sélection EOM.

## 5. Recommandation statistique proportionnée aux preuves

Le `RAPPORT_JUGE.md` donne un contrôle dev sur la réplique 1 : à 8k avec
remplissage, `mixq` gagne environ 0,0046, l'oblique 0,0016 et `eom2w` 0,0008.
Ces résultats rapportés ne sont pas recalculés par le présent audit. Le juge
précise les réglages post hoc et l'absence de mesure multi-K à 16k/32k.
Ils ne motivent pas une refonte de FULL ni une promesse de gain 0,02.

Les rapports allocation et shrink sont explicitement dev. Leur intérêt est
diagnostique : le remplissage et la sélection produisent des compromis entre
familles, par exemple feuilles favorables à l'anisotropie mais mauvaises sur
les coquilles. Un sélecteur adaptatif par scène est une nouvelle méthode ; ses
règles et son coût doivent être figés avant de nouvelles graines de confirmation.
La réplique déjà inspectée pour choisir ce sélecteur n'est plus une validation
neuve de la nouvelle règle.

Priorité : préserver un squelette de points laminaire et explicitement défini,
distinguer l'affectation des points de la sélection des branches, puis publier
qualité, couverture et stabilité sous perturbations avec protocoles séparés.
Ni l'exactitude de FULL ni la stabilité d'un objet bifiltré ne se transmettent
sans preuve à un argmax EOM ou à une règle de remplissage.
