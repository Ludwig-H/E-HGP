# Comptages WSPD : réemploi, valeur q2 et non-crédit de blocs

6 septembre 2026, lecture depuis 4931906b. Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed. Aucun C++, moteur ou benchmark lancé. Les mesures et compilations constructeur restent attribuées à leurs captures.

**Le terminal en un passage est mathématiquement justifié avec le compteur actuel. Deux propriétés sont nécessaires : domination du compte avec coins et indépendance de chaque lane vis-à-vis du masque demandé.** Le contrôle d’un cœur q2 strictement positif, demandé pendant cette contrelecture, est désormais ajouté au gate permanent : sa modification est vérifiée statiquement.

## 1. Deux lemmes sur le compteur inchangé

Pour deux facteurs disjoints, les mêmes boîtes A/B, les mêmes populations exclues A∪B et les mêmes seuils h_q, noter C_q⁻ le compte sans coins et C_q⁺ celui avec coins, après écrêtage. Le [compteur](../../src/spindle/witness_count.hpp) vérifie C_q⁻≤C_q⁺ ; en q2, C₂⁻=C₂⁺.

Sans coins, q3/q4 créditent les points extérieurs aux facteurs qui entrent strictement dans leur boule-cœur arrondie. Activer les coins conserve tous ces crédits et autorise d’autres témoins aux feuilles. Un rayon nul retire une lane seulement du passage sans coins. Les masques de sous-arbre empêchent le double crédit ; atteindre le seuil plus tôt donne toujours la même valeur écrêtée h_q. q2 n’utilise pas cette autorité supplémentaire.

Le second lemme est **l’indépendance du compte d’une lane demandée par rapport aux autres bits du masque**. Chaque lane a son propre compteur, son propre seuil et son bit par sous-arbre. Un crédit retire uniquement ce bit. L’arrêt global n’interrompt pas une lane encore sous son seuil : tant qu’elle a du travail, son parcours reste dans la pile ou elle a déjà reçu le crédit du nœud parent. Supprimer d’autres lanes peut éviter des visites inutiles pour elle, sans changer les témoins qu’elle acquiert. Le partage H/Ξ aux coins arrête une lane seulement après son propre échec ; prolonger la boucle pour l’autre lane ne réactive pas la première.

Ces arguments portent sur le nominal, sans mutant de masques, sur un index valide et des additions représentables. Ils conservent les populations et les arrondis du code. Ils ne remplacent pas les preuves géométriques du [front](../FRONT_ET_TEMOINS_COURANT.md).

## 2. Équivalence du terminal

Dans le chemin à deux passages, une lane meurt si C_q⁻≥h_q ; sinon elle meurt au second passage si C_q⁺≥h_q. Par domination, l’union de ces deux cas est exactement C_q⁺≥h_q. L’indépendance du masque garantit que le second passage sur les seules survivantes donne le même C_q⁺ que le passage unique sur tout le masque d’entrée.

Par conséquent, le passage unique conserve les lanes vivantes et leurs crédits. Aux tâches non terminales, garder le passage sans coins conserve la subdivision ; l’induction sur les vagues conserve alors rectangles, ordre, masses émises/tuées et tailles prospectives des vagues, si l’assemblage des shards reste inchangé.

Deux détails du port sont nécessaires :

- Verser la masse tuée une seule fois par lane et tâche.
- Laisser core[q]=0 pour une lane morte. Copier indistinctement les trois comptes écrêtés dans un rectangle vivant changerait ses champs hors masque.

Ne pas additionner C⁻ et C⁺ : leurs témoins se recouvrent. Le nouveau passage remplace l’ancien résultat terminal. La même indépendance du masque justifie le réemploi q2 déjà intégré ; retirer q2 du second parcours ne change pas les comptes q3/q4 ni leurs évaluations de coins.

**Aucune domination du coût n’en découle.** Le passage unique peut payer des coins sur une lane auparavant tuée par le passage économique. Il calcule aussi la séparation sur les tâches auparavant éliminées avant ce test. Les visites et coins peuvent augmenter ou diminuer ; les compteurs doivent sommer uniquement les appels exécutés. L’équivalence géométrique ne prédit pas le comportement d’une allocation défaillante ou un temps mural.

## 3. Pourquoi la domination seule ne suffit pas

Le [contre-modèle permanent](countermodels.py) fixe deux lanes avec h=(2,2). Sur leur masque commun, un compteur fictif donne C⁻=(2,0) et C⁺=(2,1). Sur le singleton de la seconde lane, il donne C⁻=0 et C⁺=2. La domination tient pour chaque masque, mais les deux passages tuent les deux lanes tandis que le passage unique en garde une. C’est une réfutation de la seule prémisse de domination, **pas une erreur du compteur v7**, dont l’indépendance vient d’être démontrée.

## 4. Fixer une valeur q2 positive dans le gate

Le différentiel constructeur clos compare littéralement les rectangles et leurs cœurs ; il protège donc le transfert de valeur. La porte permanente initialement lue (tests/wspd_terminal_reuse_gate.cpp), au hash 81a8657a…, contrôlait les masses, les masques, les cœurs sous seuil et le passage n2 de six à trois visites. Sa seule valeur de cœur explicitement fixée était zéro sur cette paire sans témoin.

Omettre seulement l’affectation ff.c[0]=fc.c[0] laisserait tous les cœurs q2 survivants à zéro. Les assertions de cette version resteraient satisfaites : le premier passage a déjà tué les lanes q2 saturées ; pour les autres, zéro reste sous le seuil, sans changer masques, masses, nombres de rectangles ou visites. Zéro reste un minorant géométriquement sûr, mais perd le crédit utile en aval et l’identité des valeurs promise par ce réemploi. C’était une lacune de cette version du test permanent, pas un défaut présent de l’affectation nominale ni une invalidation du différentiel.

Le correctif demandé tient dans une fixture existante : scène 1, s8, masque 1, threshold=1 donc h₂=10, le rectangle de feuilles (-1,-3) relie (0,0,0) à (20,0,0). Son seul témoin diamétral est (10,0,0), donc core[0]=1. Les points (30,0,0) et (40,0,0) sont extérieurs. Exiger que ce rectangle soit trouvé une fois et que son cœur vaille un couvre l’oubli de copie ; le test n2 conserve son rôle de contrôle du coût.

**Réponse du constructeur contre-lue avant publication de cet audit :** le gate courant 35d28f2c… contient maintenant ce contrôle de valeur, l’unicité du rectangle et q2_positive_core_checks==1. La lacune est donc levée par lecture du code. Les captures précédentes restent attribuées au gate 81a8657a… ; aucun nouveau run ou mutant C++ n’est qualifié par cette levée statique.

La contre-fixture géométrique minimale du programme utilise seulement X={(0,0,0),(1,0,0),(2,0,0)}, s8, q2 et h₂=2. Les trois paires sont des terminaux de feuilles ; la paire extrême a un témoin strict, les adjacentes aucun. Sa sortie attendue porte trois unités de masse émises et les crédits 0,1,0. Le modèle vérifie aussi que le mutant de valeur zéro passe l’ancienne condition locale « cœur sous seuil » et échoue sur l’égalité proposée. Il n’exécute ni le front C++ ni son gate.

## Décision de coût communiquée pendant la contrelecture

Le constructeur a ensuite annoncé un différentiel correct sur 174 fronts et les 754 686 rectangles d’un cas uniforme 8k, mais n’a pas retenu le terminal unique pour intégration. Sa paire front seul O2 donne 37 767,10→38 286,55 ms, 563 616 452→547 864 549 visites et 167 115 088→335 509 837 coins. Le compromis prévu se manifeste : moins de visites ne suffit pas à payer les coins supplémentaires. Cette annonce provient du dialogue de coordination ; ce lot ne relit pas encore le paquet négatif complet et ne réattribue pas ces temps à une exécution indépendante. La preuve d’équivalence demeure, sans recommandation de remplacer systématiquement le chemin courant.

## Variante constructive : conserver la frontière du premier passage

Le négatif ne condamne pas toute réutilisation du parcours. Pour q3/q4, le passage économique pourrait conserver les sous-arbres où il retire un bit uniquement parce que la boîte est extérieure à la boule-cœur. Pour un rayon nul, la frontière initiale est la racine. À une vraie feuille de CloudIndex, la boîte est ponctuelle : la distance proche égale la distance lointaine, donc box_vs_ball ne retourne jamais le cas mixte ; les nœuds extérieurs couvrent aussi les feuilles non créditées. Les rejets hmax≤0 et les positions des facteurs restent définitifs.

Par lane, enregistrer seulement les nœuds où l’on arrête sa descente produit une frontière sans relation ancêtre–descendant. Elle est disjointe des sous-arbres crédités. Une lane saturée abandonne sa frontière ; pour une survivante, reprendre cette frontière avec le compteur initial C_false complète exactement les témoins avec coins, sans compter deux fois les crédits acquis. q2 n’a rien à reprendre. Des frontières de lanes différentes peuvent se recouvrir : conserver leurs masques et partager les prédicats lorsque possible.

Cela exige une API de reprise : rappeler le compteur actuel à la racine puis lui ajouter C_false serait faux. Cette proposition conserve l’étape économique qui tue des lanes avant les coins ; elle ne garantit aucun gain, car la frontière peut être linéaire, demande du stockage même pour des lanes finalement tuées, et peut réduire le partage des évaluations entre lanes. Aucun prototype ou benchmark de cette variante n’est livré ici.

## Non-crédit de blocs q3/q4 : réponse à la nouvelle question

Fixer une ancre a et un point entier b₀∈Box(B). Pour une boîte de témoins Z, M₄=hmax4_boxes({a},{b₀},Z) vaut quatre fois le maximum de H sur Z : les extrémités sont fixées, donc aucun minimum ambigu sur des ancres variables ne subsiste. Encadrer les composantes du produit vectoriel par les intervalles C_i, puis poser :

$$\Xi_{\min}=\sum_{i=1}^{3}\mathrm{dist}(0,C_i)^2.$$

Cette somme minore Ξ, même lorsque les intervalles perdent leurs dépendances. Si M₄≤0, tous les points échouent dans W2. Sinon, le certificat suivant rejette le bloc comme source de contributions q3/q4, avec t=3 ou 2 :

$$tM_4^2\leq16\Xi_{\min}.$$

En effet, pour tout z avec H(z)>0, tH(z)²≤t(M₄/4)²≤Ξ_min≤Ξ(z), donc le test strict échoue. L’égalité doit bien être rejetée. Un b₀ intérieur à la boîte est valable : si tous ses coins passaient pour ce z, la convexité en b ferait passer b₀. Cela réfute le prédicat utilisé par les histogrammes ; cela ne désigne pas un site réel de B en échec, et **ce n’est pas une mort d’ancre**.

Pour R=65535, |M₄|≤12R², donc tM₄²≤432R⁴<2^73 et 16Ξ_min≤192R⁴<2^72. i64 suffit avant les carrés ; convertir en i128 avant les produits. Ces bornes utilisent un b₀ entier u16 : un point rationnel demanderait sa propre mise à l’échelle.

Le contre-modèle fournit un bloc non vacant proche d’une extrémité : a=(0,0,0), b₀=(100,0,0), Z de coins extrêmes (1,4,0) et (2,5,1). On obtient H_min=73>0, M₄=720, Ξ_min=160000. Toute la boîte est dans W2, mais le certificat q3/q4 la rejette. A formé de a et des huit coins de Z est séparé de B={b₀} au paramètre s8. Deux frontières exactes vérifient le sens non strict du rejet.

Un mutant emploie au contraire la borne **supérieure** de Ξ déjà utile au crédit : sur Z de coins (1,0,0) et (2,5,1), il rejette à tort alors que (1,0,0) est un vrai témoin. Le juge le réfute dans les deux lanes. La distance d’un intervalle à zéro doit être zéro lorsqu’il contient zéro ; prendre son extrémité de plus grande valeur absolue ne convient pas ici.

Ce certificat peut éviter une descente jusqu’aux feuilles à l’extérieur du cône q3/q4. Sa non-vacuité est prouvée, son coût face aux coins et aux grands facteurs reste à mesurer. Aucun prototype C++ de ce rejet n’est qualifié ici.

## Reproduction et attribution

Exécuter countermodels.py avec python3 -B, puis python3 -B -O ; stdout est un JSON compact. Les reçus [normal](normal.json) et [optimisé](optimized.json) gardent les résultats des contre-modèles, sans import produit. Le [relevé daté](source_review.json) lie la lecture des sources et captures constructeur. Aucun nouveau variant C++ n’est ajouté au manifeste et aucune performance n’est qualifiée.
