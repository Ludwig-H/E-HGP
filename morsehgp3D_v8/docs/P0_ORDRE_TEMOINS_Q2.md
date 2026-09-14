# q2 : un ordre de témoins compact propre à la requête

14 septembre 2026. Dixième tranche ouverte après 39b58f37, cadre
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. La qualification du
[certificat frère](P0_CERTIFICAT_FRERE_Q2.md) reste historique, non transférée.

## Ce qui change

L'index global est inchangé. Pour une requête racine a×B₀, l'ordre du
census devient : tous les sites hors B₀ sauf a, puis tous les sites de
B₀. Ce report cherche à rencontrer les témoins proches de a avant que
le parcours ne fragmente B faute de crédit. Retirer a du **comptage
d'intérieur** est exact : $H(a,b,a)=0$. La collecte des coquilles n'est
pas modifiée et garde a, b et tous les autres sites de frontière.

L'option `Q2WitnessOrder::ComplementFirst` s'ajoute au raccord intégré
SharedBlocks. `GlobalDfs` reste le défaut, avec les anciens compteurs
géométriques inchangés. Le certificat frère est une option orthogonale ;
les quatre combinaisons restent comparables sur le même front, les mêmes
candidates et les mêmes supports. Pairwise+ComplementFirst est refusé.

## Contexte et continuation

Un contexte privé de trois indices identifie B₀, son échappement DFS et
le rang spatial de a. Ce rang est déjà connu dans la boucle d'ancres :
pas de recherche ni de permutation inverse. Les descendants empruntent
le même contexte immuable pendant l'appel synchrone. Ils ne remplacent
jamais B₀ par leur groupe B courant.

La continuation garde phase, curseur et compte. Pendant la phase
complément, les ancêtres propres de B₀ sont divisés **avant toute borne
ou consommation**, puis B₀ est sauté vers son échappement. Après la fin
de l'index, le curseur revient à B₀ pour la seconde phase, qui termine à
son échappement, pas à la fin de l'index. Les ancêtres de la feuille a
sont également divisés structurellement et cette feuille est reconnue
comme contribution nulle sans évaluation géométrique.

Le compte garde donc un préfixe résolu du même ordre pour tous les
descendants. Les populations différées ne sont pas consommées pendant
leur saut. Les groupes crédités restent disjoints ; les groupes non
positifs contribuent zéro ; aucune population différée n'est perdue.
Une subdivision B transmet compte, phase et curseur ensemble.
L'ancre reconnue nulle contribue une unité au compteur de travail
`consumed_witness_sites`, jamais au compte d'intérieurs.

## Coût et compatibilité avec la distribution future

Aucun arbre, tableau de facteur, frontière persistante, copie de
population ou histogramme A²+B² n'est ajouté. Le contexte tient trois
`size_t` ; le reste de l'état est local à la tâche. Dans le produit
actuel, le contexte est emprunté à la pile racine et le parcours est
synchrone. Une future file CPU/GPU devra posséder ce contexte ou sa
valeur : copier son pointeur de pile ne ferait pas une continuation sûre.

`Q2OrderWork` distingue divisions structurelles, sauts du B différé,
sauts de l'ancre et changements de phase. Les anciennes visites Z
comptent toujours les évaluations géométriques seulement. Le total à
étudier comprend ces opérations structurelles, les propositions/tests
du frère, les tâches et la collecte, en plus du front et de l'index.
Chaque tâche peut sauter B₀ et a au plus une fois et changer de phase
au plus une fois. Les deux chemins d'ancêtres ont profondeur au plus
48 dans le profil u16, mais une subdivision B peut dupliquer leur
travail restant : ne pas annoncer O(profondeur) par racine initiale.

Ce changement d'ordre ne borne toujours ni le nombre de tâches, ni les
petits groupes de témoins mixtes extérieurs qui ne contiennent pas a.
Les témoins utiles principalement situés dans B₀ peuvent aussi rester
tardifs. La preuve de conservation n'est pas une preuve sous-quadratique.

## Fixtures, qualification et décision

La fixture 3D proposée est B₀={0,1,2,3}³, a=(1000,1000,1000),
W={997,998,999}×{998,999}², K10, front Pure, s12. Tous les douze W
sont strictement intérieurs pour toutes les paires a×B₀. Le report de
B₀ seul ne suffit pas : le petit bloc {a}∪W est indécis à cause de a,
donc la règle de diagonales peut encore fragmenter B. L'exclusion
structurelle de a doit exposer W sans division de cette requête.

L'oracle indépendant compare les supports, clés, intérieurs et
coquilles, y compris après crédit, réflexion et permutation. Les résultats
agrégés de l'entrée publique ne sont pas des compteurs par ancre ; ne
pas confondre le modèle de cette fixture avec une instrumentation absente.
Les [56 mesures propres et qualifications](../receipts/q2_witness_order_20260914/README.md)
couvrent n8k/16k/32k à s8, et les quatre variantes à s8/10/12 pour
8k. Les 45 CTests Release/Clang ASan/UBSan passent ; 1 798 appels du
nouveau juge comparent 48 922 supports complets. L'audit A publié à
a1ee8cb0 vérifie séparément la continuation et les reprises de son modèle.
L'option reste désactivée par défaut : aucun gain temporel uniforme8k,
et les amas restent quasi quadratiques malgré leur accélération locale.
Leurs visites géométriques font ×4,106 puis ×4,229. La structure est
payée séparément ; les bons résultats sur rangées ne ferment pas P0.
GCP non utilisé.

## Prochaine expérience : garder les deux groupes dans le census

Le changement d'ordre ne supprime pas la boucle qui lance une recherche
par ancre du petit facteur. La proposition suivante reprend les
[extrema exacts de l'auditeur A, §9–9.1](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches) :
une tâche conserve A, B, compte commun, curseur, phase et B original.
Un minimum strictement positif crédite tout Z pour tout A×B ; un maximum
non positif résout Z pour le seul compte. Une indécision raffine Z, A
ou B en gardant la même continuation. Quand A devient singleton, le
chemin actuel peut reprendre **sans redémarrer ni recompter** le préfixe.

Ne pas exclure tout A du compte : les autres sites de A peuvent être
intérieurs à une paire donnée. L'exclusion connue-zéro de l'ancre est
possible seulement après passage singleton. La collecte des supports
admis reste individuelle et payée. Les bornes de boîtes conjointes
sont plus coûteuses que les bornes à ancre fixe ; leur nombre seul
n'est donc pas un comparatif suffisant.

Fixture proposée, pas encore gate produit : A={(100,0,0),(100,4,0)},
B={(0,1,0),(0,2,0),(0,3,0)}, z=(50,2,0), front Pure/s12.
Le témoin z donne un minimum H=2498. K1 permet le rejet conjoint des
six paires. K2 doit préserver le crédit hérité : leurs profondeurs
respectives sont [1,2,3] et [3,2,1], donc deux paires sont admises.
Recommencer Z en gardant le crédit compterait z deux fois et les perdrait.

Compter racines de produits, tâches conjointes, subdivisions A/B/Z,
tests conjoints, reprises singleton après crédit et masses
rejetées/admises/transmises. Chaque masse doit partitionner le domaine
de candidates. Mesurer aussi le pire état vivant et les rejets arrivés
jusqu'à la paire. Cette proposition n'est ni implémentée ici ni une
garantie sous-quadratique : la fragmentation complète reste possible.
