# Quotient local des gateways historiques — Phase 10.14-QPROP

## Statut et portée

Le jalon 10.14-QPROP conserve `backend=reference_cpu`, `profile=hgp_reduced`, `mode=certified`, la sémantique `partial_refinement` et `public_status=not_claimed`. Il transforme les résolutions temporelles 10.13-TRES en une proposition de quotient strictement locale à chaque lot 10.7. Il reste en lecture seule : il ne publie aucune action HGP et ne modifie ni locator, ni racine, ni forêt.

La porte d'entrée locale exige un journal 10.7 fraîchement rejoué, une localisation 10.9 et une résolution temporelle 10.13 fraîchement vérifiées contre les mêmes autorités, ainsi qu'une correspondance exacte entre chaque projection de suppression, son candidat factorisé, son lot source et sa résolution au préfixe strictement pré-lot. La discipline pré-lot de l'orchestrateur, le gel synchronisé du locator et l'autorité scientifique externe des témoins de liaison restent des prémisses explicites.

## Sommets typés

Soient $C$ le nombre de candidats factorisés 10.7 et $P$ le nombre de leurs projections de suppression 10.9. Pour un candidat $c$, notons $b(c)$ son lot canonique et $J_c$ l'ensemble de ses suppressions. Comme sa coface logique possède au plus $K+1$ facettes et que $K\leq10$ :

$$P=\sum_c\lvert J_c\rvert\leq(K+1)C\leq11C.$$

Chaque projection $j\in J_c$ référence une résolution temporelle $q(j)$. Au préfixe strictement pré-lot de $b(c)$, cette résolution est soit positive avec une vraie racine historique $r(j)$, soit latente sans payload positif. On lui associe le sommet typé :

$$v(j)=\begin{cases}R(r(j))&\text{si la résolution est positive},\\L(q(j))&\text{si la résolution est latente}.\end{cases}$$

Les tags $R$ et $L$ font partie de l'identité : une valeur numérique de handle égale à un indice de résolution ne crée jamais une collision. Les sommets sont en outre placés dans l'espace de noms de leur lot. Deux occurrences `R(root)` ou `L(resolution)` égales dans un même lot désignent le même sommet; la même étiquette observée dans deux lots distincts ne crée aucune arête entre eux.

Une latence reste une information positive sur le protocole : la facette n'avait aucune liaison dans le domaine positif historique consulté. Elle ne signifie ni absence géométrique, ni isolation, ni singleton.

## Hyperarêtes et clôture locale

Pour chaque candidat $c$ du lot $b$, la coface logique factorisée définit l'hyperarête :

$$E_c=\lbrace v(j):j\in J_c\rbrace.$$

La relation $\sim_b$ est la plus petite relation d'équivalence sur les sommets touchés du lot $b$ qui identifie toutes les paires de sommets d'une même hyperarête. De manière équivalente, deux sommets sont équivalents si un chemin alternant hyperarêtes candidates et sommets typés les relie sans quitter $b$.

Cette clôture est simultanée et strictement locale au lot. L'ordre des candidats dans le lot n'a aucun effet sur le résultat. En revanche, aucune fermeture n'est propagée d'un lot au suivant : 10.14 ne simule pas l'application séquentielle des actions d'une forêt et ne remplace pas une transaction HGP.

Une DSU de scratch sur les candidats peut calculer cette clôture : deux candidats du même lot sont unis lorsqu'ils partagent un sommet typé, et le tag de lot interdit toute union transverse. Elle n'est ni une DSU du locator, ni une DSU de racines persistantes, et aucun de ses représentants internes n'est publié comme identifiant scientifique.

## Le pont latent

Les sommets latents doivent participer à la clôture avant la projection sur les racines. Par exemple :

$$E_{c_1}=\lbrace R(r_1),L(q)\rbrace,\qquad E_{c_2}=\lbrace L(q),R(r_2)\rbrace\quad\Longrightarrow\quad R(r_1)\sim_b R(r_2).$$

Projeter séparément chaque hyperarête sur ses seules racines donnerait deux singletons et perdrait cette connexion. Le sommet `L(q)` est donc un pont simultané légitime entre cofaces du même lot, tout en restant distinct d'une racine. Cette règle capture exactement la raison combinatoire pour laquelle une facette nouvelle au niveau du lot doit être conservée même lorsqu'elle n'était rattachée à aucune composante stricte.

Le pont exige l'identité complète de la résolution temporelle. Deux tokens distincts, ou le même token résolu à deux préfixes distincts, ne partagent pas un sommet latent. La déduplication temporelle 10.13 par couple `(préfixe, token)` est ainsi préservée; dédupliquer par la seule clé de facette serait incorrect.

## Projection des vraies racines

Pour une classe connexe $A$ de $\sim_b$, sa projection de racines est :

$$\rho(A)=\lbrace r:R(r)\in A\rbrace.$$

La fermeture est calculée sur $R\sqcup L$, puis seulement projetée par $\rho$. Les handles de $\rho(A)$ sont dédupliqués et ordonnés canoniquement. Deux vraies racines sont proposées dans le même groupe de quotient local si et seulement si leurs sommets appartiennent à la même classe après tous les ponts latents du lot.

Ici « vraie racine » signifie un handle de composante positivement présent dans le locator au préfixe historique, par opposition à un token latent ou à un identifiant synthétique. Ce handle reste relatif au snapshot et n'est pas, par cette seule projection, une racine publique persistante de la hiérarchie.

Trois dispositions purement combinatoires restent distinguées :

- si $\lvert\rho(A)\rvert=0$, la classe est `latent_only_unresolved`;
- si $\lvert\rho(A)\rvert=1$, elle est `one_known_root_class`;
- si $\lvert\rho(A)\rvert\geq2$, elle est `multiple_known_root_class`.

Ces dispositions décrivent uniquement le cardinal des racines connues dans la projection combinatoire observée; elles ne signifient ni naissance, ni continuation, ni multifusion HGP. En particulier, une classe uniquement latente ne devient jamais une naissance. Même si plusieurs facettes latentes et plusieurs candidats y sont connectés, le sous-univers direct fourni ne prouve ni qu'une composante réduite globale vient de naître, ni que toutes les cofaces silencieuses pertinentes sont présentes. 10.14 n'invente donc aucun identifiant de racine et n'émet aucun `RootBirth`.

## Sorties plates et canonicité

La sortie possède exactement cinq arènes scientifiques plates :

- `batches` décrit chaque lot source dense, son préfixe, son nombre de candidats et sa tranche de composants;
- `candidate_to_component`, conservée dans l'ordre des `gateway_candidates` 10.7, lie chaque candidat à son lot et à son composant, puis référence sa tranche existante dans `deletion_projections` 10.9;
- `components` décrit les classes batch-locales et les tranches contiguës de leur projection;
- `component_root_ids` conserve les handles de racines vraies, strictement croissants dans chaque tranche;
- `component_latent_resolution_indices` conserve les indices 10.13 des sommets latents, strictement croissants dans chaque tranche.

Il n'existe donc aucune sixième arène d'incidences typées : les couples `(deletion_projection_offset, deletion_projection_count)` référencent les occurrences canoniques déjà publiées par 10.9, sans recopier clé de facette, `PointId` ou record de projection. Chaque sommet `L` publié conserve explicitement son `temporal_resolution_index`, de sorte que son identité `(préfixe, token)` reste fraîchement reconstructible. Il n'est jamais remplacé par un handle de racine synthétique, par un représentant de DSU de scratch ou par le seul indice de son composant. Cette provenance est indispensable au rejeu du pont latent.

Les composants sont ordonnés canoniquement par `(source_batch_index, representative_gateway_candidate_index)`. Dans chaque composant, racines et indices de résolutions latentes sont strictement croissants. Chaque candidat est lié exactement une fois, chaque tranche de suppressions coïncide avec celle de 10.9, chaque sommet publié est référencé et chaque composant est non vide. Le candidat représentatif est une provenance canonique; ce n'est ni un sommet typé, ni un handle de racine.

Le builder borne toutes les populations, les comparaisons et les octets avant les allocations correspondantes. Les remontées de parents et les compressions de chemins de la DSU possèdent en plus un plafond dynamique explicite; l'atteindre produit le même échec atomique borné. Une forme incohérente, une projection manquante, une résolution hors lot, une disposition sans payload conforme ou un budget insuffisant ne publie aucune arène scientifique partielle.

## Coût et structures évitées

Chaque projection produit une seule incidence typée transitoire et chaque candidat une seule hyperarête. Si $G$ est le nombre de composants publiés, $R$ le nombre total de racines distinctes dans leurs tranches et $L$ le nombre total de résolutions latentes distinctes dans leurs tranches, alors :

$$G\leq C,\qquad R\leq P,\qquad L\leq P.$$

Les cinq arènes utilisent donc $O(B+C+P)$ entrées pour $B$ lots. Comme chaque candidat possède au moins une projection, $C\leq P$; les trois heapsorts déterministes bornés et la DSU donnent $O(P\log(P)+(P+C)\alpha(C))$ opérations, auxquelles s'ajoute le balayage des $B$ lots. Le scratch reste $O(B+C+P)$ et aucune table dense lots-par-facettes n'est nécessaire. Le nombre concret de comparaisons et celui des remontées de parents DSU sont plafonnés séparément. Une voie 10 M+ pourra remplacer les tris comparatifs d'identifiants bornés par un radix sort déterministe sans changer le contrat des cinq arènes.

10.14 ne construit ni Gamma exhaustif, ni catalogue global de facettes ou de cofaces, ni cellule top-$m$, ni mosaïque de Delaunay d'ordre supérieur. Il ne matérialise pas de clé persistante de coface à onze points : la coface reste la paire factorisée 10.7 `(facet_token, added_point_id)` et ses suppressions sont déjà référencées par 10.9. Il ne copie aucun historique de parents du locator, ne crée aucun snapshot ou DSU par préfixe et ne conserve aucune structure globale proportionnelle à l'univers combinatoire $\binom{n}{K}$.

## Actions interdites et prémisses résiduelles

Le résultat ne contient et n'autorise aucun `RootBirth`, `AtomicUnionBatch`, `GatewayAttach`, `CoverageDelta` ou mutation de forêt. Il n'appelle pas `apply_batch`, ne réécrit aucun parent de composante et ne publie pas les représentants de sa DSU de scratch. Une intégration ultérieure devra vérifier la complétude du sous-univers de gateways, reconstruire une transaction globale atomique et attribuer séparément les identifiants persistants.

Le fresh verifier doit rejouer les autorités amont plutôt que faire confiance à des booléens auto-déclarés. Il reconstruit les projections temporelles, le partitionnement par lot, les sommets typés, les hyperarêtes, la clôture et les projections de racines, puis compare la sortie canonique observée. Il hérite du rejeu AUTH/CLOCK en mémoire et peut donc certifier `external_clock_authority_replayed=true`. Son succès reste toutefois conditionnel à l'autorité de liaison externe, au gel synchronisé du locator, à la discipline scientifique strictement pré-lot et à la validité en mémoire du journal AUTH. `external_binding_authority_replayed=false`, `external_freeze_synchronization_replayed=false`, `in_memory_replay_only=true` et `crash_durable=false` demeurent explicites.

La complétude globale des gateways silencieux non directs reste ouverte. Par conséquent, 10.14 reste une proposition de quotient local certifiée relativement aux candidats fournis; il ne ferme ni la Phase 10, ni M.1, ni la qualification du SLO 50 k sous la seconde, ni la voie 10 M+, et ne promeut aucun statut public.
