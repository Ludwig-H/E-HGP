# B livré : raccord au census de la Session A

8 octobre 2026, Codex. Lecture ciblée favorable au pin **41d4d828b** : aucun nouvel emprunt périmé ni allocation variable non admise trouvé dans le raccord de B à A. Ce résultat ne qualifie ni l’exécution concurrente entière, ni un temps A+B. Aucun moteur, test natif ou appel GCP exécuté.

La capture initiale des vingt fichiers B est stable à 08:03:54 UTC sur HEAD e02256457 ; ces vingt contenus sont exactement ceux ensuite livrés en 41d4. Vingt-sept blobs choisis sont vérifiés ; les sept du raccord/admission sont inchangés depuis e022. La fermeture live à 08:13:58 trouve deux nouveaux changements non commis : `pipeline_run.cpp` adopte LeafWindow et `tests.cmake` change les portes FULL. Le présent avis reste épinglé au livré 41d4 ; ces travaux ultérieurs ne sont pas qualifiés ici.

## Port et preuves réutilisées

Seize des vingt fichiers sont égaux octet pour octet au prototype **545b5e5b6** du [reçu de mutants B](../t2d_b_mutants/README.md). Les quatre différences sont bornées : commentaire de `proposal.hpp` corrigé (« canonique parmi F », pas S* global), assertion de `guarded_test.cpp` corrigée de `guard_outside` vers `guard_disjoint` pour une feuille d’un seul site, dix mutants A ajoutés au manifeste tower, et portes A ajoutées à CMake. La comparaison de fichiers n’hérite pas des résultats d’exécution du prototype.

Preuves réutilisées : [garde resserrée](../garde_census/README.md), [census à témoins](../census_temoins/README.md), [proposition entière et oracle Fraction](../t2d_b_proposition/README.md). B≤32 reste couvert : carré/dot ≤3(2^B−1)², i64 jusqu’à B30 puis i128 ; garde signée i64, paliers et certificat s+2 conservés. Les mutants s/s+1 décrivent désormais une politique de voie.

## Durées de vie et concurrence dans A

`resolve.cpp:121,174–176,304–314` : Located possède sa CertifiedBall et ses quatre SiteIdx. La référence de sphère de CensusStep et le span des témoins restent valides durant tout `workspace.query`. Aucun de ces emprunts n’est placé dans la file G-L7. Part est copiée dans la file et résolue synchroniquement (`passes.cpp:70–93`). Le callback ne conserve pas les spans du résultat : il recopie la prochaine Part, ou un indice de boule.

`census_workspace.cpp:99–122` : Bounds et son GuardedSphere référencent la boule encore vivante ; le parcours finit avant le callback. Les compteurs accumulés sont initialisés à zéro par GuardLedger/LaneCount, transférés avant la destruction de Bounds. Le verrou ActiveQuery survit au callback et se libère aussi sur refus/exception. Le stockage I/U est réutilisé seulement après le retour de query ; les cursors/compteurs du parcours sont locaux et réinitialisés à chaque appel, sans remise à zéro intégrale du tampon requise.

Dans A, `run_region` exécute successivement les tâches d’un identifiant de fil ; `run_g` transmet ce même identifiant à resolve_cells. `passes.cpp` choisit `workers.census[worker]`. Deux ordres peuvent donc résoudre en même temps, mais avec deux espaces distincts ; un même fil ne lance pas deux queries simultanées. Catalogue, index et points exacts restent immuables jusqu’après la sortie de la région. La destruction de SessionRun détruit Pipeline avant Workers ; le retour des tours déplace les sorties, pas les tampons de census. Cette lecture complète le [contrôle du graphe A](../t2d_a_dependances/README.md), sans le rejouer.

## Admission et portée

`pipeline.cpp:30–39,175–183` inclut workers_bytes dans l’admission commune AVANT staff/build_tables/région. `stage.cpp:196–208` calcule **W·(4n + sizeof(OrderCounters) + sizeof(SectionCycles))**, puis prépare W workspaces, chacun propriétaire de n SiteIdx. Le tableau de profils absent hors mode instrumenté rend cette partie conservatrice. Les admissions de make vérifient le restant sans seconde réservation. Le budget compte les Buffer, avec marge de cache, pas la pile ni les petits objets de contrôle : aucune revendication de RSS.

B ne change pas ces formules, n’ajoute aucun Buffer, ni allocation par query. Témoins, accumulateur de garde, coordonnées locales et états numériques ont une taille bornée indépendamment de n ; les paliers étroit/large ne créent pas un espace par ordre. Il reste **W**, non K·W, buffers de census en coexistence avec les tableaux de tous les ordres déjà prévus par A. Ceci ne revérifie pas toutes les formules M/V/R.

L’objet et ses incidences restent protégés par les mêmes certificats, census complet/saturé, canonicalisation globale et contrôle de descente. L4 peut cependant changer proposition, départage, route T1/certificat/census et travail physique : « routes inchangées » n’est qu’une observation des cas effectivement rejoués, pas un théorème universel. Aucune baisse de temps ou gain asymptotique ne découle de cette relecture.

Rejeu léger, sans assertions désactivables :

```sh
python check.py /chemin/depot
python -O check.py /chemin/depot
```

Les deux sorties sont égales à `results.json`. `--prototype /chemin/git-du-prototype` revérifie en plus les vingt hashes historiques ; il change seulement `prototype_relu`. Aucun octet source ni log brut n’est recopié dans ce reçu.
