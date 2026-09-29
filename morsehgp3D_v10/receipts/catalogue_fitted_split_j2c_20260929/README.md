# Reçu : J2c, arbre de boîtes ajustées à leur liste et coupées en deux (29 septembre 2026)

`backend=reference_cpu`, `public_status=not_claimed`. GCP non utilisé pour ce reçu : les mesures sont locales, et la
mesure G4 à 48 fils suit.

## Objet

L'utilisateur a demandé si une découpe « à la WSPD » ferait mieux. L'audit de l'agent
([`AUDIT_AGENT_DECOUPE.md`](AUDIT_AGENT_DECOUPE.md)) a corrigé la prémisse : les boîtes étaient déjà cubiques, et la
découpe ne dépend pas des points. Ce qu'apporte l'arbre de la WSPD, c'est d'ajuster chaque boîte à ses points avant
de la couper. Le reçu complet de l'agent, avec preuves, décisions et mesures, est
[`RECU_AGENT_J2C.md`](RECU_AGENT_J2C.md).

**Changement** (`src/catalogue/generator.cpp`, `catalogue.hpp`, CLI du catalogue) :

- chaque nœud remplace sa boîte Q par S = Q ∩ [env.lo, env.hi + 1), env étant l'enveloppe fermée de sa liste
  certifiée, puis coupe S en deux au milieu de son plus long côté ;
- D-loc et la clé du réservoir passent à un côté par axe ;
- la pré-ignorance A6 de J2 est retirée : une fille est incluse dans le S de son parent, donc A6 ne se déclenche
  jamais ;
- stagnation : plus long côté de S comparé à 2^kT, arrêt après 9 niveaux binaires sans décroissance (3 niveaux de
  l'octree) ;
- M(K) = 12, 16, 24, puis 28 au-delà de K = 10 (32 avant).

## Relecture

- **S contient tout centre admis de Q.** Le support d'une boule admise est dans sa liste (théorème C). Le centre est
  dans l'intérieur relatif de l'enveloppe convexe du support, donc dans [env.lo, env.hi]. Les coordonnées de
  l'enveloppe étant entières, il est dans le demi-ouvert [env.lo, env.hi + 1).
- **Chaque boule est trouvée une seule fois.** Les deux moitiés pavent S, et le même argument vaut pour chaque fille.
- **S vide est exactement l'ancien lemme K** (enveloppe hors de Q).
- **D-loc par axe** reste le maximum exact de |Y − C|² − |X − C|² sur le pavé fermé.
- **La clé du réservoir** ne règle que la puissance du filtre, pas l'exactitude. C'est pourquoi le mutant M3 n'est
  vu que par le grand livre.

## Contrôles

- **Agent**, sur `b662673b2` + J2c :
  - différentiel des 10 entrées contre `568d45297`, à 1 et 4 fils, en build normal et empoisonné : dumps, compteurs
    du catalogue et niveaux exacts identiques, identité de l'arbre binaire vérifiée ;
  - 9 portes ; ASan et UBSan ; ThreadSanitizer sur le quart 01 et l'oracle ;
  - dégénérescences (grilles, sphères entières) ;
  - mutants M1 (borne haute sans + 1) et M2 (côté de l'axe 0 pour les trois axes) tués par les dumps et l'oracle ;
    M3 (clé du réservoir cubique) tué par le grand livre.
- **Intégration**, dans le worktree à `b662673b2` + J2c :
  - 9 portes sur 9 ([`ctest_gate_integration.txt`](ctest_gate_integration.txt)) ;
  - différentiel des 10 entrées contre `568d45297`, à 1 et 4 fils, 10 sur 10 identiques, identité de l'arbre
    binaire comprise ([`differentiel_integration.txt`](differentiel_integration.txt)).

## Mesures de l'agent (codespace, `t_boxes`, médianes de 3)

| Cas | Avant (s) | Après (s) | Gain |
| --- | ---: | ---: | ---: |
| Trame 02, K = 5, 1 fil | 6,265 | 5,164 | ×1,21 |
| Trame 02, K = 5, 4 fils | 1,601 | 1,277 | ×1,25 |
| Trame 02, K = 10, 1 fil | 25,52 | 22,12 | ×1,15 |
| Quart 01, K = 5, 4 fils | 0,250 | 0,192 | ×1,30 |

L'arbre compte 33 à 40 % de nœuds et 54 % de feuilles en moins.

**Point à surveiller** : la frontière initiale (`t_frontier`) coûte 60 à 100 % de plus (0,05 → 0,086 s à 1 fil sur
la trame 02, K = 5). Il faut environ trois fois plus de tours pour atteindre 64 × P tâches.

**Conséquence GPU** : les conceptions « arbre » J5 et J6 supposaient un octree implicite. Elles sont à revoir :
boîtes explicites, éventail de 2, profondeur environ triple.
