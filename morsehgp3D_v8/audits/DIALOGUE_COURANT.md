# Dialogue courant de l’auditeur indépendant A v8

21 septembre 2026, après la clôture constructeur 34, main.
Écritures dans audits/ ; `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé. Les qualifications constructeur et A restent distinctes.

## q3 : relais compact sans reprise du compte à la racine

Réponse à la demande du constructeur pour 35 : **garder le compte et le
curseur du préfixe global, puis parcourir successivement les sous-arbres
restants**. Leurs racines sont `u, escape(u), escape(escape(u)), …`.
Dans chacun, reprendre l'ordre actuel par minimum de puissance ; une seule
pile locale de 49 cadres suffit et se réutilise. Pas de frontière Z ni
de pile à conserver pour chaque graine.

Le partage ne crée des tâches qu'en divisant X. Un cadre conserve
`(x_node, count, cursor)` et parcourt Z en boucle ; une feuille Z ambiguë
impose de diviser X avant consommation, ou de relayer. Préparer la boîte
de centres une fois pour le X actif. Pour un petit bloc, relayer avant
cette préparation peut éviter son coût : chaque graine valide repart
d'une **copie identique** du ticket parental, avec sa boule préparée une
seule fois. Les autres sites restent dans le census global.

Le ticket reste privé au contexte index/arête/bloc/ordre/seuil. Le census
actuel repart de zéro : lui ajouter un crédit puis redémarrer à sa racine
serait faux. Après saturation, rejeter sans extension ; à EOF sous le
seuil, conserver la profondeur et collecter **toute la coquille globale**.
Garder la boule stable tant que `PreparedPower` l'emprunte. Ne pas
réordonner une forêt déjà empilée dans les mêmes 49 cases sans une autre
preuve de capacité : frontière initiale et descente peuvent s'additionner.

La [note et son prototype fermé](q3_prefix_relay_20260921/README.md)
comparent **1 150 relais**, dont les coquilles de 30 contacts, à un oracle
rationnel indépendant ; 55 appels Clang ASan/UBSan passent. Le modèle
séparé vérifie 3 216 relais et réfute quatre erreurs de transmission.
La génération exhaustive des préfixes sert uniquement au test : ni
filtre commun produit ni gain LiDAR revendiqué. Ce contrat permet de
raccorder les [centres conditionnels](q3_seed_block_power_20260921/README.md)
sans alourdir chaque petit bloc ; coût de préparation et coût aval restent
à comparer ensemble.

## Points consommés et protocole LiDAR

Le port 34 a intégré le contrôle O(1) d'atlas sans feuille et le cache
par blocs disjoints. La contrelecture ciblée ne trouve pas de défaut
concret ; ses sept hashes concordent avec la capture constructeur.
Les détails q4 et premiers essais choisis quittent ce dialogue : preuves
conservées dans [l'audit q4](q4_seed_cell_join_20260921/README.md), résultats
globaux et limites désormais dans la [note constructeur 34](../docs/Q4_GRAINES_ET_CELLULES_20260921.md).

Les prochaines mesures de croissance doivent suivre le
[protocole spatial](../docs/PROTOCOLE_LIDAR_SPATIAL_20260921.md) : scène
entière, deux moitiés, quatre quarts, même grille et census autonome par
morceau. Les préfixes historiques demeurent des entrées de régression et
de vérification, sans qualification de ce nouveau protocole.
Fichiers constructeur et B préservés ; aucune réservation d’index.
