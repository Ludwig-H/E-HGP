# État courant audité de MorseHGP3D v5 — 28 août 2026

- **Pin fonctionnel jugé :** `369f3ac0`.
- **Pins principaux inclus :** `90baa0bb` (fold concurrent), `d86b4ec7` (census inline et RSS), `82f613d3` (grille de cellules), `b164fcbe` et `369f3ac0` (sondes du fold).
- **Reçus relus :** `69daa148` et `685e8e22`.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
- **Worktree observé :** les seules modifications suivies sont le présent nettoyage d'audit ; `.codex_fold_contract_probe.cpp`, non suivi et produit par un autre auditeur, est exclu du périmètre et du commit.

## Verdict

**Orange : l'exploration CPU peut continuer, mais aucun claim ni conclusion de dimensionnement n'est reçu.**

La grille repose sur un certificat affine entier pertinent et le code emploie la bonne inégalité. Les campagnes appariées montrent des digests inchangés et un gain de temps borné sur les cas mesurés. Ces éléments sont utiles et justifient de poursuivre la voie.

Deux verrous empêchent toutefois de prendre le tip comme base sûre d'une nouvelle optimisation :

1. le fold concurrent peut déplacer son `RunResult` avant d'avoir joint les fils sur un retour anticipé ;
2. la sonde de facettes « vivantes » sous-estime structurellement l'état nécessaire et ne dimensionne pas un fold streamé compact.

La réception formelle de la grille reste également conditionnée à la correction du théorème publié et à une porte indépendante de localisation/comptage. Aucun faux kill produit n'a été observé dans les campagnes nominales relues.

Le `README.md` produit n'est pas frais par rapport à ce pin et transforme encore une cible de conformité en affirmation générale (« même objet que la v4 », « niveaux et événements exacts »). Le présent état ne lève donc pas l'interdiction de claim.

## P1 — sûreté du fold concurrent

Dans `src/pipeline/run.hpp`, `BJoiner` ne joint les fils que dans son destructeur. Or plusieurs `return rr` restent possibles après le lancement d'un fil B. La valeur de retour est initialisée avant la destruction des variables locales : sans NRVO, `rr` peut être déplacé pendant qu'un fil écrit encore ses temps, cartes, digests ou RSS. Les verts nominaux ne couvrent pas cette séquence.

Une seconde fenêtre d'exception existe entre la construction de `slot->t` et `slots.push_back`. Si l'insertion du `unique_ptr` lève, le `BSlot` local détruit un `std::thread` encore joignable et appelle `std::terminate`.

Correction demandée :

- centraliser annulation, notification et jointure explicites avant chaque retour post-lancement ;
- faire posséder le slot par le conteneur avant de démarrer le fil, ou joindre dans le chemin d'échec ;
- injecter un échec d'étage A à `K >= 2` pendant qu'une réduction est active, avec `-fno-elide-constructors` et TSan ;
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
- Les portes device ne comparent pas tous les nouveaux compteurs cellule, notamment `grids_built`. Les sorties G4 observées concordent ; l'enforcement permanent reste incomplet.

Il faut différer la grille après le routage ou transmettre explicitement « grille déjà appliquée », unifier le seuil et comparer les compteurs sur les chemins hôte, device et surdimensionné.

### Profil du fold et mémoire

- `369f3ac0` échantillonne après chaque macro-lot. Une facette née et terminée dans le même lot disparaît avant l'échantillon ; un fold mono-lot peut donc annoncer zéro facette vivante tout en les ayant toutes requises.
- Une facette sans incidence directe future peut rester racine ou ancêtre d'une composante ayant encore des incidences. Le payload final conserve en outre les clés et canoniques de toutes les facettes. La mesure directe n'est qu'une borne basse descriptive.
- La sonde est insérée entre les `ptick` : son initialisation et ses mises à jour contaminent les temps attribués au lot suivant, tandis que la dernière mise à jour n'est pas imputée. Ses lignes concurrentes n'indiquent pas K.
- `rss_mb[4]` est échantillonné après `reduce_fold` et la publication, donc après libération de l'état lourd de cet ordre. Ce n'est pas un « maximum pendant le fold ». Sur `uniform` 200 k, le brut de la session 11 rapporte `75 828 184 KiB`, soit environ 72,3 GiB, au pic externe contre `66 257 MiB`, soit environ 64,7 GiB, pour cet échantillon interne.
- « −190 o par boule » n'est pas un invariant de layout. Le nouvel objet possède une base inline fixe alors que l'ancien payait deux vecteurs et des capacités variables ; le gain dépend donc de l'ABI, des anciennes allocations et de l'allocateur. Le `sizeof(BallData)` annoncé doit être gravé par la mesure. La session 11 mélange cette modification avec la grille.

Avant de conclure à un état borné, mesurer le pic intra-lot, la fermeture des racines actives et les octets persistants sur une chaîne adversariale et un plateau mono-lot. Renommer le palier RSS ou conserver `ru_maxrss` comme autorité de pic. Une ablation mémoire doit garder le même pin, le même allocateur et graver `sizeof(BallData)`.

## Preuves positives, mais bornées

- Le reçu G4 n° 11 contient 31 statuts terminés au pin `82f613d3`, dont 30 succès nominaux et le rejet attendu d'un mutant. Les digests communs aux sessions 10 et 11 concordent.
- Sur le cas `scanline` 200 k, la session rapporte 502 vers 268 secondes, dont la lane q4 438 vers 215 secondes. Ce résultat justifie une ablation propre ; il n'isole pas la grille des listes inline et ne prouve pas l'exactitude générale.
- Les deux tests cellule étiquetés uniquement `oracle` ne sont pas inclus dans la commande G4 `-L gate`. Ce n'est pas un besoin de GPU, mais le résumé « oracle » du reçu ne doit pas être lu comme un rejeu de la suite oracle complète.
- Les journaux versionnés ne rendent pas encore les reçus G4 entièrement autoportants : le journal de session ignoré est le seul fichier qui agrège validation, transfert et certification finale `TERMINATED`.

## Validation locale de cet audit

Configuration indépendante :

```text
cmake -S morsehgp3D_v5 -B build/v5-audit-codex -DCMAKE_BUILD_TYPE=Release
cmake --build build/v5-audit-codex --parallel 8
```

Résultats au pin jugé :

- build Release : succès ;
- ciblés `api_guard`, fixture et oracles d'ancre/cellule : 4/4 succès ;
- suite `gate` : 168/168 succès, 854,16 s réelles ;
- suite `oracle` : 9/9 succès, 14,17 s réelles, y compris les deux oracles cellule absents de `gate` ;
- `python tools/check_docs.py` : 212 Markdown actifs validés ;
- `python tools/check_implementation_status.py` : 20 phases et leurs portes validées ;
- validation explicite du dossier `audits/` : 6 Markdown validés.

Aucun test CUDA, sanitizer ou GPU n'a été revendiqué. Un vert CTest ne ferme ni la course de retour, ni la vacuité de la concurrence, ni les lacunes d'oracle décrites plus haut.

## Ordre de fermeture proposé à Claude

1. sécuriser tous les retours et l'ownership des fils du fold, puis ajouter les fautes injectées et le plancher de concurrence ;
2. corriger le théorème 10.5 et fermer localisation, compteur direct et mode force-grid ;
3. supprimer les doubles constructions batched, unifier les seuils et comparer les compteurs ;
4. redéfinir les sondes de vie/RSS avant de parler de dimensionnement, puis faire les ablations au même pin ;
5. rafraîchir `README.md`, `docs/PROVENANCE.md` et le plan de tests avec des formulations de cible et de preuve bornée.

Les erreurs de prose du théorème 10.4, F5 qui annonce 28 sites mais en construit 26, F7 qui reste coplanaire et l'ablation contrefactuelle agrégée sont conservées dans le pont V7–V14. Elles ne doivent pas interrompre les deux corrections P1 ci-dessus.

GCP non utilisé pour cet audit.
