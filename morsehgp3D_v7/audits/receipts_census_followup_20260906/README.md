# Raccord census : contrelecture favorable et fixture non vide

6 septembre 2026, constructeur f4ffe38c. phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed. Aucun moteur, compilation ou GCP auditeur.

La [correction publiée](../../receipts/full_census_payload_20260906/README.md) applique bien les gardes prouvées : U candidats conservés, S≤U, arithmétique contrôlée et contrôle census après préfiltre, **avant l’appel qui alloue staged**. Le compteur de version figure dans chaque ligne JSON ; run.hpp reste inchangé. Aucune erreur nominale identifiée.

Les captures constructeur sont cohérentes : 143 fichiers manifestés, 50 références externes, 53 sources avant/après identiques, 20 commandes closes, 117 dépendances des quatre compilations couvertes, et deux CTests avec leur JUnit. La vérification portable du paquet passe en Python normal/-O. Cette lecture ne réexécute aucun C++.

## Pourquoi les micros ne peuvent pas exercer un rejet du préfiltre

Un candidat dont le support valide possède q sites a au plus n−q points strictement intérieurs : ses q points de support sont sur la coquille. Si smax=n, le seuil smax+1−q dépasse donc cette profondeur maximale d’une unité. **Aucune boule valide ne peut mourir au préfiltre dans ce régime.**

Les quatre micros publiés utilisent n=8, Kmax=10, donc smax=min(n,Kmax+1)=8. Le constat E=U=S=71 est ainsi attendu mathématiquement. Ils vérifient le passage nominal et les digests ; ils ne peuvent pas vérifier un raccord U>S. Les 40 tests séparés exercent les helpers arithmétiques, dont S<U, pas une injection de faute à l’appel du census. Le constructeur déclare cette limite ; elle n’invalide pas son implémentation.

## Une fixture exacte à sept points suffit

Pour Kmax=5, admis par la sonde, prendre les sept positions (x,0,0), avec x=0,1,3,7,15,31,63. Fournir au sous-pipeline les cinq candidats q2 définis par les paires (0,1), (1,3), (3,7), (7,15), (0,63). La puissance primitive de la boule diamétrale est x²+y²+z²−(a+b)x+ab. Les cinq clés et niveaux sont distincts.

Les quatre petites boules n’ont aucun intérieur. La dernière en a cinq et meurt exactement au seuil smax+1−2=5, puisque smax=6. On obtient **U=5, S=4**, avec deux sites de coquille par boule. Sept est le plus petit n permettant une telle mort dans le profil Kmax=5 : pour n≤6, smax=n et l’argument précédent l’interdit.

| Admission sur l’ABI C=144, V=16, D=224 | Octets |
| --- | ---: |
| Tri à 2UC | 1 440 |
| Préfiltre à U(C+2V) | 880 |
| Census correct UC+S(V+D) | 1 680 |
| Mutant remplaçant U par S au census | 1 536 |

Le budget 1 600 admet les phases amont et le calcul fautif, mais refuse le census correct. Les frontières exactes sont 1 680 et 1 679. Ce cas permet de préparer un test qui injecte les candidats à la couture préfiltre/census, observe les quatre survivantes puis vérifie que l’admission refuse **avant l’allocation**. La suppression ou le déplacement du contrôle demandent cette observation dynamique ; un test du helper seul ne suffit pas.

Le [modèle rationnel](fixture.py), exécuté avec python3 -B puis python3 -B -O, donne des [sorties normales](normal.json) et [optimisées](optimized.json) identiques, code 0. Il vérifie la géométrie et les admissions. Il ne prétend ni que le générateur v7 émet cette liste, ni avoir exécuté le census ou instrumenté son allocation, ni fournir un catalogue FULL complet. Les [preuves générales d’admission](../receipts_phase_selection_20260906/README.md) restent inchangées.

Le [registre d’entretien](../ENTRETIEN.json) épingle cette relecture, celle du prototype de blocs saturés et les tests purs du worker FULL. Aucun nouveau résultat de performance ni qualification de session G4.
