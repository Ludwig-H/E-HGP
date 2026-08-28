# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Derniers commits techniques relus :** `17ab71e0`, intégration des objections au plan d'échelle, et `ba31c169`, porte de préfixe étendue ; `a2597c1a` ne modifiait que le présent état d'audit.
- **Code en cours de relecture :** worktree postérieur à `ba31c169`, encore non committé. Il prépare la sûreté du fold, l'instrumentation device et une campagne `SCALE_THREADS`. Les observations sur ce code sont des indications de travail, pas une réception sur pin.
- **Pins de performance conservés :** `82f613d3` pour les campagnes CPU 50–200 k et `63deda74` pour les étapes device à 50 k.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Verdict utile à Claude

**Orange constructif : la trajectoire est bonne et les trois chantiers actifs méritent d'être terminés. Ne pas lancer encore la campagne G4 de scaling.**

Les commits `2074b2f0` et `17ab71e0` répondent utilement à l'audit de conception : ils abandonnent le fold tuilé et les halos, séparent le digest d'intégrité du convertisseur v4, choisissent un comptage externe exact des premières et dernières incidences, bornent la reprise initiale à `resume=replay_current_K` et font du préfixe K = 1..5 un premier échelon. Le cadre et la doctrine différentielle v5 sont maintenant corrigés. `ba31c169` ferme en plus le trou du digest v4 sur les événements et les niveaux de lots. Le document est désormais une architecture falsifiable avec des portes, ce qui est le bon statut à ce stade.

Le worktree courant corrige également plusieurs défauts concrets du fold : ownership du slot avant lancement, drainage explicite, arbitrage par ordre K, validation de `fold_inflight`, pic mesuré et tests à fautes injectées. L'instrumentation device supprime une synchronisation H2D intrusive et sépare enfin copies, kernels, attente et tailles de lots. Ce sont des progrès directement réutilisables.

Le plus petit jalon sûr consiste maintenant à fermer trois raccords, graver les tests correspondants, puis donner un pin propre à l'auditeur. Les autres questions ne doivent pas détourner Claude de ce jalon.

Deux solutions d'architecture sont désormais suffisamment précises pour guider
les commits suivants :

- le fold streamé peut éviter entièrement le reroot union-find en séparant
  `logical_root_fid` du stockage small-to-large des seules facettes encore
  vivantes ; l'invariant obtenu est `components <= live_aliases` ;
- le GPU peut réduire le trafic répété en téléversant la géométrie une fois et
  en envoyant un indice u32 par site, après mise en place d'un petit pool
  d'exécuteurs persistant.

Les algorithmes, preuves locales, wires et fixtures sont détaillés dans
[AUDIT_PASSAGE_ECHELLE_20260828.md](AUDIT_PASSAGE_ECHELLE_20260828.md) et
[AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md](AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md).

## P0 — trois raccords à fermer avant une campagne

### 1. Le validateur SCALE ne lit pas encore la sortie réelle du fold

Le programme imprime maintenant une ligne de la forme :

```text
temps_fold_mur_ms=... (..., fold_inflight=N, pic_mesure_en_vol=P)
```

Le validateur et ses deux faux producteurs attendent encore l'ancien texte `N ordre(s) en vol`. Une vraie exécution est donc refusée alors que les selftests passent.

Correction minimale : faire reconnaître le nouveau schéma, exiger `1 <= P <= N`, `P == 1` lorsque `N == 1`, puis alimenter les fixtures avec cette même forme. Pour une porte qui prétend exercer le chevauchement, ajouter un plancher `P > 1`; ne pas l'exiger sur une charge qui peut légitimement se sérialiser.

### 2. Le plan par défaut ne tient pas dans la session qu'il doit mesurer

Les axes par défaut produisent 192 runs. Avec `7200 s` par run, le budget théorique vaut 16 jours, alors que la session est bornée à quatre heures et l'arrêt invité à 230 minutes. Le runner n'a ni échéance globale, ni marge réservée au rapatriement, et continue après un premier run impossible. L'exemple de session huit heures conserve en outre une clé OS Login de 250 minutes, insuffisante pour la durée annoncée.

Réutiliser ici le mécanisme déjà présent dans `session_scale_threads_g4.sh` et le protocole v4 :

1. calculer et imprimer le budget depuis le runner épinglé avant toute mutation GCP ;
2. transmettre une `DEADLINE_EPOCH` et réserver une `RETRIEVE_MARGIN` ;
3. refuser un nouveau run s'il ne tient plus dans la fenêtre ;
4. arrêter la phase au premier statut non nul, tout en conservant ses artefacts ;
5. commencer par un pilote de 24 runs au plus.

Le renversement actuel des seuls niveaux de fils est utile pour mesurer le scaling des fils à l'intérieur de chaque strate. Il ne contrebalance pas encore `family`, `fold_inflight` ou `digest`. Soit borner explicitement le claim à cet effet, soit inverser la séquence factorielle entière et exiger un nombre pair de répétitions supérieur ou égal à deux. « Ordre miroir » est plus précis que « ABBA » pour huit niveaux.

### 3. L'observateur `kPublished` intervient après la publication irréversible

Dans le worktree courant, un ordre K avance `next_publish`, notifie et libère son état avant d'appeler `on_fold_phase(kPublished)`. Si cet observateur lève, un K supérieur peut déjà appeler `on_forest` et se déclarer publié. Le contrat annoncé — la faute de l'ordre observé gagne avant les publications supérieures — n'est alors pas respecté. Les nouveaux tests injectent une faute dans `on_forest`, mais pas dans l'observateur `kPublished`.

Correction minimale : ne faire avancer `next_publish` qu'après le retour réussi de l'observateur, ou rendre explicitement cet observateur non autoritatif. La première option correspond au contrat et aux tests existants. Ajouter un scénario déterministe où K = 2 lève sur `kPublished` pendant que K = 3 a fini sa réduction; K = 3 ne doit appeler ni `on_forest`, ni `kPublished`. Préserver aussi la première exception d'un ordre si `kReduceBegin` et `reduce_fold` lèvent tous deux.

Le fil B a par ailleurs perdu l'enveloppe `catch (...)` globale de la version précédente. Les calculs principaux sont protégés, mais la construction de `sp->message` après une violation alloue encore hors `try`; un `bad_alloc` quittant la fonction de thread appelle `std::terminate`. Stocker seulement le statut et K puis construire le message après la jointure, ou rétablir une enveloppe externe qui transforme toute exception en verdict ordonné du slot.

## P1 — durcir sans agrandir le chantier

### Domaine et résidence de `fold_inflight`

Le profil accepte actuellement jusqu'à 16, alors que K est borné à 10 et que le préflight mémoire reste individuel par ordre. Une valeur 16 peut donc rendre les dix états B résidents; aucun majorant agrégé de fenêtre ne le couvre. Tant que ce préflight n'existe pas, le domaine public le plus honnête est `1..3`, qui correspond au chevauchement effectivement forcé par la porte. L'ouverture au-delà doit venir avec une somme majorée par rôle, une porte de pic RSS et un mutant qui force tous les ordres de la fenêtre à rester résidents.

### Validation et transport du plan SCALE

Rejeter avant GCP les familles inconnues, les valeurs hors domaine, les répétitions impaires lorsqu'un ordre miroir complet est revendiqué, les axes dupliqués et un nombre total de runs excessif. Un axe dupliqué produit aujourd'hui deux noms identiques, réécrit le même dossier puis permet au validateur de relire deux fois le même fichier.

La commande SSH incorpore les valeurs `SCALE_*` entre apostrophes dans une chaîne distante. Une apostrophe ou un saut de ligne peut casser cette enveloppe avant la validation distante. Construire un tableau `env ...` dont chaque valeur est sérialisée avec `printf %q`, et remplacer le test statique qui exige aujourd'hui précisément la forme fragile par un test d'apostrophe et de saut de ligne.

### Sémantique de la télémétrie device

La décomposition H2D/kernels/D2H est le bon instrument. Trois libellés doivent toutefois rester honnêtes :

- des événements CUDA mesurés sur plusieurs streams incluent l'attente et la contention de planification entre ces événements; ce ne sont pas des coûts intrinsèques isolés des kernels ;
- `q4 issue_ms` englobe encore de grosses allocations ou initialisations hôte avant l'enfilement CUDA ; les sortir de ce champ ou créer une catégorie de préparation hôte ;
- `executor_ms_sum` commence après une partie du setup de `scan()` ; déplacer le départ ou documenter précisément l'exclusion.

Le compteur de concurrence ne doit pas être un singleton statique remis à zéro par chaque appel : deux pipelines concurrents se contamineraient. Le porter dans l'invocation et le passer aux lanes. Comme `kernel_ms` change de sens par rapport aux reçus antérieurs, versionner le schéma ou employer un nouveau nom, par exemple `kernel_events_ms_sum`.

## Questions actives qui ne bloquent pas ce jalon

- La réception de la grille cellulaire reste ouverte : corriger le théorème 10.5, comparer le compteur à une évaluation i128 directe et ajouter une frontière située juste du côté vivant. Un centre exactement sur une frontière touchant une cellule morte ne constitue pas un test anti-faux-kill valide.
- Les doubles constructions de grille et les deux autorités de seuil restent à unifier, mais cette correction peut suivre le pin fold/instrumentation.
- Les théorèmes T3–T6 de `docs/ECHELLE.md` restent des obligations de preuve, pas des résultats. L'audit de passage à l'échelle propose toutefois un réducteur sans parents historiques, une identité FIRST/LAST exacte et une porte de rejeu qui les transforment en petits jalons codables. Le terme `profil=prefixe_k5/complet_k10` doit éviter de collisionner avec le champ normatif `profile` ; `tower_scope` convient.
- Le débit de 290 Mio/s est provisionné par `deploy.sh`, pas mesuré. Une porte `fio` doit fournir le débit du disque effectivement attaché avant toute projection de durée.

## Passage de relais conseillé

1. Corriger les trois P0 ci-dessus et faire enregistrer les nouveaux tests dans CMake/CTest.
2. Exécuter localement la porte fold, les selftests du protocole et le validateur sur une sortie factice au format réel.
3. Committer un pin cohérent sans lancer GCP; l'auditeur rejouera alors le build CPU, les portes et les checks documentaires.
4. Une fois ce pin reçu, extraire la porte `catalogue + deltas -> final_canon_fid`, puis implémenter le réducteur vivant en mémoire avant toute externalisation.
5. En parallèle, remplacer les exécuteurs CUDA éphémères par un pool borné ; le wire par indices vient dans le commit suivant.
6. Lancer seulement alors le pilote SCALE borné. La campagne complète viendra après interprétation du pilote.

## État de validation

Les commits `2074b2f0` puis `17ab71e0` ont été relus comme décisions
documentaires ; `ba31c169` est la dernière porte fonctionnelle reçue. Dans un
worktree détaché propre à ce pin : configuration et build Release ciblé
réussis, puis 6/6 CTests `^mhgp5_prefix_` réussis en 24,49 s. Le code actif
décrit ci-dessus n'est pas encore committé et ses nouvelles portes ne sont pas
encore enregistrées dans CMake au snapshot observé. Aucun résultat CUDA ou GCP
nouveau n'est revendiqué.

Les validations antérieures restent bornées à leurs pins : suites CPU reçues à `369f3ac0`, campagne CPU 50–200 k à `82f613d3`, instrumentation device de la session G4 n° 12 à `63deda74`. Elles ne valident pas automatiquement le worktree courant.

GCP non utilisé pour cet audit.
