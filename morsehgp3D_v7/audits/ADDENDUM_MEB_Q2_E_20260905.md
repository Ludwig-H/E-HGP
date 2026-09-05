# Addendum E : prétest q2 de la MEB locale

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict local favorable : le prétest q2 conserve l'objet, le support, le niveau et les refus de la MEB dans le domaine u16.** La preuve d'identité est directe et le même oracle rationnel indépendant que pour D retrouve exactement les objets et diagnostics D sur son corpus. Aucun défaut reproductible trouvé dans ce delta. Le rejeu décrit ici reste ciblé. La [qualification E ultérieure](AUDIT_QUALIFICATION_20260905.md) dispose maintenant de ses propres 324/324 portes Release, 33/33 ciblées Release et 33/33 ASan/UBSan, contre-vérifiées depuis les preuves constructeur ; les 323 tests D ne lui sont pas transférés.

## Delta effectivement relu

Le constructeur a étendu à q2 le rejet avant matérialisation déjà présent pour q3/q4. La [source](../src/forest/silent_incidence.hpp) remplace l'appel immédiat à `accept` par un prétest de contenance, puis construit la même paire clé/niveau pour le candidat contenant. Le contrôle `accept` et le contrôle final de coquille restent présents. Les charges de supports gardent leur emplacement avant l'examen du candidat.

Au moment du rejeu, `HEAD=e6d33698e62ebecf74dff01c16d7de17149d7a4e` et le worktree E modifie `CMakeLists.txt`, `src/core/mutants.hpp`, `src/forest/silent_incidence.hpp` et `tests/meb_lazy_gate.cpp`. Le header exécuté a pour SHA-256 `f75a136a320ddd1ace025436874585c2226e6150b8f2ebc37920b8dfc7e36c76`. Le reçu E épingle tous les headers effectivement consommés ; aucune preuve D n'est réétiquetée E.

Le [delta observé](receipts_20260905/e_q2/product_delta.patch) et ses
[hashes](receipts_20260905/e_q2/product_delta.json) sont conservés dans
l'audit pour reconstruire ces octets depuis D. Ce sont des pièces de
preuve ; les fichiers produit E ne sont pas inclus dans le commit auditeur.

## Identité exacte et borne de chaque calcul

La clé q2 a A=1, B=−(a+b), C=a·b. Pour chaque position z, l'identité est :

$$P_2(z)=(z-a)\cdot(z-b)=\lVert z\rVert^2-(a+b)\cdot z+a\cdot b.$$

Le nouveau prétest est donc égal à la puissance primitive elle-même, sans facteur de normalisation. Le rejet strict `power > 0` conserve tous les zéros, notamment les sommets du support et une éventuelle coquille étrangère. Le test final conserve ensuite son refus des supports non essentiels.

Avec M=65535, chaque soustraction écrite a module au plus M, chaque produit au plus M², et les sommes partielles ont module au plus M², 2M² puis 3M². On a `3*M*M = 12884508675 < 2^34`, très inférieur à la borne i64 signée. Les arguments et la valeur retournée par `p3_dot` sont réellement i64 : aucun produit i32 ni promotion tardive ne se cache dans cette expression. La borne autorise des puissances dépassant i32 dans les deux signes ; le test permanent ajoute précisément ces deux dents, les 512 triplets de coins du cube et les rejets de coordonnées hors profil avant toute évaluation.

Chaque candidat écarté aurait échoué à l'ancien `accept` avant toute écriture de `LocalBall`. Chaque candidat conservé appelle les mêmes `q2_ball_key`, `q2_exact_level` et `promote_level`, sur les mêmes coordonnées et dans le même ordre de supports. Le niveau réduit D²/4 et sa représentation restent donc inchangés. Les refus cap conservent leur compteur, leur raison et l'objet sentinelle d'entrée ; la limite de 550 supports à onze sites demeure la même.

## Rejeu indépendant sur E

Le [pilote E](meb_q2_replay_20260905.py) charge la [sonde D inchangée](meb_rational_oracle_20260905.py), construit un binaire séparé et dirige les nouvelles preuves vers `receipts_20260905/e_q2/`. Les reçus D sont préservés. L'appel suivant a rendu **0** :

```bash
python3 -O morsehgp3D_v7/audits/meb_q2_replay_20260905.py
```

Compilation C++20 `-O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=undefined -fno-sanitize-recover=all`, sans diagnostic. Le juge Fraction/Gram retrouve les mêmes 89 ensembles, 431 appels, 164 refus cap, 6 refus shell et 6 176 puissances q3/q4. Les 81/138/48 résultats contenant de support q2/q3/q4, les sentinelles de refus et les représentations de niveau sont jugés comme dans D. Les hashes des commandes/corpus et des sorties MEB et puissances sont identiques entre D et E, ce que le pilote contrôle explicitement ; les hashes de sources restent distincts.

Le nouveau mutant réel `silent-meb-q2-reject-shell` est activé par le pont existant. Sur la paire `(0,0,0), (65535,65535,65535)`, le nominal indépendant confirme un succès q2 ; le mutant entraîne `invariant_violated / silent_no_local_miniball`. Le pilote rend 0 après avoir vérifié cette divergence attendue. Le code processus 4 relève de la nouvelle porte CTest dédiée et n'est pas revendiqué par ce pont.

Preuves : [reçu du juge E](receipts_20260905/e_q2/meb_rational_optimized.json), [comparaison D/E et mutant q2](receipts_20260905/e_q2/q2_addendum.json), [entrées et sorties brutes E](receipts_20260905/e_q2/meb_rational_raw.json). Les contrôles de non-vacuité restent effectifs sous Python optimisé et les sources sont vérifiées inchangées durant chaque calcul.

## Portée pour le constructeur

L'obligation mathématique locale q2 est satisfaite, et la qualification intégrée E est désormais close sur ses reçus propres. Les trois paires D/E à s=8/10/12 conservent les objets de la tour candidate complétée, également entre séparations. Le [rapport de qualification](AUDIT_QUALIFICATION_20260905.md) distingue les compteurs avant préfiltre, qui varient avec s, des survivants et objets conservés. Chaque séparation ne comporte qu'une paire sur hôte partagé ; aucun gain statistique, SLO ou résultat GPU n'en est déduit. Le prétest ajoute une passe sur le candidat accepté et évite des clés/niveaux sur les candidats rejetés ; cette balance garde son protocole de mesure.

L'auditeur n'a écrit que dans `morsehgp3D_v7/audits/`, sans modifier les quatre fichiers constructeur. **GCP non utilisé.**
