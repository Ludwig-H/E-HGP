# MES-G-APP : frontière native et raccord CPU proposé

Lecture du 8 octobre 2026, source Git `389b5e453cdc1e70839a8e5a37eea7c42ff93a8d`.
Les 12 fichiers épinglés sont identiques au commit de livraison `c648b3857c83ec1ed174139eef36106b187d2cf0`.
Ce reçu ne rejoue ni moteur ni compilation et n'admet aucun chrono de campagne. Il complète la lecture du juge
et de la provenance par les autres auditeurs ; aucun défaut de résultat natif n'est affirmé.

## Ce que les temps incluent

Références ci-dessous relatives à `morsehgp3D_v12/`, aux octets Git épinglés.

| Mesure | Inclus | Préparé ou traité hors de cette mesure |
| --- | --- | --- |
| Census CPU produit, `mes_g_app.cpp:566` | Mur de `Pool::parallel_for`, grain 64, appel réel à `CensusWorkspace::query`, parcours, écriture dans le workspace, inversion de U et callback réduit à genre/p/m | Récolte, tri des requêtes, allocation d'un workspace privé par worker et des sorties |
| Census HD CPU, `:585` | Même Pool/grain, préparation de garde, parcours HD et écriture des résultats/I/U dans des tableaux fixes | Conversion des boules en descripteurs, copie des nœuds au format plat, allocations ; aucune validation d'API ni callback produit |
| Sondes CPU, `:571` | Ordres 2..K successifs ; trace, empreinte et file de prélecture G-L7, véritable `PopulationTable::find` | Construction des tables et du catalogue ; résolution après échec |
| Propositions CPU, `:621` | Chargement des coordonnées locales, `DWelzl`, écriture de la proposition | Choix préalable des parties, tri final du support, certification exacte, repli et suite de chaîne |
| GPU, `appareil.cu:171,227,270` | Événements CUDA autour des lancements sur un flux ; sondes de plusieurs ordres dans une seule fenêtre | Ouverture, allocations, préparation des entrées, H2D préalable et D2H final ; ce n'est pas le mur de l'appel hôte |

Les trois lots GPU sont des calculs résidents sur des entrées **déjà connues**. Les census ont été produits par une
résolution G préalable puis triés (`mes_g_app.cpp:460–495`) ; les propositions couvrent seulement les premiers pas
dont la première sonde échoue (`:591–618`). G produit réellement une nouvelle partie après certains census saturés
(`src/tower/resolve.cpp:298–307`). La mesure en lots supprime ces dépendances adaptatives et leur ordonnancement.
Elle n'est donc pas un temps G intégré. Le champ `recolte.g_ms` inclut lui-même l'interception, les allocations et
copies des enregistrements dans les callbacks (`mes_g_app.cpp:84–130,460–465`) : ce n'est pas un témoin G sans sonde.

La règle publiée annonce bien « noyau seul contre lot du produit ». Ces différences bornent son interprétation,
elles ne constituent pas à elles seules une violation de ce protocole. Un futur raccord devra mesurer aussi
tables, dépendances, transferts réellement nécessaires, refus et publication des cibles, dans G puis FULL.

## Portée de l'identité

L'identité est vérifiée après toutes les passes, sur les **derniers** tableaux ; ce n'est pas une comparaison de
chaque répétition (`mes_g_app.cpp:693–817`). Census HD : genre/p/m, I, U jusqu'à 64 sites, neuf compteurs dont la somme
des voies ; le contrôle séparé des coquilles larges du bras HD hôte ferme ce plafond dans la configuration nominale.
Ce contrôle ne sérialise pas tout `CensusLedger` : `passes`, `bounds` et la ventilation des voies ne sont pas comparés
directement. Le replay chronométré du produit est comparé à la récolte sur genre/p/m ; la référence I/U et compteurs
vient de la récolte. Les sondes comparent les naissances par représentant, et les propositions leurs champs/bits
utiles (centre/rayon/support/statut), sans comparer du padding C++.

Les voies contrôlée/large sont explicitement non résolues par le HD. Le booléen natif `identite` seul n'impose pas
`non_resolues == 0`. Le juge fixe `PART_NON_RESOLUES = 0.001` : il tolère jusqu’à 0,1 % de requêtes non résolues,
ce qui ne qualifie pas leur repli exact dans ce microbanc. Le lot réel en comporte zéro selon la contrelecture
de campagne de l’auditeur E ; ce reçu ne refait pas son admission. Aucun census complet au-delà de 64 sites de coquille,
aucun profil numérique général et aucune chaîne de résolution GPU complète ne sont ainsi qualifiés automatiquement.
La saturation tardive doit toujours annuler une coquille provisoirement trop grande.

Le format est propre au microbanc : K≤12, compteurs locaux u32, descripteur 144 octets et sortie 52+76×4=356 octets
par requête (`noyau_g.hpp:37–97`). Soit 500Q octets pour ces entrées/sorties seules, hors index, tables, propositions
et copies hôte/appareil simultanées. Le census produit réserve 4n octets par workspace dans `MemoryBudget`.
Le microbanc utilise un budget produit illimité et des vecteurs/cudaMalloc hors de ce contrat : aucune qualification
de budget produit ne se transfère de ces allocations.

## Aide CPU concrète, sans gain prédit

« Sans verrou » ne veut pas dire suppression d'une contention globale mesurée. Le produit utilise un
`atomic_flag` **par workspace** et le lot donne un workspace privé à chaque worker. Le bras HD change simultanément
la présentation des boules/nœuds, les frontières de fonctions et de résultats, les validations, le stockage de U
et le callback. Son ratio ne permet pas d'attribuer la différence à l'atomic_flag seul.

Raccord proposé en deux étapes vérifiables :

1. Conserver l'API et son verrou, extraire un seul parcours interne spécialisé pour les voies native/certifiée,
   alimenté par les vues du propriétaire certifié. Garder la préparation de garde dans le coût et éviter une copie
   complète d'index par requête. Conserver les voies contrôlée/large exactes. Cela teste la simplification du parcours
   sans changer la réentrance publique, la durée de vie des résultats ni le nombre de requêtes de G.
2. Si le coût de frontière reste significatif, acquérir une réservation exclusive du workspace pour un lot privé
   et valider une fois son propriétaire/index. Les descripteurs de requête doivent être construits avec seuil et
   témoins vérifiés, attachés à ce propriétaire vivant. Un parcours interne sous cette réservation évite l'acquisition
   répétée ; le flag reste pris pendant les callbacks et toute réentrance publique est toujours refusée. La garde
   RAII doit couvrir erreurs et exceptions. Une simple suppression de l'atomic_flag viole le contrat public.

Dans les deux étapes : mêmes I/U croissants en SiteIdx, coquille complète, arrêt au même seuil, refus, callback après
certification et durée de vie empruntée ; comparer **tout** `CensusLedger` et les compteurs de G. Un repli qui refait
du travail doit le compter, pas hériter artificiellement les anciens compteurs. Ne pas remplacer le stockage n du
workspace public par les 76 places du microbanc sans contrat dédié et gestion exacte des coquilles/saturations.
Portes nécessaires : index déplacé/étranger, réentrance, exceptions, saturation tardive, coquille >64, voies
contrôlée/large ; puis mêmes sources/paramètres et FUL1 pour la comparaison G et FULL, W1 et W48 distincts.

La limitation utile au développeur reste celle du [reçu CPU précédent](../cpu_feuilles_finition/README.md) : C représente
environ 83–84 % du mur FULL CPU dans la capture M. Une estimation « environ 10 % de G » n'est pas « 10 % de FULL ».
Les fenêtres G/TMVR recouvertes ne forment pas une décomposition additive du mur ; aucune projection proportionnelle
universelle n'est acquise par ce microbanc.

## Rejeu de provenance seule

```sh
python3 -B check.py /chemin/du/depot
python3 -B -O check.py /chemin/du/depot
```

Le lecteur compare les 12 blobs Git et la liaison 389→c648 ; il ne compile rien et ne prétend pas prouver par des
recherches de chaînes les propriétés sémantiques examinées ici. Résultat attendu : champ `result` de `capture.json`.

Relecture avant première publication : la phrase sur les requêtes non résolues a été précisée le 8 octobre 2026
(seuil du juge de 0,1 %, et non exigence de zéro). Sources et calculs de ce reçu inchangés.
