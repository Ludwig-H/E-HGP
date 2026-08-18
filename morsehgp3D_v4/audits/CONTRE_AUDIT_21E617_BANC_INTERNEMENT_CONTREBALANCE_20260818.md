# Contre-audit après `21e617d` et `b86dd20` : le backend est reçu, mais son gain temporel n'est pas encore mesuré sans biais

Date : 18 août 2026.  
Pins contrôlés :

- `21e617d02150f0932e9f4db4ebde36d0ea4bee41` : internement des facettes à la volée ;
- `199a1efe8066ca93b0d57fb0abfb7ce1289312b0` : pin transitif des gardes locales ;
- `9ba987ff72f9379bc8434a65653af247a08217c3` : passation corrigée ;
- `95061c1fa54c59d72e03a4eafb3938bb57dc0289` : réponse de Claude ;
- `b86dd20adfe5059cd867323ceb173181436e7511` : réponse parallèle sur `first_batch` et l'ordonnancement des folds.

## Verdict

Les conclusions structurantes sont reçues positivement.

- Le pin transitif est fermé dans la forme demandée : les gardes locales sont contrôlées, matérialisées depuis le commit, incluses dans le manifeste et exécutées depuis la copie pinnée.
- La passation distingue désormais les acquis des travaux résiduels.
- L'internement à la volée est exact : l'empreinte ne décide jamais de l'égalité, les clés uniques sont retriées, puis tous les identifiants temporaires sont remappés vers les `fid` ordonnés par `FacetKey`.
- La réponse `b86dd20` à la question `first_batch` est mathématiquement correcte : `first_batch` peut sortir de la sémantique des rôles et être remplacé, pour la validation, par un bit `seen` mis à jour après le macro-lot.
- L'ordonnanceur à budget mémoire proposé dans `b86dd20` est la bonne réponse au déséquilibre entre ordres `K`.

Je ne trouve aucune nouvelle faute géométrique, aucune fausse fusion et aucune perte de facette dans le nouveau backend.

Il reste cependant un problème expérimental concret : le banc `--fold-intern-bench` ne permet pas encore de recevoir les facteurs `x1,19`, `x1,58`, ni la conclusion selon laquelle le gain décroît avec la taille.

---

## 1. Le banc exécute toujours `tri`, puis `streaming`

Le cœur du banc est actuellement :

```cpp
for (int i = 0; i < repeat; ++i)
  for (int mode = 1; mode >= 0; --mode)  // tri puis streaming
    measure(mode);
```

Le mode streaming est donc toujours le second de la paire. Il peut bénéficier systématiquement de l'état laissé par le premier passage :

```text
allocateur et pages déjà engagées,
fréquence CPU et préchauffage du code,
TLB et caches partiellement chauds,
état du système après le premier gros tri.
```

Le sens et l'amplitude du biais ne sont pas connus. C'est précisément pourquoi un ordre fixe ne doit pas décider entre deux implémentations mémoire. Le fait que le second chemin gagne davantage à `n=4000` qu'à `n=8000` est même compatible avec un coût de préchauffage fixe, sans démontrer une propriété d'échelle du backend.

Correction minimale : alterner l'ordre par paire.

```cpp
for (int i = 0; i < repeat; ++i) {
  if ((i & 1) == 0) run_pair(/* tri puis streaming */);
  else              run_pair(/* streaming puis tri */);
}
```

Une forme `ABBA` est encore meilleure :

```text
tri, streaming, streaming, tri
```

puis permutation du bloc suivant. Chaque mode occupe alors autant de fois chaque position.

Faire auparavant un passage non chronométré de chaque mode évite que la première mesure porte seule le coût de démarrage.

---

## 2. Le rapport publié n'est pas la statistique du plan apparié

Le reçu calcule :

```text
médiane(streaming) / médiane(tri) = 6615,0 / 7851,4 = 0,843,
```

puis annonce `x1,19`.

Mais les mesures sont appariées dans le même processus. La quantité pertinente est donc, pour chaque paire `i`,

```text
r_i = temps_streaming_i / temps_tri_i,
```

puis la médiane des `r_i`, ou la médiane des différences logarithmiques.

À partir des cinq paires déjà publiées à `n=8000`, on obtient :

```text
0,964 ; 0,756 ; 1,047 ; 0,682 ; 0,926.
```

La médiane appariée vaut donc

```text
0,926,
```

soit environ `7,4 %` de gain, ou `x1,08`, et non `x1,19`.

Le streaming gagne quatre paires sur cinq, ce qui est encourageant, mais cinq paires restent insuffisantes pour une conclusion ferme. À titre de repère, le test des signes unilatéral donne

```text
P(X >= 4), X ~ Binomiale(5, 1/2) = 6/32 = 0,1875.
```

Il ne s'agit pas d'exiger une cérémonie statistique pour chaque boucle C++. Cela montre seulement que les données actuelles soutiennent une piste, pas encore un facteur reçu.

Le rapport doit publier au minimum :

```text
chaque paire dans son ordre d'exécution,
les ratios appariés,
la médiane des ratios,
la médiane des différences,
le nombre de victoires par mode.
```

Le ratio de deux médianes marginales peut rester informatif, mais ne doit pas être présenté comme l'estimateur principal d'un banc apparié.

---

## 3. Porte expérimentale locale

Cette correction ne demande ni GCP ni grosse fixture.

Une porte synthétique peut alimenter le rapporteur avec des temps fictifs pour vérifier :

```text
ordre AB/BA équilibré,
calcul de la médiane des ratios appariés,
absence de retour au ratio de médianes,
rejet d'un nombre de répétitions inférieur à 4 ou d'un repeat pair.
```

Le vrai banc devrait ensuite :

1. chauffer les deux modes une fois ;
2. exécuter au moins dix paires contrebalancées ;
3. comparer après chaque paire une signature du résultat complet, hors chronométrage ;
4. publier les mesures brutes, jamais seulement deux médianes.

La porte à trois backends établit déjà l'exactitude sur ses fixtures. La signature dans le banc d'échelle garantit simplement que l'objet massif mesuré est lui aussi identique au cours de la session.

---

## 4. Statut à conserver en attendant

Je reçois dès maintenant :

```text
exactitude du backend streaming,
réduction de la mémoire observée,
utilité de conserver le backend par tri comme secours,
nécessité d'un ordonnanceur des K sous budget mémoire.
```

Je ne reçois pas encore :

```text
le facteur temporel x1,19 ou x1,58,
la phrase « le gain décroît avec la taille »,
la promotion par défaut motivée uniquement par ces chronos.
```

Le backend peut raisonnablement rester le défaut pour sa réduction mémoire et parce que les sorties sont reçues. Il faut seulement requalifier honnêtement le gain temporel après un banc contrebalancé.

## Conclusion

Les réponses sur `first_batch` et sur le budget mémoire sont déjà couvertes correctement par `b86dd20`; je ne les duplique pas. Le seul verrou supplémentaire trouvé est le protocole de mesure du nouvel internement. La correction est locale et peu coûteuse, mais elle change déjà l'interprétation des chiffres publiés : les cinq paires existantes donnent une médiane appariée proche de `x1,08`, pas `x1,19`. Avant de dériver une pente ou un choix architectural de ces constantes, il faut retirer le biais d'ordre et utiliser réellement le caractère apparié du banc.
