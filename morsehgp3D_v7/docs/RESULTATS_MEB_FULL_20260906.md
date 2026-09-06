# Raccord MEB au Builder FULL — qualification du 6 septembre

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. GCP non utilisé.

Le [raccord C++](CONTRAT_MEB_FULL.md) est un opt-in partagé par tous les
appels MEB d'un ordre. P=0 reste le défaut, F reste inchangé. Les cinq
diagnostics séparent les essais proposés, pivots, certificats, replis et
formes réellement exécutées dans F. Aucun catalogue ou mémo géométrique
global n'est ajouté. Le nombre d'appels FULL et son plafond sont inchangés.

## Sources et séparation des autorités

| Source | SHA-256 |
| --- | --- |
| `src/forest/full_gabriel.hpp` | `a946e31dde8fbd8ec528d6f5e94f9c727998acc172b4dd29c084dd522c730d1d` |
| `src/forest/meb_proposal.hpp` | `f922544b5cfdc214de96ecd49520e318ea8632d14a8142ef21fd248f9cc38fb3` |
| F inchangé, `silent_incidence.hpp` | `f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76` |

Les deux premiers headers sont qualifiés sur leurs propres captures ;
la qualification privée `484a89bc` n'est pas transférée. Le succès FULL
reste relatif à des catalogues complets, exacts et réguliers fournis.
Les fixtures bornées vérifient ces catalogues par l'oracle Gamma ; elles
ne qualifient pas inconditionnellement le producteur à grande échelle.
Les scripts et captures sont dans le [paquet du raccord](../receipts/full_meb_product_20260906/README.md).

Les moteurs sont tous clos, au plus tard le 6 septembre à **09:36:43 UTC**.
R3 passe **30/30 CTests Release et 30/30 ASan/UBSan**, LeakSanitizer actif,
avec 120 pins source inchangés. Les reçus complémentaires ferment 21
commandes locales/rationnelles et 15 commandes de mutations/injections.

## Composition et budgets

La suite CMake contient 30 portes : les 20 antérieures et dix nouvelles.
Les huit binaires produits n'ont pas `MHGP7_TESTING` ; les deux anciens
différentiels singleton/successeurs gardent leur macro de test explicite.
Les options strictes sont C++20 et `-Wall -Wextra -Wpedantic -Werror`.

La nouvelle composition couvre 21 nuages avec s=8/10/12, 93 ordres,
eager et lazy C=0/1/1M, à P=0/1/3/10M : **1 488 sorties et 33 792 coupes
Gamma par mode de la porte**, 1 116 comparaisons au chemin P0. La porte
rejets refait ce même corpus, pas un second ensemble de nuages distincts.
Chaque comparaison conserve la forêt littérale et les 33 anciens compteurs ;
les cinq nouveaux diagnostics ont leurs juges propres. Le corpus exerce
42 cas avec budget P partagé, certification puis repli, ainsi que K=1,
K=n, chaîne à plusieurs pas, cache saturé et identités de minima distinctes.

Les cinq plafonds appels MEB, ordinal L, chaîne, successeurs et requêtes
spatiales ont chacun 32 témoins au coût exact et 32 refus à coût−1.
120 refus conservent un budget proposé déjà dépensé. Les 28 rejets de
métadonnées restent précoces. Aucun refus P supplémentaire n'est créé.

Les balayages P=0/1/1M injectent chacun 49 fautes eager et 209 lazy :
**774 fautes par build**, sans échappement. Les refus tardifs conservent
les miroirs et purgent la sortie. Chaque tentative et reprise garde ses
entrées immuables ; le balayage lazy compare les 38 compteurs complets.

## Géométrie locale et injections

Les nouvelles portes locales de produit exercent 109 appels positifs ou
géométriquement refusés, puis 45 frontières budgétaires et trois injections.
Elles utilisent des Builders F persistants, comparent 13 champs F, le
`LocalBall` littéral et les cinq champs Work. Une injection avant forme
contrôle p prospectif ; deux injections après un vrai retour F contrôlent
A, dont une près de MAX. Elles ne prétendent pas interrompre l'arithmétique F.

Les portes privées adaptées compilent elles aussi **le nouveau header
produit**, pas l'ancien prototype : 9 344 comparaisons F/Trace/NoObserver,
59 frontières, 1 507 ordinaux, puis 3 430 appels jugés rationnellement par
build O2/ASan-UBSan. Les nouveaux contrôles A portent sur chaque appel
des deux gates adaptées ; le protocole rationnel conserve ses quatre
anciens champs Work et n'est pas présenté comme un juge de A.
Coquilles, représentations q4 brutes, rang 550 près de MAX et transitions
budgétaires restent exercés. Ces corpus se recouvrent.

Quatre copies O2 physiquement fautives sont réfutées avec code 1 et cause
précise : charge P après forme, miroir A supprimé, Work remis à zéro à
chaque appel et miroir Work FULL supprimé. Elles ne sont pas des options
compilées dans le produit. Les deux dernières sont déjà réfutées par le
préfixe P0 où A doit égaler le cumul de référence.

Une cinquième copie change seulement le callback `NoObserver.before_form`,
qui perd `noexcept` pour une injection explicite après deux certificats.
Douze cas par build O2/SAN vérifient `bad_alloc`, `length_error` et
`runtime_error`, sur wrappers eager/lazy et Builder à résultat observable :
quatre refus publics, deux propagations runtime, six propagations Builder,
dix miroirs, huit comparaisons de préfixes et six reprises. L'appel FULL
interrompu est payé tandis que l'appel géométrique n'est pas encore compté.
Le `NoObserver` nominal ne lève aucune exception ; ce témoin ne lui en invente pas.

## Incidents conservés et limites

La première configuration échoue parce que Boost n'est pas dans le chemin
système ; la reprise utilise la copie locale déjà disponible, sans installation.
R2 passe 30/30 en Release, puis son compilateur SAN atteint la limite de
fichier temporaire de 64 Mio. Ce n'est pas un échec scientifique ou une
exécution SAN refusée. R3 repart de builds neufs avec une borne de 512 Mio
pour la compilation, 64 Mio pour les exécutions ; les sources et plafonds
HGP restent identiques. Les deux tentatives sont conservées sans réétiquette.

Les temps de compilation et CTest ne sont pas des mesures de performance.
Aucun 8k/16k/32k n'est réattribué au nouveau code, ni aucun gain de tour
déduit des formes supprimées. Les contrats 50k/tour 1..10 sous 1 s,
repli tour 1..5, puis 100 ms et dizaines de millions sur G4 restent ouverts.
La suite exige une sonde qui publie P, `meb_accounting` et les cinq coûts,
puis des mesures appariées. Le mémo de terminaison, l'archive FULL et la
verticale restent des chantiers distincts. Le registre formel est inchangé.
