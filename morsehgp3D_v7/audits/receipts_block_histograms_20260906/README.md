# Crédit des histogrammes d’extrémités par blocs

6 septembre 2026. Contrelecture indépendante de la priorité WSPD h/h_a/h_b du constructeur, sur e16e857b. Cadre : phase=exploration_v7_hors_registre, backend=cpu_reference, profile=quantized_u16_input_only, mode=audit_independant_math_and_architecture, public_status=not_claimed. Aucun C++, moteur, benchmark ou GCP lancé. Les preuves prolongent le [front existant](../FRONT_ET_TEMOINS_COURANT.md) ; les juges Python ci-dessous sont des modèles indépendants bornés.

**La piste utile est de créditer des groupes de témoins proches d’une extrémité, puis de partager les listes de B survivantes.** Le certificat strict proposé évite potentiellement des tests du produit A×A/B×B, sans fournir une borne sous-quadratique ni une économie mesurée.

## 1. Les minima globaux ne renforcent pas le cœur

Choisir une paire (a₀,b₀) de distance minimale dans A×B. Tout témoin z crédité dans h_a(a₀) appartient à A et vérifie H=(z−a₀)·(b₀−z)>0, même en q3/q4. Alors :

$$\lVert b_0-z\rVert^2=\lVert b_0-a_0\rVert^2-\lVert z-a_0\rVert^2-2H<\lVert b_0-a_0\rVert^2.$$

C’est impossible par minimalité. Donc h_a(a₀)=h_b(b₀)=0 : les deux minima globaux sont nuls, simultanément sur une même paire. Cette preuve n’utilise ni séparation WSPD ni unicité de la paire la plus proche. Une lane encore vivante ne peut donc être éliminée en ajoutant ces minima au cœur.

En revanche, pour A′⊆A et B′⊆B non vides, conserver les histogrammes calculés sur les **populations originales** donne :

$$p\geq h_{\mathrm{coeur}}+\min_{a\in A'}h_a(a)+\min_{b\in B'}h_b(b).$$

Les populations sont respectivement X∖(A∪B), A∖{a} et B∖{b}, donc disjointes. Le seuil h_q=smax−q+1 tue tout le sous-bloc lorsqu’il est atteint. Exemple sur un axe : A={0,1}, B={10,11}, cœur nul. Les crédits valent h_a(0)=h_b(11)=1, h_a(1)=h_b(10)=0. Le sous-bloc {0}×{11} meurt au seuil 2, dans les trois fuseaux. Raffiner le cœur contre les nouvelles boîtes reste sûr **en excluant toujours A∪B**. Exclure seulement A′∪B′ pourrait recompter les témoins déjà présents dans h_a/h_b.

## 2. Certificat strict d’un bloc de témoins

Soient U une boîte d’ancres dans A, T=Box(B), et Z une boîte de témoins du facteur A. Le [minimum exact existant](../../src/spindle/spindle.hpp) donne H_min sur U×T×Z. Pour chaque axe i, former les intervalles D_i=T_i−U_i et V_i=Z_i−U_i. Encadrer chaque composante du produit vectoriel par C_i=D_j V_k−D_k V_j, avec permutations cycliques, puis poser :

$$\Xi_{\max}=\sum_{i=1}^{3}\max\bigl(\lvert\inf C_i\rvert,\lvert\sup C_i\rvert\bigr)^2.$$

Les conditions H_min>0 et t H_min²>Ξ_max, avec t=3 en q3 et t=2 en q4, impliquent l’appartenance stricte au fuseau pour **tout** (a,b,z) du produit de boîtes. Les dépendances perdues entre intervalles ne peuvent qu’affaiblir ce certificat. Un échec signifie « descendre ou tester les points », jamais « créditer quand même ».

Sous u16, avec R=65535, chaque différence est bornée par R, chaque extrémité d’intervalle de produit vectoriel par 2R², Ξ_max≤12R⁴ et 3H_min²≤27R⁴. Les différences, produits simples et H_min tiennent en i64 ; les carrés et comparaisons tiennent en i128. Convertir avant les multiplications au carré. Les égalités sont refusées : aucun témoin du shell n’est acquis.

Autre certificat, plus coûteux : tous les coins de U×T×Z. À a,b fixés, écrire m=(a+b)/2. La condition de fuseau est la stricte négativité de :

$$\lVert z-m\rVert^2+\frac{\lVert(b-a)\times(z-a)\rVert}{\sqrt{t}}-\frac{\lVert b-a\rVert^2}{4}.$$

Cette fonction est convexe en z. Avec les convexités séparées en a et b déjà prouvées, vérifier successivement les coins étend le prédicat à tout le produit. Cela demande jusqu’à 64 tests pour a fixé, 512 pour U variable ; H et Ξ peuvent servir les deux lanes. Aucune convexité conjointe n’est nécessaire.

## 3. La boule-cœur centrale serait vacue ici

Réutiliser directement core_ball(U,B) pour les témoins z∈A ne peut pas créditer h_a en q3/q4 sur les rectangles séparés s≥8. Soient D la distance des centres de A et B, r_A le rayon de Box(A), et c_U le centre de U⊆Box(A). Le centre de cette boule est m_U=(c_U+c_B)/2, donc :

$$\lVert z-m_U\rVert\geq\frac{D}{2}-\frac{3r_A}{2}.$$

Les deux rayons minorants du code, avec leurs arrondis dirigés, sont au plus κ_q|c_U−c_B|≤κ_q(D+r_A). Or κ₃=1/(2√3)<3/10 et κ₄=sin(15°)<3/10. Le [test de séparation](../../src/wspd/wavefront.hpp) impose D≥(s+2)r_A≥10r_A. La distance moins le rayon est donc strictement supérieure à D/5−9r_A/5≥r_A/5 ; si r_A=0, D>0 assure aussi la stricte positivité. Aucun point de A n’entre dans la boule. La conclusion est symétrique pour h_b.

Cela explique pourquoi le certificat H_min/Ξ_max est pertinent : il atteint les fuseaux près des extrémités. La boule-cœur reste utile pour ses témoins extérieurs habituels. Cette impossibilité vise sa réutilisation directe dans les histogrammes, pas tous les certificats de boîtes.

## 4. Première implantation et deux pièges de quantificateurs

La première variante simple fixe a, parcourt le sous-arbre original A et ajoute le nombre de positions d’un nœud Z certifié ; sinon elle descend et reprend universal_over_corners(a,B,z) aux feuilles. Les nœuds crédités ne sont plus parcourus. On peut ainsi retrouver exactement les histogrammes actuels, éventuellement saturés au seuil. Le [code actuel](../../src/pipeline/generate.hpp) compte les positions par incrément unitaire, pas leur multiplicité : conserver cette unité ; le profil régulier exige des positions distinctes.

Une variante à deux arbres crédite uniformément U×Z et utilise des additions sur l’intervalle Morton des ancres U. Elle doit partitionner les paires ordonnées ancre–témoin, sans diagonale ni double crédit par lane. Si les boîtes U et Z se rencontrent, leur produit continu contient z=a et H=0 : soustraire seulement le nombre d’ancres ne rend pas un certificat strict uniforme possible. Il faut descendre ou séparer les blocs.

**hmax4_boxes(U,T,Z)≤0 ne permet pas de jeter Z pour toutes les ancres individuelles de U.** Il prouve l’absence de témoin universel sur tout U×T. Une ancre défavorable suffit à ce minimum ; d’autres peuvent avoir des témoins. La contre-fixture entière prend les ancres (0,0,0) et (3,3,0), le témoin (4,0,0), et B={(100,100,0)}. Leurs clés Morton 0,27 et 64 permettent deux sous-arbres disjoints dans A ; la boîte du facteur A est [0,4]×[0,3]×{0} et sa séparation s8 avec B est vérifiée. Le majorant vaut −816, mais la première ancre a H=384 et Ξ=160000, donc un témoin q3 et q4 ; la seconde a H=−204. Jeter tout Z perdrait la première contribution. Ce contre-exemple porte sur le prédicat de témoins, pas sur une forêt FULL régulière q4. Avec a fixé, le rejet actuel est valable.

Le premier chemin à a fixé évite cette difficulté. Garder la boucle scalaire sur les petits facteurs est raisonnable : le certificat, ses échecs et les descentes ont un coût. Mesurer séparément nœuds visités, crédits de blocs, tests ponctuels et paires logiques couvertes. Le compteur historique p_factor=nA(nA−1)+nB(nB−1) ne représenterait plus les tests réellement payés après ce changement.

## 5. Listes stables par seuil et saturation

Après le cœur, poser need=h_q−h_coeur>0. Une ligne a survit seulement si h_a(a)<need et utilise exactement la liste L_t={b : h_b(b)<t}, avec t=need−h_a(a). Construire les listes nécessaires dans l’ordre Morton de B, puis garder **l’ordre original de A**, préserve la suite de paires et leurs crédits. Les lanes sont déjà traitées séparément par la boucle extérieure. Ne pas regrouper les lignes A par crédit : cela change le préfixe d’émission.

La saturation h↦min(h,need) préserve les rejets ; une paire survivante a ses deux valeurs strictement inférieures à need, donc ses crédits transmis restent exacts. Avec R le nombre de lignes h_a≥need, les comptes sans arrêt anticipé deviennent : lignes tuées=nB·R, paires tuées par seuil=Σ_{a vivant}(nB−|L_t|), survivants=Σ_{a vivant}|L_t|. Leur somme est nA·nB.

Pour smax≤11, need≤9 en q3 et ≤8 en q4. Les listes peuvent économiser les tests de seuil répétés ; seules, elles ne réduisent pas p_factor. Arrêter tôt le calcul d’un histogramme saturé réduirait au contraire le travail géométrique, qui doit alors être compté réellement. Au plus need·nB indices temporaires par worker doivent être pris en compte ; les listes peuvent être construites seulement pour les seuils présents et les gros facteurs. Ce stockage ne fait pas partie automatiquement du budget existant de candidats.

Conserver les contrôles d’arrêt du corps d’émission. En mono, la stabilité des paires permet de préserver le préfixe ; en parallèle, on conserve le contrat d’overshoot existant, pas une identité de préfixe entre ordonnancements. Le juge de listes vérifie les paires et leur comptabilité, pas l’ordonnanceur C++ ni les émissions géométriques de chaque ancre.

## Reproduction

Les programmes [certificat](block_certificate_probe.py) et [listes stables](stable_threshold_probe.py) n’importent aucun helper produit et n’utilisent pas assert. Exécuter chacun avec python3 -B, puis python3 -B -O depuis la racine ; les résultats JSON vont sur stdout.

- [Blocs normal](block_normal.json) / [optimisé](block_optimized.json) : codes 0, octets identiques ; 39 460 triples exacts de coins et milieux, 278 621 contrôles, positifs non vacants q3/q4, frontières strictes et contre-fixture hmax. Deux ancres réelles reçoivent chacune deux témoins dans la fixture proche d’une extrémité. La grille finie vérifie le modèle arithmétique ; elle ne remplace pas la preuve continue.
- [Listes normal](stable_normal.json) / [optimisé](stable_optimized.json) : codes 0, octets identiques ; 54 modèles pour need=1..9, 108 comparaisons, 126 paires tuées par lignes, 203 par seuil et 211 survivantes. Le mutant qui regroupe A conserve les comptes et le multiensemble mais change le premier survivant : il est rejeté sur l’ordre.

Le [relevé de sources](source_review.json) borne la lecture ; les variantes C++ D–O demeurent inchangées. Aucun gain de latence ou contrat industriel n’est acquis par ces modèles.
