# Une famille q4 : événements exacts et profondeur partagée

20 septembre2026. Tranche22 qualifiée dans le périmètre ci-dessous, `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Première brique q3/q4, pas un générateur complet ni FULL.

## Objet calculé

Fixer trois sites formant un triangle strictement aigu. Toutes les sphères
passant par ces sites forment une famille à un paramètre. Chaque autre
site non coplanaire rencontre exactement une sphère de cette famille.
Au lieu de recompter tout le nuage pour chaque rencontre, produire les
événements, les trier et mettre à jour le compte d'intérieurs.

La première interface consomme **tous les sites** du propriétaire immuable
`PreparedCloud`. Aucune hypothèse de cover ou de corde n'est nécessaire.
La seed n'est pas conditionnée par son acceptation en q3. Une rencontre
ne certifie ni un tétraèdre positif, ni son propriétaire, ni son admissibilité
dans la tour. Ces décisions restent des étapes suivantes.

## Formes et comparaison

Poser d=b−a, u=x−a, v=z−a, D=d·d, E=u·u, F=d·u, G=DE−F² et
W=E(D−F)d+D(E−F)u. La factory n'admet que G>0 et F,D−F,E−F>0.
Elle possède les coordonnées et ses coefficients, non forgeables.
La puissance de la boule de face a le signe de P(z)=G|v|²−W·v.
Avec B(z)=(d×u)·v, la puissance dans la famille a le signe de
P(z)−μB(z). Pour B non nul, l'événement est μ=P/B.

Le comparateur ne multiplie **pas** P1 par B2 en i128. Ces produits
peuvent dépasser128 bits avant de se simplifier. Il calcule Δ, déterminant
des quatre lignes (d,|d|²), (u,|u|²), (v1,|v1|²), (v2,|v2|²).
L'identité est GΔ=P2B1−P1B2 ; le signe de μ1−μ2 est donc
−sign(Δ)sign(B1)sign(B2). Six mineurs de la seed se préparent une fois.
Δ=0 définit un même événement ; l'ID ne sert qu'à départager son stockage.

Avec M=65535, les différences sont bornées par M. Les bornes conservatrices
D,E,|F|≤3M², G≤9M⁴, |W_i|≤36M⁵ donnent |P|≤135M⁶<2^104.
|B|≤6M³<2^51. La somme des modules des24 termes du déterminant est
au plus72M⁵<2^87. Toutes les promotions précèdent les multiplications.
Ces bornes concernent de vraies coordonnées u16 et des formes construites
par la factory, pas des coefficients arbitraires injectés dans une structure.

Les identités proviennent de la
[stratégie mathématique précédente, §6](Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md),
pas d'une copie du moteur v7 ni d'un transfert de sa qualification.

## Groupes et frontières strictes

- B>0 : entrée, intérieur après l'événement.
- B<0 : sortie, intérieur avant l'événement.
- B=0 et P<0 : intérieur permanent.
- B=0 et P=0 : coquille permanente, notamment les trois sites de la seed.
- B=0 et P>0 : extérieur permanent.

Avant le premier groupe, le compte vaut intérieurs permanents + toutes
les sorties. À chaque groupe égal : soustraire ses sorties, publier le
compte strict, puis ajouter ses entrées. **Ne jamais saturer à K** : le
compte peut redescendre. Les sites du groupe sont sur la coquille, jamais
intérieurs à cet instant, même si entrées et sorties s'y mélangent.

Le callback reçoit l'ID d'un représentant, la profondeur et deux vues
disjointes : sites de cet événement, coquille permanente. Ces vues sont
empruntées pendant le callback seulement. La coquille permanente n'est pas
copiée pour chaque groupe. Aucun intérieur par groupe n'est matérialisé.
Une exception arrête l'appel ; les callbacks déjà effectués ne sont pas
annulés. Aucun résultat partiel n'est annoncé comme un appel complet.

## Travail, mémoire et parallèle

Pour n sites d'UNE seed et m≤n événements, scan O(n), tri O(m log(1+m)),
groupement O(m), mémoire O(n). La restitution parcourt les IDs de chaque
groupe une fois ; si le consommateur recopie toute la coquille permanente
à chaque fois, son coût supplémentaire lui appartient et doit être compté.
La préparation du propriétaire est partagée et mesurée séparément.

Pour S seeds, le coût reste une somme de ces coûts ; S·n peut être trop
grand. La brique n'apporte **aucune borne globale sous-quadratique** au
générateur. Le prochain raccord doit borner/mesurer seeds, covers, résidus,
événements et sorties, pas seulement accélérer ce scan.

Les événements peuvent être produits indépendamment par blocs ; plusieurs
seeds peuvent fournir des segments indépendants. La version initiale
mono est une référence de comparaison, pas une nouvelle équipe de threads
par seed ni une arène contenant toutes les seeds du nuage. Le GPU exigera
encore une arithmétique portable qualifiée, tri et préfixes segmentés,
gestion des groupes égaux coupés entre tuiles, et mesure VRAM/transferts.

## Qualification close

82 CTests Release passent dans un build neuf. La nouvelle gate q4 et
trois sondes n32 passent aussi sous Clang ASan/UBSan ; **la suite complète
des82 tests n'a pas été rejouée sous sanitizers dans cette tranche**.
La gate confronte206 appels sur57 nuages à un oracle rationnel indépendant
par élimination de Gauss :67 354 contrôles, groupes mixtes, profondeur
qui redescend, coplanaires, refus géométriques, permutations et extrêmes
u16. Profondeur35, coquille30 et produit naïf de145 bits sont exercés.
Quatre appels indépendants concurrents sont testés, pas un backend
parallèle de famille ni une nouvelle qualification TSan.

Trois captures ferment15 mesures : six sondes n32 et neuf observations
8k/16k/32k. Pour une famille uniforme, scan+tri+groupes+callback prennent
3,942/8,153/17,464 ms sur un CPU local ; les comparaisons de tri croissent
de×2,083/×2,161 et les buffers de×2. Ce sont des observations de composant,
sans comparaison de gain avec un ancien moteur ni extrapolation à la tour.
Les135 sources sont inchangées avant/après ; dix commandes ferment les
lectures normal/−O et les seize mutations du lecteur.

Les [reçus](../receipts/q4_family_20260920/README.md) et la
[régression](../receipts/reprise_20260920/README.md) distinguent ces preuves
des qualifications historiques. Les builds `build/v8_q4_family_20260920`
et `build/v8_q4_family_sanitize_20260920` sont désormais épinglés : ne pas
les reconstruire pour poursuivre. Le paramètre s n'intervient pas dans
cette primitive ; s8/10/12 retrouvera son rôle au raccord du front WSPD.
Aucun test G4 annoncé ici.
