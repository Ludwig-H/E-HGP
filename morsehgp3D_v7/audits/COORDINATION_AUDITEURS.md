# Coordination entre auditeurs

11 septembre 2026, reprise sur **069bb6a2**. Écritures dans le seul
`morsehgp3D_v7/audits/`, sur `main`. Réservation de publication ci-dessous.

## Réducteur mono : contraction φ pendant la consommation

Le paquet constructeur `streaming_graph_20260911` est contre-lu : lecteurs
normal/−O PASS, suppression effective des tableaux targets[R], requests[R_K]
et du graphe complet. Le résultat négatif mono n8000 et le coût des
recompactions sont correctement rapportés. Cette première demande de flux
réel est close ; ne pas répéter ce benchmark avant correction du réducteur.

**La preuve va plus loin que le brouillon OrderedReducer : on peut contracter
les pivots avant de les soumettre au DSU, pendant la même passe.** Initialiser
φ des naissances avec leurs identités natives stables. À `local0(B)`, la
terminale T est strictement plus ancienne, donc φ(T) est déjà disponible.
Affecter φ(B)=φ(T), sans `find` DSU, et ne pas émettre le pivot. Pour les
autres occurrences, soumettre directement φ(B)–φ(Tj) à Kruskal sur les L
naissances. Consommer dans l’ordre source actuel, même si la géométrie
s’exécute autrement ; une fenêtre peut couper un hub sans perdre son φ.

Par induction, le pivot joint un hub encore isolé ; toute autre union est
acceptée exactement quand l’union projetée l’est. Le certificat garde donc
exactement les arêtes non pivots de Kruskal ordonné sur hubs, avec mêmes
ordinaux et dates. Il n’y a plus besoin de DSU sur A, de certificat A−C,
de passe finale de calcul φ, ni de projection/compaction finale. Restent
φ[A], atlas/masques, semis, marques, fenêtres, DSU[L] et sortie. Les comptes
sont A−L pivots, R−A+L tests d’union et L−C arêtes retenues. Les naissances
isolées/futures et les points K1 restent des sommets explicites.

Une table φ portant des indices denses de naissances permet aussi de joindre
les extrémités en O(1) après accès au rôle atlas ; convertir en BlockId natifs
pour l’export. Ne jamais y stocker le représentant DSU : l’identité de
naissance sert encore aux contributions, ancres et verticales. La [gate indépendante abstraite](receipts_birth_stream_20260911/README.md)
est close normal/−O : 14 cas, 42 essais, 3 276 comparaisons BFS et neuf
rejets causaux. Le mutant `find(φ)` conserve les composantes mais perd les
identités natives, ce que la gate distingue explicitement. Aucune intégration
C++ ni vitesse n’est encore qualifiée ici pour cette spécialisation. Le brouillon ordonné constructeur
conserve sa portée propre. Les lots parallèles hors ordre gardent leur
contrat de composition distinct.

Le constructeur a lu favorablement la preuve et prépare un draft distinct.
Son raccord ordonné sur hubs sert de différentiel ; ses nouvelles captures
annoncées et sa mesure n8000 en cours restent sous sa responsabilité. Les
compteurs cités par notre paquet sont ceux, scellés, de **069bb6a2**.

## Acquis conservés et répartition

Le [raccord FULL](../receipts/atlas_graph_full_20260911/README.md) est clos sur
son corpus borné. La [compatibilité physique historique](receipts_historical_export_20260911/README.md),
publication **b823c369**, fournit une option qualifiée O2/SAN : dix entrées,
soixante ordres. Le constructeur a contre-vérifié ses lecteurs. Les anciennes
demandes de banque/indices ne restent pas ouvertes ici ; consulter cette
preuve pour les minima silencieux et représentants rationnels bruts.

Les preuves de [composition](receipts_composable_msf_20260911/README.md),
d’[objets parallèles](receipts_parallel_objects_20260911/README.md) et de
[canonisation du support](receipts_certified_support_20260911/README.md)
restent acquises à leur portée. Les fichiers `NOTE_CLAUDE_*` et leurs preuves
restent sous la responsabilité du second auditeur. Aucun changement de ses
fichiers, du constructeur, de v6 ou du registre par cette passe.
GCP non utilisé.

## Publication de cette passe

Réservation auditeur : **13 chemins**, index constaté vide sur **7ccd9d6e** :
les sept fichiers de `receipts_birth_stream_20260911/`, cette coordination,
`DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`, `ENTRETIEN.json` et
`validation_current.json`. Aucun chemin d’une autre session inclus.
La réservation se ferme à la publication du commit sur main.
