# A/w902 : fins de calcul, disponibilité et fenêtres de feuilles

Lecture du prototype A, base déclarée `902041f66`, sans modification du produit, compilation,
moteur ni GCP. Les **131 fichiers `src/` sont identiques à A/w8da** : ceci précise la portée
du nouveau schéma, sans présenter un ancien corps comme une régression récente. La
[capture](capture.json) épingle l'arbre et les sources ; les défauts d'admission déjà prouvés
restent dans [le reçu du schéma 902](../t2d_a_schema902/README.md).

## Fenêtre déplacée dans `foret_apres_g`

`pipeline_run.cpp:180–188` mesure une prépasse de feuilles sur ses propres instants
`[l0,l1]`. Dans `run_g:235–250`, G se termine à `t1≤l0`, puis le compteur reçoit
`charge_forest(t1,t1+(l1−l0))`. La durée totale est juste ; sa fenêtre est déplacée vers
le passé. Pour une fin globale G déjà publiée à `g`, la contribution annoncée est
`max(0,t1+d−max(t1,g))`, au lieu de `max(0,l1−max(l0,g))`.

Un ordonnancement autorisé suffit : fin de cette tâche G à 10, prépasse de 20 à 30,
autre tâche G terminée et publiée à 25. Le compteur ajoute **0**, l'intersection réelle
vaut **5**. Ce sont des unités abstraites, aucun chrono observé. Une préemption dans
l'intervalle entre G et feuilles rend ce décalage arbitrairement grand ; aucune borne
de petitesse ne découle du source. La traduction vers le passé ne peut qu'abaisser la
contribution, à longueur fixée. [check.py](check.py) vérifie cette propriété sur 4 536
combinaisons bornées et le témoin explicite, normal et −O.

Correctif ciblé proposé : transmettre les vrais début/fin de `slice_leaves` à la charge
de la prépasse, sans charger une seconde fois les feuilles exécutées dans le noyau.
Cela ne corrige pas la convention distincte, déjà documentée, qui classe avant G une
tâche finie avant la publication de sa fin globale. Le compteur conserve cette limite.
Ni l'objet FULL, ni `queue_ns`, ni le mur, ni la durée totale des feuilles/forêt ne sont
réfutés par ce témoin. Une proportion exacte de travail caché ne se déduit donc pas de
`foret_apres_g` actuel ; le ratio queue/ancien mur séquentiel ne l'établit pas non plus.

## Invariants utiles du schéma

Pour une sortie réussie, la ligne d'ordre `i` est `[G_i,N_i,M_i,V_i,R_i]`, où **N est la
fin du noyau, pas de tout T**. L'historique se calcule après N, parallèlement à la
contraction. Avec `o=ouverture_ns` et `f=fin_ns`, les dépendances donnent :

- `o≤G_i≤N_i≤M_i≤R_i≤f` ; `G_i≤fin_g_ns≤f`.
- `V_1=0` ; pour `i≥2`, `max(M_i,M_(i−1))≤V_i≤f`.
- Aucun ordre entre V et R, ni ordre croissant général des lignes selon i.

Preuve : les tranches ne publient `Done` qu'après leur éventuelle prépasse ; le noyau
consomme les tranches dans l'ordre et attend toutes ces publications. Les dates M/V/R
sont écrites avant de libérer leurs successeurs dans le graphe. R attend M et l'historique
du même ordre ; V attend M des deux ordres voisins et l'historique inférieur. Ces dates
marquent la disponibilité des résultats ; le dernier travail peut encore faire de la
comptabilité après leur publication. Ne pas sommer ces dates ou les fenêtres de tâches
comme des durées murales disjointes. `tables_ns` est inclus dans `ouverture_ns`.

## Mémoire et frontière de disponibilité

L'admission `region_bytes` additionne les besoins de tous les ordres et workers : aucune
borne « maximum d'un ordre » n'est revendiquée. Les tables G des ordres 2..K sont toutes
construites avant la région et restent possédées jusqu'à sa destruction. La fin G par
ordre ne signifie donc ni libération de cette table, ni feuilles disponibles. Le graphe
libère les événements dans `kRows`, après contraction **et** historique ; cette frontière
préserve les lecteurs parallèles. La présente lecture ne trouve pas de nouveau défaut
d'objet ou de durée de vie sur ces dépendances ; elle n'est pas une qualification de
concurrence ou du budget total. Une éventuelle libération anticipée des tables constitue
une autre modification à mesurer, pas un gain acquis du schéma.

À la capture du main indiqué dans le JSON, les deux fichiers de CPUlive sont encore
identiques au pin 8da : la [proposition CPUlive](../cpu_live/README.md) n'est pas adoptée.
Cette observation datée n'anticipe aucune intégration ultérieure.

```sh
python3 -B check.py --prototype /copie/A/w902/morsehgp3D_v12 --baseline /copie/A/w8da/morsehgp3D_v12
python3 -B -O check.py --prototype /copie/A/w902/morsehgp3D_v12 --baseline /copie/A/w8da/morsehgp3D_v12
```
