# Note en vol à Claude — plafonds v6 réellement pré-allocation

Date : 1er septembre 2026.

Coupe observée : worktree non committé au-dessus de `534289f2`. Cette note
n'est pas un verdict sur un commit et sera absorbée puis retirée après le
checkpoint propre.

Cadre :

- `phase=exploration_v6_hors_registre`
- `backend=cpu_reference`
- `profile=quantized_u16_input_only`
- `mode=audit_independant_math_and_architecture`
- `public_status=not_claimed`

## Progrès utiles

La direction est bonne : plafonds centralisés, statut
`resource_exhausted`, invalidation transactionnelle, option de budget et
fixtures nominale/mutante. Le worktree compile. Les probes directs
`--caps-refus` et `--caps-refus --inject=caps-drop-emission` rendent
respectivement 0 et 4.

Ces verts prouvent le statut observable et la non-altération du petit témoin.
Ils ne prouvent pas encore le claim plus fort « refus avant l'allocation qu'il
protège ».

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
   l'OOM qu'elle annonce. Employer une borne conservative avant le préfiltre,
   ou une passe de comptage, puis garder la vérification exacte avant
   `BallData`.

5. La CLI construit la famille avant d'entrer dans `run_pipeline`. La garde
   `n <= 2^30-1` y arrive donc trop tard. Refuser `--n` avant
   `make_family_input` et couvrir aussi l'API de famille :
   `(n + 7) / 8` déborde encore en `int` près de `INT_MAX`. Le budget
   mémoire doit également couvrir ou borner l'entrée si son nom reste
   générique.

6. La fixture crée `callbacks` sans exiger zéro et n'observe pas
   `on_fold_phase`. Ajouter ces assertions. Séparer ensuite causalement les
   refus génération, tri, préfiltre/census et fold ; « census ou fold » ne
   prouve aucune garde précise. Le mutant actuel saute le refus exact mais
   laisse `cap_stop` actif : il prouve le verdict final sur ce témoin, pas
   l'instant pré-allocation.

7. Retirer le diff dans `morsehgp3D_v5/src/cloud/families.hpp`. La demande
   utilisateur cible exclusivement la v6 ; la v5 n'est ici qu'une référence
   historique. Si une correction v5 devenait indispensable à une porte
   appariée, elle demanderait une autorisation distincte et une justification
   explicite.

## Porte de livraison raisonnable

Un checkpoint recevable n'a pas besoin de résoudre toute l'échelle. Il doit :

- appeler honnêtement `2^32-1` une limite structurelle ;
- démontrer au moins un cap mémoire utile avant chaque allocation visée ;
- tuer un mutant de moment de garde, pas seulement de statut final ;
- préserver l'objet sous budget et l'absence de callbacks sur chaque refus ;
- rester entièrement dans `morsehgp3D_v6/`.

Le GO G4 `d98f4729` n'est pas révoqué, mais le pin refuse correctement le
worktree normatif sale avec le code 2. Aucun lancement n'est donc possible
depuis cet état ; ne demander un nouveau re-pin qu'après un commit v6 propre.

GCP non utilisé par cette revue.
