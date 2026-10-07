# Relecture du premier dépôt de code : socle T0 et microbanc MES-M2

7 octobre 2026. Claude, auditeur de contre-lecture. Jugé : `a0091e2b7`. Lecture, sans construction ni portes C++ (la
machine est chargée) ; deux contrôles exécutés sur un cœur. Constat au [registre](../../audits/CONSTATS.md) : `CST-0115`. Pièces de ce reçu : `verifier_ports.py` (contrôle mécanique de `docs/PORTS.md`, lecture Git seule) et sa sortie `verifier_ports.txt`, empreintes dans `SHA256SUMS`.

```text
phase=exploration_v12_hors_registre
public_status=not_claimed
GCP non utilisé
```

## 1. Lemme des faces et forme close des compteurs : juste

Relu face par face pour un quadruplet visité $Q=(i_0,i_1,i_2,i_3)$ qui passe G3, donc tel que
$\mathrm{cnt}(Q)\leq K-3$ :

- $(i_0,i_1,i_3)$ et $(i_0,i_2,i_3)$ : les préfixes de longueur deux sont visités et développés ; $i_3$ est dans
  `live[1]`, qui contient `live[2]`, et $i_3>i_2$.
- $(i_1,i_2,i_3)$ demande en plus que le singleton $\lbrace i_1\rbrace$ ait des enfants visités. C'est vrai, puisque
  $\mathrm{cnt}(\lbrace i_1\rbrace)\leq\mathrm{cnt}(Q)\leq K-3\leq K-1$ (`cnt` croît pour l'inclusion).

Trois hypothèses cachées, toutes vérifiées :

1. **Le cache J2 de la v11 est complet pour $m\leq 32$.** C'est une table dense de $\binom{32}{3}=4960$ rangs, remise
   à zéro par feuille, sans éviction (`src/catalogue/center_line_cache.hpp`). Les évaluations sont donc bien les
   triplets distincts consultés. Au-delà de 32 sites, le cache est coupé, et la table du § 3 dit « tous les tests ».
2. **Premier événement du recensement.** La forme par vote classe les $m$ sites. Son point d'arrêt doit être le
   premier, dans l'ordre séquentiel, du $(\theta+1)$-ième intérieur et du premier site non certifié ; un site non
   certifié situé après ce point ne doit rien changer. C'est ce que dit le § 7.
3. **Conflit du lemme R.** Un site qui domine un générateur et que domine un autre est impossible quand le test
   « centre dans la boîte » précède le recensement : deux générateurs équidistants du centre ne se dominent pas sur une
   boîte qui le contient. La non-résolution de ce cas n'est donc qu'une garde d'invariant.

L'identité exacte sur 2 748 544 feuilles (15 compteurs et émissions) recoupe la preuve, sans la remplacer. Le seul
ajout arithmétique, le côté q2 en `i64`, est juste : même signe au facteur 2 près, et moins de $6\cdot 2^{40}$ sous
l'étendue $2^{20}$.

## 2. `MES-S` : ce qu'il tranche

- **Supports** : tous à $s\leq 15$. **Feuilles** : à $s\leq 17$, avec 0 à 13 feuilles à $s=17$ par cas, sur
  100 000 à 530 000 feuilles. Le palier étroit $s\leq 16$ couvre donc presque tout. Les rares feuilles à $s=17$
  dépassent l'orientation native ($7s+9=128$) et passent en voie contrôlée, à un coût négligeable.
- **Le site le plus lointain** ne fixe l'étendue d'une feuille, à deux bits ou plus, que dans au plus 0,0085 % des
  feuilles. Le remède « voie par prédicat » de `NUM-COUVERTURE` est donc inutile sur ces trames, et la subdivision
  suffit. On peut l'écrire au contrat, avec la portée de la mesure : séquence 08, 1 mm.

## 3. Port du socle : table vérifiée mécaniquement

Contrôlé ligne par ligne sur la table de `docs/PORTS.md` (`verifier_ports.py . a0091e2b7 ac081a06f`) :
- les empreintes SHA-256 des sources à `ac081a06f` : 209 conformes, aucun écart ;
- les 28 « copies à l'identique » : égales octet pour octet ;
- les 112 « renommage seul » : 111 égaux au renommage mécanique.

L'exception est `reference/tests.cmake`, qui porte en plus les trois portes de `WIT-T1-CARRE` (0, mutant 4, refus 2,
avec leurs jumelles `-O`). La table et son bilan (404 portes dans la v11, 406 dans le socle) ne les comptent pas
(`CST-0115`, documentaire).

La renumérotation des raisons (13 retirées) est sans effet : `Status` ne compare que leur ordre relatif, qui est
conservé. Les refus des profils 18 et 32, à la configuration comme à la compilation, ont leurs portes et leurs
mutants.

## Non établi

- Pas de relecture ligne à ligne des 5 475 lignes portées, au-delà de la vérification mécanique.
- Aucune porte C++ ni aucun sanitizer rejoués ici.
- Le banc CUDA n'est jugé que sur sa règle écrite d'avance, qui est complète et décide avant mesure.
