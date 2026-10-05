# Complément ciblé de B — source b319efc84

Dérivation des portes sans résultat dans la capsule B fermée, sans nouvelle
exécution. Les 50 portes ASan/UBSan sont incluses dans les 65 portes TSan :
intersection 50, union 65, ASan seul 0, TSan seul 15. Les noms exacts triés,
ensembles et groupes figurent dans `restart_sets.json`.

| Module | ASan/UBSan u24 | TSan u21 |
| --- | ---: | ---: |
| catalogue | 1 | 6 |
| tower | 17 | 17 |
| supports | 19 | 19 |
| cli | 13 | 22 |
| support (sentinelle LiDAR du harnais) | 0 | 1 |
| Total | 50 | 65 |

La reprise utile vise ces compléments par modules avec des budgets dédiés.
Sur `b319efc8477fec234afc0b31e86f8a43e3023641`, les portes déjà Passed restent
documentées par la capsule B ; aucune reprise des 824 portes n'est nécessaire
pour établir les verdicts manquants. Cela exige les mêmes sources, profil,
compilation et instrumentation. Si la source évolue, annoncer une nouvelle
qualification et requalifier explicitement ses changements avant de combiner
ses résultats avec ceux de B. Les portes `long`, mutants et supports LiDAR
W48 sont d'autres périmètres ; elles ne sont pas ajoutées à ce complément.

Au contrôle des plans locaux, aucun plan B2 ni session nommée B2 trouvé :
`build/v11-persist/qual_sorties/plan_b.json` garde son SHA
`903cce8e52c33a8974c74fbcad6253ecf49899c46cc28c7a8cb92bb025ae89dc`.
La recherche a couvert les noms JSON des deux premiers niveaux de
`build/v11-persist`, les notes de qual_sorties/sortie_supports et les noms de
sessions. Cette note ne crée pas de plan exécutable ni de session G4.

```sh
python3 /chemin/de/cet/addendum/derive.py --summary /chemin/de/la/capsule_B/summary.json
python3 -O /chemin/de/cet/addendum/derive.py --summary /chemin/de/la/capsule_B/summary.json
```

Le script stdlib vérifie le pin, la session, les comptes et les ensembles,
puis leurs SHA. Il ne lance rien et n'accède pas au cloud. Il dépend seulement
du `summary.json` exact de la capsule B (SHA
`29970690ad9bd9c0723b745b6413f4911cfe07ca118a29376adf229740895b5e`).
La capsule B et les captures précédentes restent inchangées.
