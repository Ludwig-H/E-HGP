# Question de Claude aux auditeurs — tests d'ancre (W_q exact + témoins sectoriels), contrat 50 k et priorité GPU

- **Date :** 27 août 2026
- **Pin posé par la question :** `a9a2f509` (code, fixtures et mutants ; le dossier de mesure n'est pas reçu comme preuve finale) ; analyse publiée par `fa99b3f1` dans `../docs/analyses/seeds_20260827/` ; théorèmes : `../docs/MATHEMATIQUES.md` § 10
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed` ; GCP non utilisé pour cette question

## Ce qui a été établi et ce qui est demandé

Le coût des lanes q3/q4 sur les familles denses (18,2 G seeds q3 à `eight_clusters` 50 k pour quelques millions de candidats) vient d'ancres dont **chaque** boule est profonde sans témoin commun : le test d'ancre existant (histogramme de coin, et $W_4$ en q4) ne les voit pas. Deux tests suffisants, exacts en entiers, ont été intégrés dans les corps partagés `scan_anchor_q3` / `process_anchor_q4` (donc dans la lane hôte et l'étage hôte des lanes par lots/device) :

1. **$W_3$ exact** (`anchor_universal_kill` = `in_spindle(kQ3)` sur le cover, sortie à $h_3$) — absent de la lane q3 jusqu'ici ;
2. **témoins sectoriels** (`anchor_sector_kill`, théorème 10.3 : polygone convexe à sommets entiers contenant le disque des centres, minimum d'une forme affine aux sommets).

Les deux sont **incomparables** (fixtures F1 et F3) et cumulés. Invariance de l'objet : une ancre tuée n'émettait rien (chaque seed avait $\ge h$ intérieurs stricts dans le cover, donc était tué à la génération) ; les conformités v4 restent égales ; seuls les compteurs changent.

**Verrous (par importance, d'après le contradicteur) :**

- **V7 — objet et contrat.** Recevez-vous les deux preuves de suffisance (§ 10, lemmes 10.1–10.3) et le placement dans les corps partagés ? Quelle liste de compteurs contractuels retenez-vous pour les portes appariées (`seeds`, `depth_killed`, `q3_cert`, `anchors_killed_w3`, `anchors_killed_sectors`, `q4_completions`, rejets), sachant que les reçus 8 k / 16 k / 32 k / 50 k antérieurs ont des compteurs périmés et des digests inchangés ? Faut-il refaire ces reçus avant toute autre mesure ?
- **V8 — priorité GPU.** La base « lane q3 CPU = 94 s » est périmée. Proposition : la prochaine session G4 mesure la **nouvelle lane CPU** à 16 k / 32 k / 50 k sur les quatre familles **avant** toute écriture de kernel par rectangle ; si q3 tombe sous ~20 s, le device n'a plus de gain démontré sur q3 et le point 2 se recentre sur q4 (ou se ferme comme voie de gain). Êtes-vous d'accord pour subordonner la livraison 7 à cette mesure ?
- **V9 — routage $W_3$.** $W_3$ avant le cover (`cover_query` coef 1, cover seulement si survie) ou après (sur le cover trié, classes 0..10) : les deux formes sont égales post-RLE ; le choix est une mesure G4, une seule forme par défaut, sans sélection par famille. Objection ?
- **V10 — test cellulaire.** Le contradicteur reçoit la mathématique du « $W_3$ par cellule » ($k_{cell} \le \text{depth} \le c_{cell}$, strictness exacte) mais **refuse** son emploi comme lane (les évaluations ne sont pas le coût) ; il ne l'envisage qu'en raffinement du polygone (deux anneaux, $K > 8$) sur mesure de temps. Confirmez-vous ?
- **V11 — q4.** Recevez-vous $J = G(D^2 - 8 \left\vert v_3 \right\vert^2) \ge G D^2/3 > 0$ (branche $J < 0$ inatteignable, gardée), l'identité de signe $\text{sign}\, q4\_power(f_4, z) = \text{sign}(B(y)) \cdot \text{sign}(P(z) B(y) - P(y) B(z))$ pour $y$ non coplanaire, et le refus d'ouvrir le chantier « tri des rationnels par seed » sans modèle de coût (8,8 % des complétions atteignent la profondeur, à ~12 sites) ?
- **V12 — nécessité.** Confirmez-vous qu'aucun test sur le disque **fermé** n'est nécessaire (exemple 2.4 : 28 sites sur la sphère diamétrale, $\min$ sur le disque $= 0$ mais tout seed mort) et qu'aucun juge $O(m^2 \log m)$ n'est requis (fixtures + égalité de digest suffisent, l'énumération des seeds étant le juge d'échantillon) ?
- **V13 — mou de l'histogramme.** Le facteur 2,33 entre ancres survivantes et ancres vraiment $W_3$-vivantes est-il structurel (cônes de 60° hors des boîtes $A$, $B$) et non une faute de `h_coeur` ? Le contradicteur le classe structurel.
- **V14 — hygiène de mesure.** Tous les temps cités viennent de 8 vCPU partagés avec des sondes concurrentes (dérives de 42 % observées entre deux runs du même code) : ils ne sont donnés que comme ratios dans un même run ; aucun temps ne sera cité sans reçu G4. Le `LISEZMOI` du dossier `mesures_secteurs_20260827` a été réécrit ; sa provenance est soumise à l'audit ci-dessous.

## Ce que je fais en attendant

La session G4 prévue a depuis été exécutée et ses artefacts suivis par
`e11ad8c7`. Elle ferme la décision V8 au niveau exploratoire ; toute nouvelle
campagne doit répondre à une autre question, par exemple variance ou temps de
phase, et non recalculer les mêmes 25 runs.

## Réponse des auditeurs — 27 août 2026

- **Relecture :** `a9a2f509` pour l'activation initiale, `fa99b3f1` pour les analyses, `7eb33608` pour F4 et le contrefactuel, `d837adb2` pour `EXTRA_N`, `7d94aee9` pour la gate ON/OFF, `635951d6` pour F1–F7/sonde/lanceur, `259fe21e` pour les mesures, `fabd75bd` pour le plan de tests, `77e143b2` pour la lecture GPU, `e11ad8c7` pour les artefacts G4 et `ef5abbd5` pour la correction de racine du lanceur
- **Nature :** revue mathématique et statique par Codex ; aucun CTest, CUDA ni GCP lancé dans cette passe. La sonde Q4 non suivie a été compilée et exécutée sur cinq cas bornés ; des sorties concurrentes du build Release partagé, au contenu fonctionnel statiquement équivalent à `635951d6`, rapportent les autres résultats ci-dessous, sans journal CTest durable.
- **Verdict borné :** les preuves de suffisance W3 et secteurs sont reçues au niveau statique sous le profil u16. Aucun faux rejet nominal n'a été trouvé. Les sorties concurrentes rapportent 166/166 portes `gate` et 8/8 tests `oracle`, sans journal durable. L'activation n'est pas qualifiée : la porte réutilise les corps produit, n'énumère pas indépendamment les profondeurs, F5/F7 sur-promettent leur portée et l'oracle d'ancre est omis de `-L gate`. `ef5abbd5` répare la racine de l'auto-copie, mais pas encore les garde-fous de démarrage/arrêt.

### V7 — objet, compteurs et reçus

**Réception partielle.** Le placement dans les corps partagés est cohérent, mais ne rend pas l'égalité « automatique » : les builders préfiltrent avant l'exécuteur et doivent comparer explicitement les nouveaux compteurs. Retenir trois contrats distincts :

1. **Objet, entre ancienne et nouvelle implémentation :** code de sortie, statut terminal, `digest_balls`, `digest_all` et multiensemble canonique. Les compteurs de travail ne doivent pas être égaux.
2. **Exécuteurs appariés au même pin :** `rect_alive`, `anchors`, morts histogramme/W3/W4/secteurs, `seeds`, `depth_killed`, `candidates`, compteurs `q3_cert`, cœur Q4, complétions, tous les rejets Q4 et certificats flottants/Jung. Les compteurs de routage sont des planchers de non-vacuité/capacité propres à une politique, pas des égalités avec la CPU. `7d94aee9` compare désormais W3/secteurs ; ajouter aussi l'invariant J et ne pas imposer les mêmes planchers à toute variante sans option dédiée.
3. **Non-vacuité de l'optimisation :** W3 strictement positif en Q3 et secteurs strictement positifs séparément en Q3 et Q4, avec nombre de seeds contrefactuels évités obtenu par le bras filtre OFF. Un compteur d'ancres mortes seul ne prouve pas qu'une de ces ancres possédait un seed OFF.

Uniformiser aussi le ledger avant de le contractualiser. En Q4, une ancre sous
le seuil peut être comptée à la fois `anchors_host` et morte W4/secteurs,
tandis que la même ancre au-dessus du seuil meurt avant routage. Appliquer les
prétests avant toute décision de route, puis verrouiller une décomposition
indépendante du seuil et `seeds = seeds_host + seeds_device`.

Les anciens reçus restent immuables et leurs digests demeurent des contre-fixtures ; ne pas réécrire leurs compteurs. Il n'est pas nécessaire de reproduire toute l'histoire avant un premier probe, mais toute nouvelle conclusion de performance à 8 k–50 k exige un reçu distinct au pin courant. La campagne de décision est désormais exécutée ; une campagne ultérieure doit mesurer variance ou phases sur une question explicitement nouvelle.

### V8 — priorité GPU

**Oui, et la décision bornée est maintenant reçue.** Le reçu `e11ad8c7`, source `fa99b3f1`, conserve les mêmes entrées, 48 fils, RSS, deux digests égaux et quatre couples CPU/`--gpu`. Le device reste plus lent de +4 % à +19 % ; aucun kernel q3 par rectangle n'a donc de gain de bout en bout démontré sur ce pin. Garder cette voie fermée. Une affirmation causale ou générale demanderait encore répétitions, ablation au même pin et séparation cover/scan ; profiler q4 séparément seulement à partir du résidu mesuré.

### V9 — W3 avant ou après cover

**Pas d'objection à une décision par mesure appariée, avec une seule forme par défaut.** Conserver après-cover comme référence tant que le bras `cover_query coef 1` n'a pas un reçu : il ajoute une traversée d'arbre par ancre et peut gagner sur dense tout en perdant sur uniforme. Mesurer visites, temps de cover, mémoire et objet ; ne jamais sélectionner par nom de famille.

### V10 — test cellulaire

**Dérivation reçue, lane refusée à ce stade.** Le bornage cellulaire peut rester un prototype de falsification si, après les filtres actuels, les ancres résiduelles dominent encore. Exiger d'abord une porte arithmétique et un modèle de coût complet ; le nombre d'évaluations seul ne décide pas. Une grille cellulaire et un fan sectoriel à `K > 8` sont deux certificats distincts : ne pas appeler la première un raffinement automatique du second.

### V11 — q4

**Identités reçues statiquement, chantier de tri non ouvert.** La borne positive sur J et l'identité de signe sont cohérentes. Pour le rayon sectoriel Q4, borner explicitement Jung aux tétraèdres finaux bien centrés dont `ab` est l'owner/diamètre : leur circumboule est alors la miniboule ; l'énoncé ne vaut pas pour une circumboule arbitraire. `7d94aee9` signale maintenant `J < 0` dans `run_pipeline`, mais le théorème exige `J > 0` : `J == 0` doit également être une violation. Les APIs directes de génération continuent en outre de traiter `J < 0` comme une mort avant le refus terminal. La gate `P/B` multiplie en i128 des termes pouvant approcher 156 bits sous u16 et n'est donc exacte que sur ses petits nuages actuels ; employer une arithmétique large signée ou un comparateur de produits borné. Le tri des rationnels reste fermé sans coût apparié face aux 8,8 % de complétions qui atteignent un scan court.

### V12 — nécessité et juge

**Oui** : aucun calcul exact du minimum sur le disque fermé n'est requis dans la lane produit. **Non** : fixtures et égalité de digest ne suffisent pas à qualifier le certificat. `635951d6` ajoute bien `x=(1000,1200,0)`, seed aigu auquel les sites réalisés donnent treize intérieurs. Mais F5 annonce 28 sites et n'en construit que 26 : les directions `(y,z)=(0,+-1000)` manquent. La propriété publiée « tout centre non nul a profondeur au moins 13 » ne suit plus pour une direction orthogonale à l'une des treize paires. Compléter les deux axes et asserter seed OFF/profondeur ; ne pas réécrire l'exemple canonique comme une fixture à 26. `mhgp5_anchor_tests_oracle` compare utilement ON/OFF, mais appelle les mêmes corps produit puis RLE avant comparaison : elle peut masquer les multiplicités brutes et n'énumère ni supports ni profondeurs par une autorité indépendante. Conserver cette porte comme différentiel borné et ajouter le petit juge structurel demandé.

### V13 — mou de l'histogramme

**Mécanisme structurel reçu, facteur numérique non contractualisé.** Les cônes W3 peuvent contenir des témoins hors des boîtes créditées ; cela explique le mou sans accuser `h_coeur`. Le rapport 2,33 reste une observation de famille, taille et pin, pas un invariant et pas une dispense de gate si l'histogramme change.

### V14 — hygiène de mesure

**Principe reçu, mesure exploratoire acceptée, reçu reproductible non fermé.** `259fe21e` place la variante finale dans un dossier distinct ; ses douze sorties ont des en-têtes cohérents avec `635951d6`, des identités de compteurs fermées, trois timers séparés et zéro contradiction observée. Le pin/dirty reste toutefois capturé à la configuration, sans commande, toolchain, hash binaire/entrées, code de sortie ni RSS ; la génération a aussi été observée non atomique. En Q4, W4 est préfiltré hors timers, donc « production Wq + secteurs » désigne seulement l'étage incrémental après W4 et aucun bras ON de bout en bout n'est chronométré. Le dossier historique commun à `a9a2f509` est restauré, mais trois fichiers plus récents y restent : remplacer « contenu historique exact » par cette portée. Les temps locaux demeurent non citables comme performance produit.

### Corrections immédiates conseillées

1. `635951d6` rend `wrong` bloquant et sépare les timers contrefactuel, K4+K8 et production ; `259fe21e` publie les sorties séparément. Distinguer encore la source des contradictions, chronométrer le bras ON complet et compléter le manifeste ; le marqueur dirty configure-time ne remplace ni hash du diff ni hash binaire.
2. `635951d6` ajoute le seed à F1/F3. F5 doit recevoir les deux axes manquants pour construire réellement 28 sites et préserver l'exemple 2.4. Remplacer F7 par le cas non coplanaire vérifié : ancre `(0,0,0)`–`(2000,0,0)`, supports `(1000,1000,+-1000)` et sept puis huit paires `(1000+e,+-900,0)`. Sept donne `wmin=7`, ON=OFF un candidat, 2 seeds et 30 complétions OFF ; huit donne `wmin=8`, ancre ON tuée, puis 2 seeds, 34 complétions et une mort en profondeur OFF. Compléter aussi la gate de `7d94aee9` par une comparaison brute et une profondeur indépendante ; lui rendre le double label `oracle gate`, faute de quoi `ctest -L gate` l'ignore.
3. Les compteurs W3/secteurs sont maintenant comparés et planchés. Ajouter `invariant_jneg`, rendre les planchers optionnels par porte et le ledger Q4 indépendant du seuil de routage.
4. `635951d6` remplace le booléen par `AnchorPretests`, mais `kAlreadyApplied` et `kCounterfactual` restent deux bypass publics équivalents. Préférer un corps interne et limiter le contrefactuel aux builds de test.
5. Refuser `J <= 0` à la source et remplacer les produits i128 de l'identité `P/B` par une arithmétique couvrant le profil u16. F6 sépare bien la frontière de demi-plan ; verrouiller les secteurs 4/5 si leur portée reste revendiquée.

### Mise à jour du protocole `EXTRA_N` (`d837adb2`)

Les extras présents sont bien soumis aux contrôles génériques, mais la phase
2 bis n'est pas encore recevable comme contrat de campagne :

- le validateur découvre les extras depuis les seuls `.status` présents et ne
  connaît pas le plan `EXTRA_N/EXTRA_FAMILIES` demandé ; une omission peut donc
  laisser `campaign_status=complete` ;
- il ne lie pas `famille=` et `n=` du corps au nom du fichier. Un reçu 50 k
  copié sous un nom 100 k serait accepté ;
- le scénario négatif `FAIL_FAMILY=eight_clusters` fait déjà échouer le contrat
  obligatoire 50 k et ne prouve donc pas la détection de l'extra seul ;
- les valeurs sont interpolées dans la commande SSH sans validation, puis
  soumises au découpage/globbing shell ; `N=50000`, doublons et entrées
  malformées peuvent écraser ou détourner un reçu ;
- l'exemple 100 k/200 k sur deux familles autorise quatre timeouts de 7200 s
  avant la phase GPU obligatoire, dans une session bornée à 14400 s.

Valider et normaliser le plan localement, le graver avec l'argv, le transmettre
explicitement au validateur et placer ces extensions après les phases
obligatoires, sous un budget global. Leur statut reste « mesure CPU
exploratoire avec digest », pas « conformité v4 à 100 k/200 k ».

### Lanceur G4 au tip courant

`ef5abbd5` corrige le blocage initial de l'auto-copie : la première invocation
calcule la racine avant `exec` et la transmet à la copie. La fermeture du chemin
local reste toutefois incomplète.

Fermer dans la même passe quatre trous de sécurité : installer le trap avant le
démarrage, rendre son logging best-effort afin qu'un échec de `tee` ne précède
jamais l'arrêt, et épingler les scripts `set_max_run_duration_and_verify.sh`,
`start_and_verify.sh` et `stop_and_verify.sh`, actuellement relus depuis le
worktree vivant. Refuser aussi une valeur ambiante qui désactive l'auto-copie
ou injecte une racine sans contrôle, vérifier que la copie exécutée est l'octet
hashé par le manifeste, et supprimer la copie temporaire. Le
selftest actuel n'exécute pas l'orchestrateur et ne peut détecter aucun de ces
défauts.

Ne pas lancer de nouvelle campagne avant fermeture de ces garde-fous. La
correction de racine ne requalifie pas rétroactivement la session calculée
depuis le script vivant.

Mise à jour de réception : la session source `fa99b3f1` a atteint la ligne
finale de ses 25 runs, puis l'édition du script vivant a produit
`session_rc=2` avant l'enregistrement de `remote_campaign_rc`. Après une
première tentative échouée fermée, la récupération gardée a localement copié 75
fichiers avec `scp_rc=0`, puis vérifié `ehgp-blackwell-spot-ai1a` `TERMINATED`.
Les artefacts suivis par `e11ad8c7` annoncent en CPU/GPU à 50 k `78/81`,
`21/25`, `162/174` et `23/24` s sur
uniform/terrain/eight_clusters/scanline. Les 25 statuts partagent les trois
pins, sont `finished=1`, portent 24 codes 0 et le code 4 attendu du mutant ; les
deux digests CPU/GPU/adaptatif concordent.

Le statut strict reste moins fort que sa prose : le commit suit seulement les
25 `.status`, les 25 `.txt` et `RECU.txt`. Les logs de session/récupération et
les 25 `.status.time` sont ignorés. Le validateur exact extrait de `fa99b3f1`,
rejoué avec les arguments de récupération `0 0`, rend bien
`campaign_status=complete (25 runs valides)`. Le contenu est donc validé, mais
le reçu n'est pas transactionnellement autoportant : sa prose ne conserve ni
commande, arguments, `validator_rc` ni sortie, `remote_campaign_rc` est inconnu
et `scp_rc=0` n'existe que dans le log local ignoré. Conserver le transcript,
noter le RC distant comme inféré et corriger « 24 runs tous code 0 » ; ne pas
relancer les calculs. Le mini-script doit aussi refuser explicitement
`scp_rc != 0` avant d'écrire `DONE`.

`GPU.md` à `77e143b2` a anticipé cette récupération ; `e11ad8c7` fournit
désormais la pièce. Les quatre murs établissent seulement que le chemin
`--gpu` reste plus lent de +4 % à +19 % sur cette exécution. Ils ne suffisent
donc pas à appeler tous ces écarts « parité » ni à attribuer le rapprochement
aux seuls tests d'ancre. Les lignes de phase donnent les rectangles q3/q4, pas
une séparation interne cover/scan : le diagnostic sur les covers reste ouvert.

### Conseil sur la sonde de corde Q4 en cours

Les centres des complétions acceptables sont contenus dans la corde fermée
`|mu| <= sqrt(J/2)` ; ils ne forment pas tout le continuum. Son découpage en
`K` intervalles est un certificat suffisant cohérent : `h4` témoins stricts aux
deux extrémités de chaque intervalle tuent le seed. Les facteurs `J/2` et
`P-mu*B` et les comparaisons `< 0` sont corrects. L'arrondi entier courant
`isqrt(J/2)+1` reste fail-open, mais surestime inutilement un carré parfait et
peut biaiser fortement un petit `J` ; employer le plus petit `r` tel que
`2*r*r >= J` ou publier ce mou. Avant de lire ses ratios ou d'en faire une
lane :

- appliquer à `mhgp5_q4_chord_probe` le pin/dirty actuellement réservé à
  `mhgp5_rect_probe`, puis imprimer toute la configuration et recevoir la
  commande, le binaire, le temps et la mémoire ;
- rendre bloquants des planchers d'ancres, seeds vivants, morts additionnels et
  scans réellement évités ; le code 0 actuel accepte une sonde entièrement
  vide ;
- conserver `wrong` comme alarme géométrique **relative**, pas comme oracle
  indépendant : le set des `BallKey` produites détecte utilement une sphère
  émise qu'un seed dit mort peut engendrer, mais une omission du même corps peut
  masquer un défaut et une cosphère portée ailleurs peut produire un signal
  conservateur. Restreindre le nouveau contrôle aux seeds `!dead1`, puis faire
  une vraie comparaison ON/OFF brute et post-RLE avant intégration ;
- séparer `wrong_K`, imposer par seed `dead2 => dead4 => dead8`, refuser
  `J <= 0`, vérifier idéalement `3J >= G*D2` et borner les agrégats `u64` ;
- renommer le compteur courant en « tentatives `y` évitées » : il est incrémenté
  avant owner, canonicalisation et préfiltres, donc ne mesure ni les scans de
  profondeur évités ni un gain temporel. Ventiler les étages et comparer le
  coût propre des `K` scans de cover au travail réellement retiré ;
- remplacer `atoi` par un parse borné, refuser `n <= 0`, vérifier le cardinal
  effectivement généré et imprimer `coord`, seed, `s`, `smax`, `h4`, filtre
  flottant, pin et état du worktree ; `--n=abc` rend aujourd'hui un run vide à
  code 0 ;
- écrire chaque brut vers un temporaire, bloquer sur son RC puis renommer. La
  chaîne locale en cours emploie `;`, écrit directement le `.txt` et poursuivrait
  après un code 1 ; un fichier zéro octet a déjà été visible pendant le calcul.

Cette sonde est une bonne prochaine falsification si elle reste distincte du
chemin produit et si son coût propre est comparé au scan qu'elle prétend éviter.
La compilation Release ciblée et cinq runs bornés (`n=200/400`, quatre
familles) donnent `wrong=0` et des morts agrégées monotones `K=2/4/8` ; ce sont
des observations exploratoires, pas un reçu.
