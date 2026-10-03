# Census emprunté dans FULL — option séparée

Implémentation préparée le 3 octobre 2026, sans compilation ni exécution native
locale. Aucune mesure ou qualification de performance acquise par ce port.
`FullParams.reuse_census_workspace=false` conserve le census possédé de référence.
La primitive d'index une passe est celle de `5c90e52cb` ; les verticales
parallèles raccordées sont celles de `7e7af48fc`. Le catalogue reste inchangé.

## Durées de vie et transaction

`build_full` crée un contexte `CensusSlots` avant les mémos et les tâches.
Il possède des `unique_ptr<CensusWorkspace>` : leurs adresses restent stables
lorsque le contexte est déplacé. L'index du `FullDomain` doit rester vivant et
immobile. Les mémos et le contexte parallèle sont détruits, puis les espaces,
avant le déplacement final du domaine dans `FullTower`.

`LocatedView` est interne et synchrone. Une recherche locale réussie emprunte
le catalogue. Sinon `query` remplit la population complète ou les premiers k
intérieurs stricts, puis appelle le même choix de support global et de trace.
Le callback copie seulement `Level`, `CellTrace`, `BirthSeed` et le ledger.
Ni `DescentStep`, ni `DescentResult`, ni un slot mémo ne retient de span.
Il n'y a pas deux populations simultanées lors d'une transition : le callback
est fermé avant le pas suivant. C++ peut copier un span ; la garantie porte
sur ce chemin inspectable et le contrat de callback, pas sur une impossibilité
générale d'échapper une référence.

Identité du domaine et du workspace vérifiée même avant un hit catalogue ou
mémo. Un workspace occupé refuse un nouveau census ; le garde RAII de l'index
est libéré aussi si le callback refuse. Aucun résultat partiel n'est publié.
Les paramètres facultatifs `CensusWorkspace*` des primitives privées restent
empruntés : l'appelant les garde vivants. Une table mémo déplacée transfère
son emprunt et invalide l'ancien contexte.

## Nombre d'espaces et mémoire

Pour Q=0, C=1. Pour Q>0, C=min(W,L,Q), W taille du Pool, L lanes logiques,
Q capacité des lots réguliers. Une admission commune vérifie exactement
`4*n*C` octets supplémentaires avant les factories. Chaque factory réserve
ensuite `n` SiteIdx dans le même MemoryBudget. Les petites métadonnées de
contrôle et leur allocation sont constantes par espace ; tableaux bornés
à 256 entrées, hors compte Buffer comme le Pool. Aucune croissance double.
Les réservations antérieures, résultats conservés, mémos, tableaux de forêt,
DSU et traces étendues coexistent et restent dans le même budget.

Les batches ont J=min(L,count) tâches. Si J<W, le slot physique est l'ordinal
de tâche (un worker d'ID élevé peut recevoir le seul job). Sinon c'est l'ID du
worker, exclusif pendant son callback. Cela couvre au plus C espaces sans
lier les mémos aux workers : leur routage par ordinal de lane est inchangé.
Après join, les verticales réutilisent les mêmes espaces. La voie sérielle et
les cellules étendues réutilisent l'espace 0 via le contexte `DescentMemo`,
même de capacité 0, sans tri supplémentaire ni compte mémo fictif.

Les préadmissions temporaires `4*n*concurrent` des deux dispatchs restent
actives seulement pour la voie possédée. Elles ne sont pas ajoutées au stockage
scratch déjà chargé. À la sortie réussie, aucun octet scratch n'est conservé
dans la tour. Les diagnostics publiés ensemble au succès indiquent
`census_workspaces` et `census_workspace_reserved_bytes`. L'absence de
FullTimings ne crée aucune horloge. Tout refus conserve domaine et anciens
résultats ; toutes les tâches ont rejoint avant retour.

## Travail exact et contrôles préparés

Les choix de graines, dates initiales/terminales, traces et hits catalogue
sont identiques. Le census emprunté effectue UNE traversée ; six compteurs
réels changent donc d'un facteur deux face au census possédé sur la même
requête. `census.passes == census_calls` dans cette voie, contre
`2*census_calls` dans la référence. Aucun doublement artificiel dans le
produit. Mémos, MEB et les autres compteurs restent inchangés à routage égal.

Portes natives préparées : `census_reuse_test.cpp` (transitions, contacts,
saturation, support local/global, profils hauts, mémos/identités, FULL
W1/W4/W48, Q0/1/2/64 et C exact, mémoire exacte/−1) et
`census_reuse_fault.cpp` (600 descentes préparées sous refus de toute
allocation, injection de chaque allocation FULL, ancien résultat et
sentinelles de diagnostics conservés). Ce sont des portes à exécuter sur G4,
pas des résultats acquis ici.

La sonde descente accepte `--workspace`. Le juge Fraction indépendant reçoit
explicitement la même option : il conserve la définition Γ fermée et juge
une passe et `4*n` octets, tandis que son défaut impose toujours deux passes.
Le modèle pur `census_reuse_model.py` refuse la confusion des deux voies,
les dates erronées et les mémoires incorrectes ; il contrôle aussi l'injectivité
des slots physiques. Aucun benchmark ou contrat FULL≤200 ms ne découle de
ces contrôles. Le raccord benchmark sera une tranche séparée.
