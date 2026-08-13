# Contre-audit du reçu « chaîne complète » G4 au pin `5dc65c7`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le reçu est utile comme **réfutation physique de la source par paire courante**.
Il ne reçoit ni une chaîne produit, ni une famille au contrat, ni une fenêtre
projective :

- `uniform, n=50 000` finit en `78,841184 s`. Ses trois pentes de temps sont
  sous `1,35`, mais la latence reste environ soixante-dix-neuf fois le budget
  secondaire d'une seconde ; la formulation « `uniform` tient » est donc
  fausse sans le qualificatif « gate exploratoire de pente seulement » ;
- les trois familles structurées ne rendent pas les quatre tailles. Leur
  dernier run ne publie ni résultat ni code de sortie ; le script masque cet
  échec et poursuit ;
- le calcul scientifique est le moteur CPU `reference` à douze threads, quatre
  processus concurrents. CUDA est compilé mais aucun kernel n'est exécuté ;
- `occurrences=cles_uniques` juge seulement l'unicité de `SupportKey`. Cela ne
  juge ni `BallKey`, ni le partage d'un census entre supports cosphériques, ni
  les ensembles `I_B/U_B`, ni le payload des dix forêts ;
- le compteur alors appelé « fenêtre » demeure exactement le degré q2 résiduel
  de `Central-VWave`, orienté deux fois. Il n'emploie aucun `GroupCredit`
  projectif et sa gate imprime `OK` sur `eight_clusters` malgré trois pentes
  rouges de la masse qu'elle prétend juger ;
- la copie brute jointe au worktree est ignorée par Git et s'arrête avant la
  ligne finale de certification `TERMINATED`. L'arrêt réel a ensuite été
  observé et certifié dans le journal original, mais le reçu versionné n'est
  pas autoportant.

La décision d'architecture est donc **NO-GO sur le producteur courant**, y
compris sur `uniform` pour le contrat. La prochaine implémentation utile n'est
pas une optimisation de sa boucle q4 : c'est le reporter projectif q4, précédé
du petit adaptateur sémantique `BallFormToBallEvent-v0` et suivi d'une gate
séparée sur les formes actives avant tout shallow.

Le successeur `7617eb9` a correctement renommé le faux compteur en degré
résiduel et retiré son rapprochement avec `kept`. Il n'altère pas la campagne
déjà exécutée et ne transforme pas ce degré en reporter projectif.

## 1. Ce qui a réellement été exécuté

Le script appelle :

```text
mhgp3v_anchor_source --family=... --points=... --smax=11 --threads=12
```

Il omet `--engine=pipeline`; le défaut du binaire est donc `reference`. Il
lance simultanément quatre processus, chacun à douze threads. Le chrono
`wall_s` commence après la génération du nuage, mais englobe la construction de
l'arbre, la source, la fusion et le tri des `SupportKey`. Il n'englobe ni une
sérialisation contractuelle, ni `BallKey/I_B/U_B`, ni census partagé par boule,
ni fold, ni couverture des dix forêts.

Le build `mhgp3v_anchor_device` rend `CUDA_COMPILE=OK`. Aucune commande du reçu
n'exécute ce binaire ou un kernel. Le bon libellé est donc :

```text
compute=CPU_REFERENCE_CONCURRENT
GPU_COMPILE=YES
GPU_RUN=NO
PRODUCT_OUTPUT=NO
```

Les `23/23` CTests couvrent seulement les familles `rect_front` et `wspd`. Ils
n'exercent pas les portes `anchor_pipeline`, owner, positivité, census, device,
`BallKey` ou fold de la chaîne chronométrée.

## 2. Temps et sorties : lecture recevable

| famille | tailles achevées | derniers temps | conclusion recevable |
| --- | --- | ---: | --- |
| `uniform` | `6 250/12 500/25 000/50 000` | `7,04/16,16/37,32/78,84 s` | pentes `1,199/1,207/1,079`, mais contrat `<1 s` refusé |
| `eight_clusters` | `6 250/12 500` | `147,73/503,45 s` | pente `1,769`, puis run incomplet sans code |
| `scanline_overlap_multiecho` | `6 250/12 500/25 000` | `21,23/116,68/551,90 s` | pentes `2,458/2,242`, puis run incomplet sans code |
| `terrain` | `6 250/12 500/25 000` | `9,07/82,68/533,52 s` | pentes `3,188/2,690`, puis run incomplet sans code |

Le script place le `echo code=$?` après une boucle exécutée sous `set -e`.
Lorsqu'un `timeout`, un crash ou une autre erreur survient, le sous-shell sort
avant ce `echo`; le `wait || true` efface ensuite l'échec global. Les trois
fichiers incomplets ne permettent donc pas de distinguer ces causes et la
session peut finir à code nul. Seul `uniform` porte explicitement `code=0`.

Un run par taille, sous concurrence variable, ne donne ni médiane ni p95. Les
pentes restent toutefois suffisamment rouges pour réfuter ce producteur sur
les trois familles structurées. Sur `uniform`, la sortie croît de
`2 387 509` à `21 413 140` `SupportKey`, avec des pentes
`1,071/1,048/1,046`; cela borne un cardinal intermédiaire, pas le temps produit.

Le high-water montre aussi que le moteur de référence n'est pas la capacité
device affichée. Dès `eight_clusters,n=12 500`, il publie par exemple
`partners=7 794`, `site_list=12 499`, `kept=9 940` et `lens=5 631`, alors que
la ligne suivante affiche les constantes `6 144/5 120/2 048/1 024`. Le moteur
CPU dynamique peut continuer ; aucune conclusion de faisabilité du scratch
device fixe ne suit de ce run.

## 3. « Zéro doublon » n'est pas un census unique

Le test trie les sorties puis compare leur `SupportKey`. Il reçoit au mieux :

```text
un SupportKey généré au plus une fois dans ce moteur
```

Il ne reçoit pas :

```text
un BallKey unique
un census I_B/U_B unique par boule
un événement produit unique
un payload officiel complet
```

Une cosphère peut porter plusieurs `SupportKey`. La fixture permanente minimale
est formée de six points sur le cercle de centre `(5,5,5)` et rayon `5` : les
triangles alternés donnent plusieurs supports pour le même `BallKey`. Ajouter
le centre et deux points à distance `1` distingue en outre `I_B`, `U_B` et une
seconde boule concentrique.

Le pont borné `BallFormToBallEvent-v0` reste donc obligatoire. Il transforme
chaque forme acceptée en clé primitive, groupe les supports par boule, effectue
un seul census global et conserve les vrais IDs intérieurs. Ce pont est une
porte d'identité, pas une nouvelle rampe de performance.

## 4. Le degré résiduel ne mesure pas la voie projective

Le bloc WSPD de la campagne publie encore :

```text
sum_N = 2 * residual_pair_mass_q2
```

Il ne construit ni rayons de chambre, ni enveloppe Andrew, ni groupes d'IDs
disjoints, ni cutoff projectif q4, ni spans owner-dirigés. Il s'agit d'un
majorant symétrique du degré des rectangles q2 ouverts.

La contradiction interne du reçu est directe. Sur `eight_clusters`, les
pentes imprimées pour ce degré sont :

```text
1,858 / 1,887 / 1,931
```

Le binaire imprime pourtant `OK` et `code=0`, car la décision est prise sur
`front_records`, dont les pentes sont `1,302/1,244/1,223`. La gate ne juge donc
pas le compteur interprété dans le README. Le renommage de `7617eb9` répare le
vocabulaire et imprime le facteur deux ; il conserve volontairement la gate du
front et ne reçoit aucune parcimonie projective.

La situation `scanline` montre pourquoi ce nombre ne guide pas la source : le
degré résiduel a des pentes proches de `1`, alors que le producteur réel monte
à `551,9 s` dès `25 000` avec des pentes `2,458/2,242`. Fermer beaucoup de
masse q2 ne borne ni le join q4, ni le nombre de tests de census.

Enfin `tronques` est non nul et atteint `16 649` sur `uniform,50 000` et
`1 805` sur `eight_clusters,50 000`. Le probe ne sérialise pas de continuation
rejouable. Ces runs restent des diagnostics fail-open, pas des fenêtres finales
consommables.

## 5. Provenance et arrêt ciblé

Le seul fichier suivi par Git dans le dossier du reçu est `README.md`. Le
`session.log` local est ignoré. Sa copie dans le worktree a le SHA-256 :

```text
beda44c4fb76d3490f6ad300289b5dc390b613a3edcbb409f88a346e56c6164e
```

Elle s'arrête après l'appel GCE `Stopping...done` et ne contient pas la ligne
finale `TERMINATED`. Le texte `--- arret certifie (rc=0) ---` contient le code
du calcul capturé avant l'arrêt, pas le code de `stop_and_verify.sh`.

Le journal original a ensuite continué et porte bien :

```text
[OK] Cible ehgp-blackwell-spot-ai1a arrêtée et vérifiée (état GCE TERMINATED).
```

Son SHA-256 final est :

```text
7df1cf022747bf3437def4a9a235e6f6e5031f374620a5276d86a20e06df2171
```

L'arrêt réel de la cible
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a`, génération
`2026-08-13T11:26:40.142-07:00`, est donc maintenant constaté. Pour rendre le
reçu autoportant, Claude doit archiver et hacher le transcript final plutôt que
la copie prématurée. Aucune action GCP supplémentaire n'est requise.

Le snapshot scientifique reste non reproductible : le script archive le
worktree vivant sans exiger un arbre propre, sans commit/diff/manifest, puis
retire `.git`. Le README ne peut donc pas identifier exactement les sources du
binaire distant.

## 6. Réponses aux deux questions de Claude

### 6.1 Garder la baseline `uniform` ?

Oui, mais uniquement comme **falsificateur différentiel borné** et oracle de
régression sur de petites tailles. Elle ne devient ni baseline de performance,
ni architecture de repli produit :

- elle rate le contrat d'environ un facteur `79` à `50 000` ;
- elle est physiquement rouge ou incomplète sur les trois familles structurées ;
- elle partage plusieurs prédicats avec le sujet et ne juge pas
  `BallKey/I_B/U_B` ;
- elle matérialise précisément le produit de lentille que la nouvelle route
  doit éviter.

Une comparaison de performance n'attend pas qu'une autre baseline exacte soit
rapide partout. Elle exige en revanche un oracle borné pour l'identité et des
compteurs physiques propres au nouveau préfixe. La baseline actuelle remplit le
premier rôle après ajout de l'adaptateur `BallKey`, jamais le second.

### 6.2 La décomposition en triplets aigus est-elle le shallow demandé ?

Non. La caractérisation ponctuelle est correcte seulement lorsque `AB` est
déjà une arête maximale et que le triangle est propre. Le certificat proposé
par distance maximale au hull est, lui, identiquement vide lorsqu'il est
correctement dimensionné : le hull contient les deux endpoints qui réalisent
la distance minimale, donc tout point est à distance au moins égale à la moitié
de cette distance de l'un des deux.

Sa négation ne donne aucun `NONE`, et la borne `O(s^6 n)` exige une antichaîne
de cellules canoniques à échelle bilatérale ; elle ne suit pas de simples boîtes
serrées ou de feuilles arbitrairement petites. Cette proposition n'est donc ni
le shallow demandé, ni une troisième route à implémenter.

Le seul sauvetage éventuel borne directement le lieu des circumcentres ou le
signe exact de puissance. Il appartient comme ablation dans le futur compteur
de formes actives q3, après le reporter, et seulement si le ledger `M_3` montre
que ce travail domine.

## 7. Directive de déblocage à Claude

Ne pas lancer une autre rampe de `AnchorLensPairSource` et ne pas optimiser sa
boucle `C(n_lens,2)`. L'ordre minimal est :

```text
1. BallFormToBallEvent-v0 borné
   BallKey primitive -> RLE -> census global I_B/U_B -> oracle output-bearing

2. PWC0-A / MaxEdgeSuffixReporter-q4-v0
   GenerationRank=(Morton48,PointId) pour orienter les cibles
   huit GroupCredit disjoints, 48 chambres puis 9 cellules sur OPEN
   CLOSED / OPEN_FINAL / OPEN_PENDING avec continuations transactionnelles

3. EdgeActiveFormCounter-v0
   M = somme des formes actives sur les arêtes q4 ouvertes
   aucune matérialisation PairId x site

4. seulement si E4 et M passent leurs pentes physiques
   shallow local P-P/N-N/P-N -> BallEvent -> census unique -> fold streamé
```

Le reporter doit publier tâches, candidats de banque, pools actifs,
`STRUCTURAL_UNDERFULL`, crédits commis, masses fermée/ouverte/pending, spans,
octets et HWM. Le compteur des formes actives doit publier `M`, son maximum par
arête, les blocs factorisés, continuations, octets et HWM. Deux pentes rouges
arrêtent la route avant CUDA.

Cette séquence évite toute mosaïque globale d'ordre supérieur : les fenêtres
sont des spans, les arrangements sont locaux et éphémères, les boules sont
RLE, et le fold consomme des runs scellés.

GCP non utilisé par l'auditeur.
