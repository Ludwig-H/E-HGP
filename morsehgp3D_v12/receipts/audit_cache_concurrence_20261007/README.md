# Cache : correction de la course d'éviction et causalité du test

7 octobre 2026. Contre-lecture d'un **nouveau correctif développeur non commis**, épinglé par
[capture.json](capture.json). `buffer.cpp` :
`1844a7d6e6d063347ace171be4ca9899669b08b30b2bf2106fa73d9b096c47cd` ;
`buffer.hpp` inchangé : `3c7fffc6ed97b6456f666ef849ddd6f4f3a1b3f2427c0fc875b7d5edcd14130b`.
Cadre `exploration_v12_hors_registre`, `cpu_reference`, `not_claimed`. Aucun GCP, sanitizer, moteur ou matrice.

**La course d'éviction reproduite est corrigée. La nouvelle porte développeur doit cependant changer de second
tampon pour tuer son mutant de relecture.** Aucun ancien reçu n'est réécrit.

## Six petites exécutions causales

Même budget/cache de 1 Mio, même inactif de 1 Mio à évincer, étage admis avant les demandes. Le premier tampon demande
300 Kio. Le second demande soit 300 Kio (classe du cache), soit 200 Kio (taille sous le seuil du cache).
`operator delete` du bloc évincé est ralenti ; le contrôleur le libère après le départ du second fil et une fenêtre
de 100 ms. Il n'appelle pas `cache_stats` pendant cette attente : la nouvelle version protège cette lecture par le
même mutex, ce qui bloquerait le contrôleur du témoin historique. Aucun échec d'allocation système injecté.

| Source | Second tampon 300 Kio | Second tampon 200 Kio |
| --- | --- | --- |
| Ancienne capture `fb3c5239…` | refus `memory_budget` pendant éviction | refus `memory_budget` pendant éviction |
| Correctif `1844a7d6…` | succès après éviction | succès après éviction |
| Correctif, seule relecture finale de `held` supprimée | **succès : mutant survivant** | **refus : mutant tué** |

Dans les six cas, premier tampon et admission réussissent ; `used == held <= 1048576` à la jointure et une seule
éviction est comptée. Au succès, les deux coexistences valent respectivement 630784 et 520192 octets.
[Résultats natifs](result.json). Les observations de fin avant/après libération figurent aussi dans la capture.

## Pourquoi la correction fonctionne sur cette course

- `cache_pop` retire l'inactif **et** ajoute sa capacité physique à `used` sous le verrou.
- `release_class` retire la capacité de `used` puis publie l'inactif ou termine sa restitution système sous le
  verrou : la fenêtre précédente « plus vivant, pas encore dans la liste » n'est plus visible aux décisions de
  cache prises sous ce même verrou. L'empoisonnement commence avant, alors que le bloc reste encore compté vivant.
- `evict_locked` conserve le verrou de son retrait de liste jusqu'au vrai `delete`, puis à la soustraction de
  `held`. Le plafond physique ne se libère donc pas avant la restitution effective.
- Faute de place sur la première lecture, `hold` acquiert ce verrou, tente l'éviction et **relit** `held` avant de
  refuser. Une lecture ancienne de cache plein ne suffit plus pour refuser après l'éviction terminée d'un autre fil.

Les blocs exacts et `BudgetReservation` gardent leurs transitions atomiques hors de ce verrou. Ils appartiennent aux
tampons déjà comptés au début de l'étage ou à sa somme d'allocations admise ; cette somme et sa marge restent une
borne conservatrice tant que le contrat « un pilote, admission avant tâches et aucune allocation étrangère » est
respecté. La présente contre-épreuve ne prétend pas qualifier une admission lancée au milieu d'un autre étage.

Dans les chemins ordinaires relus, aucun verrou du cache n'est acquis pendant que ce fichier détient un verrou de
l'allocateur : allocation neuve hors verrou du cache, libération désormais sous verrou. Cela ne constitue pas une
preuve pour un remplacement arbitraire de `operator delete` qui réentrerait dans ce même budget ; nos remplacements
de test ne le font pas. Aucun test TSan ou qualification générale des allocateurs n'est acquis.

## Porte développeur à rectifier

Les fichiers `alloc_fault.cpp`, `tests.cmake` et `mutants/core.json` sont lus aux hashes de la capture, sans copier
leur contenu. La porte `eviction_admise` demande **deux tampons de 300 Kio** et le mutant `refus_sans_relecture`
remplace la relecture finale par `return false`.

Or, pour 300 Kio, le second fil entre d'abord dans `cache_pop` : il attend déjà le mutex de l'éviction, avant sa
première lecture de `held` dans `hold`. Après cette attente, il voit la place disponible ; supprimer la relecture
finale n'affecte donc pas ce parcours. Notre mutant conserve effectivement le succès sur ce bras.

**Correction ciblée du témoin : second tampon de 200 Kio**, avec somme admise et compte attendu adaptés, ou seconde
demande par `BudgetReservation`. Cette demande saute `cache_pop`, lit `held` encore plein, puis attend le verrou :
la relecture finale devient causale. Le bras de 200 Kio tue exactement ce mutant ici.
Les compteurs globaux du remplacement d'allocateur dans la capture développeur sont déjà atomiques ; aucune nouvelle
course n'est alléguée sur ces compteurs.

## Rejeu et portée

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_cache_concurrence_20261007/check.py
```

[check.py](check.py) reconstruit `core` depuis `f601b36ac`, applique l'ancienne capture publiée au commit `c9356b5b8`,
puis le seul [delta de correction](delta.patch), le tout dans `/tmp`. Les hashes ancien et nouveaux sont vérifiés
avant compilation. Trois binaires ciblés GCC 13.3, C++20, `-O2`, u21 ; deux bras par binaire, durée bornée par le
lecteur. Le seul mutant supprimant la relecture est construit dans une copie temporaire.

**Clôture technique proposée du résidu d'éviction `CST-0007/0019` sur ces sources**, avec preuve indépendante des
deux bras. La nouvelle porte développeur reste à rendre causale avant d'en revendiquer le mutant tué. Aucune
qualification de performance ni extension implicite de la qualification ASan de la capture précédente.
