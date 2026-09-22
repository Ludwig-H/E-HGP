# État courant — audit Morse HGP 3D v9

22 septembre 2026. Rôle : **audit et conseil**, pas développement du moteur.
Le dossier produit `morsehgp3D_v9/` n'existait pas à l'ouverture ; il a
depuis été créé avec des notes d'un autre auditeur. Ce sous-dossier racine
conserve donc notre **photographie indépendante de la v8** ; la suite de
la contrelecture vit dans [morsehgp3D_v9/audits](../../morsehgp3D_v9/audits/CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md).
Aucun GCP lancé pour cet audit.
`origin/main` a d'abord avancé de 13 commits d'audit v8 jusqu'à `12294241`,
puis de l'ouverture v9 `3595725a` jusqu'à `c5ba6e7f`, qui a intégré les
contre-audits A, amendé le plan et publié la tranche u18 v8. Au commit
`d2700314`, arrivé pendant notre contrelecture, un **premier moteur v9**
générateur → catalogue recoupé → tour FULL a été publié. Ces commits
ont été relus sans fusion
dans le worktree partagé, toujours chargé de modifications
constructeur/auditeurs. Ils contiennent des documents, tests et reçus v8,
et désormais du code v9. Le `main` local reste
à `a74e90f2` dans ce worktree partagé. Le reçu 1 mm est désormais
versionné depuis `3f0d188f`, mais ne qualifie que le flux q3/q4 CPU sur
une seule trame, non la tour. Les modifications non commises du
développeur sont examinées en lecture seule, sans les promouvoir en preuve
publiée ; aucune branche parasite créée.
Les commits `ba762036` et `e28296bb` ont depuis publié la campagne FULL,
la porte u18, le protocole G4 SPOT, le jugement de la tour publique T2 et
les refus CLI. Aucune exécution GCP/GPU n'en découle.

Le commit d'audit A `efc14c99` propose une sélection exacte des centres
q4 peu profonds sur une droite de graine en `O(Km)` comparaisons, `m`
étant le nombre de droites du cover. Son oracle local est publié ; la
borne reste **par arête**, et si graines et droites sont toutes deux
nombreuses, elle n'établit pas le sous-quadratique de la trame. La
contrelecture indépendante de cette proposition continue. Son complément
`2291da13` factorise la comparaison des racines par un déterminant
entier signé de moins de 121 bits, au lieu du produit brut de 160 bits ;
cela réduit la largeur de cette primitive, sans modifier le moteur.
Le complément A `ce949e2e` publie un oracle de cover collectif et un plan
de résidence FULL. Son lemme de boule autour de l'arête propriétaire est
correct **si cette arête est maximale dans le support strictement positif** ;
il ne certifie pas les arêtes omises par le front ni les centres non positifs.
Le batch ne livre que des handles de cover : les arrangements et le coût
`s·m` des graines restent à traiter. Les propositions de runs externes et
cache par ordre n'ont pas encore de mesure produit ; le cache actuel est
indexé par facettes, non directement par BallKey.

## Reprise de l'audit au commit `5ab4326c`

La [synthèse produit à jour](../../morsehgp3D_v9/audits/ETAT_COURANT.md)
porte désormais les développements postérieurs à `e28296bb`. Le [premier
reçu G4](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PREMIER_G4_20260922.md)
est réel et clos : huit tours CPU sur trames sans sol de la séquence 08,
grille 1 mm, s8, **K1..5 en 15–29 s** et **K1..10 en 82–125 s** à W48
(70 s pour 000000/K10 avec FULL statique W48). Il utilise le paquet
`e28296bb`, **avant** l'optimisation MEB et le nouveau ledger ; aucun GPU,
contrat d'une seconde ou caractère sous-quadratique n'est acquis.

Le [contre-audit de provenance et de schéma](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md)
recoupe 175 hashes, les huit cas et l'arrêt ciblé. Il identifie deux
blocages **pour la prochaine session**, sans invalider R1 : le worker v2
refuse les champs MEB textuel/tableau de la vraie sonde malgré 17 selftests
verts, et un cas censuré avec `group_closed=false` peut être accepté comme
`partial`. Corriger puis tester le raccord réel avant de facturer G4.
La [contrelecture du MEB diamètre](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md)
est verte localement sur ses petites portes, mais son gain LiDAR n'a pas
de reçu apparié ; les [niveaux q4 orientés](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md)
restent une piste exacte locale, pas un port ni une réduction globale de
`Σ_e cover_sites(e)=2,779` milliards sur la ligne 1 mm publiée.

## Lecture prioritaire

1. [Audit initial v8, héritage et architecture q3/q4](AUDIT_INITIAL_V8_20260922.md).
   Pour les nouveaux lemmes v9, voir la [contrelecture B](../../morsehgp3D_v9/audits/CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md) :
   signes et gardes sûrs, mais couverture des cellules et coût global
   non prouvés ; distinguer catalogue FULL et flux de tous les supports.
2. [Contrat trames entières](../../morsehgp3D_v8/docs/CONTRAT_TRAMES_SEMANTICKITTI_20260921.md) :
   grille entière 1 mm principale pour v9 (décision utilisateur du
   22 septembre), float32 brut secondaire ; premier jalon temps sur trames
   sans sol, puis ligne brute avec sol distincte ; plusieurs séquences ;
   FULL K1..10 <1 s G4, repli K1..5, puis 100 ms. La réussite sans sol
   ne qualifie pas le brut.
3. [Reprise u18/atlas](../../morsehgp3D_v8/docs/REPRISE_U18_ET_ATLAS_SATURANT_20260922.md) :
   les anciennes lignes u16/2 cm ne sont pas des lignes 1 mm ; la seule
   première trame sans sol à 1 mm est un flux q3/q4 CPU, pas FULL.

## Ce qui est acquis, ce qui ne l'est pas

- Des briques géométriques exactes et des filtres certifiés existent en v8,
  avec index partagé, front WSPD et multi-CPU. Les preuves et tests sont
  **locaux à leurs profils, sources et périmètres**.
- La première trame sans sol 08/000000 à 1 mm, 39 885 sites, K5/s8/W8 :
  **104,63 s mur et 812,82 CPU·s** pour q3/q4 seulement. Elle développe
  23,687 M paires, construit 2,044 M covers, paie 3,252 G bornes d'atlas,
  7,316 G tests ponctuels et 5,547 G IDs de frontière copiés. Le tri q4
  compte 163,678 M comparaisons : ce n'est pas le premier goulot.
- Huit workers locaux sont occupés ; le travail excessif est le verrou
  premier. Une arête lourde n'est pas encore subdivisée ; la contention W48
  doit être **mesurée**, pas présumée.
- Ni tour FULL v8, ni catalogue canonique global, ni GPU v8, ni preuve de
  croissance sous-quadratique LiDAR, ni contrat G4 ne sont acquis. La voie
  float32 q3 globale non suivie est un chantier à qualifier, pas q4/FULL.
- La première chaîne v9 `d2700314` relie effectivement les trois voies au
  catalogue et à FULL, avec refus explicite pour une coquille de plus de
  12 sites. Un essai **exploratoire sans reçu** sur 08/000000 sans sol à
  1 mm/K5/W8 rapporte environ 131 s mur, dont 117 s q3/q4 et 11 s FULL,
  1 306 696 boules et coquille maximale 5. C'est un changement de statut
  fonctionnel important, **pas** une qualification des contrats.
- Une [première campagne locale complète publiée à `ba762036`](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md)
  a désormais six lignes sur 08/000000, 000100 et 000200 sans sol/1 mm,
  s8/W8 : **K5 132–264 s**, **K10 381–802 s** de mur, 1,10–1,41 M
  puis 4,38–5,51 M clés. À K10, q3/q4 prend 278–687 s et FULL
  94–130 s. Sur 08/000000, FULL passe de 24,86 M à 1,065 G tests
  de puissance MEB entre K5 et K10 : l'aval devient un verrou propre.
  Les 30 SHA des fichiers versionnés du reçu passent, mais le binaire
  exact de la mesure n'est pas archivé ; le lecteur ne ferme pas toute la
  provenance source/binaire/entrées. La série ne fait varier que K, jamais n : ni
  sous-quadratique ni contrat G4 n'en découlent.
- Le [contre-audit FULL B](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md)
  relève les coûts hérités `2^u`, l'intrus global et les tableaux de
  résolutions, toujours présents dans le port v9. Le recoupement certifie
  les boules **émises**, pas l'absence de clés omises. Le juge T2 utilise
  `run_tower=false` puis appelle la tour directement. Une
  [porte publique T2 séparée du moteur](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md)
  appelle désormais `run_tower=true`, compare son catalogue à un oracle
  rationnel et sa forêt aux coupes Γ pour K1..3 : 24 configurations
  Release et Clang ASan/UBSan passent, avec une q4 non issue des faces q3.
  La mutation ciblée de suppression de clé est détectée **dans le harnais
  après appel**, pas dans le producteur ; ce petit cas ne certifie pas les
  grandes trames.
  `e28296bb` a ajouté un jugement de la **tour publique K1..10** sur trois
  petits nuages, s8/W1 et W4 ; les inventaires s8/10/12 et ses mutants
  historiques restent des routes de test distinctes. Le
  [mini-gate K5 à deux intérieurs](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_Q4_K5_DEUX_INTERIEURS_20260922.md)
  cible encore un autre seuil q4.
  Notre rejeu indépendant du commit `d2700314` a compilé en Release et
  sous Clang ASan/UBSan, **20/20 CTests dans chaque build** ; le
  probe public K1..5 sur un préfixe de
  10 sites a donné un digest identique pour s8/10/12 et W1/W4, sans q4.
  C'est un diagnostic de raccord, non une trame qualifiée ni un oracle.
  Une [fixture entière K3 à 12 sites](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)
  a une q4 valable et **quatre faces q3 rejetées** ; son oracle exact
  passe et le probe émet une q4 sous quatre permutations d'IDs. Elle est
  désormais couverte par la porte d'inventaire, mais pas par un mutant
  compilé de clé omise dans le producteur.
- Le [contre-audit u18/sonde](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md)
  vérifie en C++ O2 et sous ASan/UBSan une fixture de plateau dont un
  produit dépasse int128 signé ; S192 la traite. Les bornes générales
  du port sont désormais échantillonnées par la porte `arith_u18` publiée
  à `ba762036`, que nous avons relancée en lecture seule avec code 0.
  `e28296bb` refuse maintenant `K=2^32+1` avant conversion et restreint
  `--grid` à un alphabet sûr, fermant ces deux défauts de CLI précis.
  La porte `arith_u18` emploie l'oracle
  numérique, mais elle est compilée sans `MHGP9_TESTING` : sa revendication
  de mutant `level-trunc-hi` tué n'a pas de test causal activable.
- Le [contre-audit de résidence](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)
  relève `224P` octets de capacités simultanées pour les présentations,
  un cache temporel par défaut de **24 Gio à 30 M sites** et au moins
  **6,48 Go** de sortie K1 explicite à cette taille. Le cache est
  rescanné à chaque K>1 ; ses octets ne sont pas publiés par le probe.
  Au pic analytique après K1 et avant K2, cache présent, un plancher
  simultané est **43,05 Go + 248B** (`B` clés distinctes), sans overhead
  d'allocateur. La construction K1 impose aussi au moins **trois petites
  allocations par site** au fil de l'appel. Ce sont des planchers de
  format/capacité LP64, pas des mesures de RSS ni des temps extrapolés
  sur G4 ; le cache est libéré avant la publication finale, donc ne
  s'additionne pas mécaniquement à ses 6,48 Go.
- Le [contre-audit du protocole G4 SPOT](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_GCP_SESSION_20260922.md)
  constate des garde-fous de cible/arrêt et un snapshot lié au commit par
  son constructeur. Les scripts publiés mesurent **CPU seulement**
  sur trois trames sans sol de la seule séquence 08 ; un reçu `partial`
  peut retourner code 0 sans aucune tour achevée. Le plafond du plan
  personnalisé est corrigé de 1 024 à 48 fils par `e28296bb` ; 17
  selftests passent en Python normal/−O et un paquet exact de ce commit
  est `prepared_not_executed`, sans appel GCP.
- Le banc publié à l'ouverture v9 juge **v8** : 129 CTests exécutés et
  verts sur 132 enregistrés, trois désactivés. Son `SHA256SUMS` référence
  quatre journaux `logs/*.log` absents du commit `3595725a` ; un clone
  ne peut donc pas rejouer entièrement `sha256sum -c` sur ce reçu. Ce
  n'est ni une qualification u18 R2 ni une preuve v9.

## Alerte immédiate de preuve au développeur

Les reprises u18 `release_r2` et `sanitize_r2` ont respectivement 136/136
et 131/131 CTests exécutés verts, mais leurs
[`COMPLETION.json`](../../morsehgp3D_v8/receipts/u18_resume_20260922/release_r2/COMPLETION.json)
disent **failed**. Le lecteur
[`run_u18_resume_checks.py`](../../morsehgp3D_v8/bench/run_u18_resume_checks.py)
cherche `<skipped>` alors que le XML CTest porte `status="disabled"`.
Corriger le lecteur et prendre une **capture R3 neuve** ; ne pas renommer R2
PASS ni inférer une croissance globale de la sonde mono-arête prévue.

## Avis architecturaux à instruire

1. Réduire **avant l'arête** la masse de produits A×B survivants, sans
   préparation O(|A|²+|B|²), et mesurer les covers + aval inclus. L'auditeur
   indépendant propose une cascade testée sur rectangles LiDAR 1 mm : h
   commun préfiltré par exclusion, décision singleton réutilisée, puis
   h_a/h_b disjoints sur gros produits. Elle accélère le **filtrage**
   ×2,55–×4,46 sur un échantillon apparié, mais conserve les mêmes arêtes et
   ne supprime pas d'atlas. Voir le [rapport publié](https://github.com/Ludwig-H/E-HGP/blob/12294241/morsehgp3D_v8/audits/SYNTHESE_PRIORITES_LIDAR_20260922.md).
   L'[artefact Actions désormais rapatrié](https://github.com/Ludwig-H/E-HGP/blob/c399808e/morsehgp3D_v9/receipts/actions_artifact_lidar_rectangles_20260922/README.md)
   couvre six configurations de filtre échantillonné, confirme les paires
   survivantes identiques et ne chronomètre ni atlas, ni census, ni FULL.
2. Traiter q3/q4 **ensemble sur le plan des centres d'une arête**, tout en
   conservant leurs seuils et propriétaires indépendants : comparer
   cellules adaptatives peu profondes, extraction d'arrangement de droites
   et bornes de blocs sur les segments de graines. Aucun de ces trois
   schémas n'est encore un algorithme v9 prouvé/qualifié.
   Le complément A de `e8ce8f33` établit un certificat local exact de
   blocs de graines **avant** la partition Z de l'atlas q4 ; sa couverture
   de toutes les arêtes/graines/cellules et son coût net restent ouverts.
   Le [contre-audit pré-atlas B](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)
   détaille le coût déjà payé du cover/domaine et le risque d'invalider
   une part des 153 M rejets q3 par l'atlas partagé ; `NoSeed` reste un état
   distinct, sans certificat Z implicite.
   La [contrelecture du certificat par capsule](../../morsehgp3D_v9/audits/CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md)
   démontre un rejet exact de rectangle, mais il est **dominé à gardes
   et boîtes identiques même par la recherche rectangulaire native v8**.
   Sa variante AABB est mathématiquement inopérante. Ne pas le porter
   comme nouvelle priorité : sur la ligne 1 mm qui active `RectanglePair`,
   après h terminal complet, il doit produire zéro rejet supplémentaire.
   Seul un raccourci de coût ou un rejet plus
   tôt dans le front pourrait le justifier ; le vrai verrou reste les
   complétions discrètes après h.
3. Exploiter un fragment **exact** d'atlas pour finir le census q3 sans
   reparcourir tout Z ; ne jamais confondre ce fragment avec un certificat
   q4 profond à K−2 ou un terminal saturé à K−1.
4. Construire tôt le catalogue par clé exacte, les intérieurs/coquilles et
   les vrais parents FULL, en comparant aux oracles Γ et à la v7 par boules.
   Pour les grandes coquilles, la
   [note sur le quotient sphérique](../../morsehgp3D_v9/audits/PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
   prouve qu'une représentation en `O(u²)` régions peut déterminer les
   composantes locales **sans** table de `2^u` masques. Elle n'est pas
   implémentée ; construire les régions, leurs signatures et `q_min`
   exactement reste à payer et à qualifier.
   La [contrelecture B avec oracle exact](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md)
   confirme les composantes, représentants et contributions sur sept
   petites coquilles, mais `q_min` et son support témoin restent une route
   distincte ; `BallData`, masques u16 et parents FULL bloquent encore les
   coquilles de plus de 12 sites.
  Le complément A `8054540c` factorise le calcul de `q_min=3` pour la
  grille u18 et publie un [oracle local de plans](../../morsehgp3D_v9/audits/check_qmin_planes_u18_20260922.py)
  (quatre fixtures et 755 sous-coquilles). Il ne teste pas encore les
  composantes du quotient ni la tour FULL.
  La [contre-lecture B des sommets q4 peu profonds](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_Q4_SHALLOW_20260922.md)
  confirme une borne locale `O(Km)` de **sortie**, mais le schéma direct
  de sélection paie encore `O(sKm)` pour `s` droites de graines parmi
  `m` droites du cover. Son oracle exact inclut un faux sommet q4 qui
  passe même le disque propriétaire : `centre∈conv(coquille)` reste
  obligatoire. Les algorithmes classiques des niveaux de droites ne
  sont pas transférés aux demi-plans orientés mixtes avec dégénérescences.
5. Après réduction du travail, répartir les cellules/graines d'une arête
   lourde entre CPU/GPU avec tableaux compacts résidents, intervalles sûrs
   et repli exact. Juger le temps de **toute** la tour, pas un kernel.
   Le [contre-audit des unités parallèles](../../morsehgp3D_v9/audits/CONTRE_AUDIT_B_PARALLELISME_Q34_FULL_20260922.md)
   localise l'arête et l'atlas comme tâches actuellement indivisibles, puis
   les barrières de niveau/K dans FULL ; le code publié est CXX CPU seulement.

Dans la ligne 1 mm actuelle, 26,23 M fragments sont fabriqués et leurs
frontières d'entrée représentent 6,00 G slots réservés cumulés, soit au
moins 48,0 Go de capacité logique demandée, **pas un pic RAM**. La sonde
était en mode digest sans records conservés : aucun de ces nombres ne
chronomètre le futur catalogue. Séparer cellules, fragments, slots
alloués/libérés, clés uniques et sorties FULL dans la prochaine mesure.

## Protocole de prochaine preuve

Faire des séries appariées sur trames entières de **plusieurs séquences**,
RAW et sans sol avec le même masque figé, grille 1 mm principale et float32
secondaire dans des reçus séparés,
K5/K10 et s8/s10/s12 ; les sept coupes spatiales servent uniquement au
diagnostic de croissance. Publier travail de front, paires, covers,
graines, atlas, census, tri, catalogue, parents, sortie, temps CPU/mur,
mémoire/VRAM et taux de repli exact. Une sortie FULL explicite peut avoir
Ω(n²) objets dans des familles adverses : viser le sous-quadratique sur
les régimes LiDAR et l'optimalité sensible à la sortie, sans promettre
une borne universelle impossible.
