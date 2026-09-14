# Reprendre le front sans changer sa géométrie ; répartir ensuite sa charge

14 septembre 2026. Auditeur indépendant A, écritures dans `audits/`
uniquement. `exploration_v8_hors_registre / cpu_reference /
quantized_u16_input_only / audit_independant_math_and_architecture /
not_claimed`. GCP non utilisé.

**Une tâche du front peut être reprise avec quatre champs et sans refaire
le travail de ses ancêtres.** Le prototype le vérifie contre le front
public de **ba11e3abfe686d8078c13bacfc066e372d6bfc10**. En revanche,
préparer statiquement 256 sous-arbres laisse encore environ 30 % des
descentes de recherche dans une seule tâche sur LiDAR50k. Le résultat
utile pour le constructeur est donc double : le point de reprise est
disponible ; la répartition dynamique reste nécessaire à explorer.

Ce prototype exécute les tâches **en série, front seul**. Il ne construit
aucun worker, ne lance ni Pool ni census, et ne mesure pas une accélération
parallèle. Le [port Pool](../../docs/P0_POOL_TERMINAL_Q2.md) a été relu
favorablement ; ses points déjà documentés et la campagne indépendante B
ne sont pas répétés ici.

## L'objet à reprendre et sa preuve

[front_tasks.hpp](front_tasks.hpp) adapte explicitement le `front.cpp`
publié, dont les primitives de distance, filtrage et émission conservent
les corps. Les sources produit restent inchangées et sont compilées
séparément pour fournir la référence monolithique.

`Task { a, b, mask, depth }` désigne un produit **pas encore traité**,
dans l'index exact partagé. Le pas indivisible fait le même filtre,
le même test de séparation, puis émet ou engendre les mêmes enfants.
La coupe intervient entre ces pas, jamais au milieu des propositions
de témoins. Orientation, masque après filtrage, profondeur absolue,
K/s/mode et recherche depuis l'index global restent inchangés.

La décomposition diagonale LL/LR/RR partitionne les paires non ordonnées ;
la division d'un facteur disjoint partitionne son produit en deux.
Il faut découper **cet arbre des décisions** : une simple partition du
nuage suivie des calculs intra-partitions oublierait les paires croisées.
Forcer des subdivisions avant le test de séparation changerait aussi
la décomposition du front.

Le préambule développe les vrais enfants en FIFO jusqu'à atteindre la
largeur demandée ou épuiser le front. Des rejets et émissions peuvent
déjà y avoir lieu : ils restent comptés une fois. Chaque tâche pendante
est ensuite traitée par DFS ; une pile est réutilisée entre les jobs.
La largeur peut être dépassée d'une tâche lors d'une division ternaire,
ou ne jamais être atteinte si la file se vide.

Pour une tâche t, poser $\mu(t)=\binom{|A_t|}{2}$ en diagonale, et $\mu(t)=|A_t||B_t|$ sinon. Pour chaque voie active ℓ, la coupe vérifie :

$$R_{\mathrm{prefixe},\ell}+E_{\mathrm{prefixe},\ell}+\sum_{t:\,\ell\in\mathrm{mask}(t)}\mu(t)=\binom{n}{2}.$$

R est la masse rejetée, E la masse déjà émise. La clôture globale
`choose(n,2)` ne doit pas être appliquée séparément à chaque sous-arbre.
Les compteurs additifs et la profondeur maximale restent identiques au
front public. `max_stack_size` devient le maximum des piles DFS locales ;
la FIFO a son propre compteur `maximum_ready`.

Le gate conserve un témoin à sept sites colinéaires
x={0,2,5,8,10,1000,1001}, K2, masque q2|q3 : certaines tâches héritent
seulement q2, tandis que des rectangles ont déjà été émis au préambule.
Réactiver une voie ou perdre ces émissions détruit l'identité de reprise.
Le premier changement n'est pas assimilé à un rejet géométrique non sûr.

## Mesure sur les scans LiDAR

Les [entrées KITTI08 préparées](../lidar08_20260914/README.md) sont
reprises sans téléchargement ni hypothèse d'alignement exact. Kmax10,
s8, Samples, q2 seul ; 24 configurations sur six entrées. Chaque entrée
compare front public, reprise largeur1, largeur64 et largeur256.
Un CPU fixé, hôte partagé, aucune répétition ni warmup. Le callback compte
et hache les rectangles sans les stocker tous. Les jobs sont mesurés
séquentiellement ; leur dispersion n'est pas un p95 de latence produit.

| Entrée | Front public, s | Descentes dans le plus gros job, largeur64 | Même fraction, largeur256 |
| --- | ---: | ---: | ---: |
| Scan 0, 8k | 0,451 | 28,99 % | 26,35 % |
| Scan 100, 8k | 0,427 | 24,83 % | 22,70 % |
| Scan 200, 8k | 0,406 | 27,86 % | 19,99 % |
| Scan 0, 16k | 0,962 | 30,76 % | 27,92 % |
| Scan 0, 32k | 1,936 | 31,23 % | 28,33 % |
| Scan 0, 50k | 3,108 | 32,97 % | 29,93 % |

Les rectangles, digests et **tous les compteurs géométriques** sont
identiques dans les quatre bras. À 50k, la reprise largeur1 prend
3,067 s, largeur64 3,109 s et largeur256 3,137 s. Ces faibles écarts
locaux ne constituent pas un gain ni une régression qualifiée du produit.
Le préambule à256 paie 207 pas et 0,100 ms sur cette entrée ; augmenter
la largeur seule ne résout donc pas le déséquilibre constaté.

À largeur256, le job le plus coûteux représente 52 567 131 paires
initiales et paie **36 884 040 descentes**, soit 0,962 s. Le job de masse
initiale maximale représente 161 064 432 paires, mais ne paie que
52 429 descentes et 1,47 ms. **La masse couverte ne prédit pas à elle
seule le travail des recherches.** Elle reste un invariant de couverture,
et éventuellement une heuristique à mesurer, pas un équilibrage certifié.

Ces durées isolent ce front et son callback de digest. Il serait incorrect
de les soustraire à une ancienne durée front+census pour prétendre mesurer
exactement le reste : interleaving, callbacks et conditions de cache diffèrent.
Ni une largeur pour la chaîne q2 ni une accélération GPU ne sont sélectionnées.

## Raccord parallèle concret à proposer

Conserver un contexte de lot portant l'index et les paramètres, et un
moteur avec vues B, pile et buffers par worker. Une entrée encore synchrone
peut emprunter ce contexte et joindre tous les workers avant son retour :
un `shared_ptr` atomique dans chaque petite tâche n'est pas nécessaire.
Les quatre champs de reprise mesurent ici **32 octets** à l'ABI compilée.

Distribuer dynamiquement des tâches encore pendantes dans les piles DFS,
en les retirant du donneur. Un worker prend un nouveau seed lorsque sa
pile est vide. Si la file globale est pleine, conserver les enfants en
local et continuer le DFS évite de bloquer tous les producteurs sur leurs
propres créations. Le prochain essai doit payer ces transferts, les
workers inactifs, les gros retardataires et le raccord complet au census.
Ce mécanisme n'est pas exécuté par le présent prototype.

La borne de pile actuelle peut être justifiée. Avec δ la profondeur
spatiale, $\Phi(A,B)=\delta(A)+\delta(B)\leq96$ sur le profil u16.
Une division diagonale ajoute deux frères en attente et augmente Φ de2 ;
une division disjointe ajoute un frère et augmente Φ de1. Un DFS local
issu d'un seul seed a donc **au plus97 tâches en pile**. La capture en
observe au plus41. Avec W workers et une file de capacité Q, le plafond
des descripteurs pendants est Q+97W, plus les tâches actives/temporaires.
Empiler des racines indépendantes sur une pile non vide ferait perdre
cette justification.

Cette borne exclut index, états, statistiques, plans Pool et sorties.
Le prototype conserve aussi O(largeur) statistiques de jobs et sa FIFO :
`sizeof(Task)` n'est ni une capacité allouée ni un pic RSS. Un gros plan
Pool partagé entre jobs doit réellement leur survivre, sans être préparé
de nouveau. Les coquilles ne sont pas bornées par K : borner seulement le
nombre de supports en file ne borne pas leurs octets.

Un digest par worker se réduit par additions vérifiées pour les compteurs,
somme modulo 2^64 pour les hashes et xor pour les xor. Un callback opaque
n'est pas réputé concurrent : prévoir des lots possédés et un consommateur
sériel avec contre-pression. Conserver l'ancien ordre DFS global demanderait
un protocole supplémentaire de numérotation et réordonnancement borné.
La couverture ne requiert pas cet ordre. Un pas du front contenant un
callback peut payer tout un rectangle : sa frontière de reprise ne borne
pas le census, la collecte ni le délai de ce callback.

## Preuves et reproduction

- [Gate](gate.cpp) : 9 nuages, deux ordres, 864 références publiques,
  4 320 reprises, oracle scalaire H/Gram sur les paires et voies ;
  289 410 rectangles comparés. Dix rejets d'options, deux exceptions
  callback et quatre mutations de modèles de sortie/compteurs.
- [Release](r1_BUILD.json) et [Clang ASan/UBSan](san_r1_BUILD.json) :
  mêmes résultats du gate, avertissements stricts, fuites activées.
  Le sanitizer porte sur le gate, pas sur la sonde de mesure.
- [Campagne](campaign/MANIFEST.json), [sorties brutes](campaign/MEASURES.jsonl)
  et [clôture](campaign/COMPLETION.json) : 24 lignes sans essai perdu.
  Commandes, entrées, affinité, charge et hashes conservés.
- [Lecture normale](VALIDATION.json) et [lecture optimisée](VALIDATION_OPTIMIZED.json)
  identiques : archives confrontées au Git ba11, sources/binaires
  inchangés, sept corruptions de reçus rejetées.

Depuis la racine, un nouveau build utilise un nom neuf et ne remplace
aucun témoin. Les lecteurs vérifient les binaires/snapshots locaux épinglés ;
une recompilation nouvelle produit ses propres reçus.

```bash
python3 -B morsehgp3D_v8/audits/front_tasks_20260914/build.py --name replay
python3 -B morsehgp3D_v8/audits/front_tasks_20260914/build.py --name san_replay --sanitize
python3 -B morsehgp3D_v8/audits/front_tasks_20260914/verify.py
python3 -B -O morsehgp3D_v8/audits/front_tasks_20260914/verify.py
```

P0, census parallèle, q3/q4 aval, FULL, tour50k/G4 et dizaines de millions
restent ouverts. Les preuves acquises ici concernent la reprise du front.
