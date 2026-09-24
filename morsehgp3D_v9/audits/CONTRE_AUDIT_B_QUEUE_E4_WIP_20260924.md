# Contre-audit B — queue FULL E4 pipelinée, WIP

24 septembre 2026. Lecture sans build ni modification du commit local
propre `2ff38b10a4f9fce4f3f8c3e7e6d551f8bec4af6c`, dérivé de
`d1d038393`. `full_ball_tower.hpp` SHA-256
`831917daa6a8a970cf4f060647f6f7f47f8c67a34c81cf42ce95925673c13405` ;
sonde `tower_probe.cpp` SHA-256
`56ae6891a5dc62804a298038cd198b7fd5fa203f84f625ddd61936e7aa52f7fc`.
Ce WIP n'est pas un reçu G4 E4 et ne démontre aucun gain de temps.

E4 démarre les populations et images verticales de chaque ordre K dès
que leurs dépendances de phase A sont prêtes, au lieu de tout laisser
après la jonction des ordres. Les IDs de populations sont attribués par
premier ordre contributeur et préfixes ; les références à une population
née à K inférieur sont nommées après la jonction. Les gates de petits
catalogues comparent ces sorties à la voie témoin. La lecture des
durées de vie ne montre pas de course causale : la phase B modifie les
références de contributions, C lit les parents et écrit les images ;
les lignes de banque sont distinctes, et tous les fils sont joints
avant publication. Cela ne remplace pas TSan ni une comparaison FULL
littérale sur des trames entières.

## Mesurer le coût complet, pas la disparition d'une colonne

`compute_population_offsets()` parcourt toutes les boules, y compris
les rangs des coquilles étendues, pendant `validate_catalogue()`
(`full_ball_tower.hpp:1210–1248,1752`) **avant** la fenêtre E4. Son
coût est dans `validate_parts[7]`/`validate_ms`, pas dans le nouveau
`populations_ms`. Un recul de cette colonne peut donc être un simple
déplacement de travail. Comparer en paire `validate_ms`, FULL total,
chaîne totale, CPU·s et même tour explicite. À titre d'échelle, R20
08/000000/K5/W48 donnait FULL 421,527 ms, dont validation 70,516,
populations 19,571 et images 26,862 ms : ces deux dernières colonnes
ne pèsent ensemble qu'environ 46,4 ms du reçu antérieur.

La sonde E4 `tower_probe.cpp` ne permet pas encore de choisir ou de
rapporter `tower_pipelined_tail` dans ses leviers/options et n'exporte
pas `populations_by_k`, `images_own_by_k` ni les compteurs du chemin.
Elle garde le schéma `mhgp9_tower_probe_v25`, alors que
`populations_ms` signifie désormais la queue **exposée** après la
dernière phase A, et `images_by_k` seulement la portion après la
dernière population, non la durée entière de chaque ordre. Les anciens
et nouveaux champs ne sont donc pas directement comparables. Ajouter
un bras témoin sélectionnable, les sous-chronos propres, le mode dans
le reçu et une version/description de schéma avant une campagne G4 E4.

## Ressources et comptabilité

Chaque runner qui a fini A ouvre un helper B ; ce helper peut ouvrir
jusqu'à W fils pour les lignes de banque pendant que les autres
runners et la phase statique utilisent eux aussi W. A et B ne se
recouvrent pas dans **le même** ordre, mais la borne par le code atteint
environ **299 fils présents à K5/W48**, **549 à K10/W48** si les
charges suffisent, sans plafond global W. Ce ne sont pas des pics
observés. Les gates E4 ne dépassent pas W8 ; mesurer pic de fils,
échecs de ressource et temps CPU/mur sur trame entière avant de
conclure à un gain de parallélisme.

La totalité du tableau de lignes de populations est allouée par K1
avant sa phase A, avec les autres ordres et la phase statique vivants ;
la voie témoin l'alloue après leur jonction. Mesurer le pic RSS, pas
seulement le volume final. `pipelined_orders` est incrémenté de Kmax
même si une panne empêche des phases B/C : il compte une **voie choisie**,
pas des ordres entièrement pipelinés. Renommer/documenter ou compter
les étapes effectivement accomplies, avant de l'utiliser comme plancher
de gate.

E4 est un troisième frère des WIP [E2 pool](CONTRE_AUDIT_B_POOL_E2_WIP_20260924.md)
et [groupement haché](CONTRE_AUDIT_B_GROUP_HASH_WIP_20260924.md).
Il ajoute lui aussi un booléen après `overlap_static` dans l'API FULL.
L'intégration des trois doit utiliser des options distinctes et un
gate croisé, sans réinterprétation silencieuse d'un argument positionnel.

GCP non utilisé dans ce contre-audit.
