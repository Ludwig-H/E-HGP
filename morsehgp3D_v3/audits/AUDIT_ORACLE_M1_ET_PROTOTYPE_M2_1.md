# Audit historique de l'oracle M1 et du prototype M2.1

> [!CAUTION]
> **Archive figée aux snapshots `8ac683a` et delta B.** Ce fichier conserve
> l'audit hostile du premier oracle et la réfutation de M2.1; il ne décrit plus
> le live ni l'ordre courant des travaux. Le point d'entrée actuel est
> [`AUDIT_DELTA_ORDER_K_FLATS_2532FD5.md`](AUDIT_DELTA_ORDER_K_FLATS_2532FD5.md).

> [!IMPORTANT]
> Snapshot A, commité : `8ac683ad1e167937fe7f9e964860f6be374a48d0`, daté du 8 août 2026 à 23:09:18 UTC; [`oracle_main.cpp`](../oracle/oracle_main.cpp) avait l'empreinte `7787b24804ce79d5f1fa4013e12dff46e2f062c00c5692bdf26e9ad4f4c14a7d` et [`anchored_catalogue.hpp`](../prototype/anchored_catalogue.hpp) `ba5ef6aeb7e5384c9a825d0138fd37967bf9559457347164c78229013b00eeb5`. Snapshot B, delta non commité relu après stabilisation : `oracle_main.cpp` `91d234f53c44e6d2a3977746c3cc714325507722a247349e1988d17a7251a18f`, `anchored_catalogue.hpp` `014da1b9963b36b20779fb17ae3a4705370a0ea5b6fec0c27b562316eb7ef500`, [`CMakeLists.txt`](../CMakeLists.txt) `86a16a6d2ad76eb5614605c1c19b4c6c15791f639beb17722c7d30a6607a0c9a`. Le delta B corrige déjà plusieurs findings du snapshot A; les deux états sont distingués ci-dessous.

> [!NOTE]
> Contexte : `phase=exploration_v3_hors_registre`, `backend=cpu_reference_oracle_under_audit`, `profile=quantized_u16_input_only`, `mode=m1_hostile_judge_and_m2_1_anchored_falsifier`, `public_status=not_claimed`. Ce document audite le juge et son premier sujet expérimental; il n'ouvre aucune porte produit.

> [!CAUTION]
> **Verdict : M1 progresse réellement mais n'est pas encore une autorité qualifiante. Le certificat M2.1 commité est réfuté; le delta B l'a correctement retiré.** Le nouveau régime exhaustif est un bon falsificateur borné et fournit une mesure a posteriori honnête. Il ne constitue toujours ni un certificat local, ni le peeling A2pe, ni une architecture compatible avec les tailles produit.

## 1. Matrice de décision

| composant | décision | justification courte |
| --- | --- | --- |
| `BigInt` signe--magnitude | **GO conditionnel** | représentation indépendante utile; rationnels pas encore différentiels contre GMP |
| `Rational` | **correction réelle** | dénominateur nul et division nulle échouent maintenant fermés |
| fermeture de campagne M1 | **correction réelle** | planchers nuls ou négatifs maintenant refusés |
| catalogue M1 | **GO conditionnel** | centre exact et lecture hostile ajoutés; échec structurel pas encore propagé atomiquement |
| forêt M1 | **NO-GO porte structurelle** | topologie publique mieux comparée, mais une source de multifusion étrangère de même rang et niveau passe encore |
| reçu M1 | **NO-GO reproductibilité** | provenance binaire et digests d'entrées absents; statistiques ancrées incomplètes |
| certificat M2.1 du snapshot A | **réfuté et retiré dans B** | support lointain omis malgré `certified=true` |
| régime `exhaustive` de B | **bon oracle borné** | complet par balayage du nuage entier, pas par localité |
| régime `assumed_window` de B | **diagnostic seulement** | fenêtre explicitement hypothétique et incomplète |
| M2.1 comme architecture | **NO-GO produit** | réénumère les tuples jusqu'à taille quatre; ne construit ni complexe shallow ni peeling |

## 2. Corrections réelles intégrées dans M1

L'audit précédent a provoqué cinq améliorations vérifiables :

1. `--min-decided <= 0` et `--min-nodes <= 0` rendent maintenant le code 2; la campagne entièrement vide ne peut plus passer par cette voie.
2. Le constructeur rationnel refuse un dénominateur nul et la division refuse un diviseur nul. L'ancien `0/0 -> 0/1` a disparu.
3. Le catalogue compare désormais le centre rationnel exact en plus du niveau exact.
4. Les supports, tranches, rangs, IDs et dénominateurs du catalogue sont contrôlés avant leur lecture normale.
5. La forêt contrôle maintenant les liens parent--enfant, `n_children`, les racines, l'ordre, les compteurs et les drapeaux de censure. Le selftest arithmétique sans GMP rend le code 3, sauf mode de développement explicitement demandé.

Ces corrections doivent rester des régressions permanentes. Elles ferment des voies précises; elles ne suffisent pas à qualifier toute la campagne.

## 3. P0 historique — le certificat de localité M2.1 du snapshot A est faux

Le raisonnement commité était : tout membre d'une sphère de rayon $r$ passant par l'ancre $p$ est à distance au plus $2r$ de $p$; le plus grand rayon des supports déjà émis permettrait donc d'écarter tous les points plus lointains.

La première implication est vraie pour une sphère **déjà connue**. La seconde est fausse : le maximum des rayons des supports déjà trouvés n'est pas un majorant du rayon d'un support encore inconnu qui emploie précisément un point exclu. Le certificat utilisait la sortie partielle pour borner ce que cette sortie n'avait pas encore vu.

### 3.1 Contre-exemple entier façonné

Une exécution différentielle hors dépôt a utilisé :

- $p=(1000,1000,1000)$ comme ancre d'identifiant 0;
- vingt voisins proches du côté $x<1000$;
- $q=(2000,1000,1000)$ d'identifiant 21;
- `s_max=3` et `seed_neighbours=16`.

Le snapshot A publiait `certified=1`, `exhausted=0`, `neighbourhood=16`. L'oracle exhaustif trouvait pourtant le support critique de rang 2 `{0,21}` et le catalogue ancré l'omettait : 63 supports de référence contre 61, sans dégénérescence déclarée. L'ancre 21 ne réparait pas l'omission, car le propriétaire canonique du support est l'identifiant 0.

Les inégalités de cette fixture sont ouvertes. Une perturbation transverse suffisamment petite supprime les alignements accidentels sans réparer le raisonnement. Cette contradiction doit devenir une fixture permanente.

### 3.2 Reproduction aléatoire sans fixture spéciale

Le même défaut apparaissait par :

```text
mhgp3v_oracle --subject anchored --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --seed-neighbours 16
```

Résultat observé sur A : code 1, 70 supports de référence contre 69, support `{6,10}` manquant. Les 22 ancres étaient néanmoins annoncées certifiées, aucune épuisée et la taille moyenne/maximale du voisinage valait 16.

Le commit affirmait `ZERO uncertified anchors` et publiait des distributions de `|W|` jusqu'à 1 000 points. Ces chiffres ne mesuraient pas un voisinage complet certifié; ils sont invalidés par le contre-exemple.

## 4. Correction live B — deux régimes honnêtement séparés

Le delta B retire explicitement le faux certificat et nomme deux régimes :

- `exhaustive` utilise tous les points et n'affirme la complétude que par exhaustivité;
- `assumed_window` utilise une fenêtre bornée, annoncée comme hypothèse non certifiée.

Le départage des distances égales par `PointId` a été ajouté. Les noms trompeurs `two_faces` et `certified` sont remplacés par `anchor_member_pairs`, `exhaustive` et `incomplete_anchors`. Le build ajoute désormais la racine v3 aux includes et un CTest `anchored` exhaustif sur des nuages plus grands que l'ancienne fenêtre.

La quantité `sufficient_neighbours` est maintenant calculée, dans le régime exhaustif, depuis le rayon maximal de tous les supports acceptés contenant l'ancre : elle compte les voisins dont la distance à l'ancre est au plus le diamètre maximal. C'est une mesure a posteriori légitime de la fenêtre qui aurait suffi pour **ce catalogue exhaustif**. L'égalité doit bien être incluse, puisque le rang est fermé.

Cette quantité ne devient pas pour autant une règle d'arrêt en ligne : son rayon maximal n'est connu qu'après l'énumération exhaustive. Sous `assumed_window`, elle reste explicitement un minorant et ne doit pas être comparée aux mesures exhaustives comme si elle avait la même sémantique.

## 5. P1 — B reste une cascade exhaustive, pas le peeling proposé

Le fichier énumère tous les supports contenant l'ancre jusqu'à la taille quatre. En régime exhaustif, il paie cette cascade sur tout le nuage; son ordre combinatoire demeure incompatible avec le produit. Il ne construit ni arrangement de médiateurs, ni sous-complexe shallow stratifié, ni préfixe de niveaux, ni peeling.

Le nouveau CTest `anchored` emploie `--max-order 1`, donc `s_max=2`. Il exerce les singletons et les paires, mais ni support trois ni support quatre, précisément les arités qui dominent la cascade et les obligations A2pe. Il faut des tests exhaustifs bornés séparés pour les arités trois et quatre, y compris les égalités et les shells dégénérés.

Le mode `measure-only` est `exhaustive` par défaut. C'est le seul régime où `sufficient_neighbours` est une vraie mesure, mais son coût en énumération ancrée jusqu'à quatre sommets devient rapidement prohibitif. Les anciennes mesures à 200, 500 et 1 000 points ne peuvent pas être reprises : elles venaient du certificat faux. Toute nouvelle mesure large en `assumed_window` devra porter `status=diagnostic_only` et ne dira rien sur la complétude.

Des commentaires aux lignes de l'ancienne fonction parlent encore d'un certificat qui « se ferme » et d'un retour faux après épuisement. Ils sont désormais contraires au code et doivent être corrigés pour éviter une réintroduction du raisonnement réfuté.

Le helper inverse `diameter_squared_at_most` n'est plus utilisé depuis le retrait du certificat. Il devrait disparaître : conserver côte à côte les deux sens d'inégalité, dont celui de l'ancien arrêt invalide, augmente inutilement le risque d'une réintroduction accidentelle.

Enfin, le scan des témoins s'arrête dès que le nombre de membres dépasse `s_max`. Une cosphéricité située après ce point peut ne jamais incrémenter `degenerate_shells`. Le différentiel exhaustif peut encore voir le désaccord de domaine grâce à sa référence, mais `measure-only` sous-compte les dégénérescences et ne doit pas publier ce compteur comme une identité complète.

## 6. P0 — la source d'une multifusion n'est pas réellement comparée

M1 exige maintenant qu'une naissance pointe vers une sphère de rang $k$ et qu'une multifusion pointe vers une sphère de rang $k+1$. Ce garde ne prouve pas que la source a contribué au lot ou à la composante fusionnée.

Un probe hostile a construit deux minima `{0}` et `{1}` fusionnés au même niveau, puis a remplacé la source de la multifusion par une sphère de rang 2 entièrement étrangère `{2,3}`, tout en gardant des liens, compteurs et niveaux cohérents. `compare_forests` a rendu zéro échec.

La référence doit conserver, pour chaque nœud, les cofaces contributrices admissibles au lot. Le juge doit alors vérifier la participation effective et la règle canonique de choix de source, pas seulement le rang et le niveau.

## 7. P1 — la lecture hostile n'échoue pas encore atomiquement

`compare_catalogues` retourne `void`. Lorsqu'un record illisible est trouvé, elle ajoute un échec puis retourne seulement de cette fonction. Le `main` poursuit ensuite avec `compare_forests`, qui peut redéréférencer le même catalogue malformé. Une mutation censée faire rougir le juge peut donc encore le faire sortir de ses tableaux ou l'aborter.

La validation doit produire un booléen ou un type de résultat et interdire toute lecture aval du sujet après le premier échec structurel. Les tests négatifs doivent muter séparément chaque champ public sous ASan/UBSan.

Les sentinelles sont également trop permissives : `parent`, `first_child` et `next_sibling` rejettent les valeurs positives hors bornes, mais toute valeur négative est traitée comme absence. Si le contrat réserve `-1`, `-2` et les valeurs inférieures doivent rougir.

Le catalogue du régime `assumed_window` publie correctement des zéros dans `certified`, mais le juge ne compare pas ce diagnostic et construit quand même les forêts v2. Un passage vert prouve seulement l'accord sur les nuages effectivement comparés; il ne doit jamais promouvoir la fenêtre supposée en algorithme complet.

## 8. P1 — reçus et intégration encore incomplets

Le snapshot A ne compilait pas avec son propre CMake : l'include `prototype/anchored_catalogue.hpp` n'avait pas la racine v3 dans les chemins. Le delta B corrige ce défaut et ajoute le test exhaustif ancré. Cette correction doit être validée sous Release et ASan/UBSan avant commit.

Le delta B corrige le sujet codé en dur dans le JSON et ajoute `regime`, `minimum_clouds_decided` et `minimum_nodes`. Il manque encore :

- `seed_neighbours` et les statistiques ancrées;
- commit, digests des sources et du binaire, compilateur et options de build;
- digest des nuages effectivement générés;
- quotas par arité, égalités, dégénérescences et rejets attendus;
- statut explicite du mode `measure-only`, qui ignore toujours `--receipt`.

Le seed ne suffit pas à reproduire les nuages à travers les bibliothèques standard : la transformation de `std::uniform_int_distribution` n'est pas un format de données portable. Le reçu doit porter le digest de chaque entrée ou définir et versionner son propre mapping RNG.

## 9. P1 — arithmétique encore incomplètement jugée

Le fail-closed sur zéro est une bonne correction. Il reste trois obligations :

- comparer les opérations de `Rational` directement à `mpq_class`, pas seulement tester des identités internes susceptibles de partager le même défaut;
- compter uniquement les assertions réellement exécutées : le selftest ajoute huit vérifications GMP même lorsque le cas zéro en saute plusieurs;
- distinguer clairement un CTest qualifiant exigeant GMP d'un passage de développement `--dev`; le README dit encore seulement que GMP est optionnel.

L'oracle couvre actuellement la grille `quantized_u16_input`. Il n'a ni décodeur ni campagne pour `exact_dyadic_input`.

## 10. Portes minimales avant toute nouvelle revendication

### M1-Q0 — juge hostile

- validation atomique avant comparaison;
- fixture pour chaque champ et chaque sentinelle;
- source de multifusion contributrice et canonique;
- ASan/UBSan sur toute la matrice de mutations;
- comparaison de tous les diagnostics publics du `Result`.

### M1-Q1 — campagne et reçu

- quotas positifs par famille : catalogue, forêt, égalités, dégénérescences et rejets attendus;
- reçu auto-descriptif avec sujet réel, paramètres complets, toolchain et digests;
- profils u16 et dyadique distincts;
- permutations, nombres de fils et ordonnancements.

### M2-F0 — falsificateur ancré

- fixture `{0,21}` et graine 4242/22 points permanentes;
- régime exhaustif obligatoire pour l'autorité;
- test séparé et explicitement non qualifiant de `assumed_window`;
- arités 1/2/3/4, égalités de distance et shells dégénérés;
- compteurs et mesure a posteriori vérifiés contre un calcul indépendant.

### M2-P0 — vrai prototype A2pe

- sous-complexe shallow stratifié éphémère par ancre;
- faces, arêtes, sommets et projections candidates;
- comparaison exhaustive sur petits $n$;
- ledger des plans, strates, conflits et objets HGP aval;
- aucun catalogue global d'ordre supérieur.

## 11. Conclusion

M1 est devenu capable de trouver plusieurs classes de corruption qui lui échappaient. C'est un progrès concret. Son résultat vert doit néanmoins rester un résultat de développement tant que la source des multifusions, la lecture atomique et la provenance ne sont pas fermées.

Le commit `8ac683a` a introduit puis documenté un certificat faux. Le delta B réagit correctement : il retire le claim, garde l'échec comme avertissement et revient à l'exhaustivité pour la seule complétude disponible. Cette réaction est saine. Elle transforme M2.1 en bon falsificateur borné, pas en voie produit. Le prochain geste décisif reste le véritable constructeur du complexe shallow stratifié décrit dans l'[audit mathématique](AUDIT_PROPOSITION_2.md).

GCP non utilisé pour cet audit.
