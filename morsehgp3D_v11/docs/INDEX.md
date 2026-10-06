# Index global exact pour les descentes

Tranche v11 du 2 octobre 2026, CPU, profils entiers 18/21/24, défaut 21.
Qualification G4 close à **`e8520481d`** : Release 251/251, ASan24/TSan21/
profils21/24 176/176 chacun, poison 177/177, complément num/index ASan18 36/36.
[Reçus, oracles, mutants et mesures](../receipts/index_20261002/README.md).
Cette brique ne calcule ni MEB ni FULL.
Le catalogue qualifié à `ffc2ff95f` reste inchangé.

## Propriété et domaine

`build_index(Cloud&&, IndexParams, MemoryBudget&)` construit un `GlobalIndex`
immuable. Il ne transfère le Cloud qu'au succès : un refus laisse le Cloud
et ses vues intacts. L'index possède alors nuage et arbre ; son déplacement
transfère les tableaux sans conserver de pointeur vers l'ancien objet Cloud.
Les budgets respectifs doivent survivre aux tableaux. Construire le futur
catalogue avec `index.cloud()` conservera la même interprétation des SiteIdx.
Les objets déplacés deviennent vides et une nouvelle requête les refuse.

Les sites suivent l'ordre Morton déjà certifié par Cloud. L'index conserve
tous les sites, leurs poids et tous les PointId. Le census compte **les sites
géométriques distincts**, sans développer les poids. Cette convention ne
qualifie pas FULL pondéré. Aucun masque, sous-échantillonnage ni copie des
coordonnées n'est introduit par l'index.

Une requête prend une Sphere valide q1..4 et un seuil u32 strictement positif.
Ses supports peuvent être extérieurs au Cloud ; elle ne suppose ni centre
dans les supports ni centre dans leur boîte. Elle rend obligatoirement :

- `saturated` si au moins K sites sont strictement intérieurs : exactement
  K témoins distincts, triés par SiteIdx, coquille vide ;
- `complete` sinon : tout l'intérieur I et toute la coquille U, chacun trié.

Les témoins ne sont pas certifiés K plus proches. Le résultat possède ses
listes et ne dépend plus de la durée de vie de l'index pour lire ses IDs.
Leur interprétation géométrique exige toujours le même Cloud.
L'index ne matérialise ni cellules de Delaunay, ni cofaces, ni graphe global
de couples de sites ; son seul intermédiaire global est cet arbre linéaire.

## Arbre et parcours

L'arbre est l'**arbre radix de Morton** (Karras ; levier V3, 6 octobre 2026) :
toute plage de plus de `leaf_size` sites (défaut 8, domaine 1..256) est coupée
au bit de Morton le plus haut qui diffère entre son premier et son dernier
site. Le Cloud étant trié par clé croissante, sans site répété, les sites dont
ce bit vaut 1 forment un suffixe non vide de la plage ; une recherche
dichotomique le trouve en lisant la seule coordonnée de ce bit, sans clé
stockée. Chaque nœud est donc une cellule de Morton, et deux frères sont séparés
par un plan de coordonnées, au lieu de se recouvrir comme les deux moitiés de
l'ancienne coupe médiane des rangs. L'arbre est stocké en préordre ; chaque nœud
porte une boîte fermée exacte, sa plage et le premier indice après son
sous-arbre. Les feuilles scannent leurs points une fois à la construction ; les
boîtes internes réunissent celles des deux enfants. Mémoire O(n). Chaque coupe
fixe au moins un bit de plus du préfixe commun : la profondeur est au plus
3B+1 (55, 64 ou 73), atteinte par l'origine et les 3B points dont la clé n'a
qu'un bit (porte `mhgp11_index_unit_structure`). Ces bornes commencent
**après** la préparation Cloud et ne bornent pas une tour ou le nombre de
requêtes FULL.

Le nombre de nœuds dépend des positions. Un premier passage applique les mêmes
coupes et le compte avant toute allocation (compter, réserver, remplir) ; le
remplissage vérifie qu'il retombe exactement sur ce compte. Chaque nœud interne
a deux enfants non vides : il y a moins de 2n≤2^33 nœuds. Les sites restent
rencontrés dans l'ordre de Morton, donc les listes, les témoins saturés et les
descentes FULL sont inchangés. Sur les trois trames LiDAR à K = 5, la borne
entière et l'arbre radix ramènent les tests de points du census à ×0,258–0,265
de la borne continue sur l'arbre médian (×0,50–0,52 avec la seule borne).

Le census parcourt ces liens sans pile. Sur la boîte, il borne
H(x)=D‖x−a‖²−2N·(x−a)=D(‖x−c‖²−r²), c=a+N/D. Un minorant **strictement positif**
écarte tout le bloc ; un majorant **strictement négatif** accepte ses sites
intérieurs. Les égalités sont raffinées, puis jugées exactement aux feuilles.
Toute coquille étendue est donc conservée ; aucune tolérance n'intervient.
Les boîtes sont globales au propriétaire, jamais celles d'une liste locale
du catalogue réutilisée après déplacement du centre d'une MEB.

**Bornes sur sites entiers** (levier V3 de l'audit des transpositions, contrat
de la revue indépendante 10 ; 6 octobre 2026). `num::LatticeSphere` prépare une
fois par parcours, pour chaque axe, l'entier le plus proche de c_j (ex æquo :
le plus petit) et le seuil ⌈2c_j⌉, par une division entière i128 de
C_j=a_jD+N_j (moins de 2^(5B+6)). Le minorant est H au point entier le plus
proche de c **ramené dans la boîte** : chaque axe étant une parabole convexe,
c'est le minimum exact de H sur les points entiers de la boîte, donc sur ses
sites. Le majorant est H au **coin le plus éloigné** de c : le maximum exact sur
la boîte continue. Les deux points sont dans la boîte, leurs puissances gardent
les budgets natifs de `side`. Ce minorant ne vaut que pour des points entiers :
sur le segment entre (0,0,0) et (1,0,0), le q2 de ces deux points vaut 0 aux
sites et −D/2 au milieu ; il ne remplace donc pas `num::power_bounds`, qui borne
la boîte continue. La voie Wide (q3 non certifié en 21/24) garde les bornes
continues antérieures, qui séparent les extrema quadratiques et linéaires par
axe (moins de 72 M^5 pour q4 et 216 M^6 pour q3, M=2^B). Les listes rendues
sont inchangées ; sur les trois trames LiDAR à K = 5, les tests de points du
census passent de 21,15 / 15,52 / 14,76 M à 10,48 / 7,99 / 7,58 M (×0,50–0,52),
tous les autres registres et les sorties FULL restant identiques. La validité
ne suppose aucune positivité barycentrique.

## Ressources, déterminisme et portée

Un premier parcours compte jusqu'à K ; le second remplit des buffers de
taille exacte. Réservation propre du résultat : 4K octets si saturation,
sinon 4(|I|+|U|). La coquille abandonnée d'une réponse saturée n'est jamais
allouée. Un échec restitue les réservations de cet appel, sans publier de
préfixe. Le travail des deux parcours est **cumulé** dans CensusLedger.
L'arbre réserve exactement `nodes()*sizeof(Node)` octets ; aucune allocation
de taille dépendant de n n'est extérieure aux Buffer. Les appels ont une
pile bornée à 3B+2 cadres à la construction et constante à la requête.

L'ordre gauche/droite suit les plages Morton, y compris lors de l'acceptation
d'un bloc : les sorties sont déjà croissantes. L'index partagé reste constant ;
chaque requête possède son état et son budget avec pilote unique. Au pire,
une requête visite O(n) nœuds/sites, et ses deux passes doublent ce travail.
Aucune borne sous-quadratique sur FULL ne découle de cette brique.

Les portes indépendantes utilisent Gram/Fraction puis un scan global pour
les petites entrées. Le banc massif compare aussi chaque réponse à un scan
global utilisant la primitive `num::side` qualifiée : ce second contrôle
est indépendant du parcours, **pas** un oracle arithmétique indépendant.
Les requêtes artificielles du banc ne représentent pas encore les descentes
d'une tour. Les coûts Cloud, arbre, requêtes et scan témoin sont séparés.
Sur les trois trames sans sol entières, l'arbre u21 prend 0,341–0,418 ms
après Cloud ; les 64 census choisis prennent 0,621–0,810 ms. Les 18 processus
terminent, avec sorties sémantiques et travail identiques aux trois profils.
Une répétition, même séquence 08, GPU inutilisé : aucun transfert au contrat FULL.

## Raccord des audits indépendants

Les revues publiées à `108350f45` portent sur `d0dc9cd8b` et sur le WIP
numérique, avant cette qualification. La [revue des bornes](../receipts/audit_independant_20261002/global_census_bounds_review_9/README.md)
confirme LB/UB et leur domaine. Son majorant supérieur exact par axe UB*
est une option future : il pourrait resserrer les certificats intérieurs,
avec un coût arithmétique supplémentaire à mesurer. Il n'est pas porté ici.

La [revue du contrat d'index](../receipts/audit_independant_20261002/global_index_contract_review_9/README.md)
est traitée par propriété du Cloud dans GlobalIndex, résultat à tag privé,
listes exactes et admission des sorties après comptage. Les deux parcours
reprennent le même index et la même Sphere immuables ; seule l'écriture des
sorties change. Leur égalité de cardinalités résulte de ce parcours commun,
sans garde supplémentaire de comparaison au retour. Les portes vérifient
les populations complètes, les refus transactionnels et les deux passes.

Le census publié a un domaine géométrique explicitement distinct de FULL
pondéré. Les doublons sont admis comme un seul site ; le catalogue refuse
déjà w≠1. Le futur contexte FULL devra certifier ce régime une fois et
lier Cloud/index/catalogue/résultats par une identité privée commune.
Un Census actuel possède ses IDs mais ne porte pas encore ce token :
il ne certifie aucun raccord à un autre propriétaire, ni un job asynchrone.
Son interprétation requiert le Cloud de sa requête. Enfin, plusieurs
sorties complètes retenues coûtent jusqu'à4Tn octets d'IDs ; un budget par
requête ne remplace pas l'admission globale du futur ordonnanceur FULL.
