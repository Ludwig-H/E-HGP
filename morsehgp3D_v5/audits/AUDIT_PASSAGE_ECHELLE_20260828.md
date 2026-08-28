# Audit de résolution — passage à 10–30 millions de points

- **Derniers commits techniques relus :** `17ab71e0` pour
  `docs/ECHELLE.md` révisé et `ba31c169` pour la porte de préfixe étendue
  aux événements et niveaux de lots.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Périmètre :** résolution constructive des verrous T3–T6, amont externe,
  payload et reprise. Aucun résultat 1 M, 10 M ou 30 M n'est revendiqué.

## Verdict utile à Claude

Le plan révisé est nettement meilleur que sa première version. Il abandonne le
tuilage spatial du fold et ses halos, nomme le payload, distingue le digest du
flux du digest v4, relève les budgets de sortie et de disque, retient
`resume=replay_current_K` comme première reprise réaliste et fait du préfixe
K ≤ 5 un jalon autonome. Ces décisions sont conservées.

Le verrou restant n'exige pas un reroot compliqué de l'union-find. Il peut être
supprimé par une représentation plus simple :

> conserver uniquement les alias des facettes qui auront encore une incidence,
> stocker chaque composante comme une liste de ces alias et séparer sa racine
> sémantique de son conteneur physique.

Avec une prépasse PREMIÈRE/DERNIÈRE exacte sur les `FacetKey` complètes, cette
représentation donne un vrai majorant mémoire : à toute frontière de lot, toute
composante conservée contient au moins une facette encore vivante. Ainsi le
nombre de composantes résidentes est au plus le nombre de facettes vivantes.
Cela remplace utilement T3, T4 et T6 par un invariant directement testable.

### Réception positive de la porte de préfixe

Le commit `ba31c169` ferme substantiellement l'ancien angle mort du digest
v4 : la signature couvre le tuple événement complet et la séquence de tous les
`batch_levels` pour chaque K. Une construction Release détachée du worktree
actif donne 6/6 CTests `mhgp5_prefix_*` en 24,49 s ; le mutant historique
sort au code 4. La fixture de réseau quantifié contient effectivement 535
événements K = 5 pour 408 lots : la porte n'est pas vacue.

Trois renforcements sont utiles mais ne bloquent pas ce jalon :

- `min_plateau_batches` mesure actuellement `events - batches`, pas le
  nombre de lots de multiplicité supérieure à un ; renommer ce seuil
  `min_tie_excess` ou compter réellement ces lots ;
- `uniform n=48 coord=14` est un réseau déterministe riche en ex æquo, pas une
  fixture explicitement cocirculaire ; corriger le libellé ou construire cette
  géométrie ;
- le mutant de profondeur tue déjà l'ancien digest. Ajouter deux petits tampers
  dédiés, l'un ne modifiant qu'un champ d'événement omis par le digest v4,
  l'autre seulement `batch_levels`, donnera des dents propres aux nouveaux
  contrôles.

## Solution 1 — fold vivant sans ancienne forêt union-find

### État minimal

Pour un ordre K, le réducteur conserve :

- un `Alias` par facette réutilisable, avec `fid_u64`, clé, `seen`,
  rôles du lot, pointeur de composante et liens intrusifs ;
- un `Component` par composante ayant au moins un alias, avec
  `logical_root_fid`, `canon_fid`, `historical_mass` et la liste de ses
  alias ;
- les scratchs du lot : facettes touchées, parents pré-lot, naissances et
  sorties post-lot.

`logical_root_fid` reproduit exactement la règle actuelle « la racine du
composant de `first` absorbe ». `canon_fid` reste le minimum de toutes les
facettes historiques de la composante. Le record qui porte physiquement la
liste n'a aucune autorité sémantique.

### Union exacte et compacte

Pour chaque union ordonnée `unite(first, other)` :

1. mémoriser comme nouvelle racine logique celle du composant de `first` ;
2. garder physiquement le record de plus grande `historical_mass` ;
3. relier les alias du plus petit record au plus grand ;
4. écrire le minimum des deux canoniques et la somme des masses ;
5. détruire le record physique devenu vide.

Le choix small-to-large peut donc être opposé à l'absorption sémantique sans
changer le résultat. Chaque alias déplacé entre dans un composant dont la masse
historique a au moins doublé, d'où au plus un nombre logarithmique de
relocalisations par alias. Le tri des deltas par `logical_root_fid` reproduit
le tri actuel de `post_list`, même lorsque le gagnant physique est l'autre
composant.

Le lot doit rester une opération en deux passes :

1. calculer tous les rôles et figer les composantes/canoniques pré-lot ;
2. rejouer ensuite les unions dans l'ordre total des événements.

Une réduction événement par événement changerait les multifusions à niveau
égal et n'est pas admissible.

### Libération prouvable

Les alias dont `last_batch == b` sont supprimés seulement après l'émission du
lot `b`. Si une composante n'a alors plus aucun alias, aucun événement futur
ne peut l'atteindre : une connexion future devrait nécessairement réutiliser
une de ses facettes, en contradiction avec leurs dernières incidences. Le
record est donc définitivement libérable.

Pendant le lot, les facettes telles que `first_batch <= b <= last_batch` sont
comptées. Après le lot ne subsistent que celles telles que
`first_batch <= b < last_batch`. Les facettes nées et mortes dans le même lot
sont ainsi bien budgétées, contrairement à la sonde historique échantillonnée
après le lot.

Conséquences utiles :

- aucune chaîne de parents morte, aucun reroot et aucun refcount d'ancêtres ;
- `components <= aliases <= peak_live_exact` ;
- les listes touchées, parents et naissances sont bornées par les alias du lot ;
- la partition finale n'impose pas de garder les membres morts en RAM, si son
  rejeu depuis le catalogue et les deltas est reçu séparément.

## Solution 2 — PREMIÈRE/DERNIÈRE et préflight exacts

L'empreinte 64 bits peut servir à router des fichiers, mais jamais à décider
l'identité. Aucune marge fixe appliquée au pic par empreinte ne constitue un
majorant déterministe : une collision adversariale peut agréger un nombre
arbitraire de clés.

La prépasse exacte par ordre est :

1. attribuer un `event_rank_u64` au flux déjà trié par
   `(ExactLevel, BallKey, emit_rank)` ;
2. émettre chaque incidence sous la forme logique
   `(FacetKey complète, event_rank_u64, slot)` ;
3. partitionner éventuellement par hash pour les E/S, puis trier et comparer
   chaque partition par `FacetKey` complète ;
4. fusionner les partitions en ordre lexicographique, attribuer les
   `fid_u64` et marquer exactement une incidence FIRST et une LAST ;
5. retrier le join `(event_rank, slot, fid, flags)` dans l'ordre du fold.

Le pic inclusif par lot est ensuite calculé sans heuristique :

```text
live += first_count[batch]
peak = max(peak, live)
live -= last_count[batch]
```

La première porte doit injecter un hash constant. Le résultat, FIRST/LAST et le
pic doivent rester identiques ; un mutant `lifetime-by-hash-only` doit
diverger.

Cette exactitude a un coût de wire. La ligne actuelle « 620 Go par empreinte +
position » ne dimensionne pas le tri de clés complètes. Le jalon L2 doit graver
la taille sérialisée de `FacetOccurrenceWire`, le nombre d'octets écrits/lus
et le facteur temporaire du tri K par K, puis recalculer le besoin SSD. Une
compression préfixe après tri est possible ; elle ne doit pas remplacer la
comparaison exacte.

## Solution 3 — prouver d'abord que les deltas suffisent

Un décodeur borné existe déjà dans
[`tests/forest_judge.cpp`](../tests/forest_judge.cpp) : il rejoue les deltas
pour reconstruire la partition sans consulter `final_canon_fid` comme
autorité. Le premier petit commit utile est de l'extraire en une porte
indépendante :

```text
catalogue de facettes + deltas -> final_canon_fid reconstruit
```

La porte compare ensuite tous les champs au `ForestResult` résident et
calcule le véritable `mhgp4-digest-v1`. Elle doit dépasser les seules petites
instances du juge et inclure les contre-fixtures ci-dessous.

Le wire massif peut alors référencer les facettes par `fid_u64` et contenir,
par K :

- le catalogue des `FacetKey` uniques, trié une fois ;
- les lots critiques avec `legacy_batch`, niveau exact et deltas
  `output/parents/born` ;
- un sidecar explicite et versionné si les niveaux des lots sans delta restent
  contractuels ;
- un digest logique indépendant du découpage physique ;
- dans le manifeste, tailles et SHA-256 des fichiers physiques.

Le digest v4 n'est pas un format massif : `ForestResult::final_canon_fid` et
`mhgp4-digest-v1` sérialisent des `u32`. L'extrapolation K = 10 à 10 M
dépasse largement `UINT32_MAX`. Le convertisseur v4 reste donc une porte
différentielle bornée ; le flux à grande échelle doit avoir son wire et son
digest u64 propres. Aucun cast silencieux n'est acceptable.

## Solution 4 — amont externe plus simple

Le Morton du centre reste une bonne optimisation de localité, mais il n'est pas
nécessaire à l'exactitude et ne borne pas un seau chaud. Le premier amont
streamé peut être plus direct :

1. traiter les rectangles par vagues bornées et écrire des runs de
   `BallCandidate` triés avec le comparateur produit actuel ;
2. effectuer une fusion externe globale et le RLE exact sur la clé complète ;
3. calculer `digest_balls` au passage ;
4. préfiltrer, censer puis expanser chaque boule unique une seule fois ;
5. envoyer chaque événement directement dans le buffer/run de son K avec
   `BallKey` source et `emit_rank` ;
6. trier extérieurement chaque flux K par sa clé totale.

Cette couture évite de matérialiser d'abord environ 1 To de `BallData` et
évite la ré-expansion répétée par ordre. Le centre Morton pourra ensuite
partitionner le census pour la localité, avec spill obligatoire, sans devenir
une autorité de complétude ou de capacité.

La première porte de cet amont est petite : découper les mêmes candidats en
runs de tailles 1, 2, 3 et 7, fusionner/RLE, puis comparer au
`rle_candidates` résident. Elle ferme une couture réelle avant tout pilote
SSD.

## Fixtures qui résolvent les risques au lieu de les reporter

1. **Étoile K = 1 de 300 arêtes à niveaux croissants** : tue tout compteur
   d'incidences `u8`, conserve un canonique dont la facette est morte et force
   souvent gagnant physique et racine logique à différer.
2. **Chaîne K = 1** : événements `{0,1}`, puis `{0,2}` ; l'ancienne racine
   n'a plus d'incidence propre mais la composante continue.
3. **Deux simplexes K = 2 partageant une facette** : ferme l'analogue
   d'ordre supérieur.
4. **Plateau mono-lot** : un seul événement q3 donne trois facettes
   FIRST = LAST et un pic transitoire de trois, jamais zéro.
5. **Grand composant absorbé logiquement par un singleton** : le stockage
   small-to-large garde le grand record, tandis que `logical_root_fid` doit
   rester celui du singleton.
6. **Frontières externes** : même facette dans plusieurs runs et hash forcé
   constant.

Mutants prioritaires : `last-mark-shifted`, `free-on-absorb`,
`root-key-mutable`, `canon-not-min-on-union`,
`lifetime-by-hash-only` et `physical-root-is-logical-root`.

## Reprise minimale sûre

Conserver `resume=replay_current_K` :

1. entrées de phase immuables et hashées ;
2. sortie du K courant dans un temporaire unique ;
3. `fdatasync` ou `fsync`, relecture et validation du hash ;
4. renommage vers le nom final puis `fsync` du répertoire ;
5. manifeste temporaire synchronisé, renommé atomiquement, puis répertoire
   synchronisé.

Une coupure laisse au pire un orphelin non référencé. Le K courant est rejoué ;
un K précédent n'est repris que si son manifeste est `committed`. Ne pas
annoncer de reprise au lot tant que toute la map vivante, les composants,
l'état du digest et les offsets de sortie ne sont pas sérialisés.

## Ordre de commits proposé

1. **Porte de rejeu** : promouvoir
   `catalogue + deltas -> final_canon_fid`, puis graver les six fixtures.
2. **Réducteur vivant en RAM** : FIRST/LAST par clé exacte dans une
   `std::map`, composants small-to-large et égalité complète avec le résident.
3. **Coutures externes** : RLE multi-runs, lifetime avec hash constant, puis
   join retour vers les événements.
4. **Payload/reprise** : wire u64, digest logique et publication atomique par K.
5. **Pilote 1 M** seulement après mesure des octets, du pic inclusif et du
   débit du disque réellement attaché.

Cet ordre donne à Claude trois petits commits falsifiables avant le chantier
SSD. Il conserve son architecture générale tout en retirant les deux
inconnues les plus risquées : la fermeture union-find et la marge
probabiliste.

## Corrections documentaires restantes

- **fermés à `17ab71e0` :** le cadre contient désormais `mode`, la v3/v4
  est requalifiée comme différentiel et le comptage exact par clé complète est
  retenu dans le § 4.3 ;
- **fermé à `ba31c169` sur les familles et tailles gravées :** la porte de
  préfixe signe les événements canoniques et tous les `batch_levels`, avec
  des ex æquo non vacants ; les trois renforcements ci-dessus restent P2 ;
- remplacer encore T4 au § 6 : il décrit toujours une empreinte et une marge,
  en contradiction avec le comptage exact déjà retenu au § 4.3 ;
- remplacer T6 par l'invariant `components <= live_aliases` du réducteur
  small-to-large ;
- renommer T8 : le découpage déterministe n'est pas un théorème d'équilibre ;
- retirer « surgénération aux frontières de seaux » si chaque rectangle est
  émis exactement une fois ;
- ne plus promettre le digest v4 au-delà du domaine u32 ;
- recalculer le poste SSD PREMIÈRE/DERNIÈRE à partir du wire exact de clés
  complètes, et non des 16 octets empreinte/position encore tabulés.

GCP non utilisé pour cet audit.
