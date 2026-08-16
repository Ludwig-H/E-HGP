# `PairFrame + CoreDepthLedger` extraits sur CPU — et deux corrections que les portes ont imposées

Date : 16 août 2026 UTC.
Dossier : `morsehgp3D_v3/`.
Répond à `CONTRE_AUDIT_A617_PROFONDEUR_FRONTIERE_PAIRFRAME_20260816.md`,
`AUDIT_RECEPTION_A617_TELEMETRIE_NONE_WQ_PAIRFRAME_20260816.md` et
`AUDIT_POSITIF_DESCENTE_CIBLEE_PAIRFRAME_288032_20260816.md` §§ 8–9.

Cadre :

```text
phase=exploration_v3_hors_registre
backend=cpu_reference
profile=abi_modele_abstrait
mode=diagnostic_counter_only
public_status=not_claimed
```

Reçus **développeur** : `940` CTests, `939` verts, l'unique rouge étant
`mhgp3v_arith_selftest` (en-têtes GMP absents du conteneur, antérieur à ce
travail). Aucun workflow GitHub n'est attaché ; je ne présente rien d'autre que
des exécutions locales.

---

## 1. Ce qui est livré

`prototype/pair_frame.hpp` et `prototype/pair_frame_probe.cpp`, avec `34`
portes CTest sous le préfixe `mhgp3v_pairframe_*`.

Le modèle remplace le fuseau `W_q` par une matrice de statuts déclarée, mais il
conserve **exactement** la structure des bornes du gateway ternaire :

```text
lower(P, F) = somme sur n dans F, si TOUS les (p,w) de P x n sont vrais, de |n|
upper(P, F) = somme sur n dans F, si UN SEUL (p,w) de P x n est vrai, de |n|
```

Ces bornes encadrent le compte de chaque paire du bloc et sont monotones sous
raffinement de `P` comme de `n`. Le juge est une force brute par paire, calculée
sans jamais passer par le ledger.

**Ce que ces portes ne couvrent pas, et je préfère l'écrire que le laisser
supposer** : ni les prédicats géométriques, ni le masque endpoint relationnel —
un témoin `z` appartenant à `A` ou `B` n'a aucun analogue dans un modèle
abstrait —, ni le census de circumboule. Votre G5 sur géométrie réelle
appartient à « recevoir q2 de bout en bout ». Ce qui est certifié ici est
l'**ordonnanceur**, et rien d'autre.

---

## 2. Le cap de la section 9 ne peut pas se déclencher

C'est la correction qui compte, et elle n'est pas venue d'une relecture.

J'ai armé un plancher de couverture sur la branche `PENDING_RESOURCE`. La porte
`--politiques` a **refusé de se valider elle-même** avec `pending_total=0`. En
cherchant pourquoi, la preuve tient en une ligne.

`kPending` exige trois choses simultanément :

```text
pair_mass > exact_tile_cap
aucun span mixte scindable
endpoint non scindable
```

Or « endpoint non scindable » signifie `A` et `B` tous deux feuilles, donc
`pair_mass = |A| |B| = 1 <= cap` pour tout `cap >= 1`. Contradiction. **Sous le
cap de la section 9, `PENDING_RESOURCE` est du code mort**, et l'argument porte
aussi bien sur le modèle abstrait que sur la géométrie réelle, puisqu'il ne
parle que de `|A| |B|`.

La ressource qui déborde vraiment est celle que vous nommez au § 5.3 : la
**largeur de frontière**, le `kCapRacines` d'un CTA. Le coût d'une tuile exacte
est `pair_mass * |frontière|`, pas `pair_mass`. D'où deux caps :

```text
exact_tile_cap  borne pair_mass * frontier_width
frontier_cap    borne frontier_width, donc interdit la scission de témoin
```

Un état à paire singleton, frontière saturée et tuile trop chère ne peut alors
ni se fermer, ni se scinder, ni s'exactifier : il rend une continuation, l'hôte
re-dispatche avec plus de place, et le résultat final est inchangé.

Le défaut est **gravé** plutôt que corrigé en silence :
`mhgp3v_pairframe_cap_section9_sans_continuation` exécute les mêmes paramètres
avec `--cap-sur=masse` et exige le code `3` sur « plancher de continuations non
atteint ». Aux mêmes paramètres, `--cap-sur=tuile` rend `26` continuations et
`--cap-sur=masse` en rend `0` — les deux avec `ecart=0`, ce qui est le point :
un cap ne change jamais le résultat.

Corollaire d'ABI : `frontier_width` est un **scalaire** dans `CoreDepthLedger`,
pas un `frontier.size()`. Une politique qui appellerait `size()` à chaque vague
paierait `O(F)` là où elle annonce `O(1)`, et un CTA ne matérialise pas sa
frontière pour savoir combien de spans il porte.

---

## 3. Vos G1 et G2, gravées

Une seule exécution les met côte à côte, `adversaire` à `256` témoins :

```text
state_refine_iterations_max=76      lbvh_internal_depth_max=8
frontier_span_count_peak=128        frontier_candidate_mass_peak=256
```

Soixante-seize itérations pour un arbre de profondeur interne huit : le
compteur que j'appelais `refine_depth_max` ne mesurait jamais une profondeur, et
`frontier_peak` ne mesurait jamais une frontière. Vous aviez raison sur les deux,
y compris sur la cause — **le nom était le défaut**, et il m'a fait écrire
« profondeur `0,83 n` », ce que le radix LBVH u16 ne peut structurellement pas
produire à `n=120`.

Les renommages sont faits dans `acute_owner_gateway_probe.cpp` :

```text
frontier_peak      -> frontier_candidate_mass_peak
refine_depth_max   -> state_refine_iterations_max
continuation_mass  -> pending_state_point_incidence_mass
frontiere_max      -> frontier_span_count_peak
```

Les deux commentaires obsolètes du § 8 sont corrigés en « raffinement piloté par
le seuil ; aucune borne du travail total par le seul seuil », et le commentaire
long porte désormais votre borne `48 + 16 = 64`.

J'avais aussi commencé par imprimer `lbvh_internal_depth_max<=64` dans le reçu
du vieux probe. Je l'ai retiré : c'était une **affirmation non mesurée dans un
reçu**, exactement le défaut dont cette note traite.

---

## 4. Le sélecteur à buckets, et sa mesure

Implémenté comme vous le décrivez : `bucket = floor(log2(population))`, masque
de seize bits, plus haut bit non vide, pile par bucket — aucun `erase` au
milieu. Il est la politique **par défaut**, et son invariant

```text
selector_scan_items <= witness_splits + terminal_checks
```

est armé en permanence, sans option.

À `256` témoins, les deux sélecteurs font un travail géométrique **identique** :

| | buckets | vectoriel |
|---|---|---|
| `witness_splits` | `2 431` | `2 431` |
| `child_classifications` | `4 863` | `4 863` |
| `frontier_span_count_peak` | `128` | `128` |
| décisions | identiques | identiques |
| `selector_scan_items` | **`2 431`** | **`238 560`** |

Facteur `98`. La rampe `16 / 32 / 64 / 128 / 256` témoins donne des rapports
`8,0 / 16,0 / 32,0 / 60,5 / 98,2`. Votre § 4 est reçu et mesuré.

Le sélecteur vectoriel est conservé **uniquement** comme terme de comparaison —
c'est ce reçu qui justifie de ne pas le porter.

Une conséquence sur laquelle je ne veux pas laisser d'ambiguïté : l'énoncé de ma
rétractation, `O(spans classés + scissions endpoint)`, n'était pas le coût du
code d'alors, et vous aviez raison de le refuser. Le commentaire du gateway le
dit maintenant.

---

## 5. La continuation est un objet, pas un compteur

`CoreContinuation` porte l'antichaîne complète — spans décidés **inclus**,
puisqu'ils portent le minorant et que les perdre reviendrait à recommencer —,
la provenance `PairFrame`, la version de politique et l'époque.

La porte ne se contente pas de l'existence du type. Sous `--reprise=octets`,
l'état pendant est **sérialisé, l'état mémoire abandonné, le tampon relu**, et
c'est l'objet reconstruit qui redémarre le calcul. La configuration `10` de la
porte différentielle est la configuration `9` à ceci près, et les deux vecteurs
de décisions sont identiques entre eux et à la force brute.

Reçus par famille (`cap=2`, `front_cap=3`) :

| famille | `pending_state_count` | `resume_count` | `continuation_octets` | `ecart` |
|---|---|---|---|---|
| `prefixe` | `9` | `1` | `540` | `0` |
| `aleatoire` | `55` | `1` | `3 300` | `0` |
| `damier` | `20` | `1` | `1 200` | `0` |
| `adversaire` | `64` | `1` | `3 840` | `0` |

`adversaire` est la contre-famille de travail de votre G3 : un témoin sur deux
est vrai avec une phase qui dépend de la paire, donc **aucun** sous-arbre n'est
homogène avant les feuilles. `pruned=0` et `clear=0` : tout s'y décide par tuile
exacte. C'est le pire cas du schéma, et il est en porte.

---

## 6. La porte différentielle : douze politiques

Cinq heuristiques de sélection (`buckets`, `plus-gros`, `plus-petit`, `premier`,
`feuilles`), trois caps de tuile, deux caps de frontière, trois budgets de
témoin, reprise en mémoire et par octets. **Même vecteur de décisions pour les
douze**, égal à la force brute, sur quatre familles. Le travail géométrique
varie d'un facteur mille.

`--min-ecart-travail` et `--facteur-scan` interdisent à cette porte d'être vide :
si les politiques ne se distinguaient pas en travail, elle comparerait douze
fois la même exécution.

C'est G1 et G2 dans un seul passage, plus G4 par les compteurs de sélection et
G5 par la reprise sérialisée — **au niveau de l'ABI**. Sur géométrie, elles
restent à faire.

---

## 7. Les cinq mutants, dont deux que le juge ne peut pas voir

| mutant | nature | tué par |
|---|---|---|
| `clear-large` (`upper <= h`) | **sûreté** : un support survit à tort | juge de résultat + fixtures |
| `pending-pruned` | **sûreté** : une continuation devient un verdict | juge de résultat + ABI exhaustive |
| `sature-tronque` | **sûreté** au-delà de `255` | saturation + descente à `256` témoins |
| `pruned-large` (`lower > h`) | *conservateur* | fixture d'égalité + saturation |
| `cap-avant-verdict` | *conservateur* | invariant `TERMINAL <=> verdict` + plafond de tuiles |

Les deux derniers dégradent le travail sans fausser le résultat, donc **aucun
juge de résultat ne peut les voir**. Le dire est plus utile que de prétendre une
couverture qui n'existe pas ; c'est la même leçon que `phi-large` sur le
gateway.

Un fait de méthode, parce qu'il a failli me coûter une porte vide :
`pending-pruned` était **inerte** dans ma première descente, parce que la sonde
consultait l'action et ignorait le fate rendu. Un mutant qui ne peut pas agir
n'est pas un mutant tué, c'est une porte qui mesure le vide. La sonde agit
maintenant sur le fate, comme le ferait tout appelant réel.

---

## 8. Ce que je n'ai pas fait, et pourquoi

Vous écrivez : « ne pas consacrer une nouvelle série de commits à optimiser le
scheduler ternaire historique ». Je m'y tiens. Sur `acute_owner_gateway_probe`
je n'ai touché **que** la sémantique des compteurs et les deux commentaires : ni
buckets, ni continuation typée, ni suppression du rescan. Il reste l'oracle de
transition.

Les compteurs causaux que vous demandez au § 4.2 — `selector_scan_items`,
`selector_vector_moves`, `child_classifications`, `terminal_checks` — existent
dans `pair_frame_probe`, pas dans le vieux probe, pour la même raison.

---

## 9. Deux questions

**Q1 — le cap de tuile et la lane.** J'ai posé `exact_tile_cap` sur
`pair_mass * frontier_width`. Pour q4, la tuile ne coûte pas seulement une
lecture par `(paire, span)` : elle enchaîne `carrier -> Jung -> axial ->
owner/positivité -> BallKey`. Le cap doit-il rester un produit à deux facteurs,
ou porter un poids par lane — quelque chose comme
`pair_mass * frontier_width * poids(q)` — pour qu'une tuile q4 et une tuile q2
ne soient pas budgétées au même prix ? Le choix change les états qui deviennent
pendants, jamais le résultat, mais il change la comparabilité des reçus entre
lanes.

**Q2 — l'antichaîne dans la continuation.** Je sérialise les spans décidés
`ALL` et les spans mixtes, et je **supprime** les spans `NONE` : ils créditent
zéro des deux bornes et zéro du compte exact de chaque paire du bloc, et sous
restriction de `P` un `NONE` reste `NONE`. La suppression me paraît donc sûre et
elle économise réellement (`spans_elimines=320` sur la descente à `256`
témoins). Mais elle rend la continuation **non reconstituante** de la partition
initiale des témoins. Est-ce acceptable pour votre contrat transactionnel, ou
exigez-vous que la continuation puisse rejouer la partition complète — auquel
cas je garde les `NONE` avec un drapeau ?

---

## 10. Étape suivante

Étape 3 de votre ordre : **recevoir q2 de bout en bout**. C'est là que le masque
endpoint relationnel devient testable, puisqu'il n'a pas d'analogue abstrait, et
q2 est le seul cas où le fuseau EST la miniboule du support — donc le seul où
`CORE_CLEAR` et vivacité exacte coïncident, et le meilleur banc d'essai comme
vous l'écrivez.
