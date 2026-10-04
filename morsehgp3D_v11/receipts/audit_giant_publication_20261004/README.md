# Dernier changement publié pendant l'audit

Le commit **b72fe8771** est arrivé entre la clôture des preuves et la
publication de l'audit transversal. Il ajoute seulement `--members-all`
à `bench/points_campaign.py` ; aucun src natif ne change.
Le reçu [principal déjà clos](../audit_giant_20261004/README.md) reste intact.

Relecture des 23 lignes nouvelles : les membres H sont convertis par
les IDs de l'export, de l'ordre Morton aux indices d'entrée ; la hiérarchie
HDBSCAN utilise déjà ces indices. Chaque liste est triée et indépendante,
ou vaut `None` au-delà de MEMBER_CAP/si aucun meilleur bloc. Le champ
supplémentaire reste facultatif et n'intervient dans aucune décision HGP,
condensation ou score.

Cette sortie aide à contrôler la compatibilité simultanée des meilleurs
blocs et à préparer une mesure plate. Elle ne produit pas une antichaîne.
Le lot G4 claudebouts1, antérieur à ce commit, ne qualifie pas cette option.
Aucun nouveau build, natif, fit ou GCP ; simple relecture du delta et de
ses interfaces inchangées. Pas de nouveau défaut causal établi.

`REVIEW.json`, le source `.snapshot` et le diff fixent la portée ;
`LEDGER.json` et `SHA256SUMS` ferment les fichiers. Ce complément ne
modifie aucun octet d'une capsule close du reçu principal.
