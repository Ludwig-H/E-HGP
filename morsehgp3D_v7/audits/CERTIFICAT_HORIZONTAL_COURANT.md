# Certificat horizontal réduit — domaine CPU E

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans `audits/`.

**La composition horizontale réduite est fermée pour la route CPU E dans le domaine ci-dessous.** Les lemmes géométriques, les bornes de primitives, leur qualification compilée et le lecteur des deltas sont maintenant raccordés. Aucun resolver top-K, catalogue Gamma ou hypothèse de position générale globale supplémentaire n'est requis. Ce certificat est une preuve composée sous des préconditions explicites, accompagnée de falsifications exécutées ; il ne transforme pas leur nombre en preuve universelle du compilateur.

Autorité produit : E, publié dans `2b94abddfde08101607f4639d42149156fb39e6c`, inchangé par `61f72a6805e27f1bc216b5d7444164b31fc970b6`. La préparation concurrente F n'est pas assimilée à ces octets. Le [manifeste courant](validation_current.json) et la [qualification](AUDIT_QUALIFICATION_20260905.md) identifient les sources, binaires et reçus auxquels chaque conclusion appartient.

## 1. Objet reconstructible et énoncé

Pour K supérieur à un, la référence est Gamma élémentaire restreint à ses composantes **non triviales** : ses sommets retenus sont les facettes incidentes à au moins une coface active. Une facette est un ensemble de K PointId ; une coface en contient K+1. Le niveau est le rayon carré de sa miniball. Les coupes ouvertes et fermées sont distinguées exactement.

Cette réduction vient du [théorème transverse 1](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#1-référence-exacte-par-gamma). La proposition 5 du manuscrit justifie les composantes du graphe élémentaire, sans identifier ses adjacences au graphe complet. La suppression Gabriel brute réfutée par E5 n'est pas une prémisse. Les parties I et II du [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), pages PDF 35–134, ont été lues intégralement ; leur hash reste `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.

Soient R les composantes reconstruites depuis les seuls deltas du sous-flot retenu et C celles de Gamma réduit. L'inclusion des facettes définit, à chaque coupe t, une bijection préservant les points couverts :

$$\Phi_{K,t}:R_K(t)\longrightarrow C_K(t),\qquad U\bigl(\Phi_{K,t}(A)\bigr)=U(A).$$

Elle commute avec les inclusions entre coupes ordonnées, y compris ouverte puis fermée au même niveau :

$$\Phi_{K,t'}\bigl(i_{t,t'}(A)\bigr)=j_{t,t'}\bigl(\Phi_{K,t}(A)\bigr).$$

Les naissances réduites, niveaux, parents abstraits, nombres de parents et multifusions sont donc conservés. Une continuation à un parent qui gagne des points demeure un changement à conserver. Des deltas supplémentaires peuvent matérialiser des facettes sans changer les points ni la composante abstraite.

À K=1, les points d'entrée sont les racines normatives initiales. La présence du graphe Gabriel et d'un EMST donne les composantes du single-linkage. Le drapeau `normalized_reduced` enregistre la demande de route même à K=1 ; le comportement du fold conserve explicitement cette exception des singletons.

La bijection est construite par les **facettes**, jamais par égalité des ensembles de points : pour K supérieur à un, ces ensembles peuvent se recouvrir. Elle n'impose ni un catalogue exhaustif de facettes, ni les identifiants de Gamma, ni un `batch_id` public canonique entre certificats clairsemés différents.

## 2. Domaine d'appel effectivement couvert

| Condition | Portée et autorité |
| --- | --- |
| Entrée | De 2 à `2^30−1` enregistrements, PointId distincts, positions distinctes dans `[0,65535]^3` ; n désigne ce cardinal |
| Fenêtre | `s≥8`, `2≤smax≤11`, `rmax=min(smax,n)`, `K_eff=rmax−1` ; pas de K0 ni d'événement q1 |
| Chemin | `run_pipeline`, CPU de référence, `complete_silent_incidence=true`, classic ou CSR ; aucune substitution arbitraire par `prefilter_census_override`, aucun mutant ou backend GPU |
| Arithmétique et exécution | [Domaine CPU](DOMAINE_CPU_COURANT.md), largeurs et préconditions des helpers qualifiés ; environnement numérique stable pendant chaque opération, pas de modification concurrente des entrées |
| Régularité | Refus des extra-shells dans la fenêtre reçue ; support essentiel et absence d'extra-shell pour les miniballs de cœur/chaîne effectivement contrôlées |
| Autorité du résultat | Retour terminal `complete_regular`, sans exception ni abandon nécessaire ; tous les callbacks antérieurs restent provisoires jusque-là |

Une collinéarité globale n'est pas un motif de refus : trois points collinéaires distincts sont acceptés par la garde exécutée. En revanche, une boule avec extra-shell pertinente est refusée sur cette route. Une irrégularité hors fenêtre, non rencontrée par la descente locale, reste compatible avec le théorème d'inertie. Une MEB rencontrée irrégulière n'est jamais autorisée par ce seul argument.

Les plafonds portent sur le travail demandé. Mettre les cinq plafonds silencieux à zéro accepte le cas n=2/K1, car aucune complétion silencieuse n'y est appelée ; sur un ordre qui en demande, le refus est prospectif. Le succès ne promet pas que toutes les entrées géométriquement admissibles termineront dans un budget donné.

Les [18 cas CLI de domaine](receipts_horizontal_20260905/domain/results.json) comptent six succès et douze refus attendus : cardinal minimal, fenêtre effective, IDs/positions, u16, carré avec extra-shell et `--require-exact`. Ils utilisent le binaire E scellé. Les bornes structurelles immenses sont prouvées statiquement, pas testées par une allocation correspondante.

## 3. Décharge des obligations de composition

| Obligation | Preuve et raccord désormais disponibles |
| --- | --- |
| Toute boule positive à support minimal q et `p+q≤rmax` atteint le RLE | [S1](S1_COURANT.md), partition [index/front/cover](AUDIT_RACCORD_INDEX_FRONT_20260905.md), owner et seed canoniques ; les minorants stricts ne tuent aucune boule pertinente |
| Les décisions et identités ont leur domaine représentable | [Lanes](ARITHMETIQUE_LANES_COURANTE.md), [entiers larges](ARITHMETIQUE_LARGE_COURANTE.md), [MEB](AUDIT_MEB_DIFFEREE_20260905.md), [q2 E](ADDENDUM_MEB_Q2_E_20260905.md), [front compilé](receipts_front_compiled_20260905/README.md) |
| Census fermé, refus de shell et catalogue direct | [AxisBounds/census](CENSUS_AXIS_COURANT.md), [fenêtre de rang](RETOUR_MATH_COURANT.md) ; la coface directe régulière est exactement `I(B) ∪ U(B)`, à l'ordre `K=p+q−1` |
| Une première incidence choisie suffit | [Confluence et ancrage](REPONSE_AUDITEUR_COMPOSITION.md) ; le choix conserve son suffixe strict jusqu'à une coface directe ou un suffixe déjà certifié |
| Les omissions ne fusionnent pas secrètement deux groupes retenus | Apex strict, contact du cœur, unicité de la MEB aux contacts égaux et induction aux contacts stricts, y compris hors cœur ; [composition §§3–5](../docs/PREUVE_HORIZONTALE_COMPOSITION.md) |
| Le résultat public représente le sous-flot retenu | Fold par niveaux rationnels égaux, parents pré-lot, première matérialisation et lecteur indépendant des tokens ; sondes du §4 et portes E existantes |
| Aucun préfixe échoué n'est certifié | Invalidation terminale, refus de ressources et exceptions ; [interfaces/archive](AUDIT_INTERFACES_20260904.md), portes `mhgp7_mono_inline` et archive de la [qualification E](AUDIT_QUALIFICATION_20260905.md) |

Le passage délicat entre les troisième et cinquième lignes est explicite. Toute coface directe Q vérifie `p(B_Q)+q(B_Q)≤|Q|≤rmax`. Après le refus des shells pertinents, le catalogue direct est donc complet. Une facette de son cœur retirant un essentiel est contrôlée localement ; retirer un intérieur conserve la même boule régulière.

Pour un bloc omis de haut rang, le saturé vérifie `|S_B|≥p+q≥K+2`. Son graphe strict connexe couvrant les points possède plusieurs facettes : chaque facette stricte y a une coface incidente antérieure. Une première incidence égale du cœur dans un bloc irrégulier contredirait l'unicité de la MEB et la régularité déjà contrôlée de cette facette.

Chaque maillon retenu possède son suffixe strict vers un direct actif. Un contact égal avec un bloc irrégulier lui imposerait une même miniball régulière et irrégulière ; un contact égal régulier conflue. Un contact strict hors cœur passe par les apex antérieurs déjà identifiés par induction. Il n'est donc pas nécessaire de matérialiser artificiellement sa facette de contact dans le préfixe.

Chaque composante Gamma possède à son tour un ancrage direct : aucune pièce omise, déjà rattachée à un apex strict non trivial, ne peut constituer seule une première naissance. L'inclusion est alors injective et surjective sur les composantes. Les pièces omises n'ajoutent pas de point ; les directes ajoutent les mêmes points aux mêmes groupes. La naturalité horizontale découle de cette même inclusion, sans construire d'applications entre ordres.

Le contrat des rôles relie cet argument au fold. Sur une boule régulière, retirer un essentiel diminue strictement le rayon de la miniball ; retirer un intérieur le conserve. Une facette déjà incidente avant le lot est donc active quand elle est touchée à ce lot. Les parents figés avant les unions sont exactement les anciennes composantes incidentes ; une active encore latente n'ajoute aucun parent. Toute nouvelle racine DSU vient soit d'une composante déjà vue, soit d'un singleton touché marqué à la fin du lot. Cet invariant suffit pour conserver `seen` sans supposer un OR caché dans `unite`. À K1 les événements conformes ont `q=2,d=0,active_mask=3`.

## 4. Falsifications indépendantes du vrai payload

Le [pont du pipeline](horizontal_pipeline_bridge.cpp) transporte les deltas effectivement livrés. Le [juge rationnel](horizontal_rational_oracle.py) construit les miniballs par supports positifs et élimination de Gram en `Fraction`, puis Gamma sur tous les sous-ensembles du petit nuage. Il reconstruit le résultat produit depuis `parents`, `born`, `output` et les niveaux ; aucune coface candidate, union ou décision géométrique du moteur ne sert d'oracle.

Quatre nuages sont examinés : E5, sa version translatée et dilatée, sept points de la courbe des moments et une paire aux extrêmes u16. Les identifiants sont clairsemés jusqu'à `4294967294`. Chaque nuage est exécuté avec deux layouts, un/deux threads demandés, ordre physique direct/inversé et séparations 8/12. Cela ne prouve pas que chaque très petite exécution a créé deux travailleurs effectifs.

Par build O2 et O1 UBSan strict : **16 appels, 60 ordres, 840 coupes ouvertes/fermées, 200 deltas, 1 124 carrés de naturalité, 44 naissances, 52 multifusions et 64 croissances en points**. Huit cofaces silencieuses sont ajoutées. Le juge compte aussi 820 facettes Gamma absentes des représentations retenues, cumulées sur les coupes : cette dent garantit que le test ne suppose pas un catalogue exhaustif. Les digests sont identiques entre les quatre configurations appariées de chaque nuage.

Le vrai mutant `silent-drop-coface` rend encore un succès moteur, mais perd une activation du cœur : le juge le refuse pour `horizontal.core_activation`. Le négatif de lecture de la route compatible est distinct : il ignore explicitement son type, tente de consommer ses tokens comme des deltas normalisés et refuse un parent absent du pré-lot. Le pont rend 0 ; ces divergences attendues sont jugées séparément, sans leur attribuer un code 4 CTest.

Les [reçus du pipeline](receipts_horizontal_20260905/pipeline/summary.json) conservent les entrées, sorties et sources E figées. Le premier essai live non attribuable pendant la préparation E/F, avec une hypothèse erronée du juge sur le drapeau K1, reste conservé comme [essai invalide](receipts_horizontal_20260905/pipeline/initial_attempt.invalid.json.gz). Aucun défaut produit n'en est inféré. Les exécutions retenues utilisent le snapshot E isolé.

Les quatre nuages sont globalement réguliers et parcourus jusqu'à `K=n−1`. Ils ne testent pas les blocs irréguliers hors fenêtre ; la [fixture de frontière onze/douze](RETOUR_MATH_COURANT.md) et le lemme statique gardent cette attribution. Les numéros publics des batches ne sont pas l'autorité du juge : il regroupe les niveaux rationnels égaux.

La [sonde séparée du fold](horizontal_fold_probe.cpp) complète ce raccord sur des hypergraphes synthétiques, indépendamment de la géométrie et de Gamma. Par build O2/O1 UBSan : 40 flux, 272 coupes, 128 deltas, 32 continuations omises, 32 matérialisations sans nouveau point couvert et sept vrais mutants code 4. Elle vise les niveaux représentés différemment mais égaux, les parents figés avant le lot, les premières matérialisations et les deux layouts. Les flux synthétiques respectent le contrat des rôles ; aucune réalisation euclidienne n'est revendiquée. Le fold seul connaît les points de son catalogue d'événements ; le lecteur du pipeline initialise bien tous les points d'entrée comme racines K1. Ses reçus et mutants sont liés depuis l'[index des preuves](receipts_horizontal_20260905/README.md).

## 5. Portée pour la construction industrielle

**La qualification des primitives et l'assemblage horizontal réduit ne restent plus des demandes ouvertes sur E.** Les conclusions antérieures les laissant à compléter sont remplacées par ce certificat et ses liens. Une modification réelle de ces sources demandera sa conservation ou sa requalification propre.

Si deux fenêtres terminent dans ce domaine, les composantes abstraites de chaque ordre commun représentent le même Gamma réduit. Cette conséquence ne transforme pas les anciennes portes de préfixe de la route compatible en preuves des octets du payload complété. Le cas CLI du triangle aigu vérifie séparément son seul préfixe K1 entre `smax=2` et `smax=11`.

Le payload reste `normalized_horizontal_h0_candidate`, `born` une première matérialisation du sous-flot et l'archive `vertical_maps=none`. Un sérialiseur appelé sur des `ForestResult` fabriqués arbitrairement par l'utilisateur ne vérifie pas ce théorème ; l'autorité ici est la route CPU qualifiée et terminée.

Restent des livrables distincts : domaine des plateaux à étendre, applications verticales et leurs carrés de naturalité, masses d'incidence et vote du §9.1, contrat des identités publiques du quotient, puis coûts de chaîne, de stockage et de bout en bout. Les facettes peuvent rester les feuilles d'un arbre ; le recouvrement apparaît dans la projection vers les points. Aucun oracle exhaustif du présent audit n'est proposé comme architecture produit.

Le refus `--require-exact` et le statut public `not_claimed` restent en place. GCP non utilisé.
