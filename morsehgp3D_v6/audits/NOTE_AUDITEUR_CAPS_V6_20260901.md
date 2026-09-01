# Note en vol à Claude — plafonds v6 réellement pré-allocation

Date : 1er septembre 2026.

Coupe observée : worktree non committé au-dessus de `7c4d5e0a`, empreinte du
diff normatif `bdd1b6d932f8a133b1392431d7188ccafe5b62d3fb3d3db8f6cc54416185014c`.
Cette note n'est pas un verdict sur un commit et sera absorbée puis retirée
après le checkpoint propre.

Cadre :

- `phase=exploration_v6_hors_registre`
- `backend=cpu_reference`
- `profile=quantized_u16_input_only`
- `mode=audit_independant_math_and_architecture`
- `public_status=not_claimed`

## Progrès utiles

La direction est bonne : plafonds centralisés, statut
`resource_exhausted`, invalidation transactionnelle, option de budget et
fixtures nominale/mutante. Une configuration Release isolée compile les
cibles `mhgp6_selftest` et `mhgp6`. Les deux CTests `^mhgp6_caps_` passent
2/2 en 47,78 s ; le probe CLI avec un budget de 1 octet rend le code 2 et le
refus du tri attendu.

Ces verts prouvent le statut observable et la non-altération du petit témoin.
Ils ne prouvent pas encore le claim plus fort « refus avant l'allocation qu'il
protège ».

## Rejeu de la réponse en vol

Claude a utilement retiré le diff v5, sécurisé les produits de budget, placé
la borne conservative census avant le préfiltre, prévalidé `--n` dans la CLI
et renommé le budget en « partiel de tampons ». Après correction de la vague
initiale, les trois CTests `^mhgp6_caps_` passent 3/3 en 67,94 s. La première
version de la nouvelle porte avait justement échoué sur
`pic de vague <= cap`, ce qui a permis de garder aussi la vague initiale avant
son `reserve`.

Le cap candidat n'est toutefois pas encore un cap de matérialisation. Deux
probes Debug arrêtés au site exact du refus ont observé, pour un cap de 1 000 :

- `uniform(2000, 200, 3)` : 1 009 candidats dans les quatre shards ;
- `eight_clusters(2000, 400, 3)` : 1 033 candidats dans les quatre shards.

La porte ne publie pas cet observable et reste donc verte. Le commentaire
borne l'overshoot par `T × 4096 + une ancre par ouvrier`, mais q3/q4 ne
remontent l'arrêt que hors de leur helper : la boucle du rectangle peut encore
visiter d'autres ancres, chacune poussant avant d'observer `stopped`. Soit
réserver le quota avant chaque `push_back`, soit propager explicitement
l'abandon jusqu'au rectangle et exposer/tester le pic ainsi que le slack réel.

De même, la porte wave à 64 exerce maintenant la vague initiale ; elle ne
prouve pas que les shards `lnext`/`lout` sont bornés, car ceux-ci sont encore
matérialisés avant le calcul prospectif. Le mutant prouve l'instant de la
seconde insertion dans `next/out`, pas l'absence d'OOM dans les shards.

## Corrections matérielles avant livraison

1. Dans `alive_rectangles_fused`, les tailles de `next` et `out` sont
   contrôlées après `vector::insert`. L'allocation ou `bad_alloc` peut donc
   précéder le refus typé. Calculer les tailles prospectives avec addition
   contrôlée, refuser avant chaque insertion et donner aux sorties locales un
   plafond injectable pour une fixture causale.

2. Le compteur de candidats dit rapporter par tranches de 64 K, mais il n'est
   consulté qu'à la fin d'un rectangle. Un seul rectangle, plus les ouvriers
   déjà engagés, peut matérialiser une masse non bornée avant `cap_stop`.
   Réserver un quota avant les `push_back`, ou contrôler les émissions dans
   leurs boucles, avec une borne d'overshoot explicitement prouvée.

3. `kMaxRawCandidates = 2^32-1` est un plafond de type, pas un plafond RAM :
   une seule copie à environ 144 octets dépasse 575 Gio. Le G4 n'en offre
   qu'environ 180 Gio, `memory_budget_bytes` vaut zéro par défaut et ce budget
   ne réduit pas le cap transmis à la génération. Distinguer clairement cap
   structurel et cap de résidence, puis dériver `go.max_raw_candidates` du
   budget déclaré ou exposer un cap CLI sûr.

4. La garde dite « census » intervient après `prefilter_balls`, donc après
   la matérialisation des shards et de `surv`. Elle ne peut pas prévenir
   l'OOM qu'elle annonce. Elle sous-compte aussi le pic suivant : les shards
   `lb` de `BallData` coexistent avec le vecteur `balls` pendant leur fusion,
   alors que la formule ne compte qu'une population `BallData`. Employer une
   borne conservative avant le préfiltre, ou une passe de comptage, puis
   compter les coexistences exactes avant `BallData`.

5. La garde du fold ne compte que `ForestEvent × (inflight + 1)`. Elle omet
   notamment l'ordre de tri, les batches, les tableaux `ev_fid`/`ev_part`,
   les records de partitions, tables, pools, deltas et résultats qui
   coexistent dans `prepare_fold`/`reduce_fold`. Ce n'est donc pas encore un
   budget mémoire du fold. Soit fournir un majorant de phase vérifiable, soit
   renommer ces trois contrôles en budgets partiels de buffers et retirer la
   promesse anti-OOM globale.

   Le renommage actuel en budget partiel est la bonne résolution de portée.
   Il faut encore éviter d'appeler « conservative » une formule basée sur
   `cands.size()` après RLE : le vecteur garde alors sa capacité brute, souvent
   bien supérieure, et les shards `Survivor`/`BallData` peuvent avoir une
   capacité supérieure à leur taille. Compter les `capacity()` réellement
   résidentes ou libérer/réserver explicitement avant de publier un majorant.

6. La CLI construit la famille avant d'entrer dans `run_pipeline`. La garde
   `n <= 2^30-1` y arrive donc trop tard. Refuser `--n` avant
   `make_family_input` et couvrir aussi l'API de famille :
   `(n + 7) / 8` déborde encore en `int` près de `INT_MAX`. Le budget
   mémoire doit également couvrir ou borner l'entrée si son nom reste
   générique.

7. La fixture crée `callbacks` sans exiger zéro et n'observe pas
   `on_fold_phase`. Ajouter ces assertions. Séparer ensuite causalement les
   refus génération, tri, préfiltre/census et fold ; « census ou fold » ne
   prouve aucune garde précise. Le mutant actuel saute le refus exact mais
   laisse `cap_stop` actif : il prouve le verdict final sur ce témoin, pas
   l'instant pré-allocation.

8. Retirer le diff dans `morsehgp3D_v5/src/cloud/families.hpp` du checkpoint
   v6. La v5 est une source différentielle épinglée, pas une simple source
   historique : la modifier change l'entrée normative contrôlée par
   `v6_campaign_pin.sh` et demande une requalification v5 séparée. En outre,
   le patch actuel reste asymétrique : `(n + 7) / 8` déborde dans les deux
   lignées et seul le scanline v6 élargit `(n * 3) / 5`.

9. Les additions et produits de garde restent non contrôlés (`fetch_add`,
   somme exacte des shards, calculs d'octets). Employer des helpers par
   soustraction/division ou en `u128`, puis tester égalité, seuil moins un et
   overflow. Les caps wave/alive doivent devenir abaissables en test afin que
   leurs branches et leur instant pré-insertion soient réellement exercés.

## Porte de livraison raisonnable

Un checkpoint recevable n'a pas besoin de résoudre toute l'échelle. Il doit :

- appeler honnêtement `2^32-1` une limite structurelle ;
- démontrer au moins un cap mémoire utile avant chaque allocation visée ;
- tuer un mutant de moment de garde, pas seulement de statut final ;
- préserver l'objet sous budget et l'absence de callbacks sur chaque refus ;
- distinguer un budget résident complet d'une estimation partielle de buffer ;
- rester entièrement dans `morsehgp3D_v6/`.

Le GO G4 `d98f4729` n'est pas révoqué, mais le pin refuse correctement le
worktree normatif sale avec le code 2. Aucun lancement n'est donc possible
depuis cet état ; ne demander un nouveau re-pin qu'après un commit v6 propre.

GCP non utilisé par cette revue.
