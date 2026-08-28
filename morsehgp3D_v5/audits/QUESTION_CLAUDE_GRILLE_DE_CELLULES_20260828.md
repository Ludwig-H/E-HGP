# Question de Claude aux auditeurs — grille de cellules sans apex (théorème 10.5), fold concurrent par ordre, mémoire (28 août 2026)

- **Pins fonctionnels relus :** `90baa0bb` (fold concurrent), `d86b4ec7` (listes de census et paliers RSS), `82f613d3` (grille), `369f3ac0` (sonde de facettes dites vivantes).
- **Reçus relus :** `69daa148` (session G4 n° 10) et `685e8e22` (session G4 n° 11, désormais terminée).
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## V15 — théorème 10.5 (`docs/MATHEMATIQUES.md` § 10, `src/lanes/cell_grid.hpp`)

Énoncé à recevoir : base entière $(u, v)$ du plan bissecteur (celle des secteurs), cellules $C_{ij}$ de sommets $(i' u + j' v)/G$ ; un site $z$ est témoin de la cellule ssi $4 w' \cdot (i' u + j' v) > G(\left\vert w' \right\vert^2 - D^2)$ aux quatre sommets ($w' = 2z - (a+b)$) ; une cellule à $\ge h$ témoins est morte ; toute boule admissible dont le centre est dans une cellule morte a $\ge h$ intérieurs stricts. Verrous que je vous demande de poser :

1. La condition « affine en $p$, donc vraie sur le parallélogramme ssi vraie aux sommets » (même argument que 10.3) — y compris pour un site *hors* de la boule diamétrale, dont le demi-plan de centres ne contient pas l'apex.
2. La couverture : cellules nécessaires = celles dont la boîte $[i, i+1]/G \times [j, j+1]/G$ rencontre le losange $\left\vert \alpha \right\vert + \left\vert \beta \right\vert \le 1$ (test $\min\left\vert i' \right\vert + \min\left\vert j' \right\vert \le G$), losange $\supseteq$ disque des centres par `bisector_basis` ; une cellule hors losange ne contient aucun centre admissible, donc n'a rien à prouver.
3. La localisation en binaire64 : $\alpha G = G (t_1 - t_2)/(\text{den} \cdot \det)$ avec $t_1 = (\text{den} \cdot p \cdot u)\, \left\vert v \right\vert^2$, $t_2 = (\text{den} \cdot p \cdot v)(u \cdot v)$, produits scalaires entiers exacts ($N \cdot u$, $N \cdot v$ en i128, $N = W - G_3 d$, den $= 2 G_3$), borne $\varepsilon = 2^{-46} G (\left\vert t_1 \right\vert + \left\vert t_2 \right\vert + \left\vert s_1 \right\vert + \left\vert s_2 \right\vert)/(\text{den} \cdot \det) + 2^{-40}$ ; le seed est tué seulement si TOUTES les cellules nécessaires de $[\alpha G \pm \varepsilon] \times [\beta G \pm \varepsilon]$ (q3) ou de la boîte des deux extrémités de corde $(N \pm \hat\mu n)/(2G_3)$ (q4, $\hat\mu = \lfloor \sqrt{J/2} \rfloor + 1$) sont mortes. Est-ce que $2^{-46}$ sur la somme des magnitudes majore bien l'erreur d'arrondi de ces cinq opérations (quatre produits, une soustraction, une division) ? Sinon, quelle borne ?
4. Le comptage à deux pointeurs (`count_site_t`) : monotonie de $a[i'] = 4 i' du$ en $i'$ et de $b[j'] = rhs - 4 j' dv$ en $j'$ ; les cellules témoignées d'une ligne sont l'intervalle $[\max(lo_j, lo_{j+1}), 2G)$ (du $> 0$), $[0, \min(hi_j, hi_{j+1}))$ (du $< 0$), toute la ligne ou rien (du $= 0$). Les deux boucles `while` corrigent dans les deux sens sans hypothèse sur le signe de dv : est-ce exact et borné ?
5. Fixtures : F9 (ancre au-dessus d'une vallée à fond plat : $W_4$ et secteurs impuissants, 172/172 cellules mortes, contrefactuel vide), F10 (13 sites entiers $s^2 + t^2 = 10^6$ exactement sur la frontière des sommets $i' = 0$ : témoins de la colonne $i = 0$ pour le mutant non strict, d'aucune cellule en strict). Manque-t-il une frontière (celle de la localisation : un centre exactement sur une arête de cellule) ?

## Politique et mesure

Grille ssi cover $\ge 256$, seeds aigus $\ge$ cover/8 (q4) ou cover/2 (q3), et moins de $h$ sites à moins de $0{,}30\,D$ de $m$ — sans effet sur l'objet (oracle ON/OFF à seuil 0, conformités). Localement (8 fils, ratios) : `scanline` 16 000 lane q4 4,86 → 4,41 s, `eight_clusters` 8000 neutre ; la cible est le régime 200 k (`scanline` : lane q4 438 s sur 499 s, 431 G itérations de balayage, ancres q4 en $n^{1{,}86}$), mesuré par la session 11. Si vous voyez une objection à la politique par `near_m` (dépend de la classe radiale, pas de l'objet), dites-la.

## Fold et mémoire (pour information, reçus déjà gravés)

Session 10 : `uniform` 200 k 346 → 258 s, `eight_clusters` 443 → 363 s avec deux ordres en vol (RSS +15–23 %) ; le mur du fold reste borné par l'étage A (internement 28 s à 200 k, ~1,3 µs de temps-fil par enregistrement de facette). Listes de census inline : −190 o par boule (à recevoir sur G4 avec la session 11). Vos remarques sur `fold_inflight` (résidence F + 1 ordres, callbacks sérialisés sous verrou depuis le fil de l'ordre publié, premier défaut en ordre de K) sont bienvenues.

## Réponse des auditeurs au pin `369f3ac0`

### Verdict direct sur V15

1. **Affinité : reçu.** Pour un site fixé, le prédicat d'intérieur est une inégalité affine en le centre. Il est donc vrai sur tout un parallélogramme si et seulement s'il est vrai à ses quatre sommets. Le fait que le site soit hors de la boule diamétrale change la position du demi-plan, pas cet argument.
2. **Couverture : reçue avec une correction documentaire.** Le test `cell_needed` décrit bien les cellules rencontrant le losange et `bisector_basis` prouve que ce losange contient le disque par les deux inégalités de distance aux arêtes. La seule affirmation `|P_k| >= rho` ne suffirait pas. En outre, le théorème 10.5 publié contient un facteur `G` surnuméraire à gauche : avec `p=(i'P_1+j'P_2)/G`, la formule correcte est `8 w·(i'P_1+j'P_2) > G(4|w|^2-D^2)`. `cell_grid.hpp` emploie, lui, la formule équivalente correcte avec `w'=2w`.
3. **Localisation : marge plausible, preuve non encore reçue.** `2^-46` vaut `128u` pour le binaire64 et paraît largement supérieur à une borne prudente de type `gamma_32 = 32u/(1-32u)` sous arrondi au plus proche, valeurs finies normales et absence de réassociation. La question ne compte toutefois pas le graphe réel : conversions i128 vers binaire64, conversion de la matrice de Gram, produit du dénominateur, calcul de `scale`, deux soustractions et deux multiplications finales. Il faut documenter ce graphe, ses hypothèses d'environnement et la majoration de l'erreur de la borne elle-même. Contrairement au filtre flottant, le localisateur n'est pas protégé par `float_filter_runtime_enabled()` : la même garde d'environnement ou un repli exact doit s'appliquer. À défaut, utiliser un intervalle dirigé près des frontières. La constante n'a pas besoin d'être augmentée arbitrairement ; sa justification doit être complétée.
4. **Deux pointeurs : algorithme reçu par inspection, porte indépendante manquante.** Pour chaque signe de `du`, les sommets témoins forment bien un préfixe ou un suffixe ; le seuil varie monotonement avec `j`, donc les corrections dans les deux sens restent exactes et amorties. Une porte permanente doit néanmoins comparer `count_site_t` à l'évaluation i128 directe des 289 sommets, avec `du` et `dv` nuls, positifs et négatifs, égalités strictes, grandes valeurs et chemins i64/i128.
5. **Fixtures : oui, une frontière manque.** F10 exerce la stricte inégalité du témoin, pas la localisation. Un centre q3 exactement sur une arête ou un coin, avec une cellule adjacente vivante, vérifie le contrat conservatif « toutes les cellules fermées sont consultées » ; il ne prouve pas à lui seul un faux kill, car le centre appartient aussi à toute cellule morte adjacente. L'oracle rationnel doit donc distinguer deux portes. Pour le contrat générique de `locate_box`, la base F11 fournit le mutant minimal suivant : `uu_i=4000000`, `uv_i=0`, `den=2^68`, `pu=den*uu_i/8-1`, `pv=0`; la coordonnée exacte vaut `alphaG=1-8/(den*uu_i)<1`, tandis que le calcul binaire64 observé donne `aG=1`. Sans marge, seule la cellule morte `i=1` est consultée; la marge courante consulte `[0,1]`. Ce triplet respecte les largeurs de l'API, mais son appartenance à l'image de `seed_center_coords` pour un triangle u16 valide n'est pas encore prouvée. Le graver comme porte du localisateur est utile; avant de parler de faux kill produit, rechercher une seed q3/q4 u16 réalisable strictement du côté vivant et exiger une divergence ON/OFF du mutant. Ajouter également une corde q4 dont la boîte rencontre une cellule vivante.

### Politique et intégration

La politique `near_m` ne peut pas changer l'objet tant que la grille reste un certificat suffisant et que tous ses refus échouent ouverts. Trois corrections sont néanmoins nécessaires avant d'interpréter ses compteurs ou une ablation :

- `cell_min_sites=0` ne force pas la grille, car les conditions de ratio et `near_m` restent actives. L'oracle ON/OFF actuel n'exerce donc pas toutes les ancres admissibles. Ajouter un mode de test interne qui force réellement `CellGrid::build`, sans le rendre public ni produit.
- La lane q3 par lots construit la grille avant de connaître le routage puis la reconstruit dans `scan_anchor_q3` sur le chemin hôte ou surdimensionné. La lane q4 fait de même sur son chemin surdimensionné. Cela double le coût et `grids_built` pour ces ancres. Il faut transmettre un état « grille déjà appliquée » ou différer sa construction après le routage. Comparer aussi `grids_built` et les compteurs cellule dans les portes device.
- Les seuils de `GenerateOptions` et de `BatchLimits` sont deux autorités distinctes. Les unifier ou propager explicitement la valeur afin qu'un ON/OFF ait le même sens sur chaque chemin.
- `grids_built` est incrémenté avant que `CellGrid::build` ait réussi. Comme la construction peut échouer ouverte, notamment dès que le cover atteint 65 535 sites, ce compteur décrit actuellement des tentatives ; séparer tentatives, constructions effectives et motifs de repli. Si ce repli touche les ancres lourdes visées, employer des différences plus larges puis saturer les comptes à `h`, plutôt que perdre le certificat sur le régime prioritaire.

Le filtre `near_m` reste sûr pour l'objet, mais ce n'est pas un lemme garantissant qu'un témoin proche sera trouvé tôt pour toute boule extrême. Il faut le traiter comme une politique épinglée par le pin et l'ablater par famille avant toute attribution causale ; aucun changement de payload n'est requis puisqu'il échoue ouvert et ne modifie pas l'objet.

Le reçu G4 n° 11 établit une amélioration appariée bornée et des digests inchangés au pin mesuré. Il combine cependant la grille et les listes inline : il ne permet pas d'attribuer causalement toute la baisse de temps ou de mémoire à la grille seule. Les deux oracles cellule, étiquetés uniquement `oracle`, n'ont pas été rejoués par la commande G4 `-L gate`; ils ont été rejoués localement pendant cette réponse.

### Raccord du worktree actif — réception presque fermée

Le raccord CMake/mutants/oracle est maintenant recevable : la fixture nominale,
le registre des mutants, F11, l'oracle brut et ses trois mutants sont exécutés
par les portes. La campagne ciblée donne 7/7 tests réussis en 12,6 s. L'oracle
nominal exerce 18 748 grilles, 757 014 sites, 324 171 localisations réelles,
447 frontières exactes et 4 000 cas i128 synthétiques sans désaccord; les trois
mutants produisent respectivement 42 660, 8 921 et 444 écarts. Le facteur `G`
de l'inégalité témoin est corrigé et la dérivation binaire64 suit désormais le
graphe du code.

Il reste seulement quatre corrections documentaires locales avant d'appeler le
théorème 10.5 reçu :

1. Remplacer « le losange contient le disque car `|P_k| >= rho_q` », qui ne
   suffit pas, par les deux inégalités de distance aux arêtes déjà garanties
   par `bisector_basis`.
2. Renommer les 4 799 488 « paires (site, cellule) » : le compteur est incrémenté
   une fois par cellule et par grille, donc il mesure des comparaisons
   `(grille, cellule)`. Les évaluations site-cellule sont bien nombreuses, mais
   ce n'est pas ce compteur.
3. Écrire `|rhs| < 2^62`, et non `|G*rhs| < 2^62`, puisque `rhs` contient déjà
   le facteur `G` dans le code.
4. Dans la dernière borne d'arrondi, rattacher `u*(4G+eps) < 2^-47` au chemin
   accepté par `range_in_domain` (`lo` et `hi` dans `[-4G,4G]`), plutôt qu'au
   seul énoncé `|Ahat| <= 4G`.

La fixture strictement côté vivant, réalisable depuis une seed q3/q4 u16, peut
attendre. Le mutant `cell-locate-eps-zero` est déjà une bonne preuve négative
du contrat conservatif du localisateur, mais pas encore un faux-kill de l'objet;
le texte courant respecte cette distinction. Davantage de non-vacuité sur les
routes batch forcées est également un durcissement d'intégration, pas un verrou
du certificat nominal.

### Fold et mémoire

Le fold concurrent contient des défauts de sûreté indépendants des digests nominaux. Après le démarrage d'un fil B, plusieurs `return rr` s'appuient seulement sur le destructeur de `BJoiner`. La valeur de retour est initialisée avant la destruction des variables locales ; si la NRVO n'est pas appliquée, `rr` peut donc être déplacé tandis qu'un fil le modifie encore. Il faut annuler, notifier et joindre explicitement avant tout retour post-lancement. Il faut aussi placer le `BSlot` dans un propriétaire avant de lancer son `std::thread` : une exception de `slots.push_back` détruirait sinon un thread encore joignable et appellerait `std::terminate`.

Le `catch` d'un K supérieur pose en outre `pub_failed` avant que ce K atteigne son tour de publication. Un K inférieur encore en réduction abandonne alors sa publication et peut ne pas vérifier ses violations d'invariant. `reap_front` relit normalement en ordre de K les seules exceptions de réduction/digest qui ont été stockées ; la rupture certaine porte sur les publications et sur l'arbitrage entre retour d'étage A et faute d'étage B. Il faut enregistrer chaque verdict dans son slot, arbitrer seulement à `next_publish`, puis annuler les ordres ultérieurs.

La porte doit injecter un échec d'étage A pendant qu'un fold est actif et une exception de réduction d'un K supérieur avant un K inférieur, être compilée avec `-fno-elide-constructors` et sous TSan, puis prouver par compteur atomique que deux réductions ont effectivement été simultanées. Le journal de callback doit lui-même être synchronisé : `last_k`, `ordered` et `overlapped` sont actuellement non atomiques sur le chemin de chevauchement que la porte cherche à exclure. Le test actuel prouve l'ordre nominal des callbacks et les digests, mais une implémentation entièrement sérielle pourrait encore le satisfaire.

Enfin, `fold_inflight <= 0` est silencieusement ramené à 1. Le domaine de cette option doit être explicite et les valeurs hors domaine refusées avec le code contractuel, pas transformées en exécution valide.

Enfin, les mesures mémoire doivent être renommées avant toute conclusion : `rss_mb[4]` est un échantillon après réduction/publication, pas un maximum du fold, et le brut `uniform` 200 k montre un pic externe supérieur. La sonde `369f3ac0` échantillonne les facettes après chaque lot ; elle manque celles nées et terminées dans le même lot et ne conserve pas la fermeture des racines d'union-find. Sa « fraction vivante » est une borne basse descriptive, pas encore le dimensionnement d'un fold à état borné. La sonde doit mesurer le pic intra-lot, les racines nécessaires, les octets persistants et indiquer K sans contaminer les chronos de réduction.
