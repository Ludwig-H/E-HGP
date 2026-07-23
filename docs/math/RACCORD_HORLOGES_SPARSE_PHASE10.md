# Raccord des horloges sparse — Phase 10.11

## Porte d'entrée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique `partial_refinement`, `public_status=not_claimed`. La porte locale est satisfaite par les lots de candidats 10.7, leur localisation finale 10.9 et le balayage exact des préfixes internes du locator 10.10. Le raccord reste conditionnel : aucun artefact courant ne prouve encore quelle frontière du locator était simultanée à un lot 10.7.

Le premier sous-jalon, `10.11-PSTAMP`, reconstruit des stamps historiques du locator en partageant la chaîne SHA-256 de 10.5a. Il prépare le certificat de raccord mais ne porte aucun `source_batch_index` et ne change pas le statut scientifique de la Phase 10.

Le sous-jalon `10.11-CLOCK` ajoute l'identité canonique propre de 10.7 et vérifie un certificat dense lot-source--préfixe-locator. Il reste strictement conditionnel à une autorité temporelle externe que le noyau ne rejoue pas.

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

## Identité canonique exacte de 10.7

`10.11-CLOCK` engage les cinq arènes de 10.7 sous le domaine SHA-256 `MorseHGP3D/phase10/direct-sparse-gateway-candidate-journal/scientific-identity/v1/sha256/`. L'encodage est indépendant de l'ABI : `u32` et `u64` big-endian, tags d'arène et enums sur un octet explicite, booléens sur l'octet `0` ou `1`, `CanonicalId` sous ses 32 octets binaires. Aucun `sizeof` de record C++, padding, capacité de vecteur, adresse ou ordre natif des octets n'entre dans le préimage.

L'en-tête engage le schéma 10.7, le tag explicite de l'ordre LBVH, le nombre de points, le nombre d'événements sources et les quatre identifiants canoniques amont. Ces quatre ancres sont nécessaires : sans elles, deux sources étrangères dont les cinq arènes sont vides auraient la même identité. Chaque arène est précédée de son tag et de son nombre de records :

- `deletion_projections` encode les sept index, comptes ou `PointId`, les deux enums, le booléen et le niveau exact de selle;
- `facet_tokens` encode l'index, le cardinal et les dix slots `PointId` de la clé fixe, les deux niveaux exacts, les dix-huit compteurs et le booléen de l'audit 10.6, puis les quatre bornes de tranches et l'index de lot;
- `gateway_candidates` encode les trois index ou `PointId`, les quatre slots du support positif, son cardinal et les deux booléens;
- `batches` encode l'index, le cardinal, le niveau exact et la tranche des références;
- `batch_facet_token_indices` encode chaque index sur un `u64`.

Un `ExactLevel` est encodé par la longueur `u64` et les octets décimaux canoniques de son numérateur, puis la longueur `u64` et les octets décimaux canoniques de son dénominateur. La longueur évite toute ambiguïté de concaténation. Le numérateur est non négatif, le dénominateur strictement positif et la fraction déjà réduite par le contrat `ExactLevel`.

Notons $R$ le nombre de projections, $D$ le nombre de tokens, $C$ le nombre de candidats, $S$ le nombre de lots, $I$ le nombre de références de tokens et $E$ la somme des octets décimaux des numérateurs et dénominateurs. La longueur exacte du préimage hors domaine est $194+75R+313D+66C+48S+8I+E$ octets. Les 194 octets fixes comprennent l'en-tête de 149 octets et les cinq cadres `tag+count` de neuf octets chacun. Cette égalité est vérifiée par arithmétique bornée avant le second passage d'encodage.

Le plafond cumulatif $E$ ne suffit pas à protéger la première conversion décimale d'un entier hostile : une chaîne immense pourrait être allouée avant que sa longueur soit comparée au plafond. Chaque composante entière reçoit donc un garde préalable de longueur binaire, avant tout appel à la conversion décimale; la somme exacte des longueurs décimales et celle du préimage sont ensuite vérifiées. Le préflight impose aussi que la longueur du domaine plus celle du préimage reste dans la limite en octets du builder SHA-256. Une valeur `size_t` qui ne tient pas dans `u64` est un dépassement de capacité, jamais une troncature.

## Certificat temporel conditionnel 10.11-CLOCK

L'ancre externe fournie par l'appelant est séparée du certificat. Elle contient une autorité temporelle non nulle, un jeton de rejeu non nul et le digest attendu du certificat. Le certificat recopie l'autorité et le jeton afin de les engager, puis contient :

- une identité canonique propre du journal 10.7 fraîchement rejoué;
- le stamp final du locator gelé;
- exactement une frontière par lot source, avec `source_batch_index`, `strict_pre_locator_prefix_count` et le stamp historique correspondant;
- un digest présent, calculé sous le domaine SHA-256 `MorseHGP3D/phase10/direct-sparse-gateway-clock/certificate/v1/sha256/`.

Un stamp occupe exactement 76 octets : schéma `u32`, autorité `u64`, quatre compteurs `u64` et digest de 32 octets. Le préimage fixe du certificat occupe 136 octets : schéma, autorité, jeton, identité source, stamp final et nombre de frontières. Chaque frontière ajoute 92 octets : index source, préfixe et stamp historique. Pour $S$ lots sources, la longueur exacte du préimage hors domaine est donc $136+92S$ octets. Le digest du certificat et son booléen de présence sont exclus de leur propre préimage. Un digest tout-zéro reste une donnée possible et n'est jamais une sentinelle; la présence et l'égalité fraîche déterminent sa validité.

La table contient exactement $S$ frontières dans l'ordre canonique des lots 10.7 et impose `boundaries[s].source_batch_index=s`. Chaque $p_s$ appartient à $[0,T]$, mais la suite $(p_s)$ n'est pas supposée croissante : l'ordre cardinalité-major de 10.7 ne prouve rien sur l'ordre d'exécution du locator. Le vérificateur copie seulement les couples $(p_s,s)$ dans un scratch borné, les trie canoniquement par préfixe puis index source, appelle une unique reconstruction PSTAMP non décroissante et réassocie les stamps à l'ordre source. Deux lots peuvent partager un préfixe; une même clé observée à deux préfixes distincts ne doit pas être dédupliquée entre ces préfixes.

La vérification fraîche suit l'ordre fermé suivant :

1. préflight des $S$ frontières, des $2S$ scans, des deux vecteurs scratch et de leurs octets exacts, puis consommation du cap avant chaque comparaison du tri;
2. contrôle de l'autorité et du jeton non nuls, de leur égalité entre ancre et payload, et de la présence du digest;
3. rejeu frais complet de 10.7 depuis ses autorités géométriques, avant d'accepter son identité;
4. vérification structurelle complète et bornée du locator vivant sur ses $T$ commits, même lorsque $\max_s p_s<T$;
5. recalcul de l'identité 10.7 et du digest du certificat, puis comparaison au payload et à l'ancre;
6. contrôle dense des frontières, tri scratch, unique balayage PSTAMP et comparaison de chaque stamp historique;
7. égalité du stamp final avec les stamps du locator à l'entrée et à la sortie.

Le journal 10.7, le locator et toutes leurs arènes doivent rester gelés par synchronisation externe pendant l'appel. Les stamps détectent une mutation séquentielle, notamment un commit vide ajouté après scellement, mais ne constituent pas un verrou et ne rendent pas définie une course de données C++.

Cette séparation empêche une mutation du payload suivie de son seul rehachage de passer contre une ancre inchangée. Tant que le journal réel de l'orchestrateur n'est pas rejoué, le noyau conserve toutefois `external_clock_authority_replayed=false` et `conditional_on_caller_clock_authority_replay=true`. Une table assemblée après coup, même auto-cohérente, n'établit pas la simultanéité; remplacer conjointement le payload et l'ancre caller reste une nouvelle prémisse conditionnelle, pas une contradiction détectable localement. `result_certified` signifie donc uniquement « certificat conditionnel fraîchement vérifié » et ne promeut ni `public_status`, ni complétude des gateways, ni exactitude globale de MorseHGP3D.

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

La campagne PSTAMP couvre le préfixe zéro, les répétitions, un lot vide, un doublon compatible, le préfixe final, les huit caps exacts puis moins-un, les entrées décroissantes ou hors journal, un stamp vivant périmé et une mutation du digest.

La campagne CLOCK ajoute :

- les longueurs golden 194 octets pour l'identité 10.7 vide et 136, 228, 320 octets pour zéro, une et deux frontières;
- une mutation distincte dans chacune des cinq arènes 10.7, les quatre ancres amont, le tag LBVH, un enum invalide, un booléen et un niveau exact;
- un entier exact dont la longueur binaire dépasse le garde avant toute allocation décimale, puis chaque plafond exact et moins-un;
- une suite non monotone avec répétition, par exemple $(p_0,p_1,p_2)=(2,0,2)$, ainsi qu'un lot omis, dupliqué ou hors ordre dense;
- une frontière déplacée de part et d'autre d'un commit vide, un stamp d'un autre préfixe pourtant structurellement valide et un stamp d'autorité ou de schéma étranger;
- un suffixe locator corrompu après $\max_s p_s$, afin d'imposer la vérification structurelle complète et pas seulement le rejeu du préfixe utile;
- un commit vide ajouté après scellement, qui doit invalider le stamp final même si les compteurs de liaisons et d'unions sont inchangés;
- les digests absents, tout-zéro, mutés ou intervertis, sans jamais employer zéro comme test de présence;
- une autorité ou un jeton nul, puis une autorité non nulle mais étrangère au payload;
- les dépassements de $136+92S$, du scratch de tri et de la limite d'entrée du builder SHA-256.

Le faux raccourci $p_s=s$ exige deux tests distincts : modifier puis rehacher le payload sous une ancre attendue inchangée doit être rejeté; substituer aussi une nouvelle ancre caller peut seulement produire un résultat conditionnel avec `external_clock_authority_replayed=false`, ce qui matérialise précisément le théorème d'impossibilité locale. Une course réelle writer--reader n'est pas une fixture valide, car elle serait un comportement indéfini C++; la campagne simule les mutations séquentiellement et vérifie séparément le contrat de gel externe.
