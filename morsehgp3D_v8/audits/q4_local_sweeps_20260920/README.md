# q4 : balayages locaux exacts et frontières de témoins

Audit indépendant A, 20 septembre 2026, après `2920b8b5`, sur main.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
Écritures limitées à audits/. GCP non utilisé.

La carte de centres peut servir au **repli exact**, en conservant dans chaque
feuille indécise un compte strict exact et les témoins encore actifs. Le
prototype balaie maintenant ces témoins et vérifie les profondeurs/coquilles,
au lieu de seulement prévoir un scan du cover par face. Le dense permuté
conserve cependant un carré par cellule. Une proposition supplémentaire
retire exactement du tri les racines hors cellule ; elle est vérifiée en
modèle rationnel, pas encore portée dans ce C++.

## Contrat à porter

La [preuve](MATH.md) partitionne le cover sur une cellule fermée C :
intérieurs uniformes si `max L<0`, extérieurs uniformes si `min L>0`, actifs
sinon. Le compte hérité c et les actives donnent exactement
`p(t)=c+Σ_actives [L_z(t)<0]` dans C. Les endpoints a,b sont une coquille
commune, sans réplication par feuille.

Deux gardes distinguent ce payload d'un filtre de rejet :

- **Garder les contacts `min=0`.** Le retrait `min>=0` perd une vraie racine
  q4 et sa coquille dans la fixture de tétraèdre régulier au coin possédé.
- **Ne pas recycler un minimum d'enfants comme compte exact.** Il reste un
  minorant. Les fragments exacts et cellules seulement certifiées DEEP ont
  des contrats distincts ; le seuil q3 reste indépendant.

Le C++ balaie toutes les racines des actives, initialise à −∞, regroupe les
égalités et retire les sorties avant de lire profondeur/coquille, puis ajoute
les entrées. Il ne sature pas ce compte décroissant. Un localisateur exact
attribue une racine frontalière à une seule feuille. Le comparateur emploie
un déterminant réduit de degré six en i128 ; les produits rationnels naïfs
de degré huit dépassent cette largeur sur une fixture aux bornes des
coefficients. Cette dernière n'est pas présentée comme un tétraèdre positif.

**La sortie est une racine de faible profondeur dans le cover**, pas encore
un support positif propriétaire. Positivité, rang, présentations, canonisation
des boules et collecte des IDs intérieurs restent à raccorder. Le compte c
ne contient pas ces IDs. L'oracle vérifie séparément que les vraies racines
positives propriétaires requises ne sont pas perdues. Le transfert au census
global exige toujours le contrat géométrique du cover.

## Coût mesuré : 66 configurations closes

K10, profondeur 7, budget 4096 ; domaine disque et domaine positif pour chaque
cas. Les balayages, tris, groupes et coquilles sont exécutés. `forecast_scan_reads`
reste seulement le coût prévu de l'ancien repli complet, non exécuté ici.

Pour une feuille C, noter a_C sa population active et s_C les seeds qui la
traversent. Les reçus distinguent `A=Σa_C`, `I=Σs_C` et **`W=Σs_C a_C`**,
nombre réel de lectures d'actives. La mémoire et les tris s'ajoutent à W.

| Dense, domaine positif | I = A | W exécuté | Comparaisons de tri | Racines de faible profondeur |
|---|---:|---:|---:|---:|
| Préfixe 8k | 5 044 | 5 300 660 | 63 632 229 | 50 |
| Préfixe 16k | 9 494 | 13 779 726 | 173 238 164 | 50 |
| Préfixe 32k | 10 358 | 15 770 686 | 199 677 775 | 50 |
| Permuté 8k | 2 508 | 941 668 | 9 486 595 | 74 |
| Permuté 16k | 4 960 | 3 659 588 | 41 157 848 | 73 |
| Permuté 32k | 10 060 | 14 901 148 | 186 227 961 | 50 |

Dans ces cas denses, les sites actifs sont aussi les seeds : **W=Σa_C²**.
Le permuté fait ×3,886 puis ×4,072 aux doublements. La seule subdivision à
profondeur fixe déplace donc le carré. Une garantie `max a_C≤L` donnerait
W≤LI, mais une grande coquille au même centre interdit de promettre cette
garantie par le seul raffinement. Les permutations restent celles de l'audit
précédent, distinctes du SplitMix64 constructeur 27.

À 32k préfixe positif : construction 647 948 tests de formes, requêtes 1 711 306
visites, 101 nœuds ; 126 992 octets de capacité d'actives retenues et 2 027 600
octets au pic des capacités d'actives vivantes. Ces derniers incluent les
temporaires parentaux, pas le RSS, l'allocateur ou les transitoires de
réallocation. L'arbre réserve 4096 nœuds de 104 octets et le scratch atteint
32 864 octets : aucun de ces postes ne doit être omis.

Temps total instrumenté avant JSON : préfixes positifs 1,859/5,045/5,881 s,
permutés 0,354/1,458/6,688 s. Mesures uniques sous charge partagée ; aucune
accélération produit n'en est déduite. `sweep_included` est inclus dans
`queries_and_sweeps`, pas une durée supplémentaire. JSON final/destruction
hors intervalle. Sans domaine positif, le préfixe 32k atteint 157 733 197
lectures et 2 071 136 179 comparaisons : payer aussi l'aval change le diagnostic.

## LiDAR réel : portée et préparation

Les 27 cas reprennent les scans séparés 0/100/200 de SemanticKITTI 08 quantifiés
à 2 cm : neuf arêtes à 8k, les mêmes IDs à 50k, puis neuf arêtes plus larges.
Ni superposition ni correspondance entre scans n'est supposée. Les sources,
hashes, choix d'arêtes et transformations sont dans chaque reçu et les
[fixtures épinglées](../q4_center_blocks_20260920/fixtures.py).

Sur 1 180 seeds, le domaine positif totalise I=143, A=302, W=263,
68 comparaisons et 120 groupes ; trois racines couvertes de faible profondeur,
12 IDs de coquille. Le domaine disque donne I=643, A=1525, W=1133 et 18 racines.
Ce sont des sélections d'arêtes, pas toute la WSPD ni des supports q4 qualifiés.
La préparation scalaire par arête relit au total 594 000 points par domaine,
pour 15 792 sites couverts cumulés : **ne pas porter cette préparation**.
Partager l'index et construire les covers par blocs reste prioritaire.

## Proposition suivante : retirer les événements extérieurs avant tri

À 32k positif, 87 % environ des groupes denses sont hors cellule ; le préfixe
disque monte à 95,8 %. La [preuve de clipping](CLIPPING.md) permet de les
retirer **avant tri** : choisir un point exact t₀ du segment fermé seed∩C,
remplacer chaque racine extérieure par sa contribution constante
`[L_z(t₀)<0]`, puis balayer seulement les événements intérieurs, contacts
compris. Oublier les événements sans cette contribution est réfuté.

Le [modèle](clip_gate.py) passe 612 appels 1D rationnels et 524 constructions 2D
de t₀ ; ce ne sont pas 612 nuages 3D. Les lectures W restent inchangées ;
localisation et évaluation de t₀ ajoutent un coût. Mesurer ces postes avec
les tris, groupes, présentations et sorties avant toute conclusion de temps.
Le C++ de cette capture conserve le balayage complet des actives.

## Réponse au constructeur 27 : blocs Z et continuations

Contrelecture du port 27, source finale `66b1551f` comprise : aucun défaut
nouveau identifié
dans la composition pool 26/carte lazy. Raté de droite local, UNKNOWN en fin
de budget, compression seulement d'enfants globalement certifiés et seuil
q3 indépendant sont cohérents. Cette lecture n'est pas une qualification.
Le pool 64 donne un minorant, pas le compte exact sur tout le cover de ce
prototype. Les [hashes de lecture](REFERENCE_PINS.json) bornent ce constat.
Le suivi final des capacités parentales/enfants, la compression et l’état
non reprenable après exception acquise ont aussi été relus. Les tests du
constructeur ne sont pas rejoués ; ses preuves restent propres au produit.
La présente capture indépendante part de `2920b8b5`.

Une première continuation compacte peut garder `(cellule, c, curseur Z)`
avec les liens DFS/escape du même index immuable :

1. Le préfixe avant le curseur est entièrement classé sur la cellule.
   Le bloc courant ambigu n'est pas consommé lors d'un split des centres ;
   les enfants héritent chacun le compte et ce curseur.
2. Diviser Z remplace seulement son bloc courant par ses enfants disjoints.
   Créditer un bloc le retire définitivement ; ne jamais ajouter ensuite
   ses descendants. Un bloc traversant la frontière du cover est raffiné
   avant d'employer toute sa population.
3. Si l'on saute un bloc ambigu pour poursuivre plus loin, un curseur unique
   ne représente plus les trous. Conserver alors une frontière persistante
   possédée/partagée, ou rescanner le suffixe non consommé. Un filtre peut
   rester un simple minorant ; le repli exact doit retrouver tous ces sites.

Les bornes conservatrices sur Z×C peuvent classer un bloc entier ; toute
incertitude reste UNKNOWN et conduit à un raffinement ou au repli. Pour
émettre des coquilles, le rejet extérieur exige encore un minimum strictement
positif. Ces invariants ne choisissent pas à eux seuls la politique optimale
entre split Z et split des centres.

Pour le parallélisme, partager propriétaire/index et racine immuable de
frontière ; chaque tâche possède cellule, compte, curseur/stade et sortie
privée. Un pointeur partagé ne rend pas gratuit le graphe sous-jacent :
compter allocations, références, pics vivants, scans répétés et collecte.
Les fragments UNKNOWN peuvent ensuite partager leur payload entre plages
de seeds. Le prototype est mono ; ni cette organisation parallèle ni les
frontières Z ne sont implémentées ici.

## Preuves et reproduction

[Qualification](qualification/MANIFEST.json) : neuf commandes, C++20 strict
GCC Release et Clang ASan/UBSan ; oracle cartésien rationnel normal/−O et
sanitizers. Par oracle : 210 appels détaillés, six paires de modes,
840 racines vérifiées, 12 457 tests de sites, 123 racines positives requises
préservées et coquille 30. Le modèle de balayage tue sept fautes, celui de
clipping une huitième ; ce sont des mutants de modèles, pas du produit.
Les deux erreurs préliminaires du modèle sont conservées dans
[PREFLIGHT.json](PREFLIGHT.json), sans les confondre avec la qualification.

[Capture](capture/MANIFEST.json), lecteurs [normal](READ_normal.json.gz) et
[optimisé](READ_optimized.json.gz), [fermeture globale](CHECKS.json).
[COUNT_PREFLIGHT.json](COUNT_PREFLIGHT.json) est une sonde préalable sans
balayage, distincte des 66 mesures finales. Sources et fixtures de 2920b8b5
réutilisées explicitement, sans code produit lié. Ne pas modifier ces preuves.

Depuis ce dossier, relire avec `python3 -B read.py capture` et
`python3 -B -O read.py capture`. Pour reconstruire, utiliser
`python3 -B qualify.py qualification_nouvelle` ; le runner refuse un dossier
existant. Une nouvelle campagne emploie
`python3 -B run.py .build/qualification_nouvelle/release capture_nouvelle 1`.
Les données LiDAR historiques doivent rester accessibles aux fixtures.
Clipping : `python3 -B clip_gate.py`, puis avec `-O`.
Depuis la racine du dépôt, `python3 -B morsehgp3D_v8/audits/q4_local_sweeps_20260920/close.py`
contrôle aussi les liens entre qualification, binaires et fichiers d’entrée,
qui dépassent le rôle du seul lecteur `read.py`. Il vérifie les liens des
documents et les hashes finaux ; son résultat normal/−O est identique.

P0 global, q3/q4 produit, tour FULL, contrats 50k/massif et GPU/G4 restent ouverts.
