# Lecteur renforcé R2 du filtre relatif

30 septembre 2026. Le [reçu original](../relative_filter_20260930/README.md)
reste clos, à octets identiques. Sa contre-relecture ne trouve pas d'erreur
géométrique, mais relève deux angles morts du lecteur : chargement du juge
avant vérification des hashes et comparaison de tables before/after
potentiellement toutes deux vides.

Ce [lecteur R2](verify.py) vérifie d'abord le manifeste original épinglé et
tous ses fichiers, puis impose les ensembles d'empreintes attendus et zéro
exécution native aux rejugements R2/R3. Il ne charge les modules archivés
qu'après ces gardes. Cinq contrôles structurels refusent les trois tables
vides et deux faux compteurs natifs. Les sorties natives et leurs trois
juges sont ensuite rejoués à l'identique, sans nouvelle exécution native.

Utiliser désormais ce point d'entrée :

```bash
python3 -B morsehgp3D_v10/receipts/audit_continu_20260929/relative_filter_reader_r2_20260930/verify.py
python3 -B -O morsehgp3D_v10/receipts/audit_continu_20260929/relative_filter_reader_r2_20260930/verify.py
```

Les 3 600 requêtes, 196 contacts, 40 translations et 147 contrôles statiques
de rang sont le même panel, pas des cas supplémentaires. L'adversarial R1
est contrôlé par ses conclusions enregistrées, pas réexécuté. Aucun module
du moteur, nearest natif, exact fallback, FULL, GPU ou chrono n'est qualifié.
GCP non utilisé. Les dépendances système des compilations restent externes.

Le contrôle des espaces sur l'ensemble des données signale une ligne vide
finale dans le script adversarial R1 clos ; elle est conservée. Les nouveaux
documents et scripts, ainsi que le contrôle documentaire ciblé, sont propres.
