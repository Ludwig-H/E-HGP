# Contre-audit B du grand audit C : portée des 100 ms et des coûts

24 septembre 2026. Base :
[`AUDIT_C_GRAND_AUDIT_V9_20260924.md`](AUDIT_C_GRAND_AUDIT_V9_20260924.md),
commit `21b83463c`, reçus R20 publiés, code v26 `09885f163` et WIP
R21 `61cfba666` relus sans modifier le moteur. GCP non utilisé.

## Verdict

Le grand audit C décrit correctement **l'écart mesuré** : sur G4,
les trois trames 08 sans sol prennent 1,010–1,260 s pour la tour
K1..5, dont FULL seul 374–456 ms. Les 100 ms de **sortie explicite**
ne sont pas qualifiés, pas plus que la seconde sur toutes ces trames.
Les trames avec sol, s10/s12 sur le chemin GPU courant, plusieurs
séquences et le régime multimillionnaire ne le sont pas non plus.

La phrase « 100 ms infaisable avec les algorithmes connus » n'est en
revanche **pas démontrée** par les preuves du dépôt. Les mesures R20
bornent le code courant ; les colonnes « ingénierie » et « refonte »
sont des projections, non une borne inférieure sur toute méthode exacte.
Le constat étayé est : **aucune architecture implémentée ou projetée
avec preuves de coût ne satisfait aujourd'hui 100 ms**. Des étapes
intermédiaires à 1 s puis 250–500 ms sont utiles pour piloter la
recherche, mais ne remplacent pas le contrat demandé de 100 ms FULL K5
sans sol, ni celui de trames brutes entières.

La seule taille de la sortie n'établit pas l'impossibilité. R20
08/000000/K5 publie 1 541 750 nœuds, 1 541 745 parents et 897 776
contributions. L'ABI documentée des cinq tableaux de sortie implique
au moins `80·nodes + 8·parents + 80·contributions = 207 496 040`
octets, soit environ 198 Mio. Les écrire en 100 ms demanderait au
moins 2,075 Go/s **pour ces seuls tableaux** ; allocations, lectures,
clés et temporaires augmentent le besoin. Ce calcul n'est ni une
preuve de faisabilité matérielle ni une barrière >100 ms. Il montre
seulement que l'argument « 1,54 M nœuds donc impossible » serait invalide.

## Trois corrections de lecture des chronos

1. R20/08/000000/K5 mesure `q34=525,810 ms`. Les sous-chronos publiés
   `front+filter+certificate+lanes+edges+tail` totalisent
   `446,939 ms` ; la **différence arithmétique** est `78,871 ms`.
   Certains intervalles peuvent se recouvrir : ce n'est pas encore
   une durée exclusive par poste. Le grand audit l'appelle « 80 ms
   hors de tout chrono ». Elle est en réalité hors des **sous-chronos
   attribués**, mais incluse dans `q34_ms` puis `chain_total`. Des
   contrôles sériels de survivants et de lots sont des candidats à
   l'instrumentation avec intervalles exclusifs ; leur attribution
   exacte n'est pas encore publiée. Déplacer ces contrôles hors de
   `chain_total` ne constituerait pas un gain.
2. La descente des phases statiques par K est optimale **dans un modèle
   simplifié** où les durées de phase 0 et de phase A sont fixes, la
   phase 0 dispose d'une machine et les phases A de ressources
   indépendantes illimitées. Le vrai `run_orders_overlapped` fait
   concourir les runners A et la phase 0 sur les mêmes CPU/mémoires ;
   ses durées ne sont donc pas figées. R20/08/000000/K5 donne
   `prefix_static+A ≈254,9 ms` à K4 et `≈254,1 ms` à K3 ; leur
   concordance observée ne prouve pas une optimalité physique ni
   architecturale. Réduire la résolution A, la concurrence mémoire ou
   changer son lieu d'exécution peut déplacer la fenêtre, en plus de
   réduire la phase 0.
3. Les `lots_by_k[5]≈148 ms` sont un temps mesuré **sous le partage
   actuel des ressources**, pas un plancher invariant pour toute
   reconstruction parallèle. La phase A de l'ordre haut demeure un
   verrou important ; la présenter comme borne de 100 ms pour tout
   algorithme serait excessif. La sortie explicite doit rester dans
   `chain_total` pendant toute refonte.
4. « L'appareil n'occupe que 19–22 % du mur » est trop fort sans
   profil matériel. Sur R20/08/000000/K5, les trois `device_ms`
   valent `249,621/1 108,630 ms≈22,5 %`, mais ils comprennent appels,
   transferts et synchronisations ; les noyaux instrumentés valent
   `210,297 ms≈19,0 %`. Ce sont des **fractions du mur attribuées à
   des intervalles d'appels/noyaux**, non le taux d'occupation des SM
   ou de la bande passante. Pour optimiser la cohabitation CPU/GPU,
   il faut une trace des intervalles et, si l'occupation matérielle
   devient un argument, une mesure dédiée de l'appareil.

## Ce qui reste bien établi et ce qui ne l'est pas

- R20 passe 326 empreintes, 18/18 cas `complete_relative` et 12
  comparaisons GPU/moteur égales. R16–R20 retrouvent 90/90 épingles
  sur les **trois trames de la séquence 08 sans sol**. C'est un progrès
  réel, mais ni une preuve exhaustive des clés absentes ni une
  qualification multi-séquence.
- Les pentes de travail `core_sites` des panneaux 8k/16k/32k et
  secteurs physiques dépassent 2 sur certains doublements. Le temps
  CPU fini peut y rester sous 2, sans établir un algorithme globalement
  sous-quadratique dans les régimes d'intérêt. Un changement du cœur
  doit republier paires, formes chargées, `ΣF`, sorties et CPU/mur sur
  les deux axes de croissance, brut et sans sol.
- Le rapport mémoire d'une **seule** trame brute de 123 k sites ne
  qualifie pas les dizaines de millions. Les IDs de catalogue u32,
  les gros tableaux FULL et les scénarios de centaines de Gio restent
  des portes de capacité distinctes ; tester par paliers, sans
  extrapoler les ratios comme une loi de croissance.
- Les pistes de réduction du travail amont ne sont pas encore un facteur
  de gain utilisable : le [shadow des moments](moments_rectangle_shadow_20260924/README.md)
  est négatif sur les rectangles lourds du quart testé, même après deux
  divisions ; le nouveau lemme « cellules S3 échouées → graines q4 »
  évoqué par C n'a pas de preuve et de porte publiées ici. L'échec
  d'une borne S3 signifie d'abord **indécision**, pas témoin acquis :
  il faudrait un prédicat sur tous les centres admissibles de chaque
  graine q4, des témoins distincts et le même propriétaire. Ne créditer
  ni ces filtres ni une pente sous-quadratique avant mesure du coût
  total et comparaison exacte.
- La proposition de catalogue scellé de C est conditionnelle : la
  positivité exacte q3/q4, les arités et le lien clé/niveau/index
  doivent être établis dans la chaîne **avant** de sauter la passe 1.
  Cette passe vaut environ 18 ms à K5 sur R20/08/000000 ; le levier
  est utile mais ne résout pas isolément 100 ms.
  Un sceau qui *déplace* `balls` doit également préserver son accès
  jusqu'au `catalogue_digest` et au `keep_catalogue` éventuel ; le
  digest et la sortie publique ne doivent pas changer. L'échantillon
  1/64 proposé n'est qu'un détecteur de fautes systématiques, jamais
  une preuve d'exactitude d'un catalogue scellé.

## Portes pratiques avant la prochaine session G4

1. Corriger le lecteur v26 : le mur externe doit majorer aussi
   `device_session.context_ms + reserve_ms`, séquentiels entre lecture
   et chaîne, avec mutation causale. Le [préflight B](CONTRE_AUDIT_B_PREFLIGHT_R21_V26_20260924.md)
   reproduit aujourd'hui un reçu impossible accepté.
2. Figer le plan R21 à 30 cas et rejouer les autotests normal/`-O` :
   des attendus WIP restent à 18 cas. Ajouter les épingles CPU brutes
   que C calcule en ce moment ; tant qu'elles ne sont pas publiées,
   les jumeaux bruts n'offrent qu'une égalité relative.
3. Déclarer « chaud par processus » pour R21 : chaque cas ouvre un
   nouveau processus et préchauffe avant la chaîne. Un flux GPU
   réellement persistant multi-trames exige une sonde distincte ;
   publier séparément démarrage froid, latence HGP chaude, segmentation
   et coût total depuis la trame brute.
4. Ajouter un bras T2 direct `Kmax=5`, puis comparer les sorties
   structurelles complètes à s8/10/12 et plusieurs séquences. Le
   protocole Kmax+2 et les juges stratifiés proposés par C renforcent
   les invariants, mais ne sont pas des oracles exhaustifs.

Le développement v9 n'a reçu aucun nouveau commit moteur ni reçu G4
depuis R20 lors de cette lecture. Les cinq VM G4 observées sont
`TERMINATED` ; le calcul local d'épingles brutes de C est encore actif.
