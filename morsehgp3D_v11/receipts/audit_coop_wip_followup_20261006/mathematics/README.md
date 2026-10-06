# Petites portes coopératives : masque et u18

Lecture seule du WIP `v11-impl-l3` basé sur `3b76a3fcf0ca14dd08f005e1e1ae8e3418dd247e`, capturé deux fois avec octets identiques. Aucun test natif, build ou GPU exécuté. Le noyau compact est favorable en lecture ; les deux apports portent sur ses portes de qualification.

Le mutant `masks[1]=0` exige une boîte stricte. Fixture : sites Morton `[(0,3,0),(1,3,0),(3,1,4),(3,0,6),(5,6,6)]`, boîte `[2,5)^3`, K=3. Les dominances sont `dom[0]=bit1`, `dom[3]=bit2`, toutes les autres nulles. L'oubli du premier masque fait visiter J2 `(0,3,4)` : `region_line_tests` passe de 3 à 4. Les boîtes couvrant tous les sites ont une dominance nulle et ne discriminent pas ce mutant.

L'exigence `near_max: tally.partial>0` n'est pas valide en u18 pour les trois fixtures capturées. La puissance et les poids q4 y sont certifiés ; aucun de leurs 21 triangles strictement aigus n'a de coquille étendue nécessitant un certificat d'orientation q3. Conditionner cette attente au profil concerné, par exemple B>20 pour ces fixtures, et conserver la comparaison complète dans tous les profils.

```sh
python3 -S -B replay.py
python3 -O -S -B replay.py
sha256sum -c SHA256SUMS
```

Rejeu autonome depuis cette capsule : sources causales et modèle Python inclus. Normal/−O, codes 0, sorties identiques, 40 contrôles ; aucun essai échoué. Voir `REPORT.md` pour les limites.
