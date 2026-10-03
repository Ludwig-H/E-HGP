# Assemblage optionnel par blocs

Proposition du 3 octobre 2026, après la source G4 `f718f53aa`.
`CatalogueParams::parallel_assembly=false` conserve le chemin sériel.
Le tri exact choisi, le générateur, les admissions géométriques et le ledger
de génération sont inchangés. Cette tranche n'a encore aucune qualification
native ni mesure de gain. Elle ne constitue pas la tour FULL.

## Contrat et rangs

`AssemblyPlan` est privé au module. Il reçoit les émissions déjà triées,
ou leur permutation bijective produite par `sort_indices`. Les vues sont
empruntées jusqu'au retour ; les émissions restent immobiles et seules leurs
cases `ball.rank` deviennent un scratch de rangs locaux. Une permutation
arbitraire, non bijective ou contenant des indices invalides viole cette
précondition interne ; aucune API publique n'accepte une telle permutation.

Pour B boules, le grain vaut max(4096,ceil(B/1024)) et le nombre de blocs
J=ceil(B/grain) est au plus1024. B<kNone borne tous les produits de positions.
Le découpage dépend de B seulement, jamais de W. Sans Pool, les mêmes blocs
s'exécutent sur le pilote. B=0 donne J=0 et le seul niveau nul.

Le bloc contenant i compare le niveau de i à celui de i−1, y compris sur
sa frontière gauche. Chaque paire adjacente est donc comparée une seule
fois. Le niveau initial est comparé à zéro ; l'ordre et les doublons de
support à niveau égal sont contrôlés. Le halo lit exclusivement Level et
support immuables, jamais le rang écrit par le bloc voisin.

Le rang local compte les débuts de groupes dans le bloc. Il commence à zéro
si le premier groupe prolonge celui du bloc précédent. Le préfixe sériel
ajoute le nombre de groupes ouverts avant chaque bloc. Ainsi le rang final
est prefixe+rang_local, y compris lorsqu'un groupe traverse plusieurs blocs
entiers sans aucune transition. Le nombre de niveaux comprend zéro.

## Copie et représentants

Le scan compte aussi les incidences et valide chaque intervalle source.
Le préfixe fixe une tranche disjointe de populations pour chaque bloc et
vérifie que la somme est exactement la taille de la population d'entrée.
Les sorties sont ensuite admises et allouées selon la formule sérielle.

La copie ne compare plus aucun Level. Une transition locale écrit le niveau
exact du premier record du groupe, avec son numérateur et son dénominateur
non réduits inchangés. Une continuation de rang local zéro n'écrit aucun
Level. Les blocs écrivent des boules et offsets CSR disjoints ; seul le
pilote initialise Level[0] et offset[0]. Les populations gardent leur ordre
I puis U. Chaque fin de bloc et le dernier offset/rang global sont vérifiés.
Les Level de groupes distincts sont également des cases disjointes.

## Mémoire, refus et temps

Le seul scratch supplémentaire est `Buffer<AssemblyBlock>` :32J octets,
au plus32768. Il est admis avant son allocation. Il coexiste avec émissions,
populations sources, permutation éventuelle de4B octets et sorties finales.
Les autres buffers du tri indirect sont déjà rendus. Les réservations
préexistantes et les diagnostics de génération encore vivants s'ajoutent.

La réserve additionnelle des sorties reste B*sizeof(CatalogueBall) +
L*sizeof(Level) +8(B+1) +4P pour P incidences. Il n'y a ni Buffer par tâche,
ni allocation dans les callbacks, ni copie d'émission supplémentaire.
Un défaut de budget peut donc apparaître avec l'option alors que la voie
sérielle était admissible ; aucun repli silencieux ne cache cette différence.

Tous les jobs sont joints avant le retour d'un refus. Une faute tardive
détruit les sorties et le plan privés ; aucun catalogue partiel n'échappe.
Les deux API publiques conservent leurs résultats précédents, et le wrapper
avec Pool publie timings/diagnostics seulement après le succès complet.
L'API interne `Assembly::finish` reçoit un brouillon de timings, pas un
diagnostic utilisateur à préserver sur son propre refus.

L'allocation des métadonnées est ajoutée à allocation_ns ; scan et préfixe
composent level_scan_ns ; la seconde passe compose assembly_ns. Ces durées
sont disjointes du tri et des allocations finales. Les autres coûts de
l'appel ne sont pas supposés nuls. Aucun chrono de travail cumulé des
workers n'est soustrait au temps mur.

## Provenance et portes préparées

La référence est l'assemblage sériel de la v11 `f718f53aa`, relu dans
`src/catalogue/assemble.cpp`. Son corps de copie est seulement extrait en
helper sans changer sa logique. La nouvelle voie par blocs est une
implémentation distincte ; aucune qualification R2 ou v10 n'est héritée.

Les unités préparent des rangs analytiques indépendants de `num::compare`,
des tailles autour de4096 et8192, des groupes égaux traversant plusieurs
blocs, des représentants non réduits proches des budgets18/21/24, les deux
tris et NullPool/W1/W8. Elles contrôlent halo, zéro, doublons, incidences,
metadata exacte et insuffisante, refus Pool réentrant, jointure après
erreur et égalité du catalogue public. Les limites de découpage sont
vérifiées sans fabriquer de faux grands spans.

La sonde de fautes vise les cinq allocations d'assemblage séparément,
puis la dernière allocation de sortie dans le chemin public complet avec
diagnostic antérieur conservé. Six mutations ciblent les verdicts/rangs/CSR
et le représentant exact. Validation de manifeste et lecture statique ne
signifient pas que ces mutants ont compilé ou été tués sur G4.

## Banc préparé

La sonde catalogue étend son masque à0..15 : bit1 cache J2, bit2 tri indirect,
bit4 front adaptatif, bit8 assemblage par blocs. Le masque du banc FULL reste
distinct et inchangé : son bit4 active le mémo des descentes.

`bench/catalogue_assembly.py` déclare36 processus neufs K5/W48. Les trois
sous-nuages LiDAR entiers passent en u21/u24 avec modes3,11,7,15, dans cet ordre ;
les uniformes8k/16k/32k passent en u21/u24 avec modes3,11. Les paires3/11 et7/15
isolent l'option d'assemblage à frontière donnée. Une répétition par case,
ordre fixe : aucune médiane ni robustesse temporelle n'est présumée.

Les sorties canoniques de même profil, résumés sémantiques et comptes
géométriques/J2 doivent coïncider. Cloud, Pool, allocations, scan et copie
restent mesurés ; les temps cumulés des tâches décrivent la génération,
pas l'assemblage. Les diagnostics détaillés par tâche sont désactivés dans
tous les modes de ce banc. Le plafond15s par processus et le budget450s
de campagne peuvent produire des omissions déclarées ; aucune option en
échec ne supprime sa partenaire.

Le plan gardé utilise850/180/530s pour matrice, ASan18 et banc, soit1560s.
L'archive est plafonnée à16MiB ; un tronquage ne devient pas conforme.
Le réemploi facultatif des résumés conserve une lecture SHA256 intégrale
de chaque artefact et les contrôles de la tentative courante avant publication.
Le collecteur passe757 contrôles Python normal/−O, avec333 faux enfants,
138 petits décodages réels, treize campagnes, un cas sans réemploi et des
interruptions avant lancement ou après résultat natif. Les premiers défauts
de préparation des fixtures sont conservés dans les reçus de développement.
