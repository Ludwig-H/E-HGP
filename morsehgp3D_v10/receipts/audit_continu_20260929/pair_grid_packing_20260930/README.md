# Votes locaux et coût de recherche : deux propriétés distinctes

Paquet privé autonome, moteur/natif/GCP non appelés. Pas de mesure de temps.
Seul le préfixe réel SiteGrid (__init__, _cell, candidates) est extrait des
lignes580–624 de frontier_core.py, normalisé strip()+LF, puis compilé AST.
La source partagée intégrale, l’extrait et paires_k2.py sont pinés avant/après.
Les sources partagées restent intactes. Aucun appel de résolution FULL.

## Preuve de packing global

X est fini, n sites DISTINCTS de R³, 2<=K<=n. d_K(x) est le K-ième
plus proche site, x COMPRIS. δ=min distance entre sites distincts>0,
D=max distance, Δ=D/δ. c>=1 est fixé. Considérons les paires dirigées
(x,y), x!=y, avec |xy|<=c d_K(x).

Classer les ANCRES x par rayons d_K(x) dans [R,2R), R=δ2^j.
Il y a au plus1+floor(log2 Δ) buckets. Pour une cible y et un bucket,
les ancres incidentes sont dans B(y,2cR). Paver par cubes demi-ouverts de
côté R/2 : leur diamètre entre toute paire de sites est strictement<R.
Si un cube contient K ancres, chacune a K sites (elle comprise) à distance
strictement<R<=d_K(x) : contradiction à la définition du K-ième voisin.
Donc au plusK−1 ancres par cube, même sur une égalité d_K(x)=R.
Le cube englobant de B(y,2cR) rencontre O(c³) cubes, par exemple au plus
(ceil(8c)+1)³ avec pavage local. Sommer sur y et les buckets donne

    #paires <= (K−1)(ceil(8c)+1)³ n [1+floor(log2 Δ)].

Les rayons peuvent être hétérogènes ; on compte les arcs ENTRANTS pour
chaque cible après classement des ancres. Ceci ne borne pas le nombre de
voisins d’une ancre isolée. K2 : alpha=d_2/2 et ell=|xy|/2, donc c=1+eta
pour la bande dure. K>=3 : ell>=|xy|/2, alpha<=d_K, donc c=2(1+eta).
Une grille entière B bits de sites distincts a δ>=1 et D<sqrt(3)2^B,
soit au plusB+1 buckets. À K/B/c FIXÉS la cardinalité globale est O(n).
Cette preuve ne borne ni Γ_K, ni les boules/classes/couvertures, ni le
coût de SiteGrid, de localisation des centres, des parents ou des tests.
MMp souple du prototype mmc.py emploie une autre échelle min ell : ses
constantes de voisinage ne doivent pas être copiées de la bande alpha.

check.py contre-vérifie la preuve sur de petits nuages entiers exacts,
incluant rayons hétérogènes, coquilles, Kth exacts et deux ancres par case
au K3 ; oublier le facteur K−1 est rejeté causalement. Le test finit ne
remplace pas la preuve ci-dessus.

## Contre-exemple d’index, dans u18

n=8000/16000/32000 exactement ; m=n−2 points en paires unitaires sur
une grille de pas4 autour de (110000,110000,110000), plus (0,0,0) et
(262143,262143,262143). Chaque point dense a un unique vote de bande
K2 eta1/4 : son partenaire à distance1 (tout autre site dense est à>=3).
Chacun des deux extrêmes retient les m points denses et pas l’autre
extrême ; exactement3m votes dirigés au total, donc O(n).

Le volume global impose un pas SiteGrid de32768 ou16384 : tous les
m sites denses sont dans UNE case. Les boîtes de recherche des lignes
de vote (demi-côté2) sont intégralement dans cette case pour TOUS les
points ; candidates renvoie donc exactementm IDs à chaque fois.
Le total m² IDs retournés et m(m−1) tests de distance est DÉDUIT par
cette preuve de même case, pas exécuté par une boucle quadratique.
Trois appels réels du code AST sont recoupés par taille ; le reste
est validé globalement par les bornes de coordonnées et le pavage.

Même constat pour le census diamétral des partenaires : sa boîte est
dans la case, m candidats, m−2 tests, zéro troisième site dans la boule.
La mémoïsation peut ne payer qu’un census par paire non orientée, mais
il en reste m/2 ; avec300 préchauffages de naissance au plus,
au moins(m/2−300)(m−2) tests locaux restent possibles. Ce sont des
comptes dérivés, pas une mesure exécutée du résolveur.

Contrôle positif : mêmes sites denses SANS les deux extrêmes, même
SiteGrid réel ; pas4, deux sites par case, au plus16 candidats locaux.
Mutation causale de candidates : filtrer les IDs par la boîte exacte
fait échouer la propriété 'retourne toute la case'. Ce mutant continue
à visiter toute la case : ce n’est PAS proposé comme correction du carré.

Le contre-exemple démontre un défaut d’architecture possible de cet index,
pas une croissance quadratique mesurée de SemanticKITTI, ni un contrat G4.

## Relecture

record.py gèle une capture normal/−O et les pins début/fin. read.py exige
--manifest-sha256 SHA_EXTERNE avant JSON/replay, inventaire réel fermé de
sept fichiers, vérifications causales, puis hashes après les replays.
Ne pas réécrire une capture close ; créer un nouveau répertoire si correction.
