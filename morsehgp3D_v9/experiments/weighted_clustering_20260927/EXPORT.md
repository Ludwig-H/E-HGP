# Export du catalogue Gabriel pondérable — 27 septembre 2026

Consommateur expérimental CPU/u18 distinct. Aucun moteur, ancien adaptateur,
build ou reçu n'est modifié. Ce produit expose les incidences nécessaires aux
masses du chapitre 9 ; il ne prétend pas les reconstruire depuis le seul T_K.
Il ne qualifie ni un temps FULL/GPU, ni une croissance globale, ni une supériorité
sur un autre clusterer.

## Interface figée

`native_weighted_export --input XYZ.u32le --k K [--workers W] [--verify-coverage]`
émet un objet JSON sur stdout ; erreur = code 2, diagnostic sur stderr, sans
objet de réussite. Même entrée u32le/XYZ, points distincts, coordonnées
0..262143 et IDs = rang d'entrée que l'ancien adaptateur. K∈[1,10], K≤n.
`--verify-coverage` est un rejeu coûteux de toutes les coupes, réservé aux petits
contrôles ; il n'est pas requis pour l'énumération des cofaces.

Schéma `mhgp9_weighted_catalogue_export_v1` :

- `native` : JSON `mhgp9_fixed_k_export_v1` inchangé, produit par inclusion
  explicite de l'ancien adaptateur avec renommage de son `main`. Les champs
  `facet_catalogue_exported:false` et `scope` de ce sous-objet décrivent **ce
  sous-objet seul**, pas le nouveau catalogue frère.
- `catalog_universe: gabriel_complete_boundary` ; `beta_unit:
  squared_radius_grid_units`. Les beta sont des rayons **au carré**, fractions
  exactes `{num: chaîne décimale, den: chaîne décimale}`. Ne pas les employer
  directement comme un rayon pour ψ(r)=r^(−z).
- `catalogue` : **toutes** les `catalogue_balls` rendues par la chaîne avec
  `keep_catalogue=true`, y compris celles sans effet topologique ou sans coface
  de l'ordre demandé. Fenêtre |I|+q_min≤K+1 ; ce n'est pas le catalogue de tous
  les ordres au-delà de K. Chaque ligne porte `id`, clé primitive exacte
  `key:{a,b:[b0,b1,b2],c}` en chaînes décimales signées pour
  a|x|²+b·x+c, `beta`, `q_min`, `interior`, `shell` et
  `minimal_support_masks`. I et shell sont triés par **PointId** d'entrée.
  Un bit de masque indexe cette shell triée, jamais un rang Morton.
- `cofaces` : `{vertices, beta, ball, shell_mask}`, toutes les (K+1)-parties
  Gabriel, triées lexicographiquement. `ball` est l'ID de catalogue ; `vertices`
  est trié et distinct. Tout doublon serait une erreur, jamais fusionné.
  Le consommateur construit F=∂C en supprimant successivement chacun des K+1
  sommets ; aucune facette hors de cette frontière complète n'est ajoutée.
- `stats` : volumes, candidats effectivement examinés et temps additionnel
  `index_and_enumeration_ms`. `support_table_slots` somme 2^|shell| seulement
  pour les coquilles dégénérées ; `cardinality_candidates` compte les masques
  de cardinalité admissible effectivement testés, avec le raccourci régulier.
  `global_combinations_enumerated` vaut zéro sur cette voie de production.

Les temps `native.times_ms` ne comprennent pas l'index additionnel de conversion
des IDs, l'énumération et la sérialisation. Les condensés de catalogue sont
hors du chrono rapporté par la chaîne, mais dans son mur externe. Aucun temps
de cette expérience ne devient automatiquement comparable aux anciens lots.

## Critère exact et complétude relative

Pour une boule B du catalogue, I est son intérieur strict complet, U sa
coquille complète et c son centre. Une coface Gabriel de miniboule B est
exactement σ=I∪T avec T⊆U, |T|=K+1−|I| et c∈conv(T). Les sites de U non
sélectionnés restent autorisés sur la frontière ; aucun site extérieur à σ
ne peut être strictement intérieur. C'est la convention Gabriel faible aux
contacts, et non la condition « aucun autre site dans la boule fermée ».

La nécessité de I⊆σ est immédiate par vacuité Gabriel. La miniboule d'une
partie de la coquille est B exactement lorsque son centre appartient à son
enveloppe convexe ; un support minimal positif a au plus quatre sommets en
dimension 3. Cela donne |I|+q_min≤K+1, donc la fenêtre du catalogue natif à
`kmax=K` suffit, sans demander K+1 au moteur. Réciproquement le critère
ci-dessus certifie la miniboule et la vacuité de chaque émission. L'unicité
de la miniboule interdit qu'une coface appartienne à deux clés différentes.
Cette complétude reste relative au contrat du générateur/census natif existant,
pas une nouvelle preuve générale du moteur.

En régime régulier |U|=q_min, le contrôle positif de la chaîne implique que
seul T=U convient. En dégénérescence, `local_plateau::ShellTable` recalcule
exactement les supports puis leurs sur-ensembles ; on consulte
`contains_center[mask]`. Tester uniquement |T|≥q_min serait incorrect : sur
un carré, q_min=2 mais une paire adjacente ne définit pas sa boule centrale.

L'index déterministe de la tour est reconstruit **une fois** pour convertir
les rangs géométriques conservés dans `BallData` vers les IDs d'entrée ; les
coordonnées et puissances exactes de chaque population sont vérifiées. Aucun
nouveau catalogue géométrique n'est produit par cette conversion.

Coquille≤12 et |I|≤9 : limites natives conservées. Une coquille>12 est un
refus explicite, jamais une troncature ou un catalogue partiel présenté comme
complet. Au plus 4096 cases locales par coquille dégénérée, au plus
binomial(12,6)=924 cofaces de cardinalité donnée par boule. Aucune énumération
globale des binomial(n,K+1) parties ; aucune borne sous-quadratique sur le
nombre de boules ni sur le travail du générateur n'en découle. K5/K10 et
n≈1200 ne sont pas des limites codées ni une promesse de temps.

## Construction et contrôles

`build_native.py --build REPERTOIRE_NEUF [--jobs 2]` compile les 25 unités GEN,
la chaîne, le stub GPU et ce nouvel adaptateur. Le constructeur est un port
explicitement déclaré du constructeur figé, sans réemploi d'objets. Sources,
dépendances découvertes et effectives, compilateur, commandes et artefacts
sont épinglés avant/après dans le répertoire privé ; tout échec reste conservé.
Le build initial CPU est `build/v9-weighted-native-20260927-r1`.

Le gate `--gate` énumère **uniquement sur petits nuages** toutes les cofaces et
certifie leur MEB par recherche de supports≤4 et tests de confinement exacts,
sans utiliser `ShellTable` pour l'oracle. Il traite les carrés, coquilles 3D,
intérieurs obligatoires, permutations d'IDs, K10/cofaces de 11 sites et refus
de coquille 14. Il partage les primitives entières du moteur et n'est donc
pas un oracle arithmétique indépendant. Le contrôle `qualify_geometry.py`
distinct apporte l'oracle Fraction et le rapprochement avec FULL.

Préflight observé : gate code 0, 28 cas, 2192 sous-ensembles, 258 cofaces,
102 masques de cardinalité admissible rejetés et 800 requêtes de couverture.
`test_native_export.py --native BINAIRE -v`, normal et `python3 -O -B` : trois
tests passent (gate, schéma/identité W1-W2, onze refus CLI). Ces sorties ont
été observées en session ; ne pas les présenter comme des stdout archivés
avant la capture séparée de qualification. Aucun GCP utilisé.
