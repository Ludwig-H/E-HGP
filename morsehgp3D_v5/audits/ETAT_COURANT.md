# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Dernier commit fonctionnel relu :** `ab2c2563` (reçu G4 n° 12 et interprétation des étapes device).
- **Pins de performance jugés :** `82f613d3` pour le chemin CPU 50–200 k et `63deda74` pour les étapes device à 50 k. La synchronisation q4 ajoutée au second pin reste intrusive.
- **Pins principaux inclus :** `90baa0bb` (fold concurrent), `d86b4ec7` (census inline et RSS), `82f613d3` (grille de cellules), `b164fcbe` et `369f3ac0` (sondes du fold), `9fba11a5` (préfixe et échelle), `63deda74` et `ab2c2563` (étapes device et reçu).
- **Reçus relus :** sessions G4 n° 10, 11 et 12.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
- **Worktree observé :** des modifications concurrentes non committées ajoutent notamment des histogrammes de lots dans `q3_lane_batched.hpp`/`q4_lane_batched.hpp` et préparent une phase `SCALE_THREADS` dans `gcp-migration/v5_campaign_remote.sh` ; elles sont postérieures à la validation, non jugées et exclues du commit. `.codex_fold_contract_probe.cpp`, non suivi et produit par un autre auditeur, est également exclu.

## Verdict

**Orange : l'exploration CPU peut continuer, mais aucun claim ni conclusion de dimensionnement n'est reçu.**

La grille repose sur un certificat affine entier pertinent et le code emploie la bonne inégalité. Les campagnes appariées montrent des digests inchangés et un gain de temps borné sur les cas mesurés. Ces éléments sont utiles et justifient de poursuivre la voie.

Trois verrous empêchent toutefois de prendre le tip comme base sûre d'une nouvelle optimisation :

1. le fold concurrent peut déplacer son `RunResult` avant d'avoir joint les fils sur un retour anticipé, ou terminer le processus si l'enregistrement d'un fil déjà lancé échoue ;
2. une exception d'un K supérieur peut annuler les publications de K inférieurs et contredit donc l'arbitrage annoncé par ordre de K ;
3. la sonde de facettes « vivantes » sous-estime structurellement l'état nécessaire et ne dimensionne pas un fold streamé compact.

La réception formelle de la grille reste également conditionnée à la correction du théorème publié et à une porte indépendante de localisation/comptage. Aucun faux kill produit n'a été observé dans les campagnes nominales relues.

Le `README.md` produit n'est pas frais par rapport au HEAD et transforme encore une cible de conformité en affirmation générale (« même objet que la v4 », « niveaux et événements exacts »). Le présent état ne lève donc pas l'interdiction de claim.

## Audit ciblé — rendement GPU et multi-CPU

La réponse chiffrée et le plan expérimental sont dans
[`AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md`](AUDIT_RENDEMENT_GPU_MULTICPU_20260828.md).
Synthèse :

- au reçu n° 12, le GPU est à parité mixte : 0,968× `uniform`, 1,124× `terrain`, 0,958× `eight_clusters` et 1,015× `scanline` sur le mur interne CPU/GPU ; un seul passage CPU puis GPU ne permet pas un claim de gain ;
- le port ne remplace que les corps q3/q4 après une préparation hôte coûteuse. Sur `uniform` 50 k, cette fraction ne vaut que 8,9 % du mur CPU, donc son plafond idéal est environ 1,10× ;
- le faible scaling multi-CPU G4 n'est pas démontré : aucun reçu ne balaie 1 à 48 fils au même pin et `ouvriers=48` ne mesure que des fils créés ;
- une contre-sonde locale non reçue donne néanmoins 58,04 / 31,40 / 17,70 / 13,57 s à 1 / 2 / 4 / 8 fils sur `uniform` 4 k. Le gain existe, mais son rendement chute au-delà des quatre cœurs locaux ;
- les plafonds architecturaux sont le reduce et le digest séquentiels par K, deux ordres B en vol, des rectangles lourds indivisibles, les copies/fusions de gros vecteurs et, côté GPU, les matérialisations, trois synchronisations q4 et une télémétrie `kernel_ms` non homogène.

La session n° 12 reçoit l'instrument `63deda74` : 25 runs terminés, quatre
digests CPU/GPU égaux et mutant device tué. Elle établit que K1 q3 est petit
(204 ms cumulés pour 87 M seeds `uniform`), mais pas que le kernel q4 est petit :
`k1+d2h` reste agrégé. Les temps `scan()` sont toujours additionnés sur des fils
concurrents et ne peuvent pas être soustraits au mur `rects`; le calcul de
« 2,7 s de reste hôte » publié dans `docs/GPU.md` est invalide. Q4 transfère
60 octets/site pour ses tableaux principaux, pas 64, et aucun compteur d'octets
n'est gravé. L'adaptatif modifie sensiblement la charge device, sans gain global
reproductible démontré. Mesurer un vrai mur de lane, chaque kernel/D2H et les
octets, puis neutraliser ou ablater la barrière H2D.

## Audit ciblé — passage à l'échelle

Le verdict détaillé et l'ordre de travail sont dans
[`AUDIT_PASSAGE_ECHELLE_20260828.md`](AUDIT_PASSAGE_ECHELLE_20260828.md).
`docs/ECHELLE.md` conserve une direction utile, mais ne dimensionne pas encore
une exécution 10 M :

- `profil_vivantes` échantillonne après le lot et omet la fermeture union-find ;
  les 40 facettes/point et les 30/90 Go ne sont pas des majorants ;
- « empreinte 64 bits → compte u8 » n'est ni une identité exacte, ni assez large,
  ni une table à 7 octets ; l'oubli requiert reroot/compression prouvés ;
- le stream doit reconstruire `batch_levels`, deltas, clés et partition finale.
  Avec le wire courant, sa sortie vaut déjà au moins 1,495 To à 10 M sur
  l'extrapolation `uniform`, avant cadrage et reprise ;
- le routage par centre co-localise les doublons, mais ne borne pas une tuile à
  200 k boules. Il faut spill, barrière globale puis tri/RLE exact ;
- le manifeste décrit ne permet pas une reprise intra-K. Le premier contrat
  honnête est `resume=replay_current_K` avec publication atomique par K ;
- la G4 courante utilise un Hyperdisk Balanced 100 GB à 290 Mio/s, pas un
  PD-SSD reçu à 1–2 Go/s. Le minimum séquentiel runs+sortie extrapolé vaut déjà
  environ 107 minutes d'E/S idéales ;
- le fold `uniform` ×50 donne 1,60–1,78 h selon la métrique, pas les 3–4 h du
  tableau sans facteur supplémentaire. Les 6–7 h restent un scénario, pas une
  loi ni une porte de session.

Avant de coder le compactage, spécifier le wire et le décodeur, compter les
dernières incidences par tri externe de `FacetKey` exactes, tuer les fixtures
mono-lot/chaîne, puis recevoir un pilote 1 M avec pics et E/S externes.

## P1 — sûreté du fold concurrent

Dans `src/pipeline/run.hpp`, `BJoiner` ne joint les fils que dans son destructeur. Or plusieurs `return rr` restent possibles après le lancement d'un fil B. La valeur de retour est initialisée avant la destruction des variables locales : sans NRVO, `rr` peut être déplacé pendant qu'un fil écrit encore ses temps, cartes, digests ou RSS. Les verts nominaux ne couvrent pas cette séquence.

Une seconde fenêtre d'exception existe entre la construction de `slot->t` et `slots.push_back`. Si l'insertion du `unique_ptr` lève, le `BSlot` local détruit un `std::thread` encore joignable et appelle `std::terminate`.

Enfin, le `catch` d'un fil B pose immédiatement `pub_failed`, y compris lorsque l'exception vient d'un K supérieur qui a terminé avant les K précédents. Les fils inférieurs réveillés abandonnent alors leur publication et peuvent ne jamais contrôler leurs violations d'invariant. Nuance apportée au contre-audit précédent : `reap_front` relit normalement les seules exceptions de réduction/digest stockées en ordre de K. Le défaut certain est la suppression des publications inférieures et l'absence d'arbitrage commun entre un retour d'étage A supérieur et une erreur B inférieure, pas la seule identité de l'exception relancée en chemin nominal.

Correction demandée :

- centraliser annulation, notification et jointure explicites avant chaque retour post-lancement ;
- faire posséder le slot par le conteneur avant de démarrer le fil, ou joindre dans le chemin d'échec ;
- conserver le résultat ou l'exception dans chaque slot et ne décider l'échec global que lorsque ce slot atteint `next_publish` ;
- injecter un échec d'étage A à `K >= 2` pendant qu'une réduction est active et une exception de réduction d'un K supérieur avant la fin d'un K inférieur, avec `-fno-elide-constructors` et TSan ;
- protéger le journal de la porte par mutex ou atomiques : `last_k`, `ordered` et `overlapped` entrent eux-mêmes en course si le chevauchement interdit survient ;
- ajouter un compteur atomique qui prouve `peak_fold_inflight >= 2`, car la porte courante passerait aussi avec des réductions entièrement sérielles.

Cette correction doit précéder une nouvelle optimisation du fold.

## P1 de réception — grille de cellules

La réponse détaillée à Claude est dans [`QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md`](QUESTION_CLAUDE_GRILLE_DE_CELLULES_20260828.md). Synthèse :

- l'argument affine aux quatre sommets est valide même pour un site hors de la boule diamétrale ;
- `cell_needed` et les inégalités de distance aux arêtes de `bisector_basis` couvrent bien le disque des centres ;
- `docs/MATHEMATIQUES.md` ajoute à tort un facteur `G` au membre gauche du théorème 10.5, alors que `cell_grid.hpp` emploie la formule correcte ;
- le coefficient flottant `2^-46 = 128u` paraît suffisamment conservateur sous les hypothèses IEEE usuelles, mais le décompte d'opérations donné par la question est incomplet. La preuve doit couvrir conversions, échelle, arrondis de la borne et environnement d'exécution ;
- `count_site_t` paraît exact et amorti par inspection, mais aucune autorité indépendante ne le compare encore à l'évaluation i128 directe des 289 sommets ;
- F10 exerce la stricte frontière des témoins, pas un centre exactement situé sur une arête ou un coin de cellule, ni une boîte de corde q4 traversant une cellule vivante ;
- `cell_min_sites=0` ne force pas la grille, car le ratio et `near_m` restent actifs. L'oracle ON/OFF présenté comme systématique possède donc un trou de couverture.

Réparation minimale : corriger le théorème, graver une dérivation d'erreur conservative ou un repli exact, ajouter l'oracle brut du compteur, puis fournir un mode de test interne qui force réellement la grille et les frontières de localisation. Il n'est pas demandé de rendre ce mode public ni d'en faire l'architecture produit.

## P2 — intégration et autorité des mesures

### Lanes par lots

- La lane q3 construit la grille avant le routage puis la reconstruit dans `scan_anchor_q3` sur le chemin hôte ou surdimensionné. La lane q4 la reconstruit sur son chemin surdimensionné. Le coût et `grids_built` peuvent donc être doublés sans changer l'objet.
- `GenerateOptions::cell_grid_min_sites` et `BatchLimits::cell_grid_min_sites` constituent deux autorités. Une option ON/OFF CPU n'a pas nécessairement le même effet dans la lane batched/device.
- `grids_built` est incrémenté avant le succès de `CellGrid::build`, qui peut échouer ouvert sur la base ou la capacité. Le compteur mesure des tentatives, pas des grilles construites ; il faut séparer au moins `attempted`, `built` et les replis.
- Les portes device ne comparent pas tous les nouveaux compteurs cellule, notamment `grids_built`. Les sorties G4 observées concordent ; l'enforcement permanent reste incomplet.

Il faut différer la grille après le routage ou transmettre explicitement « grille déjà appliquée », unifier le seuil et comparer les compteurs sur les chemins hôte, device et surdimensionné.

### Profil du fold et mémoire

- `fold_inflight <= 0` est silencieusement ramené à 1 par `std::max`, alors que les autres options hors profil sont refusées. Définir le domaine accepté et rejeter explicitement les autres valeurs.
- `369f3ac0` échantillonne après chaque macro-lot. Une facette née et terminée dans le même lot disparaît avant l'échantillon ; un fold mono-lot peut donc annoncer zéro facette vivante tout en les ayant toutes requises.
- Une facette sans incidence directe future peut rester racine ou ancêtre d'une composante ayant encore des incidences. Le payload final conserve en outre les clés et canoniques de toutes les facettes. La mesure directe n'est qu'une borne basse descriptive.
- La sonde est insérée entre les `ptick` : son initialisation et ses mises à jour contaminent les temps attribués au lot suivant, tandis que la dernière mise à jour n'est pas imputée. Ses lignes concurrentes n'indiquent pas K.
- `rss_mb[4]` est échantillonné après `reduce_fold` et la publication, donc après libération de l'état lourd de cet ordre. Ce n'est pas un « maximum pendant le fold ». Sur `uniform` 200 k, le brut de la session 11 rapporte `75 828 184 KiB`, soit environ 72,3 GiB, au pic externe contre `66 257 MiB`, soit environ 64,7 GiB, pour cet échantillon interne.
- « −190 o par boule » n'est pas un invariant de layout. Le nouvel objet possède une base inline fixe alors que l'ancien payait deux vecteurs et des capacités variables ; le gain dépend donc de l'ABI, des anciennes allocations et de l'allocateur. Le `sizeof(BallData)` annoncé doit être gravé par la mesure. La session 11 mélange cette modification avec la grille.

Avant de conclure à un état borné, mesurer le pic intra-lot, la fermeture des racines actives et les octets persistants sur une chaîne adversariale et un plateau mono-lot. Renommer le palier RSS ou conserver `ru_maxrss` comme autorité de pic. Une ablation mémoire doit garder le même pin, le même allocateur et graver `sizeof(BallData)`.

## Preuves positives, mais bornées

- Le reçu G4 n° 11 contient 31 statuts terminés au pin `82f613d3`, dont 30 succès nominaux et le rejet attendu d'un mutant. Les digests communs aux sessions 10 et 11 concordent.
- Le reçu G4 n° 12 contient 25 statuts terminés au pin `63deda74`, quatre paires CPU/GPU au même digest et le rejet attendu du mutant device. Il reçoit l'instrumentation, pas ses surinterprétations causales.
- Sur le cas `scanline` 200 k, la session rapporte 502 vers 268 secondes, dont la lane q4 438 vers 215 secondes. Ce résultat justifie une ablation propre ; il n'isole pas la grille des listes inline et ne prouve pas l'exactitude générale.
- Les deux tests cellule étiquetés uniquement `oracle` ne sont pas inclus dans la commande G4 `-L gate`. Ce n'est pas un besoin de GPU, mais le résumé « oracle » du reçu ne doit pas être lu comme un rejeu de la suite oracle complète.
- Les `session.log` présents dans le workspace au moment de l'audit contiennent `remote_campaign_rc=0`, `scp_rc=0`, la validation complète et la certification finale `TERMINATED`, mais `*.log` est ignoré. Les reçus n° 10 à 12 restent donc non autoportants sur toute la transaction ; un checkout frais ne contient pas ces transcripts.

## Validation locale de cet audit

Configuration indépendante dans un worktree détaché propre au pin jugé :

```text
cmake -S morsehgp3D_v5 -B build/v5 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5 --parallel
```

Réception exhaustive antérieure au pin fonctionnel `369f3ac0` :

- build Release : succès ;
- suite `gate` : 168/168 succès, 745,12 s réelles ;
- suite `oracle` : 9/9 succès, 13,62 s réelles, y compris les deux oracles cellule absents de `gate` ;
- complément exhaustif `-LE 'gate|oracle'` : 15/15 succès, 1 038,98 s réelles ; l'union couvre les 185 CTests ;

Validation locale au HEAD `ab2c2563` pendant le présent audit :

- configuration et build Release CPU : succès ;
- porte de préfixe, y compris son mutant : 5/5 CTests, 22,12 s ;
- `python tools/check_docs.py` : 213 Markdown actifs validés ;
- `python tools/check_implementation_status.py` : 20 phases et leurs portes validées ;
- validation explicite du dossier `audits/` : 8 Markdown validés.

Aucun `nvcc` n'est disponible localement : le présent audit n'a pas rejoué CUDA. La session G4 n° 12 reçoit séparément compilation et exécution device au pin `63deda74`. Aucun sanitizer n'a été revendiqué. Un vert CTest ne ferme ni la course de retour, ni la vacuité de la concurrence, ni les lacunes d'oracle décrites plus haut.

## Ordre de fermeture proposé à Claude

1. sécuriser tous les retours et l'ownership des fils du fold, puis ajouter les fautes injectées et le plancher de concurrence ;
2. spécifier le wire du stream, son décodeur et `resume=replay_current_K`, puis fermer les comptes exacts et les fixtures d'éviction avant le compactage ;
3. corriger le théorème 10.5 et fermer localisation, compteur direct et mode force-grid ;
4. supprimer les doubles constructions batched, unifier les seuils et comparer les compteurs ;
5. redéfinir les sondes de vie/RSS, recevoir le routage avec spill puis un pilote 1 M avant tout dimensionnement 10 M ;
6. rafraîchir `README.md`, `docs/PROVENANCE.md` et le plan de tests avec des formulations de cible et de preuve bornée.

Les erreurs de prose du théorème 10.4, F5 qui annonce 28 sites mais en construit 26, F7 qui reste coplanaire et l'ablation contrefactuelle agrégée sont conservées dans le pont V7–V14. Elles ne doivent pas interrompre les deux corrections P1 ci-dessus.

GCP non utilisé pour cet audit.
