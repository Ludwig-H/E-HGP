# Contrelecture du juge des reçus FULL mono

5 septembre 2026, contexte communiqué `98bb6578`. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le juge est adapté à la cohérence déclarative de cette campagne. Ses conclusions sont correctement bornées dans les reçus et le document constructeur : un code de jugement 0 n'est pas un succès du moteur, et les sentinelles terminales ne prouvent pas l'égalité des forêts. Quatre relations nécessaires entre les valeurs déjà imprimées permettraient de renforcer cette porte. Les valeurs nominales satisfont ces relations ; les contre-fixtures ci-dessous établissent une lacune du juge capturé, aucun défaut des cinq calculs observés.

La [copie exacte du juge relu](judge_runs/judge_at_review.py) porte le SHA-256 `24e789459ee7adb8b48819dddc8bef8832b2b152ad9418c1a1d281038315e2c7`. Les sources et les entrées sont épinglées telles que lues dans [le reçu normal](judge_runs/normal.json) et [le reçu optimisé](judge_runs/optimized.json). Aucun moteur C++, build, benchmark, Git ou GCP n'a été exécuté par ce rejeu.

## Résultats du rejeu borné

[Le runner indépendant](judge_runs/replay.py) exécute cinq jugements réels et un selftest par mode Python. Normal et `-O` donnent chacun :

- cinq reçus cohérents, trois tentatives réussies à 8k et deux refus `full_gabriel_alias_budget` à 16k/K9 et 32k/K7 ;
- 46 lignes d'ordre, dont 44 réussies et deux refusées ; les 14 ordres réussis des tentatives finalement refusées restent diagnostiques ;
- les neuf mutations de données du selftest rejetées au motif attendu, un positif réel, un refus synthétique avec ligne d'ordre et un refus synthétique de lecture sans ligne d'ordre suivante ;
- quatre corruptions supplémentaires acceptées par le cœur du juge capturé, mais refusées par nos relations nécessaires ;
- aucune violation de ces relations sur les 44 lignes réussies et les cinq terminaux réels.

Les neuf selftests exercent terminal manquant, promotion du statut, dépassement de plafond, ordre K sauté, durée NaN, capture désaccordée, Kmax du protocole modifié, inventaire des workers incomplet et dernier compteur désaccordé. Ce sont des mutations de données d'audit, pas des mutants du moteur. Les portes utilisent des exceptions explicites et restent effectives sous `-O`.

## Autorité réellement vérifiée

Le juge lie brut JSONL, reçu, intention, paramètres et commande exacte ; il rejette clés JSON dupliquées, valeurs non finies ou négatives, absence de terminal, mauvais codes de sortie et ordres non consécutifs. Il distingue correctement code moteur 0, refus typés de code 2 et invariant de code 3. Il contrôle l'inventaire des douze étapes et des six compteurs de workers, les plafonds fixés, la transition entre catalogues, la partition des catalogues en succès, le grand-livre par lane lorsqu'il est clos et la concordance des derniers compteurs publiés.

Une ligne réussie doit annoncer une racine terminale, une couverture de n points, des minima et des nœuds non vides et `parent_refs = nodes − 1`. Une ligne refusée doit avoir ses cinq comptes de payload à zéro. Le terminal invalide la publication d'une tentative échouée ; le cas de lecture refusée avant impression de la prochaine ligne est conservé comme diagnostic sans fabriquer cette ligne. Le selftest l'exerce précisément.

Le brut ne contient ni arène de parents, ni identités des minima, ni coupes intermédiaires, ni digest. Le juge ne peut donc contrôler les identifiants des parents, leur activation antérieure, les offsets CSR, les composantes aux coupes, ni l'égalité des forêts entre valeurs de s. Même les identités renforcées ci-dessous sont seulement nécessaires. La sonde C++ effectue séparément la lecture de la racine finale et la comparaison des PointId de sa couverture à l'entrée ; le juge ne reçoit que son cardinal déclaré.

La fonction principale compare les déclarations de sources au protocole et calcule le hash du brut consommé. Elle ne recalcule pas les hashes des sources ou du binaire déclarés. La qualification du paquet et ses scellements demeurent une autorité distincte. Nos corruptions modifient ensemble le brut et son miroir dans le reçu ; elles violeraient les hashes du paquet historique et ne constituent pas un contournement démontré de cette qualification complète.

## Quatre renforcements concrets

Le point de départ est le reçu 8k/s8 inchangé. Les mutations, indices de lignes, anciennes valeurs, remplacements, hashes reconstruits et motifs indépendants sont conservés dans les deux reçus de rejeu.

| Corruption coordonnée brut/reçu | Résultat du juge capturé | Relation nécessaire qui la refuse |
| --- | --- | --- |
| K1 : minima 8 000 devient 1, avec 15 999 nœuds | Valide, tentative réussie | En FULL K1, minima = n ; ici la borne d'arité est également violée. |
| K2 : appels MEB et miroir géométrique deviennent tous deux 0 | Valide, tentative réussie | En succès, appels MEB = requêtes de portail + étapes de descente ; ici 3 297 + 4 325 = 7 622. |
| K1 : aliases 8 000 devient 8 001, sous le plafond | Valide, tentative réussie | Identité des installations EAGER explicitée ci-dessous. |
| Terminal : durée avant terminal devient 0 ms | Valide, tentative réussie | Une étape chronométrée ne peut dépasser tout l'intervalle qui la contient ; le diagnostic soustrait est aussi incohérent. |

Pour un ordre réussi, le nombre L de minima vaut n pour K1 et le nombre de lignes du catalogue de minima pour K supérieur ou égal à 2. Pour une forêt FULL à une racine, avec N nœuds et I nœuds internes, `N = L + I` et `parent_refs = N − 1 ≥ 2I`, donc `N ≤ 2L − 1`. Cette borne provient du journal qui conserve uniquement les fusions d'au moins deux parents. Elle n'est pas une preuve de validité de l'arène à partir de ses seuls comptes.

Pour le producteur EAGER épinglé `e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170`, poser D le nombre de directes du catalogue de connexions, T le nombre de visites de facettes et V le nombre de nouvelles requêtes de portail. Alors `E = 2(K+1)D − T` compte les facettes de même niveau nouvellement installées, et `aliases = L + E + V`. Notre contrôle exige aussi E non négatif. L'identité s'applique après un ordre entièrement réussi, où chaque installation et chaque connexion a été traitée ; elle ne s'applique pas au préfixe interrompu d'un refus et devra être requalifiée si le calendrier d'alias devient paresseux.

La soustraction imprimée vaut `max(0, elapsed_before_terminal_ms − provisional_output_ms)`. Le contrôle indépendant admet 0,000003 ms pour l'arrondi indépendant à six décimales des trois valeurs. Cette tolérance ne sert ni à une décision géométrique ni à comparer des performances. Le rejeu vérifie aussi chaque durée d'étape au plus égale à la durée totale ; il n'impose aucune égalité artificielle entre des chronomètres de périmètres différents.

## Unités, facteurs et portée des mesures

Les champs de temps en `_ms` proviennent de `steady_clock` et de `duration<double, std::milli>` ; le tableau constructeur les convertit en secondes. Les champs `rss_mib_sample` et `hwm_mib_sample` utilisent une division par 2 puissance 20 des octets, respectivement par 1 024 des kB de `/proc`. Le lecteur de RSS suppose des pages de 4 096 octets ; cette hypothèse appartient au helper épinglé et ne se transporte pas automatiquement à un autre hôte. Les valeurs GNU time en `kbytes` sont présentées en KiB dans le document. Ce sont des mesures distinctes : échantillon RSS, maximum historique et pic du processus entier.

La porte candidate contrôle exactement `8 GiB // (2 × sizeof(BallCandidate))`. Le facteur 2 couvre le tampon de fusion ; le facteur historique d'événements 4 provient de `fold_inflight + 2`, avec `fold_inflight = 2`. Le nouveau garde de catalogues borne séparément census + catalogue précédent + deux copies de l'expansion. Ces facteurs et les plafonds de nombres de nœuds/parents/alias ne constituent pas un budget RSS complet, notamment pour les conteneurs associatifs et leurs capacités. Le juge contrôle des valeurs déclarées et leurs bornes, sans mesurer lui-même ces allocations.

Le temps principal inclut les sorties provisoires et les libérations, et exclut la configuration initiale et l'écriture du terminal. GNU time couvre séparément le processus entier. Le juge vérifie les noms des périmètres et la syntaxe des mesures ; il ne prouve pas la réalité d'un chronomètre à partir du reçu. Il ne convertit pas les horodatages de collecte en durée de calcul. Les documents consultés conservent correctement les deux refus, n'attribuent pas une réussite au préfixe, et ne déduisent ni accélération appariée face à F, ni classement statistique de s, ni contrat 50k ou de tour inter-K.

La suite constructive consiste à intégrer les relations nécessaires dans une révision explicite du juge et de ses selftests, puis à rejouer les bruts déjà conservés. Aucun nouveau run moteur n'est nécessaire pour fermer ces quatre lacunes de cohérence.
