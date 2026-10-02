# Dates de maturité : ordre exact de la copie courante

2 octobre 2026. `exploration_v10_hors_registre`, `cpu_reference`,
`audit_scalar_date_order`, `not_claimed`. Aucun GCP ni moteur modifié.

Le nouvel helper `em_dates.py` b238d2cf place les dates interpolées et
les dates libres dans une table raffinée. La copie source, son repli
`vc_certif.py` et leurs hashes sont conservés ici. Le lecteur charge
explicitement ces copies ; aucun module exact vivant ne décide l'attendu.

Oracle indépendant : toutes les racines choisies sont rationnelles.
`Fraction` détermine directement les dates, les égalités, l'ordre total
et la table fusionnée, sans utiliser le signe du helper pour l'attendu.
15 tables positives, trois ordres de déclaration, chemin filtré et chemin
tout exact : **90 exécutions, 2 880 clés et 2 430 contrôles de raffinement**,
tous conformes. 36 exécutions ont plusieurs niveaux exacts partageant
le même double. Les dates libres incluent des valeurs flottantes biaisées
de ±0,5·10⁻⁹ dans leur borne déclarée. Les deux mutations prévues par le
helper rendent 423 et 69 clés fausses, sans crash.

Rejeux normal/−O : code 0, sorties identiques ; commandes, stderr et
hashes avant/après dans [execution.json](execution.json).

```bash
python3 -B check.py
python3 -B -O check.py
```

Portée : helper scalaire capturé et hypothèses numériques déclarées,
pas des MEB géométriquement réalisées ni un export FULL. Ces données
ne qualifient ni la projection ER0h, les sommes radicales irrationnelles,
la condensation entière, les mesures historiques VC ou la précision u32.
Le résultat encourage à conserver l'identité exacte des événements,
y compris les dates ajoutées par une future hiérarchie v11.
