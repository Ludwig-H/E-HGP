# Raccord des horloges sparse — Phase 10.11

## Porte d'entrée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique `partial_refinement`, `public_status=not_claimed`. La porte locale est satisfaite par les lots de candidats 10.7, leur localisation finale 10.9 et le balayage exact des préfixes internes du locator 10.10. Le raccord reste conditionnel : aucun artefact courant ne prouve encore quelle frontière du locator était simultanée à un lot 10.7.

Le premier sous-jalon, `10.11-PSTAMP`, reconstruit des stamps historiques du locator en partageant la chaîne SHA-256 de 10.5a. Il prépare le certificat de raccord mais ne porte aucun `source_batch_index` et ne change pas le statut scientifique de la Phase 10.

## Deux horloges sans fait commun

Un lot 10.7 d'indice $s$ est l'une des classes canoniques définies par le couple formé du cardinal de facette et du niveau exact de première incidence. Un préfixe 10.5a d'indice $p$ est l'état après exactement $p$ commits locator. Les deux entiers sont denses dans leurs propres journaux, mais leur égalité numérique ne signifie rien.

Le journal 10.7 ne consulte pas le locator. Il ne possède ni stamp ni digest propre engageant ses cinq arènes scientifiques; ses quatre identifiants canoniques sont des ancres amont. Réciproquement, le locator engage ses clés, unions, témoins et compteurs, mais aucun index, niveau ou digest 10.7.

### Théorème d'impossibilité locale

À partir des seuls artefacts 10.7, 10.9 et 10.5a finaux, aucune fonction $s\mapsto p_s$ ne peut être certifiée comme simultanéité d'exécution.

**Preuve.** Fixons les trois artefacts finaux. Deux orchestrations peuvent produire exactement ces mêmes contenus tout en observant un même lot source avant deux commits locator différents, dès lors que les deltas finaux et leur ordre interne restent identiques. Aucun champ existant ne distingue ces orchestrations. Toute règle reconstruite seulement depuis les artefacts, notamment $p_s=s$, $p_s=s+1$ ou $p_s=T$, renvoie la même valeur dans les deux cas et se trompe donc pour au moins une orchestration. $\square$

Un couple de digests valides ne résout pas le problème : il prouve l'intégrité séparée de deux objets, pas leur simultanéité. Le fait commun doit être produit au moment de la frontière par l'orchestrateur, ou être engagé atomiquement dans un futur schéma du locator.

## Stamps de préfixe partagés

Pour préparer ce fait commun sans dupliquer la sérialisation canonique, `10.11-PSTAMP` demeure dans le target du locator. Les requêtes sont des nombres de commits non décroissants dans $[0,T]$, répétitions permises. Pour un préfixe $p$, le stamp reconstruit contient :

- `committed_batch_count=p`;
- le nombre cumulé de liaisons uniques;
- le nombre cumulé d'unions;
- le nombre cumulé de demandes de liaison, doublons compatibles inclus;
- le digest de chaîne après exactement $p$ transitions.

Le digest initial, les domaines SHA-256, l'encodage des compteurs et le builder de transition sont ceux de 10.5a. Le vérificateur structurel et le balayage de stamps appellent le même helper privé; une seconde transcription du digest est interdite. Un lot vide modifie donc le nombre de commits et le digest, même lorsque les trois autres compteurs restent inchangés.

Pour $Q$ requêtes, $B$ transitions jusqu'au plus grand préfixe, $M$ slots, $I_B$ liaisons actives, $U_B$ unions et $P_B$ identifiants de points engagés par ces liaisons, le coût propre est $O(Q+2B+M\mathbf{1}_{I_B>0}+U_B+I_B+P_B)$, le scratch est $O(I_B)$ et la sortie $O(Q)$. Les huit caps portent sur requêtes, scans de lots, slots, index scratch, unions, liaisons, points et octets scratch exacts. Aucun DSU, parent de composante, snapshot complet par lot ou structure globale n'est construit.

Le helper est exact relativement à un locator vivant gelé. La recertification scientifique ultérieure doit d'abord exécuter le vérificateur structurel borné 10.5a, puis reconstruire les stamps et les comparer. Le coût de cette recertification ne fait pas partie du coût propre ci-dessus.

## Certificat temporel conditionnel

Le sous-jalon suivant devra séparer une ancre externe compacte du payload du certificat. L'ancre fournie par l'appelant contient une autorité temporelle non nulle, un jeton de rejeu non nul et le digest attendu du certificat. Le payload contient :

- une identité canonique propre du journal 10.7 fraîchement rejoué;
- le stamp final du locator gelé;
- exactement une frontière par lot source, avec `source_batch_index`, `strict_pre_locator_prefix_count` et le stamp historique correspondant;
- un digest séparé par domaine de la séquence ordonnée.

Le noyau pourra vérifier la forme dense, l'égalité du digest calculé au digest attendu par l'ancre, chaque stamp historique, le stamp final et la composition avec 10.9/10.10. Cette séparation empêche une mutation du payload suivie de son seul rehachage de passer contre une ancre inchangée. Tant que le journal réel de l'orchestrateur n'est pas rejoué, le noyau doit toutefois conserver `external_clock_authority_replayed=false` et le sens `conditional_on_caller_clock_authority_replay`. Une table assemblée après coup, même auto-cohérente, n'établit pas la simultanéité; remplacer conjointement le payload et l'ancre caller reste une nouvelle prémisse conditionnelle, pas une contradiction détectable localement.

Le cœur ne suppose pas encore que $p_s$ est croissant avec $s$ : 10.7 définit un ordre canonique cardinalité-major, pas une preuve de l'ordre d'exécution du locator. Une autorité séquentielle future pourra ajouter cette prémisse. La composition sparse trie alors les couples réellement attestés par préfixe avant d'appeler un seul balayage 10.10.

## Composition sparse visée

Pour chaque projection 10.9, le couple formé de `source_batch_index` et `localized_facet_token_index` désigne une clé complète existante. Plusieurs projections peuvent partager le même couple et plusieurs lots peuvent partager le même préfixe. Une même clé à deux préfixes distincts ne doit jamais être dédupliquée : elle peut être latente au premier et positive au second.

La sortie temporelle minimale conserve une référence par projection vers une résolution dédupliquée par le couple formé du préfixe attesté et du token. Elle ne recopie pas les clés. Les invariants de monotonie insert-only imposent :

- un hit historique implique un token final positif avec le même témoin de première liaison;
- un token final latent reste latent à tout préfixe historique;
- un token final positif peut être latent à un préfixe antérieur;
- la racine historique peut différer de la racine finale après des unions ultérieures.

Le target reste en lecture seule : aucun `apply_batch`, quotient, union persistante, forêt ou `GatewayAttach` n'est autorisé. Il ne matérialise ni Gamma, cellule, coface globale, tableau dense lots-par-facettes, univers binomial ou mosaïque de Delaunay d'ordre supérieur.

## Portée performance

Le balayage de stamps et le raccord de couples effectivement observés sont linéaires dans leurs arènes sparse. Ils sont compatibles architecturalement avec les profils visés $K\leq10$, 50 000 points interactifs et 10 millions de points ou davantage. Ils n'effacent cependant pas les coûts amont de rejeu 10.7/10.9, l'absence de checkpoint externe ni les copies encore présentes ailleurs dans la voie de référence. Aucun SLO sous la seconde et aucune qualification 10 M+ ne sont revendiqués par ce jalon.

## Falsifications permanentes

La campagne doit couvrir le préfixe zéro, les répétitions, un lot vide, un doublon compatible, le préfixe final, les huit caps exacts puis moins-un, les entrées décroissantes ou hors journal, un stamp vivant périmé et une mutation du digest. Pour le certificat temporel futur, elle doit déplacer une frontière de part et d'autre d'un commit vide, substituer un autre préfixe structurellement valide, omettre ou dupliquer un lot, réutiliser une autorité nulle et prolonger le locator par un commit vide après scellement. Le faux raccourci $p_s=s$ exige deux tests distincts : modifier puis rehacher le payload sous une ancre attendue inchangée doit être rejeté; substituer aussi une nouvelle ancre caller peut seulement produire un résultat conditionnel avec `external_clock_authority_replayed=false`, ce qui matérialise précisément le théorème d'impossibilité locale.
