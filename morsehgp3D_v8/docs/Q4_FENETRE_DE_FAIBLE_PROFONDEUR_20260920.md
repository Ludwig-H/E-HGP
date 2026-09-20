# q4 : ne trier que les événements qui peuvent encore être peu profonds

20 septembre2026, tranche30 après31b0243a. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `public_status=not_claimed`. Port implémenté ;
qualification détaillée en fin de note.

## Problème visé

Les couches29 réduisent ensemble faces et témoins, mais le balayage
construit encore beaucoup de racines qui seront trop profondes.
Sur dense permuté32k/K10,803471 des804574 groupes sont rejetés pour
profondeur ; sur adversaire256/K10,63628 des64224. Ces constats portent
sur une arête donnée, pas sur le générateur global.

Cette tranche conserve la sélection29 et remplace le tri de tous ses
événements par une fenêtre exacte définie par leurs rangs. Ce n'est
ni un plafond de recherche, ni un rejet d'après deux échantillons.
Les premiers et derniers événements donnent une preuve pour tout
le complément de la fenêtre. Les anciennes voies restent inchangées.

## Preuve de la fenêtre fermée

Pour une seed aiguë, le produit utilise $P_z-\mu B_z<0$ pour l'intérieur
strict. $B_z>0$ donne une entrée : le site est intérieur après sa racine.
$B_z<0$ donne une sortie : il est intérieur avant sa racine. Les sites
avec $B_z=0$ sont constamment intérieurs, sur la coquille ou extérieurs.

Poser $T=K_{\max}-2$ et c le nombre de constantes strictement intérieures.
Si c≥T, toute la famille est rejetée. Sinon, poser H=T−c>0.
Soit U la H-ième racine d'entrée en ordre croissant, ou +∞ s'il y a
moins de H entrées ; soit L la H-ième racine de sortie en ordre
décroissant, ou −∞ s'il y a moins de H sorties. Les rangs comptent
les **IDs**, donc les multiplicités, pas les groupes distincts.

- Pour μ>U, au moins H entrées sont strictement intérieures.
- Pour μ<L, au moins H sorties sont strictement intérieures.
- Toute racine admissible est donc dans l'intervalle **fermé** [L,U].

L>U rejette toute la famille. L=U ne la rejette pas : des sorties et
entrées simultanées peuvent créer un creux ponctuel de profondeur.
Il faut conserver **tous** les IDs ex æquo à chaque borne, même si
leur nombre dépasse K. Les constantes sur la coquille sont conservées.

Dans l'intervalle fermé, une entrée strictement après U ou une sortie
strictement avant L est toujours strictement extérieure. Une entrée
strictement avant L ou une sortie strictement après U est toujours
strictement intérieure et fournit une contribution fixe exacte.
Les autres événements sont sur les bornes ou strictement entre elles.
Ainsi toute la profondeur et la coquille du sous-ensemble29 sont
restituées, pas seulement un minorant. Le certificat29 rétablit ensuite
le cover lorsque cette profondeur est<T ; positivité et propriété
rétablissent enfin le nuage global, comme auparavant.

Strictement entre L et U, il y a au plus H−1 entrées et H−1 sorties,
donc **au plus2H−2 IDs à trier**. Cela reste vrai si une borne est
infinie : le type correspondant a moins de H événements au total.
Les deux groupes d'extrémité, potentiellement grands, sont traités
séparément. Un grand groupe intérieur compte tous ses IDs dans cette
borne ; pas de perturbation des égalités ou d'hypothèse de régularité.

## Implémentation retenue

Un premier parcours des r IDs retenus par29 remplit deux tas de T IDs
au plus : premières entrées et dernières sorties, avec départage par ID.
Il compte aussi les constantes et garde leur coquille. Après ce parcours,
les deux petits tas suffisent à déterminer les rangs H exacts.
Ce départage ne retire **pas** les autres IDs ex æquo : le deuxième
parcours recueille les groupes d'extrémité complets, les événements
intérieurs et les contributions fixes.

Les IDs retenus sont déjà en ordre original. Les groupes d'extrémité
et la coquille constante sont donc collectés triés, sans retri caché
d'une grosse coquille. Seuls les événements strictement intérieurs
sont triés par le comparateur exact réduit q4, puis par ID.
À chaque groupe : retirer les sorties, examiner la profondeur stricte,
chercher une présentation positive/propriétaire/canonique, puis ajouter
les entrées. Un point L=U est traité une seule fois.

Les tableaux et compteurs sont privés à l'appel et réutilisés entre
ses seeds ; le contexte29 reste possédé, immuable et partageable.
Les appels ne collectent toujours pas les intérieurs d'une boule ni
le catalogue global. Une exception conserve les émissions antérieures
et ne renvoie pas de rapport de succès partiel. Aucun nouvel index
du nuage, copie de coordonnées ou file par seed.

## Coût : progrès précis, verrou restant explicite

Pour une seed, sélection O(r log(1+min(T,r))), deux petits tris de tas,
tri intérieur O(min(T,r) log(1+min(T,r))) et collecte linéaire des contacts.
À K fixé, ce producteur de vues est donc O(r), hors travail ajouté par
le consommateur. Mémoire dynamique O(r+min(T,r)), tous buffers simultanés
comptés, pas RSS. Ni réserve mémoire de taille K aveugle ni quota sur
les égalités : K supérieur au nuage reste un cas exact.

**Le produit S×r des scans par seed n'est pas supprimé.** Si les couches29
ne réduisent rien, ce chemin peut encore être quadratique. Ne pas appeler
la suppression du logarithme une preuve de sous-quadratique global.
Les comptes des deux passages, tas, comparaisons de fenêtres, petits tris,
coquilles et préparations doivent être publiés ensemble.

## Étape suivante : exploiter les couches comme un index

L'étude parallèle identifie un objet utile au-delà des seuls IDs29 :
frontières convexes cycliques, groupes de duaux coïncidents et préfixes
de multiplicités. Une seed aiguë a c_x>0, car elle est strictement hors
de la boule de diamètre de son arête : son pivot dual est fini.

Sur un arc de frontière où le dénominateur garde son signe, l'ordre
projectif des racines change de monotonie aux tangences vues du pivot.
Un polygone convexe donne un nombre constant de chaînes, après coupures
aux tangences et à B=0. Fusionner leurs extrémités pourrait trouver les
H rangs sans rescanner r sites par seed. Les orientations doivent rester
des déterminants homogènes réduits, pas des produits naïfs de fractions.

Ce n'est pas implémenté ni qualifié : recherches binaires en présence
de bords plats, pivot sur une arête, ensembles collinéaires, témoins c=0
(index angulaire distinct), partition sans doublons et restitution
ordonnée d'une grosse coquille restent à traiter. Un tri de C contacts
coûte O(C log C) ; ne pas l'annoncer linéaire sans méthode payée.
Les T voisins de la couche de la seed fournissent un préfiltre possible,
pas une borne du résidu sur les autres couches.

Le retour indépendant A, `audits/q4_kernel_composition_20260920/COMPOSITION.md`,
précise aussi que deux noyaux forts ne s'intersectent pas aveuglément :
leurs témoins de rejet peuvent être perdus. Ici la fenêtre porte sur
la population déjà réduite29 et en restitue exactement le census sur
son intervalle ; les certificats sont bien emboîtés. La composition
avec l'atlas ou des couches recentrées reste distincte. La
fixture A à huit sites (`DEGENERACIES.md` dans le même dossier)
confirme l'importance des régions fermées de dimension zéro : son port
constructeur est jugé à nouveau, sans transférer les preuves de l'audit.

Retour A reçu à la clôture, `WINDOW_INDEX.md` : le certificat fermé et
la borne2H−2 sont confirmés indépendamment ; son modèle de508 cas n'est
pas la qualification du port30. Sa comparaison LiDAR28/29 renforce aussi
le choix de garder les blocs28 : sur ses neuf arêtes productives50k/K10,
29 paie14 219 lectures et100 670 comparaisons de tri, contre5628/4725
pour28. Ce sont des preuves d'audit distinctes, pas nos mesures30 ;
la future comparaison de l'index devra inclure ces arêtes réelles et
le coût de préparation, pas seulement les grands denses synthétiques.

## Résultats et périmètre des preuves

Le [dossier de reçus30](../receipts/q4_window_20260920/README.md) distingue
les deux gels, leurs sorties brutes et tous les coûts. Au premier essai
dense permuté32k/K10,29/30 donnent les mêmes candidats : run577,742/84,302ms,
préparation propre et callback compris, hors nuage/index commun et juge.
Les comparaisons de racines totales passent de20 458 496 à2 446 344 ;
ce dernier total inclut1 937 559 comparaisons de tas,21 767 de tri des tas,
480 485 de fenêtre,5262 du petit tri et1271 de regroupement. Ne publier
que les5262 aurait masqué l'essentiel de ce travail.

Le premier scan reste identique29 :1 575 024 sites, auxquels s'ajoutent
254 968 au second. À8k/16k/32k, les deux passages font208488/596136/1829992
visites (×2,859/×3,070) et les comparaisons totales×2,509/×2,766.
Sur l'adversaireK10, le premier passage fait960/3968/16128/65024,
soit×4,133/×4,065/×4,032 : le carré n'est pas supprimé. Les petites
mesures adverses ne qualifient pas ce régime à8k/16k/32k.
Le fond lointain paie même23/25 comparaisons K5/10 contre13 auparavant.
Il n'y a pas de promotion universelle au détriment des chemins28/29.

Les buffers de balayage dense permuté32k/K10 passent de16 416 à320octets,
mais le pic global propre reste1 861 533octets, car la préparation duale
domine. Capacités couplées mesurées, hors nuage/index partagé, pas RSS.
Un essai par configuration sur hôte partagé n'établit pas un gain stable.

La gate compare6206 contrôles à un oracle rationnel indépendant et au
moteur29 : rangs, profondeur stricte, support, boule et coquille complets,
K énorme, invalides, allocations fautives et quatre appels concurrents.
Trois mutations compilées perdant le point L=U, les intérieurs fixes ou
la coquille constante sont tuées sur les sorties géométriques avant les
registres de travail. L'oracle des grandes sondes vérifie chaque boule
publiée sur tout le nuage ; il ne prouve pas seul la complétude globale.

Les premières52 mesures passent ; la première suite Release est en
échec90/91 à cause d'un ancien mutant de reçu q2 parfois inchangé
(`worker_digest=0` déjà nul). Le correctif change toujours un bit dans
l'intervalle u64, avec contrôle causal pour0,1 et2^64−1. Ce seul test
Python rouvre le gel189 ; moteur et binaires sont inchangés, l'échec
et la lecture de clôture prématurée restent conservés. La reprise
distincte passe91 CTests Release, neuf gates/dix sondes Clang ASan/UBSan
(pas91),52 nouvelles mesures, trois mutants compilés et20 différentiels
contre29. Les32 configurations scale sont identiques hors temps entre
les deux gels ; aucune preuve ancienne n'est réécrite comme PASS global.
Les deux builds `v8_q4_window[_sanitize]_20260920` sont épinglés.

WSPD multivoie, q3 global,
catalogue/intérieurs, FULL, GPU/G4 et massif restent ouverts.
s8/10/12 se compare au raccord global, pas à cette arête fournie.
GCP non utilisé.
