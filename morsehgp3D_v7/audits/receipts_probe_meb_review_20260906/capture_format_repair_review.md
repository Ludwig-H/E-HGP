# Levée du défaut de transport — 6 septembre 2026, 10:27 UTC

Les cinq obstacles de la [contre-fixture historique](capture_format_review.md)
sont **levés** dans le comparateur `910b30ac…` : `source_map_sha256`, calendriers
lus dans le protocole, inventaire des deux streams et treize champs d'intention.
Les captures d'origine et la contre-fixture restent inchangées. La lecture du
port confirme aussi les liens du spawn, de la clôture et des horodatages.

Le [reçu de cette lecture](capture_format_repair_review.json) contrôle directement
les sorties constructeur de `build/v7_meb_compare_20260906_preparation/checks_micro_r1/` :
reçu `37f6a202…`, sorties normal/`-O` identiques `a758d6c7…`, deux codes 0.
Une vérification Python indépendante, sans importer comparateur ni juges,
a relu 735 fichiers stables et effectué 9 171 contrôles : 72 tentatives,
48 paires et 312 ordres, dont 156 par opt-in P=1 et P=584000000.
Les 25 anciens champs Work et tous les autres champs non mesurés des ordres
et terminaux concordent ; les différences MEB et de mesures affichées sont
recalculées depuis les captures. Les 144 résultats des juges correspondent à
leurs sorties capturées, avec leurs liens aux bytes bruts et reçus.

Il s'agit d'une **lecture datée de captures locales**, pas d'un paquet complet
de rejeu portable ni d'une nouvelle qualification des juges. Aucun verdict
géométrique, digest de forêt ou ELF n'est recalculé ; les 413 commandes de
l'admission ne sont pas toutes revues ici. Toutes les tentatives comparées
sont complètes relativement : aucun refus réel n'est exercé par ces 48 paires.
Sur n=8, Kmax demandé 10 devient K effectif 8. Aucun temps lourd, moteur,
nouveau résultat de complexité, claim FULL global ou SLO ; GCP non utilisé.
