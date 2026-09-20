# État de l'audit v8

20 septembre 2026. Audit constructeur, avec contrelectures parallèles.
Ce dossier n'est pas l'auditeur indépendant propriétaire des audits v7.

État constructeur courant : [reprise après3e94c868](../docs/REPRISE_DEVELOPPEMENT_20260920.md).
La relecture des deltas19–21 ne relève pas de défaut d'exactitude avéré ;
les sources, artefacts et synthèses de la tranche21 sont recontrôlés.
La [famille q4](../docs/Q4_FAMILLE_ET_BALAYAGE_20260920.md) est qualifiée :
82 CTests Release, nouvelle gate et sondes sous Clang ASan/UBSan,
206 appels indépendamment jugés, quinze mesures et lectures normal/−O
closes. Priorité suivante : génération q3/q4, catalogue et reconstruction FULL.
Aucun nouveau gain q2 revendiqué. GCP non utilisé ; public_status=not_claimed.

Suite après9ae4e28b : [candidats q3/q4](../docs/Q3_Q4_CANDIDATS_PAR_SEED_20260920.md)
qualifiés :84 CTests Release, nouvelles gates Clang ASan/UBSan,
1807 cas de géométrie,635 appels de raccord, quatre mutants compilés tués,
trente mesures closes. Clé commune, vraie positivité, propriété et seed canonique
se raccordent au balayage ; la complétude visée reste locale à une seed
fournie, pas au générateur global. Pas de nouvelle micro-variante q2.

Tranche24 après785d0589 : [cover fermé partagé et accès par arête](../docs/Q3_Q4_COVERS_PARTAGES_20260920.md).
La relecture ne relève pas de défaut géométrique ; elle a corrigé la borne
de coût initiale qui omettait les tris des coquilles en IDs originaux.
Le juge compare les sorties à un census rationnel sur tous les sites et
vérifie expressément un compte intermédiaire non global. La limite
structurelle S·m est conservée et mesurée, non transformée en claim
sous-quadratique. Aucun changement du moteur q2 ou de ses défauts.
Qualification close :85 CTests Release et trois portes/six sondes sous
Clang ASan/UBSan ;186 arêtes/184 seeds contre oracle/global, trois mutants
compilés et32 mesures. Sources151 et deux builds q34_cover épinglés,
lectures normal/−O closes. La fixture adverse garde14/54 sorties K5/10
mais n(n−2) lectures ; le partage du stockage ne clôt donc pas P0.

Tranche25 après77f659e4 : [témoins universels de famille](../docs/Q3_Q4_REJET_FAMILIAL_20260920.md).
Préfixe exact i128, pool spatial commun par arête, deux voies certifiées
séparément. Le test q4 universel implique l'intérieur q3 d'un site, pas
le rejet q3 de la face : seuils différents. Les oracles exercent les
deux masques restants et le compte zéro du repli.86 CTests Release,
quatre portes/18 sondes Clang ASan/UBSan et96 mesures clos ; lectures
normal/−O identiques. Les158 sources et deux builds pruning sont épinglés.
Le petit adversaire est réduit, pas supprimé ; les grands fonds ne
gagnent aucun rejet. Pas de borne sous-quadratique globale, pas de FULL.
Le [retour indépendant A à4215dd16](q34_collectif_20260920/README.md)
confirme le certificat initial et propose minimum collectif et corde
resserrée ; ces renforts ne sont pas portés dans cette tranche.

Tranche26 après8d0a0f0f : [corde resserrée et minimum collectif](../docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md)
portés et jugés indépendamment du modèle A.87 CTests Release, cinq
portes/24 sondes Clang ASan/UBSan,8549 contrôles, trois mutants compilés,
20 différentiels contre25 et208 mesures passent. Le minimum rationnel
exerce tangences, groupes mixtes, creux ponctuels et baisse du compte ;
les fractions de la borne évitent les produits i128 trop larges.
Les [reçus](../receipts/q34_collective_20260920/README.md) séparent
96 mesures de grands fonds à deux faces et48 filtres denses auxiliaires.
Ces derniers laissent à32k/C64 au moins185,344M/358,240M lectures futures
K5/10, non exécutées. L'amélioration ne ferme pas le régime dense ni
P0 global. Prochaine priorité : carte commune entre familles, pas
micro-variantes de pool ni généralisation prématurée au front WSPD.
Pas de nouveau parallèle/GPU, de FULL ou de contrat G4.

## Historique : vingt-et-unième tranche publiée à3e94c868

Vingt-et-unième tranche P0 close,
[témoins hérités du front q2](../docs/P0_TEMOINS_HERITES_Q2.md). Option
explicite `WspdFrontProposals::inherit_witnesses`, voie q2 seule : un produit
non rejeté transmet à ses enfants les rangs de ses témoins certifiés, comptés
une fois ; le moteur sans héritage égale la tranche 20 compteur pour compteur
(capture différentielle contre son build épinglé, trois fenêtres). 81 CTests
Release et Clang ASan/UBSan, portes héritage, dispatch et jobs sous Clang
TSan, rejeu indépendant du front q2, modèle Python indépendant, oracle de
sûreté par force brute, dix mutants causaux tués dont quatre non sûrs, 854
mesures, lectures normal/−O concordantes. Exécution avec héritage rapportée à
sa jumelle à n8k/16k/32k, fenêtre 2K petits facteurs : ×0,86 à ×0,93
(uniforme), ×0,88 à ×0,96 (amas), ×0,89 à ×0,96 (terrain), ×0,99 à ×1,03
(rangées, aucun gain) ; supports identiques, candidates à 76 à 88 % de la
jumelle hors rangées. Reprise exacte de la descente mesurée à ×0,94 à ×0,98
et non portée. [Reçus](../receipts/q2_front_inheritance_20260917/README.md).
Deux relectures indépendantes avant gel, conception puis implémentation.
Pas de borne sous-quadratique, P0 global non clos, FULL/G4 ouverts. GCP non
utilisé.

## Historique : vingtième tranche publiée à 8190e7ab

Vingtième tranche P0 close,
[fenêtre de propositions élargie du front q2](../docs/P0_SURPROPOSITION_TEMOINS_Q2.md).
Option explicite `WspdFrontProposals` (facteur 1, 2 ou 4, limite petits
facteurs), voie q2 seule, défaut identique au moteur précédent (capture
différentielle contre le build épinglé de la tranche 19). 78 CTests Release
et Clang ASan/UBSan, portes proposals et dispatch sous Clang TSan, rejeu
indépendant du front q2 avec dix mutants causaux tués, oracle force brute
sur les cinq entrées, 684 mesures, lectures normal/−O concordantes. Fenêtre
2K petits facteurs à n8k/16k/32k : temps q2 complet ×0,42 à ×0,54
(uniforme), ×0,51 à ×0,62 (amas), ×0,64 à ×0,71 (terrain), ×1,01 à ×1,18
(rangées, régression conservée) ; supports identiques, candidats à 26 à 40 %
hors rangées. [Reçus](../receipts/q2_front_proposals_20260917/README.md).
Pas de borne sous-quadratique, P0 global non clos, FULL/G4 ouverts.
Changement de constructeur le 17 septembre : l'ancien auditeur B reprend le
chantier, son canal est clos, ses reçus d'audit restent des mesures
indépendantes de l'ancien code. GCP non utilisé.

## Historique : dix-neuvième tranche publiée à 8d615cfd

Dix-neuvième tranche P0 close,
[lots de petits census q2](../docs/P0_LOTS_SINGLETON_Q2.md), **résultat
négatif qualifié**. Entrée Coarse distincte, états privés de 72 octets,
flush avant Pool et en fin de seed. 75 CTests Release et Clang ASan/UBSan,
porte Clang TSan, 595 appels à lots contre oracle, 172 mesures et lectures
normal/−O concordantes ; comptes géométriques égaux à Coarse. Le format
est plus lent que Coarse dans les 54 comparaisons n8k/16k/32k (×1,005 à
×1,225, médiane ×1,112 au quantum 64) et l'entrelacement n'apporte rien.
Coarse reste le défaut ; la piste est inscrite aux fausses pistes.
[Reçus de clôture](../receipts/q2_singleton_batch_20260915/README.md).
Changement de constructeur le 17 septembre : l'ancien auditeur B reprend le
chantier, son canal est clos, ses reçus d'audit restent des mesures
indépendantes de l'ancien code, pas une qualification du nouveau. La suite
annoncée, la surproposition de témoins au front, est la vingtième tranche.
GCP non utilisé.

## Historique : dix-huitième tranche publiée à2741d614

Dix-huitième tranche P0,
[plages d'ancres et plans Pool possédés](../docs/P0_PLAGES_ANCRES_Q2.md).
Qualification propre close :415 appels ranges/135 Coarse, oracle
indépendant et tous les comptes historiques ; coquille30 et dons de
bandes Pool filtrées exercés. Gate Clang TSan,72 CTests Release et Clang
ASan/UBSan,174 mesures8k/16k/32k K5/10 s8/10/12 PASS. Les
[lecteurs/analyseurs normal/−O](../receipts/q2_anchor_ranges_20260915/README.md)
concordent. Les trois builds anchor_ranges sont désormais épinglés.
Parents préparés une fois, moteurs privés réutilisés, ancien défaut
Coarse conservé. Aucun contrat FULL/G4 ni gain de temps acquis ici.
Les278 dons mesurés sont Shared/repli, sans bande filtrée à grain64 ;
ces dernières sont exercées dans les gates. Six postes sous×3 sur les
séries Pool64, mais F rangées×4 et quinze ratios de scheduling>×4 : pas
de borne générale. Les petits census en lots compacts étaient la suite
annoncée ; la tranche 19 les a mesurés et fermés.
La campagne B sur instantané antérieur reste indépendante ; la correction
de tangence q3 est acquittée et couverte par deux fixtures constructeur.

## Historique : dix-septième tranche publiée à beee3341

Dix-septième tranche P0,
[équipe persistante front+census q2](../docs/P0_EQUIPE_PERSISTANTE_Q2.md).
69 CTests Release et Clang ASan/UBSan, ainsi que la gate Clang TSan, PASS ;
reprise après redémarrage close, première capture conservée comme incomplète.
Les trois builds sont épinglés.235 appels
coopératifs,91 Coarse, oracle scalaire indépendant ;174 mesures closes,
tous comptes géométriques et supports égaux. Pas de gain stable : le
chemin Coarse existant reste le défaut. Les dons sont réels sur rangées
sans Pool, mais leur surcoût n'est pas amorti dans17/18 observations.
Les gros Pool/replis synchrones et millions de petites racines restent
ouverts : curseurs de plages d'ancres et plans parentaux possédés, puis
lots compacts singleton, sont proposés dans la note, pas implémentés.
Six postes sous×3 sur la campagne Pool64 ne prouvent pas une borne générale ;
F des rangées fait×4 et l'aval Pool des amas jusqu'à×3,447.
Les [preuves propres](../receipts/q2_cooperative_20260915/README.md)
ne sont pas celles de l'auditeur B. Aucun FULL/G4/GPU acquis ; GCP non utilisé.

## Historique : seizième tranche publiée à897085f8

Seizième tranche P0,
[détachement des frères q2 et répartiteur d'ancre](../docs/P0_DETACHEMENT_CENSUS_Q2.md).
66 CTests Release/Clang ASan/UBSan et la gate Clang TSan du répartiteur PASS.
Les [preuves propres](../receipts/q2_census_split_20260915/README.md) couvrent
302 scénarios de détachement,2 304 appels parallèles,144 mesures d'ancres
et12 régressions q2 complet. Les trois builds de qualification sont épinglés.
Travail géométrique et payloads identiques au mono, pas de gain de vitesse
général. Le raccord à l'équipe persistante du front et aux plans Pool
parentaux reste ouvert ; le pipeline q2 complet n'est pas modifié.
Sa croissance mesurée reste sous×3 sur les quatre familles, sans borne générale.
La preuve de l'impossibilité d'admission multiple est maintenant précisée.
L'oracle B5124095b q3/q4 et la campagne Bbc9b2dc5 restent indépendants.
Aucun contrat FULL/G4 ni GPU acquis. GCP non utilisé.

## Historique : quinzième tranche publiée à d09e2207

Quinzième tranche P0,
[continuations possédées d'une ancre q2](../docs/P0_CENSUS_REPRENABLE_Q2.md).
62 CTests Release/Clang ASan/UBSan et la gate Clang TSan passent.
Les [preuves propres](../receipts/q2_census_resume_20260914/README.md)
sont closes :768 reprises,144 mesures d'ancres sélectionnées,12 mesures
de q2 complet inchangé, lecteurs/analyseurs normal/−O conformes.
Les trois builds de reprise sont épinglés. Le budget de transitions ne
tronque rien ; collecte par support encore atomique, frères B encore
séquentiels. Aucun raccord Pool/dispatcher, aucun gain de tour revendiqué.
La croissance géométrique mesurée reste celle de la tranche14, pas une
borne générale. [q3/q4](../docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) :
architecture proposée, six contre-fixtures rationnelles exécutées,
comparateur de racines réduit en i128 prouvé mais non implémenté.
Les contrelectures B7b86e36b restent indépendantes. Aucun GCP utilisé.

## Historique : quatorzième tranche publiée à4e878754

Quatorzième tranche P0,
[redistribution des produits pendants](../docs/P0_REDISTRIBUTION_FRONT_Q2.md)
implémentée et qualifiée :57 CTests Release/Clang ASan/UBSan, deux gates
Clang TSan et32 différentiels PASS. Les [192 mesures propres](../receipts/q2_dynamic_front_20260914/README.md)
sont closes, lecteurs/analyseurs normal/−O identiques. `Coarse` conservé
par défaut : gains modestes ou instables, régression mesurée sur amas8k.
Compteurs de distribution distincts : les six principaux comptes géométriques
restent sous×3 aux doublements8k/16k/32k sur cinq séries, mais pas tous les
compteurs variables de dons/attentes. Aucune borne générale ni tour FULL.
Les quatre builds sont épinglés, y compris l'échec GCC-TSan et sa reprise
Clang distincte. GCP non utilisé.

## Historique : treizième tranche publiée à b268cf6f

Treizième tranche P0,
[front et census q2 multi-CPU](../docs/P0_FRONT_WORKERS_Q2.md) implémentés.
Qualification propre close : 53 CTests Release/Clang ASan/UBSan,
ThreadSanitizer et32 différentiels ancien/nouveau mono passent.
Les [134 mesures propres](../receipts/q2_front_workers_20260914/README.md)
sont closes, lecteurs normal/−O identiques. Les trois builds sont épinglés.
Sur quatre cœurs physiques, médianes8k/K10 un/quatre workers : ×3,91
uniforme, ×3,40 terrain, ×3,89 amas, ×1,93 rangées. Les principaux
comptes restent sous ×4 à chaque doublement8k/16k/32k ; aucune borne
globale déduite. Les deux affinités de CPU restent séparées. Aucun résultat
de tour FULL ou G4 transféré depuis le mono ni les prototypes d'audit.
Les sorties et compteurs géométriques sont confrontés au mono et à
une force brute indépendante, les threads sont joints même sur erreur.
GCP non utilisé.

## Historique : douzième tranche P0 publiée à ba11e3ab

[Pool terminal q2](../docs/P0_POOL_TERMINAL_Q2.md) implémenté et qualifié
localement : 49 CTests Release/Clang ASan/UBSan PASS, lecteurs normal/−O
identiques et 80 mesures closes. Les deux builds Pool terminal sont
épinglés. Le plan partage
l'index global et ne développe que les bandes résiduelles ; si aucune
paire n'est éliminée, le chemin de census initial est conservé. Cette
politique ne tronque rien et laisse visibles les coûts de préparation.
Les [reçus propres](../receipts/q2_terminal_pool_20260914/README.md) sont
distincts de ceux du prototype A fbbecc01 et de la tranche conjointe.
À s8/K10, les amas8k/16k/32k passent de 13,412/47,179/184,306 s
à 3,589/7,614/19,180 s ; les visites census font ×2,958/×2,701
contre ×4,106/×4,229 sans Pool. Aucun gain de travail sur uniforme
et terrain, où aucun plan n'est sélectionné. Croissance favorable
observée, aucune borne globale déduite. Priorité suivante : sous-arbres
du front et census des nombreux petits rectangles par workers privés.
P0, FULL, parallélisation et contrats G4 ouverts ; GCP non utilisé.

## Historique : onzième tranche P0 publiée à b2106c3c

[census conjoint A×B](../docs/P0_CENSUS_CONJOINT_Q2.md) qualifié localement
en r2 : 47 CTests Release/Clang ASan/UBSan passent, 44 mesures closes
et lecteurs normal/−O identiques. Les compteurs conjoints sont séparés ; reprise à
ancre fixe sans perte ni redoublement de crédit. La capture initiale
conserve un échec de collecteur à l'interruption, corrigé avec six
contre-fixtures déterministes. Le code C++ ne change pas entre captures.
[Preuves et mesures r2](../receipts/q2_joint_r2_20260914/README.md).
Le [raccord Pool suivant](../docs/P0_POOL_TERMINAL_RACCORD.md) reste
non intégré au produit ; l'audit A fbbecc01 en mesure désormais un
prototype q2 complet. Aucun résultat de tour ou de parallélisation acquis.
Sur les amas, les visites A seul font encore ×4,107 puis ×4,231 à
8k/16k/32k, pour 12,121/45,934/182,155 s ; la croissance demandée
reste en échec dans ce régime. Le défaut Individual est conservé.

## Historique : dixième tranche P0 publiée à e3af11a7

[ordre complément/B original](../docs/P0_ORDRE_TEMOINS_Q2.md), option
SharedBlocks à contexte et continuation compacts. Aucun crédit préchargé,
aucune modification de la collecte. Les quatre opérations structurelles
sont comptées séparément. 45 CTests Release/Clang ASan/UBSan passent ;
[56 mesures propres](../receipts/q2_witness_order_20260914/README.md) closes,
sans transfert des résultats antérieurs. Préflight avant gel conservé.
À s8, Complement/sibling donne 0,201/0,437/0,942 s sur rangées
8k/16k/32k, contre 11,378/43,750/173,471 s sur amas. Les visites des
amas font ×4,106 puis ×4,229 : croissance non résolue, malgré des
supports proches du linéaire. Uniforme8k ne gagne pas en temps ; le
coût structurel est explicite. Le défaut reste Global/none.
La contrelecture modèle A est publiée à a1ee8cb0 et reste indépendante.
Partage conjoint A×B et reprise singleton sans redémarrage : prochaine
proposition, non implémentée ; P0 et contrats FULL/G4 ouverts. GCP non utilisé.

## Historique : neuvième tranche P0 publiée à 39b58f37

[certificat autonome du frère q2](../docs/P0_CERTIFICAT_FRERE_Q2.md),
option intégré SharedBlocks seulement. Le frère doit certifier K sites
stricts à lui seul ; jamais d'ajout au compte ou de modification du
curseur Z. Le défaut est inchangé. Les
[32 mesures et qualifications propres](../receipts/q2_sibling_20260914/README.md)
sont closes, 44 CTests Release/Clang ASan/UBSan passent et lecteurs
normal/−O identiques. Les essais initiaux en échec sont conservés ;
le runner corrige une course d'interruption pendant Popen sans perte
de ses sorties, six nouvelles contre-fixtures déterministes passent.
Rangées8k : 1,494→0,241 s ; sibling8k/16k/32k :
0,241/0,481/0,969 s et évaluations ×2,20/×2,13. Amas :
×4,29/×4,22, donc échec persistant de croissance malgré les rejets
supplémentaires. s8/10/12 comparés à 8k seulement dans cette tranche.
L'ordre compact complément/B original et l'exclusion de a du seul
comptage sont une proposition contre-vérifiée, pas encore un produit.
P0, FULL et GPU restent ouverts. GCP non utilisé.

## Historique : huitième tranche P0 publiée à f7edd646

[front et census q2 raccordés](../docs/P0_FRONT_ET_CENSUS_Q2.md),
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Un seul nuage/index, pas de préparation B par rectangle,
pas de scan de couverture ; collecte complète des intérieurs/coquilles.
Le front demandé q2 seul ne paie pas Xi. Le gate indépendant passe
1 255 appels et compare 46 762 supports, pas seulement leur masse.
Les 43 CTests Release et Clang ASan/UBSan passent ; contrôles CLI et
lecteurs normal/−O inclus, sans désactivation des fuites.
Les [preuves propres à ce raccord](../receipts/wspd_q2_census_20260914/README.md)
ne sont pas celles du front historique à trois voies. Les 53 mesures
closes couvrent quatre familles, n8k/16k/32k et s8/10/12. Sur amas/s8,
visites 0,973→4,199→17,665 milliards, soit ×4,315 puis ×4,207 malgré
des supports proches du linéaire : la croissance demandée échoue dans
ce régime. Le certificat autonome du bloc frère était alors une proposition
contre-vérifiée, non intégrée, pour éviter le raffinement prématuré.
Le coût des
ancres et des visites Z, la canonisation globale des boules, q3/q4,
FULL, les continuations parallèles et les contrats G4 restent ouverts.
GCP non utilisé.

## Historique : septième tranche P0 publiée à da366f7f

[front WSPD réel](../docs/P0_FRONT_REEL.md), sans plan ni scan de facteur
par produit, masques q2/q3/q4 et propositions de coût O(D+K) par produit.
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Les [preuves et mesures](../receipts/wspd_front_20260914/README.md)
portent sur le front, pas encore son raccord au census ou FULL.
Le ledger massif est distinct de la couverture exhaustive bornée du juge.
40 CTests Release/Clang ASan/UBSan passent. Uniforme32k/s8 : réduction
56,8→20,9 millions de rectangles mais coût 4,87→37,4 s, avec 954 millions
de pas d'index. Résultat négatif conservé : filtre sûr, pas encore gagnant
en temps de front. Le raccord census doit décider du coût total.
72 mesures closes, lecteurs normal/−O identiques : quatre familles,
n8k/16k/32k et s8/10/12. Les résidus des amas font presque ×4 à chaque
doublement ; ne pas appeler cette chaîne sous-quadratique. Les builds
`v8_front_20260914` et `v8_front_sanitize_20260914` sont épinglés.
Les deux rangées parallèles réfutent une résolution universelle de q3/q4
par témoins ponctuels seuls. Reprises, travailleurs CPU et GPU restent
à implémenter ; contrats 50k/G4 et massif ouverts. GCP non utilisé.

## Historique : sixième tranche P0 publiée à 85015a8c

[nuage/index partagé](../docs/P0_NUAGE_ET_INDEX_PARTAGES.md),
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. 37 CTests Release/Clang ASan/UBSan passent. Les identités
nuage/rectangle/ordre sont préservées, la préparation globale est unique
et les boîtes des facteurs sont obtenues en O(log n), sans scan local.
Le terme Ω(R|B|) de Pool/Axis reste ouvert ; R doit croître avec n dans
les tests, et pas seulement rester fixe. Le nouvel auditeur B précise le
régime du front pur v4 et la différence de convention s, sans qualification
v8 héritée. Pilote WSPD, préparations de facteurs, continuations, q3/q4,
FULL et GPU restent ouverts. GCP non utilisé.

Les [66 essais locaux](../receipts/cloud_reuse_20260914/README.md) passent
les lecteurs normal/−O. Le partage divise le travail de préparation globale
par R, mais R croissant donne ×3,94 puis ×3,97 sur les copies locales de
restrictions des grilles. Les nappes subdivisées restent coûteuses ; cette
capture ne clôt donc ni P0 ni le contrat de tour. Les temps locaux et les
preuves de préparation sont distingués de toute qualification G4.

## Historique : cinquième tranche P0 publiée à 3c29ea1e

[bornes préparées](../docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md),
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`. Les 34 CTests Release et Clang ASan/UBSan passent ;
1 424 cas pour la nouvelle primitive, les gates census restent inchangées.
Les [124 mesures](../receipts/q2_prepared_bounds_20260914/README.md)
comparent f481 et le nouveau moteur, mêmes compteurs et digests.
Sur grille/nappe32k/K10/s8, baisse du temps total Shared de 4–10 % dans
les médianes répétées ; pas de domination universelle ni changement
de complexité. Les doublements observés restent inférieurs à ×4,
sans prouver la borne globale. Propriétaire/index global sur WSPD,
continuations parallèles, q3/q4 et FULL restent ouverts. GCP non utilisé.

## Historique : quatrième tranche P0 publiée à f4815cd4

Census q2 individuel
et partagé, index global immuable et curseur de continuation DFS ; collecte
séparée des IDs intérieurs/coquille. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. Le nouveau gate géométrique passe
277 cas. Les 31 CTests Release et Clang ASan/UBSan passent ; les
[204 mesures appariées](../receipts/q2_census_20260913/README.md) et leurs
lecteurs normal/−O sont clos. L'intersection gagne sur les grilles quand
on paie l'aval ; le partage seul ne domine pas tous les résidus. À 50k/K10,
le composant individuel mesure 176–186 ms sur grilles et 4,05–4,09 s sur
nappes, sans tour. Lire le
[contrat census](../docs/P0_CENSUS_Q2_PARTAGE.md). La représentation à curseur
reprend explicitement la preuve publiée `f47559b1` de l'auditeur, sans
transférer ses résultats de prototype au produit. Canonisation globale,
q3/q4, WSPD complète et FULL restent ouverts. GCP non utilisé.

## Historique : troisième tranche P0 publiée à f5430f57

Mode axial additif,
intersection intégrée avec un plan local q2 et suppression des allocations
aussitôt remplacées. `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`, hors registre. 26 CTests Release
et Clang ASan/UBSan ; [648 mesures valides](../receipts/additive_q2_20260913/README.md),
sans expansion des grandes candidates, census ou FULL. Le résidu des
nappes alignées diminue mais leur sélection ralentit. L'intersection
est plus sélective que Pool seul, dont la préparation reste plus rapide.
Lire le [contrat](../docs/P0_ADDITION_ET_INTERSECTION.md). Les contrelectures
mathématiques des deux auditeurs sont intégrées ; leurs prototypes de
census et composition restent distincts des résultats du produit courant.
La prochaine étape doit mesurer le coût complet du census q2 partagé.
P0, complexité générale, contrats 50k et massif restent ouverts. GCP non utilisé.

## Historique : deuxième tranche P0 publiée à 8e406f9b

Préparation Tubes partagée
entre q2/q3/q4 et filtre axial q2 par colonnes exactes et index B.
`cpu_reference`, `quantized_u16_input_only`, `implementation_v8_p0`,
`not_claimed`, hors registre. Vingt et un CTests Release/sanitizers et
[594 mesures appariées](../receipts/shared_axis_20260913/README.md), sur
un rectangle à la fois, sans census ni FULL. Le partage garde les mêmes
plans ; le filtre axial est sûr mais spécialisé et peut être inopérant
après rotation. Voir le [contrat](../docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).
L'auditeur a fait corriger l'affectation après panne mémoire et deux
omissions des reçus appariés. Ses propositions additives et de queues
A/B restent distinctes des optimisations effectivement mesurées.
P0, contrats 50k, GPU et massif ouverts ; GCP non utilisé.

## Historique : première tranche P0 publiée à 3589a2c9

Implémentation ouverte ensuite par l'utilisateur le 13 septembre :
`phase=exploration_v8_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=implementation_v8_p0`,
`public_status=not_claimed`. Première brique implémentée : crédits
certifiés et sélection de paires résiduelles sur rectangle séparé,
avec Pool, DualBlocks et Tubes. Huit CTests locaux passent en Release
GCC et sous Clang ASan/UBSan. Les
[729 mesures mono](../receipts/p0_local_credits_20260913/README.md) distinguent
préparation et résidu ; aucune mesure aval ni qualification FULL.
Le [contrat actif](../docs/P0_CREDITS_LOCAUX.md) fixe les preuves, limites
et consignes de raccord. P0 reste ouverte, notamment sur les nappes.
Les défauts signalés sur la copie/affectation du propriétaire, les alias
mutables du tampon d'entrée et l'admission des reçus sont corrigés et ont
des contre-tests permanents. Les coordonnées sont copiées avant certification.
Les premières captures sont conservées à part ; la qualification active
porte sur les builds r3 et les nouveaux reçus uniquement.
La porte d'entrée est satisfaite par les lectures et décisions consignées
ci-dessous ; l'historique suivant décrit l'audit documentaire initial.

La porte d'entrée documentaire est ouverte : demande explicite de refonte,
cadre et périmètre déclarés dans [README](../README.md), base v7 publiée
`dc57ffd5`, état local sale distingué. Les règles du dépôt restent en vigueur.
La lecture intégrale antérieure des parties I/II est consignée dans
[la lecture v7](../../morsehgp3D_v7/docs/LECTURE_ET_CONTRATS.md) ; les
définitions et preuves utiles sont réexaminées pour cet audit.

Trois contrelectures ont traité séparément WSPD/supports/témoins,
contrats et mesures, architecture/CPU/GPU/tests, puis confronté leurs
rapports. La synthèse et les fondements du constructeur ont aussi été
contrelus. Les précisions de régularité, voies actives, compteurs et
maturité ont été intégrées. Aucun avis indépendant futur n'est anticipé.

La [synthèse](../docs/AUDIT_V7_SYNTHESE.md) et le
[périmètre vérifiable](PERIMETRE_ET_PREUVES.md) constituent la livraison
d'audit. Les états « prouvé sous hypothèses », « testé borné », « mesuré »,
« proposé » et « manquant » sont distingués. L'audit couvre la chaîne et
ses contrats, sans prétendre relire chaque ligne des 26 777 fichiers ni
relancer les suites C++. Six lecteurs et deux recalculs exacts sont clos.

Verdict public inchangé : `not_claimed`. Contrats 50k et massif ouverts.
La CLI reste F, les sondes FULL et les prototypes privés sont distingués.
Aucun moteur v8 lors de cet audit initial, aucun usage GCP. Les contrôles documentaires et d'index
de livraison sont conservés dans les reçus v8, sans valeur de preuve moteur.

Décision de priorité ultérieure à l'audit, 13 septembre : l'utilisateur
place en **P0** la suppression des histogrammes systématiques
O(|A|²+|B|²). Le [plan de refonte](../docs/PLAN_DE_REFONTE.md) compare
plusieurs familles d'architectures ; le petit ensemble de témoins ne
constitue pas un choix définitif. Minorants certifiés, coût des résidus
et absence de déplacement du carré sont des critères obligatoires.
C'est une orientation ouverte, pas un nouveau résultat mathématique,
un test moteur ou une qualification de complexité globale.

Complément de passation demandé le 13 septembre :
[VERROUS_ARCHITECTURE](../docs/VERROUS_ARCHITECTURE.md) organise les cinq
autres verrous B1–B5 avec références, changements à comparer et critères
de validation. La note distingue coûts cumulés dangereux, constante liée
à K, sérialisation et résidence, sans confondre ces constats avec la borne
intrinsèque de sortie FULL. P0 reste premier ; aucun moteur ni benchmark
n'est introduit par cette formalisation documentaire.
