# Audit Phase 7 — primitive de puissance

## Statut du jalon 7.1

- phase : `7`, `ready`; la phase 5 reste l'unique phase `in_progress`;
- backend : `cuda` pour la primitive candidate, `reference_cpu` pour l'oracle borné;
- profile : `generic_core`;
- mode : `benchmark_only`;
- porte d'entrée : satisfaite par les phases 2B et 4 fermées;
- porte de sortie : non satisfaite;
- statut public : aucun; ce jalon ne produit ni catalogue, ni forêt, ni résultat MorseHGP3D;
- GCP : non utilisé.

Le jalon 7.1 fige la provenance amont, la convention analytique des poids et la taille du premier oracle indépendant. Il ne choisit pas encore entre un fork minimal et une primitive interne.

## Provenance figée

| composant | référence auditée | rôle |
|---|---|---|
| Paragram | dépôt `https://github.com/zenseact/paragram.git`, commit `cadf96c854d27c8234d5b64749b8998e3d1af7f8` | primitive CUDA de proposition |
| cuBQL | gitlink `paragram/external/cubql`, commit `d18c5fa1a5c98665d13484841ae65774da7751e8` | BVH amont |
| licence Paragram | Apache-2.0 au commit figé | autorise un fork sous respect des notices et du journal des modifications |

Le manifeste [`paragram_source_pin.toml`](../references/paragram_source_pin.toml) et le checker `tools/check_paragram_source_pin.py` vérifient cette identité sans sélectionner la primitive pour le produit. Toute mesure future doit enregistrer ces deux commits, le mode fast math, la configuration de chunking et le matériel.

## Résultat de l'audit amont

La surface publique retourne l'adjacence CSR, ses offsets et un statut par site. Elle ne retourne ni les sommets des cellules, ni leurs plans liants, ni les incidences sommet--plan nécessaires au rejeu exact. Le noyau emploie des capacités fixes de 64 plans et 64 sommets; les statuts non nuls distinguent notamment les overflows de plans ou de sommets, la cellule vide, le rayon de sécurité non atteint et une frontière incohérente.

Ces statuts ne ferment pas toute perte. Le parcours du BVH de puissance conserve une pile de 64 entrées et, lorsqu'elle est pleine, évince silencieusement l'entrée la plus éloignée si une entrée plus proche arrive. Cette éviction ne possède pas de statut de complétude. Le succès d'une cellule ne prouve donc pas que tous les coupeurs pertinents ont été visités. Le chunking borne la mémoire d'exécution mais ne transforme pas cette proposition en certificat.

La compilation active `-use_fast_math` par défaut et `NO_FAST_MATH=1` le désactive. La boîte amont est l'AABB flottante du BVH paddée par la constante `1.0f`; elle n'est ni la boîte dyadique explicite ni le témoin de padding exigés par le contrat MorseHGP3D.

## Décision d'architecture

L'adaptateur direct de la surface amont est rejeté. Il ne peut fournir les objets requis pour reconstruire et fermer une cellule, et un statut `success` ne couvre pas l'éviction de pile du BVH.

Un fork minimal épinglé reste candidat, sans être choisi. Pour devenir évaluable, il devra au minimum :

1. recevoir la boîte dyadique explicite de l'appelant sans padding caché;
2. exposer les sommets proposés, les plans actifs et leur provenance site ou face de boîte;
3. exposer les incidences sommet--plan avant compactage de l'adjacence;
4. transformer toute éviction du parcours, tout overflow et toute frontière incohérente en statut fail-closed;
5. conserver deux builds identifiables, avec et sans fast math;
6. documenter les modifications et notices Apache-2.0.

La primitive interne reste le repli si ces modifications deviennent plus complexes qu'un clipper spécialisé et rejouable.

## Jalon 7.2 — série candidate fail-closed

Le deuxième jalon ne sélectionne toujours pas Paragram. Il isole le plus petit correctif qui retire l'impasse identifiée en 7.1 : une pile BVH pleine ne doit jamais devenir une décision implicite de pruning. La série atomique est conservée sous [`third_party/paragram`](../../third_party/paragram/cadf96c854d27c8234d5b64749b8998e3d1af7f8/README.md). Elle part exactement de l'arbre `ad17c2eff4c5beec43330f597ee5d6616b62d32d` et produit l'arbre candidat `591ed6a3257ecd0be8544ba129e08c081ed4eb80`.

Le patch conserve les codes historiques de statut 0 à 5, partage une seule ABI `Status : int` entre cellules ordinaires et pondérées, puis réserve le code 6 à `traversal_stack_overflow`. Les parcours de boîte retournent désormais un résultat typé. Chaque tentative de push sur une pile pleine arrête la cellule avec ce statut, sans éviction ni remplacement; toute branche lointaine dont la marge vaut exactement zéro est conservée comme non strictement exclue; le helper de rayon inutilisé qui possédait une autre omission bornée est supprimé. Une erreur géométrique déjà acquise reste prioritaire.

Une cellule de statut non nul n'écrit plus d'adjacence brute. Le compactage CSR conserve ensuite une arête uniquement si sa source et sa destination sont toutes deux en succès. Cette vérification bidirectionnelle précède la symétrisation et n'ajoute aucun retour anticipé autour des barrières CUDA du bloc. Ainsi une cellule fautive ne peut ni publier sa ligne, ni être réintroduite comme destination par une cellule saine.

Le manifeste `series.toml` impose l'application complète du patch, son SHA-256 `e80e330918a748d969a745671b2aa5604473a5180ecb5243d10d84558789c6eb`, sa taille, ses neuf chemins autorisés et les arbres Git avant et après. `tools/check_paragram_patch_series.py` repart d'un checkout local validé par le pin 7.1, applique la série dans un index et un worktree temporaires sans réseau, puis contrôle les actions de fichiers, les arbres, la licence, le gitlink cuBQL et les invariants structuraux fail-closed. Sa fixture hôte de quarantaine ne conserve que l'arête entre deux cellules saines lorsque les autres extrémités portent des statuts d'erreur. Les essais négatifs courts refusent un checkout sale, les renames, binaires, changements de mode et octets NUL, une configuration Git injectée et le retour de l'une ou l'autre tangence à une comparaison stricte.

Cette validation est structurelle et hors ligne. Le patch n'a pas encore été compilé avec NVCC ou PyTorch, aucun overflow forcé n'a été observé sur GPU et le statut 0 reste une proposition flottante. Il ne corrige pas encore l'emploi amont du stream CUDA par défaut ni les retours ignorés de certaines synchronisations; une faute CUDA asynchrone n'est donc pas couverte par la garantie ciblée de débordement sémantique. La boîte vient encore de l'amont et aucun plan, sommet ou incidence n'est exporté. Le jalon 7.2 ne satisfait donc ni la porte de sortie de la phase 7, ni celle de la phase 8.

## Jalon 7.3 — boîte explicite sans marge cachée

Le patch `0002-explicit-caller-aabb.patch`, de SHA-256 `07a2d798f8cf74064c13503062cfd7a1a4a2bc67a8f73bec24084a7178416215`, s'applique après 7.2 et produit l'arbre candidat `732b1c2f9f42aa452a3282483ec9a8d947000497`. Les deux API exigent désormais `bounds` comme argument keyword-only. La valeur est un tensor CPU `float32` de forme `(2,3)`, fini et strictement ordonné sur chaque axe. Une vue non contiguë est copiée sans conversion et conserve les six mots binary32.

La frontière C++ répète ces contrôles avant toute activité GPU. Les appels directs valident aussi les points, poids et guesses, leur co-présence, dtype, rang et identité de device avant `.contiguous()`. Un `CUDAGuard` fixe ensuite le device des points pour la durée de l'appel et restaure le précédent au retour; il ne sélectionne pas encore le stream PyTorch courant.

Les six composantes de `world_bounds` viennent uniquement de la boîte validée. Les deux cellules suppriment `CUBE_EPSILON` et construisent exactement leurs six plans de boîte. Les noyaux ne copient plus la racine BVH et ne lui ajoutent plus `1.0f`; leur `BoxRadii` initial est calculé depuis `cell.lower_bound` et `cell.upper_bound` immédiatement après `CCInit`, au lieu du huituple fixe `1e10`.

Les 18 tests CPU d'interface passent en 1,50 s. Ils couvrent notamment la conservation bit à bit, les valeurs non finies, les largeurs nulles ou inversées sur les trois axes, les zéros signés et la signature publique. Le header hôte passe aussi une vérification syntaxique Clang; deux avertissements C++20 proviennent des headers PyTorch, pas du candidat. Le checker hors ligne rejoue les deux patchs séquentiellement, reproduit l'arbre final et rejette quatre mutations ciblées de l'AABB ou du device guard.

La portée demeure bornée : les sites ne sont pas vérifiés dans la boîte, les extrêmes finis sont seulement acceptés par l'entrée, les sommes de carrés binary32 de `BoxRadii` ne sont pas encore des majorants à arrondi dirigé, le stream courant et les erreurs CUDA asynchrones restent ouverts, et aucun plan, sommet ou incidence n'est exporté. Aucune compilation NVCC, aucun binding CUDA réel, aucun G4 et aucun statut public ne découlent de 7.3.

## Convention analytique des poids

Pour le site $p_i$ de poids $w_i$, la puissance utilisée par l'audit est

$$\delta_i(y)=\left\Vert y-p_i\right\Vert^2-w_i.$$

La cellule de $i$ dans une boîte dyadique explicite $\Omega$ est définie par $\Phi_{ij}(y)=\delta_i(y)-\delta_j(y)\leq0$ pour tout $j\neq i$, soit

$$\Phi_{ij}(y)=2(p_j-p_i)\mathbin{\cdot}y+\left\Vert p_i\right\Vert^2-\left\Vert p_j\right\Vert^2-w_i+w_j.$$

Un poids $w_i$ plus grand agrandit donc la cellule de $i$, et ajouter la même constante à tous les poids ne change aucune cellule. Les cas analytiques du jalon doivent vérifier ces deux conséquences avant toute comparaison de débit.

## Oracle hôte borné implémenté

Le premier oracle est volontairement limité à $1\leq n\leq8$. Pour une cellule, il reçoit les sites et poids dyadiques exacts ainsi que les six demi-espaces d'une boîte $\Omega$ explicite. Il construit au plus

$$P=6+(n-1)\leq13$$

plans. Il énumère les triplets de plans, rejette exactement les rangs inférieurs à trois, calcule l'intersection rationnelle unique des autres, teste toutes les contraintes puis déduplique les sommets rationnels. Les capacités conservatives sont

$$\binom{13}{3}=286\ \text{triplets},\qquad V\leq286\ \text{sommets},\qquad PV\leq13\times286=3718\ \text{incidences}.$$

Ces bornes dimensionnent un falsificateur court; elles ne sont pas une revendication de complexité scalable. `build_exact_bounded_power_cell_reference` réutilise les formes affines, plans et intersections rationnelles existants, trie les concurrents par identifiant et reconstruit toutes les incidences actives. Ses décisions complètes portent sur le polyèdre local fermé, y compris les sites confondus et les cellules de dimension inférieure; elles ne sont pas un certificat de position générale.

Les tests hôte ferment le cube symétrique, la couture avec le bisecteur singleton non pondéré, les poids signés et leur translation commune, les sites confondus, une cellule vide par contraintes propres incompatibles, les dimensions deux, un et zéro, la permutation canonique, les cinq budgets juste insuffisants et les entrées hors contrat. Les targets et CTests ciblés passent sous GCC et Clang stricts en moins de 0,02 s chacun. Aucun différentiel Paragram, sanitizer, benchmark, CUDA ou G4 n'en découle.

## Frontière proposition--certification

- Paragram, avec ou sans fast math, émet seulement une proposition flottante d'adjacence ou de géométrie future;
- un statut amont non nul interdit d'accepter la proposition de la cellule concernée sans recertification;
- un statut amont nul ne certifie pas sa complétude;
- l'oracle hôte borné décide exactement sa propre cellule locale et sert de comparateur indépendant;
- seule une recertification ultérieure de tous les plans requis, sommets et incidences pourra fermer une cellule du pipeline;
- aucun accord moyen, benchmark ou différentiel borné ne crée un `public_status=exact`.

## Prochaine décision

Le jalon suivant chiffre séparément l'export des plans, sommets et incidences sans l'implémenter d'avance. Il doit aussi décider si les rayons dirigés, le stream courant et la propagation d'erreurs peuvent rester hors de cuBQL. Si ces coutures restent locales, le fork candidat sera compilé avec une capacité de pile forcée à un et confronté à l'oracle avec et sans fast math. Si elles propagent une refonte dans le builder cuBQL ou exigent une seconde représentation géométrique non rejouable, le fork sera abandonné au profit d'un clipper interne spécialisé. Une expérience G4 courte ne devient utile qu'après cette décision, la compilation et l'exposition de la géométrie. La phase 7 restera `ready` jusqu'au choix documenté et à l'enregistrement des mesures requises.
