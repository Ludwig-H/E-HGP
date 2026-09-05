# MEB différée : obligation locale levée

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict : le delta de matérialisation différée conserve la MEB locale et son contrat de refus dans le domaine annoncé. Aucun défaut reproductible trouvé.** La preuve algébrique est valide et reçoit ici un juge supplémentaire : centres et puissances calculés par élimination de Gram sur les rationnels arbitraires de Python, indépendamment des formules q3/q4 du produit. Le constructeur peut considérer cette obligation locale comme satisfaite pour poursuivre la qualification intégrée et la mesure C/D. Il n'est pas nécessaire de rouvrir une recherche d'algorithme de MEB pour justifier ce delta.

Source auditée : `e6d33698e62ebecf74dff01c16d7de17149d7a4e`, dépendances précisément épinglées dans les reçus. La lecture ciblée du manuscrit porte sur les pages PDF 110–117, notamment les définitions 25–28 et le Fait 12. La lecture intégrale des parties I et II est portée par l'audit principal de cette session. Sources du delta : [note de conception](../docs/OPTIMISATION_MEB_DIFFEREE.md), [grand-livre arithmétique](../docs/ARITHMETIQUE_PRIMITIVES.md), [Builder::miniball](../src/forest/silent_incidence.hpp), [q3](../src/lanes/q3.hpp), [q4](../src/lanes/q4.hpp), [clés](../src/lanes/keys.hpp) et [porte différentielle permanente](../tests/meb_lazy_gate.cpp).

## 1. Pourquoi le premier support contenant suffit

Le Fait 12 du manuscrit donne l'unicité de la plus petite boule englobante et un support d'au plus quatre points en dimension trois. La caractérisation utilisée par le code est suffisante sans supposer la position générale de tout le nuage : si une circumboule contient l'ensemble local et si son centre est une combinaison convexe de ses points de support, elle est minimale. En effet, pour des poids positifs de somme un et de barycentre c, l'identité suivante vaut pour tout autre centre y :

$$\sum_{s\in S}\lambda_s\lVert s-y\rVert^2=R^2+\lVert c-y\rVert^2.$$

Toute boule contenant S a donc un rayon au moins R, avec égalité seulement en c. Les supports minimaux sont affinement indépendants et strictement positifs : q2 est diamétral, q3 est un triangle strictement aigu, q4 un tétraèdre strictement bien centré. Le parcours q2/q3/q4 du code énumère ces possibilités et peut accepter la première qui contient tous les sites. Les critères de positivité restent inchangés dans le delta.

La régularité intervient ensuite pour l'essentialité exigée par la descente silencieuse. Le contrôle `shell != q` refuse une coquille contenant des points supplémentaires, même quand la boule minimale a été correctement calculée. Ce refus est cohérent avec le domaine régulier déclaré ; il ne signifie pas que la MEB géométrique n'existe pas.

## 2. La puissance brute donne exactement la même décision

Pour q3, la source développe une même expression avant et après normalisation :

$$G\lVert z-a\rVert^2-W\cdot(z-a)=G\lVert z\rVert^2-(2Ga+W)\cdot z+G\lVert a\rVert^2+W\cdot a.$$

Pour q4, la canonisation simultanée du déterminant et des numérateurs assure un déterminant positif avant le prétest :

$$\Delta\lVert z-a\rVert^2-2N'\cdot(z-a)=\Delta\lVert z\rVert^2-2(\Delta a+N')\cdot z+\Delta\lVert a\rVert^2+2N'\cdot a.$$

`ball_key_reduce` divise les coefficients par un PGCD g strictement positif, avec g au plus A. La puissance brute est donc g fois celle de la clé primitive. Signes et zéros coïncident. Le refus anticipé reste strictement `power > 0` : un point sur la sphère doit survivre, notamment tout sommet du support.

Par induction sur le même ordre des supports, chaque prétest négatif écarte exactement un candidat qu'aurait refusé l'ancien `accept` avant toute écriture du résultat. Chaque prétest positif construit la même clé et le même niveau, puis traverse encore le contrôle de contenance final. Le premier support retenu et les champs du résultat restent identiques. Le niveau q4 garde ses numérateur et dénominateur non réduits ; cette identité de représentation est vérifiée séparément de l'égalité rationnelle par la sonde indépendante.

## 3. Domaine, débordements, budgets et diagnostics

Les préconditions réellement utilisées sont des positions distinctes dans `[0,65535]^3`, de deux à onze indices locaux valides, et des formes calculées depuis les mêmes supports. `Builder::run` refuse l'index invalide ou les positions dupliquées ; les catalogues vérifiés imposent un ordre constant de 1 à 10 et des identités distinctes. Le cas K=1 sort avant ces calculs. La descente ajoute un intrus étranger ou remplace un support par un intrus étranger, donc conserve cette borne locale et l'unicité des sites. Les appels directs à `miniball` avec `n>11`, des indices invalides ou une forme fabriquée hors domaine ne sont pas une API géométrique validée.

En posant M=65535, les bornes du grand-livre couvrent les intermédiaires écrits : pour q3, les trois termes Wv et leurs sommes puis la soustraction restent sous `135*M^6 < 2^104`; pour q4, la somme N'v, son double et la soustraction restent sous `576*M^5 < 2^90`. Ils tiennent en i128 signé. Les coordonnées et leurs différences restent en i64 avant ces multiplications promues. La réduction PGCD ne peut produire de cast signé hors domaine puisque `0 < g <= A`. La preuve ne repose sur aucune compensation après un débordement.

Les charges `meb_supports` restent placées avant l'examen de chaque support, y compris les candidats non positifs. Au plus 55+165+330=550 supports sont examinés pour onze points. La charge refuse prospectivement quand le compteur a atteint le cap, avec `resource_exhausted / silent_meb_support_budget`. La sonde vérifie cap zéro, cap immédiatement inférieur au support accepté, cap exact et cap supérieur, ainsi que la conservation de tous les champs d'un résultat sentinelle quand aucun support n'a été accepté. `meb_calls` est incrémenté une fois par appel.

Le refus `unsupported_degeneracy / silent_local_nonessential_shell` conserve la boule trouvée dans l'objet local, comme avant le delta. L'effacement des événements au niveau de `build_silent_cofaces` demeure une obligation de la composition, vérifiée par les portes intégrées ; l'absence d'événements dans un appel local ne serait pas une preuve transactionnelle.

## 4. Qualification indépendante exécutée

La [sonde rationnelle](meb_rational_oracle_20260905.py) résout les centres par élimination de Gauss sur la matrice de Gram et décide la positivité par les coefficients barycentriques. Elle reconstruit la clé primitive par dénominateur commun et PGCD Python. Le [pont C++](meb_oracle_bridge_20260905.cpp) n'effectue que les appels au vrai produit et leur sérialisation. Aucun prédicat produit, OBig, centre flottant, formule de niveau q3 ou adjugée de Cramer n'est utilisé pour décider la référence. La représentation brute q4 est reconstruite depuis le déterminant calculé par élimination et le centre rationnel.

| Observation | Résultat |
| --- | --- |
| Ensembles locaux, tailles 2 à 11 | 89 |
| Appels MEB jugés | 431 |
| Refus prospectifs de cap | 164 |
| Refus de coquille non essentielle | 6 |
| Résultats contenant de support q2/q3/q4, refus de coquille compris | 81 / 138 / 48 |
| Puissances brutes et primitives q3 / q4 | 3 096 / 3 080 |
| Puissances négatives / nulles / positives | 898 / 3 323 / 1 955 |
| Cas de réduction primitive non triviale | 5 684 |
| Supports singuliers exclus par le juge avant appel aux primitives | 2 |
| Mutants q3 et q4 `reject-shell` distingués du nominal | 2 / 2 |

Les derniers mutants sont activés dans le vrai registre `MHGP7_TESTING`, chacun sur sa fixture minimale de support. Ils font passer l'appel MEB d'un succès indépendant confirmé à `invariant_violated / silent_no_local_miniball`. Le pilote d'audit rend 0 lorsqu'il observe cette divergence attendue ; il ne prétend pas que le pont a rendu le code processus 4. Les portes CTest permanentes fixent leur propre contrat de code 4. Le mutant de coût `eager-materialization` est déjà couvert par la porte permanente et ses compteurs logiques ; cette sonde mathématique supplémentaire ne refait pas cette qualification.

Le corpus contient les extrêmes u16, un carré cocyclique, une famille colinéaire, un triangle de déterminant de Gram très petit et 80 ensembles issus d'un flux entier fixé. Les puissances des supports q3/q4 non positifs mais de rang plein sont également jugées ; la preuve de signe ne requiert pas le bien-centrage.

Compilations C++20 avec `-O1 -g -Wall -Wextra -Wpedantic -Werror -fsanitize=undefined -fno-sanitize-recover=all`, sans diagnostic. Les deux exécutions suivantes ont rendu **0**, avec les mêmes résultats :

```bash
python3 morsehgp3D_v7/audits/meb_rational_oracle_20260905.py
python3 -O morsehgp3D_v7/audits/meb_rational_oracle_20260905.py
```

Les gardes Python n'utilisent pas `assert`. Reçus [normal](receipts_20260905/meb_rational.json) et [optimisé](receipts_20260905/meb_rational_optimized.json), avec commandes, versions, hashes avant/après, état du worktree et hash du binaire. Les [entrées et sorties brutes](receipts_20260905/meb_rational_raw.json) rendent les résultats inspectables sans exécuter la sonde. Chaque exécution contrôle que ses sources n'ont pas changé pendant son calcul.

## 5. Suite constructive

Le delta n'ajoute aucune structure globale, coface ou mosaïque : il supprime des constructions de clés/niveaux pour des candidats locaux déjà rejetables. La borne de 550 est locale ; elle ne borne pas le nombre de MEB, les longueurs des chaînes ni le volume global de facettes. La passe de prétest sur le candidat accepté ajoute aussi du travail : la preuve de conservation ne prédit pas le gain de temps.

La [qualification intégrée D](AUDIT_QUALIFICATION_20260905.md) est désormais contre-vérifiée et la [reconstruction indépendante](receipts_20260905/release/summary.json) passe 323/323 portes CPU. La comparaison C/D du constructeur garde son protocole et ses objets propres ; cet audit mathématique ne la transforme pas en qualification du coût complet. Le [prétest q2 E postérieur](ADDENDUM_MEB_Q2_E_20260905.md) reçoit sa contre-épreuve locale distincte. Pour la revendication industrielle, les obligations globales horizontales, la verticale, les coûts à 50k puis massifs et l'exécution GPU restent celles de leurs contrats propres. Cet audit lève le verrou précis de la MEB différée ; il n'ajoute aucune réserve mathématique nouvelle à ce delta.

Travail écrit exclusivement dans `morsehgp3D_v7/audits/`. Aucun code produit modifié. **GCP non utilisé.**
