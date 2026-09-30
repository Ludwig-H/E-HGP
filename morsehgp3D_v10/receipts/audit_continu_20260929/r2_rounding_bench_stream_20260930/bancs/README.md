# Audit court des bancs statistiques R2

30 septembre 2026. La correction des scores hors domaine est confirmée sur
les sources figées de `/tmp/mhgp10-r2/bancs`. Deux angles nouveaux restent
ouverts : les paramètres de décision du préenregistrement ne sont pas
validés, et des colonnes CSV de même nom peuvent se masquer.

Cet audit n'a exécuté aucun moteur, généré aucun nuage ou lancé aucun GCP.
Les scores de la fixture sont fabriqués, sur quatre unités seulement.
Ils servent à tester le juge, pas à comparer réellement HGP et HDBSCAN.
`public_status=not_claimed`.

## Paramètres de décision invalides acceptés

Le témoin à `alpha=0.05` termine code 0 sans revendication de supériorité.
Avec les mêmes lignes de scores mais un préenregistrement à `alpha=2`,
le juge accepte aussi le lot puis écrit « la tour bat HDBSCAN ».
La p-valeur corrigée publiée vaut alors 1/3. Une valeur de seuil 2 n'est
pas un niveau de test valide.

Le changement de préenregistrement change aussi le SHA et donc la graine
du test aléatoire : les deux p-valeurs ne doivent pas être présentées comme
identiques. Le constat est l'acceptation et l'utilisation d'un seuil
invalide, pas une expérience de clustering ou un gain de qualité.

`alpha=NaN` passe également les contrôles et publie une décision code 0,
avec une conclusion sans différence significative. Une paire qui référence
`not_registered`, ou l'absence de `bootstrap`, est admise par
`--check-only` puis provoque un `KeyError` et un code 1 en décision
complète, au lieu d'un refus de configuration propre. Des définitions de
méthodes dupliquées sont admises par le contrôle du lot.

Action proposée : valider le schéma du préenregistrement avant toute
décision, notamment la finitude et le domaine de alpha, les nombres de
tirages, les paramètres requis, l'unicité des noms et les références des
paires. Étendre ce contrôle aux familles secondaires et descriptives.
Les préenregistrements A et C actuels ont un seuil 0.05, des paramètres
présents et des noms/paires cohérents : ces contre-épreuves ne disent pas
que leurs décisions historiques sont fausses.

## Un en-tête CSV ambigu est accepté

La fixture `duplicate_ari_header` contient deux colonnes `ari_s`.
Pour les lignes de la tour, la première contient 1.25 et la dernière 0.8.
`csv.DictReader` conserve la dernière : contrôle code 0, décision code 0,
ARI moyen publié 0.8. La même valeur 1.25, sous un en-tête unique, est
correctement refusée code 2.

La garde de domaine n'est donc pas absente : elle contrôle la valeur que
le lecteur a gardée. Le problème nouveau est l'acceptation d'un schéma
ambigu, dont un autre lecteur pourrait interpréter la première colonne.
Refuser les noms de colonnes dupliqués avant lecture des valeurs ferme
ce chemin sans modifier les métriques ni le moteur.

## Ce qui reste correctement traité

Nos passages [normal](normal/receipt.json) et
[optimisé](optimized/receipt.json) exécutent vingt-deux appels courts chacun,
avec les mêmes codes, verdicts et statistiques. Les contrôles refusent
ARI 1.25, NaN sur une ligne non refusée et une mauvaise métadonnée de bruit.
Le NaN d'une ligne `refused=1` est intentionnellement remplacé par zéro :
la décision complète est finie et publie un ARI moyen nul pour cette méthode.
Ce n'est pas une régression.

`ari_nc`, `coverage` et `clusters` ne participent pas à la décision ;
leur absence de contrôle est explicitement documentée dans la source R2.
La fusion d'une session à ARI 1.25 écrit une campagne structurellement
complète, mais le juge appliqué ensuite la refuse code 2. La fusion seule
n'est donc pas une qualification numérique, et cet essai n'a pas contourné
le juge complet.

Le [résumé des mutants de complétude](developer_proofs/preuves/b_completude/mutants/resume.jsonl)
correspond aux correctifs annoncés : les quatre mutants M09, M10, N03 et
N04 sont tués par leurs cas isolants. Le survivant N02 est équivalent après
unicité des noms d'unités du plan et contrôle d'appartenance/doublons :
l'égalité des ensembles devient équivalente à l'égalité des effectifs.
Nous n'avons pas relancé cette longue campagne.

## Signaux réels et simulations distingués

Le sous-ensemble de preuves relu contient soixante essais de groupe sous
Python 3.12 et soixante sous Python 3.10. Les cent vingt marqueurs de tour
sont présents, aucune tour n'est encore vivante au contrôle du harnais et
les codes externes sont −15. Les scripts utilisent de vrais signaux POSIX
et l'enveloppe de processus prévue pour le worker, mais des binaires factices,
sur CPU local ; ce n'est pas une exécution G4.

Les fichiers `*_simulation_close_group.txt` documentent, eux, un second
signal simulé par appel du gestionnaire depuis un proxy. Les enfants sont
réellement lancés et fermés, mais cette injection déterministe ne doit pas
être comptée comme un nouveau signal POSIX. Nos propres quarante-quatre
appels ne lancent aucune expérience de signal.

## Provenance du paquet

[receipt.json](receipt.json) précise les limites et les codes ;
[source_inventory.json](source_inventory.json) lie chaque copie à son
chemin d'origine, sa date exacte en nanosecondes et son hash.
[observed_binaries.json](observed_binaries.json) conserve huit empreintes
de binaires natifs, sans les copier ni les exécuter dans nos sondes.
Le manifeste original du développeur est vérifié avec code 0 au moment
de la capture ; les sources figées restent identiques avant/après les tests.

`cases/` contient les fixtures immuables, `runs/normal/` et
`runs/optimized/` les entrées et sorties distinctes de chaque passage.
Le collecteur [short_adverses.py](short_adverses.py) termine code 0 lorsque
la collecte réussit : ce code ne certifie pas l'absence des défauts décrits.
`SHA256SUMS` ferme ce paquet. Aucun résultat de clustering, de croissance,
FULL ou de performance G4 n'est qualifié.
