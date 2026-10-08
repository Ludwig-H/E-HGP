# T1-d : frontières exactes, budget et publication

Lecture indépendante du commit `5f8e777cf`, sur la base `4171b2653`, le 8 octobre 2026. Aucun moteur, compilation,
GPU, donnée géométrique ni journal de campagne exécuté ou lu. Les sources viennent des objets Git ; les travaux R1
non commis sont préservés. Ce reçu ne qualifie ni CUDA ni les mesures locales annoncées. Le pilote et le juge sont
contre-lus séparément par l'auditeur coordinateur.

**Conclusion.** Aucun défaut de recomposition géométrique trouvé sous les invariants du catalogue et la validité
antérieure de F3/F4. Deux écarts de portée sont établis par le code : des métadonnées variables échappent au budget,
et la matérialisation finale des niveaux manque à la ventilation de la voie appareil par tranches. Le mur total
continue d'englober ce travail ; aucune mesure réelle n'est invalidée ici.

## Frontières et recomposition

Dans `Planner::walk`, une unité ne se ferme que si `key_order(unit.hi,b.lo)<0`. Une unité trop grosse est raffinée,
jamais coupée pour satisfaire seulement sa taille. Les extrema de deux intervalles suffisent : pour des clés
binaires64 positives finies, l'inégalité F4 reste vraie en diminuant la clé gauche ou en augmentant la droite.
Le raffinement ne peut franchir les frontières certaines du parent. Les unités de voisins ambigus sont donc
indivisibles, même lorsqu'elles traversent plusieurs cases. Un plateau trop lourd donne `memory_budget`.

Une coupe certaine sépare STRICTEMENT tous les niveaux exacts de gauche et de droite. Aucun niveau égal, doublon
d'émission ou chaîne à réparer ne peut traverser cette coupe. Le tri exact local conserve l'ordre secondaire des
positions de S*, donc aussi le premier support choisi pour les mots NON réduits de chaque niveau. La concaténation
rend le même ordre global ; ajouter le nombre de niveaux antérieurs rend les mêmes rangs denses. La garde de
`slice_finish` précède l'addition aux rangs ; la somme des incidences est en u64. Le CSR reçoit la base d'incidence,
et sa dernière case est écrite une fois avec le total vérifié. Enfin `host_table` emploie les mêmes clés de support
et la même sentinelle que `TableKeyKernel`, dans les indices de boules globaux. Ces arguments ne recertifient pas
les bornes numériques antérieures de F3/F4 ni des entrées arbitraires malformées.

Les téléchargements/envois CUDA attendent les copies avant de rendre la main ; `stream_out` attend également ses
copies en cas d'erreur. Les temporaires de rassemblement vivent donc jusqu'à la fin de leur envoi. `chunks->clear()`
n'arrive qu'après toutes les tranches ; la fermeture ne relit ensuite que le nombre de sites, pas le span invalidé.
Le repli CPU est réservé aux refus mémoire survenus avant cette destruction. Une sortie partielle reste privée.
La nouvelle libération CUDA synchronise avant de rendre un tableau. Son retour `cudaFree` n'est toutefois pas
contrôlé : recommandation de le traiter par `check` avant de vider le pointeur et sa réservation. Aucun incident
CUDA n'est démontré ; les portes à transit simulé n'éprouvent pas cette branche du runtime.

## Budget : trois conteneurs de métadonnées non comptés

`slices.cpp:107–114` fait grandir `std::vector<Bin> slices` sans réservation de budget. Pour S tranches, ses seuls
éléments vivants occupent au moins `40*S` octets ; capacité et coexistence lors d'une croissance peuvent ajouter du
stockage. `SliceWords::words` fait de même pour S descripteurs de Buffer (`finish_slices.hpp:126–132,222–225`).
La nouvelle arène de la voie appareil emploie aussi `std::vector<Chunk>` (`device_pipeline.hpp`, `take_arena`).
Les contenus de ces Buffer sont bien comptés ; leurs tableaux de descripteurs ne le sont pas. Le mécanisme Chunk
existait déjà côté CPU : ce reçu ne le présente pas comme une invention de T1-d.

La règle `core/buffer.hpp:24–28` demande un Buffer pour tout tableau dépendant de l'entrée, et `slices.hpp:16`
annonce que tous les tableaux le sont. Capturer `bad_alloc` ferme l'échec système, sans soumettre la croissance au
budget. Ainsi `peak<=limit` ne prouve pas que ces métadonnées sont couvertes. Aucun dépassement RSS concret n'est
mesuré et aucun poids dominant n'est attribué à ces vecteurs dans les cas annoncés. Correction proposée : stockage
compté du plan et des descripteurs, en réservant aussi la coexistence ancien/nouveau pendant une croissance ;
injection d'un refus à cette réservation, avec contrôle du rendu et du réemploi du contexte.

Autre limite déclarable du dimensionnement : `assign_slices` alloue `8*ceil(B/65536)*S` octets de comptes, en plus
des indices ; chaque histogramme emploie jusqu'à `40*65536*W` octets privés, plus `40*65536` octets fusionnés (les histogrammes
fusionnés des ancêtres restent vivants pendant le raffinement). Les raffinements rescannent
les B clés. Le code contrôle ces allocations, mais la mémoire de travail de la planification n'est pas simplement
celle d'une tranche. Aucun coût global linéaire ni gain de latence n'est déduit de ce découpage.

## Publication : un travail présent dans le mur mais absent du détail appareil

La voie complète transfère ses niveaux avec un consommateur `LevelChunk` marqué publication : l'exécuteur les
impute à `TransferMeter.publish_ns`. T1-d transfère des mots avec un simple `CopyChunk` (quatre segments marqués
`false`), puis appelle `materialize_levels` dans `slices_close`, AVANT le début de `table_watch`, sans montre ni
compteur. Aucun compteur englobant n'est ensuite ajouté dans `device_diagnostics` : la conversion des niveaux,
leur allocation et la destruction des mots se retrouvent dans le résidu du mur. Le rassemblement hôte préalable
de chaque tranche est lui aussi hors des montres de `slice_finish` ; ce détail interdit de lire ses sous-étapes
comme une partition exhaustive du travail. Côté CPU, `Assembly::finish` utilise le résidu du mur et les inclut.

Correction proposée avant interprétation de profils : attribuer une fois la fermeture des niveaux à la publication
appareil, et le rassemblement à l'assemblage ; conserver l'absence de double compte sur CPU. Le mur de C et les
compteurs logiques sont inchangés. Ne pas sommer ce détail comme s'il qualifiait le temps DMA.

## Pic appareil et portée des portes

Avec `--budget-appareil`, `catalogue_probe` crée un budget propre ; sa ligne `tranches.device_peak` publie son pic
cumulatif depuis la création, sans remise à zéro entre les passes. Garde sûre : capacité courante du catalogue =
`tranches.device_bytes`, capacité ≤ pic ≤ limite commandée, pics non décroissants. Sans budget séparé, cette ligne
publie conventionnellement zéro pour le pic ; ce zéro ne mesure pas la mémoire GPU. Aucune égalité pic/capacité
ni somme des pics ne convient. Les tableaux rendus en fin de tranches peuvent laisser une faible capacité courante.

Les portes `slices_plan`, `slices_identity`, `slices_cpu_budget`, `slices_device_budget` et `slices_device_reuse`
contiennent les témoins pertinents de frontière, identité et réemploi. Leur source est lue, elles ne sont pas
rejouées ici. `device_open_budget` peut réussir localement en annonçant un appareil indisponible ; seul son chemin
positif avec appareil qualifie CUDA. Les faux refus dus à l'estimation des capacités résidentes sont déjà déclarés
au §12 du contrat : aucune promesse de réussite dès que le seul catalogue final tient en mémoire.

Rejeu de l'inventaire Git et des ancrages (lecture source seulement) :

```sh
python check.py --repo /workspaces/E-HGP
python -O check.py --repo /workspaces/E-HGP
```

Résultat identique à `results.json`. `capture.json` épingle les sources ; aucune source complète n'est copiée.
