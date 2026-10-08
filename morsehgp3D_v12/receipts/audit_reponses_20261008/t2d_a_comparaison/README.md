# T2-d-A : isoler la nouvelle route du gain historique

8 octobre 2026. Proposition **avant toute qualification G4 de A**. Capture persistante du pilote à07:20:36 UTC, puis confrontation aux blobs de sa livraison **`bcd742c0517f38b33ef85e212c94414377998157`**. Aucun moteur, CUDA, contrôleur, GCP ni brut de scène exécuté ou lu. Aucun nouveau temps mesuré.

**Constat de périmètre.** `BRAS=(avant,apres)` dans le pilote livré signifie `902041f66` contre le paquet actuel avec `--recouvert`. L'actuel comprend aussi le catalogue C de `02b735d6b` et le Pool de `5b3362bbd`. Le rapport mesure donc le gain du **lot complet** depuis902, pas la seule contribution de A. `apres_sequentiel` utilise déjà la même sonde sans le drapeau, mais uniquement dans les prises d'identité ng00–02/K5 : deux passes avec empreinte. Il n'entre ni dans les tours de la campagne K5 ni dans la formule d'adoption. Ses temps d'identité ne remplacent pas une cohorte chronométrique appariée.

**Proposition concrète.** Avant de lancer G4, déclarer deux comparaisons distinctes :

| Comparaison | Sources/exécution | Question traitée |
|---|---|---|
| Principale A/S | Même binaire courant, SHA identique : `apres` avec `--recouvert` contre `apres_sequentiel` sans ce seul drapeau | La route A améliore-t-elle la route séquentielle actuelle ? |
| Historique A/H | Binaire courant contre archive902 épinglée | Quel est le gain du lot entier depuis902 ? |

Pour A/S, reprendre ng00/ng01/ng02,K5,W48, le même nombre de processus/passes, exclusion de la première passe et ratio des médianes chaudes par tour. Conserver la moyenne géométrique, le bootstrap apparié sur les tours, la graine et la condition « borne haute sous1 sur chacune des trois trames » **si cette règle est annoncée comme celle de A/S avant la mesure**. Les prises doivent différer seulement par la route : mêmes données, profil, catalogue, Pool, numérique, options/budgets/cache, contexte résident par processus et options de compilation. Alterner les deux routes dans les tours. L'égalité du binaire évite de faire dépendre cette attribution d'une liste incomplète de fichiers supposés inchangés.

Le pilote doit enregistrer S dans la cohorte chronométrique et le juge doit le prendre explicitement comme dénominateur : ajouter une clé de plus à un tour ne suffit pas, car le juge actuel l'ignore. Les journaux conservent le schéma séquentiel pour S et recouvert pour A, chacun admis depuis sa commande. Étendre les identités nécessaires aux deux routes et conserver le contrôle historique séparé. Exiger les paires complètes, sans éliminer silencieusement les échecs de S. Si les trois bras sont joués, garder une cohorte de triplets commune pour les décompositions ; ne pas diviser des IC ou des agrégats provenant de cohortes différentes.

**Ce que A/S isole réellement.** `full_probe.cpp:247–261` appelle encore `resolve_tower`, construit les `forest_input`, puis appelle `build_forests` en séquentiel ; `run_overlapped_wall:292` appelle `build_tower`. Cette dernière route ouvre/prépare les structures, alloue/admet une région et les index par ordre, puis joue son graphe dans le Pool (`pipeline.cpp:147–209`). A/S inclut donc ces changements d'ouverture, d'allocation, de raccord et d'ordonnancement. Ce n'est pas une mesure du chevauchement **seul**. Pour isoler celui-ci strictement, il faudrait une ablation supplémentaire de la même route A avec une barrière empêchant la forêt d'avancer avant la fin de G, tout en gardant structures, admission et travail ; cette ablation n'est pas présente et n'est pas exigée pour nommer correctement A/S.

Publier FULL par passe comme mesure principale, avec CPU, pics hôte/appareil, ouverture/admission et fins par ordre comme diagnostics. Les murs T/M/V/R de S et les sommes de fenêtres de A sont de natures différentes : ne pas les soustraire pour annoncer un gain de recouvrement. Le [correctif de fenêtre de pré-passe](../t2d_a_fenetre_patch/README.md) reste applicable au corps `pipeline_run.cpp` SHA`08b385e6…` capturé ; il concerne un diagnostic, pas la durée FULL. Aucun résultat déjà pris ne doit être rebaptisé ou rejugé rétroactivement sous la nouvelle comparaison.

**Contre-épreuve pure Python.** [check.py](check.py) extrait seulement les fonctions pures du juge livré et utilise sa voie synthétique `verifier=False`. Normal/−O identiques ; valeurs **inventées, en unités arbitraires** :

| H / S / A | A/H, jugement historique | A/S, même formule |
|---|---|---|
|100 /70 /77|0,77 : adopté|1,10 : rejeté|
|100 /120 /108|1,08 : rejeté|0,90 : adopté|

Les ratios satisfont par tour `A/H = (S/H) × (A/S)`. Les deux jugements répondent à des questions différentes ; ce témoin n'est ni un défaut d'admission de bruts ni une mesure de performance. Le test n'appelle aucune sonde, construction ou détection GPU.

**Pins et capture.** Le pilote SHA`ea9023cf…` et huit autres fichiers capturés sont égaux aux blobs bcd. Le snapshot contenait déjà un nouveau `pool.cpp` local SHA`3d3985de…`, différent du blob livré : ce delta est explicitement séparé dans [capture.json](capture.json), jamais attribué à bcd. Le lecteur recontrôle les blobs Git livrés et902, pas un checkout mobile. La copie externe reste une sauvegarde de provenance ; aucune source complète supplémentaire n'est ajoutée au reçu.

```sh
python -B check.py --repo /workspaces/E-HGP
python -B -O check.py --repo /workspaces/E-HGP
```

Résultats : [results.json](results.json). Fermeture : [SHA256SUMS](SHA256SUMS). Proposition de protocole uniquement, sans modification produit, règle livrée ou registre.
