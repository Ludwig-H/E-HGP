# Contre-audit du reçu de rampe de raffinement G4

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Snapshot et portée

Le document relit `HEAD=35fcea884cb93eff24db1e7c5962f8be23d4cb04`.
Les exécutables mesurés proviennent du pin propre
`3c11bc8f99dd5f43eeaa973d61157ac2ae58e74e`. Les sources produit sont
bit-identiques entre ces deux pins :

- `prototype/wspd_wavefront_probe.cpp` :
  `cfddfc89222a9179086f99b247abf933cc24f2d22f2d2422099b86aebad8ad74` ;
- `prototype/ball_event.hpp` :
  `eedd8521c31fa7963506b4fc1030eb6f92491d3d79f0e8356641c7035660b24a` ;
- `prototype/ball_event_probe.cpp` :
  `4a85f6cbdd74054160266ee2bbb1dd13a1d54fb7ffb9ce522855aff93be3e793` ;
- `CMakeLists.txt` :
  `3be8e878ea86f8a406e01bcf1e21e5a6986929d66420323118095c8a60c4b223`.

Le transcript reçu a pour SHA-256
`93df964d6430f80be51132bd47655a01d46a7722e1750834702ce1829af27f90`.
Il certifie l'arrêt `TERMINATED` de la cible exacte démarrée. Cette campagne a
employé la VM G4 comme machine **CPU** ; elle n'a exécuté aucun kernel CUDA et
ne mesure ni `warm_e2e`, ni débit GPU, ni payload Morse/HGP.

Les onze hashes du manifeste concordent et les dix fichiers `rampe_*.txt`
rejouent exactement les blocs correspondants du transcript. L'archive tar
elle-même n'est pas conservée : son hash reste un engagement du transcript,
pas un contenu re-hachable après coup. GMP était absent sur la VM ; le témoin
arithmétique large optionnel n'a donc pas tourné.

## 1. Verdict court

Le reçu est un diagnostic reproductible et utile de la relation produite par
`Central-VWave + raffinement local`. Il ne reçoit toutefois ni quarante
fenêtres finales, ni le coût du raffinement, ni l'étape 1, ni le contrat G4.

Il établit trois résultats bornés :

1. sur `uniform`, le **surensemble E4 du certificateur courant** a des pentes
   empiriques basses après profondeur quatre ;
2. sur `eight_clusters`, la configuration `s=8`, `window=512`, profondeurs
   zéro et quatre garde deux pentes successives proches de `1,9` et doit être
   abandonnée comme source scalable sous cette forme ;
3. sur `terrain`, le dernier intervalle reste rouge et certaines
   continuations ne sont pas consommées : la fenêtre n'est pas finale.

La phrase « `uniform` passe » signifie donc seulement « la porte empirique
`sum E4` de cette source passe sur ce run ». Elle ne reçoit ni `M4`, ni les
BallKeys, ni le census, ni le fold, ni un temps.

## 2. `COMPLETUDE=OK` ne signifie pas fenêtre finale

Les dix fichiers contiennent bien quatre lignes `code=0`, et les quarante
processus ont terminé. Le filtre du script ne conserve cependant pas
`fenetre_finale`, et le probe rend zéro même avec `pending>0`.

Les lignes massiques prouvent les continuations suivantes :

| famille | profondeur | taille | lane | masse pendante terminale | compteur d'appels `pending_lane` |
|---|---:|---:|---:|---:|---:|
| `terrain` | 0 | 50 000 | q4 | 22 723 | 2 |
| `terrain` | 4 | 25 000 | q3 | 16 | 7 |
| `terrain` | 4 | 25 000 | q4 | 134 | 9 |
| `terrain` | 4 | 50 000 | q3 | 4 217 | 48 |
| `terrain` | 4 | 50 000 | q4 | 19 006 | 141 |

Le dernier compteur n'est pas un nombre fiable de terminaux : il est incrémenté
avant la décision de split et peut garder des parents ensuite raffinés. La
masse pendante, dérivée du ledger terminal, est en revanche positive et suffit
à prouver la non-finalité.

Dans ces lignes, `masse_ouverte` et `sum_E` valent
`open_mass + pending_mass`. Ce sont des surensembles cap-dépendants. Les
pentes q4 publiées pour `terrain` ne sont donc pas des pentes de fenêtre
finale.

Le défaut numérique est petit mais le défaut contractuel est absolu. La masse
finale après continuation est inconnue entre l'ouvert strict et le surensemble
actuel ; retirer le pending ne « corrige » donc pas la fenêtre. On peut en
revanche borner la dernière pente q4 profondeur quatre entre `1,533460` et
`1,536806`, et celle sans raffinement entre `1,615659` et `1,616587`. Le verdict
rouge est robuste à toute résolution de ces continuations, mais aucune valeur
finale exacte n'est reçue avant `pending=0`.

`COMPLETUDE=OK` doit être renommé ou qualifié comme complétude des fichiers.
Une future porte scientifique exige, par lane, l'identité exclusive
`input=closed+open+pending`, puis `pending=0` et `fenetre_finale=OUI`.

## 3. Le rapport `1,62` contre `1,57` n'est pas un prix

À `n=50 000` sur `eight_clusters` :

- `E4` descend de `852 642 889` à `525 902 961`, soit
  `326 739 928` candidats retirés ;
- le front monte de `20 264 055` à `31 852 043`, soit
  `11 587 988` records de front supplémentaires ;
- les tests de front montent de `40 478 111` à `63 654 087`.

Comparer les deux **rapports multiplicatifs** `1,62` et `1,57` n'est pas une
comparaison de coût : les numérateurs, les unités et les coûts unitaires sont
différents. Les mêmes données donnent environ `28,2` candidats E4 retirés par
record de front supplémentaire, ou `14,1` par test de front supplémentaire. Cela
pourrait être favorable si une incidence E4 aval coûte cher, ou défavorable si
chaque recertification relit de grandes banques. Le reçu supprime précisément
les recertifications, octets, HWM, temps et masses aval qui permettraient de
trancher.

La proximité est en outre fortuite à 50 000 : de 6 250 à 50 000, les facteurs
de réduction E4 restent environ `1,64 / 1,65 / 1,64 / 1,62`, tandis que les
facteurs de front décroissent `2,70 / 2,23 / 1,83 / 1,57`.

Le rejeu local à `n=3 000` est un avertissement : profondeur quatre multipliait
les recertifications par `6,32` sur `eight_clusters`. On ne peut donc conclure
ni « coût exact », ni « aucun gain structurel » depuis les seuls fronts.

## 4. Réponse à la question du cœur commun

Non. La proximité `1,62/1,57` ne prouve pas que toute sous-boîte, si petite
soit-elle, possède un cœur commun vide.

Elle mesure un unique classifieur suffisant, une profondeur bornée, un `s`, une
graine et une famille. Même une paire singleton peut échouer le masque central
et être fermée par :

- le spindle exact corrélé `SOC64/CORNER512` ;
- un groupe projectif dont le témoin dépend du centre ;
- une cage/fleur ;
- une combinaison de crédits disjoints.

Pour distinguer « perte de rectangle » et « absence de profondeur universelle
dans le pool », il faut classifier le même résiduel par étages :

```text
central-MIXED
  -> SOC64/CORNER512
  -> LP UniversalSphereDepth-1 puis -8 sur pool authentifié
  -> cage/fleur
  -> OPEN avec continuation
```

L'échec du LP universel complet peut réfuter un cœur **universel** de profondeur
donnée pour la paire et le pool. Il ne prouve toujours pas l'absence d'un
support Morse, car il porte sur toutes les sphères passant par la paire.

## 5. Terrain : cages ou plus de profondeur ?

Le reçu ne justifie pas de choisir l'un contre l'autre. La séquence terrain
profondeur quatre `1,296 / 1,344 / 1,537` **monte** sur le dernier intervalle ;
elle ne décroît pas lentement. Les deux derniers points ne sont en outre pas
finaux.

La décision sûre est une cascade, pas un remplacement :

1. geler le raffinement aveugle à profondeur quatre ;
2. brancher `SOC64-shadow-q4` counter-only sur exactement les tâches
   `central-MIXED` ;
3. utiliser le LP projectif comme oracle de cause sur un échantillon scellé ;
4. mesurer ensuite `CertifiedCageWindow` sur le même résiduel si le LP révèle
   assez de profondeur universelle réutilisable, avec taux `FULL/UNDERFULL`,
   formes, arêtes proches et masse `UNDERFULL x UNDERFULL` ;
5. seulement ensuite comparer profondeurs `0/2/4/6` avec héritage des preuves
   `ALL/NONE`, continuations consommées et coût incrémental.

Les cages sont vraisemblablement un bon fast path pour les ancres intérieures
volumiques ; terrain/scanline peuvent être de rang effectif inférieur ou
UNDERFULL. Elles doivent donc rester un complément fail-open de PWC, jamais la
nouvelle source complète par décret.

## 6. Portée du NO-GO `eight_clusters`

Le résultat justifie d'arrêter la route suivante comme candidat scalable sur
la famille mesurée :

```text
Central-VWave + certificat central actuel + s=8 + window=512
+ raffinement local de profondeur <=4
```

Deux pentes successives de `sum E4` restent proches de `1,9`, et la masse à
50 000 vaut `525 902 961`. Continuer à micro-optimiser cette combinaison sans
changer de certificateur viole l'esprit de la porte de revue algorithmique.
Ce no-go d'ingénierie ne reçoit pas le protocole asymptotique préannoncé, qui
demandait plusieurs graines et une sélection plus complète des leviers.

Cela ne réfute ni tous les rectangles, ni `SOC64`, ni `CORNER512`, ni PWC/LP,
ni les cages, ni un raffinement porteur de preuves branché après un meilleur
certificat. Il faut éviter le raccourci « aucune boîte assez petite ne peut
fermer ».

## 7. `uniform` seule ne qualifie pas le SLO enregistré

La lecture « une famille volumique favorable » de la section 14.4 du plan ne
peut pas être isolée de la section 14.5 et de la porte G6 :

- le régime 1 est Poisson uniforme ;
- le régime 2 est un mélange équilibré de huit amas ;
- les objectifs sont évalués sur **1 et 2** ;
- G6 exige explicitement les **deux familles favorables**.

Une campagne `uniform` seule peut publier un résultat borné
`uniform-only` ou recevoir une brique de source pour cette famille. Elle ne
qualifie ni G6, ni le SLO produit actuel. Modifier cette portée demanderait une
décision explicite dans le plan et la spécification, pas une interprétation a
posteriori d'un run défavorable.

Même sur `uniform`, `E4` vert ne suffit pas : `M4`, les formes, les
intersections shallow, les BallKeys, la sortie, les octets et le
`BenchmarkOutputContract-v1` restent inconnus.

## 8. Corrections de la recette réutilisable

La session précise s'est terminée et son arrêt ciblé est reçu. Le script reste
cependant impropre à une nouvelle qualification sans réparations :

- armer le trap de l'orchestrateur avant toute commande faillible suivant le
  retour de `start_and_verify.sh` ;
- capturer `PIPESTATUS[0]` sous `set +e`, restaurer `set -e`, continuer les
  quatre tailles et exiger quatre `code=0`. La validation finale refuse bien
  une ligne manquante, mais l'héritage d'`errexit` peut faire sortir le
  sous-shell avant sa publication ; contrairement à son commentaire, le
  script perd alors ce code et abandonne les tailles restantes ;
- conserver les sorties brutes, `fenetre_finale`, les trois masses et les
  compteurs de coût ;
- calculer et gater les pentes dans un analyseur versionné, au lieu d'un calcul
  documentaire postérieur ;
- rendre cohérents les timeouts par run, le coupe-circuit invité et le
  `maxRunDuration` ;
- archiver aussi le reçu d'échec et rendre tout échec de copie bloquant.

Les `27/27` CTests WSPD ne couvrent pas l'option `--raffine`; les quarante runs
ne possèdent ni oracle PairId de raffinement, ni comparaison de fenêtre finale.
Le `10/10` BallEvent reste auxiliaire et ne corrige aucun des P0 de `0A`.

## 9. Décision transmise à Claude

1. Conserver la réfutation empirique de la configuration centrale sur
   `eight_clusters`, en resserrant sa portée.
2. Retirer les formulations « prix exact », « aucun rectangle si petit » et
   « SLO qualifiable sur uniform seule ».
3. Ne pas approfondir aveuglément terrain : mesurer d'abord
   `SOC64 -> LP diagnostic -> cages conditionnelles`, puis le raffinement
   porteur de preuves.
4. Garder q3 sur la route `owner-edge x carrier -> pied -> BallKey/RLE ->
   profondeur batchée`; aucune conclusion q4 de ce reçu ne la réfute.
5. Fermer `0A/0B` et mesurer `M3/M4` avant tout claim de seconde ou de GPU.

GCP non utilisé par cet audit. Le reçu relu certifie l'arrêt de la session GCP
qu'il documente.
