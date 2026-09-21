# Conditions des mesures33

21 septembre2026, machine locale Codespace, 8 CPU logiques / 4 cœurs
physiques selon `lscpu -p=CPU,CORE,SOCKET`. Aucune exclusivité CPU,
affinité ni fréquence fixe revendiquée. GCP non utilisé.

Au lancement de la comparaison mono puis quatre workers, un auditeur
indépendant exécutait encore le probe32 LiDAR8k avec témoins désactivés
et census scalaire (environ375% CPU). La régression CTest95 constructeur
était également active, avec deux tests simultanés. Ces processus sont
distincts : ne pas arrêter celui de l'auditeur ni assimiler ses résultats
aux nôtres. Les temps sont donc des observations sous charge concurrente,
pas une qualification de gain stable ou d'accélération parallèle.

Les matrices grandes tailles sont exécutées séquentiellement par le même
collecteur. Les empreintes d'entrée, travaux discrets et digests de sortie
permettent néanmoins des comparaisons de croissance et d'équivalence ;
ils ne changent pas de signification avec la charge CPU.

Les scans100/200 sont lancés pendant la fin K10/32k du scan0 : les deux
collecteurs peuvent donc se concurrencer, en plus de l'audit indépendant.
À l'intérieur de chaque collecteur les commandes restent séquentielles.
Ces temps ne sont pas appariés pour revendiquer une accélération ; le
bilan de croissance repose en priorité sur les travaux discrets.

La matrice s10/12 est également lancée pendant les dernières grandes
commandes des deux matrices précédentes. Ces chevauchements visent la
collecte des travaux exacts sur une machine partagée ; ils excluent toute
lecture de leurs rapports de temps comme mesure isolée de complexité.
