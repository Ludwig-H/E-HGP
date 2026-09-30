# Capture indépendante — fixtures de projection corrigées

29 septembre 2026. CPU de référence, u18, hors registre, `not_claimed`.
Aucun moteur, CLI, fichier d'un autre acteur ou compte externe modifié.

`independent/` contient nos preuves exécutées. La paire d'autorité est
`exact_final_normal.json` / `exact_final_optimized.json` : les deux commandes
terminent avec code 0, zéro écart et sorties identiques. Les deux JSON
`exact_normal.json` / `exact_optimized.json` conservent la première passe,
avant ajout des valeurs F2 et calcul explicite du raffinement de vote.
Cette première passe était également verte ; elle n'est pas la paire finale.

Comptage **par mode** : 75 couples nuage/K, 604 partitions core, 604 nombres
de composantes, 604 couvertures, 235 entrées et 235 bornes, 280 hauteurs,
50 coupes comparées à la référence, 72 paires et 72 entrées de stabilité.
Le −O répète exactement les cas du normal ; ne pas doubler le nombre
de fixtures indépendantes. Les premières passes ne s'ajoutent pas au corpus
de la paire finale.

Rejeu autonome, sans build ni binaire produit :

```sh
python3 scripts/check_projection_exact.py
python3 -O scripts/check_projection_exact.py
```

Le script importe les copies inchangées du juge `Line` et de la référence
exacte, construit indépendamment Γ par toutes les k-parties, contrôle les
frontières, K1/r0, K=n et les hauteurs, et conserve un contre-exemple abstrait
de vote réévalué par coupe. Il n'exécute aucun moteur. Les snapshots sont
copiés mécaniquement depuis `build/v10-fixes/faits_math/src`.
`sources_before.sha256` / `sources_after.sha256` contrôlent aussi les
originaux et la copie `faits_math-verif` du juge ; leurs contenus sont égaux.

`developer_observed/` contient exclusivement des résultats lus dans les
sessions développeur, jamais relancés par nous. Les timestamps et listes
de processus documentent leur état lors des lectures. Les deux campagnes
CTest ont des fins 10/10 et code 0 explicites ; le résumé terminal de leur
contre-oracle Line/Γ ne contient pas de code de retour archivé.

La note courante est
`audits/audit_continu_20260929/CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md`.
`SHA256SUMS` ferme ce dossier et la note, hors le manifeste lui-même.
