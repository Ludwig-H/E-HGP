# Cache de témoins S2 par tuiles — plan G4 du 26 septembre 2026

**Campagne maintenant close** : [reçu brut](../../receipts/g4_tile_cache_20260926/README.md),
[analyse et décisions](../AUDIT_B_CACHE_S2_G4_20260926.md). 14 cas,
42 passages, réponses identiques, gain net du filtre d'environ 1–4 ms.
G4 SPOT arrêtée et `TERMINATED` relu. Le plan ci-dessous est celui fixé
avant la mesure ; il n'est plus une campagne en attente.

Ce plan prépare une expérience du filtre de paires q3/q4. Il ne mesure ni
le générateur complet, ni le catalogue, ni la tour FULL. Aucun gain GPU ne
résulte du seul ajout du plan. Le cache reste opt-in (`--tile-cache`).

La question est simple : peut-on réutiliser les témoins trouvés pour une
paire auprès de ses voisines, sans refaire leur recherche globale ? Les
témoins sont **retestés exactement** pour chaque voisine ; un cache
insuffisant déclenche la recherche globale habituelle. Les petits facteurs
B, de taille inférieure à 16, gardent directement le chemin de référence.
Pour les autres, une tuile contient au plus 32 colonnes et un représentant.
Ces constantes ne sont pas des plafonds de recherche ou de sortie.

## Population et ordre fixés avant mesure

Les entrées sont les trois masques sans sol entiers historiques de
08/000000, 08/000100 et 08/000200, en grille entière 1 mm. Les identifiants
de protocole `00`, `01`, `02` ne désignent pas trois séquences SemanticKITTI.
Leurs tailles et hashes restent ceux du registre des entrées du protocole.
Ni préfixe ni sous-échantillonnage ne sont admis.

`plan.json` contient 14 processus de sonde, chacun avec trois passages
GPU publiés séparément :

- 00 / K5 / s8 : ordre référence, cache, cache, référence (ABBA) ;
- 00 / K10 / s8, 01 / K5 / s8 et 02 / K5 / s8 : référence puis cache ;
- 00 / K5 / s10 et s12 : référence puis cache.

Seul 00/K5/s8 a deux répétitions indépendantes par bras. Pour les autres,
trois passages internes ne remplacent pas trois processus indépendants.
Rapporter tous les temps, le premier passage et leur dispersion, pas
seulement le meilleur. La comparaison s8/s10/s12 porte sur la vraie WSPD.

## Ce que le juge exige

La sonde v3 conserve le calcul CPU exhaustif de référence et compare chaque
masque GPU à son masque CPU. Le cache de ligne CPU doit lui aussi être
identique. Le préflight exécute référence et cache, chacun avec un mutant
qui inverse exactement un masque de paire ; les deux mutants doivent être
détectés avec code 1. Les 14 cas réels gardent cette référence CPU.

Sans cache GPU, les comptes de visites doivent rester identiques au CPU.
Avec cache, ils peuvent changer : `visits_equal` rend l'égalité numérique
réelle, jamais une réussite géométrique fictive. Les visites des rectangles
restent identiques. Tous les masques de tous les passages et leurs comptes
de travail doivent rester identiques d'un passage à l'autre
(`repeat_mismatches=0`). Une expérience entière du cache doit avoir au
moins une tuile ; le petit préflight peut légitimement n'en avoir aucune.

Les compteurs v3 séparent :

- `tiles = representatives` : un représentant par tuile de facteurs B
  d'au moins 16 sites ;
- `trace_node_tests` : visites DFS des représentants ;
- `cache_node_tests` : tests géométriques des nœuds proposés par le cache ;
- `pair_visits` : visites globales, **représentants inclus**, puis replis et
  petites paires hors cache. Ne pas lui ajouter `trace_node_tests`.

Pour comparer le travail géométrique total aux visites de référence,
compter `pair_visits + cache_node_tests`, tout en publiant les deux termes.
Une baisse des seules visites globales ne suffit pas à déclarer un gain.

`gpu.passes` publie chaque passage : upload, rectangles, scan/préparation,
paires, download et total. `pair_ms` inclut production des traces et
application du cache. `scan_ms` inclut aussi la préparation/allocations
des préfixes de tuiles. Le temps synthétique hérité est un des passages de
total minimal ; il ne peut assembler les meilleurs sous-temps de passages
différents. Les transferts restent inclus selon le périmètre existant de
la sonde, sans le convertir en temps mur de processus ou en chrono FULL.

## Protocole, budget et fermeture

Plan v2 et sonde v3 sont stricts. Les plans v1 et sorties v2 sans cache
restent lisibles dans leur ancien domaine. Une source v2 ne peut pas
qualifier un plan contenant `tile_cache=true`. Le snapshot réel est issu
d'un commit : protocole non committé interdit à l'exécution GCP.

Utiliser exclusivement `gpu_filter_snapshot_v9.py`, puis
`gpu_filter_session_v9.py` avec ses pins de snapshot, manifeste, worker et
contrôleur. Aucun démarrage ici : la cible SPOT G4 fixe, les deux
coupe-circuits, le budget utile de 1 500 s, la limite de cas de 600 s et
l'arrêt ciblé dans `finally` restent ceux du protocole existant. La
génération exacte doit être certifiée arrêtée ; publier son état final.

Les tests hors ligne `gpu_filter_selftest_v9.py` tournent en Python normal
et `-O` avant la campagne. Ils doivent couvrir succès référence/cache,
refus de schéma/options/compteurs/temps incohérents, lecture historique,
mutants, fermeture après panne et détection des captures modifiées.

## Publication de la capture close

`publish_closed.py` ne fait aucun appel GCP. Il exige le reçu hôte final
`targeted_shutdown_certified=true`, une réception `completed` rejugée et
une relecture GCE `TERMINATED` de la même génération. Il copie les seuls
octets de `received/output` vers `vm/`, vérifie leurs empreintes, puis
publie une liste fermée de preuves hôte. Les sorties des gardes sont
explicitement expurgées des comptes et clés publiques ; leurs empreintes
originales et publiées restent distinctes. Le répertoire parent contenant
la clé privée n'est jamais parcouru. Le paquet binaire n'est pas publié :
il contient des coordonnées LiDAR et la v9 n'accueille pas de nouveaux
octets KITTI.

Exécution seulement **après** fermeture et relecture de la cible :

```bash
python3 -B morsehgp3D_v9/audits/b_g4_tile_cache_20260926/publish_closed.py \
  --host /chemin/session/gpu_filter_v9_host \
  --package /chemin/paquet/PACKAGE.json \
  --after-stop /chemin/relecture_gce_apres_arret.json \
  --output morsehgp3D_v9/receipts/g4_tile_cache_20260926
```

`PACKAGE.json`, le manifeste, le plan et l'empreinte du snapshot permettent
de reconstruire celui-ci depuis son commit d'origine. Le lecteur
`analyze_receipt.py <reçu> --snapshot <paquet-local>/snapshot.tar.gz`
rejoue le validateur intégral épinglé et recalcule les tableaux depuis les
sorties brutes. Le snapshot original est requis : pour le dossier publié,
son absence entraîne un refus, pas une relecture de portée réduite.
Le lecteur annonce séparément si les journaux bruts d'arrêt sont présents
et rehachés ; la publication n'en donne que les copies expurgées.
`sha256sum --check SHA256SUMS`, exécuté dans le dossier du reçu, contrôle
l'inventaire publié. Celui-ci couvre tous les fichiers à sa création :
tout ajout ultérieur requiert sa fermeture à nouveau, sans réécrire les
sorties brutes.

Le helper de publication possède quatre tests locaux, passés en Python
normal et `-O` : refus d'un reçu non clos, refus du répertoire parent de
session, expurgation des comptes et clés publiques sans toucher à la
génération ni à l'état final, refus d'un marqueur de clé privée. Ce ne
sont ni des opérations GCP ni des mesures supplémentaires.

Incident préalable déclaré par l'opérateur : le premier lancement du
contrôleur a été refusé pour une clé privée de mode `0644`, corrigé en
`0600`. Ce contrôle a précédé tout appel GCP ; ce n'est ni une panne du
benchmark ni une mesure GPU.
