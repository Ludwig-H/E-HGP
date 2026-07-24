# Tour verticale directe compacte — Phase 11A

## Statut et séparation des autorités

La Phase 1 possède déjà l'autorité verticale exacte de référence : `reference_cpu / hgp_reduced / certified`, base `gamma_exhaustive_reference`. Elle construit les applications entre coupes Gamma adjacentes, vérifie l'unicité de chaque cible, projette vers les composantes non triviales du profil réduit et ferme les carrés de naturalité. Cet oracle reste borné et extérieur au chemin produit.

Le présent incrément construit seulement la couture directe `reference_cpu / hgp_reduced / certified`, avec sémantique `conditional_vertical_candidate`, déploiement `architecture_only` et `public_status=not_claimed`. Sa source horizontale est le journal direct conditionnel de Phase 10. Les graines de cible sont fournies par une autorité externe séparée, jamais auto-attestée par le journal. Même une sortie localement complète conserve donc `external_target_authority_replayed=false`, `global_morse_obligation_replayed=false` et `vertical_maps_complete=false`.

Les identifiants de nœuds du journal direct sont locaux au rejeu. Cette couche ne sérialise aucun `VerticalMap` v2, ne calcule aucun identifiant public et ne transforme pas un indice local en identité durable.

Le périmètre réduit omet volontairement les naissances isolées d'ordre supérieur : chaque famille adjacente compte les `birth_records` sources d'ordre au moins deux qui restent des carriers latents sans nœud source. Ce compte est distinct d'une famille réellement vide. Une cible externe absente ou non résolue n'est en revanche jamais reclassée comme composante isolée omise : elle reste une dette de résolution explicite.

## Pourquoi une proposition par nœud ne suffit pas

Une continuation $q_R=1$ ne crée aucun nœud, mais peut ajouter de nouveaux carriers et de nouveaux labels à la composante. Une table contenant seulement une flèche par nœud ignorerait donc exactement la régression où la cible inférieure grandit après la naissance du nœud.

Pour chaque groupe atomique direct $g$, 11A parcourt tous ses `saddle_records`, puis tous leurs `arm_root_bindings`. Les `strict_arm_key` sont triées et dédupliquées; pour chaque clé distincte, le plus petit `binding_index` devient son représentant compact. La requête et la résolution conservent cet indice, jamais une seconde copie des `PointId`.

Si $A_g$ est le nombre de références de bras du groupe et $R_g$ son nombre de clés distinctes, alors $R_g\leq A_g$. Sur toute la forêt, $R=\sum_g R_g\leq A=\sum_g A_g$. Le tri transitoire contient seulement $A_g$ indices et le scratch maximal vaut $O(\max_g A_g)$; aucune arène globale de facettes, cofaces ou incidences n'est créée.

## État fermé de la cible

Pour un groupe source d'ordre $s\geq2$ et de niveau carré exact $a$, une graine cible doit désigner un nœud d'ordre $s-1$ né à un niveau inférieur ou égal à $a$. Un sweep de la forêt cible applique exactement les arêtes enfant--parent dont le parent est né au plus à $a$. Sa DSU transitoire donne la racine fermée

$$\rho_{s-1}(v,a)=\text{racine active contenant le nœud cible }v\text{ après tous les lots de niveau au plus }a.$$

Une graine d'ordre incorrecte, hors domaine ou née après $a$ est invalide. Une graine ancienne peut être normalisée par la chaîne de parents jusqu'à $\rho_{s-1}(v,a)$. Le sweep traite seulement deux ordres adjacents, libère sa DSU avant la famille suivante et ne conserve aucun snapshot par niveau.

Un budget borne séparément les scans de la source, les requêtes, les résolutions, les groupes, les checkpoints, les références de racines antérieures, le scratch d'indices, les comparaisons de tri et les sauts de parents. Les tailles et les entiers des niveaux exacts sont contrôlés avant toute comparaison potentiellement coûteuse. Tout dépassement publie un échec atomique sans préfixe scientifique.

## Règles par groupe

Chaque valeur présente parmi les labels et les ancres antérieures est d'abord normalisée dans le même état cible fermé. Deux valeurs présentes distinctes constituent une contradiction, pas une simple résolution manquante.

### Naissance réduite, $q_R=0$

Le groupe ne possède aucune racine source antérieure. Si toutes ses requêtes de labels sont résolues et donnent la même cible $T$, 11A pose un checkpoint sur le nœud créé. Une requête `missing` ou `unresolved` rend le contrôle partiel. Des cibles présentes en désaccord font échouer tout le journal.

### Continuation, $q_R=1$

Le `resulting_root_node_id` reste le nœud source courant. Son dernier checkpoint éventuel est propagé horizontalement jusqu'au niveau $a$, puis comparé à toutes les cibles de labels présentes.

Si aucune ancre antérieure complète n'existe mais que tous les labels du groupe sont résolus vers le même $T$, 11A peut créer un `late_q1_checkpoint` au niveau $a$. Ce checkpoint permet les contrôles futurs; il ne rétro-certifie jamais la naissance ni les niveaux antérieurs. Si un label manque, le groupe reste partiel même lorsque l'ancienne ancre se propage mécaniquement.

### Multifusion, $q_R\geq2$

Les racines sources antérieures sont exactement les enfants du nœud créé. Chaque dernier checkpoint enfant est propagé jusqu'à $a$ et comparé aux cibles de tous les labels du groupe. Une cible commune peut ancrer le parent. Le groupe n'est complètement vérifié que si tous les labels et toutes les racines antérieures requises sont résolus; une ancre de parent obtenue malgré un passé incomplet ne transforme pas les carrés enfants manquants en preuves.

## Comptabilité des contrôles élémentaires

Une multifusion contribue un carré élémentaire attendu par racine source enfant; une continuation contribue le contrôle élémentaire de son unique racine courante. Les naissances $q_R=0$ n'ont pas de carré horizontal antérieur, mais exigent la résolution complète de leurs labels pour une ancre complète.

Le journal publie séparément :

- le nombre de labels attendus, présents, résolus, manquants et non résolus;
- le nombre de naissances sources isolées d'ordre supérieur omises du profil réduit;
- le nombre de groupes complets et partiels;
- le nombre de carrés élémentaires de groupe attendus, vérifiés et non vérifiables;
- le nombre de contradictions, nécessairement nul pour toute issue certifiée;
- les checkpoints de naissance et les checkpoints tardifs, avec leur niveau et leur caractère complet ou partiel.

L'identité `checked_elementary_group_squares + unresolved_elementary_group_squares = expected_elementary_group_squares` est fermée sans débordement. Zéro contradiction n'implique jamais la complétude lorsqu'un contrôle reste non vérifiable ou qu'un label manque.

Ces compteurs ne sont pas ceux de l'oracle Gamma, qui vérifie les carrés entre toutes les coupes ouvertes et fermées consécutives. Les contrôles de 11A portent sur les générateurs horizontaux visibles du journal direct et se composent seulement relativement à des ancres totales et à une couverture complète. Tant que les continuations et couvertures directes ne sont pas recertifiées contre Gamma, le fait `all_naturality_squares_replayed` reste faux.

## Coût et structures évitées

Avec $V$ nœuds directs, $E$ arêtes de forêt, $G$ groupes, $A$ références de bras et $R\leq A$ labels distincts, le travail propre est $O(V+E+R+\sum_g A_g\log A_g)$, augmenté des sauts DSU bornés. Le scratch vivant est $O(V+\max_g A_g)$ pour une seule paire d'ordres. Le payload compact est $O(R+G+E+V)$ dans le pire cas et ne recopie aucune clé.

11A ne construit ni Gamma, ni cellule, ni coface globale, ni catalogue d'incidences, ni mosaïque de Delaunay d'ordre supérieur. L'oracle Gamma de Phase 1 peut falsifier ou recertifier une petite sortie, mais il n'est ni appelé ni réimplémenté par le target produit.

## Conséquence pour les phases industrielles

La porte de Phase 11 peut distinguer deux conclusions :

1. les applications Gamma exactes de référence sont déjà uniques et naturelles sous l'autorité bornée de Phase 1;
2. le flot direct possède désormais une représentation verticale compacte qui reste partielle ou conditionnelle tant que ses graines externes et la fidélité horizontale globale ne sont pas rejouées.

Cette séparation autorise l'architecture de streaming de Phase 15 à transporter des résolutions, contrôles et checkpoints explicites sans prétendre que le flot direct possède `vertical_maps_complete=true`. Aucun benchmark, accord moyen ou grand nuage ne peut lever cette condition.
