# Note de Claude — l'enveloppe projective, et les deux défauts que les portes ont trouvés

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Suite de
[`AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`](AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md),
section « Déblocage mathématique prioritaire ». Les crédits cellulaires sont
implémentés, l'enveloppe projective aussi, et le certificat ferme enfin. Aucune
réception n'est demandée.

## 1. Le mur était d'implémentation, et c'est mesuré

Le test « `r` appartient-il au cône de `G` ? » par énumération de Carathéodory
coûte `O(m^3)`. À `pool=16` il plafonne à cinq crédits disjoints, quand q4 en
exige huit : le certificat fermait **zéro**. L'ablation le chiffre exactement.

| pool | crédits max par cellule | q4 dirigé fermé, `eight_clusters n=300` |
| ---: | ---: | ---: |
| 16 | `5` | `0` |
| 32 | `10` | `537` |
| 48 | `10` | `5 595` |

L'enveloppe n'est donc pas une optimisation : c'est le préalable à toute mesure
de ce certificat, exactement comme l'audit l'annonçait.

## 2. Ce que l'enveloppe est, et pourquoi elle est exacte

Tous les membres du pool vérifient `m_C(s) > 0`, donc `w.s > 0` avec
`w = r_0+r_1+r_2` : ils vivent dans un même demi-espace ouvert et se projettent
sur la carte `w.u = 1`. La question devient « le projeté de `r` est-il dans
l'enveloppe convexe des projetés ? », c'est-à-dire un problème plan.

Les dénominateurs `w.s` étant strictement positifs, l'orientation projective de
trois vecteurs a **exactement le signe de `det(s_i,s_j,s_k)`**. La marche de
Jarvis n'a besoin que de ce prédicat : aucune division, aucune racine, aucun
flottant, et un coût `O(m h)` au lieu de `O(m^3)`.

## 3. Deux défauts, tous deux trouvés par une porte

**Le départage des colinéaires.** J'ai comparé le chemin par enveloppe à
l'énumération exhaustive sur `10 794` tests de rayon : désaccord. La marche de
Jarvis doit, sur une arête portant plusieurs points, retenir le plus **éloigné**
du sommet courant ; ma première version comparait la coordonnée de la carte, ce
qui n'est correct que si l'arête n'est pas parallèle à l'autre axe. La marche
sautait un sommet, l'enveloppe excluait une région, et le certificat perdait des
crédits sans que rien ne le signale. Le test exact est conique et sans
division : `i` est plus loin que `next` si et seulement si `next` appartient au
cône de `cur` et `i`, ce qui s'écrit avec deux déterminants dans le plan de
normale `cur x i`.

Après correction : `3 598` pools, `10 794` accords, `3 135` inclusions — la
porte n'est pas vacueuse.

**Le rang du carrier.** L'enveloppe rendait systématiquement le triangle de
l'éventail, donc trois `PointId` par crédit : `rang3` à `123 022` contre `824`
en rang deux, là où l'énumération exhaustive trouvait `7 885` carriers de rang
deux. Or un crédit **consomme** ses identifiants, et c'est leur nombre qui borne
les crédits disjoints, donc la fermeture. Un test de rang un en `O(h)` et la
lecture des poids nuls de Cramer récupèrent les bas rangs sans rien coûter : sur
`terrain`, le rang deux passe de `2 007` à `40 506`.

Ce second défaut n'était visible que dans le ledger. Aucun juge ne l'aurait vu :
le certificat restait parfaitement correct, seulement plus pauvre.

## 4. Le falsificateur porte sur la conclusion, pas sur l'hypothèse

Un crédit ne fournit **aucun témoin universel**. Il fournit un intérieur par
sphère, potentiellement un membre différent à chaque sphère. Le juge ponctuel ne
peut donc pas confirmer une fermeture, et l'employer ainsi serait une faute de
raisonnement, pas une approximation.

Ce qui est réfutable est la conclusion : si la lane est fermée pour `(a,b)`,
**chaque** sphère admissible doit porter au moins son seuil d'intérieurs
stricts. Les centres admissibles sont `(a+b)/2 + t` avec `t.d = 0` ; la porte en
échantillonne le long d'une base entière du plan, magnitudes de `0` à `401` des
deux signes. Mesure : `3 400` sphères, minimum `29` intérieurs pour un seuil de
`8`, zéro désaccord.

## 5. Mutants : un seul armé sur quatre

`credit-ids-partages` meurt — `17` désaccords. C'est la faute qui casse le
compte : deux crédits servis par le même site ne donnent qu'un intérieur.

`credit-activation-frontiere`, `credit-un-seul-rayon` et
`credit-sans-positivite` **survivent** sur les familles génériques. La cause est
mesurée et c'est la même que pour la dominance et le cœur : les paires que le
certificat ferme portent bien plus d'intérieurs que le seuil — minimum observé
`29` contre `8`. Une perturbation doit donc franchir une marge de vingt et un
intérieurs avant d'être visible.

Je ne livre pas de porte vacueuse pour ces trois-là. Les armer demande des
nuages gravés serrés, où les intérieurs du crédit sont exactement au seuil, et
c'est un travail que je n'ai pas fait.

## 6. Question

Pour armer ces trois mutants, deux voies me semblent possibles et je ne sais pas
laquelle vous jugez recevable.

La première grave un nuage par mutant, comme `--cloud-seuil` l'a fait pour la
dominance : sept témoins colinéaires exactement, un de moins que la lane
n'exige. C'est ce qui a marché, mais chaque mutant demande sa propre
construction et le nuage n'a plus rien d'un nuage.

La seconde change la porte plutôt que le nuage : au lieu d'exiger qu'un mutant
produise une fermeture fausse, exiger qu'il produise une **fermeture
différentielle dont la marge d'intérieurs est minimale**, et refuser si cette
marge est identique à celle de la référence. Cela testerait que le mutant touche
bien le prédicat, sans exiger qu'il franchisse une marge que la géométrie du
nuage rend inaccessible.

La seconde me paraît plus honnête — elle mesure ce que le mutant change au lieu
d'espérer qu'il casse — mais elle n'est plus un mutant tué au sens du dépôt, et
je ne veux pas affaiblir la convention sans votre accord.

## 7. Non-claims

Aucune pente, aucun octet, aucun high-water. La boucle de mesure du ledger reste
en `n(n-1)` et n'est pas l'ordonnance. Le résiduel n'est ni matérialisé ni
consommable. La fermeture porte sur des candidatures d'arête maximale. Le
raccord factorisé en rectangles `A x B` de votre section « Raccord factorisé »
n'est pas écrit. Le contrat `50 000` reste entièrement ouvert, et G4 reste
NO-GO.
