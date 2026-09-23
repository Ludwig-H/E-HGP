# Contre-audit B — WIP C6/tri v6 observé pendant la reprise v9

23 septembre 2026, lecture du worktree de développement à `6200bb5a`
avec sept fichiers v6 modifiés/non suivis. **WIP non publié, non qualifié** :
aucun de ces chemins n'est appelé par `morsehgp3D_v9/src/chain` ou
`src/tower`, les deux nouveaux gates ne sont pas dans CMake et aucun test
G4 n'a été lancé pour eux. Les observations ci-dessous sont des demandes
de correction/mesure du prototype v6, **pas** des défauts attribués à la
chaîne v9 publiée ni des gains transférables au profil u18/1 mm.

## C6 : défauts et portée réelle

1. **Compteur réutilisé → refus d'une entrée valide.**
   `route_c6.hpp:175–184` emprunte `*cnt` sans le remettre à zéro ;
   `:253–256` incrémente `c.tours` et le compare au plafond du *nouvel*
   appel `4·L+16`. Deux appels successifs avec le même objet de compteurs
   peuvent donc déclencher à tort « ordonnanceur sans progrès ».
   Réinitialiser la sortie au début, puis ajouter une porte qui réutilise
   le même compteur et compare objets/erreurs aux appels avec compteur
   frais. Les gates actuels créent toujours un compteur frais.

2. **Balayage O(L²) des tickets pour L lots.**
   `route_c6.hpp:491` appelle `LotRing::live_tickets()` à chaque tour ;
   `lot_ring.hpp:357–362` balaie les `L` tickets. Il y a Θ(L) tours,
   soit Θ(L²) inspections. À 20 M candidats et lot 997, L≈20 061 et
   l'ordre de grandeur est 400 M inspections supplémentaires. Un compte
   vivant mis à jour lors de `admit`/`merge_ready` donne O(1) ; pendant
   l'opération active, `lots_admitted−lots_retired` a la même valeur,
   mais `abort_all` remet les états à zéro sans ajuster ces compteurs :
   ne pas étendre cette formule à l'après-abandon sans preuve.

3. **Ni u18 ni GPU réel.** `route_c6.hpp:180,198–205` emploie le wire
   v6 ; `wire.hpp:153–170` refuse les coordonnées/boîtes >65 535 et
   `census_kernels.cuh:101–115` prend `u16*`. Le chemin est un stub
   synchrone, avec vecteurs hôtes, sans allocation device, flux, événement
   ni transfert asynchrone. `lot=0` choisit un lot unique N par défaut
   (`route_c6.hpp:84,207`), et `lot_ring.hpp:298` refuse un lot >2²⁴ :
   ce défaut de configuration n'est pas la route « dizaines de millions ».
   Un port u18 et un backend GPU réel demandent de nouvelles bornes
   arithmétiques, une preuve de baux asynchrones et un reçu G4 séparé.

4. **Résidence encore globale malgré les lots.** Avec `witness=true` par
   défaut (`route_c6.hpp:87`), `LotRing::pending_` et le témoin final
   gardent 8N octets ; tickets, vecteurs temporaires, coquilles et journal
   gardent O(L) octets. `witness=false` retire les 8N de l'ordonnanceur,
   mais l'identité exacte des sorties doit alors rester jugée par une
   autre porte. Mesurer RSS/HWM avec et sans témoin, en comptant candidats
   et sorties sémantiques globales ; le simple nombre d'octets de slots
   n'est pas le pic de résidence.

5. **Transaction latente.** `route_c6.hpp:507–535` échange `surv`/`balls`
   et met à jour `st` avant de vérifier l'ordre complet du témoin. Une
   erreur de ce témoin retournerait un refus avec sorties modifiées,
   contraire au commentaire `:500–503`. Aucun chemin causal n'a été
   trouvé dans l'ordonnanceur synchrone actuel avec les mutants existants :
   les lots finissent déjà en ordre et les erreurs de base/époque sont
   arrêtées avant publication. Classer ce point comme **risque du futur
   ordonnancement asynchrone**, pas comme divergence observée ; vérifier
   le témoin local avant l'échange final et lui donner une injection
   causale avant de qualifier ce contrat.

6. Les nouvelles piles de `census.hpp` sont privées et semblent éviter
   l'allocation **par boule**. Dans `expand.hpp:88–98,128–144,188–203`,
   elles sont créées par **tranche**, environ huit par worker, non une
   seule par worker comme l'indique le commentaire. C'est une nuance de
   coût, pas une course identifiée ; mesurer les allocations et le RSS.

## Tri par permutation v6 : correct en chemin normal, portes incomplètes

- Sous un comparateur à ordre faible strict, le départage par indice
  dans `sort.hpp:291–305` puis les cycles `:311–345` conservent stabilité,
  cardinalité et éléments ; aucune perte/duplication ou course normale
  identifiée à cette lecture.
- `sort.hpp:253–283` crée des `std::thread` sans jointure RAII et ne
  capture pas les exceptions de worker. Un échec de lancement après un
  premier fil déjà actif détruit un `std::thread` joinable et termine le
  processus ; une allocation refusée dans `stable_sort` fait de même
  depuis le worker. Cela contourne le refus `resource_exhausted` attendu
  de `run.hpp`. Le comparateur produit ne lance normalement pas, mais
  l'échec de création de fil reste possible. Ajouter une porte causale
  de panne de lancement et de worker, avec arrêt/jointure de tous les fils.
- Les quatre nouveaux mutants `perm-apply-scatter`,
  `perm-apply-partial`, `perm-tie-desc` et `census-stack-per-ball` ne sont
  pas déclarés dans `core/mutants.hpp:kMutants`. Leur injection est
  refusée en code 2 : les portes « mutants tués » du commentaire ne
  qualifient rien tant que registre, CMake et exécution capturée manquent.
- `run.hpp:518–525` exige encore un budget `2·sizeof(BallCandidate)·N`
  avant le tri. La nouvelle économie de tampon ne peut donc pas rendre
  admissible une entrée rejetée par cette garde. En sens inverse,
  `sort.hpp:48–50,348–350` annonce `8N` octets, mais ne borne pas les
  temporaires internes de `std::stable_sort` parallèles ; mesurer le HWM
  et les allocations, puis ajuster la garde avant tout gain de capacité.

**Priorité d'audit :** corriger d'abord le compteur C6 et la sécurité des
threads, enregistrer les gates/mutants, puis mesurer travail total et
résidence sur des tailles qui font varier **N et L**. La priorité produit
v9 demeure q3/q4 avant expansion, FULL et GPU **u18/1 mm** ; ce WIP v6
ne répond pas encore à ces contrats.
