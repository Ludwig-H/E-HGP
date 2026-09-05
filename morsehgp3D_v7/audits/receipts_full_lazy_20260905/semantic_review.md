# Raccord sémantique du cache FULL paresseux

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Aucun défaut concret identifié dans le raccord statique lu.** Le header `13c6cc72…` réalise la séparation annoncée entre minima, ancres directes et cache facultatif. Cette revue compare ses octets à e02 capturé ; elle ne constitue ni une compilation ni une qualification du nouveau produit. Le contexte annoncé est `6f4b4de5`, sans commande Git dans cette tâche. Les captures, le diff exact et les pins sont dans [semantic_review.json](semantic_review.json).

La [preuve J1 antérieure](../receipts_full_producer_20260905/lazy_alias_next_step_review.md) reste close. Le présent raccord conserve son autorité relative aux deux catalogues Gabriel fournis, complets, exacts et réguliers ; il n'authentifie pas leur producteur. Le [modèle mémoire](../receipts_full_mono_20260905/memory_model_review.md) distingue toujours stockage logique, dépenses cumulées et RAM. Aucun résultat des anciennes campagnes e02 n'est attribué au nouveau header.

Pendant la clôture, le contrat documentaire est passé de `a3583e6e…` à `05942992…`, avec une section annonçant les résultats de qualification du constructeur. Cette version a été relue et capturée ; ces résultats ne sont ni exécutés ni contre-vérifiés dans la présente sous-tâche. Le header `13c6cc72…` est resté identique.

## 1. Minima séparés et identité des naissances

`minimum_anchor` cherche les minima avant le cache (`full_gabriel.hpp:345–407`). À K>1, la recherche porte sur la clé complète triée. Une correspondance impose un niveau strictement inférieur à la coupe, un jeton installé inférieur à `prior_count`, puis une racine normalisée elle aussi antérieure. Une absence de clé permet de poursuivre vers le cache ; une clé présente mais future refuse, sans la faire passer pour un miss ordinaire.

La liaison `minima[minimum_order[i]].token = prior_count + (i-mb)` (`529–531`) est correcte : les naissances ont été ajoutées dans cet ordre exact, avant les multifusions. Les égalités de niveau sont départagées par clé dans `minimum_order`, donc le tri des labels dans le lot et les IDs concordent. `node_room` contrôle l'addition avant allocation et installation ; le dernier ID est strictement inférieur au nombre obtenu. L'addition de liaison ne crée pas un nouveau risque de débordement ou de collision avec la sentinelle `kAbsent`.

À K1, les naissances à zéro sont les PointId triés ; leur offset donne exactement leur ID. Aucun minimum K1 n'est mis dans le cache, même avec C=0. Toutes les directes admises sont de niveau positif, donc ces jetons sont strictement antérieurs. À K=n admissible, le catalogue direct est vide : la dernière feuille est installée et le certificat est finalisé sans appel au cache. Le delta conserve la borne K≤10.

La disparition du contrôle `late_minimum` contre la table d'alias dans la branche lazy ne crée pas de naissance tardive sur le domaine déclaré. La table des minima est consultée avant toute résolution susceptible de mémoriser F ; si F est dans ce catalogue mais pas encore né, le contrôle de niveau/jeton refuse immédiatement. Les doublons de clés du catalogue restent rejetés avant les lots. En cas de miss géométrique J=0, `full_gabriel_minimum_missing` refuse au lieu de créer une feuille (`423–426`).

## 2. J1, terminal direct et coupe stricte

Le miss est facturé avant sa MEB ; le niveau de F doit être strictement inférieur au lot, puis `intruders` termine le contrôle global de bord (`410–422`). Le helper F inchangé continue de parcourir les branches de bord même après avoir trouvé deux intrus ; il distingue bien J=0, J=1 et J≥2, sans confondre une liste tronquée avec un compte égal à un (`silent_incidence.hpp:294–335`).

Pour J1, la clé F+z est construite avec le PointId exact de l'intrus puis triée. `direct_anchor` (`375–389`) exige la clé entière, l'égalité rationnelle du niveau, une date strictement antérieure, un jeton installé et antérieur, puis normalise jusqu'à la racine courante. Une recherche absente devient `full_gabriel_terminal_missing`. La fonction ne remplace jamais une comparaison de labels par celle du rayon.

La branche J1 ne recalcule ni MEB ni census de F+z et ne charge aucun pas de descente. Elle utilise précisément le certificat de F déjà établi. Dans J≥2, les choix d'intrus, de premier essentiel, la réduction stricte du niveau et la recherche du terminal sont ceux d'e02 ; la recherche terminale a simplement été extraite dans le helper commun. Les contrôles de niveau et d'antériorité y restent dans le même ordre.

Le contrôle des métadonnées géométriques fournies n'est pas devenu exhaustif : le catalogue terminal fait autorité dans le contrat déclaré. Demander une nouvelle MEB de sa coface pour J1 annulerait justement le raccord fermé ; aucun tel recalcul n'est requis par cette revue.

## 3. Cache nul, cache plein et lots atomiques

La nouvelle API exige explicitement `limits.max_aliases==0`, sinon elle renvoie `kInvalidInput/full_gabriel_lazy_alias_budget_conflict` (`113–115`). Le champ `alias_policy` est positionné sur lazy avant toute construction, donc reste interprétable même sur ce refus. L'ancienne API conserve sa politique et son plafond propres.

Dans lazy, `put_alias` n'est atteint qu'après une résolution non minimale ayant fourni sa racine. Si la table est pleine, y compris à C=0, `cache_skips` augmente et la fonction rend succès sans insertion (`287–303`). La racine reste celle rendue à l'appelant ; le lot poursuit donc son union. Une saturation n'est ni une absence de composante ni un refus de travail déjà autorisé. Le cache retient les premières C clés résolues distinctes ; il n'effectue aucune éviction. Un skip peut entraîner plusieurs résolutions ultérieures de la même facette.

Les minima sont des jetons du catalogue, les directes des jetons de leurs records et les successeurs un vecteur indépendant : aucune de ces autorités ne consomme une entrée du cache. Les ancres de toutes les directes sont toujours normalisées après les unions du lot entier, **avant** le `continue` qui supprime l'installation des K+1 alias (`537–550`). Une directe muette conserve donc son ancre. Le lot peut ne publier aucun événement et rester disponible comme terminal ultérieur.

Toutes les recherches de parents ont lieu avant les unions globales (`473–507`). Insérer un cache ou compresser un chemin pendant cette première passe ne modifie pas les composantes du snapshot strict. Une demande ultérieure du même lot peut utiliser cette entrée tout en retrouvant la même racine ancienne. Les minima simultanés sont installés après cette passe ; les groupes de multifusion portent des ensembles disjoints d'anciennes racines. Le cache ne réintroduit donc ni parent du même lot ni fusion binaire artificielle entre égalités.

## 4. Dépenses persistantes et refus transactionnels

Une seule instance de Builder, de limites géométriques et de helper F vit pendant tout l'ordre (`100–111`, `188–206`). Aucun chemin cache plein, J1 ou descente ne réinitialise les compteurs. `face_visits`, `portal_requests`, `chain_steps`, `meb_calls`, les supports/query nodes du helper et les lectures/écritures de successeurs restent chargés avant les opérations correspondantes. Supprimer la seconde passe supprime réellement ses visites et normalisations ; ce n'est pas un remboursement de visites déjà faites.

Les nouveaux compteurs non munis d'un plafond dédié sont dominés par des charges existantes : recherches/hits de minima ou du cache par les visites strictes ; insertions/skips et résolutions J1 par les portails ; recherches de directes par les MEB admises. Leur incrément ne déborde pas dans une tentative publique fraîche respectant les compteurs u64 gardés. Une recherche de catalogue est comptée comme demande, pas comme nombre de comparaisons de clés ; les limites ne sont donc pas un plafond de temps CPU.

`cache_inserts` augmente avant `emplace`. Sur succès il égale la taille de la table et reste ≤C ; lors d'une exception d'allocation, il peut inclure la dernière admission non matérialisée, ce que le contrat documente. `cache_skips` ne rembourse aucune dépense. Les copies des statistiques géométriques se font dans le destructeur du Builder, y compris pendant le déroulement des exceptions. Les deux wrappers conservent les refus `bad_alloc` et `length_error`, la politique choisie et des arènes de résultat vides (`562–603`). Les seules mutations intermédiaires concernent les structures privées ; les catalogues empruntés restent immuables.

Les limites de capacité ne bornent pas toutes les allocations transitoires du lot, des records et de la finalisation. Ce fait était déjà distinct du plafond logique d'alias dans e02. Il ne devient ni un nouveau défaut géométrique ni une garantie RSS du cache.

## 5. Compatibilité historique et portes précises pour le pont

Lorsque `cache_caps==nullptr`, les branches nouvelles de minima et de cache sont inactives. L'installation des naissances et de toutes les facettes, le refus J≤1, les charges historiques et les états publiés conservent la sémantique d'e02. L'extraction de `direct_anchor` ajoute seulement le compteur diagnostique `direct_lookups`, également renseigné en eager. La compatibilité à comparer porte sur les champs historiques et la forêt ; les nouveaux champs changent la disposition de `FullGabrielStats/Result`, sans promesse d'ABI binaire. Les politiques eager/lazy ne promettent pas les mêmes terminaux de budgets entre elles.

Sur une **réussite lazy**, avec T=`face_visits`, V=`portal_requests`, S=`chain_steps` et J1=`singleton_intruder_resolutions`, les points d'appel ferment les identités suivantes :

- `minimum_lookups = T = Σq` ; `minimum_hits + cache_lookups = T`.
- `cache_hits + V = cache_lookups` ; `cache_inserts + cache_skips = V`.
- `terminal_direct = V` ; `meb_calls = geometry.meb_calls = V + S`.
- `direct_lookups = J1 + S` ; `J1 <= V` ; `aliases = alias_hits = 0`.
- À C=0, `cache_inserts = cache_hits = 0` et `cache_skips = V` ; les minima restent accessibles.

La seconde identité de cache utilise la séquence privée : un miss ne peut recevoir une insertion concurrente entre sa recherche et son `put_alias`. Les appels qui touchent une clé déjà mémorisée sont des hits et n'entrent pas dans les portails. Ces égalités ne sont pas toutes exigibles sur un refus ou une exception survenant entre deux compteurs.

Lorsque toutes les strictes résolues restent mémorisées, sans skip, et que les mêmes catalogues, calendrier et choix géométriques sont utilisés, le raccord au modèle antérieur donne `V_lazy−J1_lazy = V_e02` et `S_lazy = S_e02`. Les appels MEB supplémentaires sont exactement J1 ; J1 est au plus le nombre de hits eager. C=0 ou C plein peuvent au contraire répéter des descentes J≥2, donc ces égalités comparatives y seraient un faux attendu.

Les fixtures les plus ciblées pour le pont indépendant sont déjà identifiées : K1/C0 et la feuille K=n ; la fixture J1 à quatre points AB/ABC/ABW ; C=1 avec deuxième clé refusée au cache puis redemandée ; une ancre de directe muette après une fusion intermédiaire ; plusieurs directes dans le même lot ; minimum futur/absent, terminal mal daté ou label modifié ; plafonds de requêtes, MEB et successeurs atteints après des skips ; échec d'allocation après admission au cache avec statistiques conservées et forêt vide. Ces propositions vérifient le raccord effectif, pas une nouvelle preuve de J1. Aucun de ces essais n'a été lancé dans cette revue.

Le contrat documentaire lu décrit correctement ce périmètre. La prochaine décision est la qualification compilée indépendante des deux politiques et de leurs refus, suivie seulement d'une comparaison de coût appariée. Cette note ne qualifie ni le nouveau digest de sonde, ni une CLI FULL, une verticale, une archive, des masses ou un gain de performance.
