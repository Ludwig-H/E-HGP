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

Session G4 gardée mesurant la nouvelle lane CPU (et les contrats `--gpu` inchangés, pour l'égalité des deux digests) ; aucune écriture de kernel par rectangle avant votre réponse sur V8.

## Réponse des auditeurs — 27 août 2026

- **Relecture :** `a9a2f509` pour l'activation initiale, `fa99b3f1` pour les analyses, `7eb33608` pour F4 et le contrefactuel, `d837adb2` pour `EXTRA_N`, puis `7d94aee9` pour la gate ON/OFF, les compteurs et l'invariant J
- **Nature :** revue mathématique et statique ; aucun test ni GCP lancé par les auditeurs dans cette passe ; les sorties 165/165 de `7eb33608` sont des résultats concurrents observés, pas un rejeu du tip
- **Verdict borné :** les preuves de suffisance W3 et secteurs sont reçues au niveau statique sous le profil u16. Aucun faux rejet nominal n'a été trouvé. `7d94aee9` ajoute une comparaison ON/OFF post-RLE et des planchers sectoriels utiles, mais ne qualifie pas encore l'activation : la porte réutilise les corps produit, n'énumère pas indépendamment les profondeurs et les reçus courants restent mal épinglés.

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

Les anciens reçus restent immuables et leurs digests demeurent des contre-fixtures ; ne pas réécrire leurs compteurs. Il n'est pas nécessaire de reproduire toute l'histoire avant un premier probe, mais toute nouvelle conclusion de performance à 8 k–50 k exige un reçu distinct au pin courant. La prochaine campagne G4 peut donc être la campagne de décision actuelle, avec montée 16 k, 32 k puis 50 k et arrêt anticipé sur échec de capacité.

### V8 — priorité GPU

**Oui.** Subordonner tout kernel q3 par rectangle à une mesure appariée de la nouvelle CPU et du chemin `--gpu` courant, au même pin G4, mêmes entrées, mêmes fils et deux digests. La décision ne doit pas dépendre d'un seuil isolé de 20 s : recevoir les murs par phase, la variabilité, RSS/VRAM et le résidu réellement device. Si q3 n'offre plus de marge nette et reproductible, fermer q3 comme voie de gain et profiler q4 séparément.

### V9 — W3 avant ou après cover

**Pas d'objection à une décision par mesure appariée, avec une seule forme par défaut.** Conserver après-cover comme référence tant que le bras `cover_query coef 1` n'a pas un reçu : il ajoute une traversée d'arbre par ancre et peut gagner sur dense tout en perdant sur uniforme. Mesurer visites, temps de cover, mémoire et objet ; ne jamais sélectionner par nom de famille.

### V10 — test cellulaire

**Dérivation reçue, lane refusée à ce stade.** Le bornage cellulaire peut rester un prototype de falsification si, après les filtres actuels, les ancres résiduelles dominent encore. Exiger d'abord une porte arithmétique et un modèle de coût complet ; le nombre d'évaluations seul ne décide pas. Une grille cellulaire et un fan sectoriel à `K > 8` sont deux certificats distincts : ne pas appeler la première un raffinement automatique du second.

### V11 — q4

**Identités reçues statiquement, chantier de tri non ouvert.** La borne positive sur J et l'identité de signe sont cohérentes. Pour le rayon sectoriel Q4, borner explicitement Jung aux tétraèdres finaux bien centrés dont `ab` est l'owner/diamètre : leur circumboule est alors la miniboule ; l'énoncé ne vaut pas pour une circumboule arbitraire. `7d94aee9` signale maintenant `J < 0` dans `run_pipeline`, mais le théorème exige `J > 0` : `J == 0` doit également être une violation. Les APIs directes de génération continuent en outre de traiter `J < 0` comme une mort avant le refus terminal. La gate `P/B` multiplie en i128 des termes pouvant approcher 156 bits sous u16 et n'est donc exacte que sur ses petits nuages actuels ; employer une arithmétique large signée ou un comparateur de produits borné. Le tri des rationnels reste fermé sans coût apparié face aux 8,8 % de complétions qui atteignent un scan court.

### V12 — nécessité et juge

**Oui** : aucun calcul exact du minimum sur le disque fermé n'est requis dans la lane produit. **Non** : fixtures et égalité de digest ne suffisent pas à qualifier le certificat. L'exemple 2.4 publié est lui-même vacu côté produit : ses 28 points ont `q=0`, donc aucun n'est un seed aigu. Ajouter `x=(1000,1200,0)` conserve la profondeur nulle au centre, réalise un seed non nul et lui donne au moins 13 intérieurs. La nouvelle `mhgp5_anchor_tests_oracle` compare utilement ON/OFF, mais appelle les mêmes corps produit puis RLE avant comparaison : elle peut masquer les multiplicités brutes et n'énumère ni supports ni profondeurs par une autorité indépendante. Conserver cette porte comme différentiel borné et ajouter le petit juge structurel demandé. Ces extensions peuvent éviter l'arrangement continu en `O(m^2 log m)`.

### V13 — mou de l'histogramme

**Mécanisme structurel reçu, facteur numérique non contractualisé.** Les cônes W3 peuvent contenir des témoins hors des boîtes créditées ; cela explique le mou sans accuser `h_coeur`. Le rapport 2,33 reste une observation de famille, taille et pin, pas un invariant et pas une dispense de gate si l'histogramme change.

### V14 — hygiène de mesure

**Principe reçu, correction du reçu refusée en l'état.** Le `LISEZMOI` versionné par `a9a2f509` attribuait aux bruts historiques des valeurs absentes. `7eb33608` ajoute bien un contrefactuel filtre OFF et régénère les douze sorties, mais les réécrit dans le même dossier et toutes affichent `pin_execution=fa99b3f1` : CMake capture HEAD à la configuration, avant le diff devenu `7eb33608`, sans hash du worktree ni du binaire. Le résumé conserve aussi `HEAD 312034ce + sonde`. Restaurer au tip le contenu historique exact de ce dossier et placer la variante finale dans un nouveau reçu complet, après commit/reconfiguration. Les temps locaux `33,0 -> 13,7 s` restent non reçus. En revanche, la suite canonique a été rejouée directement : `7eb33608` passe 165/165 gates, pas les 168 annoncées dans le message précédent.

### Corrections immédiates conseillées

1. Le worktree rend maintenant `wrong` bloquant et sépare les timers contrefactuel, K4+K8 et production. Conserver ces corrections, distinguer encore la source des contradictions et publier les sorties dans un nouveau reçu correctement épinglé ; le marqueur dirty configure-time ne remplace ni hash du diff ni hash binaire.
2. Le worktree ajoute le seed à F1/F3 et une F5 non vacue. Corriger son cardinal annoncé : six quadruplets plus une paire font 26 sites, pas 28. Compléter la gate de `7d94aee9` par une comparaison brute, une profondeur indépendante et une fixture sectorielle Q4 avec seed OFF ; lui rendre aussi le double label `oracle gate`, faute de quoi `ctest -L gate` l'ignore.
3. Les compteurs W3/secteurs sont maintenant comparés et planchés. Ajouter `invariant_jneg`, rendre les planchers optionnels par porte et le ledger Q4 indépendant du seuil de routage.
4. Le worktree remplace le booléen par `AnchorPretests`, mais `kAlreadyApplied` et `kCounterfactual` restent deux bypass publics équivalents. Préférer un corps interne et limiter le contrefactuel aux builds de test.
5. Refuser `J <= 0` à la source et remplacer les produits i128 de l'identité `P/B` par une arithmétique couvrant le profil u16. Le commentaire 25/37 et la frontière de demi-plan sont corrigés dans le worktree ; en revanche F7 reste vacue pour Q4, car son nuage F1 est coplanaire et ne contient aucune complétion Q4.

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

### Alerte sur le worktree post-`7d94aee9`

Le mécanisme d'auto-copie ajouté à
`session_campagne_v5_scale_g4.sh` doit être corrigé avant commit : après
`exec bash /tmp/ehgp-v5session-copy...`, `BASH_SOURCE[0]` désigne la copie dans
`/tmp`. Le calcul courant `dirname(BASH_SOURCE[0])/..` fixe donc `REPO_ROOT` à
`/`, et les chemins relatifs `gcp-migration/...` ne peuvent plus fonctionner.
Calculer/graver le dépôt source **avant** l'`exec`, le passer dans une variable
dédiée et le vérifier dans la copie. Cette correction future ne requalifie pas
la session déjà lancée depuis le script vivant.

La session G4 gardée peut continuer pour la décision V8. Ces mesures ne ferment toutefois V7/V12 que si elles sont épinglées au code effectivement mesuré et accompagnées des autorités indépendantes ci-dessus.
