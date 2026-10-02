# LiDAR massif et précision — contrat proposé au développeur

2 octobre 2026. Demandes utilisateur : dizaines de millions et précision paramétrable. Produit src/cli inchangé depuis 4b7d70422 ; lecture v10 jusqu’à afb081774 et ouverture v11 52687f8e5. Primitives larges/filtre isolés. public_status=not_claimed. Aucun GCP, allocation massive ou moteur modifié par cet audit. [Massif](../receipts/audit_independant_20260930/massif/README.md), [précision](../receipts/audit_independant_20260930/precision_grille/representation/README.md).

**Décision utilisateur confirmée : grille u32 par paliers u24 puis u32 complet ; float32 natif hors chantier courant.** Exposer le pas physique h, publier le domaine exact certifié et conserver un repère commun. Pour le massif : segments depuis les boîtes de centres certifiées, fusion externe exacte. Cela traite la capacité du catalogue ; atlas, verticales, incidences et reprise restent à concevoir.

## Précision : pas et largeur distincts

Étendue de grille = h·(2^b−1). Cloud/Morton actuels acceptent b≤21 ; générateur et FULL restent **u18**. Distance u128 et Morton96 sont isolés, non raccordés. Notre [complément](../receipts/audit_independant_20260930/grid32_followup/README.md) passe 7 157 contrôles normal/UBSan ; Morton change d'ordre sous translation : jamais un ID persistant ou une clé commune à des origines différentes.

| Pas | Étendue u18 par axe |
| --- | ---: |
| 10 mm | 2 621,43 m |
| 1 mm, défaut courant | 262,143 m |
| 0,1 mm | 26,2143 m |
| 0,001 mm | 0,262143 m |

Une carte de 1 km à 0,1 mm demande 24 bits : dimensionnement, pas qualification. Retirer la garde u18 serait faux : tétraèdre (0,0,0),(m,m,0),(m,0,m),(0,m,m), m=2²¹−1, centre intérieur mais D² de 130 bits, formé en u128 par [level4](../src/arith/geometry.cpp#L88). [Calcul symbolique](../receipts/audit_independant_20260930/precision_grille/representation/README.md), aucune exécution hors domaine.

Contrat minimal proposé :

1. Paramètre precision_mm décimal positif exact ; pas actif 1 mm, proposition large 0,1 mm à déclarer explicitement. Poser h=precision_mm/1000 en mètres pour l'export physique. Profil parmi les voies certifiées ; pas de bits libre supposant la preuve acquise. Calcul large avant conversion, contrôle d'étendue et refus si aucun profil disponible ne convient.
2. Grille, origine et repère communs, arrondi/ex æquo déclarés, puis translation entière commune. Pas d'adaptation silencieuse du pas, écrêtage ou origines indépendantes par tuile.
3. Manifest versionné lié au hash des coordonnées : h rationnel, origine/traduction, repère, profil, unités, IDs, correspondance retours→sites et fusions. Le u32le nu/CLI ne portent pas h/origine et recréent les IDs par ligne.
4. Voie large qualifiée ensemble : Morton/identité, centres, prédicats, niveaux et filtres. Un pas fin ne récupère pas une précision déjà perdue par le capteur.

Niveaux exacts en cellules² : β_phys=h²β_grille, r_phys=h·r_grille, λ_phys=h^(−z)λ_grille. EOM idéal invariant pour le même arbre, masses/z et conventions de zéro. **Requantifier change le nuage.** Garder unité interne et dates exactes. Le [filtre relatif certifié](../receipts/audit_continu_20260929/relative_filter_20260930/README.md) conserve les contacts en prototype ; READY ne certifie pas la positivité du support. Signe exact : borne q3 de 201 bits, protocole générique sur 192 bits jusqu'à 258 bits. Repli, nearest natif et port commun restent à faire ; aucun FULL large ou gain acquis. [Complément ordre/KNN](../receipts/audit_independant_20260930/wide_order_followup/README.md) : deux boules q2 K3 de niveaux E et E+1/4 coïncident en double u32, même physiquement ; coupe exacte au seuil E distincte. Prévalider les niveaux avant tri, jamais traiter un refus comme une égalité. Distances/q2 u24 exacts en double ne couvrent pas q3/q4.

Borne utile au dispatch : MEB certifiée dans une boîte de largeurs Δ donne 4β≤ΣΔ² ; ainsi β u32<2^64, même si les K-NN demandent 66 bits. Le raccourci fermé 4e≥ΣΔ² exige ce certificat et le même profil ; équidistance ou READY ne suffisent pas. [Preuve, neuf MEB exactes et contre-cas obtus](../receipts/audit_independant_20260930/wide_order_followup_bound/dispatch_meb_bound.md), normal/−O. Une valeur β bornée ne dispense pas des numérateur/dénominateur larges ; aucun constructeur ou gain de débit acquis.

## Mesures historiques et dimensionnement

Pas de délai ni enveloppe RAM/disque/sortie massif v10 fixé ; cadre historique GCP G4. Anciens délais/plafonds [historisés](../../docs/PERFORMANCE_MORSEHGP3D.md#L245). Aucun transfert du jalon de 100 ms. Hypothèse : FULL 1..10, repli explicite 1..5 ; attaches, tête, retours/export mesurés séparément puis dans le total.

Session5 CPU : 169 configurations réussies, 7 non lancées sur budget ; une graine/exécution ; 67 synthétiques et 21 morceaux de trois trames de la seule séquence 08. [Extraction](../receipts/audit_independant_20260930/massif/dimensionnement/README.md).

| Même amas synthétique, 1 024 000 sites | K5 | K10 |
| --- | ---: | ---: |
| Boules | 87 574 708 | 478 482 791 |
| Catalogue + tour | 20,8461 s | 124,3633 s |
| Mur du processus | 21,51 s | 126,82 s |
| Pic RSS | 23,405 Gio | 135,284 Gio |

Commandes sans points : tous ordres/verticales, aucune attache, tête ou export durable complet. Lecture/préparation hors catalogue+tour ; masque/quantification hors ligne. B provient d'un processus distinct : conserver comptes/digest du processus réellement chronométré. Aucune capacité de 10–50 M LiDAR ni FULL GPU qualifiée.

303,6–314,9 octets/boule dans les grands cas K10 sont empiriques. **Scénarios seulement** à 30 M sites : 120 boules/site → 3,6 milliards et 1,09–1,13 To ; 460 → 13,8 milliards et 4,19–4,35 To. Ratios/coûts variables. VM capturée : 176 Gio, 79 G disque libres : aucun spool massif dimensionné.

## Verrous avant montée en taille

| Verrou | Correction |
| --- | --- |
| Boules / forêt | B→u32 sans garde au catalogue ; garde proposée en copie R2. Atlas/représentants protègent déjà la [forêt/CSR](../receipts/audit_independant_20260930/forest_cardinality/README.md) valide actuelle. Promouvoir les réserves ; conserver ces invariants globaux dans le futur port segmenté. |
| RankIndex | Milieu sûr et produit élargi intégrés dans 4b7d70422 ; [helper réel, 36 220 cas normal/UBSan](../receipts/audit_independant_20260930/rank_search_preintegration/README.md) passent. Raccord observé dans le binaire u18 ; cardinalité avant cast de level.size() distincte. |
| Atlas | Refus cellules/représentants≥kNone présent, après catalogue résident ; B représentable ne garantit pas l'atlas. |
| Arène ExtCell | rep_first et son addition restent u32 sur l'arène commune à tous K. Les gardes par K donnent Σsr[k], compatible u64 ; garde globale append/span recommandée. [392 contrôles scalaires normal/UBSan](../receipts/audit_independant_20260930/developer_rebound/catalogue/README.md), sans corruption de catalogue géométrique exécutée revendiquée. |
| Mémoire | Budget Buffer partiel, grands vecteurs et budget par défaut illimité. Réserver états simultanés/disque et fermer workers sur refus. |
| Retours | Cloud conserve poids/IDs, mais FULL refuse les multiplicités. Déduplication des scans exige modèle déclaré et correspondance complète. |

N retours, n sites, B boules, P occurrences I/U, L niveaux : catalogue : 42B+4P+56L ; tri : 168B+4P ; assemblage : 179B+8P+56L. Deux derniers pics distincts, hors capacités/socle/tour. Socle≈160n si N=n ; attaches et extraction aval décrites ci-dessous. [Formules](../receipts/audit_independant_20260930/massif/representation/FORMULES.json).

Linéaire en B n'est pas linéaire en n. [Famille rationnelle v7](../../morsehgp3D_v7/docs/CROISSANCE_ET_BORNE_DE_SORTIE.md) : n²/4 naissances FULL dès K2, précision croissante, pas asymptotique infinie dans u18 fixe. Streaming/GPU ne suppriment pas la sortie explicite.

## États aval simultanés : garde nouvelle pour v11

[Revue source actuelle et sonde native bornée](../receipts/audit_independant_20261002/massive_review/README.md) :
les attaches FULL de tous ordres coûtent 12nK octets en core et
(16K−4)n en cover. À 50 M sites/K10 : **6 Go / 7,8 Go**, hors forêts,
catalogue, têtes et capacités. Cover conserve notamment des niveaux core
non utilisés ; distinguer les représentations évite cet élargissement gratuit.
Le core fait déjà UNE recherche kmax par site puis nK descentes/rangs,
pas nK recherches nearest indépendantes.

L'extraction d'un ordre ajoute, catalogue et tour encore vivants,
28Q+4E+20n+4(Rmax+2)+8Ld+4 octets logiques : Q nœuds, E arêtes de cet
ordre, Rmax dernier rang, Ld dates double. Le vote inverse les incidences :
8(n+1)+4Pv persistants, plus 8n au remplissage ; ball_nodes coûte4B.
La CLI clustering traite ses ordres séparément : ne pas y additionner
K caches ball_nodes simultanés. Les vecteurs/capacités et la tête sont en sus.

Nearest ne réserve pas seulement K : ties et candidats ambigus sont
collectés/triés dans des vecteurs privés par worker. **144 sites u18
cosphériques, k=1** donnent144 candidats ; les deux capacités restent256
après une requête donnant seulement2 candidats. Ce témoin prouve le
highwater retenu, pas une panne mémoire ou un LiDAR massif réalisable.
Budgeter aussi ces espaces temporaires et leur durée de vie.

`--repeat` garde l'ancien Catalogue/Tower pendant construction du suivant :
une mesure répétée peut avoir un pic supérieur à une passe seule, sans
empiler toutes les répétitions. Aucune nouvelle mesure de temps massif.

## Segments exacts et prochaine livraison

[GEN §§3.1–3.5](../docs/conception/GEN_v2.md#L110) : N_Kmax(c) inclus dans L(Q), voisins ex æquo compris, émission unique par feuille demi-ouverte. Kmax suffit : p+q_min≤Kmax+1 implique p≤Kmax−1. Conserver I/U complets.

Kmax témoins S donnent R_Q²=max des ||s−v||² sur sommets v de Q. Un bloc global Z avec dist(Q,Z)²>R_Q² peut être rejeté ; égalité conservée. [Piste Q×Z](audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md), sans gain ni borne globale acquis ; listes parfois larges.

[Protocole de moments sur vraies feuilles](../receipts/audit_independant_20260930/developer_rebound/catalogue/MOMENTS_PROTOCOL.md) : capturer S/candidats du générateur, boîtes potentiellement très allongées, ancres hors Q fermé. Groupes fixes préparés une fois, credits recouvrants pris par maximum ; mesurer taux éligible, préparation, Dom et coût aval. Petit témoin cubique ne prouve ni feuille atteignable ni gain natif. La réduction Euler des attaches ajoute aussi O(P) résumés de workers ; éviter une table dense n×W supposée gratuite.

Chaîne : index global → boîtes certifiées → segments triés (niveau exact,S*) → fusion externe → plateaux globaux → atlas/verticales → incidences/points → tête/retours. IDs globaux, rangs exacts uniques, accès disque unions/descentes et réservations par phase à définir. [Tri externe](https://www.ittc.ku.edu/~jsv/Papers/AgV88.IO.pdf), [graphes externes](https://www.ittc.ku.edu/~jsv/Papers/CGG95.external_graph.pdf) : références d'E/S, aucune borne HGP complète.

Point partagé/halo fixe ne suffisent pas : K2 {0,1,2} couvre 1 deux fois à β=1/4, fusionne à β=1 ; {0,1,10,11} naît dans le vide à 81/4, fusionne à 25 avec trois parents. [Calculs exacts](../receipts/audit_independant_20260930/massif/semantique/receipt.json).

**Pour les fondations v11 ouvertes :** fixer le contrat de sortie FULL et de masse frontière, puis pas/manifeste, port u24 complet, gardes de cardinalité et réservation RAM/disque par phase. Puis différentiel résident/segments : plateaux transverses, verticales fermées, incidences internes, segments vides et reprise. Sceller segments et publier seulement les plateaux validés. Hiérarchie de points : un K fixé conformément au choix courant ; le [croisement inter-K](../receipts/audit_independant_20260930/cover_band_followup/README.md) ne bloque pas cette cible.

La [revue d’ouverture v11](../../morsehgp3D_v11/audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md)
précise les décisions de budget, durée de vie et publication transactionnelle.
Les primitives et Cloud B18/21/24 sont désormais qualifiées sur G4 à
`a97180667` ; aucun moteur FULL de ces profils ni moteur u32 ne l'est.

## Préparation v11 qualifiée — 2 octobre 2026

La [lecture cloud immuable v11](../../morsehgp3D_v11/receipts/audit_independant_20261002/cloud_immutable_review_3/README.md)
recoupe vingt portes G4 dans six configurations et le comptage des deux
tampons de tri et du résultat. Au futur raccord index, fixer la durée de vie
du propriétaire emprunté et tester le pic avec les entrées déjà réservées.
Pour n retours/s sites, pic propre=max(2Rn+H,Rn+4n+24s+8), R16 en B18/21,
R32 en B24 ; ajouter les entrées et autres allocations encore vivantes.
Avec s=n et quatre Buffer d'entrée 16n : B18/21=max(48n+H,60n+8),
B24=80n+81920. À 30 M, environ 1,8/2,4 Go décimaux pour ces seules phases,
hors index/catalogue/FULL ; aucune allocation géante, RSS ou mesure de temps.
Le massif reste hors jalon trame v11 ; aucun ancien résultat transféré.
Le [port numérique 9d](../../morsehgp3D_v11/receipts/audit_independant_20261002/native_arity_bounds_review_6/README.md)
élargit la voie native q1/q2/q4 jusqu'à B24 et met u21 par défaut ; la matrice
ffc est conforme 1 014/1 014. Le complément du même paquet exerce q3 natif
B18 sous ASan/UBSan (12 portes num +2 style, séparées de la matrice).
Ces bornes ne limitent pas les candidats.
Le [contrat B/h](../../morsehgp3D_v11/receipts/audit_independant_20261002/precision_mapping_review_6/README.md)
sépare étendue et précision physique : à 1 mm, u21 couvre 2 097,151 m par axe,
u24 16 777,215 m. Un pas plus fin exige de régénérer depuis les coordonnées
d'origine ; IDs, collisions et poids restent publiés. Aucun massif qualifié.

Le [catalogue séquentiel v11](../../morsehgp3D_v11/receipts/audit_independant_20261002/catalogue_capacity_review_4/README.md)
est qualifié à `ffc2ff95f`, sans qualification FULL/100 ms. Deux passes
réservent leurs émissions pendant le second DFS, puis deux populations I/U
coexistent à l'assemblage : pic propre=max(W+T+E,E+F), avec T listes DFS,
W workspace, E émissions/population et F résultat. Ajouter Cloud et tout U
antérieur ; les sizeof incluent l'alignement. Un catalogue linéaire en sorties
peut encore payer de très nombreuses présentations d'une même boule.
La [coquille entière de 150 sites](../../morsehgp3D_v11/receipts/audit_independant_20261002/catalogue_boundary_work_review_4/README.md)
force 20 822 900 préfixes dans sa feuille par passe à K5/K10 ; témoin de coût
local, aucune nouvelle borne générale ni mesure massif.
Le [banc multi profils clos](../../morsehgp3D_v11/receipts/audit_independant_20261002/q4_qualification_review_9/README.md)
termine trois trames sans sol K5 de 35 551–45 845 sites en 19,777–24,962 s
pour B21, pics Buffer avec Cloud 259,73–326,51 Mo ; un essai par trame,
une seule séquence. Leurs K10 expirent. Mêmes XYZ/IDs 1 mm et travail
géométrique déclarés dans B18/21/24 ; aucune précision nouvelle mesurée.
Leurs différences de temps ne constituent pas un gain statistique apparié.
Aucun FULL ou coût de segmentation, aucune extrapolation vers les millions.

[Attribution des quinze pics achevés](../../morsehgp3D_v11/receipts/audit_independant_20261002/q4_cost_attribution_review_9/README.md) :
ils égalent Cloud+E+F, donc la phase d'assemblage domine ces réservations.
Pour n retours uniques, N boules, L niveaux zéro compris, P incidences,
Cloud=28n+8, E=e_B N+4P, F=40N+l_B L+4P+8 ; layouts reconstitués et
corroborés (e_B,l_B)=(96,48)/(104,64)/(112,72) en B18/21/24.
À 08/000200/B21 : 326,510 Mo au pic, 154,031 Mo conservés ; les émissions
transitoires valent 172,479 Mo dont seulement 26,059 Mo de population ancienne.
Réduire le DFS seul ne réduit pas ce pic si E+F domine encore. Conserver
CSR et incidences complètes après tri ; admettre N,L,P et buffers coexistants
avant un essai massif. Ce sont des Buffer, pas RSS ni mesure à 30 M sites.

Le [port q4 différé ffc](../../morsehgp3D_v11/receipts/audit_independant_20261002/q4_catalogue_port_review_8/README.md)
conserve deux passes, I/U et fractions non réduites ; qualification propre
close. Environ 99,7 % des calculs de niveau candidats LiDAR sont évités,
mais les quinze pics Buffer restent exactement ceux de 9df : mêmes émissions
retenues et coexistence avec le résultat. Les rapports de temps observés
ne démontrent pas un gain stable entre ces sessions à une répétition.
L'extension proposée des [témoins de familles sur la ligne propriétaire](../../morsehgp3D_v11/receipts/audit_independant_20261002/q4_owner_line_witness_review_8/README.md)
requiert des signes jusqu'à 201 bits en B24 et le coût du scan ; aucune
borne massive ou voie i128 supplémentaire n'en découle.

Le [raccord parallèle proposé](../../morsehgp3D_v11/receipts/audit_independant_20261002/catalogue_parallel_contract_review_5/README.md)
requiert les capacités des listes parentes et des jobs coexistants : celles-ci
se chevauchent malgré la partition des centres. Workspace à max_leaf,
sorties/incidences et scratch à admettre ensemble ; aucune borne mémoire n×W
ou performance parallèle implicitement acquise.

Le [contrat de raccord de l'index global](../../morsehgp3D_v11/receipts/audit_independant_20261002/global_index_contract_review_9/README.md)
doit admettre aussi les résultats complets déjà terminés et encore vivants :
limiter les workers ne limite pas cette accumulation. Pour T réponses I/U,
borne sûre d'IDs=4Tn, indépendamment de K ; Cloud unitaire=28n+8. Cloud+IDs
seuls donnent environ 0,96/1,80 Go à 30 M pour T1/T8, 1,60/3,00 Go à 50 M.
Sous-totaux analytiques hors index/catalogue/FULL, scratch, entrées et RSS ;
aucune coquille géante réalisée ni qualification massive. Découverte/count,
admission commune et fill exact sont une option ; MemoryBudget::admit ne
réserve rien. Le port e852 possède le Cloud et compte explicitement les
sites, même avec poids ; lecture initiale favorable, qualification G4 en
préparation. Préserver la lignée Cloud et certifier le régime unitaire une
seule fois au futur raccord Catalogue/FULL.

La [preuve géométrique](../../morsehgp3D_v11/receipts/audit_independant_20261002/boundary_stability_review_2/README.md)
donne une borne en rayon pour FULL et les dates de première couverture, quand
tous les retours sont appariés avec déplacement ≤ε. Pour l'arrondi isotrope
au plus proche, sans clipping et dans un repère commun : ε≤sqrt(3)h/2.
Cette borne ne stabilise ni les supports/coquilles du catalogue ni une attache
figée à la première couverture ; le témoin LCA existe déjà en entier u18.
