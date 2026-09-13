# Census q2 : exceptions et réemploi des objets

13 septembre 2026, après `256957a5`. Cadre :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `audit_independant_math_and_architecture`,
`not_claimed`. Snapshot privé des sources census non publiées ; aucun
build épinglé utilisé ni fichier moteur modifié.

## Avis borné favorable

Les pannes de construction d'index, les pannes d'appel et une exception
du callback après plusieurs émissions laissent l'index et le plan
réutilisables. Une reprise complète retrouve les mêmes supports, clés,
IDs intérieurs/coquilles et compteurs. Le flux partiel précédent reste
présent chez l'appelant jusqu'à son abandon explicite.

La lecture explique ce résultat : l'index possède son propriétaire et
ses données immuables ; l'arbre B de requêtes, les comptes, les curseurs,
les buffers d'IDs et le résultat courant appartiennent à un moteur local
à chaque appel. Aucune continuation incomplète n'est enregistrée dans
l'index ni dans le préfiltre. L'ordre des contrôles propriétaire, mode et
callback précède le retour sur résidu vide.

Le contrat d'emprunt est déjà clarifié dans l'API : index, plan et callback
restent vivants et non invalidés pendant tout l'appel. Le juge respecte
ce contrat, copie les payloads pendant le callback et ne conserve aucun
span emprunté. Il ne demande pas de supporter leur invalidation depuis
le callback, ni d'annuler les effets externes du consommateur.

## Ce que la porte supplémentaire exerce

La gate constructeur traite déjà une exception dès le premier callback
et les entrées invalides sur résidu nul. Le [nouveau juge](q2_census_lifetime_probe.cpp)
ajoute les allocations ordinaires et les interruptions **après sortie**.
Une seule fixture active de 19 sites, deux grilles 3×3 et un témoin extérieur
aux facteurs, donne 81 candidates. Les deux modes livrent chacun 61 supports
et rejettent 20 paires. Le stockage fixe du callback n'alloue pas : les
pannes injectées pendant l'appel appartiennent au census.

| Contrôle, par exécution du juge | Résultat |
| --- | ---: |
| Pannes injectées à chaque allocation de construction d'index, jusqu'au succès | 10 |
| Pannes injectées dans les deux parcours, jusqu'à leurs succès | 20 |
| Parmi elles, pannes après des émissions déjà copiées | 8 |
| Exceptions au troisième callback, propagées avec leur type et leur valeur | 2 |
| Supports conservés dans ces deux flux interrompus | 3 + 3 |
| Reprises complètes sur les mêmes objets | 24 |
| Appels valides sur résidu vide, sans allocation ni callback | 2 |
| Mauvais owner/mode/callback ou plan déplacé refusés sur résidu vide | 8 |

Les injections visitent toutes les positions d'allocation des chemins
bornés jusqu'au premier appel qui ne les atteint plus. Elles couvrent
aussi la fin de construction du `shared_ptr` d'index : une défaillance
de son bloc de contrôle doit libérer l'objet fraîchement construit.
Après chaque échec, le juge compare le solde des allocations ordinaires,
les nombres de références au propriétaire et à l'index, l'identité des
propriétaires et tous les compteurs de l'index/préfiltre. La reprise compare
les résultats discrets et tous les compteurs du census ; les durées ne
sont pas comparées. Aucun déséquilibre ni changement trouvé.

Les payloads de référence sont copiés dans un stockage possédé et contrôlés
ponctuellement sur les 19 sites : 122 supports, 2 318 tests signés indépendants.
Les témoins globaux, les endpoints et les coquilles sont non vacants.
Ce contrôle de contenu sert la vérification du réemploi ; il ne remplace
pas l'oracle exhaustif indépendant du constructeur pour la complétude.
La comparaison des préfixes d'émission porte sur ces exécutions déterministes,
sans ajouter une garantie générale d'ordre des callbacks à l'API.

Après une exception, le juge remet explicitement la taille du flux partiel
à zéro avant la reprise. Le moteur n'a donc pas annulé les trois supports
reçus et ne prétend pas fusionner automatiquement deux tentatives.

Deux mutants de copies temporaires sont réfutés avec code 1 : avaler
l'exception du callback, ou retourner immédiatement sur résidu vide avant
les validations. La version positive sort avec code 0. Les erreurs sont
causales et distinctes dans le reçu, pas assimilées à une erreur de compilation.

## Sources et reproduction

Le [reçu autonome](Q2_CENSUS_LIFETIME_CHECKS.json) conserve neuf sources
produit et le juge, les hashes, commandes, sorties et variantes mutées.
Le cpp census est `3c513cc474c3d3a249779032f5cd03dac47198cf4b25d7698855bd118e0e593a` ;
le hpp clarifié est `2116ac4ba0fcc896e4daa89583ecff6b32f307fcfa579121403b77b52d6b7a9e`.
Les neuf sources sont stables pendant la capture et la fermeture normales.
Le head sert uniquement de contexte : ces octets census ne sont pas encore
dans `256957a5` et aucune qualification antérieure n'est transférée.

Le [runner](q2_census_lifetime_checks.py) construit dans un répertoire
temporaire neuf avec GCC 13.3, C++20, `-O1`, avertissements stricts et UBSan
sans récupération. Normal et Python −O donnent les mêmes résultats.

```bash
python3 audits/morsehgp3D_v8_complementaire/q2_census_lifetime_checks.py --replay audits/morsehgp3D_v8_complementaire/Q2_CENSUS_LIFETIME_CHECKS.json --output /tmp/q2_census_lifetime_replay.json
```

L'injection couvre les allocations ordinaires exercées, pas un stockage
suraligné ni un allocateur externe. Aucun benchmark lourd, temps de
performance, concurrence de threads, census général de toute la WSPD,
canonisation globale ou tour FULL n'est qualifié. GCP non utilisé.
