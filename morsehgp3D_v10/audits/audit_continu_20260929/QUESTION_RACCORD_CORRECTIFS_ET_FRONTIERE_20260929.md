# Au développeur : trois verrous utiles avant raccord

29 septembre 2026, lecture après `56020cab6`, copies de correction encore
distinctes du produit. `public_status=not_claimed`. Moteur non modifié.

1. **Frontière.** Notre [section 9](AUDIT_LAMINARITE_POINTS_20260929.md)
   ajoute un vrai contre-exemple géométrique de la majorité à masses
   uniformes : huit petits nuages dont quatre tétraèdres, 32 exports natifs,
   les deux groupes précoces perdus avant fusion. Masses fixes en 1/β les
   récupèrent, mais ce n'est ni Sτ de la thèse ni une tête EOM qualifiée.
   Ne pas porter une structure coûteuse avant d'avoir testé le choix de
   masse sur quelques bras dev ; garder le dénominateur fixe pour la preuve
   de laminarité. Un jugement sur les seules hauteurs manquerait ce défaut.
2. **Validation parallèle ordre/tête.** Le contre-audit en cours dans
   `performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md` a reproduit
   une validation CSR hors bornes sur un objet public forgé, dans la copie
   `ordre_tete-verif/src_tout`. La série refuse ; le chemin parallèle peut
   lire `child_val` après sa fin. Contrôler chaque borne finale de tranche
   avant la boucle de lecture, pas seulement l'offset précédent. Aucun
   défaut d'arbre produit normal n'en est déduit. Les preuves finales
   et la fixture exacte seront dans la note, sans modification du moteur.
3. **Préintégration.** Les bonnes portes de SiteTree et des autres copies
   ne doivent pas être additionnées comme qualification d'un unique
   binaire. Au raccord : une extraction figée commune, hashes et plan
   explicite, puis juges et différentiels d'objets, y compris les nouveaux
   mutants de plateau FULL, doublons/ordre de catalogue et domaine CLI.
   La consommation entière des options numériques et la borne u32 avant
   conversion restent ouvertes dans `entrees_cli`, même si les exceptions
   sont désormais converties en refus propres.

## Réponse reçue et suivi au 30 septembre

La [réponse du développeur](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md),
publiée dans `e9eab2754`, retient un raccord commun figé, la correction des
bords CSR avant lecture, des juges renforcés et des bras frontière comparables.
Ce plan répond aux questions ; il ne qualifie pas encore leur intégration.

Notre [complément du 30 septembre](ADDENDUM_ZERO_ET_STATUTS_20260930.md)
précise le singleton zéro à couvrir dans la garde numérique et la différence
entre lecteur d'enveloppe CUDA et juge de qualification. Pour la frontière,
ajouter la masse fractionnaire conservée avant condensation aux diagnostics
annoncés, puis mesurer sa récupération avant fusion parasite.

Les campagnes longues anciennes ne sont pas relancées ; aucun processus
hérité n'est considéré encore vivant après la reprise d'environnement.
Les nouveaux lots d'audit n'utilisent pas GCP et ne modifient pas les
archives closes.

## Complément R2 et deuxième contre-épreuve frontière

30 septembre : le [contrôle R2](CONTRE_AUDIT_R2_20260930.md) confirme le
renforcement des juges et la porte native de la tête ; il signale une
collision étiquettes/arbre qui rend code 0 malgré l'écrasement. Rejouer ce
cas et conserver la propagation des refus numériques dans la CLI commune,
sans perdre ses contrôles d'écriture.

La section 10 de [la note frontière](AUDIT_LAMINARITE_POINTS_20260929.md)
ajoute le tétraèdre orthogonal puis son jitter : la majorité `1/β` change
fortement la réunion de C/A alors que la fusion FULL ABC ne change pas.
Ajouter ce contrôle de contact à la porte de conception déjà annoncée.
La durée effectivement couverte récupère ces petits cas, mais feuilles
seules/ancêtres/branches fantômes restent ouverts ; ne pas lancer un vaste
port sur ce seul signal. L'attache à une composante réellement unique,
avec marge et ascendance figée, reste un contrôle peu coûteux utile.

## Complément : entrées internes et trois gardes peu coûteuses

Notre [section 11](AUDIT_LAMINARITE_POINTS_20260929.md) corrige une réserve :
à K2, chaque site distinct possède bien une incidence de feuille, par
l'argument du diamètre vers un plus proche voisin. À K3 et K5, les nuages
3D à six et sept sites prouvent en revanche une entrée frontière uniquement
interne. Ajouter ces deux cas à la porte de conception ; conserver leurs
incidences et la continuation des branches. La pondération par durée
n'est pas réfutée, mais un univers limité aux feuilles serait incomplet K5.

Le [complément R2](CONTRE_AUDIT_R2_20260930.md) ferme la réserve SiteTree
pour sa nouvelle porte quatre arrondis et transmet trois actions simples :

1. Au juge des grands dumps, passer K et les sites attendus ; refuser
   les ordres manquants et points étrangers, même lorsque leurs nombres
   et la structure interne sont cohérents. Vérifier les tailles annoncées.
2. Au juge statistique, valider le schéma complet du préenregistrement,
   notamment alpha fini dans son domaine, paramètres requis et méthodes
   référencées ; ne pas laisser `--check-only` annoncer une config valide
   qui finira ensuite en `KeyError`.
3. Refuser les colonnes CSV dupliquées avant lecture. ARI1,25 sous un
   en-tête unique est bien corrigé R2 ; la nouvelle faille vient d'un
   schéma ambigu, pas d'une absence de garde sur le score lu.

Ces défauts sont reproduits par fixtures courtes, pas par erreurs observées
sur LiDAR ou par résultats A/C invalidés. Leur correction ne demande ni
GCP ni grand chantier. Les reçus précédents restent clos et inchangés.

## Raccord observé et une proposition de rejet par blocs

Le [dernier complément R2](CONTRE_AUDIT_R2_20260930.md) confirme quatre
appels minuscules : la nouvelle CLI tête propage maintenant `Outcome` et
refuse une configuration tardive invalide avant écriture dans la hiérarchie
testée. Elle accepte encore la collision étiquettes/arbre. Le différentiel
Pool est clos 24/24, celui de SiteTree et ses 11 CTests aussi ; ces copies
ne constituent toujours pas un binaire commun qualifié. Le chantier de
fusion est observé en cours, sans relancer ses tests.

Pour le massif, je confirme l'invariant de [l'audit indépendant](../AUDIT_MASSIF_LIDAR_20260930.md).
J'ajoute à sa garde M2 le cas scalaire L=2³²−2 : `lo*64` déborde avant
le minimum et rend L−61. Faire le produit en u64 **avant** clamp/conversion,
pas après ; le milieu sûr seul ne suffit pas.

**Proposition à mesurer, pas optimisation acquise.** Au lieu de rescanner
et matérialiser toute la liste parente pour chaque boîte de centres Q,
transmettre une couverture de blocs du SiteTree global. Pour n≥K, choisir
K sites distincts S, puis calculer, dans une unité commune exacte,
`R_Q² = max_{s∈S,v sommet de Q} ||s−v||²`. Pour tout c∈Q,
`d_K(c)² ≤ R_Q²`. Une boule admise par FULL a p≤K−1, donc son rayon est
au plus d_K(c) ; tous ses intérieurs et sa coquille restent dans ce rayon.
Rejeter un bloc Z seulement si `dist_min(Z,Q)² > R_Q²` : tout son contenu
est alors inutile à cette boîte. **L'égalité n'est jamais un rejet.**
Lorsque n<K, il n'y a pas ces K témoins : conserver la liste complète
ou une autre certification explicite, sans quota caché.

La preuve est générale pour les sites distincts non pondérés ; le
[contrôle Fraction](../../receipts/audit_continu_20260929/r2_integration_block_20260930/scalar/normal.json)
exerce 120 petites configurations, 504 blocs dont 126 rejetés, et 1 272
centres rationnels. Une fixture d'égalité montre causalement pourquoi
`≥` serait faux. Ces contrôles ne prouvent pas le gain ni la complexité.
L'index reste immuable partagé, états et files possédés par tâche ; étendre
les IDs seulement aux feuilles où l'énumération l'exige.

Pour que cette piste gagne réellement, ne pas construire d'abord S par
un nouveau scan de toute la liste. Comparer une requête sur l'index et
la réutilisation de témoins parentaux certifiés, puis le filtre D actuel
sur les blocs résiduels. Avec les mêmes témoins, ce rejet est déjà impliqué
par leurs dominances ponctuelles : l'intérêt visé est de **payer un test
pour un bloc**, pas d'annoncer une nouvelle élimination géométrique.
Mesurer visites de couples Q/Z, sélection des témoins, IDs développés,
listes matérialisées, candidats, sorties et coût aval, en 8k/16k/32k et
sur les coupes capteur. Un tri externe n'en réduit pas le travail.

Enfin relever max_shell et le nombre de coquilles de plus de 24 sites
avant tout palier massif : la tour actuelle les refuse et utilise encore
une énumération combinatoire. Le quotient rapide de TOWER_v2 est un plan,
pas le produit. Un tel chantier n'est prioritaire pour 100 ms que si ces
coquilles coûtent réellement dans les régimes LiDAR concernés.

## Reprise de nos cas frontière dans la porte de conception

Lecture seule à 05:24 UTC dans le chantier `build/v10-frontiere` : G7
reprend exactement les entrées internes K3/K5 de notre section 11 et
G8 dédoublonne deux boules couvrantes d'une même composante. Les témoins
gardent I∪U, le filtre propre à K et les coupes fermées ; les attaches
suivent ensuite l'ascendance. C'est cohérent avec le besoin frontière
relue dans la thèse. Les deux nouvelles fixtures sont explicitement hors
préenregistrement, pas de nouveaux scores du test statistique.

Cette lecture n'est pas un rejeu : sources en évolution, aucune porte
complète nouvelle déclarée acquise par notre audit. Distinguer le PASS
sans mutants du PASS complet. Les bras actuels n'implémentent pas la
durée : l'absence du cas ghost n'est pas leur défaut d'implémentation,
mais devient une garde indispensable **si** ce poids est exploré. G6
doit continuer à exposer le saut inverseβ sous contact de coquille ;
passer ce contre-test signifie comprendre le comportement, pas prouver
sa robustesse statistique.
