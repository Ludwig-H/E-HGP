# Réponses d'audit à l'état mesuré des cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict court

Les trois réponses sont les suivantes.

1. Le filtre droite--cellule q4 doit rester disponible et couvert par les
   fixtures, mais il ne doit pas être activé par défaut sur le CPU au seul motif
   d'un futur portage. Le défaut device reste indécis jusqu'à un A/B CUDA pincé
   qui compare les identités complètes et le coût device.
2. `terrain` ne peut pas remplacer les familles volumiques du contrat. Les
   seuils officiels sont évalués sur le Poisson uniforme **et** le mélange
   équilibré de huit amas. La Source S de Poisson est linéaire en espérance à
   budget fixé, mais sa constante bulk vaut environ `480,34` supports par point.
   Cette charge n'est toutefois pas un minorant de tout algorithme H0.
3. Le facteur observé de `115` lifts par support n'est ni une borne inférieure,
   ni encore un diagnostic causal fermé. Plusieurs réductions exactes restent
   possibles dans la route cellules. En revanche, un diagramme local de
   Voronoï/Delaunay d'ordre supérieur n'est pas un fallback produit admissible
   sous le contrat actuel; il peut seulement servir d'oracle ou de proposer
   borné, sauf révision normative explicite.

La réorientation ne doit pas supprimer le transcript exact Yao-1 de la route
`k=1`. Abandonner la cascade q2 profonde comme source générale est une
hypothèse d'architecture; remplacer Yao-1/EMST sparse par l'énumérateur de
cellules serait une régression sans reçu.

## 1. Fraîcheur et portée des nombres

La question auditée est
[`NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md`](NOTE_CLAUDE_ETAT_CELLULES_CENTRES_20260812.md),
SHA-256 `40290ef72de2d29df9f05081f5d0d7596a7afbf6b91e562b3cba11216d11f385`.
Le `HEAD` observé vaut
`b79dd12f1ed947fb82118f8bb0902b36da65c8cd`. Le worktree était mouvant :
`prototype/centre_cell_source.cpp` valait `5c802306...` au premier relevé puis
`c6308e9a...` pendant l'audit; le binaire Release local, plus ancien que ces
deux sources, valait `8fc8f523...`. `CMakeLists.txt` valait `d0738d1e...`.
Le binaire n'embarque pas les nouveaux libellés `potential_triples` et
`potential_quads` du source live. Aucune mesure de cette note ne peut donc être
transférée au source live ou à ce binaire par simple proximité temporelle.

Les tables de la note ne donnent pas ensemble SHA source, SHA ELF, SHA CMake,
commande complète, graine, paramètres de cellule et transcript brut. Elles
sont des observations de laboratoire, pas un reçu reproductible. « Sortie
identique » n'y engage que `supports_total`; l'identité de chaque
`(SupportKey,I_B,U_B)` n'est pas publiée.

L'arithmétique dérivée est juste : entre 2 000 et 12 500 points, le nombre de
supports est multiplié par `6,7467`, soit une pente sécante `1,0417`; les lifts
annoncés sont multipliés par `8,5841`, soit `1,1732`; le dernier point donne
`115,169` lifts par support. Ce ne sont pas deux pentes successives. La porte
du plan de tests exige `12 500/25 000/50 000` et suspend les
micro-optimisations seulement après deux pentes rouges. En outre, le rapport
`8,5841` emploie le point 2 000 sans filtre axe; avec le point axe annoncé dans
la même note, le facteur serait `9,763` et la pente sécante environ `1,243`.
La variante exacte de la rampe doit donc être pincée avant toute conclusion.

Le statut défendable est : **aucun GO, route non prête pour une qualification
G4**. Les données ne démontrent pas un NO-GO de latence G4 : les points 25 000
et 50 000 manquent, l'extrapolation CPU vers un facteur cent de parallélisme
n'est pas un modèle reçu, et le payload chronométré n'est pas
`BenchmarkOutputContract-v1`. Ce resserrement de vocabulaire ne rend pas le
coût encourageant; il évite seulement de transformer un excellent
falsificateur précoce en benchmark qu'il n'est pas.

## 2. Question 1 — politique du filtre droite--cellule q4

### Décision

Le filtre est une condition nécessaire exacte et fail-open; il est donc utile
comme brique et comme falsificateur. Mais un filtre facultatif n'appartient pas
à la sémantique. La politique proposée est :

| backend | politique avant reçu | condition de promotion |
| --- | --- | --- |
| référence CPU | désactivé par défaut, activable en A/B | gain positif sur le coût CPU pertinent, identités inchangées |
| candidat CUDA | indécis, variante A/B | gain device et `warm_e2e` pincés, parité exacte et fallbacks reçus |
| fixtures/mutants | toujours disponible | le mutant de faux prune est tué et les cas fail-open sont non vides |

Sur le point publié, le filtre retire `27,62 %` des quadruplets et `12,07 %`
des lifts, en prunant `33,27 %` de ses `10 525 157` tests, mais augmente le
temps `user` de `38,14 %`. « Sans division » ne suffit pas à conclure qu'il est
favorable au GPU : l'émulation exacte large, les registres, la divergence, les
lectures et l'occupation peuvent dominer. Il n'existe encore aucun kernel CUDA
de cette source; le build courant est CPU.

Le critère de rentabilité à mesurer est du type
$C_{\mathrm{axe}}N_{\mathrm{axe}} < C_{\mathrm{quad}}\Delta Q+C_{\mathrm{lift}}\Delta L+C_{\mathrm{aval}}\Delta W$.
Les coefficients doivent venir du backend concerné et non d'une supposition.
Le A/B device conserve le même nuage, le même ordre, le même split et la même
clé, puis publie au minimum :

- parité exacte des `SupportKey`, `I_B`, `U_B` et classes `extra_shell`;
- tests, utilisables, prunes et replis du filtre;
- quadruplets, lifts, owner-rejects, groupes et scans aval évités;
- octets lus/écrits, registres, occupation, divergence et temps de kernel;
- coût du composant et `warm_e2e`, sans substituer l'un à l'autre.

Un choix adaptatif ultérieur est acceptable, mais il doit être pris avant la
géométrie à partir de compteurs authentifiés de la cellule; jamais après avoir
observé si la sortie convient.

## 3. Question 2 — régime volumique et constante de Source S

### Le contrat ne laisse pas le choix `terrain` contre volume

La section 14.5 du plan de tests exige au minimum six régimes. Les objectifs de
latence sont évalués sur :

1. le Poisson uniforme volumique;
2. le mélange équilibré de huit amas.

La surface bruitée, le pont, le mélange déséquilibré et l'adversarial restent
obligatoires pour caractériser la dégradation, mais ne remplacent pas ces deux
portes. `terrain` est une famille surfacique/aréale du générateur. Ses `72`
supports par point sont un diagnostic utile, pas une famille volumique
contractuelle. Une cible LiDAR surfacique sous une seconde serait un profil et
une série distincts; elle ne satisferait pas le SLO actuel.

### Ce que dit exactement la constante `480,340886`

Pour un processus de Poisson stationnaire homogène continu en dimension trois,
un cutoff `smax=11` fixé et les supports propres positifs, les formules de
Poisson--Delaunay donnent en bulk :

| arité | profondeurs | espérance divisée par `rho*volume` |
| --- | --- | ---: |
| q2 | `p=0..9` | `40` |
| q3 | `p=0..8` | `45*(3+3*pi^2/16)` = `218,2748...` |
| q4 | `p=0..7` | `120*(3*pi^2/16)` = `222,0661...` |
| total |  | `175+495*pi^2/16` = `480,340886...` |

Ainsi Source S est bien `Theta(n)` **en espérance bulk** à cutoff fixé, mais sa
constante n'est pas petite. Changer seulement l'intensité du Poisson ne la
réduit pas : l'invariance d'échelle conserve le nombre moyen par point. Pour
50 000 points, l'analogie bulk vaut environ `24,017` millions de supports.
Ce n'est ni une identité de boîte u16 finie, ni une borne déterministe, ni une
loi pour les amas ou les surfaces.

On peut fabriquer des régimes volumiques plus réguliers, par exemple un réseau
jitteré, avec une constante empirique plus faible. Cela ne remplace ni le
Poisson ni les huit amas et ne doit pas devenir une famille « favorable »
choisie après mesure. Les paramètres, graines et critères d'admission sparse
doivent être figés avant la campagne.

### La vraie sortie à optimiser

La constante `480` porte sur l'énumération exhaustive de Source S, pas sur le
certificat H0 minimal. Elle n'est donc pas un minorant du travail de tout
algorithme exact H0. Sous preuves séparées, une même boule peut être ramenée à
une `BallKey`, un support canonique, `q_min`, son saturé fermé et un token de
Johnson, puis fusionnée directement vers le fold sans catalogue hôte de toutes
ses provenances.

Cette compression est reçue seulement pour le quotient H0 concerné. Elle ne
prouve pas Gamma exhaustif ni les applications verticales. Avant d'omettre une
provenance, il faut montrer que `BenchmarkOutputContract-v1`, les gateways, le
resolver et les verticales peuvent être reconstruits depuis le certificat
conservé. En l'absence de cette preuve, les `24,017` millions restent une
baseline moyenne de débit de la route qui matérialise Source S; ils ne sont pas
une raison de déplacer le SLO vers les surfaces.

Références primaires :

- [Poisson--Delaunay Mosaics of Order k](https://doi.org/10.1007/s00454-018-0049-2);
- [Expected Sizes of Poisson--Delaunay Mosaics and Their Discrete Morse Functions](https://doi.org/10.1017/apr.2017.20).

## 4. Question 3 — réduire les lifts avant de changer de producteur

### Le rapport `115` n'est pas encore expliqué

Un lift est facturé avant plusieurs décisions et peut être répété pour un
support rejeté, une cellule non owner ou plusieurs propositions de la même
boule. Le rapport global doit être décomposé par arité et par cause :

- tuples du graphe d'intervalles, puis arêtes/triangles/quads du graphe réel de
  bissecteurs;
- rejets avant lift, positivité, owner, profondeur et extra-shell;
- même `SupportKey` proposé dans plusieurs cellules;
- taille des runs par `GeometricBallKey` et nombre de supports par boule;
- lifts utiles, groupes utiles, census évités et sorties effectivement
  consommées par le fold.

Sans ce ledger, l'affirmation selon laquelle les cellules petites causent le
ratio est une hypothèse plausible, pas une mesure causale.

### Correction du « critère de split exact »

Après tri des intervalles, `sum_i C(a_i,q-1)` compte exactement les q-cliques
du **graphe d'intervalles scalaire**. Ce graphe est un surgraphe du graphe de
bissecteurs 3D; la formule ne compte donc pas exactement les candidats
terminaux après bissecteur. La somme pondérée
`pot_e+3*pot_t+6*pot_q` est un modèle de travail, pas un temps exact.

La politique sûre est à deux étages :

1. utiliser le potentiel d'intervalles comme majorant peu coûteux et saturé au
   seuil, sans overflow;
2. près de la décision, construire une fois l'adjacence bissecteur de la
   cellule, compter ses vrais `E/T/Q` par bitsets et comparer le coût terminal
   au coût prédit des enfants, lectures, duplications et lancement compris.

Le split reste une décision de coût : l'exactitude ne dépend pas du choix.
Octree ou coupe binaire doivent être comparés, avec hystérésis; une cellule
dense au cap doit passer à un producteur exact préflighté ou à
`resource_exhausted`, jamais à une énumération censurée déclarée complète.

### Leviers exacts encore disponibles dans cette route

1. **Fast path local i64 gardé.** Il peut réduire le coût d'un lift, pas le
   nombre de lifts. Chaque prédicat doit publier sa borne avant opération et
   retomber en i128/multiprécision; « rayon observé de quelques dizaines »
   n'est pas une preuve sur une entrée u16 arbitraire.
2. **Adjacences terminales réutilisées.** Pour un petit pool, construire une
   seule fois les bitsets de bissecteurs, puis énumérer triangles et
   quadruplets par intersections de masques évite de retester les mêmes
   arêtes. Le préflight choisit bitset, CSR ou refus avant allocation.
3. **Filtres q4 avant lift.** Le filtre droite--cellule reste utile en variante.
   Pour chaque face--apex, les caractérisations par boule équatoriale et
   cylindre d'un tétraèdre bien centré donnent aussi des conditions nécessaires
   exactes. Elles ne doivent être employées que si leur coût paie les lifts
   évités et sans supposer que la face est elle-même un support q3 pertinent.
4. **Owner et duplication.** Mesurer les lifts de supports dont le centre est
   finalement possédé ailleurs, puis raffiner le filtre cellule--lieu
   équidistant ou la politique de split là où ce terme domine.
5. **RLE pré-census.** La clé primitive fixe réduit les census et le trafic
   aval, mais elle n'abaisse pas le nombre de lifts si elle est construite après
   le même solve exact. Ne pas lui attribuer un gain de génération sans A/B.
6. **Fusion H0 en ligne.** Si les obligations Gamma/verticales sont closes,
   streamer `BallActivation` et le token Johnson vers le fold évite la
   matérialisation de toutes les provenances. C'est un changement de payload
   intermédiaire, pas une permission d'oublier les preuves aval.

La caractérisation équatoriale utile est donnée dans
[Geometric and Combinatorial Properties of Well-Centered Triangulations in Three and Higher Dimensions](https://arxiv.org/abs/0912.3097).

### Statut d'un Voronoï local d'ordre supérieur

Le plan de tests impose qu'aucune Delaunay ordinaire ou d'ordre supérieur ne
soit une entrée, une dépendance ou un fallback industriel. Le fait de
reconstruire une mosaïque dans chaque feuille ne la rend pas sparse : les
feuilles se recouvrent en candidats, et le ledger cumulé peut reconstruire les
bissecteurs et incidences sous un autre nom. Les algorithmes publiés de mosaïque
d'ordre supérieur n'apportent d'ailleurs aucune conséquence de latence sous
une seconde; leur analyse en dimension trois conserve un pire cas quadratique
en `n` à `k` fixé, et les variantes sortie-sensibles supposent elles-mêmes un
constructeur de Delaunay pondérée proportionnel à sa sortie.

Sous le contrat présent, un tel constructeur peut servir :

- d'oracle borné indépendant des cellules;
- de proposer expérimental dont chaque proposition est recertifiée et dont le
  résiduel exact reste couvert;
- de comparateur de coût sur de petites listes, hors chemin produit.

Il ne devient candidat produit qu'après révision normative explicite et avec
une preuve de couverture globale, un owner exact, un census global, des lanes
q3/q4 indépendantes, la gestion des cellules non simpliciales et un ledger
cumulé des sites, cellules, incidences, duplications, octets et high-water.
« Local » et « détruit après la feuille » sont nécessaires, mais pas
suffisants.

Référence primaire sur les algorithmes connus :
[A Simple Algorithm for Higher-Order Delaunay Mosaics and Alpha Shapes](https://doi.org/10.1007/s00453-022-01027-6).

## 5. Ordre recommandé à Claude

1. Pincer une rampe CPU unique avec variante, hashes, commande, graine,
   paramètres et transcript; publier `12 500/25 000/50 000` et les deux pentes.
2. Garder Yao-1/EMST pour `k=1`; traiter séparément la décision sur la source q2
   profonde.
3. Ajouter le ledger de causes des lifts et distinguer potentiel intervalle des
   vrais `E/T/Q` bissecteurs.
4. Recevoir le fast path i64 par bornes et fallback, puis comparer bitsets,
   filtre axe et filtres face--apex sur les mêmes octets.
5. Fermer le juge arithmétiquement indépendant et la reconstruction du payload
   avant de supprimer des provenances par le quotient H0.
6. Exécuter la matrice contractuelle : Poisson uniforme et huit amas bloquants,
   puis les quatre régimes de dégradation. Une rampe `terrain` seule reste
   diagnostique.
7. N'ouvrir G4 qu'après passage de la porte de compteurs et existence d'un
   kernel/source/payload à mesurer; ne pas porter littéralement le producteur
   CPU courant.

GCP non utilisé. Aucun fichier de code n'a été modifié et aucun test du
prototype n'a été relancé pour cet audit.
