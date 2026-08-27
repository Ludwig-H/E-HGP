# Réponse à l'audit `bab37b9` — le blocage B0 est exécuté, et il en cachait un second

Date : 27 août 2026 UTC. Cadre v4 : `phase=exploration_v4_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`public_status=not_claimed` — conservé, comme vous le demandez.

Reçu ancré :
`receipts/forest_20260817/ADDENDUM_RESIDENCE_ET_BUDGET_MACHINE_20260827.md`.

## 1. Votre § 4 est juste, et je l'ai vérifié plutôt que concédé

Le pas invalide est exactement celui que vous désignez : `reserved` n'est
pas la résidence. `run_folds_budgeted` fait `reserved -= bytes[idx]` au
**retour** de la tâche, alors que le résultat vient d'être déplacé dans
`per_k_result[K]` et y reste jusqu'au dernier fold.

J'ai reconstruit votre minorant indépendamment, depuis les comptes du
reçu n=8000 et les `sizeof` réels, et je retrouve **2 599 581 876** octets
contre les **2 597 650 400** annoncés : **1 931 476** manquants, votre
chiffre au bit près.

**Et la mesure sur objets vivants est plus sévère que votre minorant.**
En gardant les dix `ForestResult` vivants à n=8000, ils pèsent
**2 548 288 512** octets mesurés — 18,6 % de plus, puisque votre minorant
excluait les `parents` et `batch_levels`. L'ancienne formule manquait
donc **400 804 864** octets. Votre borne inférieure suffisait à
condamner ; la mesure dit l'ampleur.

## 2. Les trois points de la Priorité 0 sont exécutés

**1. Le stopgap.** La décision porte sur `bytes_events + Sigma_K m_K`.

**Et surtout, le nom a changé** : `bytes_bound`, ligne
`preflight borne_travail_cumule`, et le refus dit lui-même « Cette borne
est un MAJORANT CONSERVATEUR, pas un pic de résidence ». Vous écriviez
« il peut refuser trop tôt, mais ne doit pas être nommé pic résident
précis » — c'est la moitié la plus importante de la correction. Ma faute
n'était pas l'arithmétique, c'était la promesse.

**2. La séparation** des trois postes n'est **pas** faite. Je ne la
revendique pas : la comptabilité actuelle les majore par une somme, et le
reçu porte un § « ce qui reste ouvert » qui reprend votre formulation
mot pour mot.

**3. La porte indépendante.** Vous avez raison de dire que la précédente
était auto-référentielle — elle comparait la décision de la formule à la
même formule gravée, donc elle pouvait voir une faute de recopie, jamais
une faute de modèle. Il y en a maintenant deux, dont aucune ne fait
entrer une ligne de `project_output_budget` :

- `--output-budget-gate` reconstruit le minorant de l'état final depuis
  les comptes du reçu et les `sizeof` réels ;
- `--fold-residency-gate` déroule les dix folds, **garde les dix
  résultats vivants**, et mesure — c'est littéralement la fixture que
  vous demandez.

L'accumulation y est le fait mesuré : la somme des dix vaut **3,4 fois**
le plus gros pris seul.

Un point que je préfère dire plutôt que de le laisser trouver : à n=6000
l'ancienne formule **couvre encore** (marge +280 Mo). La divergence
n'apparaît qu'au-delà d'environ n=7000. La porte rapide tourne donc à
n=6000 et le mutant est tué à n=8000, là où la faute existe.

## 3. Ce que votre audit m'a fait trouver ensuite

En vérifiant si la campagne G4 était lançable, la ligne
`fold_ordonnancement budget_octets=2147483648` m'a arrêté. **2 Gio,
c'est exactement le huitième des 16 Gio de ce conteneur** : une constante
calibrée sur la machine de développement, écrite en dur, qui voyage avec
le binaire.

Sur la `g4-standard-48` (48 vCPU, **180 Go**), elle vaut 1,1 % de la
mémoire. Et la borne d'un seul fold la dépasse bien avant n=64000 —
**mesuré au préflight** : à n=16000, K=10 pèse déjà **3,02 Gio**, donc
K=9 et K=10 sont admis **seuls** par la garde anti-blocage ; à n=64000 ce
sont K=5…10, qui portent **94 %** des incidences.

**La phase `n64000` aurait donc sérialisé le poste dominant sur une
machine à 48 cœurs louée pour mesurer la montée en fils.** Elle aurait
produit des digests corrects — l'objet ne dépend pas de l'ordonnancement,
la porte à modes le prouve — et un profil de scalabilité faux.

En corrigeant, un second défaut est tombé : `--fold-memory-budget`
n'atteignait que le banc d'ordonnancement, jamais le chemin de
production. Le drapeau était mort là où il comptait. C'est l'incohérence
entre `budget_source=memoire_hote` et `budget_octets=2147483648` sur la
même ligne publiée qui l'a révélé — un argument de plus pour publier les
provenances.

Règle retenue : `max(2 Gio, MemAvailable/4)`, lue une fois, publiée avec
sa provenance. Plancher pour qu'aucune petite machine ne régresse et
qu'aucun reçu antérieur ne soit invalidé ; portes figées au plancher,
parce qu'une porte dont le verdict dépendrait de la RAM de la machine
d'intégration ne serait pas une porte.

Mesure intra-processus à n=8000, **signature identique aux quatre
modes** : plancher 2 Gio **15 593 ms**, budget dérivé 3,73 Gio
**6 959 ms**, pic RSS inchangé. Réserve : `LPT_unbounded` (9 177 ms) et
le budget dérivé ne sont pas départagés par une seule paire ; ce que la
mesure établit, c'est que le plancher coûte.

## 4. Sur le reste de votre audit

Je ne prétends rien fermer d'autre. Votre § 5 (trace exhaustive
incompatible avec le SLO à 30 M), votre Priorité 1 (versionner
`full_symbolic_stream` / `connectivity_index` / `warm_query_or_labels`)
et vos portes de sortie § 10 sont pris tels quels et restent ouverts.
Votre verdict `public_status=not_claimed` est conservé sans réserve.

Je retiens en particulier votre Priorité 5 : la campagne G4 vient
**après** le GPU et après la décision d'objet produit, et la porte 50 k
doit couvrir le produit décidé — « une mesure de front, de composant
isolé ou de préfiltre ne vaut pas `warm_e2e` ». Le protocole
`scale_threads` actuel mesure la montée en fils du chemin dense, ce qui
n'est pas cela.

## 5. État

`ctest --test-dir build/v4` : **153 tests**. `check_docs`,
`check_passation`, `check_implementation_status`, `check_scope`,
`check_gcp_workflows`, `check_references` : verts.
