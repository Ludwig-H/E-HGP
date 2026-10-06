# Index global : portes indépendantes

Tranche neuve sur la base `d0dc9cd8b`, qualifiée sur G4 à `e8520481d`.
Aucun index v8/v10 n’est copié. `GlobalIndex` possède le Cloud transféré
seulement après succès ; les SiteIdx gardent l’ordre Morton du Cloud.
Le résultat possède ses populations et peut survivre à l’objet index tant
que ses budgets demeurent vivants. Les doublons sont des sites géométriques :
leur multiplicité n’est pas développée dans le census.

L’oracle ne parcourt aucun arbre et ne réutilise aucune formule de bornage.
Il résout le centre et le rayon par Gram/Gauss en `Fraction`, puis scanne
tous les sites. Un support peut être externe au Cloud, et son centre peut
sortir de sa boîte et du domaine des coordonnées. Il exige soit exactement
K témoins stricts distincts, soit I et toute U si |I|<K. Tous les SiteIdx
sont croissants ; aucun contrat de K plus proches n’est ajouté.

Les 47 fixtures comprennent 24 permutations q4, les arités 1..4, des
centres rationnels, une coquille à douze sites, les seuils p−1/p/p+1,
les grands bits, les blocs entièrement intérieurs/extérieurs et un seuil
u32 maximal. Trois tailles de feuille et 47 paires d’entrées permutées
donnent 1 010 requêtes par profil : 302 census complets, 702 saturations,
six refus de paramètres ou de mémoire. Chaque oracle normal/−O attend
36 020 contrôles. La suite ne prétend pas qualifier FULL ni un census
pondéré. Les trois profils 18/21/24 ont joué leurs propres portes.

`model_test.py` joue 110 790 contrôles de réponses modèles, neuf faits fixes
par profil, dix-huit corruptions et trois JSON invalides, en normal et −O.
Depuis l'arbre radix (levier V3, 6 octobre 2026), le juge exige le nombre de
nœuds et la profondeur exacts, recalculés en Python à partir des sites ; deux
corruptions (nœuds superflus, profondeur réduite) le vérifient.
Ces vérifications légères passent localement ; aucun binaire natif n’a été
construit ni exécuté localement. Les réponses modèles ne qualifient pas
le produit. Le protocole batch est documenté en tête de `probe.cpp` ;
les budgets Cloud, index et résultat sont séparés, les entrées/JSON relèvent
du harnais.

Les groupes natifs contrôlent les fixtures, la structure, la propriété,
les budgets, les blocs certifiés, les bornes entières et la concurrence.
L’arbre radix est comparé à une récurrence indépendante (clés recalculées bit
à bit, coupe par balayage linéaire) pour 13 tailles, dont 8/16/17/24/257, et
quatre tailles de feuille ; son budget est exactement son nombre de nœuds
fois `sizeof(Node)`. Une chaîne de 3B+1 sites atteint la profondeur maximale
3B+1, et le census y reste exact. Le groupe `lattice` (census possédé et
emprunté) grave deux boîtes que la borne entière décide à la racine et que la
borne continue laisse raffiner, avec des registres absolus. Les tests déplacent Cloud/index/résultat, préservent
les vues valides, interrogent les objets déplacés vides et conservent un
résultat après destruction de l’index. Quatre fils exécutent chacun seize
requêtes privées sur le même index immuable. La porte de pénurie refuse
l’allocation de l’index puis les une/deux allocations des deux types de
census, vérifie la restitution et la reprise, et refuse toute allocation
système pendant une requête vide.

Les huit mutations visent : contacts exclus/admis stricts, SiteIdx dupliqué
dans un bloc, coquille plafonnée à K, saturation perdue à l’égalité, compteur
de passes omis, allocation de deux nœuds superflus et consommation du Cloud
sur refus mémoire. Le mutant d’allocation est jugé avant tout parcours par
le budget exact ; aucun dépassement de tampon n’est nécessaire pour le tuer.
Les 845 contrôles natifs, sanitizers ASan18/24 et TSan21, et les huit morts
causales sont observés sur G4. Le complément num/index passe36/36 portes,
la matrice complète 1 149/1 149. [Reçus](../../receipts/index_20261002/README.md).
Les reçus de qualification antérieurs restent inchangés.
