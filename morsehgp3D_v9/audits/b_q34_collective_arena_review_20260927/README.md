# Contrelecture ciblée de l'arène collective q3/q4

27 septembre 2026. Lecture indépendante des sources gelées
[arena.hpp](../b_q34_collective_arena_20260927/arena.hpp),
[probe.cpp](../b_q34_collective_arena_20260927/probe.cpp),
[run.py](../b_q34_collective_arena_20260927/run.py) et
[check_capture.py](../b_q34_collective_arena_20260927/check_capture.py).
Complète le [contrat de raccord GPU](../b_q34_arena_review_20260927/README.md),
sans le modifier. Aucun moteur, source gelée, reçu ou build changé ;
aucun exécutable de cette arène relancé par cet audit. GCP non utilisé.

## Résultat

Aucun défaut concret d'offset, de propriété ou de course n'a été identifié
par lecture. **Un défaut de couverture du gate a été trouvé et signalé** :
le négatif censé exercer les ordinaux non croissants est masqué par un
chevauchement antérieur. Le développeur prépare un complément séparé ;
cette note ne lui attribue aucun succès tant que son reçu n'est pas clos.

La qualification en cours n'est pas une preuve de tour FULL ni de raccord
GPU. Les sources constituent une préparation collective de facteurs et
bandes, avec référence directe vérifiée. La clôture Release/sanitizer et
les chiffres de performance relèvent du reçu du développeur, non de cette
lecture de code.

## Offsets, tailles et allocations

Les cumuls collectifs `F`, `C`, `D`, le nombre de tâches et les masses
passent par `counter_add`. Ce helper, dans `gen/core/types.hpp`, **lève**
une exception sur dépassement u64 ; il ne sature pas silencieusement.
Les allocations à partir de ces cumuls passent par `checked_size`.
Tailles locales et compte de bandes sont contrôlés avant conversion u32.

Le préfixe local des classes, les histogrammes et les préfixes q3/q4 sont
en u32. Leurs sommes sont des cardinalités de sous-ensembles disjoints du
même facteur, donc au plus sa taille déjà validée u32. Atteindre exactement
UINT32_MAX n'impose pas de représenter UINT32_MAX+1 dans ces champs.
Les tableaux de crédits ont des domaines bornés par K≤10 : q3≤9, q4≤8,
donc les indices `10*q3+q4` restent dans les 100 cases et les indices de
préfixes dans les 11 cases. Les crédits sont produits en interne, non
acceptés comme un tableau arbitraire de l'appelant.

Les tâches d'ancres calculent `first+grain` en u64 avant de borner et
convertir ; chacun des deux termes est au plus u32. Chaque tâche écrit
un intervalle valide du facteur. Les accès globaux restent dans les
préfixes F/C/D clôturés. Le nombre de facteurs `2*requests.size()` est
contrôlé avant multiplication ; le tableau de préfixes des pools ajoute
une entrée sans débordement après ce contrôle et l'allocation des facteurs.

Les tailles de capacité rapportées ne sont **pas un RSS** ni une garantie
de réussite d'allocation. Elles additionnent les tableaux possédés et
scratch présents aux phases comptabilisées. Les piles, l'allocateur,
l'infrastructure de threads et le propriétaire nuage/index sont hors de
ce chiffre ; le nuage/index et les requêtes appelantes sont publiés à part
dans la sonde. L'absence de tableaux P/E globaux dans ce constructeur a
été vérifiée, mais le futur consommateur doit conserver cette propriété.

Pas de test d'injection d'échec d'allocation dans cette tranche. Une
exception d'allocation détruit le propriétaire encore privé, sans sortie
partiellement publiée ; sa chronologie précise n'est pas qualifiée.

## Écritures concurrentes et exceptions

Le constructeur copie d'abord les requêtes et retient le pointeur partagé
du même index immuable. Aucun vecteur externe déplacé ne devient stockage
certifié. Le propriétaire est non copiable/non déplaçable et ses accès
publics sont des vues const. Aucun pointeur de travail n'est publié avant
la fin du constructeur.

Les plages d'écriture sont disjointes à chaque phase :

- sélection : intervalle de pool propre à chaque facteur ;
- certification : jobs partitionnant les ancres, un octet de crédit par
  ancre et aucun second census géométrique ;
- classes : un worker par facteur, puis préfixe global après jointure ;
- scatter stable : plages de classes, rangs et crédits propres au facteur ;
- bandes : un worker par rectangle, préfixe puis plage propre d'émission.

Les compteurs privés sont indexés par l'identité fixe du worker, non par
un slot qui pourrait être simultanément repris. Les phases joignent leurs
threads avant la suivante et avant toute lecture de leurs résultats.
Des octets adjacents de `vector<uint8_t>` restent des objets distincts ;
ce n'est pas un stockage `vector<bool>` à mots partagés.

La première exception capturée est protégée par mutex ; `stop` atomique
interrompt les prises de nouvelles tâches. Des tâches déjà prises peuvent
finir, mais tous les threads sont joints avant propagation. Une erreur
pendant la création des threads arrête puis joint les threads déjà créés.
Dans tous ces cas, le propriétaire ne s'échappe pas.

Le gate compare réellement 1/4 workers et grains 7/1, y compris tous les
tableaux finaux et le travail géométrique. Il injecte une exception dans
un job puis vérifie son motif. Cela n'est pas une porte TSan et ne prouve
pas que deux workers aient travaillé simultanément sur un facteur donné ;
aucune qualification par détecteur de courses ne doit en être déduite.

## Défaut trouvé dans le négatif d'ordinal

Dans `arena_gate`, chaque négatif commence par
`bad = {{0,0,0,6}}`. Pour `failure==8`, une copie du même rectangle est
ajoutée pour tester les ordinaux répétés. Or le **premier** rectangle
utilise deux fois le nœud 0 : la vérification des facteurs disjoints lève
avant de visiter l'ordinal du second rectangle. Le test exige seulement
`invalid_argument`, pas le motif, et passe donc sans exercer ce qu'il
semble tester.

Les 72 refus annoncés sont 72 appels effectivement refusés ; ils ne
constituent pas neuf branches de refus indépendamment démontrées. Les
autres cas configuration, nœud hors domaine et masque invalide sont
contrôlés avant ce chevauchement et ne souffrent pas du même masquage.

Correction de preuve demandée sans retoucher les captures gelées : partir
d'un rectangle WSPD valide, vérifier qu'il est accepté, puis ajouter une
seconde requête elle aussi valide avec ordinal identique ou décroissant.
Exiger précisément `arena.request`. Un complément source/build/reçu neuf
permet de fermer ce point sans effacer le premier essai.

Limites voisines : le gate ne revendique pas une injection couvrant chaque
allocation, le débordement d'un facteur nécessitant plus de 2^32 sites,
ni toutes les combinaisons de requêtes publiques invalides. Les ordinaux
croissants ne prouvent de toute façon ni la couverture ni la disjonction
globale d'une WSPD ; ces propriétés restent celles du producteur.

## Relecture du runner

Le runner utilise un collecteur épinglé et exige capture close, recette
complète, codes de sortie, groupes de processus clos, commandes/intentions
liées et hashes des flux. Les sources, archives liées et entrées sont
rehachées avant/après ; le lecteur LIVE rejoue ces contrôles et les pins
de binaires/options CMake. Les timings sont finis, non négatifs et les
principales phases sont additionnées dans le temps total. Les compteurs
vérifient explicitement trois lectures de crédits `3F`, une écriture `F`
et deux regroupements de bandes par rectangle préparé.

Le lecteur exige les deux gates égaux et contrôle les motifs exacts des
mutants scatter/masque. Les mesures sont appariées 1/4 puis 4/1 ; elles
gardent une comparaison complète et un contrôle indépendant contre le
plan direct hors des chronos de préparation. Ce dernier est une référence
de représentation réutilisant les mêmes primitives géométriques, pas un
nouvel oracle arithmétique.

Le petit adaptateur `check_capture.py` reprend un harness haché et vérifie
ses propres hashes de fermeture. Les huit corruptions de reçus portent
sur recette, provenance, fermeture et statut GPU, pas sur l'exhaustivité
du gate C++. Elles ne corrigent donc pas le négatif d'ordinal décrit plus
haut. Le lecteur LIVE dépend des sources, builds et entrées non archivées
autonomement. Aucun PASS de lecture de la capture arène encore en cours
n'est ajouté par la présente contrelecture.
