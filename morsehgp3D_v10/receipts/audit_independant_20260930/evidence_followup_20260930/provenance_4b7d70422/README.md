# Contre-vérification bornée de provenance — 30 septembre 2026

Source : commit `4b7d7042226e4299e57e1257f4034d45d82c8b77`, lu par `git show`.
Le contrôle lit les 105 payloads du ledger de développement, compare les
51 puis 78 sources déclarées au commit et vérifie que pool, tête,
dendrogramme et les trois CLI portent toujours les sources de `777406b82`.
Deux exécutions pures du test structurel de bande, normal et `-O`, passent
chacune les cinq tests. Aucun nouveau défaut produit n'est démontré.

La première capture reste un moteur u18 avec le correctif RankIndex et
un bras Python extérieur. La seconde capture CMake construit uniquement
les cibles autonomes de recherche de rang et de primitive u32 : elle ne
qualifie pas une intégration commune des correctifs R2 du pool, de la tête
ou des CLI. Les six nuages natifs K3/K5, cinq à sept sites, et leurs
815 contrôles par mode sont des régressions structurelles bornées.
Aucune exécution du moteur, compilation, allocation massive, GPU ou GCP
n'a été faite pour cette contre-vérification.

Relecture : `python3 -B check.py --repo <checkout> --out <nouveau.json>`.
Le reçu ne certifie pas les sources ou binaires vivants d'un autre build.
