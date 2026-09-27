# Facteurs → bandes, sans anciennes cellules

27 septembre 2026. `phase=exploration_v9_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u18_input_only`,
`mode=audit_direct_q34_factor_bands`, `public_status=not_claimed`.
GCP non utilisé. Aucun moteur modifié.

Le [premier prototype](../b_q34_bands_20260927/README.md) ajoutait les bandes
aux cellules déjà calculées. Cette nouvelle tranche supprime réellement
leur construction sur le chemin candidat : `direct.hpp` ne fabrique
**aucun `fp::Plan` et aucune cellule A×B de crédits**. L'ancien objet est
seulement construit par la sonde, sur l'autre bras de la comparaison.

## Préparation inchangée, regroupement changé

La méthode privée `factor_plan::Plan::prepare` de `ddf4776d7` est portée
explicitement dans le nouveau propriétaire. Le fichier historique reste
inchangé. Sélection des K propositions, départage par ID original, prédicats
stricts huit coins, saturation q3/q4 et permutation stable des classes sont
conservés. Le différentiel compare **tous** les vecteurs/propositions/classes
et vingt compteurs de préparation/géométrie, pas seulement le nombre de
paires survivantes.

Le facteur B est déjà trié par crédits `(b3,b4)`. Une passe sur ses classes
occupées repère ses lignes et construit deux préfixes **marginaux**. Pour
une classe A, le préfixe `b3<r3` donne la première bande ; dans chaque ligne
restante, une recherche binaire trouve la fin du préfixe `b4<r4`. Les masses
q3 et q4 viennent des préfixes marginaux, tandis que l'union est la somme
des tailles des bandes **disjointes**. Aucun préfixe 2D n'est nécessaire.
Le masque d'une paire reste calculé exactement avec ses deux crédits.

La propriété de disjonction et les seuils K−1/K−2 sont ceux du
[premier prototype](../b_q34_bands_20260927/README.md). Le regroupement
interne teste aussi K1 et les voies inactives après normalisation du masque ;
le propriétaire géométrique garde l'interface K2..10 du Pool, sans inventer
un générateur q3/q4 à K1.

Coût du regroupement : `O(C_B + Kmax + C_A R_B log(Kmax))`, avec au plus
K lignes occupées et `C_A <= K(K−1)`. Ce n'est **pas** O(K²) lorsque les
deux indices A/lignes varient. Le Pool garde sa préparation O(KF), la somme
F reste à mesurer et le résidu E est inchangé. Les vingt-deux cases
marginaux remplacent les 341 cases et cent étapes du prototype précédent ;
les recherches binaires et lignes visitées sont comptées explicitement.

## Propriété sûre et mémoire déclarée

`Plan` construit ses deux facteurs depuis l'index partagé, dans des vecteurs
privés neufs. Aucun vecteur fourni par l'appelant n'est adopté. Seules des
vues const sortent ; copie, déplacement et affectations sont interdits.
Les bandes stockent des indices locaux, pas des pointeurs sur une mémoire
adoptée. La préparation emprunte l'index immuable ; aucun alias mutable des
facteurs n'est conservé à l'extérieur. L'outil `detail::group` est un
adaptateur interne de facteurs déjà construits, pas une factory publique
de certificat pour des facteurs arbitraires.

Les offsets locaux u32 sont validés avant préparation. La résidence
collective GPU demanderait ses propres offsets u64 ; elle n'existe pas
ici. Le compteur `retained_buffers` compte les **buffers non vides retenus**,
pas les appels malloc ni les réallocations temporaires. Huit buffers de
facteurs restent présents par rectangle, plus éventuellement un buffer de
descripteurs. Un changement vers une arène collective reste nécessaire
avant une affirmation sur le coût d'allocations GPU.

Les capacités complètes des plans et leurs maxima séquentiels sont publiés
séparément de celles des seuls descripteurs. Les deux objets sont vivants
ensemble dans le comparateur ; les maxima par objet ne représentent donc
pas sa RSS. La copie de rangs/crédits de chaque facteur reste effectivement
payée ; elle n'est pas supprimée par les bandes.

## Mesure appariée et portée

Sur chaque rectangle préparé, le même binaire construit successivement
**ancien, nouveau, nouveau, ancien**. Il publie séparément les deux sommes
anciennes et les deux sommes nouvelles, afin de révéler un effet d'ordre.
Le chronomètre couvre les constructeurs et leurs allocations ; il exclut
la destruction, la vérification et le front/filtre partagé. La sonde paie
et publie toutes ces autres étapes dans son mur complet. Le contrôle de
tous les facteurs et cellules est réellement exécuté après chaque paire.

Ce sont deux appariements intra-processus par rectangle, **pas** deux
campagnes G4 ni deux réalisations indépendantes de trame entière. Le cache,
le compilateur et l'hôte local partagé peuvent influer sur les petits
écarts. Le port direct appelle la certification avec le mode normal constant,
tandis que la référence historique garde son branchement de mutant dans
la préparation : les décisions et le travail géométrique discret sont
identiques, mais le compilateur n'est pas obligé de produire les mêmes
instructions. Le gain apparié porte sur **les deux implémentations complètes
de préparation**, pas sur une attribution causale aux seules bandes. Un
contrôle utilisant exactement le même corps compilé de préparation serait
nécessaire pour isoler ce dernier effet. Une baisse du coût de préparation
ne supprime ni S2, ni les graines,
ni le census, ni FULL. Restaurer l'ordre natif et les sidecars reste à payer
avant tout raccord.

La [capture](../../receipts/q34_direct_bands_20260927/README.md) contient
les portes Release/Clang ASan/UBSan/LSan, trois mutations natives, puis
quinze cas appariés : neuf synthétiques 8k/16k/32k, trois trames sans sol
entières K5/s8 de la seule séquence 08 et trois variantes K10/s10/s12 de
08/000000. Les sources et entrées sont épinglées avant/après ; les lecteurs
normal/−O comparent aussi chaque masse et les compteurs géométriques aux
reçus Pool publiés.

Aucune qualification FULL/GPU, aucun contrat 100 ms et aucune nouvelle
borne globale sous-quadratique ne sont revendiqués par ce prototype.
