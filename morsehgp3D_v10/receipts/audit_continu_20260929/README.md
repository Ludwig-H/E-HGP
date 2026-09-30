# Captures de l'audit continu v10

Sources, captures et contre-exemples du 29 septembre 2026, relocalisés depuis
`audits/audit_continu_20260929/` à la demande de rangement de l'utilisateur.
Les 34 fichiers déplacés sont conservés **octet pour octet** ; le
[manifest](RELOCALISATION.json) donne ancien chemin, nouveau chemin et SHA256.
Les rapports courants restent dans [audits/](../../audits/AUDIT_ETAT_COURANT.md).

Les commandes, diagnostics et chemins absolus inscrits dans les JSON décrivent
leur exécution historique ; ils ne sont pas réécrits pour paraître récents.
Cette relocalisation n'est ni un rejeu des campagnes ni une qualification
supplémentaire. Les essais interrompus restent identifiés comme tels.

- `catalogue/` : contre-oracle, feuilles, entrée tronquée, discontinuité cover.
- `pool_head/` : sécurité du pool, coût de la tête, verticales, multi-K et bande.
- `projection_band/` : petit oracle Fraction autonome, normal et `-O`.
- `timeout/` : enfant survivant au délai et réplique hôte CUDA historique.
- `RELECTURE_COORDINATION.json` : relecture fermée sur les chemins de l'époque.

Les scripts catalogue peuvent écrire des sorties : les rejouer uniquement
depuis une **copie dans un nouveau répertoire temporaire**, avec leurs chemins
de capture adaptés. Ne jamais lancer un script écrivain sur cette archive.
La profondeur relative au dossier v10 est inchangée ; les imports locaux
restent groupés. `timeout/reproduce.py` retrouve donc encore le même moteur.
Une nouvelle exécution doit produire un reçu distinct, avec sa propre version.

## Captures ajoutées après la relocalisation

- `math_catalogue_cover/` : oracle exact autonome de toutes les composantes
  couvrantes, K1/K2/K3, normal et `-O` ; `native_r2/` ajoute 64 appels sur
  huit petites géométries K1–K4 face à un nerf rationnel indépendant,
  sans nouvelle construction du moteur ni qualification globale.
- `thesis_boundary/` : provenance de la relecture des parties I et II et
  petit contre-exemple abstrait du vote recalculé à chaque coupe.
- `pool_corrected/` : refus réel de création partielle de fils sur copie
  corrigée ; `consumer_r2/` ajoute un rejeu indépendant des pannes catalogue
  et tour, ainsi que des logs terminés ASan/TSan observés du développeur.
  Les copies ne qualifient pas leur intégration.
- `interfaces_corrected/` : neuf appels courts, entrée par pipe complète,
  et trois options numériques silencieusement acceptées à tort sur copie.
- `majority_lamination/` : preuve et oracle abstrait de majorité fixe,
  180 cas ; `native_kmax_r2/` montre sur exports natifs le biais Kmax et
  son élimination par les témoins propres à K. La nouvelle tête n'est pas
  implémentée dans le produit ; aucune qualification statistique.
- `timeout/cuda_corrected/` : contrôle arithmétique hôte UBSan de la sonde
  unsigned ; aucun chrono ou reçu GPU nouveau.
- `oracles_corrected/` : mutants de dumps, dont une multifusion ternaire
  binarisée encore acceptée ; juges figés et rejeu autonome normal/−O.
  Les 24/24 gates du développeur sont observées, non relancées ici.
- `head_numeric_corrected/` : petite sonde API, λ finis mais stabilité
  pondérée infinie et EOM faussé ; fusion zéro → NaN. Notes de bornes
  positives distinctes du domaine API général et des résultats du producteur.
  `singleton_20260930/` ajoute cinq cas natifs : la racine consomme un niveau
  zéro même avec une masse inférieure à min_cluster_size ; garde future
  non implémentée, aucune qualification de sa correction.
- `bench_corrected/` : scripts figés, 18 commandes courtes, vrais processus
  arrêtés au délai/signal, complétude refusée ; ARI impossible encore accepté.
  CSV volontairement corrompus de fixture, jamais scores de clustering.
- `performance_corrected/` : journaux CPU et sources observés, gains J3,
  frontière et tour séparés ; mutant équivalent par parité. Une source
  temporaire a disparu après copie, échec de fermeture conservé. Aucun
  nouveau résultat d'exécution G4/GPU.
- `majority_boundary_geometry/` : huit petits nuages K2, quatre de dimension
  affine 3 ; 32 exports natifs et juges Fraction. La majorité uniforme perd
  les deux groupes précoces avant fusion, 1/β les récupère. Piste de tête
  d'audit uniquement, sans EOM, ARI ni qualification statistique.
- `projection_facts_corrected/` : contre-oracle Γ exact de l'oracle fenêtres,
  75 couples nuage/K par passage ; niveaux zéro, K=n, frontières, stabilité
  appariée. Campagnes développeur closes distinguées des tests autonomes.
- `sitetree_corrected/` : replis et domaine corrigés ; harnais quatre arrondis
  avec trois planchers code 3, terminal global code 1 conservé, sans nouvel
  écart géométrique. Gate développeur FE_TONEAREST close observée.
- `order_head_corrected/` : défaut de validation parallèle CSR reproduit
  sur objet public forgé ; gains aval locaux et états des tests séparés.
  Ne qualifie pas une nouvelle exécution FULL/G4 ou le GPU.
- `timeout/cuda_corrected/status_20260930/` : vingt entrées d'enveloppe
  et huit simulations de chemins d'échec normal/−O, aucun processus CUDA.
  Lecteur limité au statut/code ; risque d'ancien JSON sur dossier réutilisé,
  distinct d'un échec réel GPU ou d'une preuve numérique.

Ces nouveaux fichiers ne font pas partie des 34 déplacements historiques
du manifeste. Les comptes, sources et limites sont dans leurs reçus propres
et dans les rapports pointés par l'état courant. GCP non utilisé.

## Vérification de publication du 30 septembre

Les manifestes des six nouveaux paquets ont été vérifiés sur la copie de
publication, avec translation de chemin seulement pour le manifeste absolu
ordre/tête. Les rapports rédigés et leurs liens locaux passent le contrôle
ciblé. Les sorties et patches observés restent octet pour octet, y compris
leurs espaces de fin de ligne.

Le contrôle documentaire global n'est pas vert : il parcourt aussi des
copies partielles de Markdown sous `sources/` et `observed/`, dont les liens
relatifs désignaient leur arbre d'origine. Ces fichiers de provenance ne
sont pas des pages autonomes ; les hashes clos interdisent de les corriger
silencieusement. Cette limite est distincte des liens des rapports courants.
