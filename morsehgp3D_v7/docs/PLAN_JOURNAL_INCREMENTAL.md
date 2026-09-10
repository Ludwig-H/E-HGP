# Plan de journal FULL incrémental — conception non implémentée

10 septembre 2026. Document de conception seulement : **aucun assembleur
incrémental livré ou qualifié par cette note**. Publication d'une analyse
privée, sans modification du moteur, compilation ni benchmark nouveau.
GCP non utilisé pour ce travail. Cadre :
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Choix recommandé

Ajouter un **assembleur interne incrémental**, alimenté par un lot exact complet,
qui écrit directement les quatre arènes finales v2. Il reste privé jusqu'au
scellement de toute la tour sur sa banque immuable. Conserver le constructeur
public actuel comme façade de validation/encodage, avec les mêmes règles
factorisées : ne pas créer un deuxième juge, ni modifier le resolver géométrique.

La première variante peut garder un seul `FullCoverageBatch` de travail,
immédiatement consommé puis détruit. Elle supprime la rétention globale des
brouillons, mais **pas** leurs allocations par action. Le delta performant
suivant, toujours local, est une vue de lot sur trois scratchs plats réutilisés
(descripteurs, parents, références). Le cas singleton emprunte directement
`Block::roots` et une référence sur la pile : aucune copie owning de parents,
aucun vector d'actions/contributions requis dans cette branche.

Les [mesures appariées 50k G4](../receipts/full_ball_scale_gpu_20260910/README.md)
motivent ce travail ; leurs durées et leurs autorités restent celles du reçu,
sans nouvelle certification ici. Un nombre de MEB/supports
ne donne ni le nombre d'actions ni une fraction de temps attribuable à
l'allocateur. Cette proposition ne promet pas de franchir 1 s.

## Allocations exactement visées

Dans [full_ball_tower.hpp](../src/forest/full_ball_tower.hpp), lu à SHA
`910f45ba…` (les numéros de lignes ci-dessous se rapportent à ces octets) :

- L157 et L247–284 : `Draft` possède `batches` et `lower_nodes` ; tous les
  `drafts` restent vivants jusqu'après le dernier K. Garder `lower_nodes`,
  supprimer seulement les journaux owning `batches`.
- L258–265 : le lot initial K1 alloue le vector d'actions et une référence
  owning par singleton ; il doit rester **un** lot zéro, pas n lots égaux.
- L627–658 : chaque singleton publié construit un `FullCoverageAction`, copie
  ses parents L633, alloue sa référence éventuelle L635, puis alloue un
  vector d'une action L656. Même une continuation inerte copie actuellement
  ses parents avant d'être écartée. L'assembleur n'en fera pas une action.
- L675–722 : les groupes produisent autant de vectors parents/contributions
  que d'actions ; `batch.actions` et ces allocations sont ensuite retenus.
  Les scratchs DSU, owners, groups, targets et `Block::roots` restent hors
  du premier delta : ils sont locaux, nécessaires à une fermeture correcte,
  et peuvent être optimisés séparément.
- L312–318 : une deuxième passe recopie parents et références dans le journal
  final, puis détruit chaque brouillon. Cette passe tardive disparaît ; les
  écritures finales elles-mêmes restent obligatoires.

Pour B lots publiés et A actions, on retire de la résidence globale B objets
`FullCoverageBatch`, A objets `FullCoverageAction`, B vectors d'actions et
2A vectors internes (souvent vides), plus leurs buffers non vides. Ce n'est
pas B+2A appels d'allocation exactement : les vectors vides n'allouent pas,
et les non vides peuvent croître plusieurs fois. Avec scratch plat, ces
buffers deviennent au plus trois buffers réutilisés par assembleur de lot,
plus les quatre arènes finales et le tableau `live` de l'ordre courant.
Il ne faut garder aucun descripteur de lot/action après son append réussi.

## API interne minimale et couture

Esquisse de signature, **non implémentée** :

```cpp
struct CoverageActionView {
  std::span<const FullNodeId> parents;
  std::span<const FullCoverageRef> contributions;
};
struct CoverageAppendResult {
  FullCertificateStatus status;
  const char* reason;
  FullNodeId first_new_node;
  size_t new_node_count;
};
// Types dans full_coverage_detail ; aucune arène publique avant seal.
CoverageAppendResult append_batch(
    const ExactLevel&, std::span<const CoverageActionView>);
// L'assembleur est lié à un unique propriétaire append-only de populations.
// seal est privé, appelé seulement par ce propriétaire après gel de sa banque.
FullCoverageBuildResult seal(/* capacité de scellement privée */);
```

Le scratch groupé conserve des **offsets**, pas des spans pendant sa croissance.
Créer les vues seulement une fois ses buffers figés pour l'appel synchrone.
Chaque vue n'est empruntée que pendant `append_batch`. Le singleton peut
utiliser `std::array<CoverageActionView, 1>` et une référence locale.
Ne pas ajouter un vector de résultats par action : les nouveaux IDs suivent
déjà l'ordre des actions en sautant les continuations ; contrôler
`first_new_node`, le nombre final et chaque correspondance dans les gates.

Couture proposée :

1. `close_lot` résout/grouppe tout comme aujourd'hui. `new_node` conserve
   pour ce premier delta ses histoires, verticales et vérifications actuelles.
   Il peut préparer son état privé avant l'append ; un échec invalide toute la
   construction. Ne pas fusionner ces deux implémentations dans ce delta.
2. Appeler `append_batch` une seule fois après préparation de toutes les
   actions publiables et **avant** l'installation des ancres/semis du lot.
   Un lot entièrement inerte n'est pas envoyé au journal ; ses ancres restent
   obligatoires et sont toujours installées après résolution complète.
3. En fin de K, garder les arènes privées et `lower_nodes`, libérer `live`.
   Le resolver conserve exactement ses deux histoires adjacentes actuelles.
4. Après le dernier K, libérer les états morts comme aujourd'hui, construire
   l'unique banque immuable, sceller les arènes par déplacement et publier
   seulement si tous les ordres et toutes les verticales sont cohérents.

### Banque : pas de faux scellement

Les populations sont créées paresseusement dans `population()` L602. Leur
ordre doit rester l'ordre de première contribution, y compris l'ordre des
groupes sur un plateau ; pas de préclassement global par boule/niveau.
Leur vector peut réallouer : aucun span vers ses lignes ne survit à un append.
Une ligne déjà enregistrée ne doit plus être modifiable.

Le propriétaire append-only doit être interne, partagé par les assembleurs
de cette tour ; seul lui peut produire le jeton de scellement de **sa** banque.
Ne pas proposer `seal(shared_ptr<Bank> quelconque)` : une banque différente
mais de même taille pourrait réinterpréter toutes les références validées.
Les assembleurs lisent une vue fraîche du propriétaire à chaque append.
La validation complète existante des populations demeure obligatoire au gel,
y compris pour une ligne jamais référencée. Avant de calculer `all_shell`,
contrôler aussi la largeur <=16 de la ligne consultée : le constructeur actuel
peut présupposer cette garde parce que sa banque est déjà validée.

Pour le premier delta, garder la copie actuelle vers la banque immuable est
le changement le plus petit. L'adoption par déplacement des lignes et du
domaine, sans alias mutable survivant, est un delta distinct après mesure.

## Transaction et refus à préserver

Le noyau de validation de
[full_coverage_certificate.hpp](../src/forest/full_coverage_certificate.hpp)
L191–252 doit rester
commun à la façade actuelle et à la voie incrémentale : mêmes raisons
sémantiques lorsqu'aucune panne concurrente ne les masque.

- Domaine/K, premier lot K1 zéro et exhaustif dans l'ordre du domaine ; pas
  de naissance K1 ultérieure ; niveaux de lots strictement croissants et
  K>1 positifs ; aucun lot vide envoyé à l'API.
- Capturer `prior_count` **une fois avant le lot**. Première passe sur toutes
  les actions : parents triés, uniques, pré-lot et vivants ; leur consommation
  détecte aussi un parent partagé entre actions. Ne restaurer aucune
  continuation avant que toutes les actions aient été validées.
- Références/masks non vides valides ; naissance à exactement une population
  entière de cardinal au moins K ; continuation non vide, sans nouveau nœud ;
  multifusion vide de contribution autorisée. Deux naissances restent deux
  nœuds même si leurs couvertures sont égales.
- Deuxième passe seulement : mêmes IDs, offsets CSR (y compris naissances),
  parents effectifs, successeurs historiques, dates et ordre des contributions.
  Aucune normalisation d'un successeur vers la racine finale.
- Un refus ou une exception marque l'assembleur définitivement en échec ;
  aucun append/seal ultérieur ne peut publier son préfixe. Les modifications
  privées partielles de `live`/arènes sont alors abandonnées : pas besoin d'un
  journal de rollback par lot. La sortie publique est vide, ordre nul et sans
  banque. Le constructeur de tour conserve son refus global sans ordres partiels.
- `bad_alloc`, `length_error`, débordement des additions/tailles et sentinelle
  d'ID restent des refus, sans plafond artificiel nouveau. La priorité entre
  invalidité et panne simultanées n'est pas contractuelle (audit déjà publié).

## Coût et économie : les bons dénominateurs

Noter N les nœuds, P les références de parents finales, C les contributions,
T les continuations publiées. Alors A=N+T et T<=C. Les parents lus dans les
actions sont P+T. Les deux passes et les croissances géométriques coûtent donc
**O(N+P+C)** amorti ; les arènes et `live` sont de cette taille. Un lot non vide
compte au moins une action, donc B<=A. Ce coût exclut géométrie/MEB, tri et DSU
des groupes, validation de banque O(points référencés × log n), et allocations
du catalogue/populations/histoires qui restent inchangées.

**Ne jamais faire `reserve(size()+delta)` à chaque lot** : cela peut recopier
tout le préfixe et devenir quadratique. Croissance géométrique vérifiée, puis
éventuellement une seule compaction à la fin d'un ordre. `shrink_to_fit` ne
garantit pas une capacité exacte. Si les réservations exactes ne sont plus
effectuées, ne pas annoncer `exact_structural_sizes_reserved_v1` pour cette
voie ; une étiquette d'accounting différente ne change pas le schéma v2.

ABI déjà mesurée dans le
[reçu de résidence](../receipts/full_tower_residence_20260910/capture/README.md) : Batch 80, Action 48, Ref 16,
FullNode 64, contribution datée 80, parent/successeur 8 octets. Hors capacités :

| Stockage | Octets logiques, sans verticale ni banque |
| --- | ---: |
| Brouillons actuels | `80*B + 48*A + 8*(P+T) + 16*C` |
| Arènes finales conservées | `72*N + 8*P + 80*C` |
| Écart brouillons moins arènes, à cette frontière | `80*B + 48*A - 72*N - 64*C + 8*T` |

Le tableau `live` ajoute environ N octets pour l'ordre en cours ; scratchs,
surcapacités, histoires et coexistence transitoire de réallocations s'ajoutent.
Une naissance singleton coûte ainsi **144 octets en brouillon contre 152
en arènes finales**. Une fusion à deux parents sans contribution coûte
144 contre 88. L'assembleur retire les millions d'allocations imbriquées mais
paie le format final plus tôt : son gain net de pic doit être mesuré.

Le même reçu ancien n800 observait 4 983 611 appels new malgré réservations/libérations,
contre 4 984 339 auparavant ; il ne compte pas uniquement le journal. L'[audit de résidence](../audits/receipts_tower_cost_review_20260910/README.md)
32k borne 17 114 695 batches et 17 166 975 actions régulières, soit au moins
2 193 190 400 octets d'en-têtes. Ces captures ne sont pas les nouvelles mesures
G4 et **ces octets ne sont pas intégralement un gain RSS**. Le stockage final
reste une borne incompressible de cette API ; aucune borne sous-quadratique
universelle en n ne découle de O(N+P+C).

## Gates avant raccord et première mesure utile

1. Journal actuel vs assembleur sur les mêmes lots : égalité physique de tous
   les tableaux, ordre des lignes et références, partage réel de la banque,
   move/invalidations ; pas seulement égalité des couvertures lues.
2. Reprendre les [fixtures causales de l'auditeur des gardes](../audits/receipts_journal_guards_20260910/README.md) (dix positifs,
   neuf rejets, six lectures), les 30 refus historiques, le carré K2 à quatre
   parents et le mutant `parent -> 0`. Tester simultanément plusieurs ordres
   scellés sur la même banque, puis empoisonner les buffers d'entrée.
   Le [contrôle physique des parents](../receipts/coverage_parent_array_20260910/README.md)
   reste obligatoire, indépendamment des lecteurs de couverture.
3. Nouveaux refus : dernier lot invalide après préfixe valide, parent créé
   dans le lot courant, parent partagé après continuation, tentative de reprise
   après échec, mauvais propriétaire de banque/scellement répété, réallocation
   du vector des populations entre deux lots. Pannes à chaque allocation
   réellement observée, y compris croissance tardive et scellement global.
4. Tour : [corpus Gram/Gamma existant](../receipts/ball_resolver_residence_20260910/README.md),
   28 nuages, plateaux growth/inert groupés,
   terminal K=n, K1 ; mêmes compteurs géométriques, payload physique et cartes
   verticales. Mutants ancres installées trop tôt, croissance omise, date de
   continuation antidatée. O2 et ASan/UBSan stricts ; pas de nouveau catalogue
   Gamma côté produit.
5. D'abord instrumenter n200/400/800 appariés sur sources figées : allocations,
   octets demandés cumulés et pic, capacités finales, pic par phase, temps
   de `append_batch`/scellement, B/A/T/P/C et plus grand lot. Puis n8k/16k/32k
   mono sur les régimes contractuels et s8/10/12. Une baisse des allocations
   n'est pas un gain temporel établi. G4 50k ensuite si ce delta le justifie.

Pas de prototype joint : une classe d'append isolée sans les mêmes gardes et
le lien banque/tour donnerait un doublon trompeur. Le prochain code utile
est la factorisation du validateur avec sa gate appariée, puis le raccord
`close_lot` privé, pas une reconstitution de l'oracle.

## Diagnostic proposé : représentants initiaux uniques par ordre

Le [dialogue de l'auditeur](../audits/DIALOGUE_COURANT.md) demande, à côté de
`representatives` et `resolver_meb_calls`, le nombre exact de clés de
représentants uniques par ordre. Sa
[note d'échelle](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_REFLEXION_ECHELLE.md)
motive le dédoublonnage statique. **Instrumentation proposée, non exécutée ici.**
Elle est distincte du plan de journal ; elle mesure un autre levier.

Définition : R_K est le nombre d'entrées dans `resolve` à l'ordre K et U_K
le nombre d'ensembles initiaux distincts de K indices géométriques triés.
L'identité inclut entrée/index et K ; un hash seul ne la définit jamais.
Ne compter ni les facettes successives de la descente, ni les semis de
`seed_closed_anchor`, ni seulement les cache-misses. Les K1 sont inclus,
même s'ils n'appellent pas la MEB.

La plus petite instrumentation sûre est un overlay privé sur une fermeture
source figée : observer `resolve` **après** son tri et sa garde de cardinalité,
**avant** le retour K1 et avant `resolver_cache.lookup` (L508 du header épinglé).
Le callback copie la clé initiale dans un buffer propre à l'ordre, puis rend
la main sans toucher aux sites, racines, ancres, cache ou compteurs du moteur.
Une observation de fin d'ordre trie lexicographiquement les clés entières
et compte les adjacentes distinctes, puis libère ce buffer. Aucun
`unordered_set` ni estimateur cardinal n'est nécessaire.

Commencer par une petite capture appariée nominal/observateur ; la collecte
en mémoire coûte O(K*R_K) mots et le tri O(K*R_K*log R_K), pas gratuitement.
À K<=10, une clé fixe de dix i32 occupe 40 octets logiques ; ne pas conserver
les occurrences de tous les ordres à la fois. Si ce stockage gêne un diagnostic
plus grand, écrire des runs triés bornés puis fusionner exactement leurs clés,
avec comptage, erreurs I/O et sceaux de complétude explicites. Ce catalogue
diagnostique ne devient pas une structure obligatoire du moteur produit.

Publier par ordre `representative_occurrences`, `unique_representative_keys`,
`duplicate_occurrences = R_K-U_K`, `resolver_meb_calls`, hits/semis du cache,
ainsi que source/entrée/index/configuration et digest du flux unique canonique.
Pour les trois derniers compteurs, prendre les différences avant/après K des
compteurs cumulatifs existants. Requérir R_K égal à la différence de
`representatives`, U_K<=R_K et la somme des R_K égale au total final. Ne publier
un diagnostic de tour complète qu'après succès global ; un ordre collecté
avant un échec appartient à un préfixe déclaré incomplet.

Portes minimales : présence de K1, un ordre avec doublons et un avec plusieurs
clés distinctes, clé contenant l'indice zéro, cache actif/désactivé donnant
les mêmes R_K/U_K, branche extra-shell, égalité de tous les compteurs
géométriques et du payload nominal/observé. Un petit juge indépendant par
comparaisons exactes par paires peut contrôler U_K sur micro-fixtures ;
il reste un test borné, pas le compteur à l'échelle. Le hook placé après
cache-hit doit être réfuté par R_K ; le comptage des semis ou des étapes de
descente doit aussi échouer. Aucun tableau ni hook n'entre dans les sources
de production, et aucun octet qualifié n'est modifié silencieusement.

Un observateur perturbe nécessairement le temps, la mémoire et l'allocateur :
**ses temps/RSS ne sont pas des mesures de performance du moteur nominal**.
L'égalité R_K/U_K avec/sans cache porte sur les entrées du resolver, pas sur
les coûts. U_K ne prouve pas qu'une seule MEB suffit par clé : une descente
peut demander plusieurs MEB, et les hits/semis évitent déjà certaines MEB
initiales. Il mesure exactement le partage des requêtes initiales ; le coût
résiduel d'une phase statique doit être qualifié séparément.

## Sources épinglées de l'analyse

Chemins relatifs à `morsehgp3D_v7/`. Lecture de code/documents seulement,
pas fermeture de compilation ni nouvelle preuve expérimentale :

```text
src/forest/full_ball_tower.hpp 910f45baea1750b11d2b34f40c893c9d1a34f950705cdb127ffa226de60f7b2e
src/forest/full_coverage_certificate.hpp 7608e70ec0bf7df7ed726ae2388a39e800ab2db35043b4ba42c619ceef13bac0
docs/CONTRAT_COUVERTURES_DATEES.md 4caf0178e43155a83a9d8236dff597aad9295cf917f5c27f26cf626de91ee08f
receipts/full_tower_residence_20260910/capture/README.md 1e56cf7ba2f6b55b876ad84032407679716701b18622bad13e9f1c1eae40f28f
receipts/ball_resolver_residence_20260910/README.md 74ff111dc1619b6738ef015d112b7b31912a1888b11e0cb577d9d88ec11b19d4
audits/receipts_tower_cost_review_20260910/README.md 3ed63286ab0a55fd729bd21e587876312d298312ae1233f0e1c16c472f478da6
audits/receipts_coverage_cpp_20260910/review.json 0e9b23d93ce92d8462f1dff050e5db0d8ec1ce4a868416bcad430622dd81ded5
audits/receipts_journal_guards_20260910/README.md a5f0149a51fe61c2f746f89bda1a786b52da90de777d722e7211b583a11fb0eb
```

Lecture complémentaire : [résidence massive](RESIDENCE_MASSIVE.md) (ses
chiffres du moteur C restent historiques),
[cache et préparation GPU](OPTIMISATIONS_CACHE_ET_GPU_20260910.md),
[contrat du journal v2](CONTRAT_COUVERTURES_DATEES.md),
[état de l'audit](../audits/ETAT_COURANT.md) et les deux premières parties du
manuscrit. Le dialogue a été lu au SHA
`07238d76231b9e3b78c6a679318abf2a467d439b67af5329bce972693b062a31` ;
la note d'échelle au SHA
`5b2acd641c40964214e323d1ba014f4eed4cd3487141c41e56e45eb92075e0d1`.
Ces textes vivants peuvent évoluer sans modifier les pins de cette analyse.

GCP non utilisé.
