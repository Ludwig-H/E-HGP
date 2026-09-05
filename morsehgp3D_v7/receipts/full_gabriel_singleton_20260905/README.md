# Qualification ciblée FULL du chemin singleton

5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.

Deux constructions neuves : 17/17 CTests Release et 17/17 ASan/UBSan, sept binaires par build. LeakSanitizer reste actif (detect_leaks=1), sans override ni désactivation.

Les six binaires antérieurs restent sans macro testing ; le septième singleton porte seul MHGP7_TESTING=1. Filtre de labels exact, argv et codes attendus contrôlés ; pas de suite F importée. Les deux juges de métadonnées ont des modèles distincts : deux positifs et douze mutants, sans les confondre avec des résultats moteur.

Les bruts complets, JUnit, LastTest, commandes, options, dépendances, sources et pins des binaires figurent sous [qualification/](qualification/). Les éventuelles captures de développement restent séparées et ne sont jamais promues en qualification fraîche. Aucun ELF, objet compilé ni code Boost n’est exporté ; les modules Python importés sont copiés octet pour octet et co-localisés sous qualification/protocol/. Le publisher est sous protocol/.

Provenance bornée : HEAD est éventuellement déclaré par ROOT, pas authentifié ici ; le worktree est identifié par la carte des sources effectivement qualifiées, pas par un statut Git global. Build frais mais non hermétique : headers système listés, non tous pré-épinglés. L’ancien depfile Boost sert seulement à pré-épingler les 521 headers consommés, jamais un ancien résultat.

Aucun gain de performance, contrat 50k/1 s/100 ms, verticale inter-K ou résultat massif G4 ne découle de ces tests. [Statut et références](publication.json), [inventaire](manifest.json), [sommes](SHA256SUMS). GCP non utilisé.
