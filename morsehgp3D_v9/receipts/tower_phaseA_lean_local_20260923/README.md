# Reçu local : phase A de la tour allégée (rangs de plateau, lots singletons)

23 septembre 2026. **GCP non utilisé.** Cadre : `exploration_v9_hors_registre`,
`backend=reference_cpu`, `profile=quantized_u18_input_only`,
`public_status=not_claimed`.

## Changement mesuré

Code `aa29245f`, mesuré contre sa base `3dfedcae`. Aucun changement de
l'objet.
- **Rangs de plateau exacts.** Après le tri exact `by_level`, chaque boule
  reçoit l'indice de son plateau : les niveaux exacts égaux partagent un
  indice. Le filtre flottant certifié tranche les écarts nets, la
  comparaison exacte le reste, en deux passes parallèles.
- **Comparaisons entières.** La phase A découpe les lots et vérifie qu'une
  cible est strictement sous son bloc en comparant ces indices. La phase 0
  fait de même pour les graines et la chronologie des requêtes. Ces tests
  étaient auparavant des produits U320 par facette.
- **Lots singletons.** L'action n'est construite que si elle est publiée ;
  les racines sont copiées au lieu de voler le tampon du bloc. Un bloc
  inerte n'alloue donc plus rien.

Toutes les portes passent (152/152 `gate`). Les condensés de tour sont ceux
de R12/R13.

## Mesures (trame sans sol 08/000000, 39 885 sites, s = 8, W8, tour statique 8)

Hôte partagé et chargé (charge 10 à 12 sur 8 cœurs) ; paires entrelacées
base / nouveau. Binaires : base `7f2d4c02…`, nouveau `74732c6a…`.

| K | bras | phase A de l'ordre K (ms) | tour (ms) | validation (ms) |
| ---: | --- | --- | --- | --- |
| 5 | base | 753 / 784 / 835 | 1 990 / 1 867 / 1 898 | 237 / 232 / 230 |
| 5 | nouveau | **495 / 584 / 534** | 1 830 / 1 983 / 1 875 | 311 / 329 / 302 |
| 10 | base | 3 679 / 3 119 | 14 766 / 15 858 | 1 395 / 1 437 |
| 10 | nouveau | **2 134 / 1 965** | **13 340 / 13 323** | 1 406 / 1 484 |

- **Chemin critique** : la phase A de l'ordre le plus élevé baisse de
  30 % environ à K5 et de 35 à 40 % à K10. À K10, la tour baisse de 10 à
  16 %.
- **Tour à K5** : aucun gain lisible sur ces trois paires, à cause du bruit
  de l'hôte.
- **Validation** : un chronomètre posé sur le seul calcul des rangs donne
  environ 10 ms (deux exécutions à part). La hausse de 70 à 90 ms vue dans
  les paires n'est pas expliquée par ce bloc ; à remesurer sur G4.
- **Instructions de la phase A** (callgrind, coupe emboîtée de 4 000 sites,
  K5, 2 fils) : 540,7 M → 301,3 M (−44 %).
  - `compare_exact_level` disparaît de la phase A.
  - Les allocations restantes, environ 30 %, sont celles des actions
    publiées. Les supprimer demande un brouillon plat (étape séparée).

## Contenu

- `out/` : sorties JSON de la sonde (K5 : trois paires, K10 : deux paires).
- `callgrind_phaseA_{base,new}.txt` : coûts exclusifs dans `order_lots`.
- `SHA256SUMS`.
