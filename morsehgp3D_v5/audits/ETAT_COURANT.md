# État courant audité de MorseHGP3D v5 — 29 août 2026

- **HEAD documentaire relu :** `ac43ab1a`, publié sur `main` et
  `origin/main`. Le dernier delta fonctionnel du HEAD reste `2d052921` ; les
  commits `70a62be3` et `2d052921` ajoutent un harnais local et deux notes de
  Claude, sans pinner le raccord d'enveloppe.
- **Dernier pin fonctionnel reçu :** `72090f79`. Le chemin produit de
  l'enveloppe q3/q4, ses portes et les corrections du harnais restent dans un
  worktree concurrent non commité. Ils sont jugés ci-dessous comme snapshot,
  jamais attribués au HEAD.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Frontière :** aucun résultat GPU, aucune promotion de registre, aucun
  claim d'exactitude publique ou de passage à 10–30 M. La v4 reste un
  différentiel borné, jamais une implémentation ni une preuve héritée.

## Verdict utile à Claude

Le raccord d'enveloppe est bon dans son principe et dans son placement CPU :
il conserve le cover historique, compacte paresseusement au premier scan et
réemploie le tampon du counting sort. L'appariement exercé est solide. Il doit
maintenant être épinglé avec ses portes, pas encore optimisé ni mesuré.

La base continue d'éviter la mosaïque de Delaunay d'ordre supérieur. Le filtre
réduit les sites de cœur/profondeur ; il ne retire ni visites de handles, ni
ancres, ni pire exposant q4. Le verrou d'échelle global reste donc ouvert.

La correction de cap de l'utilisateur est reçue : **q2 n'est pas le problème
architectural à traiter**. La WSPD binaire partitionne correctement les paires ;
l'explosion naît lorsque q3/q4 développent chaque produit vivant `A x B` en
ancres, puis les tiers ou les couples de carriers. L'arrangement shallow par
ancre ne ferme que le second facteur. Il doit devenir le terminal d'une source
WSPD q3/q4 fibrée, pas se substituer à cette généralisation.

Le contre-audit des notes de Claude a eu un effet concret : la formule q4 est
requalifiée comme sur-ensemble de Jung, le seuil de coût ancien est retiré et
la fusion prématurée dans la collecte des handles est abandonnée. Ces décisions
sont intégrées à la question active ; les deux notes redondantes sont retirées
du tip.

## Enveloppe q3/q4 reçue mathématiquement

- Avec `d=b-a`, `D2=|d|2`, `w=2z-a-b`, `S=|w|2-D2` et
  `Xi=|d×w|2`, q3 emploie le prédicat fermé exact
  `S <= 0 || 3*S*S <= 4*Xi` sous ses préconditions d'ancre aiguë.
- q4 emploie `S <= 0 || S*S <= 2*Xi`, sur-ensemble sûr de Jung pour les
  tétraèdres strictement bien centrés émis par la lane. Il reste intersecté
  avec le cover historique coefficient 3.
- La lentille fermée de l'ancre est incluse dans l'enveloppe q3, elle-même
  incluse dans Jung q4. Seeds et complétions historiques sont donc préservés.
- Les produits sont formés en `i128`; les frontières restent fermées. Le
  `Q_min` par distance aux intervalles est seulement un minimum continu sûr,
  pas le minimum exact du réseau u16 à parité fixée.

La dérivation, les fixtures et la réponse V49–V52 consolidée vivent dans
`QUESTION_CLAUDE_EXPOSANTS_PAR_REGIME_20260828.md`.

## Réorientation WSPD q3/q4

Le contrat actif transmis à Claude est désormais :

- garder `PairWspdBlock(A,B)` comme tape q2 et ownership des ancres ;
- ajouter `Q3FiberTask(A,B,C)` et `Q4FiberTask(A,B,C,D)`, avec carriers
  paresseux, owner total, reçus de rang et continuation fail-open ;
- tuer un bloc seulement par un certificat universel exact ; une ambiguïté ou
  une capacité atteinte conserve le parent ;
- traiter q4 par lignes et niveaux peu profonds dans le plan médiateur, jamais
  par matérialisation préalable de `C x D` ni par dépendance aux verdicts q3 ;
- conserver séparément le ledger de paires, la provenance des supports q3/q4
  et les occurrences de proposition.

Le no-go des blocs ternaires symétriques fortement séparés reste valide, mais
sa portée est étroite. Une WSSD standard n'apporte pas l'exact-once ni le rang ;
une variante possédée peut en revanche devenir une source exacte après preuve
de bijection, transitions disjointes et oracle exhaustif. En attendant, la
frontière canonique `(N_i,r_i)` reste l'autorité q3/q4 et la route fibrée un
accélérateur à recertifier.

Le premier incrément demandé n'est pas un reroutage produit : une primitive
`q34_fiber` pour une ancre exacte, oracle exhaustif contre shallow, compteurs
de lignes/intersections/visites/scratch, puis producteur counter-only q3 et q4
avec center-cover de bloc. Les seuils de fenêtre à `smax=11` sont neuf
intérieurs pour tuer q3 et huit pour tuer q4. La note active détaille ABI,
ledgers, fixtures et mutants.

Claude a répondu au pin `ac43ab1a` avec deux filtres q3 par groupes. Le lemme
du tiers aigu est reçu après ajout de l'owner `EdgeKey`; l'optimalité du cœur
est limitée à la boule concentrique d'une ancre ponctuelle. L'escalier
d'histogramme et les rejets de handles passent d'abord en counter-only : ils
ne sont ni gratuits, ni une preuve que seules deux boucles restent. Le rejet
non aigu sûr emploie la forme couplée `hmin_boxes(A,B,C) >= 0`, dans une vue de
seeds séparée qui ne retire jamais ces points du cover témoin. La réponse
V53--V56 est fusionnée dans la note active ; la question séparée est retirée
du tip après migration, son commit restant dans l'historique.

## Réception du snapshot d'enveloppe

### Fermé dans le worktree observé

- build Release complet avec `-Wall -Wextra -Wpedantic -Werror` ;
- registre `80/80/80`, Python requis et détection des portes CMake
  multiligne ;
- appariement OFF/ON sur six familles : ordre brut à un fil, catalogue RLE,
  digests, événements avec niveaux, `batch_levels` et cardinalités par K ;
- routes de prétest cover/requête, compteurs séparés et compaction q3/q4 non
  vacante ;
- fixtures strictes non axiales, frontière `i128`, point Jung q4 extérieur à
  q3 et oracle indépendant par produit vectoriel ;
- chemins batched normaux, tout hôte, mixtes et surdimensionnés q3/q4 ;
- réemploi de `cover_tmp`, remapping stable de la lentille q4, garde u32 avant
  matérialisation, autorité unique de `pretest_query_min_points`, parsing CLI
  exact et sonde compilée comme cible produit.

### Dents avant pin et mesure

1. **Pinner le delta complet.** Source, fixtures, CMake et statut doivent
   entrer dans le même commit cohérent, puis être reconstruits et rejoués sur
   ce pin.
2. **Rendre « oversized » causal.** Les exécutions auditées empruntent la route
   (`3657` ancres q3, `2961` q4), mais `expect-route=device` n'exige pas
   `anchors_oversized > 0`. Ajouter un plancher explicite.
3. **Déclarer les capacités d'override.** Une option imprimée active ne peut
   être ignorée silencieusement par un exécuteur externe ; propager ou refuser
   la combinaison.
4. **Finir le harnais de reçu.** Le correctif après `70a62be3` pinne désormais
   le protocole, refuse l'écrasement, force les vrais digests et compare les
   bras. Il doit encore échouer sur tout code de run non nul, conserver un
   statut terminal après interruption, décrire honnêtement une seule
   répétition et exposer des bras séparés q3/q4 avant d'annoncer
   `none/q3/q4/both`.
5. **Réparer le budget de la porte post-séparation.** Dans la campagne à deux
   workers, `mhgp5_postsep_refine_mutant_h1` expire à `300,10 s` alors que la
   porte nominale jumelle finit en `302,74 s`. Le rejeu isolé est vert en
   `153,94 s` avec le code 4 attendu : le mutant est bien tué, mais le timeout
   de 300 s ne supporte pas la concurrence de la campagne canonique.

Le filtre reste OFF par défaut. Aucun tableau de mur antérieur au refactor ne
sert de reçu. Mesurer ensuite `none/q3/q4/both` exige d'abord des commutateurs
internes par lane, car l'API courante ne possède qu'un booléen global.

## Autres coutures actives

### G0 — confinement du pool

1. Incrémenter `submitted_` seulement après admission réussie dans la file.
2. Garantir un `exception_ptr` fatal non nul sans allocation dans le fallback.
3. Remplacer les scénarios `sleep_for` par des barrières causales.
4. Relier une exception CUDA typée à `close_fatal` avant toute nouvelle prise
   de lot ; `submit_and_wait` seul ne poisonne pas le pool.

La fermeture hôte explicite est utile. Le confinement général d'une erreur
device n'est pas reçu.

### Fold vivant L2

- borner `x` avant `av[x]`, puis `fid` avant `slot_of_fid`, et parcourir toute
  la table pour détecter une entrée stale ;
- ajouter un mutant de partition à cardinalité conservée et une porte de
  capacité causalement autonome ;
- graver les deltas et `batch_levels` littéraux, niveau compris ;
- ajouter seulement `born_at/died_at` et `batch_levels` au modèle de capacité,
  les autres postes étant déjà comptés ;
- garder le bras sans rejeu comme ablation et rendre le miroir avec rejeu
  strict sur T5.

Le contre-exemple T5 et la borne de wire sont migrés dans `../docs/ECHELLE.md`.
À 10 M, le wire brut FIRST/LAST extrapolé vaut déjà environ 1,60 To tous K ;
l'ancienne ligne 620 Go est retirée.

### Grille et G1

La grille de cellules n'a plus que six coutures documentaires et
d'environnement, listées dans
`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`. Ne pas rouvrir son noyau.

Pour G1, conserver les bornes d'indices, la distinction géométrie absente/vide,
les mutants SoA réellement exécutés, un `PointId` q4 au-delà du bit 31, le
contexte géométrique partagé et une réservation exclusive du wire actif. Le
protocole est condensé dans `QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md` et
`../docs/GPU.md`. Aucune nouvelle matrice G4 avant fermeture locale de G0/G1.

## Validation indépendante du snapshot

- configuration canonique et build Release : succès ;
- campagne ciblée enveloppe/CLI/mutants : `26/26` en `47,10 s` de mur ;
- registre direct : `80` mutants déclarés, `80` injectés, `80` gardés ;
- campagne complète label `gate` : `250/251` en `790,97 s`, seul le timeout
  post-séparation décrit ci-dessus ; rejeu isolé vert en `153,94 s` ;
- script de reçu : syntaxe Bash valide, contrats fonctionnels encore ouverts ;
- contrôles documentaires et diff final à rejouer après consolidation.

## Ordre recommandé

1. Fermer la non-vacuité oversized, le timeout et le harnais, puis pinner le
   raccord d'enveloppe avec ses tests ; ne plus l'étendre avant réception.
2. Refaire build et campagne `gate` sur ce pin.
3. Ouvrir le primitive borné `q34_fiber`, puis le producteur counter-only de
   tâches q3/q4 ; comparer exhaustive et shallow avant tout reroutage produit.
4. Raccorder le center-cover au niveau des blocs et prouver provenance,
   exact-once et continuations avant de remplacer les boucles `A x B`.
5. Fermer ensuite G0/G1, fold vivant et grille selon leur ordre local ; aucun
   de ces chantiers ne doit masquer les compteurs de la nouvelle source.

GCP non utilisé.
