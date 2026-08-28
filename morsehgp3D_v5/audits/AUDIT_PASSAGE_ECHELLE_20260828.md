# Audit ciblé — conception du passage à 10–30 millions de points

- **Dernier commit fonctionnel relu :** `ab2c2563` ; `docs/ECHELLE.md` a été
  introduit au pin `9fba11a5` et n'a pas changé depuis.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Périmètre :** preuve d'architecture, budgets RAM/disque/temps, produit de
  sortie, reprise et porte du préfixe. Aucun run 1 M, 10 M ou 30 M n'existe.

## Verdict

La direction générale est bonne : ne pas matérialiser une mosaïque globale,
garder l'index utile, streamer les objets output-sensitive, préflighter chaque
rôle et ne publier qu'un statut terminal. Le préfixe K = 1..5 est également une
réduction de périmètre utile.

En revanche, **le document ne dimensionne pas encore une exécution à 10 M**.
Les affirmations « tient en RAM », « tuile bornée », « reprise au lot suivant »
et « 6–7 h » dépendent de quatre mécanismes qui ne sont ni prouvés ni mesurés :

1. la sonde de facettes vivantes n'est pas un majorant de l'état du fold ;
2. l'oubli par dernière incidence n'est pas exact avec une empreinte 64 bits,
   un compte `u8` et un union-find non fermé ;
3. le flux proposé ne porte pas encore explicitement tout le payload courant ;
4. le modèle d'E/S suppose un disque qui n'est pas celui de la G4 décrite.

Le document doit donc rester une **hypothèse d'architecture falsifiable**, pas
un plan de capacité. Il ne faut pas lancer l'implémentation du compactage UF à
partir des 40 facettes/point annoncées.

## P1 — la mémoire du fold n'est pas bornée par la sonde actuelle

`profil_vivantes` construit un compteur sur toutes les facettes, traite un lot
entier, puis échantillonne seulement après ce lot
(`src/forest/fold.hpp:571-696`). Une facette née et terminée dans le même lot
n'entre jamais dans `live_max` ; un plateau mono-lot peut donc annoncer zéro
alors que tout le lot a dû être résident. La sonde ignore aussi :

- les racines et ancêtres union-find nécessaires aux membres encore actifs ;
- la clé canonique d'une composante ;
- le payload final et les buffers du lot en cours ;
- la fermeture transitive exigée avant de compacter les parents.

Le chemin produit alloue encore `FidState` pour toutes les facettes. Les 18,1
facettes/point observées à K = 10 sont une borne basse descriptive, pas une loi ;
la marge arbitraire 2,2 ne la transforme pas en majorant de 40 facettes/point.
Les postes « état vivant 30 Go/90 Go » et le verdict « tient en RAM » ne sont
donc pas reçus.

La mesure minimale utile doit suivre, pendant chaque lot : facettes distinctes
touchées, fermeture des parents/racines/canoniques, état réutilisable après le
lot, octets alloués et pic externe. Deux fixtures doivent précéder le code : un
plateau massif mono-lot et une chaîne où une facette sans incidence future reste
ancêtre d'un membre futur.

Le champ `rss_mb[4]` ne corrige pas ce défaut : il est lu après réduction,
publication et libérations partielles (`src/pipeline/run.hpp:370-371`). Sur
`uniform` 200 k, le reçu donne 66 257 Mio à ce palier, alors que GNU time mesure
75 828 184 Kio, soit environ 72,3 Gio. Renommer le palier
`rss_after_publish_sample` et conserver `ru_maxrss` comme autorité du pic.

## P1 — « dernière incidence » exige une identité et une fermeture exactes

La table proposée « empreinte 64 bits → compte u8 » n'est pas une structure
exacte :

- deux `FacetKey` peuvent partager une empreinte ; le fold actuel emploie
  l'empreinte seulement pour adresser puis compare la clé exacte ;
- une facette peut avoir plus de 255 incidences ; aucune borne `u8` n'est
  démontrée et l'instrument actuel utilise justement `u32` ;
- 64 bits + 8 bits valent déjà 9 octets bruts, pas 7, avant résolution des
  collisions, facteur de charge et métadonnées ;
- une facette non racine peut rester un parent union-find d'une facette future.

À 912 facettes/point pour K = 10, le seul minimum de 9 octets extrapole déjà à
82 Go à 10 M, contre 64 Go dans le tableau, sans être exact.

Première architecture sûre proposée : produire par tri externe exact des
enregistrements `(FacetKey, rang_evenement)`, attribuer les identifiants stables
après comparaison complète de la clé et calculer une dernière position en
`u64`. Le reduce peut ensuite expérimenter l'éviction, mais seulement après une
opération explicite de compression/reroot des membres conservés et une preuve de
préservation du canonique. Un mutant `drop_at_last_direct_incidence` doit être
tué par la chaîne adversariale. Si cette fermeture ne reste pas bornée, le
résultat négatif invalide le fold compact proposé ; il ne faut pas le masquer
par une empreinte probabiliste.

## P1 — définir le flux avant de calculer sa taille ou son digest

Le payload `mhgp5-forests-horizontal-v1` contient, par K, `batch_levels`, les
deltas, toutes les `facet_keys` et `final_canon_fid`
(`src/pipeline/run.hpp:106-118`). `facet_hierarchy_stream` doit dire comment ces
quatre objets sont reconstruits, notamment les lots sans delta et la partition
finale. « Deltas et naissances » n'est pas encore un schéma de fichier.

La porte correcte aux tailles bornées est :

1. décoder le stream v5 vers un `ForestResult` complet ;
2. comparer `batch_levels`, `deltas`, `facet_keys` et `final_canon_fid` élément
   par élément au chemin résident ;
3. calculer sur l'objet reconstruit le digest v4 et le comparer au pin v4 ;
4. calculer séparément le digest d'intégrité du stream v5.

Un hash tagué `mhgp5-digest-v1:stream` ne peut pas être littéralement égal au
hash d'une autre sérialisation taguée `mhgp4-digest-v1:forest`. Au-delà de la
zone recoupée, le digest v5 atteste l'intégrité du fichier, pas son exactitude.

Le budget de sortie est également trop bas. Au reçu `uniform` 200 k :

- 467 881 200 naissances × 41 octets de `FacetKey` ;
- au moins deux parents pour chacun des 50 366 416 nœuds ;
- 62 826 653 deltas × au moins 105 octets fixes du wire v4.

Cela donne déjà **29,91 Go à 200 k, donc au moins 1,495 To à 10 M** par simple
extrapolation ×50, avant parents des croissances, cadrage, checksums et reprise.
Un wire plus compact est possible, mais doit être spécifié, versionné et mesuré.
Les « 1 To de sortie » et « disque ≥ 2 To » sont des scénarios sans marge, pas
des bornes de capacité.

## P1 — le routage par centre ne borne pas une tuile

Le théorème de centre prouve que deux émissions de la même `BallKey` sont
co-localisées. Il ne prouve pas qu'une cellule contient environ 200 k boules :
une famille cosphérique ou un centre chaud peut concentrer un nombre arbitraire
de clés et d'émissions. Il ne permet pas non plus de finaliser le RLE d'une tuile
avant que tous les rectangles aient terminé.

Le § 4.3 mélange en outre deux architectures :

- si chaque rectangle WSPD est traité exactement une fois puis ses émissions
  routées, aucun halo ni voisinage à rayon maximal n'est nécessaire ;
- si les rectangles sont répliqués vers des voisins, il faut une preuve de
  complétude et d'exact-once propre à cette réplication.

La variante la plus simple à recevoir est la première : passage append-only de
tous les rectangles, routage déterministe, barrière globale, puis tri externe
et RLE par clé exacte. Chaque seau doit pouvoir déborder sur disque et être
sous-partitionné récursivement par la clé ; sa mémoire vient d'un budget, jamais
d'une hypothèse d'occupation. Le Morton du centre peut rester une clé de
localité, mais pas l'autorité d'une borne.

Le poste « rectangles ~4 Go » n'est pas non plus relié au reçu. La seule q4
contient 21 798 342 `AliveRect` à 200 k ; à au moins 16 octets chacun, une
extrapolation linéaire ×50 vaut environ 17,4 Go. Un traitement par vagues peut
réduire ce pic, mais c'est alors cette nouvelle vague et son maximum mesuré qui
doivent alimenter le tableau.

## P1 — choisir une reprise honnête

Le manifeste proposé (pin, hashes, K et lot) ne suffit pas à reprendre au lot
suivant. Il manque au minimum les curseurs de fusion des runs, le heap, les
parents/canoniques/flags du fold, les dernières incidences, l'état du digest,
les offsets de sortie et le protocole d'atomicité. Une coupure entre écriture et
mise à jour du manifeste peut sinon dupliquer ou perdre un segment.

Pour un premier jalon, déclarer **`resume=replay_current_K`** :

- chaque K terminé est écrit dans un fichier temporaire, `fsync`, validé, puis
  renommé atomiquement et inscrit au manifeste ;
- le K interrompu n'est jamais publié et est rejoué depuis le début à partir des
  runs immuables ;
- le préflight vérifie qu'un K isolé tient dans la durée de session.

Cette politique est simple à falsifier avec `SIGKILL` à chaque frontière. Une
reprise intra-K ne viendra qu'avec un checkpoint complet, versionné et mesuré.

## P1 — le disque et le modèle de temps ne décrivent pas la VM actuelle

`gcp-migration/deploy.sh` crée un boot **Hyperdisk Balanced** de 100 GB,
provisionné à 3 600 IOPS et **290 Mio/s**. Une G4 ne supporte pas les Persistent
Disk zonaux/régionaux ; Hyperdisk Balanced est son seul type de boot. Google
indique aussi que le plafond partagé de `g4-standard-48` est 1 600 Mio/s et que
le débit Hyperdisk est half-duplex :

- [types de disque et limites G4](https://docs.cloud.google.com/compute/docs/accelerator-optimized-machines#supported_disk_types_for_g4_instances) ;
- [formules et limites Hyperdisk Balanced](https://docs.cloud.google.com/compute/docs/disks/hd-types/hyperdisk-balanced).

Le « PD-SSD à 1–2 Go/s » de `ECHELLE.md` n'est donc ni la cible courante ni un
débit reçu ; 2 Go/s dépasse même le plafond Hyperdisk Balanced de cette machine.
Avec la borne basse de sortie ci-dessus, le minimum séquentiel est environ
1,495 To de sortie + 230 Go de runs écrits + 230 Go relus, soit **1,955 To**.
À 290 Mio/s, cela représente déjà **107 minutes** idéales d'E/S, sans lecture
d'entrée, comptes, hashes, checkpoint, partage read/write ni marge.

Le modèle doit prendre en paramètres le type, la capacité, IOPS, débit
provisionné et débit `fio` réellement mesuré. Toute modification de disque GCP
reste une mutation gardée à ajouter aux scripts du dépôt ; elle n'est pas une
condition implicite de l'audit.

Enfin, le fold `uniform` mesuré à 115,0 s extrapole linéairement à 1,60 h, et le
reduce cumulé à 128,2 s à 1,78 h. Le tableau annonce 3–4 h sans nommer le facteur
supplémentaire. La génération 200 k vaut 68 s sur `uniform`, 178 s sur
`eight_clusters` et 244 s sur `scanline` : « 6–7 h » est au mieux un scénario
`uniform`, pas une loi de famille. Publier formule, facteur de tuilage/streaming,
fourchette et sensibilité au débit ; ne promettre aucune session de 8 h avant un
pilote 1 M.

Le scénario est actuellement CPU sur une VM munie d'un GPU : aucune résidence
VRAM, aucun volume transféré ni poste CUDA n'est budgété. Soit le nommer ainsi,
soit définir et mesurer une génération réellement device-résidente avant de
faire contribuer les 96 Go de VRAM au plan de capacité.

## P2 — porte du préfixe et cadre documentaire

La propriété de préfixe est plausible et les digests observés sont positifs,
mais la porte ne compare que `digest_forest` et trois cardinalités. Or le digest
v4 omet `batch_levels`, et aucun digest d'événements n'est comparé. Étendre la
porte aux événements canoniques et aux niveaux de tous les lots, ajouter un
plateau non trivial et distinguer `smax_requested` de `smax_effective`.

Le champ imprimé `profil=complet_k10/prefixe_k5` collisionne avec le profil
normatif `quantized_u16_input_only`. Employer par exemple
`tower_scope=profile_complete_k10` ou `tower_scope=prefix_k5`. « Complet » doit
toujours signifier complet **dans le profil K ≤ 10**, pas tour mathématique
illimitée.

Enfin, le cadre de `ECHELLE.md` omet `mode`, et le § 2 dit reprendre les
conclusions v3/v4 « telles quelles ». La doctrine v5 impose au contraire de les
traiter comme hypothèses différentielles à requalifier. Conserver leurs
contre-fixtures et ordres de grandeur, mais épingler séparément chaque preuve ou
mesure v5.

## Ordre de travail proposé à Claude

1. Spécifier `facet_hierarchy_stream-v1`, son décodeur, son autorité terminale,
   son wire et `resume=replay_current_K` ; recalculer les bornes de sortie.
2. Remplacer le comptage fingerprint/`u8` par un tri externe de clés exactes et
   tuer les fixtures mono-lot/chaîne avant toute éviction.
3. Livrer le routage append-only exact-once avec spill et barrière globale ; ne
   pas implémenter de halo dans ce jalon.
4. Comparer le stream décodé au `ForestResult` complet sur 8 k–200 k, puis faire
   une campagne 1 M avec pics externes, occupation des seaux et E/S mesurées.
5. Refaire seulement alors le tableau RAM/disque/temps et décider si 10 M K = 5
   ou K = 10 est la prochaine porte.

Ces étapes conservent le principe utile du document, tout en transformant les
quatre hypothèses critiques en résultats réfutables avant de consommer une
session longue.

GCP non utilisé pour cet audit.
