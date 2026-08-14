# Audit constructif — axe q4 puis descente device

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

L'auditeur n'a utilisé aucune ressource GCP. Cet audit ne modifie aucun code et
n'autorise aucune structure de Delaunay. Il juge le raccord q4 de `2d8aa5f`,
puis la qualification host/device de `2c14313`.

Snapshot noyau lu :

```text
HEAD=2c14313d5848c46b1f0abcc1e910d26b862a88a4
q4seed_axis_topr4.hpp    d86f27d30e4b81834e3d711f7c24b9916879b85fc14e3407eaf8180b6d1aaacd
lane_source_scale.cpp   1b297b2168784fb9c35398cf311b06b9bc1acd9e4de7a3d98d3cba7be88665fb
axis_device_job.hpp     d2f6ef0b1d8cde835e039931046c716bb5911c4d862fee87448e05739e5ceb0a
axis_device_kernel.cu   1fbeb822e127484cefbc6054ebe43469f814b2184940d6358f166bfdf3eb5db5
axis_device_qual.cpp    994c0da9f9030638cb2ace9279f9954f12d701eee327d8ddedc7f62cad020547
CMakeLists.txt          547a720ae45c2a4e8fbc14f2faf4dafcdf87e0a48431c0cade09e961b3e6121f
```

Le `HEAD=11130cb1f2114d3569991e96606c49bd1d6cc853` ajoute ensuite uniquement la
recette `session_axis_cuda_g4.sh` ; les cinq fichiers du noyau ci-dessus sont
inchangés. Aucun reçu `axis_cuda_g4_20260815` n'existe au moment de cette
relecture. Cette recette est donc auditée statiquement ci-dessous, sans lancer
ni interroger GCP.

## 1. Avancée reçue comme diagnostic borné

Le pivot q4 est le bon. `select_axis_topr4` conserve la règle mathématique —
un root est retenu s'il a strictement moins de `k` sites strictement meilleurs,
ties complets — mais calcule maintenant le root d'ordre `k` dans un tableau de
taille bornée puis range-reporte son groupe. Le coût passe de `O(m^2)` à
`O(m*k)` pour un seed, avec `k<=r4`.

Le raccord `--axe` supprime effectivement la boucle
`for carrier, for apex`. Les comparaisons suivantes sont vertes :

- `50/50` CTests q4axis + lane-source au worktree pré-commit, en `76,40 s` ;
- quatre exécutions `--axe --verifie`, sur `uniform/eight_clusters` et
  `smax=11/6`, donnent exactement les mêmes comptes q4 que le brute force ;
- à `uniform,n=100,smax=11`, `q4=7069` des deux côtés, zéro paire de lentille ;
- à `uniform,n=100,smax=6`, `q4=771` des deux côtés ;
- à `n=6000,smax=6`, l'ancien et le nouveau chemin donnent tous deux `89796`
  q4, tandis que les propositions passent de `48 791 131` paires à `830 044`
  roots, soit un facteur proche de cinquante-neuf.

La campagne q4axis existante reste verte après la réécriture : `39/39` en
`38,25 s`. Une comparaison indépendante de la sélection `O(m*k)` à l'ancienne
référence `O(m^2)` sur `10 000` configurations rationnelles, avec ties, caps et
les dix modes, ne trouve aucun écart pour `r4>=2`. Les groupes d'égalité, les
bouts fermés, permanents, morts par gap et seuils paramétriques restent donc
exercés. Ce résultat répond constructivement au mur mesuré : le produit de
lentille n'est plus nécessaire à la complétude q4 ponctuelle.

Il ne reçoit pas encore les identités de la source J0 : le mode `--verifie`
compare des cardinaux q2/q4, pas les ensembles de `SupportKey`, owners,
`BallKey` et `I_B/U_B`; q3 n'a toujours aucun juge brute dans cette sonde.

## 2. Le déplacement du mur est lui-même le résultat utile

Le temps `n=6000,smax=6` reste environ `25 s` pour la boucle et `28 s` pour
l'axe malgré cinquante-neuf fois moins de propositions. Le nouveau chemin
parcourt encore les deux à cinq cents sites de `inner` pour chacun des
`2 956 531` seeds, puis rescane le même voisinage pour les candidats.

Le terme physique n'est donc plus `sum_e binom(m_e,2)`, mais

```text
sum_seed |B(m_e,D_e) inter P| + rescans de census.
```

C'est un déblocage, pas un échec : la prochaine primitive est maintenant
identifiée exactement. Optimiser les déterminants ou augmenter la fenêtre ne
peut pas enlever ce terme ; une descente hiérarchique le peut.

## 3. Port device : bonne baseline, pas encore la descente

`2c14313` compile la même fonction numérique avec `MHGP_HD`, met un thread par
seed et compare host/device champ par champ sur la sortie transportée. La cible
CPU sans CUDA construit et ses trois CTests passent en `1,48 s`. La discipline
« même fonction exacte des deux côtés » est appropriée pour détecter un écart
de compilation CUDA.

Mais le lot device est encore la **baseline plate** : `AxisJob` transporte en
CSR la liste complète des sites de chaque seed et le kernel les balaie tous.
Il ne contient ni Morton BVH, ni `Q_theta`, ni best-first, ni census de boule.
Le qualifier mesure le plafond de débit du mauvais terme ; il ne le retire pas.
Ce rôle est utile s'il est nommé `axis_flat_scan_baseline` et comparé au futur
`axis_bvh_wavefront`.

Le CTest host courant est volontairement tronqué sans le déclarer comme fate :

```text
points=600, seeds=20000, sites=1679128, cap=1, code=0
```

La construction du lot s'arrête au premier `max_seeds/max_sites` atteint, puis
la qualification réussit sur ce préfixe. C'est recevable pour un débit de
microkernel seulement avec `batch_complete=false` explicite ; ce n'est ni une
preuve de source ni une mesure de tous les seeds.

## 4. Portes précises manquantes au lot device

1. `SeedOut` ne transporte pas `n_perm`, `n_shell`, les IDs permanents ou le
   shell persistant. Il conserve au plus vingt-quatre IDs par côté alors que la
   sélection en autorise soixante-quatre, puis compare seulement ce préfixe et
   `deborde_sortie`. La phrase « chaque champ de chaque verdict » est donc trop
   forte.
2. Le batch builder du probe est un triple balayage host et duplique les mêmes
   IDs dans le CSR. À pleine échelle, ce coût et ces octets précèdent le kernel
   chronométré. Ils doivent être publiés, pas cachés derrière `kernel_ms`.
3. Le commentaire annonce le transfert séparément, mais le binaire ne publie
   ni allocations, ni H2D, ni D2H, ni total. Seul le kernel est chronométré.
4. `max_sites` et `dmax_espacements` ne sont pas validés strictement ; les
   offsets sont des `int`. Il faut un preflight d'octets et d'offsets avant les
   allocations.
5. Un thread porte une `Selection` avec 192 slots d'IDs et deux tableaux de
   64 `SitePower` au cours des passes. La compilation G4 doit publier registres,
   local memory par thread, occupancy et spills ; une parité verte ne prédit
   aucun débit.
6. Aucune compilation CUDA ou exécution GPU n'a encore été observée dans cet
   audit. Les trois CTests verts sont host-only.
7. Les préconditions d'IDs injectifs/disjoints de la sélection restent le P0
   antérieur ; déplacer l'appel sur device ne les impose pas.

Deux portes d'intégration sont aussi à fermer avant que `--axe` devienne un
producteur :

- `DEBORDEMENT` est actuellement agrégé avec les morts géométriques. Il faut un
  `switch` exhaustif : `MORT_*` ferme, `OUVERT` continue, `DEBORDEMENT` déclenche
  une continuation dynamique, un fallback exact ou un refus non nul. Il ne
  peut jamais incrémenter `seeds_morts`.
- l'API accepte un `r4` supérieur à la capacité de son tableau `seuil[64]`.
  Un appel `r4=65` déborde sous ASan. Le contrat device doit vérifier
  `1<=r4<=64` avant tout accès ; la borne CLI actuelle ne remplace pas la
  précondition de l'API partagée.

## 4.1 Premier gain immédiat : supprimer le rescan de census

Avant même le BVH, le prototype peut enlever le second balayage de `inner`.
Après `owner6`, positivité et primary, appeler
`census_replay(sel,iq,seed3,pw)` puis router son fate :

```text
EXACT                    -> consommer I_B/U_B
UNSUPPORTED_DEGENERACY   -> refus RelevantGP ou lane Plateau déclarée
PENDING_CAP              -> continuation/fallback exact
HORS_DOMAINE             -> apex profond ou appel invalide, aucune sortie
```

La sélection a déjà conservé tous les permanents, tous les extrêmes strictement
intérieurs et le groupe de root égal nécessaire à un apex shallow. Ce raccord
remplace les lignes de rescan par une reconstruction de taille bornée ; il ne
change ni la source ni l'index et constitue donc un jalon J1.5 facilement
falsifiable. La gate doit comparer le multiensemble
`SupportKey -> (owner,primary,I_B,U_B,multiplicite)` au legacy et au brute, pas
seulement `cand_q4`.

Ce gain suppose les IDs du seed distincts, les IDs témoins injectifs, les deux
ensembles disjoints et l'apex présent exactement une fois dans les extrêmes.
Tant que le type `Selection` ne garantit pas ces invariants, le caller doit les
préflighter et une violation vaut `HORS_DOMAINE`, non dégénérescence
géométrique.

## 5. La primitive J2 à écrire, sans réinventer un index de roots

Pour un cutoff rationnel `theta=p/q`, `q>0`, poser

```text
Q_theta(z) = q*A_z - p*B_z.
```

Cette forme est une quadratique convexe séparable et décrit l'intérieur de la
sphère de paramètre `theta`. Elle fournit un unique moteur
`RationalBallRange` :

1. **First.** Sur `B_z>0`, le root vérifie `rho_z<theta` exactement lorsque
   `Q_theta(z)<0`.
2. **Last.** Sur `B_z<0`, le root vérifie `rho_z>theta` exactement par la même
   inégalité `Q_theta(z)<0` ; seul le signe de `B` sélectionne la lane.
3. **Census.** Pour un apex, `Q_theta<0`, `=0`, `>0` donnent directement
   intérieur, shell, extérieur.

Sur une AABB, le minimum de `q*G*s_i^2-H_i*s_i` est obtenu par clamp axe par
axe. Une borne minimale positive ou nulle taille `OUT`. Le maximum de la
quadratique convexe est à un des huit coins ; un maximum strictement négatif
crédite `ALL`. Zéro descend. Une première passe entretient le heap de
`k<=r4`, une seconde range-reporte tout le groupe égal ; le census s'arrête au
huitième intérieur.

La largeur doit être fixée dès l'ABI. Pour le root de l'apex, normaliser
`p=A_y*sgn(B_y)` et `q=abs(B_y)`. Avec `C=q*G` et
`H_i=q*W_i+p*n_i`, le minimum d'un axe, multiplié par `4*C`, vaut
`4*C*(C*e^2-H_i*e)` si le clamp tombe sur un bout `e`, et `-H_i^2` sinon.
Sous u16, les produits intermédiaires montent à environ 278 bits : un entier
signé de 320 bits (`BigInt<5>` dans l'ABI actuelle) est la borne conservative à
recevoir. `BigInt<4>` ne suffit pas à promettre ce prune. Le test est strict :
`min>0` taille, `max<0` crédite ; toute égalité descend ou range-reporte.

La source device suivante ne doit donc plus copier un CSR par seed. Elle lit un
Morton BVH global et porte des tâches :

```text
AxisNodeJob {seed_id, witness_node, side, cutoff}
BallNodeJob {candidate_id, witness_node, capped_count}
```

Les tâches `MIXED` rejoignent une wavefront, les `OUT` disparaissent et les
`ALL` créditent leur population. Les heaps restent petits par warp ; le gros
tableau `Selection` par thread disparaît. Le flat scan de `2c14313` reste alors
l'ablation de référence et le falsificateur leaf-by-leaf.

## 6. Gate causale recommandée

Avant une session G4 :

- graver des CTests `--axe --verifie` aux trois tailles bornées et comparer les
  ensembles q4, pas seulement leur cardinal ;
- ajouter un cas `batch_complete=true,cap=0` et un cas tronqué qui rend un fate
  explicite ;
- comparer `RationalBallRange` au flat scan sur chaque seed, chaque groupe de
  roots et chaque `I_B/U_B` ;
- tuer `B_sign_ignored`, `Q_theta<=0`, `max_not_corners`, `min_not_clamped`,
  shell ignoré et parent+enfant doublement crédités ;
- publier `node_visits/seed`, `ALL/OUT/MIXED`, roots, égalités, octets, HWM,
  local bytes, occupancy, H2D, kernel, D2H et total.

La porte de promotion est une baisse de `sites_lus/seed`, pas seulement un
grand nombre de `Msites/s`. Le contrat d'une seconde reste bout-en-bout et
inclut encore les trois lanes autonomes, `BallKey/RLE`, fold, forêts,
verticales et copie hôte.

## 7. Relecture statique de la recette G4 `11130cb`

La recette a de bonnes décisions : arrêt ciblé par génération, parité avant
débit et rapatriement du brut avant le verdict. Elle ne doit toutefois pas être
lancée telle quelle : ses douze runs ont chacun `timeout 900`, soit jusqu'à
`10 800 s`, alors que l'arrêt invité est armé à `4 500 s`. La variable
`RUN_TIMEOUT=3300` annoncée n'encadre aucune commande. Une campagne peut donc
être coupée avant son reçu complet.

Réparation constructive minimale :

1. faire un smoke de quatre petits cas (`2 familles x 2 smax`) avec une borne
   courte et parité obligatoire ;
2. si vert, lancer les tailles croissantes sous un timeout **global** inférieur
   à `RUN_TIMEOUT`, et `timeout --kill-after` pour chaque binaire ;
3. exiger exactement le nombre de runs planifié, `cap=0`, les hashes et
   `ecarts=0` ;
4. copier le transcript seulement après avoir ajouté un éventuel
   `[ARRET NON CERTIFIE]` : dans le trap courant, la copie précède encore cette
   ligne malgré le commentaire inverse ;
5. publier séparément construction CSR, H2D, kernel, D2H et total.

Ce protocole reçoit utilement la baseline plate. Il ne faut pas attendre la
wavefront BVH pour apprendre si l'arithmétique host/device est identique, mais
il faut garder `kernel_flat_scan` et `warm_e2e` comme deux métriques sans lien
de promotion automatique.

GCP non utilisé.
