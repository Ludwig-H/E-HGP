# Passation v8 — plages d'ancres et plans Pool possédés

15 septembre 2026. Cadre actif : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Aucun contrat de tour n'est encore acquis.

## À reprendre maintenant

Le [partage des plages d'ancres](docs/P0_PLAGES_ANCRES_Q2.md) est implémenté
dans une entrée distincte. Moteurs et buffers privés réutilisables ;
aucune continuation par petite racine ; parents Pool immuables préparés
une seule fois puis possédés jusqu'au dernier consommateur. Le repli
Pool sans rejet garde le Shared global et partage ses ancres. La file
porte des valeurs de plage, pas une pile de49 cadres.

Qualification propre close :415 appels ranges/135 Coarse,
10 920 paires et778 110 sites de l'oracle passent, coquille30 et transfert
de bandes filtrées positivement exercés.72 CTests Release et Clang
ASan/UBSan, gate Clang TSan et174 mesures8k/16k/32k K5/10 s8/10/12
passent ; lecteurs/analyseurs normal/−O identiques. Builds
`build/v8_anchor_ranges_20260915`, `build/v8_anchor_ranges_sanitize_20260915`
et `build/v8_anchor_ranges_tsan_clang_20260915` désormais épinglés.
Ne pas les écraser. Lire les [reçus propres](receipts/q2_anchor_ranges_20260915/README.md).

Les temps Pool sont maintenant préparation plus intervalles actifs des
plages ; les sommes de maxima par créateur ne bornent plus la mémoire
simultanée des plans partagés. Lire les nouveaux champs de durée de vie.
Les six postes géométriques restent exactement ceux de Coarse ; aucun
gain stable sous la charge concurrente des qualifications/audits.
Les278 dons mesurés sont208 Shared et70 replis Pool ;52 après attribution
des seeds. Aucun don Pool filtré à ce grain64 dans les grands nuages,
mais115/101/103 dans les gates Release/ASan/TSan. La file porte72 octets
par plage, pas les6 272 octets d'une pile de continuation ; elle ne peut
pas répartir une ancre déjà commencée.

**Prochaine priorité : les millions de petites requêtes, pas une nouvelle
file.** Uniforme32k conserve10,181M plages,11,084M ancres et1,001Md visites
census, sans offre au grain64. Préparer des lots de singletons compacts
avec contexte original, rang d'ancre, stade et certificat frère dû,
puis payer la collecte complète. Borner la mémoire des lots sans limiter
la recherche. Réduire et mesurer le travail réel front/census avant GPU.
Sur les quatre séries Pool64, six postes sous×3 ; F rangées×4 et Pairwise
Pool amas jusqu'à×3,447. Quinze ratios de scheduling W4 dépassent×4.
O(A+R) pour la gestion des plages ne borne ni A(n), ni F, ni les sorties.
Aucun contrat FULL/G4 ni moteur q3/q4 nouveau. GCP non utilisé.

## Dix-septième tranche publiée à beee3341 — historique

Le [raccord à une équipe persistante](docs/P0_EQUIPE_PERSISTANTE_Q2.md)
est implémenté : une seule équipe pour seeds Coarse et branches q2 hors
Pool, même index possédé, compte/curseur/phase/B original conservés.
Descripteur et masse restent chargés une fois par rectangle, pas par
fragment. Pool et ses replis restent synchrones ; la nouvelle API est
explicite et ne remplace pas le défaut.

Les [174 mesures closes](receipts/q2_cooperative_20260915/README.md) gardent
exactement géométrie et supports.69 CTests Release et Clang ASan/UBSan,
ainsi que la gate Clang TSan, PASS. La reprise ASan/UBSan est close après
redémarrage, sans effacer l'essai incomplet. La gate compare235 appels
coopératifs,91 Coarse et5 744 paires
de l'oracle exhaustif. Aucun gain de vitesse stable ; les rangées sans
Pool régressent dans17 des18 observations malgré81 042 dons au total.

**Prochaine priorité : la granularité, pas une autre micro-variante de file.**
Partager des plages d'ancres non commencées permet de réutiliser le moteur
privé du receveur sans allouer une pile complète par ancre. Le parent paie
une seule fois sa préparation, notamment Pool ; les bandes gardent leur
permutation, pas les nœuds B. Garder aussi le repli Shared sans rejet.
Pour les petits rectangles, un singleton ne nécessite plus de pile B :
préparer un état compact avec contexte original, et payer toute la coquille.
Ces deux nouveaux formats ne sont pas implémentés par la tranche17.

Uniforme32k :11,084M racines,1,001Md visites census, facteur maximal10 ;
aucune continuation au seuil16. Rangées8k/Pool64 :78,8% des visites dans
un worker à cause du repli synchrone. Six postes sous×3 dans la campagne
Pool64 ne veut pas dire tous les coûts : F rangées×4, résidu Pool amas
jusqu'à×3,447, populations de témoins distinctes des opérations. Lire
les tableaux et la portée avant toute affirmation sous-quadratique.

Builds de cette tranche : `build/v8_cooperative_20260915`,
`build/v8_cooperative_sanitize_20260915`,
`build/v8_cooperative_tsan_clang_20260915`. Tous trois désormais épinglés,
ne pas les écraser. FULL, q3/q4 produit, GPU/G4 et massif restent ouverts.

## Seizième tranche publiée à897085f8 — historique

Le [détachement intérieur q2](docs/P0_DETACHEMENT_CENSUS_Q2.md) est implémenté
et qualifié :66 CTests Release/Clang ASan/UBSan, gate Clang TSan du répartiteur,
302 scénarios de détachement et2 304 appels parallèles. Les
[preuves propres](receipts/q2_census_split_20260915/README.md) comprennent144
mesures d'ancres et12 régressions q2 complet. Builds désormais épinglés :
`build/v8_census_split_20260915`, `build/v8_census_split_sanitize_20260915`,
`build/v8_census_split_tsan_clang_20260915`. Le préflight et les échecs de
compilation/lien/collecteur sont explicitement conservés ; ne pas les effacer.

Suite prioritaire : **une seule équipe persistante front+census**, pas une
équipe par ancre. Les144 mesures montrent pourquoi : seuls23 cas W4 ont
plusieurs workers actifs, et8 des72 comparaisons W1/W4 montrent un gain
ponctuel sous charge, sans qualification de vitesse. Les36 compteurs et
les quatre étapes restent identiques ; le q2 complet garde sa croissance
mesurée sous×3 sur les quatre familles. Les sauts de l'ancre sélectionnée
sont publiés et ne constituent pas une étude de croissance globale.

Quatre obligations à raccorder : produit du front, curseur de rectangle,
continuation Shared et curseur de bande Pool. Conserver le même index,
B original, compte/curseur/phase ; ne copier aucun historique ni créer
de root_start sur un fragment. Préparer Pool une fois par parent possédé.
Une bande Pool n'est pas un nœud B spatial : garder sa voie Pairwise,
et le repli Shared si aucune paire n'est retirée. Les petits jobs restent
locaux ; la fermeture/annulation doit couvrir toutes les obligations.

La file actuelle possède des continuations complètes à6 272 octets de
pile chacune ; Q+W borne les objets internes, pas le RSS ni les objets
créés par l'utilisateur. Le prochain format pourra ne garder que la
branche Entry et son contexte parental ; ce format compact n'existe pas
encore. Collecte atomique par support, pas de délai garanti par quantum.
Admission multiple impossible pour même arbre B/Z et règle actuelle :
preuve dans la note, ne plus chercher une fixture de ce cas ici.

Réponse Bbc9b2dc5 lue, six pins sources concordants ; ses504 960 appels
parallèles restent indépendants de nos preuves. Oracle entier B5124095b
disponible pour le prochain port q3/q4. FULL, GPU/G4 et massif restent ouverts.

## Quinze premières tranches — historique

La [continuation possédée d'une ancre q2](docs/P0_CENSUS_REPRENABLE_Q2.md)
est qualifiée :62 CTests Release/Clang ASan/UBSan, gate Clang TSan,
768 reprises contre référence et oracle,144 mesures d'ancres et12 de
q2 complet closes ; lecteurs/analyseurs normal/−O égaux. Les trois builds
`v8_census_resume_20260914`, `v8_census_resume_sanitize_20260914` et
`v8_census_resume_tsan_clang_20260914` sont épinglés, ne pas les écraser.
Le [bilan propre](receipts/q2_census_resume_20260914/README.md) distingue
strictement l'ancre sélectionnée du chemin q2 complet inchangé.

Prochaine brique : détacher un frère B encore non visité avec son propre
compte/curseur/phase et le même B original ; répartir les masses et les
compteurs déjà payés sans les répéter. La classe actuelle migre la pile
entière, pas ses frères simultanément. Elle réserve6 272 octets de cadres :
ne pas instancier ce stockage pour toutes les ancres à la fois. Aucun
Pool/joint/Pairwise asynchrone n'est raccordé. Conserver une préparation
Pool par rectangle parent, possédée et partagée, jamais par tranche A_i.
La collecte d'un support reste atomique, donc le quantum ne borne pas sa durée.

Lire la [proposition q3/q4](docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) avant
le futur port : six contre-fixtures d'indépendance des voies sont exécutées.
Arête×bloc de complétions, lot de formes q3×Z et segment d'événements q4
sont les objets proposés, pas des moteurs déjà qualifiés. L'identité
`GΔ=P2B1−P1B2` permet un comparateur q4 direct i128, avec signes des
dénominateurs ; elle ne réduit pas automatiquement corde/clés/niveaux.
La réponse B7b86e36b est lue, ses preuves restent indépendantes.
P0 global, FULL, GPU et contrats G4 ne sont pas clos.

## Quatorzième tranche publiée à4e878754 — historique

La [redistribution des produits pendants](docs/P0_REDISTRIBUTION_FRONT_Q2.md)
est implémentée et qualifiée :57 CTests Release/Clang ASan/UBSan,
deux gates Clang TSan,32 différentiels et192 mesures closes, lecteurs
normal/−O identiques. Le défaut `Coarse` reste
inchangé ; `Donate` ajoute une file bornée avec poursuite locale lorsqu'une
offre échoue, et un réveil sûr des workers à l'annulation. Comparer sur les
données LiDAR déjà préparées, pas seulement les familles déjà équilibrées.
Ne pas écraser les trois builds workers historiques ci-dessous. Les nouveaux
builds portent le préfixe `v8_dynamic_front` et sont également épinglés,
y compris le GCC-TSan en échec conservé et sa reprise Clang distincte.
Le [bilan propre](receipts/q2_dynamic_front_20260914/README.md) ne montre
pas de gain général : ne pas promouvoir les médianes sans leurs étendues.
Le dispatch garde la géométrie, mais n'abrège pas un callback déjà engagé.
Suite : continuations de census possédées et plans parentaux partagés ;
aucune reprise depuis une nouvelle racine, préparation par A_i ou pointeur
vers la pile d'un worker fini. Mesurer aussi l'attente, aujourd'hui incluse
dans le temps de présence. q3/q4, FULL et GPU restent à construire.

## Treizième tranche publiée à b268cf6f — historique

Le [front distribué et ses workers q2](docs/P0_FRONT_WORKERS_Q2.md)
sont implémentés : 53 CTests Release/Clang ASan/UBSan et la porte
ThreadSanitizer passent, 32 configurations ancien/nouveau mono concordent.
La [campagne propre](receipts/q2_front_workers_20260914/README.md) est close :
134 mesures, lecteurs normal/−O identiques. Les trois builds workers
Release/ASan/ThreadSanitizer sont désormais épinglés ; repartir dans un
répertoire neuf. À 8k/K10 sur quatre cœurs physiques, médianes de trois
essais : uniforme5,387→1,379 s, terrain0,971→0,286 s,
amas2,994→0,770 s, rangées0,230→0,119 s. Ne pas mélanger avec
le corpus86 sur deux cœurs physiques/quatre SMT. À s8, les principaux
comptes restent sous ×4 à chaque doublement8k/16k/32k, sans preuve générale.
Chaque job conserve les masques hérités et l'identité du même index.
Les plans Pool et contextes d'ordre restent synchrones dans un worker,
pas dans une file de tâches empruntées. Les callbacks d'un même slot
sont séquentiels ; entre slots, leur état mutable doit être séparé.
La fermeture des threads précède tout retour ou propagation d'exception.
L'annulation est coopérative entre jobs, pas au milieu d'un gros job.
La géométrie et le travail total restent ceux du mono : la parallélisation
ne résout pas à elle seule la complexité. Prochaine implémentation :
redistribuer les produits encore pendants sans refaire leurs parents,
continuer localement si la file est pleine ; un réveil d'annulation est
indispensable avant les joins si des workers peuvent dormir. Conserver
la voie grossière comme référence, mesurer dons/refus/somme du travail.
Les gros callbacks restent un sujet distinct. FULL/G4 restent ouverts.

## Douzième tranche publiée à ba11e3ab — historique

Le [port Pool terminal](docs/P0_POOL_TERMINAL_Q2.md) est implémenté.
Les crédits appartiennent aux deux nœuds du même index, regroupés en au
plus K bandes disjointes. Ni nouveau nuage ni préparation par job, ni
préchargement de crédit dans le census global. Le plan n'est ni copiable
ni déplaçable ; seule sa durée synchrone est garantie pour l'instant.
La permutation B locale sert aux requêtes individuelles, jamais à Z.
Un filtre sans réduction retourne au parcours existant : les rangées
ont motivé ce repli, qui conserve le coût de préparation dans les reçus.

[Qualification et campagnes](receipts/q2_terminal_pool_20260914/README.md)
closes dans les builds désormais épinglés v8_pool_terminal_20260914 et
v8_pool_terminal_sanitize_20260914 : 49 CTests Release/Clang ASan/UBSan
PASS, 32 commandes par qualification, lecteurs normal/−O identiques,
80 mesures propres. Aucun temps hérité de fbbecc01.
La décision porte sur toute la chaîne q2, F et census/sorties inclus,
à K5/K10, s8/10/12 à8k et croissance s8 à16k/32k. Sur amas/K10,
13,412/47,179/184,306 s deviennent 3,589/7,614/19,180 s ; les visites
font ×2,958/×2,701 avec Pool, contre ×4,106/×4,229 sans. Le gain ne
s'étend pas à uniforme/terrain, où aucun plan n'est sélectionné. Le
travail mesuré est sous le quadruplement, pas une borne générale.
Après ce port,
prioriser front et petits rectangles ; un filtrage partiellement efficace
peut encore perdre face au partagé. Les emplois massivement parallèles
devront posséder et partager le plan parent, sans recopier B par job.
q3/q4, FULL, multi-CPU, GPU et contrats G4 restent ouverts ; GCP non utilisé.

L'audit A d608cc28 a été lu : Global sur les seules racines singleton
ne gagne pas de façon stable, ne pas intégrer cette variante. La
prochaine parallélisation doit partager le front et ses petits rectangles,
avec moteur/collecteur par worker et réduction des compteurs, pas seulement
les rares paires survivantes des gros plans Pool. Les tâches en vol
doivent posséder contexte et plan, et les temps mur/temps cumulés ne
doivent pas être soustraits entre eux. Proposition détaillée dans le
contrat de cette tranche, non encore une implémentation multi-CPU.

## Onzième tranche publiée à b2106c3c — historique

Le [census conjoint A×B](docs/P0_CENSUS_CONJOINT_Q2.md) est implémenté :
bornes 96 octets, compte/curseur/phase conservés à la reprise singleton.
SharedProduct divise A/B ; SharedAnchors ne divise qu'A avant le relais.
Aucun retrait de tout A, redémarrage de Z ou nouveau tableau de témoins.
Le défaut reste Individual. Les trois modes et les supports complets
sont confrontés à l'oracle sur 5 336 appels.

L'[essai initial](receipts/q2_joint_20260914/README.md) est épinglé :
Release47 PASS, Clang46/47, perte de stdout reproduite dans le collecteur
Python lors d'une interruption. Le correctif protège aussi les lectures,
pas seulement Popen ; six nouvelles fixtures complètent les six tests
de lancement. Ni les octets perdus ni l'échec ne sont masqués.
Les [captures corrigées r2](receipts/q2_joint_r2_20260914/README.md)
utilisent v8_joint_r2_20260914 et v8_joint_sanitize_r2_20260914,
sans changement C++ par rapport au premier gel. Les builds sont désormais
épinglés : 47 CTests Release/Clang ASan/UBSan passent, 27 commandes par
qualification, lecteurs normal/−O identiques et 44 mesures closes.
Sur amas8k/16k/32k, A seul prend 12,121/45,934/182,155 s ; les visites
après relais font ×4,107/×4,231. Ce n'est pas la réduction de croissance
cherchée. Le bras équilibré est pire à 8k, notamment sur les rangées,
car il fragmente B avant les certificats frère. Les trois s8/10/12
sont comparés à 8k, la croissance à s8 seulement. Ne promouvoir ni
un gain global ni le partage par défaut ; P0 reste ouvert.

**Priorité suivante : [Pool terminal → census global](docs/P0_POOL_TERMINAL_RACCORD.md).**
L'audit A fbbecc01 confirme le gain q2 complet sur amas/LiDAR50k,
dans un prototype séparé. Porter d'abord Pool/paires, propriétaire et
index uniques, rangs distincts des IDs, au plus K bandes résiduelles,
compte census nul. Pas de crédit préchargé, de scan A×B rejeté, ni de
plan recalculé par job. Le partagé local ne gagne pas de façon stable
dans l'audit ; le front et les petits rectangles restent dominants
après Pool. Ni ces preuves de prototype ni les tranches antérieures
ne qualifient automatiquement le futur port produit. FULL/G4 restent ouverts.

## Dixième tranche publiée à e3af11a7 — historique

L'[ordre complément/B original](docs/P0_ORDRE_TEMOINS_Q2.md) est implémenté
et qualifié localement. `ComplementFirst` garde le B original
et le rang de a dans un contexte immuable, puis transmet phase/cursor/count
aux enfants. Les ascendants de B original et de a sont divisés avant
toute borne ; a est exclu du seul comptage, jamais de la coquille.
Builds `v8_witness_order_20260914` et
`v8_witness_order_sanitize_20260914` désormais épinglés : 45 CTests
Release/Clang ASan/UBSan passent, détection des fuites conservée.
[56 mesures propres](receipts/q2_witness_order_20260914/README.md) :
s8/10/12 à 8k et croissance s8 en Complement/sibling. Temps q2
8k/16k/32k : rangées 0,201/0,437/0,942 s ; amas
11,378/43,750/173,471 s. Les visites des amas font encore ×4,106 puis
×4,229. Uniforme8k ne gagne pas en temps ; les visites retirées sont
en grande partie remplacées par des opérations structurelles.

**Priorité suivante : un census conjoint A×B**, puis reprise du chemin
actuel dès A singleton, avec le même compte/curseur/phase, sans racine
recommencée. Ne pas exclure tout A du compte : d'autres ancres peuvent
être intérieures. Lire la dernière section du contrat pour invariant,
fixture K1/K2 et compteurs de masses/tâches/bornes conjointes. Les
preuves de bornes A §9 existent, pas encore ce raccord produit. Garder
le coût plus élevé d'une borne conjointe dans la comparaison. L'audit A
publié à a1ee8cb0 contre-vérifie l'ordre actuel et propose aussi une
comparaison isolée de l'ancien ordre sur les seules racines B singleton.
Ne pas transformer les enfants déjà crédités par ce raccourci.
La représentation de contexte doit devenir possédée avant une file
asynchrone CPU/GPU. Aucun contrat FULL/G4 acquis ; GCP non utilisé.

## Neuvième tranche publiée à 39b58f37 — historique

L'option [certificat autonome du frère](docs/P0_CERTIFICAT_FRERE_Q2.md)
est implémentée dans le seul raccord WSPD q2. Elle n'additionne aucun
crédit et n'avance jamais Z ; le défaut reste Disabled. Les builds
`build/v8_sibling_r2_20260914` et `build/v8_sibling_sanitize_r2_20260914`
sont maintenant épinglés : **44 CTests Release/Clang ASan/UBSan passent**,
lecteurs normal/−O et [32 mesures](receipts/q2_sibling_20260914/README.md) clos.
Les premiers essais de qualification sont conservés en échec : course
de collecte du runner à l'interruption et LeakSanitizer bloqué dans le
sandbox. Le runner corrigé passe six contre-tests déterministes de
lancement/interruption/session en plus des 41 contrôles historiques.

Le gain est réel sur les rangées : 0,241/0,481/0,969 s à n8k/16k/32k,
s8/K10, et croissance des visites+tests frère ×2,20 puis ×2,13. Le
temps de référence apparié 8k est 1,494 s. Les amas restent presque
quadratiques : évaluations ×4,29 puis ×4,22, 214,90 s à 32k. Pas de
gain universel ni de borne globale ; ne pas activer ce mode par défaut.
Les comparaisons s10/12 nouvelles sont limitées à 8k, la croissance à s8.

**Prochaine expérience prioritaire : ordre complément/B original et
exclusion de l'ancre du comptage seulement**, selon la dernière section
du contrat. État compact phase/curseur/compte, contexte original commun
aux enfants ; raffinement structurel des ancêtres du groupe différé et
de l'ancre avant toute consommation. Tester la fixture 3D B cube4³,
a=(1000,1000,1000), douze W proches de a, puis les quatre régimes et
les données LiDAR. Ne pas remettre une liste de frontières avant cette
variante minimale. Le partage A×B×Z, les reprises distribuables et les
workers CPU/GPU restent distincts ; les contrats FULL/G4 ne sont pas acquis.
L'auditeur A a publié la preuve K−c et les limites LiDAR à 5c32ab95 ;
pas de port de cette micro-variante sans gain net justifié. GCP non utilisé.

## Huitième tranche publiée à f7edd646 — historique

Le [raccord q2 global](docs/P0_FRONT_ET_CENSUS_Q2.md) traite directement
les nœuds WSPD du même index, sans factory, copie B ou arbre local.
Pairwise et Shared comparent les mêmes supports sur le même nuage.
Le plus petit facteur fournit les ancres ; le groupe opposé hérite du
compte et du curseur DFS à chaque subdivision. Le front demande q2 seul,
pas les trois voies de la capture historique. Les temps englobent front,
census, collecte/callback et destructions : ne pas les appeler census isolé.

Le [reçu de cette tranche](receipts/wspd_q2_census_20260914/README.md) conserve les
tests et mesures de cette tranche. Le gate indépendant confronte 1 255
appels et 46 762 supports complets, avec 60 subdivisions après crédit.
43 CTests Release et Clang ASan/UBSan passent sur ces sources historiques.
Les builds `v8_front_census_20260914` et
`v8_front_census_sanitize_20260914` sont désormais épinglés. Les 53
mesures et lecteurs normal/−O sont clos ; repartir dans un build neuf.

**Priorité : diminuer le travail par ancre et celui du proposeur.**
Le raccord supprime une reconstruction mais conserve Σmin(|A|,|B|)
démarrages et beaucoup de visites Z. Sur amas8k/16k/32k à s8, ces visites
font 0,973/4,199/17,665 milliards, soit ×4,31 puis ×4,21, pour des
supports proches du linéaire : le critère de croissance demandé échoue.
Étudier un état portant encore deux groupes (A,B,compte,curseur Z), mais
agir aussi sur le choix de Z : la contre-fixture collinéaire a=1000,
B=0..63 montre que le DFS croissant fragmente B avant tout crédit.
Avec A singleton, agrandir seulement l'état en A×B×Z ne résout rien.
Expérience minimale contre-vérifiée : au split de B, tester son frère
comme certificat autonome de K témoins stricts pour chaque enfant.
S'il échoue, ne rien accumuler et reprendre compte/cursor inchangés.
Ce test de coût constant par enfant reste à implémenter et qualifier.
Un compte ne se réutilise qu'avec son préfixe consommé et son index précis. La
propagation de témoins de B est une autre proposition, pas un gain de
temps acquis pour le produit. Le test de lentille a un rendement faible
dans son nouveau prototype (0–1 % de recherches évitables).

L'auditeur A a publié ses données LiDAR et prépare la mesure q2 sur ces
entrées ; son travail reste indépendant, avec ses propres sources/reçus.
Le terrain synthétique ne remplace pas ce corpus. Ne pas confondre retrait
de Xi, amélioration du front et partage du census. Reprises distribuables,
workers CPU/GPU, q3/q4 et FULL sont encore absents ; contrats G4 ouverts.

## Septième tranche publiée à da366f7f — historique

Le [premier front WSPD réel](docs/P0_FRONT_REEL.md) manipule les nœuds du
même index Z et leur permutation. Aucun plan Pool/Axis ni parcours de
facteur n'est préparé par produit. La convention de séparation est
`box_gap_diameter_v1`, la comparaison `Pure`/`MidpointSamples` conserve
les entrées, pas nécessairement les descripteurs. Les rejets sont propres
à chaque voie et hérités ; les comptes partiels ne sont pas hérités.
Les preuves et coûts sont dans le [nouveau reçu](receipts/wspd_front_20260914/README.md).

40 CTests Release/Clang ASan/UBSan passent. La mesure uniforme32k/s8
réduit le front de 56,8 à 20,9 millions de rectangles, mais passe de
4,87 s à 37,4 s : le proposeur est trop cher (954 millions de pas
d'index). Ne pas promouvoir `MidpointSamples` comme gain de temps
acquis, ni retirer Pure de la comparaison du futur chemin consommé.
Les 72 mesures sont closes : quatre familles, n8k/16k/32k, Kmax10,
s8/10/12, deux modes. Les tâches croissent sous ×4 dans ces mesures,
mais les résidus Samples des amas font presque ×4, comme q3/q4 sur les
rangées. P0 n'est pas clos. Les deux builds `v8_front_20260914` et
`v8_front_sanitize_20260914` sont épinglés ; reprendre dans des builds neufs.

**Priorité immédiate : consommer directement le front par le census q2.**
Ne pas reconstruire une factory de rectangle, Axis ou un arbre B par
descripteur. Garder les nœuds et leur ordre spatial, factoriser l'état du
census global et réutiliser ses tampons. Comparer à l'exhaustif les vrais
supports, intérieurs et coquilles, puis mesurer le coût complet aux trois
tailles. Le proposeur recommence encore depuis la racine : test préalable
de lentille et certificats partiels distincts sont à comparer sur ce chemin.

q3/q4 nécessitent aussi une réduction collective : deux rangées parallèles
laissent m² candidates transversales même avec tous les témoins ponctuels.
Ne pas développer ce résidu par défaut, ni conclure qu'il s'agit de m²
supports utiles. La génération canonique par plus longue arête et la
rétention par boule restent requises ; un rejet q4 ne tue pas q2.
Les tâches par handles permettent une distribution future, mais le moteur
courant est mono-thread. FULL, multi-CPU, GPU et contrats G4 restent ouverts.

L'auditeur A prépare le régime LiDAR simple et superposé. Le `terrain`
actuel est un slab aléatoire mince, pas un simulateur de capteur : ne pas
choisir une variante pour le LiDAR sur cette seule mesure. Ajouter un
corpus déclaré, poses/quantification et collisions explicites, en séparant
densification de la même zone et extension du trajet. Ses captures en
préparation restent indépendantes de la présente qualification.

## Sixième tranche publiée à 85015a8c — historique

La sixième tranche partage [le nuage et l'index Z](docs/P0_NUAGE_ET_INDEX_PARTAGES.md)
entre rectangles et seuils. 37 CTests passent en Release et Clang ASan/UBSan.
Les copies globales répétées et le scan des facteurs pour leurs boîtes
sont retirés du nouvel appel partagé. L'ancien adaptateur reste coûteux
par construction et ne doit pas alimenter un front WSPD. Les builds
`v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont réservés à ces preuves.

Les [66 essais clos](receipts/cloud_reuse_20260914/README.md) couvrent
8k/16k/32k, Kmax10, s8/10/12 à R32, puis R32/64/128 sur grille/déséquilibré.
À 32k/R32/s8, partage : grille 457–465 ms, déséquilibré 87–95 ms,
nappe environ 16 s. Les mêmes sorties et compteurs locaux sont vérifiés,
les préparations globales sont divisées exactement par R. En revanche,
les copies de restrictions grilles font ×3,94 puis ×3,97 quand R et n
doublent : ce verrou n'est pas résolu. La subdivision garde 35,6 millions
de candidates sur nappe32k. Tous les chronomètres, y compris un doublement
grille à ×5,62, sont conservés ; les temps restent bruités. Les builds sont
désormais épinglés. Aucun résultat ancien n'est réattribué à ce moteur.

**Priorité immédiate : pilote de front réel avec coût cumulé des facteurs.**
Ne pas prendre le partage du nuage pour une solution complète : Pool/Axis
gardent Ω(R|B|) sur une partition A_i×B et le découpage peut perdre des
témoins utiles aux minorants. R croissant avec n doit rester dans les
tests. Faire porter permutation et boîtes par les nœuds spatiaux certifiés ;
l'arbre de plages originales n'est qu'un raccord de compatibilité.
Tester les rejets sur produits ancêtres avant séparation complète et un
chemin sans tris/allocations lourds pour petits facteurs. Le nouvel audit
du front pur v4 montre leur nombre élevé, sans qualifier une WSPD v8.
Fixer explicitement la convention de s avant la comparaison s8/10/12.

Le seuil du census vient du rectangle du plan, l'index n'en possède plus.
Les restrictions restent liées au même rectangle exact. Les ordres B/Z
ne sont pas interchangeables ; l'auditeur précise en §9.4 pourquoi une
couverture compacte dans un ordre peut devenir linéaire dans un autre.
Raccorder Pool sans axe imposé, puis suspension mono, workers CPU et GPU.
Les continuations doivent suspendre sur saturation, jamais tronquer.
q3/q4 et FULL restent ouverts ; génération par plus longue arête et
rétention par boule (union des voies qui la conservent) sont inscrites au
plan. GCP non utilisé ; aucun contrat de tour n'est acquis.

## Cinquième tranche publiée à 3c29ea1e — historique

La cinquième tranche est qualifiée : [bornes préparées](docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md)
de 48 octets, 34 CTests Release/Clang ASan/UBSan, oracle de 1 424 cas et
comparaison des révisions sur [124 mesures](receipts/q2_prepared_bounds_20260914/README.md).
Compteurs et sorties inchangés ; baisse exploratoire du total Shared
de 4–10 % sur les médianes répétées grille/nappe32k/K10/s8. Les temps
doublent par 1,58–2,41 sur ces familles, sans borne générale ni gain
garanti sur tout nuage. Les deux builds `v8_prepared_bounds_20260913`
et `v8_prepared_bounds_sanitize_20260913` sont épinglés.

**Prochain chantier : séparer nuage/index global et contexte de rectangle.**
Ne pas recopier et revalider n sites pour chaque rectangle WSPD. Préserver
les trois compatibilités : même nuage pour index/requête, même contexte
rectangle/seuil pour crédits, même permutation B et index Z précis pour
plages/continuations. Porter les contre-fixtures de l'auditeur avant de
relâcher les contrôles de propriétaires actuels. Raccorder Pool seul par
les préfixes de classes (§9.2 de son audit), sans forcer le filtre axial.
Puis expliciter les transitions du curseur pour suspension mono testée,
workers CPU et enfin GPU ; des files pleines suspendent, jamais ne tronquent.
Partager l'index ne borne pas la somme des tailles de facteurs ni les sorties.

La déduplication des boules doit précéder les collectes répétées tout en
conservant les incidences des supports. q3/q4 et la tranche FULL minimale
restent prioritaires : ne pas repartir dans une série de micro-variantes
de la seule borne q2. Les contrats 50k portent sur la tour entière sur
G4 ; mono local, G4 et massif sont des qualifications distinctes.
GCP non utilisé.

## Quatrième tranche publiée à f4815cd4 — historique

La quatrième tranche implémente le [census q2](docs/P0_CENSUS_Q2_PARTAGE.md)
sur les résidus compacts. Le parcours partagé emploie les échappements
DFS proposés par l'auditeur : compte uniforme et curseur, sans arena de
continuations. Il est comparé au parcours individuel sur le même index de
tous les sites. La seconde collecte émet réellement intérieurs, coquille
et support/clé entière ; son temps et son callback sont inclus. Le gate
géométrique passe 277 cas ; les 31 CTests Release/ASan/UBSan et les lecteurs
normal/−O passent. Les [204 mesures](receipts/q2_census_20260913/README.md)
ferment quatre campagnes, avec 164 configurations distinctes. Les builds
`v8_census_20260913` et `v8_census_sanitize_20260913` sont épinglés.

Résultat : l'intersection paie son coût sur les grilles (environ 131 ms
à n32k/K10 avec census individuel, contre 3,74–3,79 s après Additive seul).
Sur nappes32k, le census partagé réduit les visites mais reste plus lent.
À 50k/K10, individuel après intersection : 176–186 ms sur grilles,
4,05–4,09 s sur nappes ; K5 sur nappes reste à environ 1,55 s. Ce ne sont
pas des tours FULL. Les doublements 8k/16k/32k sont inférieurs à ×4
dans ces familles ; aucune borne globale n'en découle.

Suite mono bornée : préparer les constantes de bornes `(a,B)` une fois
par tâche, vérifier les mêmes décisions/visites/sorties puis mesurer le
temps complet. Éviter de choisir automatiquement Shared sur les petits
résidus. Le coût Pool seul doit encore être comparé via une interface de
résidu non liée à l'axe. Ne pas prolonger ces variantes au détriment du
propriétaire global WSPD et de la tranche q3/q4/FULL minimale.

La déduplication globale des boules, q3/q4, le partage du propriétaire
sur une vraie WSPD et les parents FULL restent distincts. Ne pas reprendre
un compte saturé à un seuil supérieur comme s'il était exact. L'ancien
parcours à listes n'est pas conservé comme moteur parallèle. GCP non utilisé.

## Troisième tranche publiée à f5430f57

La troisième tranche ajoute le mode `Additive`, son intersection intégrée
avec un plan local q2 et la suppression des allocations actives aussitôt
remplacées. Lire le [contrat](docs/P0_ADDITION_ET_INTERSECTION.md) et les
[648 mesures](receipts/additive_q2_20260913/README.md). 26 CTests Release
et Clang ASan/UBSan passent, incluant 650 plans confrontés au nouveau juge,
frontières, cœur, copie de restriction, 7 contre-modèles et 15 rejets d'API.

Résultat précis : la nappe complète n32k/K10 garde 3 928 390 candidates
contre 6 483 670 auparavant, mais la sélection additive seule coûte
environ 238 ms contre 57 ms. Sur la grille, l'intersection Pool garde
114 716 candidates en 34–35 ms ; Pool seul garde 378 840 candidates en
2,4–2,6 ms. Ne pas confondre moins de candidates et moins de temps total.
La croissance mesurée à 8k/16k/32k est inférieure au quadratique dans
ces familles, pas une qualification générale. Aucun choix automatique
de filtre ni changement du mode `Independent` par défaut n'est installé.

**Prochain jalon : payer le census q2 et partager ses requêtes par blocs.**
Il doit indexer tous les sites, distinguer intérieurs stricts et coquille,
conserver les IDs et canoniser les boules. Comparer une référence de
requêtes individuelles avec le traitement partagé, coût total inclus.
L'auditeur a livré les [bornes sur trois boîtes](audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
et un [prototype de consommation indexée](../audits/morsehgp3D_v8_complementaire/P0_CONSOMMATION_INDEXEE_Q2.md).
Ce dernier épingle `8e406f9b`, pas les nouvelles sources : ses grandes
mesures sont exploratoires. Ne pas précharger le cœur puis revoir ses IDs.

Les rangs filtrés B proposés pour composer deux plans déjà construits
restent une alternative au parcours intégré. Les fenêtres A/B peuvent
encore réduire le résidu des nappes ; ne pas retarder le census et la
tranche FULL minimale derrière une optimisation sans fin du seul cas axial.
Le vrai partage du propriétaire/index sur la WSPD, q3/q4, parents FULL,
parallélisation et contrats 50k/massif restent ouverts. GCP non utilisé.
Les builds `v8_additive_20260913` et `v8_additive_sanitize_20260913` sont
épinglés ; repartir dans un nouveau répertoire de construction.

## Deuxième tranche publiée à 8e406f9b

La deuxième tranche implémente `CreditBatch` (trois voies géométriques,
pas une tour K) et `AxisQ2Plan` (colonnes exactes + plages de l'index B).
Vingt et un CTests passent en Release GCC et Clang ASan/UBSan ;
[594 mesures appariées](receipts/shared_axis_20260913/README.md) couvrent
8k/16k/32k, Kmax5/10, s8/10/12 et deux ordres sur le même propriétaire.
Lire le [contrat expliqué](docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).

Acquis bornés : préparation Tubes divisée par trois à plans identiques ;
résidu q2 des nappes complètes passant de 256 millions à 6,48 millions
de paires à n32k. Le filtre axial coûte environ 57 ms, hors propriétaire,
sans expansion/census. Il peut faire moins bien que Pool sur les cubes,
et une rotation peut lui faire conserver toutes les paires. Aucun
algorithme général ni contrat de tour n'est acquis.

Suite prioritaire : additionner les colonnes exactes disjointes dans les
requêtes d'index, supprimer les tableaux initialisés puis remplacés,
comparer queues/fenêtres A/B et intersections de résidus. Puis implémenter
le consommateur q2 exact avec un index sur tous les sites, pas un scan
de n sites pour chaque paire ; ne pas recompter le cœur implicitement.
Le partage de validation sur toute la WSPD reste à faire. Les groupes
collectifs q3/q4 restent une piste distincte à qualifier avec l'auditeur.

Les pannes d'affectation de plans/batches sont désormais injectées et
couvertes par la garantie forte. Les reçus appariés vérifient sources,
matrices, identités, convention de checksum et provenances déclarées.
Les captures préliminaires exclues sont conservées avec leur motif.
Les nouveaux builds `v8_shared_axis_20260913` et sa variante sanitizer
sont épinglés ; ne pas les écraser. Aucun fichier v6/v7 ni statut formel
n'est modifié par cette tranche. GCP non utilisé.

## Première tranche publiée : historique à 3589a2c9

La bibliothèque `mhgp8_p0`, ses deux gates C++ et sa sonde sont implémentées.
Pool/DualBlocks/Tubes produisent des crédits sûrs et un résidu compact
sur un seul rectangle séparé. Huit CTests passent en Release GCC et en
Debug Clang ASan/UBSan ; 729 mesures mono sont conservées avec sources,
commandes, entrée et compteurs. Voir le
[contrat](docs/P0_CREDITS_LOCAUX.md) et les
[résultats](receipts/p0_local_credits_20260913/README.md).

Trois défauts d'audit sont corrigés avant livraison : le propriétaire
n'est plus copiable/réaffectable derrière un plan, ses coordonnées sont
copiées dans un stockage privé sans alias mutable hérité, et les reçus sont
confrontés strictement aux commandes avec sorties brutes et hashes de
fermeture. La première capture, antérieure aux correctifs, reste dans
`first_pass_pre_owner_fix/`, la deuxième dans `second_pass_pre_alias_fix/` ;
seules les nouvelles captures r3, copie des coordonnées incluse, font autorité.

La préparation quadratique systématique a une alternative effective :
les tubes coûtent O(m log m) par facteur. Mais **ni cette borne locale,
ni les petites latences ne ferment P0**. Les nappes gardent toutes les
paires ; sur les grilles q3/q4, les tubes sont rapides mais créditent
moins que DualBlocks. Le pool global peut rater les régions utiles :
contre-fixture des rails à 2 718 sites, q4, 1 846 881 paires contre 2 916
pour DualBlocks et Tubes. Ne pas généraliser un gagnant unique.

Suite prioritaire : partager le tri des tubes entre voies, partager la
validation du nuage, puis raffiner les produits résiduels difficiles en
préservant couverture et IDs. Il faut tester notamment les nappes, où
l'universalité sur toute la boîte opposée est trop restrictive. Une
combinaison de crédits utilise leur maximum, jamais leur somme sans
disjonction. Le cœur n'a pas encore ses IDs exportés : ne pas le recompter
implicitement dans le census. Mesurer ensuite le consommateur exact q2
minimal avant d'étendre census/FULL et la parallélisation.

Le vrai constructeur WSPD et sa comparaison s8/10/12, les voies complètes
q3/q4, census, parents et tour FULL restent à implémenter. Les benchmarks
actuels n'effectuent pas ce travail. Les builds v8 datés du 13 septembre
et les reçus sont épinglés. Aucun statut formel ni fichier v6/v7 modifié
par cette tranche ; aucun GPU/GCP utilisé.

## Historique de l'audit d'ouverture

La demande remplace l'optimisation incrémentale v7 par un audit complet
avant reconstruction. La base publiée examinée est main `dc57ffd5`.
Les modifications v6/v7 déjà présentes sont conservées, sans les inclure
dans cette ouverture v8. Le delta privé fused_history du 11 septembre
reste distinct des résultats publiés ; sa publication n'est pas poursuivie
dans ce changement de cap. Aucun processus de benchmark restant constaté
à l'ouverture du 13 septembre ; GCP non utilisé.

L'audit d'ouverture est rédigé : [synthèse](docs/AUDIT_V7_SYNTHESE.md),
[exposé pédagogique](docs/ALGORITHME_EXPLIQUE.md), quatre rapports détaillés
et [plan de refonte](docs/PLAN_DE_REFONTE.md). Il couvre WSPD et témoins,
supports q2/q3/q4, census, rattachements, histoires, verticales,
parallélisation, mémoire, tests et livraison. Sa portée n'est pas une
certification ligne par ligne de tout le corpus ni une réexécution C++.

Décisions principales : conserver le contrat FULL avec vrais parents,
partager les objets géométriques, préparer les rattachements indépendamment
de l'histoire, découper l'intérieur des gros rectangles, reconstruire
les histoires par contractions parallèles, puis réemployer les marques et
index. Le détail distingue explicitement régularité, coquilles, sorties
quadratiques et propositions encore sans qualification.

Les contrats restent ouverts : 50k 1..10/1..5 en environ 419/34 s sur les
dernières complétions publiées, pas de nouvelle capture 50k des prototypes
récents, pas de chaîne intégralement GPU, pas de dizaines de millions.

L'inventaire épingle la base publiée et les deltas locaux. Six lecteurs
de reçus clos et deux recalculs rationnels ont passé ; les contrôles de
livraison sont dans [PUBLICATION_CHECKS](receipts/audit_v7_20260913/PUBLICATION_CHECKS.json).
Le contrôleur documentaire couvre désormais la v8 et possède un test
positif/négatif de découverte, sans inclure les futurs audits indépendants.
Aucun statut formel modifié, aucun moteur compilé, GCP non utilisé.

Suite réordonnée sur demande explicite du 13 septembre : **P0 d'abord,
supprimer la préparation systématique O(|A|²+|B|²)** après l'échec des
témoins universels. Comparer minorants issus de petits ensembles,
parcours conjoints de blocs, requêtes géométriques groupées et sélection
directe de sous-produits ; aucune solution n'est imposée. Le
[plan](docs/PLAN_DE_REFONTE.md) fixe les preuves et mesures de travail
total, coût aval compris, avant les campagnes CPU/GPU. P0 reste ouverte.

Avant tout port, lire aussi [VERROUS_ARCHITECTURE](docs/VERROUS_ARCHITECTURE.md).
La note conserve les cinq obstacles suivants : recherches spatiales
répétées, triangles de départ × voisinages q3/q4, MEB/descentes répétées,
histoire/export centraux, puis résidence et échanges CPU/GPU. Elle fixe
les sources, nuances et critères de validation pour le futur développeur.
P0 garde son premier rang ; B1–B5 restent ouverts et ne sont pas des
impossibilités intrinsèques de HGP. Aucun nouveau résultat n'est revendiqué.

La tranche FULL minimale et les petits juges servent cette comparaison ;
ne pas la repousser derrière un port général de la v7. Garder les tailles
8k/16k/32k et s8/10/12. Aucun moteur n'est modifié par cette décision.
Ne pas reprendre le chantier fused v7 comme si le changement de cap
n'avait pas eu lieu. Le
[journal v8](../audits/COORDINATION_MORSEHGP3D_V8.md) porte les questions
à l'auditeur indépendant et la coordination d'index.
