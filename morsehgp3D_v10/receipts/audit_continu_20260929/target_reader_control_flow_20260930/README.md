# Deux trous du validateur des fixtures — preuve causale bornée

Observation privée du 30 septembre 2026. Paquet autonome ; aucun fichier partagé modifié.

## Résultat

Les deux sources viennent de
`/workspaces/E-HGP/build/v10-verrou-points/fixtures_cibles/lib/`.
Leurs SHA avant copie à14:08:18 UTC et après les captures à14:14:35 UTC sont identiques.
Les copies `source/` sont exactement ces octets.

1. `valide_lib.main` publie `sources_stables=False` mais rend **code0** si ses tests passent.
   Il calcule les hashes après les tests sans transformer leur différence en échec.
   Contrôle causal : neuf tests unitaires stub positifs et deux empreintes synthétiques différentes.
   Contrôles : neuf tests positifs avec empreintes identiques rendent0 ; un test stub négatif rend1.

2. `run_target.normaliser` accepte une variante ayant explicitement `target=[]`.
   `juger_fixture` produit alors zéro cible et zéro verdict pour cette variante,
   mais `all([])` donne `passe=True`, jusque dans le résultat global.
   Contrôles : une cible positive passe ; une cible négative échoue ; la BASE avec
   `target=[]` est déjà refusée.

Ces résultats sont identiques en Python normal/−O, codes0 et stderr réellement vides.
Les 3431octets de stdout sont identiques. Une expérience initiale avant capture formelle
a aussi obtenu les mêmes observations ; seule la paire horodatée de `receipt.json`
est l'autorité de clôture.

## Portée exacte des stubs

Les fonctions de contrôle sont extraites par AST depuis les copies gelées.
**Aucun module original n'est importé.** Les tests v1..v9, le parseur des points,
Scene, la règle, la compatibilité FULL et le jugement mathématique sont remplacés
par des stubs explicitement déclarés. Les différences de hashes sont INJECTÉES
dans le sampler `empreintes` ; aucune source sur disque n'a été mutée.
Les sorties internes du validateur sont interceptées en mémoire.

Il s'agit de défauts de contrôle de provenance et de non-vacuité, pas d'un nouveau
contre-exemple géométrique, d'une invalidation des 389 contrôles antérieurs source-stables,
d'une victoire statistique ni d'une qualification FULL/G4/100ms.
Le code0 de run_target signifie officiellement que le juge a tourné, pas qu'une règle
gagne : le défaut est ici le champ `passe=True` d'une variante sans tests.
Les caches Gamma/natifs ne sont pas exercés ; aucun mauvais cas K en cache n'est affirmé.

## Gardes proposées, non implémentées

- Dans valide_lib après l'empreinte finale : si avant!=apres, passer au code3 et
  conserver la cause explicite, comme le fait déjà run_target.
- Dans normaliser, exiger pour chaque target de variante une liste non vide.
  Ajouter également une garde de non-vacuité dans juger_fixture, pour les appels directs.
- Pour une future campagne : publier les nombres de cibles/variantes réellement jugées
  et refuser les inventaires vides avant toute annonce de résultat.

## Préflights conservés

Le premier probe privé a oublié le global ALIAS utilisé par _cles : NameError/code1.
Sa source et ses diagnostics sont dans preflight/. L'heure UTC exacte n'était pas
capturée ; le reçu le dit explicitement, sans l'inventer.

Après ajout d'un ALIAS vide (les fixtures unitaires utilisent des clés canoniques),
le stub des points acceptait seulement dict, alors que normaliser appelle aussi
ce helper avec une liste : AttributeError/code1 normal et−O.
Les deux captures horodatées et la source sont conservées dans preflight2/.
La correction privée rend le stub compatible dict/liste. Aucun moteur n'est corrigé.

## Lecture portable

`python3 -B verify.py`, puis `python3 -B -O verify.py`.
Le lecteur vérifie l'inventaire et tous les SHA AVANT lecture des reçus et avant
rejeu des deux micro-sondes AST. Aucun build privé, bibliothèque tierce, binaire natif,
source originale ou accès réseau n'est nécessaire.
Ne pas rejouer record.py pour lire l'archive ; c'est la source historique de capture.
