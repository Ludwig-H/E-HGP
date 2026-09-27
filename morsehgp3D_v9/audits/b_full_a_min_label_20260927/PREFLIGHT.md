# Préflight mutable, non qualifiant

27 septembre 2026. Build de développement distinct :
`/workspaces/E-HGP/build/v9-a-min-label-preflight-20260927`.
Ce répertoire et les sorties ci-dessous ne sont pas une capture de
qualification épinglée. Le runner frais Release/Sanitizer reste l'autorité
future ; ne pas lui substituer ce préflight favorable.

Commandes exécutées depuis `/workspaces/E-HGP/build/v9-resume-20260927` :

```sh
cmake -S morsehgp3D_v9/audits/b_full_a_min_label_20260927 -B /workspaces/E-HGP/build/v9-a-min-label-preflight-20260927 -DCMAKE_BUILD_TYPE=Release -DBOOST_ROOT=/workspaces/E-HGP/build/v7_boost_gate/extracted/usr
cmake --build /workspaces/E-HGP/build/v9-a-min-label-preflight-20260927 -j 2
cmake --build /workspaces/E-HGP/build/v9-a-min-label-preflight-20260927 -j 2
/workspaces/E-HGP/build/v9-a-min-label-preflight-20260927/mhgp9_full_a_min_label --preflight
/workspaces/E-HGP/build/v9-a-min-label-preflight-20260927/mhgp9_full_a_min_label --gate
```

Configuration et deux compilations terminées code0 ; handles de compilation
10958 puis 74900 tous deux joints. La seconde compilation prend en compte la
réinitialisation de Work/Times à chaque factory. Aucun appel LSan, GCP,
qualification fraîche ou grande mesure effectué dans ce préflight.

Les deux exécutions terminent code0, stdout JSON passed, sans stderr.
Le préflight réduit omet ico12 : 20 fixtures, 40 captures, 296 rejeux natifs
et 187 graphes de coupes. Le gate complet local donne :

| Champ | Valeur |
|---|---:|
| Fixtures / captures | 22 / 44 |
| Rejeux candidat issus de géométrie / abstraits / frontières | 376 / 74 / 4 |
| Comparaisons champ à champ | 1 699 |
| Sommets V / occurrences E / groupes G | 4 632 / 5 416 / 3 784 |
| Nœuds / entrées historiques | 3 368 / 3 368 |
| Groupes omis des historiques | 416 |
| Groupes silencieux / continuations | 384 / 32 |
| Maximum parents | 32 |
| Parents événementiels / draft / forêt | 3 408 / 3 024 / 2 992 |
| Entrées up / pertes | 24 152 / 4 632 |
| Requêtes / pas d'ancêtres | 10 048 / 64 080 |
| Maximum de capacités combinées observées (octets) | 11 884 |
| Graphes / requêtes de coupe | 227 / 1 886 024 |
| Vérifications de monotonie par sommet | 11 098 |
| Refus exacts / branches mutantes tuées | 682 / 3 |

Les masses V/E/G et la capacité maximale concernent les 376 rejeux issus
des captures géométriques, pas les graphes abstraits. Les compteurs des
coupes et comparaisons incluent tous les graphes. `parallel=false`,
`FULL_executed_by_candidate=false`, `GCP_used=false`.

Points corrigés avant gel, et non échecs de géométrie :

- La sortie avec ses quatre vecteurs CSR initiaux existe déjà durant la
  factory : son coût initial est ajouté au maximum de capacités combinées.
- Les coupes sont les valeurs distinctives autour des rangs, non tous les
  entiers jusqu'au maximum. Une fixture au dernier rang fini teste le bord
  sans boucle géante ni dépassement. Aucun plafond de recherche ajouté.
- Work/Times sont remis à zéro par la factory, afin que les appels répétés
  ne cumulent pas `forest_edges` et ne sous-débordent pas `components`.

Aucun échec de compilation ou d'exécution n'a été observé durant ce
préflight. L'absence d'échec n'est pas une qualification fraîche ; les
sources ont été gelées après contrelecture pour cette étape distincte.
