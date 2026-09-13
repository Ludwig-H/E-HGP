# Dialogue courant de l’auditeur indépendant v8

13 septembre 2026, après **256957a5**, sur main. Écritures limitées à ce
dossier. `phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## P2 — vérifier la préparation de l’index B dans les reçus

Sur `bench/run_q2_census_matrix.py` SHA256
`311fce7f66e3b0d6e9a0a8d60ad07ad22382e21255416d5f49b27df7219e00c4`,
le lecteur admet une comptabilité impossible du parcours partagé.
Une vraie ligne de `mhgp8_q2_census_probe 32 sheet_full 5 8 additive pairwise-first`
contient 256 candidates, n_b=16, 31 nœuds de préparation B et 16 lectures
de points B. Mettre en mémoire `query_build_nodes=0` et
`query_build_point_visits=0` dans l’arm `shared` laisse `validate_result`
accepter cette ligne. Les autres champs sont inchangés. Le contrôle
indépendant accepte aussi chacune des deux suppressions isolées, en
normal et sous `python3 -O` ; la ligne originale est admise dans les deux
modes. Binaire emprunté `build/v8_census_20260913/mhgp8_q2_census_probe`,
SHA256 `243387ac6df40e3f0f0c38a1d7d7a86735ed260ba11d904bf032bc4eb1c6e432`.

Ajouter, pour `shared` avec candidates non nulles, les égalités
`query_build_nodes == 2*n_b - 1` et `query_build_point_visits == n_b`.
La gate C++ les impose déjà : le moteur paie ce travail, mais le lecteur
ne rejette pas sa disparition des compteurs. Porter cette mutation dans
la gate de reçus, avec positif et refus normal/−O. Aucun défaut moteur,
aucune mesure réelle falsifiée ni gain de temps erroné n’est allégué.
Le cas vide possède déjà sa règle distincte de compteurs nuls.

## Contrelecture favorable du census et clôtures

Le cpp q2 reste identique au pin `3c513cc4…` déjà relu. La clarification
d’emprunt du callback est maintenant dans le header `2116ac4b…` et le
contrat principal : durée de vie et absence d’invalidation pendant tout
l’appel, propagation d’exception, émissions partielles non annulées et
reprise depuis zéro. Ce point est clos ; le détail demandé est retiré
de ce dialogue.

Les 277 cas de la gate géométrique confrontent **tout A×B** au census
multiprécision, puis comparent les supports, clés et ensembles d’IDs
émis dans les deux modes. La fixture d’héritage exerce effectivement une
division après crédit. Les 192 variantes d’une même fixture demi-entière
et les dix contre-modèles sont correctement identifiés ; ce ne sont ni
277 familles indépendantes ni dix mutations physiques du moteur.
Aucun défaut géométrique ou de transmission relevé dans cette lecture.
Les comptes au-delà du seuil et les performances restent hors de cette
preuve de sorties admises. Aucun nouveau build ou benchmark lourd d’audit.

## Suite et entretien

Les campagnes de coût complet et leur publication sont encore en cours.
Le curseur est porté et son état est compact ; cela ne suffit pas à
choisir le parcours partagé sans ses temps de comptage, de collecte et
de sortie. Le [juge indépendant](P0_Q2_CENSUS_BOUNDS_CHECKS.json) reste
une preuve distincte des tests du C++ en chantier.

Les demandes désormais documentées par le constructeur sont retirées.
Les points secondaires restent regroupés ici : Dual à budget facultatif,
maximum avec Tubes, NoCredit après restriction du facteur opposé. P0,
q3/q4, FULL, tour 50k et massif restent ouverts. Aucun nouveau rapport
ou modèle autonome ; seul ce dialogue est mis à jour.

Contrôles : 538 Markdown actifs, registre 20 phases et diff sans erreur ;
ce dialogue reçoit également sa validation explicite, car il est hors
du corpus canonique. Aucun résultat de performance déduit des petits
rejeux de reçu.

Réservation après 256957a5, index constaté vide : uniquement
`morsehgp3D_v8/audits/DIALOGUE_COURANT.md`. Fenêtre close au commit/push ;
fichiers constructeur et autre auditeur exclus. GCP non utilisé.
