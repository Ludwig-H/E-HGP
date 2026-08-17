# Contre-audit de `0328bf5` — la barrière de sortie est déjà visible rigoureusement dans la seule lane q2

Date : 17 août 2026.  
Pin de code reçu : `5107f4e53f8c027f779c3f2aa6ddc8d1400c8a28` (protocole G4), `2b7bb3299e1a75f0fe9dd3bc5fdfff96e186fb57` (fold `sort/reduce`).  
Audit contre-audité : `AUDIT_BLOQUANT_C829_TAILLE_SORTIE_30M_20260817.md`, commit `0328bf5da4f7ace9d991b80b15cd64a0466154e5`.

## Verdict

Je reçois les derniers développements :

- le fold `sort/reduce` conserve correctement les macro-lots, les rôles, les facettes nées, les `ComponentDelta` et le rendu ;
- le pin de protocole G4 rattache désormais moteur, runner et validateur au même commit et au même digest ;
- le sweep axial à deux côtés proposé par le contre-audit parallèle est exact, mais reste secondaire devant le problème désormais identifié.

Je confirme le verdict de `0328bf5` : le contrat

```text
forêt complète K=1..10 + événements/rendu explicites + n=30M + <100 ms
```

n'est pas compatible avec la matérialisation résidente actuelle. L'extrapolation depuis `n=8000` est déjà suffisante comme décision d'ingénierie. On peut en outre la renforcer par un calcul asymptotique exact sous le modèle Poisson homogène : **la seule lane q2 produit en espérance 40 événements par point pour `K_max=10`, avec 4 événements par point à chaque profondeur.**

---

## 1. Théorème Poisson q2 par profondeur

Considérons un processus de Poisson homogène d'intensité `lambda` dans une suite régulière de fenêtres tridimensionnelles dont le volume tend vers l'infini. Pour une paire non ordonnée `{a,b}`, notons `r=|a-b|`. Sa boule diamétrale a pour volume

```text
v2 r^3, avec v2 = pi/6.
```

Soit `N_j` le nombre de paires dont la boule diamétrale ouverte contient exactement `j` autres sites. Hors bord, Campbell-Mecke donne

```text
E[N_j]
 = lambda^2 |Omega| / 2
   * integral_{R^3} exp(-lambda v2 |x|^3)
     (lambda v2 |x|^3)^j / j! dx.
```

En coordonnées radiales, puis avec `t=lambda v2 r^3`, on utilise

```text
4 pi r^2 dr = 4 pi /(3 lambda v2) dt
```

et

```text
integral_0^infty exp(-t) t^j/j! dt = 1.
```

Par conséquent

```text
E[N_j] / E[n] -> 2 pi /(3 v2) = 4
```

pour **chaque** `j >= 0`.

Ce résultat est plus précis que la constante cumulée déjà dérivée dans les audits précédents :

```text
E[#{paires q2 de profondeur < h}] / E[n] -> 4h.
```

Les corrections de bord sont `o(n)` dans le régime à densité fixe et fenêtres dilatées utilisé comme modèle de la famille uniforme.

---

## 2. Chaque paire q2 donne un événement distinct

Sous position générale, une paire comptée dans `N_j` possède :

```text
support minimal S={a,b},
I_B = les j sites strictement intérieurs,
sigma = S union I_B,
K = j+1.
```

La miniboule de `sigma` reste la boule diamétrale de `a,b`, aucun site extérieur à `sigma` n'est intérieur, et le support q2 est unique. La correspondance événements-boules prouvée en Q1 donne donc **un événement q2 distinct** par paire.

Pour `K_max=10`, soit `j=0,...,9` :

```text
E[# événements q2] / E[n] -> 4 * 10 = 40.
```

Mieux, le nombre total d'identités de points apparaissant dans ces événements vaut

```text
sum_{j=0}^9 (j+2) N_j.
```

Donc

```text
E[# PointId dans le flux q2] / E[n]
 -> 4 * sum_{j=0}^9 (j+2)
 = 4 * 65
 = 260.
```

La profondeur moyenne conditionnelle d'un événement q2 utile est ainsi `4,5`; un événement contient en moyenne `6,5` identités.

Pour le profil secondaire `K_max=5`, le même calcul donne :

```text
20 événements q2 par point,
80 PointId q2 par point.
```

---

## 3. Conséquence à 30 millions de points

Au seul ordre q2 et pour `K_max=10`, le modèle homogène prévoit :

```text
~1,2 milliard d'événements q2,
~7,8 milliards d'occurrences de PointId.
```

Les seules identités u32 représentent déjà environ :

```text
7,8e9 * 4 octets = 31,2 Go.
```

La structure résidente actuelle est beaucoup plus coûteuse : `ForestEvent` réserve vingt `PointId` et un `Q4Level` pour chaque événement, soit au moins 124 octets de champs. La seule lane q2 demanderait alors au moins :

```text
1,2e9 * 124 octets = 148,8 Go,
```

avant les incidences, le DSU, les deltas, le rendu, q3 et q4.

Cette conclusion ne dépend donc pas de la pente empirique `391 événements/point` observée à `n=8000`. Cette pente rend la situation encore plus sévère, mais **q2 seule exclut déjà le `vector<ForestEvent>` résident comme produit 30M**.

Le calcul ne dit pas qu'une hiérarchie exacte est impossible. Il dit que l'objet exact ne peut pas être confondu avec sa liste explicite résidente, et que le SLO de 100 ms ne peut pas inclure la matérialisation du `full_symbolic_stream` courant.

---

## 4. Décision de produit à prendre avant CUDA

Je confirme la séparation proposée par `0328bf5` :

```text
full_symbolic_stream
  exact, output-sensitive, streaming/out-of-core ;

connectivity_hierarchy
  exact pour les composantes, sans incidences symboliques exhaustives ;

query_or_labels
  exact pour des K/niveaux/poids déclarés, seul candidat crédible au SLO.
```

Le champ `product` doit être contractuel. Le temps publié doit inclure ce que ce produit promet réellement.

Pour le cas SemanticKITTI ou segmentation, le produit utile est probablement `query_or_labels` ou une `connectivity_hierarchy` condensée, pas plusieurs milliards de certificats q2 explicitement sérialisés. Le flux symbolique complet reste précieux comme autorité et artefact scientifique, mais il doit être externe et proportionnel à sa taille.

---

## 5. Actions minimales utiles

1. Ajouter `--output-preflight-only` avec compteurs u64 par K : événements, incidences, facettes, deltas et octets projetés.
2. Promouvoir immédiatement `FRec::e`, `fid`, `first_batch`, `role_epoch` et les offsets du fold en u64, ou graver une porte qui refuse avant `2^32`.
3. Ajouter `product` et `max_output_bytes` au contrat transactionnel.
4. Confronter sur petit n :

   ```text
   chemin résident
   contre
   chemin streaming par K et macro-lot.
   ```

5. Pour `query_or_labels`, accumuler directement les quantités demandées et libérer chaque événement après consommation.
6. Pour `full_symbolic_stream`, produire des runs triés externes et un merge, jamais `ev_k[1..10]` simultanément en mémoire.

Le nouveau fold plat est une bonne base pour cette route : son internement et ses réductions peuvent être exécutés par partitions. Il faut maintenant éviter que le tri global reconstruise, sous une forme plus rapide, le même téraoctet que les `std::map` construisaient plus lentement.

## Conclusion

Le blocage de `0328bf5` est confirmé et renforcé : dans le modèle Poisson homogène, **q2 seule impose asymptotiquement 40 événements et 260 occurrences d'identité par point pour `K_max=10`**.

La prochaine décision n'est donc pas une nouvelle constante de filtre. C'est le produit exact que le backend doit rendre. Une fois ce contrat tranché, `BallKey`, `SpherePlateau` et le fold `sort/reduce` pourront être réutilisés sans ambiguïté dans une version streaming ou orientée requêtes.
