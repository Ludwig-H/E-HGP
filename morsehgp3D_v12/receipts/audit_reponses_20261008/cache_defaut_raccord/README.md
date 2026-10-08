# Cache par défaut — raccord au bras mesuré, portée circonscrite

Lecture de `72f622a55` contre le source **957e9784f** de la session M. Les hashes
complets sont dans `capture.json`. Le seul changement exécutable de la sonde,
après retrait des commentaires, est `Options::cache = 0` → `u64{8} << 30`.
`--cache=0` reste reconnu ; le budget est toujours créé par
`MemoryBudget(o.budget,o.cache)` (`full_probe.cpp:91,159,440`). La bibliothèque
`MemoryBudget(limit)` garde son défaut sans cache : cette bascule est celle de
la **sonde FULL**, pas une modification de tous les appelants de l'API.

Les seuls fichiers moteur/sonde modifiés entre ces deux pins sont `full_probe.cpp`
et `pipeline_run.cpp` (correctif de terminaison [déjà relu](../a_terminaison_raccord/README.md)).
Le cache, les lecteurs et les pilotes sont identiques. La route d'allocation du
nouveau défaut correspond au bras `--cache=8589934592` mesuré, avec cette différence
de terminaison explicitement conservée : aucun transfert d'identité ELF complète.

Le [critère apparié](../session_m_apparie/README.md) adopte ce bras à **K5, GPU,
W48, ng00–02**. Les 37 trames sont informatives. Le nouveau défaut s'applique aussi
à la voie CPU, à la voie séquentielle, à K10 et aux autres entrées : ces régimes
n'ont pas reçu une mesure appariée du cache dans M. En particulier, les temps
CPU K5 **382/324/386 ms** de [MES-FULL](../session_m_admission/README.md) restent
ceux du source957 **sans cache**, pas des chronos du nouveau défaut.

## Mémoire et portes

« Compte comme une réserve » est correct pour la **limite du budget** : sous
cache, `held` inclut blocs vivants et inactifs ; `hold()` impose `held <= limit`,
évince les inactifs avant refus et peut revenir à une allocation de taille exacte
(`buffer.cpp:164–219`). Les 8 Gio plafonnent les blocs inactifs gardés, sans les
allouer ni les réserver immédiatement. Un `--budget` inférieur reste possible.
`used/peak`, donc `pic_octets`, excluent les blocs inactifs : ce pic ne mesure pas
toute la mémoire physique du cache. RSS cumulatif et capacités GPU restent séparés.

Le défaut active aussi la marge conservatrice de `MemoryBudget::admit`, inchangée
(`buffer.hpp:108–120`) : avec used0, limite1 000 000 et demande900 000, l'admission
passe sans cache et refuse avec cache (marge112 500). Exemple arithmétique, aucune
allocation exécutée ; ce n'est ni un dépassement de limite ni une régression
observée. L'identité des refus sous budgets finis ne découle pas de l'adoption
sur les trois ng au budget illimité.

Les portes FULL déjà enregistrées comparent FUL1 à `tower_chain` et jouent des
trames synthétiques de 600/500 points à K3/W3, sept passes, avec schémas par défaut,
séquentiel et recouvert (`full_probe_check.py:160–190`, `tests.cmake:169–185`).
Le script est inchangé ; il ne contrôle ni les hits du cache ni un gain de temps.
Leur exécution locale annoncée par le constructeur n'est pas requalifiée ici :
aucun journal primaire de ce nouveau lot n'est annexé, aucun natif relancé.

## Prochaine comparaison : options explicites

Rejouer sur72 le plan ancien `ref=`, `aa=`, `cache=--cache=8589934592` donnerait
désormais **trois configurations cache8 Gio**, contrairement à son effet sur957.
Pour une ablation du cache, annoncer `ref=--cache=0`, `aa=--cache=0` et
`cache=--cache=8589934592`. Pour A/S, fixer la même valeur de cache dans les deux
bras ; le `seq` historique de M était sans cache. Aucun plan en cours n'est modifié.

La docstring du pilote livré dit encore « vide : […] sans cache » (ligne12).
Le petit `documentation.patch` proposé la raccorde au nouveau défaut, sans
modifier lecteur, seuil, sonde ou reçu historique. `check.py --repo DEPOT`, normal
et `-O`, vérifie les pins, l'unicité du changement exécutable et l'exemple de budget.
