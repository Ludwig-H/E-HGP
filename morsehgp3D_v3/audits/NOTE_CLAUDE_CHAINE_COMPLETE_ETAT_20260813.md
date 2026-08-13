# Note de Claude — la chaîne complète existe, et ma fenêtre mesure son coût dominant

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. `LocalShallowBall` n'est pas à écrire — il est là

En cherchant à implémenter votre jalon, j'ai trouvé qu'`anchor_pipeline.hpp` le
contient déjà, et qu'il respecte les règles que vous veniez de me rappeler :

- lentille `\lVert x-a\rVert^2\le d^2` et `\lVert b-x\rVert^2\le d^2` ;
- bit **aigu** par porteur, et la règle q4 correcte —
  `if (acute[i]==0 && acute[j]==0) continue`, donc **une seule face aiguë
  suffit**, exactement votre section 9 et votre contre-fixture ;
- owner d'arête maximale canonique vérifié sur les identifiants triés ;
- circumboule q3/q4, positivité, census par rang, émission avec clé.

Je n'ai donc rien à réécrire ; j'ai à **brancher la fenêtre dessus** et à
mesurer.

## 2. Ce que la chaîne produit, mesuré

`uniform`, `smax=11`, deux threads, moteur de référence :

| `n` | q2 | q3 | q4 | total | supports/pt | temps |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `1 500` | `47 453` | `227 070` | `212 425` | `486 948` | `324,6` | `22,9` s |
| `3 000` | `100 340` | `492 817` | `472 643` | `1 065 800` | `355,3` | `59,8` s |

**Identité : `occurrences = clés uniques`, zéro doublon** aux deux tailles. Le
census produit donc bien un multiensemble sans répétition.

Pente du temps : `1,38`. High-water `kept` : `446` puis `474`.

## 3. Le recoupement qui me convainc

Le `kept` du moteur — sa fenêtre par ancre — vaut `446` à `n=1 500` et `474` à
`n=3 000`. Ma mesure indépendante de `\sum_a\lvert N_2(a)\rvert` sur le front
WSPD à `s=3` donne une moyenne de `477,6`, `481,6`, `528,6` à `n=4 000`,
`8 000`, `16 000`.

**Ce sont le même objet, et les deux mesures concordent** alors qu'elles
viennent de deux codes sans aucune primitive commune. C'est la vérification
croisée qui me manquait : la fenêtre que votre compteur exige est exactement le
`kept` que le moteur consomme, et elle est mesurée **quasi constante** — donc
`O(n)` au total, donc la route passe votre critère de mort.

## 4. Où est réellement le contrat

Sortie : `355` supports par point, soit environ `17,8` M supports à `50 000`.
Le contrat d'une seconde demande donc `17,8` M supports par seconde, ce qui est
exigeant mais pas absurde pour un kernel.

Extrapolation du temps CPU actuel à deux threads : `59,8` s à `n=3 000` avec une
pente `1,38` donne environ `48` minutes à `50 000`, soit environ deux minutes sur
quarante-huit cœurs. **Je ne présente pas cela comme un résultat** : c'est une
extrapolation d'une seule famille sur deux points, et la pente `1,38` viole
elle-même votre règle si elle se confirme.

## 5. Ce que je fais maintenant, dans votre ordre

1. mesurer la chaîne complète sur la rampe et les quatre familles, sur G4, avec
   les deux pentes sur **temps**, **supports émis**, **`kept`**, octets et
   high-water — c'est la première fois que je peux mesurer une sortie, et non
   un compteur intermédiaire ;
2. rapprocher `kept` et `N_q(a)` dans le même reçu, pour que le recoupement de
   ma section 3 devienne une porte et non une remarque ;
3. seulement ensuite, le join factorisé et le kernel.

## 6. Deux questions

1. Le `kept` du moteur est produit par son propre chemin d'ancre, pas par le
   front WSPD. Faut-il **remplacer** l'un par l'autre — la fenêtre WSPD
   alimentant le moteur — ou les garder séparés et exiger qu'ils **coïncident**
   comme porte croisée ? La seconde option me paraît plus sûre mais coûte deux
   constructions.
2. La pente `1,38` du temps est mesurée sur deux points seulement, ce qui
   n'autorise aucune conclusion. Quelle rampe minimale exigez-vous pour qu'une
   pente de **temps** soit recevable, sachant que le temps mêle sortie et
   travail ?

## 7. Non-claims

Aucun `p95`, aucun octet, aucun high-water publié ici. Le fold vers la forêt des
`K` arbres n'est pas mesuré. Les chiffres de la section 2 viennent d'une seule
famille et de deux tailles. Le contrat `50 000` reste entièrement ouvert et G4
reste NO-GO.
