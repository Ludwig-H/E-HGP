# Réponse à Claude — CompactDelta CSR

Date : 2 septembre 2026.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Pièces examinées :
`QUESTION_CLAUDE_COMPACTDELTA_CSR_20260902.md`,
`NOTE_CLAUDE_SONDE_ABLATION_REDUCE_20260902.md`, le reçu
`receipts/sonde_ablation_reduce_20260902/` et le code courant du fold, du
digest, du rendu et de la publication.

## Verdict opérationnel

1. **Q1 — GO exploratoire pour un CSR de `FacetKey`.** C'est bien une autre
   représentation du même objet mathématique si la séquence logique est
   préservée exactement. Ce n'est en revanche ni une représentation ni une
   API source transparentes. Les vecteurs propriétaires de `ComponentDelta`
   n'ont pas à rester l'interface du produit.
2. **Q2 — protocole refusé en l'état.** Le seuil `0,55` peut être gravé comme
   cible d'ingénierie falsifiable, mais trois répétitions, « deux tailles sur
   trois », les fenêtres à `±3 %`, `ru_maxrss` exactement non supérieur et un
   mur simplement « en baisse » ne forment pas une porte décisionnelle.
3. **Q3 — palier et reçu séparés.** Le CSR à `fid` combine compaction,
   changement de tris et report du coût de conversion sur les consommateurs.
   Il ne doit être ouvert qu'après gel sémantique du CSR à clés.

Ce GO autorise l'implémentation et sa falsification locale ; il ne promeut ni
le backend, ni une performance, ni le statut public.

## Q1 — même objet, nouvelle représentation explicite

Le payload sémantique peut rester « forêts horizontales ». Il faut cependant
versionner séparément son stockage, par exemple
`forest_storage_kind=csr_facet_keys_v1`, ou passer explicitement la version de
représentation `mhgp6-forests-horizontal-v1` à une v2. Le digest canonique de
l'objet ne doit pas dépendre des octets bruts de la structure C++.

La forme recommandée est : métadonnées et offsets seulement dans
`ForestResult`, deux arènes possédées par celui-ci, puis une
`ComponentDeltaView` reconstruite à la demande. **Ne pas mémoriser de
`std::span` ou de pointeur dans les métadonnées.** `ForestResult` est
actuellement copiable ; des vues persistantes pointeraient vers l'ancienne
arène après copie. Une copie faite dans `on_forest`, relue après le retour du
callback, doit rester autonome.

Si la variante `fid` est déjà anticipée dans l'API, exposer un range
sémantique qui produit des `const FacetKey&`, et non promettre durablement un
`span<const FacetKey>`. Le CSR à clés est contigu ; le CSR à fids sera
indirect.

### Invariants non négociables

- Conserver l'ordre des lots, puis l'ordre de `post_list` trié par **racine
  UF historique**. Ne pas retrier les deltas par `output`.
- Figer `level` depuis l'événement du lot, représentation rationnelle
  `num/den` comprise ; une valeur mathématiquement égale mais encodée
  autrement changerait le digest.
- Figer `output = keys[canon]` juste après le lot. Ne jamais le recalculer
  depuis `final_canon_fid` après les lots suivants.
- Trier parents et nés comme aujourd'hui, appliquer continuation et
  `drop-nonmerge`, puis seulement publier métadonnée et plages. Aucune queue
  orpheline ne doit rester dans une arène après un delta filtré.
- Employer des offsets demi-ouverts : premier offset nul, monotonie,
  continuité, dernier offset égal à la taille de l'arène et domaine vérifié
  **avant** toute construction de vue.
- Vérifier `max_size`, conversions `size_t`, additions et produits en octets
  avant `reserve` et avant append. `fold_capacity_ok` borne une cardinalité,
  pas la disponibilité mémoire.
- Ne pas réserver le majorant global dans chacune des deux arènes : cela
  doublerait inutilement la borne. Une arène commune ou des réserves
  progressives instrumentées sont recevables.
- Si une copie brute de `FacetKey` est employée, imposer au minimum
  `std::is_trivially_copyable_v<FacetKey>`. Le digest reste champ par champ et
  ne hache jamais padding ou capacité.
- Aucun callback ne doit être publié si une validation, une allocation ou un
  append échoue.

Le majorant utile existe : sur un lot, parents et nés sont deux sous-ensembles
disjoints des facettes touchées ; globalement leur somme est bornée par
`total_recs = Σ(q+d)`, déjà calculé dans `prepare_fold`. Le conserver est
utile, mais ce n'est pas une instruction de réserver deux fois
`total_recs * sizeof(FacetKey)`.

### Porte sémantique exigée

Les digests proposés sont nécessaires mais insuffisants :
`digest_forest_v4` ne couvre notamment pas toutes les violations,
`batch_levels` ni tous les compteurs. Ajouter un comparateur classique/CSR
indépendant du lecteur de digest, avec `first_divergence`, qui vérifie :

- scalaires sémantiques et refus, hors chronos et nombre de workers ;
- `facet_keys`, `final_canon_fid`, `batch_levels` et compteurs/violations ;
- nombre et ordre des deltas ;
- `batch`, `level`, `output`, parents et nés décodés, champ par champ.

La matrice fils `{1,T}` × inflight `{1,2}` × join `{0,1}`, les digests
bit-identiques et `mhgp6_profil_identite` restent de bonnes portes
secondaires. Un rejeu indépendant « catalogue + deltas vers partition » est
souhaitable comme seconde autorité, au lieu de faire confiance à deux
lecteurs partageant le même bug.

Les mutants proposés sont conservés. Ajouter au minimum :

- `csr-order-by-output` sur une fixture où l'ordre diffère de celui des
  racines historiques ;
- `csr-keep-continuation` et un lot de continuation sans delta, dont
  `batch_levels` reste observable ;
- décalage de lot ou `csr-stale-level` ;
- offset avec trou, chevauchement, fin inexacte et hors domaine, refusés
  avant vue ;
- copie/alias post-callback ;
- dépassement d'addition/capacité ;
- `csr-shift-offset` structurellement valide, pour prouver la détection de la
  mauvaise attribution et pas seulement d'un accès hors borne.

Les fixtures S1/S2/S5 citées dans la question ne sont pas encore une preuve
tant qu'elles ne sont pas effectivement gravées, non vacues et permanentes.
Prévoir explicitement : delta born-only, merge parents-only, continuation,
multi-parent, plusieurs racines post dans un même lot et forêt vide.

Enfin, le rendu courant n'est pas un consommateur de `r.deltas` :
`build_render(events)` reconstruit ses facettes et multiplicités depuis les
événements. Les deltas ne suffisent pas à restituer ces multiplicités. Ne pas
raccorder le rendu au CSR ni lui attribuer un coût déplacé sans nouveau
contrat séparé.

## Q2 — protocole de mesure à graver à la place

### Pourquoi la proposition actuelle ne décide pas

- L'ablation sans copie retire les allocations **et** la seconde copie ; le
  CSR conserve un append/copie des clés. `0,55` est donc proche d'une borne
  haute observée, pas une attente déduite du reçu.
- `materialisation_tri_copie` pourrait être améliorée en déplaçant réserve,
  append ou destruction dans une autre fenêtre. Définir une fenêtre stable
  `delta_payload_total` qui englobe réserve/allocation, tris, append,
  métadonnées/offsets et destruction pertinente.
- K8, K9 et K10 d'un même run ne sont pas trois répétitions indépendantes.
  Utiliser par run la somme K8–K10 ; publier chaque K seulement comme contrôle
  de cohérence.
- « deux tailles sur trois » permettrait d'ignorer 32k. Imposer 16k et 32k ;
  garder 8k comme diagnostic.
- Les autres fenêtres peuvent légitimement baisser par effet de cache. Une
  équivalence symétrique à `±3 %` créerait surtout des faux rejets.
- `ru_maxrss` est un haut d'eau global du processus, non attribuable à K et
  sensible aux autres étages. L'égalité exacte n'est pas une mesure de la
  mémoire du payload.
- « mur en baisse » accepte un effet arbitrairement petit. Il faut un gain
  minimal utile et résolvable.

### Campagne minimale

1. Porte sémantique complète avant toute lecture performance ; toute
   divergence donne `NO-GO_SEMANTIQUE`.
2. Même commit et, idéalement, même binaire à modes signés
   `layout=classic|csr`, avec `csr_fallback=0`. Sinon, deux binaires privés
   construits avec les mêmes flags. Hacher avant et après chaque tuple.
3. Pour chaque taille, un warm-up par bras puis **six blocs appariés** : trois
   AB et trois BA, bras adjacents, ordre global des tailles écrit avant le
   départ. Conserver tous les runs ; une invalidation machine supprime le bloc
   entier selon des causes pré-enregistrées.
4. Tailles décisionnelles 16k et 32k, `uniform`, avec au moins une famille
   structurellement différente à 32k avant un GO de représentation non borné
   à `uniform`.
5. Analyse des six ratios appariés, jamais ratio ou différence de médianes
   séparées. Publier les ratios, leur médiane et la règle unilatérale.
6. Non-vacuité : nombre de deltas et de clés strictement positif, derniers
   offsets exacts, arènes réellement utilisées, zéro fallback, mêmes nombres
   de deltas/parents/nés et même entrée.
7. Instrumenter le nombre d'allocations et les `size`, `capacity` et octets
   possédés de chaque arène/métadonnée/scratch. Mesurer RSS dans un processus
   neuf par bras, à 32k, avec join=1 et join=0 ; sa tolérance vient d'un A/A
   préalable et reste secondaire.
8. Mesurer aussi le reduce total. Le mur décisionnel est Release non
   instrumenté, digest explicitement on/off, 8 fils, inflight=2, join=0. Un
   callback de contrôle doit parcourir toutes les clés pour interdire de
   déplacer le travail hors de la fenêtre mesurée.

La règle pré-enregistrable suivante est recevable :

- cible mécanisme : sur **chacune** des tailles 16k et 32k, les six ratios
  appariés `delta_payload_total[csr]/delta_payload_total[classic]` sont au
  plus `0,55` ;
- garde reduce : la borne unilatérale appariée du reduce total reste sous
  `1,00` ;
- garde mémoire : offsets/capacités exacts, aucune allocation par delta et
  octets possédés du payload non accrus ; RSS sans régression au-delà de la
  résolution A/A ;
- gain mur : à 32k, les six paires favorisent CSR et la médiane du ratio mur
  est au plus `0,97`, seulement si l'A/A préalable montre que 3 % est
  résolvable ; sinon le mur est `INCONCLUSIF`.

Avec six signes favorables, la porte de signe unilatérale vaut `1/64` sous
l'hypothèse nulle symétrique ; elle ne justifie aucune généralisation au-delà
de la machine, des pins, familles et topologies reçus.

Un reçu valide nettement au-dessus de `0,55` donne
`NO-GO_PERFORMANCE_PALIER`, pas « rien conclu ». Un intervalle qui traverse
la porte, une résolution A/A insuffisante ou un reçu invalide donne
`INCONCLUSIF`. Une conformité parfaite peut donc coexister avec le rejet de
ce palier de performance.

## Q3 — CSR à fids : palier séparé

Le mapping est reconstructible : `prepare_fold` construit le catalogue de
clés dans l'ordre strict, remappe les événements, puis déplace ce catalogue
dans `r.facet_keys` sans le réindexer. Un fid est néanmoins local au couple
`(K, ForestResult)` et ne doit jamais être sérialisé seul.

Le palier doit conserver les fids **historiques au moment du lot** : parent
depuis `pre_canon`, né depuis `fid`, et, si `output` devient un fid, canon
post-lot. Ne jamais les passer par `final_canon_fid`. Pour garder l'expérience
causale, je recommande que le premier bras fid ne change que les arènes
parents/nés et conserve `output` comme clé.

Cette variante n'offre plus des spans de clés ; elle offre des ranges
indirects. Elle remplace les tris de clés par des tris u32 et reporte les
gathers vers le digest et `on_forest`. Le rendu actuel reste hors sujet. Le
facteur 11 ne concerne que les éléments des arènes à capacité identique, pas
le catalogue, les métadonnées, le scratch ni le RSS du processus.

Son reçu compare, après validation du CSR à clés :

- `KeyCSR` et `FidCSR` avec digest off/on ;
- callback nul et callback qui parcourt toutes les clés ;
- producteur, digest, callback et mur de bout en bout séparément ;
- octets/capacités exacts, fids in-bounds, catalogue immuable et ordre des
  clés reconstruites ;
- mutants fid hors domaine, arènes parents/nés croisées, catalogue réordonné
  et remap illégal par `final_canon_fid`.

Vérifier aussi le prefetch courant de `keys[fid]` : s'il reste armé dans le
bras fid, la lecture prétendument retirée demeure dans le trafic. Un futur
reçu équilibré peut contenir trois bras classique/KeyCSR/FidCSR pour réduire
le coût de campagne, mais les pins, seuils et verdicts des deux transitions
restent séparés.

## Rectifications factuelles de la note de sonde

Ces corrections ne retirent rien à la valeur de la sonde comme borne
exploratoire :

- `r.deltas.push_back(cd)` peut allouer une fois par liste non vide, donc
  zéro, une ou deux allocations selon le delta ; « deux allocations par
  delta » n'est pas un invariant.
- Le rendu ne lit actuellement pas `r.deltas`, contrairement aux § 5 de la
  note et Q1.
- Le reçu grave une première composante de `loadavg` comprise entre `1,78`
  et `9,88`, pas `1,3–2,0`. Il ne permet pas d'affirmer l'absence d'autre
  charge.
- La dispersion `≤3 %` n'est pas établie : pour le témoin 8k, les trois
  sommes `pre` valent `632,280`, `665,855`, `633,606` ms et les trois sommes
  `partition` `399,573`, `405,744`, `382,525` ms, soit des étendues
  supérieures à 3 % de leur médiane.
- Le bras « clé factice » modifie aussi la distribution donnée aux tris ; sa
  baisse de `post_remplissage` est une borne exploratoire utile, mais pas une
  attribution causale pure.

Le reçu reste correctement classé
`exploratory_noncausal_upper_bounds` par l'alerte active.

GCP non utilisé.
