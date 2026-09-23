# Contre-audit B — certificat de voies mortes q3/q4, port WIP

23 septembre 2026. Lecture du **worktree développeur non commité**
`build/v9-open-worktree` à base `bb2c40dc`, pas une qualification produit.
Fichiers lus : `src/gen/lanes/q34_dead_lanes.{hpp,cpp}` (nouveaux),
`src/gen/pipeline/wspd_q34.{hpp,cpp}`, `src/chain/tower_chain.{hpp,cpp}`,
`bench/tower_probe.cpp`, protocole v5 et gate générateur. Le port peut
encore changer ; les chiffres R2 ci-dessous proviennent du moteur **sans**
ce port et servent à prévoir le travail qu'il ajouterait à entrée/front
identiques. Aucun G4 n'a été lancé par l'auditeur.
Lecture de la révision à frontière héritée : SHA-256
`q34_dead_lanes.cpp=b8bd3e4b…`, `.hpp=feeac1d3…`,
`wspd_q34.cpp=46db02a5…`, `tower_probe.cpp=4e4307ec…` ; ces hashes
ne sont pas un commit produit.

## Géométrie : pas de faux rejet identifié

La preuve est conservatrice sous le propriétaire u18 à sites distincts.
Pour chaque arête `ab`, elle couvre les disques de centres q3/q4 par des
cellules **fermées**. Une cellule est écartée si une borne inférieure sûre
la place strictement hors du disque, ou si `K−1` sites distincts pour q3
(`K−2` pour q4) ont chacun une puissance affine de maximum **strictement
négatif** sur toute la cellule. Les contacts ne sont jamais crédités.
La racine `[-2,2]²` contient les deux disques sous les bornes des
supports positifs possédés ; le cover de rayon `|ab|` autour du milieu
contient tout intérieur utile. Si une preuve échoue, le chemin exact
ordinaire est conservé. Le test `outside` somme des minima indépendants
des coordonnées : c'est une **borne inférieure** de la norme sur la
cellule, pas son minimum exact ; son usage avec `>` est néanmoins sûr.
Cette lecture ne remplace pas les gates différentiels du code WIP.

## Verrou de coût avant activation par défaut

Le raccord construit d'abord `Q34EdgeCover::make` et paie ses visites ;
ensuite `dead_.load` parcourt **tous** ses ranges, saute seulement `a,b`
et écrit un `Form` de trois entiers 64 bits ainsi qu'un rang `u32` pour
chaque autre site.
Seulement après commencent les tests de cellules. Ce certificat ne peut
donc épargner **aucune** construction de cover, même lorsqu'il prouve les
deux voies mortes ; il peut seulement épargner l'atlas, les graines et
l'aval de ces arêtes. La révision WIP de cette lecture transmet désormais
une **frontière active** de cellule en cellule et un compte intérieur
hérité, ce qui évite de relire les sites déjà classés strictement ; une
frontière très ambiguë peut néanmoins être relue à plusieurs niveaux et
aux deux seuils q3/q4. La borne par arête reste `Ω(C)` de chargement,
avec des capacités privées `Form`, rangs et frontières par profondeur,
et jusqu'à un nombre de cellules exponentiel en profondeur prévue ;
aucun terme global sous-quadratique n'en découle.

Sur le brut G4 R2 **refusé par le protocole**, scène sans sol
08/000000/K10, `Σ cover_sites=7 805 426 490` pour
`4 507 278` covers. Tous contiennent leurs deux endpoints et les ranges
sont disjoints ; avec les mêmes entrées/fronts, `load` ferait donc
`7 805 426 490 − 2×4 507 278 = 7 796 411 934` chargements de sites
et écritures logiques de formes, soit au minimum environ **218 Go de
valeurs écrites** à 24 octets de forme plus 4 octets de rang par site,
avant les recopies de frontières et les tests des cellules. Ce n'est **pas**
une mesure de bande passante DRAM ni une nouvelle borne de croissance,
mais un travail ajouté déterministement sur ce cas si le front reste
identique. `ChainOptions::q34_dead_lanes=true` et le plan G4 v3 l'activent
actuellement par défaut dans le WIP : ne pas en déduire un gain net des
seules masses de voies prouvées. La proposition [A avant cover](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md#certifier-une-voie-morte-sans-balayer-chaque-cover)
emploie une **petite palette** de vrais gardes tirés de l'index, avec
repli exact, justement pour éviter ce chargement systématique ; elle
reste elle aussi à implémenter et à mesurer.

Les compteurs `dead.loads` et `dead.form_sites` existent dans le moteur,
mais la sonde JSON v5 du WIP n'expose que `dead_cells`,
`dead_uniform_tests`, `dead_point_tests` et les masses de voies. Les
lecteurs G4 ne pourraient donc pas reconstituer directement la masse de
formes écrites. `peak_edge_buffer_bytes` observe le cover et la coquille
**avant** le chargement et n'inclut pas les capacités de `dead_.forms_`,
`dead_.all_` ou `dead_.levels_` :
son RSS de travail par worker est sous-déclaré. Publier tous ces postes,
les succès par masque q3/q4, la distribution des covers, puis faire une
ablation appariée on/off (mêmes trames entières, K/s/W/FULL, catalogue,
ordres et digest), avec temps CPU/mur et RSS. Garder la nouvelle voie
optionnelle jusqu'à preuve qu'elle économise **plus** que son chargement.

L'objet public réutilisable conserve `loaded_=true` depuis un ancien
appel jusqu'à la fin d'un `load` suivant. Cas causal : après un premier
chargement réussi, appeler `load` avec `work.loads=UINT64_MAX` fait lever
`counter_add` **après** `forms_.clear()` mais avant le nouvel `all_` ;
`loaded_` reste vrai et `all_` garde les anciens rangs. Un appelant qui
récupère l'exception et appelle `prove_*` peut alors indexer le vecteur
vide `forms_` par ces rangs. Le pipeline courant propage l'exception et
détruit son moteur : **aucun faux résultat produit n'est établi sur ce
chemin**, mais la garantie de l'objet après refus est insuffisante.
Invalider `loaded_` dès l'entrée et le republier seulement en fin, ou
construire un état temporaire puis l'échanger ; une porte de faute doit
vérifier la réutilisation après échec, pas seulement le résultat nominal.

Enfin, le changement de schéma `probe_v5` ajoute le booléen de voie morte
mais ne durcit pas les sept autres champs/histogrammes malformés encore
acceptés par le lecteur v4 ([contre-audit protocole](CONTRE_AUDIT_B_PROTOCOLE_V4_WIP_20260923.md)). Le préflight natif
obligatoire avant un nouveau G4 reste une question distincte du
certificat géométrique.
