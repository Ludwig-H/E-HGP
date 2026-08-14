# Contre-audit du raffinement local et de la session G4

> **Statut historique.** Ce document audite la recette avant son exécution. La
> session a depuis été lancée par Claude, arrêtée avec cible `TERMINATED`, puis
> contre-auditée dans
> [`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin et verdict

Le pin relu est `HEAD=3c11bc8f99dd5f43eeaa973d61157ac2ae58e74e`.
Les objets concernés portent les SHA-256 suivants :

- `prototype/wspd_wavefront_probe.cpp` :
  `cfddfc89222a9179086f99b247abf933cc24f2d22f2d2422099b86aebad8ad74` ;
- binaire Release local :
  `fe9cffb97c321f04ccf2f3bdfc1e1d9582a2abd7eebde167736a7a48c769417f` ;
- `gcp-migration/session_fenetre_raffinement_g4.sh` :
  `4bfa754ab9d8f3471b5ed604fb33fc2b93765fe6461e16d99060fb002aa7e612`.

Verdict : **le raffinement local est un levier réel sur la masse résiduelle
du certificateur central, mais le claim « il paie » n'est pas encore reçu.**
À `n=3000`, il réduit fortement `sum_E4`, tout en multipliant le front et les
recertifications. Aucun `M4`, aucun BallRun, aucun census, aucun fold et aucun
temps device ne permettent encore de décider le coût transitif.

La recette G4 du pin ne doit pas être lancée telle quelle : elle contient
encore deux trous de fermeture de statut, ne calcule aucune pente `E4` entre
ses quatre processus et supprime de ses logs les métriques physiques qui
doivent arbitrer le raffinement.

L'auditeur n'a modifié aucun logiciel et n'a pas utilisé GCP.

## 1. Mesure reproductible

Commandes locales, une exécution par configuration :

```text
./build/v3/mhgp3v_wspd_wavefront_probe --family=F --points=3000 \
  --sep-euclid=8/1 --tight --vwave --window=512 --window-ledger \
  --max-slope=100 [--raffine=R --raffine-lane=2]
```

Les compteurs déterministes observés sont :

| famille | profondeur | terminaux | recertifications | masse q4 ouverte | part de `C(n,2)` | `max_E4` |
|---|---:|---:|---:|---:|---:|---:|
| `eight_clusters` | 0 | 363 018 | 31 538 327 | 4 045 644 | 89,933 % | 2 912 |
| `eight_clusters` | 2 | 701 181 | 96 762 366 | 3 346 789 | 74,398 % | 2 739 |
| `eight_clusters` | 4 | 1 182 988 | 199 169 436 | 2 597 699 | 57,746 % | 2 315 |
| `uniform` | 0 | 885 188 | 108 858 186 | 1 027 538 | 22,842 % | 1 337 |
| `uniform` | 2 | 1 195 651 | 185 986 958 | 501 262 | 11,143 % | 581 |
| `uniform` | 4 | 1 221 936 | 193 020 841 | 464 599 | 10,328 % | 439 |

Dans les six cas, le ledger terminal imprime `pending=0` et
`fenetre_finale=OUI`. Les masses `CLOSED/OPEN/PENDING` sont exclusives et leur
somme vaut `C(n,2)`.

La lecture honnête des ratios structurels à profondeur quatre est :

- `eight_clusters` : `E4` est réduit d'un facteur `1,56`, mais le front est
  multiplié par `3,26` et les recertifications par `6,32` ;
- `uniform` : `E4` est réduit d'un facteur `2,21`, mais le front est multiplié
  par `1,38` et les recertifications par `1,77` ;
- les temps CPU d'une exécution varient fortement et ne sont ni un p95, ni un
  modèle du G4, ni un temps du pipeline complet.

Le raffinement déplace donc du travail de l'aval vers le certificateur. Il
« paie » seulement si la baisse de `M4`, des intersections shallow, des
BallKeys et des census dépasse ce surcoût dans le même chronomètre.

## 2. Ce qui est mathématiquement sûr

Remplacer un terminal `A x B` par `(A_0 x B) union (A_1 x B)`, ou la version
symétrique, conserve exactement la partition des paires. Une fermeture
universelle reçue sur le parent reste vraie sur chaque sous-produit. À
l'inverse, un parent non certifié peut avoir des enfants certifiables :
rétrécir les boîtes améliore les marges du certificateur sans inventer de
fermeture.

Le code suit bien l'ordre sûr pour la lane cible : certifier le parent, le
conserver s'il est fermé, sinon le scinder jusqu'au budget. La fenêtre terminale
reste donc un surensemble fail-open de la vraie relation q4, jamais une
sous-estimation.

Ce résultat porte seulement sur le certificateur branché, actuellement le
masque central et ses options. `fenetre_finale=OUI` signifie « aucune tentative
terminale tronquée » ; il ne signifie ni fenêtre Morse minimale, ni PWC
projectif complet, ni source de supports.

## 3. Deux ledgers sont encore mélangés

`certifier` incrémente `bank.closed`, `pending_lane` et `mass_closed_q2` à
chaque tentative, avant de savoir si le rectangle deviendra un terminal. Un
parent ouvert en q4 mais déjà fermé en q2/q3 est ensuite scindé, et ses enfants
créditent à nouveau ces lanes.

La reproduction `eight_clusters`, profondeur quatre, imprime :

```text
masse fermee q2=380.15%
records fermes q2=157.74%
```

alors que le ledger terminal exact imprime :

```text
q2 : fermee=4406284 pendante=0 ouverte=92216
```

soit `97,950 %` de la masse. Les valeurs supérieures à 100 % ne sont pas une
faute géométrique de la fenêtre q4 : elles prouvent que les compteurs de tête
sont des **événements de certification cumulés sur l'arbre de raffinement**,
pas des fermetures terminales.

Même distinction pour `pending_lane` : une tentative parent abandonnée reste
comptée après consommation de ses enfants. Le test final utilise ce compteur
cumulatif ; il peut donc annoncer `fenetre_finale=NON` alors que le ledger des
terminaux n'a plus aucun pending. Cette erreur est conservative, mais le statut
ne décrit pas l'objet publié.

Réparation documentaire et logicielle à demander à Claude :

```text
AttemptStats = lectures, classifications, fermetures_tentées, pending_tentés
TerminalLedger = CLOSED, OPEN, PENDING, masses, E_q
```

Seul `TerminalLedger.pending_mass==0` décide la finalité. Les anciens champs
`ferme q*` et `masse fermee q2` doivent être renommés ou supprimés du reçu.

## 4. Le raffinement qui évite de repartir de la racine

Le coût principal vient de la recertification complète de chaque enfant. Or le
classement d'un `CNode` témoin est monotone par restriction de `A x B` :

- un `ALL` du parent reste `ALL` pour chaque enfant ;
- un `NONE` du parent reste `NONE` pour chaque enfant ;
- seul un `MIXED` doit être reclassé sur les boîtes rétrécies.

Le jalon recommandé est `ProofCarryingLocalRefinement-v0`. Chaque tentative
ouverte rend, par lane :

```text
credit_spans : antichaîne de CNode disjoints classés ALL, avec PointId/digest
none_spans   : CNode disjoints définitivement élagués
frontier     : tâches (CNode,lane_mask) encore MIXED ou non visitées
credit_count : somme exacte des populations de credit_spans
```

Lors d'une scission de `A` ou `B`, les deux enfants héritent les spans et le
compte. Ils rejouent seulement `frontier`; un `MIXED` devenu `ALL/NONE` quitte
la frontière. Une tâche non consommée devient une continuation sérialisée,
jamais une absence.

Cette forme apporte trois gains à vérifier :

1. aucune relecture des preuves déjà acquises ;
2. identités distinctes garanties par les CNodes disjoints, sans rescanner les
   `PointId` ;
3. même représentation count--scan--fill et SoA pour CPU, GPU et reprise.

Les portes comparent au chemin qui repart de la racine sur petit `n`, sous
permutation, profondeurs `0..R`, caps exacts et moins un. Elles exigent les
mêmes fates terminaux, mêmes `E_q`, `planned=filled=consumed`, zéro span
chevauchant et zéro crédit réutilisé.

La politique de split actuelle choisit seulement le côté le plus large. Une
ablation suivante peut tester les deux enfants candidats par une borne de
marge et choisir déterministement le meilleur rapport
`masse_non_résolue_évitée / nouvelles_tâches`. Un budget épuisé laisse des
terminaux `PENDING`; il ne les supprime jamais.

## 5. Pourquoi `E4` seul ne décide pas l'étape 1

Le texte du probe dit que `sum_E4` et `max_E4` « décident l'architecture ».
Ils sont nécessaires mais insuffisants. Le coût total contient au moins :

```text
front + certification + ProofSpan/frontier + M4 + shallow J/H
      + BallKey/RLE + census + fold + payload
```

Le gate de raffinement publie donc, pour chaque profondeur :

- terminaux et masse par fate/lane ;
- tentatives, splits, profondeur, tâches héritées/rejouées et HWM ;
- lectures nouvelles et évitées par héritage ;
- `E4`, `M4`, formes, intersections visitées, BallKeys brutes/uniques et `H` ;
- octets, HWM et temps par phase, puis temps aval sur exactement les mêmes
  événements.

Une pente rouge peut réfuter une profondeur/configuration. Une pente verte de
`E4` ne reçoit pas le pipeline tant que les autres masses ne sont pas closes.

## 6. `SOC64`, `CORNER512`, LP et cages restent des alternatives

Le commentaire « la seule voie qui reste » est faux. Le diagnostic négatif de
`JungSpindleRect-v0` porte sur une combinaison d'extrema décorrélés. Il ne
réfute pas :

- `SOC64`, qui teste les 64 couples de coins du produit de différences relaxé ;
- `CORNER512`, exact pour `ALL` sur l'enveloppe AABB continue ;
- les crédits LP projectifs, dont le témoin peut varier avec le centre ;
- les cages positives de quatre à six sites et leurs fleurs ;
- le raffinement après l'un de ces certificateurs plutôt qu'après le masque
  central seul.

La bonne expérience croise `certificateur x profondeur` et compare le coût
transitif. Le raffinement est une ordonnance, pas une preuve que le masque
central est l'unique source viable.

## 7. La session G4 n'est pas fail-closed au pin

Le script utilise correctement les scripts gardés, `SPOT`, une durée GCE de
3600 secondes, un arrêt invité de 45 minutes et un arrêt ciblé par génération.
Ces points ne suffisent pas encore à autoriser son exécution.

### 7.1 Fenêtre sans trap après le démarrage

Le trap interne de `start_and_verify.sh` couvre correctement ses propres
échecs avant certification. En revanche, après son retour réussi, la VM est
déjà démarrée et rendue à l'orchestrateur. Celui-ci parse encore
`handoff.json` avant d'installer son propre trap. Si ce parsing échoue, aucun
trap de session n'appelle `stop_and_verify.sh` ; seuls les deux coupe-circuits
temporels subsistent. Il faut armer le trap **avant** le démarrage avec un état
`started=false`, puis reprendre le handoff sans aucune commande faillible hors
garde. Si la génération reste illisible, l'échec doit indiquer précisément la
cible et la commande de contrôle.

### 7.2 Les codes peuvent encore disparaître

La commande distante commence par `set -euo pipefail`. Les sous-shells de la
rampe héritent de `errexit`; aucun `set +e` n'est exécuté. Si `timeout` ou le
probe rend non-zéro, le pipeline `timeout | grep | sed` est non-zéro sous
`pipefail` et le sous-shell sort avant `echo code=${PIPESTATUS[0]}`. La rampe
reste alors fail-closed par accident, mais elle perd le code promis et les
tailles suivantes ; elle ne fournit pas le diagnostic complet annoncé.

Il faut entourer chaque pipeline par : désactivation locale de `errexit`,
capture immédiate de tout `PIPESTATUS`, restauration, écriture atomique du
statut, puis agrégation explicite de tous les jobs. La phase finale doit exiger
quatre codes **et** la politique admise sur leurs valeurs, pas seulement quatre
lignes `code=`.

### 7.3 Aucune pente n'est calculée

Chaque taille est lancée dans un nouveau processus avec un unique `--points=n`.
Le probe ne voit donc jamais deux tailles et n'imprime aucune pente. L'option
`--max-slope=9` est vacue pour cette question et `--max-slope-e4` n'est pas
armée. Le script ne recalcule pas non plus les pentes depuis les quatre fichiers.

La réparation la plus simple est un analyseur local exact qui lit les quatre
`sum_E4/max_E4`, vérifie les tailles et les finalités, calcule chaque pente
adjacente et rend non-zéro au-delà du seuil. Un run multi-taille dans le même
processus reste utile comme second juge, pas comme unique provenance.

Le filtre n'inclut pas `fenetre_finale`. Or le probe peut rendre zéro avec des
continuations pendantes. La campagne doit conserver puis exiger
`fenetre_finale=OUI`, `pending=0` et `mass_pending=0` sur chaque lane ; quatre
codes zéro ne suffisent pas à transformer un surensemble en fenêtre finale.

### 7.4 Les métriques décisives sont supprimées

Le filtre `sed "s/ | arbre.*//"` coupe la ligne principale avant le temps de
vague, les lectures, recertifications, splits, HWM et compteurs de banque. Il
conserve `fenetre q*`, mais empêche précisément de mesurer le prix du gain.
Le reçu brut doit rester intact ; un résumé dérivé peut être produit en plus,
jamais à sa place.

Enfin, dix jobs CPU concurrents sur une machine G4 ne mesurent aucun kernel et
peuvent saturer mémoire/bande passante de façon non comparable. Cette campagne
est un diagnostic CPU de pente, pas un benchmark G4 ni une étape vers le p95
officiel.

Chaque job autorise en outre quatre timeouts de 1500 secondes, alors que
l'arrêt invité est armé à 45 minutes et le GCE à 60 minutes. La recette ne
garantit donc pas la publication des quatre tailles. Le journal n'est copié
vers `receipts/` qu'en fin de succès et la copie est suivie de `|| true` : un
échec de campagne ou d'archivage peut perdre sa preuve locale. Le trap doit
archiver le log sur succès **et** échec, et une copie refusée doit rester rouge.

Enfin, un worktree dirty est empaqueté tel quel, mais seule sa quantité de
fichiers est publiée. Le reçu doit conserver chemins, diff binaire ou patch,
hashes source/CMake, tar et ELF ; le hash d'un tar qui n'est ensuite pas archivé
ne permet pas de reconstruire le sujet.

## 8. Décision transmise à Claude

1. Conserver le raffinement local comme ablation prometteuse.
2. Séparer immédiatement statistiques de tentatives et ledger terminal.
3. Implémenter `ProofCarryingLocalRefinement-v0` avant d'augmenter la profondeur.
4. Brancher `SOC64/CORNER512` ou LP/cages dans la même expérience, sans claim
   d'unicité du masque central.
5. Mesurer `E4 -> M4 -> H -> BallRuns -> fold`, pas `E4` seul.
6. Corriger les quatre défauts de session avant toute utilisation de GCP.

Le contrat `50000`, G4, p95 sous une seconde reste ouvert.

GCP non utilisé.
