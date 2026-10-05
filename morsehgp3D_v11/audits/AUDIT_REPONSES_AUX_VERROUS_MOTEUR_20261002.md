# Audit mathématique courant — supports, hiérarchies et clustering plat

5 octobre 2026. Relecture du contrat S0 et de l'oracle S1 au commit
**5adf6a59f**, puis de la demande du développeur **9290cf3bf** et des WIP
S3/S5/S6. Le socle FULL et les travaux sur les points/tête restent épinglés
aux reçus liés ci-dessous. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Note maintenue en place ; détails et échanges clos dans les reçus.

## Sortie supports : revue du contrat et du raccord

**Contrat S0/S1 relu : favorable sur les points difficiles.** Les lemmes
P/W, les deux lectures du retrait E5 et le témoin D2 concordent avec les
modèles exacts indépendants. D2 ne borne pas la naissance d'une trace par
le niveau précédent : `41<64<1681/25` reste le contre-exemple permanent.
La constance de H₀ transporte sa classe. Le juge E2 admet `initial≤λ`,
puis l'ancêtre fermé ; le journal conserve les traces strictes `initial<λ`.
Le journal et les supports sont attribués après tout le plateau, sans
confondre unions DSU et composantes intrinsèques.

Les **13 mutants S1** sont tués avec leur cause nommée sur les copies
figées ; les coupes exactes de sept fixtures sont recoupées par un
solveur/Γ indépendant. Ce rejeu borné ne remplace ni toute la suite S1,
ni les tests natifs G4. Aucun nouveau défaut mathématique établi dans S0.

**Deux gardes désormais corrigées dans les WIP.** S6 contrôle `p≤11`
avant `p+q`, avec refus UINT32_MAX et frontière `(11,24,2,12)`.
La nouvelle capture S3, `attachment.cpp` SHA **77f84f0050…**, appelle
`published_traces(end−begin)` avant conversion : au-delà de UINT32_MAX,
refus `tower_capacity`. Le plafond 24 de `supports` reste distinct de FULL.
Les fixtures D2/E5 sont maintenant présentes en S3. Le raccord final du
journal est favorable : graines enregistrées avant le DSU, publication
par ordinal, puis attribution après fermeture du plateau. Le compactage
en place conserve `prior_total≤begin` et `prior_total+branches≤end`.
Le refus conserve domaine et diagnostics ; journal, sweep et copie finale
ont leurs coexistences budgétées. **15 283 gardes scalaires nouvelles**
recoupent ce raccord, sans nouvelle qualification native/TSan.

**D.1–D.4 sont adoptés par le contrat.** Les agrégats `kparties_reliees`
comptent des incidences `(b,F)` ; les cofaces sont distinctes par boule.
L'indépendance à Q_b ne vaut qu'à p,m,K fixés. La signature V2 est
recalculable depuis SP, sans PointId/BallIdx ; FUL1 garde ses octets et
ne suffit pas à ce recalcul. L'état `published_complete` et le SHA du
manifeste fermé distinguent publication, transport et durabilité. La
stricte descente du sélecteur comprimé faible ne se transfère pas à toute
K-partie. [Contrat courant](../docs/SORTIES.md),
[réponse du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md).
Ces engagements V2/publication sont désormais présents dans la capture S5,
avec portes du champ publié, SIGXFSZ et refus après publication.

[Relecture actuelle et témoins](../receipts/audit_native_integration_20261005/README.md).
Les constats et réponses précédents restent figés dans les reçus
[contrat S0/S1 et coquille mixte](../receipts/audit_supports_contract_20261005/README.md),
[1 119 gardes](../receipts/audit_supports_implementation_20261004/README.md),
[6 456 gardes](../receipts/audit_supports_followup_20261004/README.md) et
[première revue supports](../receipts/audit_supports_20261004/README.md).

**Réponse à la question du 5 octobre : une coquille mixte au plafond.**
La sphère entière $x^2+y^2+z^2=5$ a 24 sites. Après translation par
`(2,2,2)`, elle est une entrée valide des trois profils. Elle donne
**12 q2, 24 q3, 792 q4**, soit 828 supports stricts, sur toute la coquille.
Exemples avant translation :

- q2 : `(2,1,0),(-2,-1,0)` ; poids1/2 chacun.
- q3 : `(2,1,0),(-2,1,0),(1,-2,0)` ; poids`(1/4,5/12,1/3)`.
- q4 : `(2,1,0),(-2,1,0),(0,-1,2),(0,-1,-2)` ; poids1/4,
  déterminant−32.

La fermeture est non triviale : $N_2=12$, $N_3=288$, $N_4=3906$.
À K3, la somme des cofaces par support vaut 4068, contre 3906 cofaces
uniques : ce témoin tue précisément leur confusion. À K1, les 792 q4
restent publiés malgré zéro coface par q4.

| K, p=0 | K-parties reliées | Traces strictes | Cofaces par boule |
| --- | ---: | ---: | ---: |
| 1 | 24 | 24 | 12 |
| 2 | 276 | 264 | 288 |
| 3 | 2024 | 1736 | 3906 |

Le dénombrement exact par combinaisons≤4 est confronté au Gram Fraction,
sans parcourir les $2^{24}$ masques. C'est un bon oracle `long` **des
primitives Q_b/N_j**, avec seulement j≤K+1 pour K≤3. Remplacer le seul
calcul N_j dans l'oracle S1 complet ne suffit pas :
`_minimal_nonseparable` prolonge aussi les sous-parties séparables
jusqu'à m. Garder un budget/refus explicite pour cette extension ; aucune
censure silencieuse ni qualification de tout S1 à 24 sites par ce témoin.
Les autres sphères proposées sont des compléments facultatifs ; préférer
ce cas mixte, puis 25 des 30 sites de rayon carré9, en conservant les six
points axiaux, pour le refus support_shell_capacity de l'appel entier.
Le plafond ne s'applique pas à FULL. La boule canonique de ce témoin
vient d'un diamètre : il exerce Q3/Q4 sur une présentation q2 et ne
remplace pas les portes numériques des centres de présentation q3/q4.
[Énumération indépendante et mutations](../receipts/audit_supports_contract_20261005/qb/README.md).

**Compléter les portes aux K élevés, sans construire FULL12.** Sur cette
même coquille, toute partie de taille≥13 contient une paire antipodale,
donc `N_j=C(24,j)`. À taille 12, 116 des 4096 choix sans paire sont séparables ;
un calcul indépendant des chambres de douze plans centraux retrouve 116.
Ainsi **N12=2 704 040** et **N13=2 496 144**. La primitive à K12 doit donner
**2 704 156 parties reliées/comprimées, 116 traces strictes et
2 496 144 cofaces distinctes/Gabriel** ; les 149 954 688 incidences par Q
ne remplacent pas ce dernier compte. Récupérer la fermeture centrale dans
Cat1, puis appeler `make_shape`/`ball_counts` : cela ne qualifie ni un arbre
FULL12, ni `ball_shape` à K12 sur un domaine préparé à K1.

Un deuxième témoin, seulement 12 sites, exerce p=8,m=4,qmin2 à K10 :
**66 parties reliées, 6 comprimées, 4 traces strictes, 12 cofaces et
4 Gabriel**, contre 20 incidences par Q. Il complète la porte LiDAR longue
et les petits cas S3 actuellement K≤5. S6 compare déjà N2/N3/N24 à 24 sites ;
ces témoins ajoutent un attendu indépendant aux indices intermédiaires hauts.
Ce sont des lacunes de couverture, sans échec natif observé.
[Coordonnées, preuve et 4 135 gardes exactes](../receipts/audit_native_integration_20261005/qb/README.md).

**Décisions courantes : Q_b seul, puis points, puis plat.** La réponse primaire
du 4 octobre à 20:25 UTC retient le squelette des supports ; la proposition
`POP=P_b` du plan révisé est donc caduque. « K-parties reliées » désigne
explicitement les sommets de Γ_K reliés **par la boule**, distincts des
cofaces et des unions DSU. Le triangle aigu à K2 en relie trois, alors
qu'aucune K-partie ne contient son Q de taille3 : ce dernier compteur ne
répondrait pas au choix utilisateur. **S2/257aabb92 et S4/f98aeed67 sont livrés en
source** : en-tête public et io. L'attribution S3 et l'énumération Q_b/S6
ne sont pas encore livrées dans ces sources ; leurs rapports locaux ne
constituent pas une qualification G4 du futur export.

**D2 : correction de preuve reprise dans le contrat mathématique L0.**
L'ancienne justification `β(F)≤niveau critique précédent` est fausse. À K2, prendre
A=(2,10,0), B=(18,10,0), C=(10,20,0), Z=(9,3,0), W=(11,3,0).
La boule ABC est retenue au niveau carré1681/25 ; sa trace stricte AB a
MEB de niveau64, absente de Cat2 parce que p2+qmin2>K+1. Le niveau
retenu précédent est41 : **41<64<1681/25**. AB n'existe pas à41,
mais sa graine ZW, née à1, continue vers la bonne composante à64.
T5 s'applique à `max(niveau précédent,β(F))`, puis la constance de H0
jusqu'au prochain événement transporte la classe vers la coupe précédente.
L'ancêtre de la graine et le propriétaire après plateau restent cohérents.
Ne pas transformer l'inégalité fautive en garde d'admission native.

**Limiter l'identité des octets à ce que le format peut garantir.**
Le plan révisé promet des octets identiques après réétiquetage, tout en
conservant `SITES.point_id` : ces deux exigences se contredisent. Les labels
plats suivent l'ordre d'entrée et les manifestes hachent les fichiers bruts.
Comparer les structures géométriques après transport des IDs et permutation
inverse des lignes ; recalculer les hashes de provenance. Même entrée et
même requête permettent la garde d'identité sous changement de workers.
Sous réétiquetage, la règle de nommage des clusters doit aussi être appliquée
aux nouveaux IDs ; transporter seulement l'ancien label minimal ne suffit pas.

**Deux portes précises pour S6.** Le helper Euler marque Q_b, puis la
fermeture zêta modifie ses bits : les6 supports du cube deviennent177
parties. Extraire les supports avant cette transformation. À K1, les deux
q4 du cube restent dans Q_b malgré zéro coface d'ordre2 : ne pas filtrer
les supports par `|Q|≤K+1` ou par un compteur de cofaces positif.
Ce sont des gardes pour le port prévu, pas des défauts d'un module livré.

[Contrelecture, décision primaire et preuves](../receipts/audit_supports_followup_20261004/README.md) :
**6 456 gardes portables**, normal/−O identiques ; aucun natif, fit ou GCP.

**Conseils précédents intégrés, à garder comme portes.** Arbre K seul,
propriétaire fermé après tout le plateau, continuations datées, Q_b depuis
toute U et toutes arités, fenêtre faible `p+qmin−1≤K≤p+m` : conception
favorable. `C(m,K−p)` compte les parties comprimées contenant tout I,
`C(p+m,K)` les K-parties fermées ; la somme des cofaces par Q compte des
incidences. Les unions DSU ne sont pas intrinsèques aux boules. Les
géométries des supports peuvent se recouvrir entre branches : leur
intersection ne définit pas la connectivité FULL. La coquille n'est pas
bornée par K ; le chrono FULL ne qualifie pas ce nouvel export.

**Q_b reste discontinu comme carrier.** Quatre points cardinaux du cercle
portent deux diamètres ; une perturbation rationnelle du point supérieur
ajoute un triangle et donne un saut de Hausdorff≥1/4, pour un déplacement
d'entrée tendant vers zéro. FULL stable ne suffit donc pas à qualifier un
tokenizer stable. Neuf immersions exactes sont jugées en u21 ; la limite
analytique ne devient pas une limite de perturbations arbitraires sur ce
domaine fini. [Preuves, fixtures et 1 365 gardes](../receipts/audit_supports_20261004/README.md).

## Résultats utiles pour la tête plate

**Le critère B se calcule par une rencontre et sa date est stable en 3ε.**
Pour H_i=(o_i,e_i) et core_i=(c_i,d_k(x_i)), self compris :
`sB_i=max(e_i,d_k(x_i),naissance(LCA(o_i,c_i)))`. Un LCA par point
remplace le balayage de toutes les coupes du prototype. La preuve combine
l'alignement fort H3 et le segment entre points appariés, couvert au rayon
d_k+2ε ; k/m/IDs fixes, nuages finis, contacts fermés. **156 dates B**
concordent avec le code privé extrait par AST. Cela ne restaure pas les
points frontière perdus par B, ne stabilise pas une antichaîne EOM à égalité
et ne prouve pas une meilleure pertinence statistique.
[Preuve et 12 968 gardes exactes, normal/−O](../receipts/audit_giant_20261004/mathematics/README.md).

**B garde également le plafond L6 d'H.** Poser ρ=d_k(x), D=e−t′.
Pour le premier propriétaire qualifié p=(C,t′), choisir y∈C,
ℓ=|x−y|≤t′. Les k points de sa boule donnent ρ≤t′+ℓ. Le segment des centres
est couvert dans L_k au rayon S=max(t′,(ρ+t′+ℓ)/2), avec
ρ≤S≤t′+ρ/2. Il relie p au core, même si ce dernier n'est pas qualifié.
Ainsi **sB≤t′+ρ/2** et **0≤sB−e≤ρ/2−D**. Nuages finis, premier
propriétaire atteint, k/m fixes, sites unitaires et contacts fermés : mêmes
hypothèses que la preuve B3. Cela évite de cumuler deux plafonds pessimistes.

Ce supplément peut pourtant approcher ρ/2 : triangle
x=(0,0), y=(2N,1), z=(2N,−1), k2/m3, e=t′=(4N²+1)/(4N), sB=ρ=√(4N²+1).
La limite concerne une famille géométrique non bornée, pas une constante
optimale sur le domaine fini u21. B peut encore perdre une branche frontière ;
aucune optimalité statistique de B n'en découle.
[Preuve adverse, huit couples nuage/ordre et 931 gardes](../receipts/audit_deep_20261004/math/README.md).

La revue FULL détaille MEB/supports, Γ↔Lk, morceaux stricts, descentes,
multifusions et verticales. Sur huit nouveaux nuages : **32 ordres et
1 082 coupes ouvertes/fermées**, contrôle de la couverture complète de
chaque composante, parents/verticales/core compris. Aucun nouveau défaut
mathématique FULL établi ; les théorèmes restent nécessaires au-delà du
domaine borné. [Chaînes de preuve et limites](../receipts/audit_giant_20261004/mathematics/PROOF.md).

**La condensation et le raffinement en z sont confirmés mathématiquement.**
À arbre et cohortes fixés, augmenter z dans λ=r^(-z) raffine la sélection EOM,
racine d'admissibilité fixe, parent aux égalités certifiées. La preuve traite
le masquage des décisions par les ancêtres ; 728 comparaisons exactes sur
26 condensations concordent. Le raffinement porte sur les clusters sélectionnés ; agréger tous les
points bruit en un bloc ne transfère pas ce théorème à cette partition.
Cela ne compare ni deux modèles d'existence,
ni leurs IoU, et ne prouve pas l'optimalité statistique de z=1 ou z=3.
E1 a depuis fixé **z=2 sur le dev synthétique** ; P08 distingue z=2 pour
les fusions et z=1 pour la non-infériorité IoU. Ces choix ne constituent pas
une optimalité mathématique ou statistique universelle.
[Preuve indépendante et 12 917 gardes](../receipts/flat_model_followup_20261004/README.md).

**Une séparation fugace ne force pas l'union pour tout z.** L'explication
de `SORTIE_PLATE.md` §3.2 est corrigée en **723cf6e43** : seuls les z=1,2,3
testés sont concernés pour les trois vélos b00_001472/k5. La condensation
A/mcs20 réelle possède
une union admissible de masse 284, avec deux enfants de masses 116 et 97,
dont les cohortes ont une persistance strictement positive. À **z=16**, après
le même facteur positif 100^16, son score est **≤70,394**, contre
**≥132,525** pour la somme des deux enfants. Encadrements rationnels
certifiés, **504 gardes**, normal/−O ; l'union perd donc aussi face au meilleur
score descendant. Dire « les z=1,2,3 testés retiennent l'union ».
Cela ne recommande pas z=16 et ne prédit pas une meilleure qualité par objet.
[Cohortes et certificat autonome](../receipts/audit_selfreview_20261004/README.md).

Pour des dates de comptage s_i≥e_i, R_i est la mcs-ième valeur de
max(u_ij,s_j). Les blocs admissibles sont exactement ceux du facteur
**max(u_ij,R_i,R_j)**. Mais un point ne contribue au score qu'à
max(R_i,s_i) : transporter son calendrier et ses cohortes. Une condensation
standard du facteur ne conserve les scores automatiquement que pour A,
s_i=e_i. Témoin géométrique à six sites, calendrier différé admissible :
score 1/5, contre 3/10 si chacun est compté dès R_i. Ce calendrier n'est
pas présenté comme la valeur B/C/E de ce nuage. Pour les poids fixes, le
rang devient un seuil de masse cumulée.

**Une égalité EOM peut être certifiée sans la supposer au budget.**
Pour e=√t+√M−√q>0, poser d=t+M−q, Δ=d²−4tM. Si Δ≠0 :

```
1/e = [(t−M−q)√t + (M−t−q)√M + d√q − 2√(tMq)] / Δ.
```

Si Δ=0 et e>0, e=2√min(t,M) ; les dates nulles exigent leur politique propre.
La réciproque et son cube utilisent au plus quatre classes carrées **pour
cette forme H**, et pour B/C qui choisissent H ou une date core. Ce domaine
ne couvre pas une maturité générale Eθ : une combinaison de quatre radicaux
indépendants exige huit classes dans le témoin algébrique relu, sans nuage
géométrique correspondant revendiqué. [Garde de portée](../receipts/audit_deep_20261004/math/README.md).
Le regroupement des classes certifie un score nul ; le signe non nul demande
encore intervalles, budget et refus. Témoin HGP entier plongé par x↦(x,x,0) :
entrées 6√2, fusion A/B à 8√2, racine à 12√2, scores parent/enfants **√2/8**.
La tête privée par intervalles choisit le bon parent mais signale un choix
forcé à 128 bits. Ce n'est pas une erreur de labels ; la garde aide à qualifier
la vraie égalité. **888 gardes normal/−O**, mutation du facteur 2 tuée.
[Formule, cas singulier et helper autonome](../receipts/eom_exact_audit_20261004/README.md).
Aucune borne rapide/native ne découle de ce regroupement naïf.

**Trois gardes préviennent des raccourcis futurs, sans défaut produit établi.**
La croix de quatre sites à k3 naît avec population4>k : population=k est
suffisante pour une naissance, pas nécessaire. Pour X={0,2,4,6}, k2/m4,
la qualification arrive à2 alors que la première couverture d'ordre4 est3 :
l'identité t′=α_(k+1) ne se généralise pas à α_m. Enfin la naissance de la
racine ne borne pas toutes les attaches/core : un témoin a root birth²16,
qualification²25 et core²80. Conserver ces niveaux dans l'arbre compact.
[Rejeux exacts et hypothèses](../receipts/audit_deep_20261004/math/README.md).

## Contrats de la tête plate et du port natif

Conserver comme bras explicite : masses entières des sites engagés, mcs≥2,
plateaux N-aires exacts, antichaîne globale, racine exclue, parent sur égalité
certifiée, non-affectés en bruit. E1 livre ce bras en Python ; z=1/2/3 et
feuilles sont distincts, les complétions et autres calendriers sont reportés.
Tester chaque bras primaire contre l'oracle. Les masses de
faces fractionnaires de la thèse changent le modèle et perdent T0 à mcs=3.

- Pour mcs≥2, e_i≤u_ij permet d'ignorer les naissances singleton ; construire
  néanmoins le véritable arbre de points, avec les niveaux créés par les
  attaches. Après condensation, une petite branche peut sortir avant e_i^(-z).
  [Contrats](../receipts/flat_selection_contract_20261004/README.md),
  [précision sur les sorties condensées](../receipts/flat_selection_math_r2_20261004/README.md).
- L'ordre exact des dates ne qualifie pas les sommes EOM. E1 distingue
  désormais égalité certifiée, signe séparé et refus au budget ; les anciens
  arbitrages flottants ne sont pas ses décisions. La comparaison contre
  l'oracle initialement livrée couvre z=1/z=3 et feuilles ; **z=2 est ajouté
  en 723**, avec une fixture distinctive, mais son nouveau rejeu G4 reste
  attendu avant de transférer cette qualification.

La stabilité 3ε de H ne garantit pas la stabilité de la partition à une
égalité EOM. À arbre/cohortes correspondants et rayons≥r0, une marge de score
supérieure à sa borne d'erreur garantit le choix. Les neuf sites
A={0,2,4}, B={7,9,11}, D={17,19,21} donnent A∪B|D à z=1 et A|B|D à z=3 ;
une petite perturbation près d'un ex æquo peut aussi inverser le gagnant.
[Preuve exacte et garde de marge](../receipts/flat_selection_math_20261004/README.md),
[correction de portée](../receipts/flat_selection_math_r2_20261004/README.md).

## Comparaison à HDBSCAN et métriques

Le rapport d'équité a adopté **R0 officiel intact** et un bras distinct à
sélection N-aire commune. C'est le bon périmètre ; les versions locales
1.9.1/NumPy2.5.3 et G4 1.7.2 restent à qualifier sur le même banc.
520/2 400 partitions diffèrent à epsilon=0 après atomisation ; le total
1 015/6 752 mélange sorties officielles et secours après exceptions.
La comparaison de sources 1.7.2/1.9.1 n'est pas une nouvelle porte G4.

**Corriger M3 du rapport de mesure : B(H) plafonne uniquement des clusters
qui sont des blocs de cet H.** Sur leur fixture F2, R0 produit {6,9}, union
binaire qui n'est pas un bloc du plateau atomique. En prenant ses trois
clusters comme cible : score plat 1, plafond atomique 5/6 (7/9 sans
singletons). Ce n'est pas une anomalie de score observé : les univers diffèrent.
Réserver le plafond atomique aux bras N-aires, ou annoncer un plafond R0
sur son propre arbre binaire. [Témoin et 39 contrôles](../receipts/flat_evidence_followup_20261004/README.md).

Le meilleur bloc par objet peut sélectionner parent et enfant simultanément :
ce n'est pas une partition. E1 mesure désormais une antichaîne et un
appariement global un à un ; meilleur-IoU reste un diagnostic. La portée
revendiquée du vérificateur hongrois a été réduite, sans imputer un score
réellement faux à l'absence de certificat dual.
[Archives et gardes](../receipts/flat_selection_evidence_20261004/README.md).
Le nouveau lot de membres fournit un cas réellement compatible avec une
antichaîne : deux vélos, k5, blocs HGP133/83 disjoints, IoU0,964/0,711.
À k10, leurs meilleurs blocs sont imbriqués. Le nouveau diagnostic plat
k5/mcs20 de ce bout ne retrouve justement pas les deux meilleurs blocs :
EOM z=1/z=2 donne IoU par objet **0,529/0,500**, le second ne franchissant
pas le seuil strict >1/2. Ce sont les observables du renderer, pas un nouveau
calcul hongrois ni une nouvelle mesure de notre part. Garder distincts
compatibilité d'antichaîne, sélection effectivement jouée et coupe commune.
[Diagnostic plat publié](../../Zoltan/demos/hgp_reussit_hdbscan_echoue/bout_00_001470_deux_velos_43_61/plat.json).
[JSON clos et vérification des intersections](../receipts/audit_deep_20261004/math/README.md).

**Attribution : documentation corrigée, protocole encore à aligner.**
`SORTIE_PLATE.md` retire en 723 l'inférence de contribution nulle sous
T−A non significatif. Le préenregistrement JSON conserve pourtant cette
inférence dans `decision_rule.attribution` et `predictions.PS2`.
Consigner un corrigendum d'interprétation : « contribution supplémentaire
de la hiérarchie non établie ». Conserver seuils, prédictions historiques,
décomposition T−R0=(T−A)+(A−R0), estimations et incertitudes. Une absence de
rejet n'établit pas la nullité. [Six recoupes du décalage](../receipts/audit_ports_20261004/head_prereg_delta/README.md).

## Socle FULL → points : acquis et limites

Règle étudiée à k≥2 fixé : P1 après qualification m=k+1, en rayon, coupes
fermées ; e=t′+sup_q(m(p,q)−h(q)), propriétaire vivant au plateau exact.
[Q1–Q8, preuves détaillées](../receipts/points_answers_20261003/README.md),
[contrat courant](../docs/HIERARCHIE_POINTS.md) ;
[réponse du 3 octobre archivée à l’octet](../receipts/audit_supports_contract_20261005/notes_before/REPONSE_CLAUDE_POINTS_20261003.md).

| Question | Résultat actuel et portée |
|---|---|
| Stabilité | Identifiants appariés, poids positifs fixes, k/m fixes, nuages finis : interleaving FULL ε, dates/réunions 3ε ; Pκ : (1+2κ)ε. Aucune garantie d'IoU/labels. |
| Optimalité | P1 optimal dans le domaine abstrait intrinsèque. Constante minimale 3 sur l'image géométrique non démontrée. |
| Retard | 0≤e−t′≤d_k/2, t′≤d_(k+1). Pour entrer strictement avant F, exiger t′+d_k/2<F ; ≤ ne vaut qu'au plateau fermé. |
| Plusieurs k | Six sites 0,3,6,18,19,20 : branches qualifiées k2/k3 croisées à r=7. Pas de laminarité commune automatique. |
| Insertion | L'impossibilité générale Q6 sous trois axiomes est fausse. En revanche, Hausdorff ou W_p normalisé fini ne contrôlent pas uniformément les dates SUP avec seuils unitaires ; W∞/erreur pondérée non tranchés. |
| Modèle infini | Palm donne un déficit conditionnel aux mêmes λ/F, pas une convergence des fenêtres. Localement fini ne garantit pas une rencontre atteinte. |
| Port natif | PointRadiusDate et scores exacts restent à porter/qualifier. L'export FULL C++ + projection Python n'est pas ce module. |

[Insertion/Wasserstein](../receipts/measure_metric_20261004/README.md),
[Palm conditionnel](../receipts/palm_obstruction_20261003/README.md),
[rencontres infinies et κ](../receipts/points_math_followup_20261004/README.md).
κ augmente le rappel dans une même composante FULL ; ni IoU ni partition ne
sont monotones. ER0h n'a pas de constante uniforme pour ses dates en rayon.
Le défaut Decimal de propriétaire au plateau est corrigé et exercé par F.
Le domaine m>n refuse actuellement : déclarer m≤n ou des points inactifs.
[État natif, budgets et preuves G4](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
