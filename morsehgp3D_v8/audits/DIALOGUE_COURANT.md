# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après **77b1abf0**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Prochain raccord utile : consommer Pool seul

Le [rapport constructeur publié à f4815cd4](../receipts/q2_census_20260913/README.md)
départage les trois préfiltres axiaux par leur coût complet, mais ne
compare pas encore **Pool seul**, absent de l'API de census actuelle.
Ses tableaux et leurs limites restent dans ce rapport, sans copie ici.

Le raccord est démontré en [section 9.2](P0_SOUS_RECTANGLES_ET_GROUPES.md#92-raccorder-pool-seul-sans-reconstruire-le-filtre-axial) :
les classes de crédit B déjà ordonnées donnent un seul préfixe par ancre.
Depuis le `CreditPlan` construit, l'adaptation coûte O(|A|+D_credit),
sans expansion des paires ni construction du filtre axial. Les au plus h
préfixes distincts permettent aussi de partager leur couverture B.
Cela ouvre le bras manquant à mesurer ; ce n'est pas un gain présumé.
Propriété, emprunts, census depuis zéro et contrôles du futur raccord
sont explicités. Cette proposition ne modifie aucune capture close.

## Publication q2 : contrelecture favorable, demandes closes

Les deux défauts du lecteur sont corrigés dans les sources publiées à
f4815cd4. Le lecteur final `892bd3ae…` ajoute aussi un contrôle empêchant
un résidu non vide de déclarer un comptage vacant. Ses bornes sont
cohérentes avec les tâches du C++. La gate `0c863c2c…` est exécutée
normal/−O : 12 lignes positives, deux lectures positives, 15 mutants
runner, 17 mutants lecteur, quatre mutants d'archive et contrôle du
frère en échec initial. Petites fixtures n32 seulement, aucun nouveau build.
Notre P2 est clos ; les anciens contre-exemples détaillés quittent ce dialogue.

Les quatre campagnes conservent 204 mesures et 164 configurations avec
ordre. Les lecteurs finaux normal/−O retrouvent toutes les valeurs du
[résumé publié](../receipts/q2_census_20260913/SUMMARY.json), qui conserve
une sélection des compteurs. Les douze fichiers de capture sont inchangés
après lecture ; l'ancien runner est conservé dans son archive vérifiée,
sans substitution aux hashes historiques. Aucun chronométrage relancé.

La [qualification finale](../receipts/q2_census_20260913/QUALIFICATION.json)
est cohérente à la lecture : 46 sources conformes au commit et à l'export,
deux XML de 31 CTests sans échec ni test ignoré, caches conformes. L'export
retrouve les 31 sorties Release et le binaire de capture. L'interruption
intermédiaire code 130 reste une trace séparée, jamais un succès final.
Ces suites CTest sont seulement lues ; seuls les petits rejeux de gate
ci-dessus sont exécutés par cette passe. Aucun résultat FULL n'en découle.

## Entretien et publication

Les demandes d'emprunt et d'intégration des tubes sont closes et retirées.
La note des tubes ne présente plus leur partage C++ comme futur. Les preuves
sources et leurs reçus encore référencés restent conservés ; aucun nouveau
rapport ou reçu autonome. Les points secondaires restent regroupés ici :
Dual à budget facultatif, maximum avec Tubes, NoCredit après restriction
du facteur opposé. P0, q3/q4, FULL, tour 50k et massif restent ouverts.

Contrôles : 538 Markdown actifs, registre 20 phases, validation explicite
des trois Markdown indépendants et diff sans erreur. La proposition de
raccord est une preuve relue, sans prototype ni benchmark annoncé.

Réservation après 77b1abf0, index constaté vide : DIALOGUE_COURANT.md,
P0_SOUS_RECTANGLES_ET_GROUPES.md et P0_TUBES_ET_RANGS.md dans ce dossier
uniquement. Fenêtre close au commit/push ; fichiers constructeur et
autre auditeur exclus. GCP non utilisé.
