# Pool à équipes bornées : contrelecture de synchronisation

**Pas de défaut de synchronisation trouvé dans les corps capturés.** Le modèle borné normal/`-O` explore **27 séquences**, **22 766 états** et **58 167 transitions**, avec trois appels successifs et équipes de 0/1/2 ouvriers auxiliaires. Il ne qualifie ni un binaire, ni TSan, ni une accélération.

Capture à 06:59:42 UTC le 8 octobre : `pool.cpp` **490963ac…**, `sched.hpp` **87f723fe…**, `unit.cpp` **ee630ffb…**, plus quatre fichiers de contrat/portes dans [capture.json](capture.json). La capture précède la livraison ; le HEAD alors observé `dd39e486…` ne contenait pas ces modifications. **Les sept objets Git du commit livré 5b3362bbdba50a5910033808daf0011c22378069 ont ensuite été comparés : ils sont tous identiques aux octets capturés.** Les sources complètes restent hors de ce reçu. Les annonces de qualification du commit ne sont pas transférées à notre modèle.

## Visibilité et durée de vie

- L'appelant écrit `current_`, initialise le Job et `remaining` avant les `go.release`. Chaque ouvrier lit `current_` après son `go.acquire` : la publication du pointeur et du Job est ordonnée.
- Chaque ouvrier écrit sa case privée `outcomes[worker]` avant son `remaining.fetch_sub(acq_rel)`. Les RMW s'ordonnent sur le même atomique ; leur acquisition transmet les écritures antérieures au dernier décrément. Ce dernier publie `done.release`, acquis par l'appelant avant la lecture des issues. Son propre travail est déjà achevé. Le Job n'est retiré qu'après ces acquittements.
- Un ouvrier peut être retardé **après** son acquittement. L'appelant peut alors publier le prochain jeton avant que cet ouvrier revienne dans `acquire`. C'est permis : l'ancien jeton a déjà été consommé, et le nouvel appel attendra le nouvel acquittement avant de publier un troisième jeton. Le sémaphore binaire contient au plus un jeton. Le modèle atteint **2 156 états** de ce type ; aucun accès au vieux Job ne suit son acquittement.
- Une tranche passe directement par `invoke` dans l'appelant, sous la même garde `active_` ; exceptions et réentrance conservent leurs refus. W1 avec plusieurs tranches garde `run_chunks`, sans attente d'ouvrier. Le nombre `ceil(n/grain)=(n−1)/grain+1` est sûr après `n>0`, même à la borne u64.
- La destruction exige l'absence d'appel en cours, comme le contrat. Tous les jetons des appels précédents ont alors été consommés ; le jeton d'arrêt est unique. `stop_` est publié avant ce réveil, puis les fils sont joints avant destruction des sémaphores. Lors d'une construction partielle, seuls les fils effectivement construits sont réveillés et joints. Une exception de `reserve` intervient avant tout fil ; le propriétaire de `Wake[]` est déjà RAII.

Cette preuve de relations de synchronisation est une lecture du C++ ; **le modèle Python ne simule pas un matériel à mémoire faible**. Il énumère les transitions publication/acquisition, réclamation, callback retardé, écriture d'issue, acquittement et retour pour W3. Il vérifie couverture unique, absence de Job expiré, tous les acquittements avant retour et bornes des sémaphores, sous progression des acteurs. Il ne prouve pas l'équité de l'ordonnanceur OS. Les deux variantes abstraites « compteur trop court » et « retour sans attente » sont rejetées causalement ; ce ne sont pas des mutants C++ compilés.

## Portes existantes et limites

La nouvelle porte `team` est enregistrée dans CMake : W1/2/8/48, appels à une tranche dans le fil appelant, puis 2/3/7/47/200 tranches et IDs d'ouvriers dans l'équipe autorisée. Elle ne force pas tous les ouvriers engagés à exécuter un callback, ce que le contrat ne promet pas. `short_jobs` alterne mille petits/grands appels ; `coverage`, `limits`, `reentrant`, `concurrent`, `outcomes` et `exceptions` couvrent les autres chemins pertinents.

`fault.cpp` compte d'abord les allocations réelles de la factory avant d'injecter chaque position : la nouvelle allocation de `Wake[]` entre dans ce dénombrement, sans borne historique figée au nombre d'allocations précédent. Les refus de création de thread vérifient les jointures ; `allocation_free` borne l'absence d'allocation pendant un appel. **Seule leur source est contre-lue ici : aucun test natif ni journal TSan n'est rejoué ou qualifié par ce reçu.**

Deux corrections de formulation sont recommandées pour cette capture : « l'ouvrier ne reçoit un jeton que lorsqu'il attend » doit devenir « le jeton précédent a été consommé avant l'acquittement ». L'affirmation que le réveil collectif *dominait* le coût des petits nuages reste une hypothèse : le [diagnostic MES-C apparié](../session_c_diagnostic/README.md) montre une pénalité de largeur, mais n'isole pas la causalité mutex/réveil. Le nouveau Pool doit être mesuré sur le même objet et la même frontière avant toute promesse de gain.

```sh
python -B model.py --snapshot /chemin/copie-source --check
python -B -O model.py --snapshot /chemin/copie-source --check
```

Sans `--snapshot`, le modèle se rejoue seul ; avec cette option, les sept sources sont également contrôlées par leurs hashes. [Résultats](results.json), sans source produit dupliquée ni nouvelle campagne.
