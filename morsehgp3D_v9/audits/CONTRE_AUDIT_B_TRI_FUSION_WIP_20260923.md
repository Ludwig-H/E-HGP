# Contre-audit B — tri de fusion et raccourci FULL (chantier mutable)

23 septembre 2026. Lecture du diff de travail du développeur après le
snapshot publié `78ce9fd4` ; **aucun reçu G4 ne qualifie ce diff**. Le
changement fait trier les présentations de la chaîne par
`tower::parallel_sort`, reconnaît par balayage les catalogues déjà
strictement triés dans FULL, et mesure le condensé de contrôle après
`chain_total` dans un champ `digest` distinct. Les portes locales du
catalogue normal et du catalogue renversé passent ; ceci n'est ni un
oracle global de complétude ni une mesure de gain.

## Ordres et chronos

Le comparateur des présentations utilise `(clé, arité, support)` ; les
présentations indiscernables à ce comparateur sont refusées lors du scan
de groupes. Le raccourci FULL exige `key[j−1] < key[j]` à chaque pas ;
sinon le tri et le refus des clés dupliquées sont maintenus. Je n'ai pas
trouvé d'écart d'ordre exact sur ces chemins. Le chiffre
`presorted_catalogues` est pour l'instant interne au résultat, pas publié
dans le JSON de sonde.

La session R6, qui exécute **l'ancien** tri et inclut le condensé dans
`chain_total`, mesure sur les cas cœur ON : fusion/tri **0,15–0,77 s**,
tour FULL **0,83–5,26 s** et q3/q4 **2,61–9,83 s**. Ces bornes sont
des temps par phase, pas une ablation du nouveau tri. À elles seules,
elles excluent que rendre seulement le tri instantané suffise au contrat
de la tour en 1 s sur ces cas. Le nouveau `chain_total`/`chain_cpu_s`
**exclut** le digest ; tout comparatif avec R6 doit additionner
`times_ms.digest` à la nouvelle mesure ou annoncer clairement la
frontière différente. Le lecteur v9 contrôle la somme contre le mur
externe ; il ne transforme pas pour autant les chronos v8 historiques.
Le digest reste synchrone dans `run_tower_chain` : la latence de cet appel
est donc au moins `chain_total + digest`, même si la construction de la
tour en mémoire s'achève avant le digest.

Le lecteur mutable v9 vérifie seulement l'inégalité
`chain_total + digest ≤ mur externe + 50 ms` et omet `read`, alors que la
sonde chronomètre la lecture avant
la chaîne et que `/usr/bin/time` enveloppe **l'exécutable entier**. Une
mutation hors dépôt a pris une vraie sortie R6 convertie au schéma v9,
fixé `digest=0,125 ms`, `read=3 600 000 ms` et un mur externe de 60 s :
`validate_probe` et `validate_external_wall` l'ont **tous deux admise**.
Le temps séquentiel publié dépassait pourtant 3 606 s. Pour la
cohérence du reçu, borner `read + chain_total + digest` par le mur externe
avec la tolérance annoncée et tuer cette mutation dans la porte worker.
Cela ne met **pas** `read` dans le contrat in-memory de 1 s.

## Résidence mémoire et échecs

Le tri parallèle alloue `buffer(n)` en plus de `all`. Sur
08/000000/K10 R6, les 881 908 + 2 898 219 + 1 732 548 présentations
q2/q3/q4 donnent **5 512 675 objets** ; avec l'ABI observée de
`Presentation` à 112 octets, ce tampon représente environ
**617 419 600 octets, soit 589 Mio**. C'est une projection à partir du
reçu R6, **pas un pic RSS mesuré** du nouveau binaire. Le raccourci FULL
évite plus tard un tri de `BallId`, dont le tampon est d'un autre ordre
de grandeur et d'une autre phase ; ne pas soustraire ces mémoires sans
mesure de résidence simultanée. La sortie K10 R6 occupe déjà plusieurs
Gio ; surveiller la crête et les capacités en vue de dizaines de
millions de points.

Le chemin `parallel_sort(all, W, …)` peut lancer des fils et allouer un
tampon. Une injection de panne dans la primitive de fils a produit
`std::system_error(Resource temporarily unavailable)` avec jointure des
fils actifs ; la primitive respecte donc la sûreté de vie. En revanche,
`run_tower_chain` ne classe comme `kResourceExhausted` que
`std::bad_alloc` ; `std::system_error` et `std::length_error` tombent
dans `kInvariantViolated`. FULL classe déjà ces deux exceptions comme
ressource. Ce désaccord est désormais atteignable depuis la nouvelle
fusion. Si la panne survient au milieu du tri, `merge_ms` reste zéro
alors que `total_ms` inclut le travail payé. Ajouter une porte causale
de panne de lancement/taille au niveau chaîne et publier le temps payé
sur échec, avant de qualifier la nouvelle voie.

Enfin, si le digest échoue après construction, la chaîne efface la tour
et le catalogue conservé mais garde les résumés `orders`. Le contrat
actuel n'exige pas explicitement leur vacuité sur échec ; c'est une
**question de cohérence d'API**, pas un défaut d'exactitude prouvé.

Priorité : corriger la classification et la comptabilité des échecs,
mesurer le pic RSS/tri avec le nouveau binaire, puis ablater le gain
FULL complet. La réduction des produits q3/q4 **avant expansion** reste
prioritaire pour le jalon 1 s et la croissance sous-quadratique.
