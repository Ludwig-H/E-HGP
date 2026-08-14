# Note de Claude — réception de `Q4SeedAxisTopR4`, et le blocage G4

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=journal_de_tranche`,
`public_status=not_claimed`.

Cette note rend compte de la tranche demandée par
[`AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md`](AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md).
Elle ne certifie rien : la réception appartient à l'auditeur.

## 1. Ce que j'ai vérifié avant de coder

L'identité algébrique qui fonde le noyau est
`|W|^2 = D G (G + (E-F)^2)`. Je l'ai développée terme à terme avant
l'implémentation, parce qu'elle porte tout le reste : elle rend
`J_f = {2 tau^2 <= T2}` **exactement** égal à la borne de Jung `R^2 <= 3D/8`.
`T2` n'est donc pas un seuil choisi, c'est Jung écrit en entiers.

J'en tire un corollaire qui n'était pas énoncé et qui rend une branche
inatteignable : **un `Q4Seed3` owner strictement aigu a toujours `T2 > 0`**. En
effet `T2 > 0` équivaut à `sin^2 C > 2/3` où `C` est l'angle opposé à l'arête
owner ; celle-ci étant maximale, `C` est le plus grand angle du triangle, donc
`C >= 60` degrés, et l'acuité donne `C < 90`, d'où `sin^2 C >= 3/4 > 2/3`. La
campagne le confirme : `morts_T2 = 0` sur toutes les familles. La branche reste
implémentée et exercée sur un préfixe obtus, jamais supprimée.

## 2. Une erreur que je me suis infligée, et la fixture qui la tue

`J_f` est **fermé**. Le tétraèdre **régulier** sature Jung à l'égalité, donc la
racine de son apex tombe exactement sur le bout du segment. Ma première
classification traitait ce bout comme ouvert et perdait l'apex. Ce n'est pas un
cas d'école : c'est la configuration la plus symétrique qui soit.

La fixture `--fixture-jung-tendu` la grave, avec les deux orientations —
`(100,100,100)`, `(100,110,110)`, `(110,100,110)`, `(110,110,100)` — plus un
témoin `(98,100,104)`, à distance carrée `75` du circumcentre `(105,105,105)`,
donc **cosphérique du même bout** et du côté négatif du plan. Il est un sortant
dont la racine sature le bout droit, jamais un permanent, et jamais un
intérieur. Deux mutants n'existent qu'ici : `a8-bouts-ouverts` et
`a8-permanence-large`.

## 3. Les trois P0 de l'auditeur, et ce qui les ferme

**Identités.** `Selection` transporte les vrais `PointId` des permanents et du
shell persistant ; `census_replay` rend les listes triées `I_B` et `U_B`, avec
range-report du groupe d'égalité complet de la racine. Le juge n'est plus un
booléen mais `insphere_j` à **trois** classes — `<0` intérieur, `==0` shell,
`>0` extérieur. Mesure : `identites_fausses = 0` sur `5867` apex à `uniform`,
`n=60`.

**Mort par gaps.** Elle est désormais **décidée par le noyau**, pas déduite du
vide par l'oracle. J'ai implémenté le prédicat tronqué de l'auditeur,
`p + min(k, N_in) + min(k, N_out) >= r4`, évalué au bout gauche, à chaque racine
retenue et juste après chacune. Le verdict `MORT_GAP` est typé et porte son
minorant. Confrontation à la profondeur minimale exhaustive sur `J_f` :
`gaps_faux = 0` sur `16 133` préfixes à `uniform`, dont `3 512` morts par gaps.

**Exact-once et provenance.** Nouveau mode `--exact-once` : `Lane4` construit
ses deux `Q4Seed3`, applique la provenance primaire — plus petit vrai `PointId`
aigu — et le multiensemble global est comparé au brute force `C(n,4)`. Résultat
`manque = 0`, `doublon = 0`, `surplus = 0` sur `uniform`, `eight_clusters` et
`terrain` à `n=50`.

## 4. Le chiffre qui prouve l'indépendance des lanes

`--exact-once` publie `seeds_rang_q3_mort` : le nombre de préfixes générateurs
dont la miniboule **propre** porte au moins `smax-2` intérieurs — donc morts
pour la lane q3 — et qui produisent pourtant un q4 pertinent.

| famille | `n` | q4 attendus | `seeds_rang_q3_mort` |
|---|---:|---:|---:|
| `uniform` | 50 | 2 333 | 27 |
| `eight_clusters` | 50 | 1 846 | 43 |
| `terrain` | 50 | 210 | 1 |

L'indépendance de q2, q3 et q4 n'est donc plus une convention d'architecture :
c'est un compteur, avec son plancher `--min-q3-morts`. Une implémentation qui
sourcerait q4 depuis les q3 **retenus** perdrait ces supports, et la porte
`mhgp3v_q4axis_plancher_independance` refuse une campagne où le compteur reste
nul — le vert par vacuité est fermé des deux côtés.

Mesure complémentaire, obtenue par une énumération par ancre d'arête diamétrale
et confrontée au brute force `C(n,4)` : exiger que l'ancre soit une paire q2
**retenue** perd `1,22 / 1,23 / 1,74 %` des q3 et `1,05 / 1,16 / 1,41 %` des q4 à
`n = 60 / 100 / 140` sur `uniform`.

## 5. Portes

Trente-six CTests `mhgp3v_q4axis_*`, `36/36` en `29,98 s`. Chaque porte à regex
est doublée d'une porte à code. Neuf mutants, neuf tués, chacun par une porte
nommée ; deux ne vivent qu'à la frontière de `J_f` et seul le tétraèdre régulier
les mord ; `a8-shell-compte-interieur` n'existe que sous coplanarité massive et
seule une famille `scanline` le mord. Deux campagnes tournent à `smax=7` et
`smax=14` : l'ABI paramétrique n'est plus testée qu'à onze.

## 6. Le blocage G4, et il n'est pas dans mon code

La session `session_axis_top8_g4.sh` a démarré la VM puis **échoué fermé** : la
garde post-démarrage de `start_and_verify.sh` attend
`terminationTimestamp` sur la cible et il n'est jamais apparu. La cible a été
arrêtée et certifiée `TERMINATED`; le transcript est conservé dans
`receipts/axis_top8_g4_20260814/`.

Diagnostic : **l'API GCE n'expose plus ce champ**. Un `describe` complet de la
cible ne contient aucune clé comportant `termin` au niveau instance ; le
coupe-circuit lui-même est bien présent et correct dans `scheduling` —
`maxRunDuration=5400s`, `instanceTerminationAction=STOP`,
`provisioningModel=SPOT` — avec `lastStartTimestamp` lisible.

La garde pré-démarrage du même script accepte déjà ce cas et imprime
« `terminationTimestamp` non exposé; échéance calculée certifiée ». La garde
**post**-démarrage, elle, l'exige strictement et échoue. Ce déséquilibre bloque
toute session G4 du dépôt, pas seulement la mienne.

Je n'ai pas modifié `start_and_verify.sh` : c'est un script de sécurité, et
aligner sa garde post-démarrage sur sa garde pré-démarrage — certifier
l'échéance par `lastStartTimestamp + maxRunDuration` quand le champ est absent —
est une décision qui appartient à l'utilisateur et à l'auditeur, pas à une
réparation de fin de tranche. C'est le prochain point bloquant à trancher.

## 7. Ce que j'ai réparé dans ma propre recette

Le contre-audit
[`AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md`](AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md)
relève un P0 de sécurité réel : quand `GENERATION` était vide, ma branche de
secours appelait l'arrêt **non versionné**, qui peut viser une session
concurrente. La recette renommée `session_q4seed_axis_topr4_g4.sh` porte
maintenant trois états — `START_ATTEMPTED=0` n'arrête rien ; génération connue
arrête cette génération et elle seule ; génération inconnue **bloque** en
signalant projet, zone, nom et commande de contrôle, sans jamais appeler l'arrêt
non versionné. Le transcript est copié après la décision finale, plus avant.

Sa matrice est aussi devenue une **rampe budgétée** : soixante-seize runs
séquentiels ne tenaient dans aucune enveloppe. Chaque palier mesure son temps et
n'ouvre le suivant que si le coût extrapolé en `n^5` tient dans quatre
cinquièmes du budget restant.

## 8. Ce qui reste ouvert

1. la garde `terminationTimestamp` (section 6) — bloquant pour toute session ;
2. les caps de groupes d'égalité : une fixture à soixante-cinq IDs égaux manque,
   et les mutants `drop_equal_id`, `drop_persistent_shell` et
   `continue_after_overflow` ne sont pas écrits ;
3. le parse CLI strict : `atoll` accepte encore suffixes et débordements, le
   domaine u16 de `coord` n'est pas prévalidé, et les options ne sont pas
   filtrées par mode ;
4. `RelevantGP` : un groupe d'égalité non trivial doit rendre
   `unsupported_degeneracy`, pas un succès ; aujourd'hui il est seulement compté
   par `degeneres` ;
5. la portée physique : la sélection reste quadratique par préfixe, et la
   généralisation WSPD avec recherche best-first sur Morton n'est pas écrite ;
6. le contrat `50 000` points reste entièrement ouvert.

GCP : session démarrée puis arrêtée, cible `ehgp-blackwell-spot` certifiée
`TERMINATED` en zone `europe-west4-a`. Aucune mesure n'en a été tirée.
