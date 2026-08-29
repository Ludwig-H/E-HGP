# Note active à Claude — lane résidente sur device

- **Base relue :** décisions G0/G1 et session 13 jusqu'au pin documentaire
  `a3c15d84` ; état fonctionnel courant tenu dans `ETAT_COURANT.md`.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.
- **Autorités :** architecture et métriques dans `../docs/GPU.md`, portes dans
  `../docs/PLAN_DE_TESTS.md`, sorties brutes et hashes dans `../receipts/`.

## Verdict de séquencement

Conserver le cœur déjà livré : pool borné, géométrie indexée, wires SoA/index
séparés et lanes par lots. Les prochaines corrections sont des coutures
locales de sûreté et de réception ; aucune ne justifie de redessiner G0/G1.

Le chemin device reste expérimental et non autoritaire. Une égalité de digest
sur quelques familles reçoit le chemin exercé, jamais un claim de rendement,
de confinement général d'une panne CUDA ou de résidence complète.

## Décisions V17–V30 encore applicables

- **V17 — ordre du cover.** Une représentation device peut conserver la
  sous-séquence stable du cover historique. Juger directement l'ordre brut à
  un fil, puis l'objet post-RLE et les compteurs sémantiques à plusieurs fils.
  Une frontière de lots ou un histogramme de politique peut changer sans
  changer l'objet ; ne pas les inclure implicitement dans la même notion
  d'égalité.
- **V18 — conversion vers binaire64.** La conversion DI128 doit avoir un
  domaine prouvé, un seul arrondi avec bit collant et un repli exact autour des
  frontières. Une concordance moyenne CPU/GPU ne remplace pas cette preuve.
- **V19 — minimiseur séparable.** Borner chaque coordonnée sur son intervalle
  fermé, évaluer les extrémités et le voisinage entier du sommet rationnel ;
  refuser toute formule qui choisit un seul arrondi sans comparer les voisins.
- **V20 — régimes S/M/L.** Les seuils ne sont qu'une politique de routage.
  Publier masse, scratch, fraction hôte et motifs de repli pour chaque régime ;
  tout dépassement échoue ouvert vers le corps CPU reçu.
- **V21 — piles.** Une pile fixe n'est permise qu'avec une borne de profondeur
  démontrée et une garde défensive. Le profil Morton 48 bits ne dispense pas
  d'un refus explicite si le contrat est dépassé.
- **V22 — ordre brut.** L'ordre des `BallCandidate` fait partie du diagnostic
  de conformité, mais les digests canoniques restent l'autorité publique. Un
  changement volontaire d'ordre doit être versionné, pas caché par le RLE.
- **V23 — famille adverse.** `scanline_overlap_multiecho` reste une
  contre-fixture utile du profil V1 ; garder les `PointId` non monotones et les
  bornes u16 dans les portes device.
- **V24 — profondeur.** Une borne radiale seule ne décide pas la profondeur
  d'une boule décentrée. Toute coupe device doit dériver du prédicat exact ou
  rester fail-open ; mesurer séparément les scans évités.
- **V25 — contrat du premier kernel.** Les sorties intermédiaires doivent être
  validées avant que l'hôte les transforme : tailles, offsets, monotonie,
  compteurs et correspondance exacte seed/ancre.
- **V26 — racines entières.** Déclarer le domaine de la racine, son arrondi et
  la preuve de borne. Aucun `sqrt(double) ± 1` ne devient une racine entière
  certifiée.
- **V27 — mutants device.** Le registre global choisit un index stable ; le
  device reçoit un masque borné. Une constante ignorée par le kernel ne prouve
  rien : chaque mutant doit modifier le point d'injection réellement lancé et
  mourir par une porte non vacante.
- **V28 — RLE externe.** La seule nécessité est la réconciliation globale des
  occurrences d'une même clé. La fusion externe triée est le premier chemin
  sûr ; un seau Morton est une localité, jamais une autorité d'unicité.
- **V29 — coût.** Les mesures par seed et le reçu `scanline` ne se
  contredisent pas. Rapporter séparément mur de lane, sommes d'exécuteur,
  H2D/D2H, kernels, attente, setup et travail hôte ; aucune somme concurrente
  ne se soustrait au mur.
- **V30 — occupation.** Distinguer occupation théorique, occupation observée
  et limiteur réel — registres, mémoire partagée, warps actifs, spills,
  files et fraction routée hôte. Un pourcentage SM isolé n'est pas un verdict.

Le diagnostic CPU et le raffinement post-séparation ont été migrés vers
`../docs/MESURES_ECHELLE.md` et `../tests/postsep_refine_gate.cpp`. Ils ne sont
plus maintenus dans cette question.

## Dents G0 avant tout claim de confinement

1. Compter une soumission seulement après admission réussie dans la file. Une
   exception de `deque::push_back` ne doit pas laisser le ledger en avance.
2. Garantir un `exception_ptr` fatal non nul sans allocation dans le chemin de
   fermeture. Construire une `runtime_error` dans le fallback peut lui-même
   lever avant `make_exception_ptr`.
3. Rendre les scénarios de fermeture causaux, avec barrières ou hooks, sans
   `sleep_for` servant d'oracle d'ordonnancement.
4. Relier l'exception CUDA typée à `close_fatal` avant que le worker puisse
   prendre un autre lot. Une exception ordinaire propagée par
   `submit_and_wait` ne confine pas l'exécuteur.

Ces quatre points précèdent tout claim « une erreur CUDA empoisonne le pool ».
Ils ne bloquent pas les mesures CPU ni les portes hôte des wires.

## Dents G1 et lanes par lots

- L'autorité unique de `pretest_query_min_points` est reçue dans le worktree
  CPU courant : `BatchLimits` ne la duplique plus. La requalifier sur le pin
  final avec les portes cover/requête.
- Toute nouvelle option de génération doit être propagée ou refusée par
  capacité déclarée. L'impression `active=1` ne doit jamais couvrir un
  override qui a ignoré l'option.
- Garder un `PointId` q4 au-delà du bit 31 et les deux mutants de retombée SoA
  dans la campagne device réelle ; le validateur doit refuser une CLI qui
  n'imprime pas la branche attendue.
- Le contexte de géométrie doit être partagé par q3/q4 et précéder les pools
  dans l'ordre de destruction. Compter séparément son upload, son mur de setup
  et ses octets.
- Réserver exclusivement les buffers du wire actif. Conserver SoA et index en
  même temps prouve la compatibilité, mais annule le gain de résidence mémoire.
- Pour toute compaction, comparer les tableaux intermédiaires sur petits lots :
  ordre des seeds, indices de lentille, offsets, `skip_a/skip_b`, candidats et
  compteurs. Les portes CPU tout hôte, mixtes et nommées surdimensionnées sont
  présentes dans le worktree. Ajouter au dernier cas un plancher explicite
  `anchors_oversized > 0` : aujourd'hui la fixture emprunte bien la route, mais
  `expect-route=device` pourrait rester verte si elle devenait vacante.

## CPU multi-worker

Avant une nouvelle infrastructure, exploiter le plan de chunks pur et le
scratch par worker déjà identifiés dans `../docs/GPU.md` :

- cpuset déclaré, `fold_inflight=1`, digest OFF puis ON, au moins trois
  répétitions et ordre miroir ;
- 1/8/16/24 cœurs physiques avant SMT, puis 32/48 CPU logiques ;
- `team_launches`, `threads_created`, `atomic_tickets`, `worker_busy_ns` et
  migrations publiés ;
- un scratch `NodeRef` réutilisé par worker doit être vidé avant chaque
  parcours, y compris après sortie précoce ou overflow.

`--threads` seul ne constitue jamais un protocole de scaling CPU.

## Passage de relais minimal

1. Fermer G0 en quatre petits correctifs et leurs mutants causaux.
2. Pinner puis requalifier les autorités d'options déjà unifiées dans le
   worktree ; refuser toute option non déclarée par un override.
3. Rejouer les portes hôte nominales, sous seuil et mixtes, puis rendre la
   porte surdimensionnée causalement non vacante.
4. Faire ensuite une réception CUDA minimale des deux wires et des mutants
   manquants, sur la cible gardée seulement.
5. Mesurer le setup partagé et la double représentation avant G2.
6. Ouvrir G2 uniquement si les retours q4 intermédiaires dominent encore.

Il n'est pas utile de rejouer une matrice SCALE complète pour fermer une dent
de sûreté locale. Chaque reçu doit nommer pin, worktree, commande, codes,
digests, compteurs, hashes et statut terminal.
