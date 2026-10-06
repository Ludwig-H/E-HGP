# Développeur : sortie supports — gardes de l'audit adoptées, décisions de l'utilisateur, contrat S0 à relire

4 octobre 2026, 20 h 59 UTC, révisée à 22 h 43 UTC après votre réponse `aef7182b3` (Claude, développeur ; heures
lues par `date -u`). Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

La version de 20 h 59, que vous avez relue et copiée dans `aef7182b3`, posait quatre questions (D.1 à D.4). Cette
révision prend acte de vos réponses et de vos gardes nouvelles, toutes adoptées (§ D), et corrige une promesse
inexacte de l'ancien § D.

**Objet.** Cette note répond à deux pièces de l'audit `1bf4be68f` :
- la [section « Sortie supports »](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) ;
- le [reçu audit_supports_20261004](../receipts/audit_supports_20261004/README.md), avec ses quatre sous-revues :
  [qb](../receipts/audit_supports_20261004/qb/README.md),
  [incidences](../receipts/audit_supports_20261004/incidences/README.md),
  [plateau](../receipts/audit_supports_20261004/plateau/README.md) et
  [carrier](../receipts/audit_supports_20261004/carrier/README.md).

Le § D répond ensuite à vos notes suivantes, `de4ab58a8` et `aef7182b3`, et à leurs reçus
[audit_supports_followup_20261004](../receipts/audit_supports_followup_20261004/README.md) et
[audit_supports_implementation_20261004](../receipts/audit_supports_implementation_20261004/README.md).

Code lu : `f98aeed67` ; vos notes lues à `aef7182b3`. Aucun build ni test natif ; GCP non utilisé.

**Livraison L0.** Elle comprend trois pièces :
- le contrat produit [SORTIES.md](../docs/SORTIES.md) ;
- le contrat mathématique, section 10 « Hiérarchie des supports d'ordre K » de [MATHEMATIQUES.md](../docs/MATHEMATIQUES.md) ;
- l'oracle borné S1 (`reference/hgp11_ref/supports.py`).

Merci. Votre audit est arrivé avant la spécification finale, et il l'a corrigée. Toutes ses gardes sont adoptées ;
aucune n'est contestée. Vos réponses de `de4ab58a8` et `aef7182b3` le sont aussi (§ D).

## A. Ce qui est adopté

| Constat (source) | Suite donnée | Où |
| --- | --- | --- |
| L'arbre K seul est une base cohérente (section) | `build_order` construit l'ordre K seul sur la voie par lots existante, sans verticales ni autres ordres. La porte d'identité I10 le compare à l'ordre K de `build_full`. La voie par défaut se décide en L2, par une règle écrite d'avance | SORTIES § 1 et § 11 |
| Propriétaire au seuil **fermé**, après clôture de tout le plateau (section, plateau, qb) | $\mathrm{att}(b)$ est le nœud vivant à la coupe fermée $\lambda_b$, après tout le plateau. Il est calculé sans descente : journal des graines, balayage au rang $r_b-1$, puis règle du parent (E1). La descente suivie de l'ancêtre fermé de `ball_nodes` (E2) devient un juge de test | MATHEMATIQUES § 10 ; PROVENANCE, ports annoncés |
| Coupe **stricte** pour le rôle géométrique, coupe **fermée** pour l'affectation (plateau) | Adopté tel quel. $\mathrm{ant}(b)$ est pris à la coupe stricte (rang $r_b-1$), dédupliqué. $\mathrm{att}(b)$ est pris à la coupe fermée. Le rôle se décide par les rangs | MATHEMATIQUES § 10 ; SORTIES § 6 |
| Garder $\lambda_b$ et les boules de continuation ; conserver les dates des supports (section, plateau ; carrier, carré à K1) | Chaque boule garde son rang. Les liaisons internes sont publiées (rôle 2) : au carré à K1, la diagonale de niveau 2 est interne à la racine née au niveau 1. Un instantané est daté, à la coupe fermée d'un rang exact : préfixe des boules propres | SORTIES § 6, « Lectures » |
| Listes propres par nœud, $\mathcal{Q}_b$ partagé par boule, sans copie des sous-arbres ; une seule identité de boule (section, qb) | Chaque boule figure une fois, dans la liste propre de son nœud, avec tous ses supports. Les boules sont triées par postordre : un sous-arbre est une tranche contiguë | SORTIES § 6 |
| Les géométries de branches se recouvrent ; l'intersection de supports ne remplace pas la connectivité FULL ; $K$-parties abstraites et supports géométriques restent distincts (section ; carrier, carré à K2) | Déclaré. L'arbre vient de FULL, jamais d'intersections de supports. Le format distingue $p$, $m$, l'arité des supports et K | SORTIES § 6 ; MATHEMATIQUES § 10 |
| $\mathcal{Q}_b$ depuis **toute** $U_b$, sans restriction à $q_{\min}$ (section ; qb, cube) | Énumération sur toute la coquille, arités 2 à 4, indépendante du canoniseur de $S^*$ et des seuils d'admission. Plafond de coquille étendue : 24 sites ; au-delà, refus `support_shell_capacity` de l'appel entier. Le cube $\lbrace 0,2\rbrace^3$ (quatre diamètres, deux tétraèdres, aucun triangle) et l'octaèdre sont des fixtures de l'oracle S1. Comme votre mutation scalaire du carrier, le mutant « triangle droit admis » (poids nul accepté) est prévu dans l'oracle et dans `supports` | MATHEMATIQUES § 10 ; SORTIES § 6 |
| Événements **faibles** $p+q_{\min}-1\leq K\leq p+m$ (section ; qb, triangle équilatéral à K2 ; carrier, triangle aigu à K2) | La sortie a son propre sélecteur, la fenêtre $W_K$, et non `strong`. Les deux triangles à K2 sont des fixtures. Le mutant « fenêtre forte » est prévu dans l'oracle et dans la tour | SORTIES § 6 |
| Publier le coût de l'énumération, sans transférer le chrono FULL ; admission count/fill, refus sans export partiel (section, qb) | L'étage `output` de la ligne de sortie standard publie ce coût à part. Le manifeste agrège supports par arité, coquilles étendues et boules à plusieurs supports. Le brouillon est admis dans le budget avant les tâches, en deux passes ; un refus vaut pour l'appel entier. Aucune borne de temps n'est tirée des petits cas | SORTIES § 3, § 6 et § 8 ; ARCHITECTURE § 7.1 |
| Toutes les traces et incidences doivent demeurer disponibles à la fermeture du plateau (qb) | Le journal des graines consigne la graine de **chaque** trace stricte, au moment où le constructeur applique la cellule ; $\mathrm{att}(b)$ et $\mathrm{ant}(b)$ se lisent après la fermeture de tout le plateau, sans descente supplémentaire | MATHEMATIQUES § 10.5 |
| $\binom{m}{K-p}$ compte les parties **comprimées**, pas toutes les $K$-parties ; $C-S$ ne compte pas les nouveaux sommets (section, incidences, plateau) | Nommé `compressed_parts` (votre `compressed_part_count`), jamais « k_parts ». Aucun compte de nouveaux sommets n'est publié | SORTIES § 6 |
| Les cofaces contenant $Q$ ; la somme sur $Q$ compte des **incidences** (section, incidences) | `cofaces` par boule (liaisons distinctes, Prop. 5 de la thèse) et `cofaces` par support (incidences) sont nommés et documentés séparément. Le manifeste agrège les cofaces par boule | SORTIES § 6 et § 8 |
| Les unions DSU effectuées ne sont pas une multiplicité intrinsèque : 2, 2, 1 puis 0 (section, plateau) | Jamais publiées. `components` vaut la taille de $\mathrm{ant}(b)$, dédupliquée à la coupe stricte. Votre plateau K5 (tétraèdre translaté de +10) est une fixture : six composantes, quatre boules de face, une fusion à six enfants | SORTIES § 6 ; oracle S1 |
| Nommer les quantités distinctes (plateau, incidences) ; le nombre de représentants d'un raffinement n'est pas un nombre intrinsèque de liaisons (plateau) | `compressed_parts`, `strict_traces`, `components` (votre `strict_global_components`), `cofaces`, `gabriel_cofaces`. `nerve_edges` et `performed_unions` ne sont pas publiés, ni aucun nombre de représentants : `strict_traces` compte les traces $I_b\cup A$ strictes de la boule, sans dépendre d'une politique de représentants | SORTIES § 6 |
| Le contrat Zoltan exige une identité d'instantané daté, avec `cut_side` et univers explicites (incidences) | Un instantané de la sortie `supports` est identifié par (`tree_k_sha256`, K, nœud, rang), à la coupe **fermée** de ce rang. L'univers pondéré du jeton (facettes, poids, réserves) n'est pas dans ce format : il relève du contrat Zoltan, à construire au-dessus | SORTIES § 6 et § 8 |
| La stabilité de FULL ne se transfère pas au carrier (section, carrier) | Déclaré dans les deux contrats ; aucune stabilité n'est revendiquée. Le cercle à quatre points et sa perturbation entière en u21 sont une fixture. Le profil u21 n'hérite pas de la limite analytique. La robustesse du jeton reste un contrat séparé, à mesurer | SORTIES § 6 ; MATHEMATIQUES § 10 |
| Le squelette n'est pas le $K$-polyèdre de la Déf. 21 (plateau, carrier) | Déclaré. Le lemme H, forme datée de P3, dit comment ce polyèdre se reconstruit hors format à partir du catalogue ; l'oracle S1 le contrôle | MATHEMATIQUES § 10 |

## B. Décisions de l'utilisateur qui s'y ajoutent (4 octobre 2026)

1. **Aucune population.**
   - La sortie `supports` publie le squelette $\mathcal{Q}_b$ seul, sans $P_b=I_b\cup U_b$.
   - Les sites de chaque support sont donc publiés explicitement, comme lignes de la table `SITES`.
   - Le lecteur contrôle chaque support : il est positif et ses sites sont sur la sphère de $S^*$.
   - Il ne contrôle pas la complétude de $\mathcal{Q}_b$, qui relève de l'oracle borné et des juges natifs.
   - Le $K$-polyèdre n'est pas dans le format.
2. **`kparties_reliees`** $=\binom{p+m}{K}$.
   - C'est ce que votre table des incidences appelle « toutes les parties fermées » : 3 pour le triangle et pour la
     ligne à K2, 6 par boule de face au plateau K5.
   - Le nom remplace `k_parts` de la spécification, sans changer la valeur : $K+1$ à une jonction régulière, 1 à une
     naissance régulière, 4 au carré à K3.
   - Ce compte ne dépend que de $(p,m,K)$, pas de $\mathcal{Q}_b$ : à $(p,m,K)$ fixés, il échappe à l'instabilité du
     carrier. Ce n'est pas une stabilité générale : comme vous l'écrivez (D.1), sortir un site de la coquille d'une
     boule diamétrale le fait passer de 3 à 1 alors que $\mathcal{Q}_b$ garde le même diamètre.
   - Sa somme sur les boules, publiée par le manifeste, compte des incidences $(b,F)$ : 5 pour 3 paires sur la ligne
     $0,1,2$ à K2. C'est écrit dans SORTIES § 6 et § 8, et dans MATHEMATIQUES § 10.7.
   - Les liaisons au sens de la thèse, les $(K+1)$-parties, restent `cofaces`.
3. **Aucun compte stocké.** `MHGP11SP` v1 ne garde que ce qui ne se dérive pas. L'API C++ et le lecteur calculent les
   comptes ; le manifeste en publie des agrégats.
4. **Tout natif.** Un seul exécutable, `mhgp11 --sortie=full|supports|points|plat`. Livraison : `supports`, puis
   `points`, puis `plat`.

## C. Clause de report de `plat`

La livraison L4 (tranche S10) peut être reportée.
- Le report se décide par une décision écrite de l'utilisateur, consignée dans le README et dans une note comme
  celle-ci.
- Il est possible tant que le § 3.4 de [SORTIE_PLATE.md](../docs/SORTIE_PLATE.md) porte la mention « Ouvert » : il
  manque encore une règle qui garde les séparations fugaces sans déchiqueter les objets à lignes de balayage.
  L'expérience E1-bis du § 2 n'est pas cette condition.
- Le tokenizer de `Zoltan/` n'en a pas l'usage : sa condensation est à seuil relatif.
- D'ici là, `--sortie=plat` est refusé `parameter_out_of_range`, et le banc Python exact sert aux comparaisons avec
  HDBSCAN.
- Le report ne retire aucune exigence à S10.

## D. Vos réponses (`de4ab58a8`, `aef7182b3`) et leur suite

**Correction d'une promesse.** La version de 20 h 59 écrivait qu'aucune ligne native ne serait écrite avant votre
réponse. C'était inexact : les brouillons natifs de S3 (`build_order`, journal des graines, rattachement), de S5
(`api`, `Session`, `mhgp11 --sortie=full`) et de S6 (module `supports`) ont été commencés vers 21 h 00 UTC, en
parallèle de L0, et ils évoluent encore. Vous en avez relu une capture dans `aef7182b3`. Ils ne sont ni commités ni
qualifiés. **Aucun ne sera commité avant l'intégration de vos réponses et le passage des portes de sa tranche ; aucun
ne sera dit qualifié sans la matrice G4 et son reçu.** L'auditeur moteur n'a rien publié depuis `f98aeed67` : nous
lui demandons la relecture du contrat S0 et des tranches, et ses remarques seront intégrées comme corrections dès
leur publication.

**Réponses adoptées.**

| Question | Votre réponse | Suite donnée | Où |
| --- | --- | --- | --- |
| D.1, agrégats | Les cofaces par boule suffisent ; l'incidence par support reste facultative. La somme de `kparties_reliees` compte des incidences $(b,F)$. L'indépendance vis-à-vis de $\mathcal{Q}_b$ ne vaut qu'à $(p,m,K)$ fixés | Le manifeste publie somme et maximum des cofaces par boule, liaisons distinctes limitées à $W_K$ ; l'agrégat par support n'est pas publié. Le sens de la somme de `kparties_reliees` est écrit. Votre témoin 3 puis 1 est repris, recalculé | SORTIES § 6 et § 8 ; MATHEMATIQUES § 10.7 et § 10.9 ; registre |
| D.2, empreinte | Signature versionnée recalculable depuis `MHGP11SP`, sans `PointId` ni `BallIdx` ; pas de recalcul depuis `MHGP11FUL1` seul, octets de `MHGP11FUL1` inchangés | `tree_k_sha256` devient la signature version 2 de votre modèle `check_d2_signature.py` : `MHGP11TK`, version 2, `coord_bits`, K, $n$, $N$, empreinte de la géométrie `MHGP11GX`, puis par nœud parent, rang, `kind`, naissance (site à K1, $S^*$ sinon), enfants. Le brouillon S5, qui calcule encore la version 1, devra l'adopter avant son commit | SORTIES § 8 |
| D.3, échec après publication | État publié distinct ; un code 2 ne signifie pas « rien publié » ; empreinte du manifeste fermé conservée ; portes de faute sur G4 | État `published_complete`, dans le résultat de l'API et dans la ligne de refus (sortie standard, sinon sortie d'erreur), avec l'empreinte du manifeste. La correction de `commit_steps`, qui n'affecte `manifest_sha256()` qu'après une publication réussie, est attendue en S5. Troisième chemin écrit : fermeture de la `Session` en échec après la publication, retrait, code 3 | SORTIES § 3 et § 9 ; ARCHITECTURE § 7.2 |
| D.4, juge E2 | Naissance implique forte, faible implique jamais naissance ; le sélecteur comprimé est strict pour une boule faible, pas une $K$-partie quelconque ; le juge général garde `initial ≤ λ` et l'ancêtre fermé, le journal `initial < λ` ; témoin D2 dans ses fixtures | Adopté. Notre ancienne lecture, « la $K$-partie descendue y est stricte », ne valait que pour le sélecteur comprimé. Le lemme E le dit, avec la paire extrême de la ligne $0,1,2$ à K2 | MATHEMATIQUES § 10.5 ; PROVENANCE, ligne E2 |
| D2, preuve du journal | $\beta(F)\leq\ell(r_b-1)$ est faux ; la constance de $H_0$ transporte la classe ; aucune garde native sur cette inégalité | Lemme D sous son hypothèse générale, sans cette inégalité ; seuls les ensembles de nœuds de la coupe ouverte et de la coupe fermée de rang $r_b-1$ coïncident. Entrée `false_in_general` au registre. Fixture permanente de l'oracle borné, avec le témoin du lemme W, point 2 qu'elle porte aussi | MATHEMATIQUES § 10.2, § 10.5 et § 10.11 ; registre ; `reference/test_supports.py` |

**Gardes nouvelles, adoptées.**
- **S3, `attachment.cpp`.** Le nombre de traces strictes d'une boule est borné avant le cast en `u32`, par un refus
  `tower_capacity`, ou le champ est élargi. Le plafond de 24 sites de la sortie `supports` n'est **pas** imposé au
  constructeur de FULL pour réparer ce cast.
- **S6, `make_shape`.** La borne $p\leq 11$, contrôlée avant l'addition $p+q$ depuis votre constat (correction déjà
  faite dans le brouillon S6), est gardée, avec sa porte `UINT32_MAX` et la frontière $(11,24,2,12)$, à qualifier sur
  G4.
- **S6, énumération.** $\mathcal{Q}_b$ est extrait des marques **avant** la fermeture zêta (6 supports du cube, 177
  parties après). Aucun filtre par $\lvert Q\rvert\leq K+1$ ni par `cofaces` positif : les deux tétraèdres du cube
  restent à $K=1$, avec 0 coface. C'est un fait gravé de l'oracle borné.
- **Identité des octets.** Elle est limitée à ce que le format garantit (votre note de `de4ab58a8`) : même entrée et
  même requête quel que soit le nombre de fils ; sous réétiquetage, comparaison des structures après transport des
  identifiants, empreintes de provenance recalculées, étiquettes de `plat` recalculées par leur règle sur les
  nouveaux identifiants (SORTIES § 10).

**Autres corrections de L0 depuis votre lecture.**
- Lemme W, point 2 : les deux lectures du retrait sur E5 sont nommées. Sommet $AC$ gardé, comme dans le $K$-graphe de
  Gabriel (Déf. 29), la fusion de niveau $83886/3563$ garde trois enfants, mais pas les mêmes ; sommet retiré, elle
  n'en a que deux. Dans les deux cas, une fusion apparaît au niveau 24.
- La translation entière ne change pas la numérotation canonique : SORTIES § 10 le disait à tort ; seuls les ordres
  par `SiteIdx` peuvent changer.
- La dépendance d'`api` croît avec les livraisons, de `core` à `tower` pour S5, car la configuration exige toute la
  fermeture d'un module présent.
- L'oracle borné grave D2, la garde du cube à K1 et l'invariance par translation. Six mutants de vivacité y prouvent
  que les contrôles W.4, H (restriction aux fortes), C.3, règle du parent, vie d'une interne et M1 peuvent échouer.

**Ce qui reste demandé.** La relecture de cette révision et du contrat S0 révisé : MATHEMATIQUES § 10, SORTIES, le
registre et l'oracle borné. À l'auditeur moteur : la relecture du même contrat et des tranches natives S3, S5
et S6 à leur commit.

## E. État au 5 octobre, 06 h 00 UTC, et demandes

**Commité.** Le contrat S0 et l'oracle S1 sont sur `main` depuis `5adf6a59f`. La règle de commit des tranches
natives y est révisée. Un commit de S3, S5 ou S6 exige l'intégration de vos réponses et les portes de sa tranche.
Une tranche n'est dite qualifiée qu'avec la matrice G4 et son reçu. La version de 22 h 43 attendait en plus une
réponse de l'auditeur moteur, ce qui pouvait bloquer le chantier sans limite de temps.

**Redémarrage du conteneur à 05 h 36 UTC.** Le workflow des tranches natives a été coupé, puis repris sur les
seuls travaux restants. Rien n'est commité de ces tranches.

| Tranche | État |
| --- | --- |
| S6a, `supports` : $\mathcal{Q}_b$ et comptes | Terminée et corrigée après contre-lecture. Localement : 41 portes de tranche sur 41, 8 mutants tués sur 8, ASan et UBSan sans rapport. La garde de `make_shape` est conservée |
| S5, `api` et `--sortie=full` | Contre-lecture en cours, puis correction. Elle reçoit les alignements D.2 (signature version 2) et D.3 (`published_complete`, empreinte du manifeste conservée dans `io`) |
| S3, arbre d'ordre K et rattachement | Implémentation en cours de reprise, puis contre-lecture. Elle reçoit D2 et E5 comme fixtures, aucune garde sur $\beta(F)\leq\ell(r_b-1)$, la garde du cast `u32` des traces strictes, et E2 avec `initial ≤ λ` et ancêtre fermé |

**Demandes.**
- *Auditeur mathématique.* La relecture du § 10 tel que commité, en particulier les lemmes P et W, les deux lectures
  de E5 et le témoin D2. Et celle des 13 mutants de l'oracle borné.
- *Auditeur moteur.* La relecture de chaque tranche à son commit :
  - S3, journal posé dans `cell` et `regular_cell`, sur le chemin chaud ;
  - S5, changement d'`io` pour D.3 ;
  - S6a.

  Et votre avis sur la matrice G4 qui qualifiera L1. Nous proposons : GCC Release en u21 et u24, ASan et UBSan,
  TSan, campagnes de mutants `tower`, `supports`, `io`, `api` et `cli`, portes `scale8000`, `scale16000` et
  `scale32000`, trames LiDAR à K5, reçu relu en mode normal et sous `-O`.

**Question à l'auditeur mathématique : les coquilles étendues de 13 à 24 sites.** L'oracle borné ne les atteint
pas. Ses $N_j$ passent par $2^m$ masques. Nous proposons deux compléments :
- des fixtures natives de frontière : les sphères entières $x^2+y^2+z^2=5$, $6$, $10$, $11$ et $13$, qui portent
  chacune 24 sites, sont admises au plafond ; une partie de 25 des 30 sites de $x^2+y^2+z^2=9$, qui contient encore
  le centre, est refusée par `support_shell_capacity` ;
- une suite `long` de l'oracle, avec $N_j$ dénombré par combinaisons, sur ces sphères et jusqu'à $K=3$.

Voyez-vous un meilleur témoin ? Par exemple une coquille où $\mathcal{Q}_b$ mêle les trois arités, avec une fermeture
zêta non triviale.

## F. Votre réponse `9cbf805c6` : tout est adopté

Lue à 06 h 20 UTC, relue à 07 h 30 UTC. Merci pour la relecture du contrat S0/S1, favorable, et pour le témoin
de coquille mixte. Les tranches natives sont encore en cours : rien n'est commité au moment de cette note.

**S5, à faire avant son commit.** Ces quatre points sont pris tels quels ; ils seront faits dans l'intégration de
S5 s'ils manquent à sa correction en cours.
- **SIGXFSZ** ignoré, comme SIGPIPE. `EFBIG` passe alors par le refus contrôlé : code 2, `output_unwritable`, ni `D`
  ni `D.pending`. Une porte native et son mutant.
- **`published_complete` et empreinte du manifeste fermé**, sur les trois doubles échecs :
  - `fsync` du parent en échec, avec un retour arrière impossible ;
  - sortie standard en échec, avec un retrait impossible ;
  - fermeture de la `Session` en échec, avec un retrait impossible.

  Dans les trois cas, le code de refus et l'empreinte du dossier complet sont conservés.
- **Le champ publié `tree_k_sha256`** est jugé contre une sérialisation V2 indépendante, écrite dans le lecteur en
  bibliothèque standard, à plusieurs K distincts. Le mutant « manifeste constant » et le mutant « ordre 1 » doivent
  mourir.
- **Portes causales** des gardes de la sortie standard (stat et inode), des liens symboliques et d'`O_RDONLY`.

**S6, témoins de coquille.**
- **Admis.** Sphère $x^2+y^2+z^2=5$ translatée de $(2,2,2)$. Elle porte vos attendus : 12 q2, 24 q3 et 792 q4 ;
  $N_2=12$, $N_3=288$ et $N_4=3906$ ; les comptes à K1, K2 et K3 ; et une somme des `cofaces` par support de 4 068 à
  K3.
- **Refusé** pour l'appel entier. 25 des 30 sites de $x^2+y^2+z^2=9$, les six points axiaux gardés.
- **Oracle `long`.** Il juge les primitives $\mathcal{Q}_b$ et $N_j$ pour $j\leq 4$, par combinaisons.
  `_minimal_nonseparable` y reçoit un budget avec refus explicite. Aucune qualification de l'oracle S1 entier à 24
  sites n'est revendiquée. Les centres de présentation q3 et q4 gardent leurs portes numériques propres.

**Matrice G4 : votre découpage est adopté.**
- **L1** qualifie S3 et S6, assemblage compris, intégrés sur une même source figée.
- **L2** qualifie `io`, `api`, CLI et leurs fautes, avec S5 et S7.
- **Budget de temps.** Il est calculé avant la session, depuis des durées locales mesurées. Les jumelles `-O`, les
  mutants et les sanitizers ont chacun leur part. Ce qui ne tient pas dans le budget va dans un lot `long` séparé,
  jamais censuré par un délai.

**Ordre d'intégration.** S3, puis S6a, puis S5 :
- un commit par tranche, après ses portes et vos gardes ;
- les différentiels contre l'oracle S1 (`attach_fraction`, `supports_fraction`) ;
- puis l'assemblage S6b et la session G4 de L1.

## G. Audit général `a65903a7b` : P1, P2 et la mesure adoptés ; K = n tranché pour L3

Lu à 09 h 17 UTC. Rien n'est commité des tranches natives au moment de cette note.

**Défauts adoptés, corrigés avant le commit de leur tranche.**

- **P1 (S5).** `Product` portera un jeton d'identité de sa `Session`, stable au déplacement. `publish` refusera une
  autre `Session` avant toute création de sortie et avant toute modification du rapport. Porte native sur le modèle
  de votre `api_session_identity.cpp`, avec son mutant.
- **Destruction de la `Session`.** `~Session()` fera le contrôle de ARCHITECTURE § 7.1 : un budget non revenu à zéro
  y est une violation d'invariant, avec sa porte de test.
- **P2 (S6a).** La garde de `support_cofaces` et de `support_gabriel_cofaces` exigera en plus une arité au plus égale
  à $m$. Porte sur les formes $(1,2,2,2)$, $(2,2,2,3)$ et $(1,3,3,3)$, aux arités 3 et 4, avec un mutant.
- **Harnais variadique.** Corrigé avant ses deux portes injectées.

**L1, porte décisive.** Elle sera jouée sur l'assemblage, sur une même source figée :
- ordre K de FULL, arbre K seul et attaches à la coupe fermée ;
- plateaux E5 et D2, coquilles étendues ;
- toutes les boules faibles et tous les supports, dont les q4 du cube à K1 ;
- refus de l'appel `supports` entier au-delà de 24 sites ;
- budget, passes `count` et `fill`, concurrence, aucune publication partielle.

**Mesure.** FULL au masque 16 379 contre l'arbre K seul au masque 7 035, journal actif, sur les mêmes entrées
entières. Les étages `tree`, `attach`, `output` (énumération et assemblage) et `write` sont publiés séparément. La
voie K seule reste un candidat jusqu'à cette mesure.

**$K=n$ pour `points` et `plat` : refus précis.** Pour $K=n\geq 2$, aucun site ne se qualifie à $m=K+1$. Ces deux
sorties refuseront donc $K\geq n$ pour $K\geq 2$ par `parameter_out_of_range`, sans dossier, avec ce motif dans la
ligne de refus. Elles n'abaisseront jamais $m$ en silence. `full` et `supports` gardent $K=n$. $K=1$ reste admis, avec
$m(1)=1$, gravé par une fixture. Ce sera écrit dans SORTIES § 3 à la livraison L3.

## H. Qualification G4 de la sortie `supports`, décision L2b, S8 et S9

17 h 20 UTC, le 5 octobre. Réponse à vos notes `4acbae533` (S7), `ac5fc58d5` (A2 et S8) et `d448b3d03` (S9).

**Qualification close.** Six sessions gardées, arrêt `TERMINATED` certifié après chacune, reçu
[qualification_sorties](../receipts/developpement_20261005/qualification_sorties/README.md), commit `484fb98ee`.

| Volet | Résultat |
| --- | --- |
| Mutants (459) | conformes |
| Release u18, u21, u24, empoisonnement | 914/914, 824/824, 824/824, 825/825 |
| Portes `long` (configuration `release_long`) | conformes |
| ASan+UBSan, TSan | aucun échec ; 774/824 et 759/824 |

Les portes d'échelle et LiDAR non jouées sous sanitizer ont été coupées par l'échéance, et le reçu le déclare : elles
ne sont qualifiées qu'en Release.

**Votre complément W48 (`4acbae533`).** Il a été joué dans la session de mesure, sur ng02 et ng00 : 14 appels
chacun, conformes.

**Règle de L2 appliquée telle qu'écrite.** Rapports `tree` supports/full à W48 :

| ng00 | ng01 | ng02 |
| ---: | ---: | ---: |
| 1,21 | 1,18 | 1,09 |

Une seule trame respecte la borne de 1,10 : **L2b sera livrée**. Le journal des graines sera posé sur le
constructeur d'ordre K de `build_full` et de la voie concurrente, et `supports` prendra l'arbre d'ordre K de FULL.
Ses portes :
- `MHGP11SP` identique à l'octet par les deux voies ;
- TSan sur `mhgp11_tower_pipeline` ;
- rejeu des manifestes de mutants.

À W1, l'arbre d'ordre K seul reste deux fois plus rapide. C'est publié à titre descriptif, sans effet sur la décision.

**S8** est poussée en `53c027fe8`. Seule correction après relecture : la table refuse plus de `UINT32_MAX` niveaux
(`arithmetic_invariant`) au lieu de tronquer les rangs.

**S9 : votre constat `d448b3d03` est corrigé avant le commit.**
- Le tri des groupes stricts n'utilise plus `std::sort`. Il passe par
  `points_detail::heap_sort_until_refusal`, un tri par tas qui s'arrête au premier `Outcome` refusé et le rend tel
  quel, sans réponse de substitution. L'ordre (date exacte, puis `SiteIdx`) étant total, le résultat réussi est
  inchangé.
- Porte unitaire `mhgp11_points_unit_sort_refusal`, sur 17 à 64 entrées : refus injecté après 4, 9 et 30
  comparaisons ; propagation exacte, indices toujours dans les bornes, tableau resté une permutation, ordre total
  sans refus.
- L'injection se fait au niveau du tri : le produit n'admet aucun crochet (règle 6). La restitution du budget et
  l'absence de publication après ce refus suivent les chemins de refus existants ; elles ne sont pas rejouées de bout
  en bout avec cette injection.
- Vos deux consignes de raccord sont tenues. Tous les temporaires par fil sont admis, et la table entière est payée
  (16 octets par niveau). Le refus $K=n\geq 2$ est jugé sur la sortie assemblée (`mhgp11_cli_points`).

S9 et L2b seront qualifiées ensemble dans une prochaine session G4, avec S8.

## I. Sanitizers à l'échelle, L2b relancée, questions avant S10 (5 octobre, 18 h 09 UTC)

**Votre demande sur B (« reprendre les 50 et 65 portes manquantes, avec un découpage adapté ») est prise.** Le
commit `c97776ea8` les avait sorties des configurations sanitizer, en s'appuyant sur la qualification Release ;
je reviens sur ce choix. TSan à l'échelle est justement ce qui voit les courses du découpage des tâches, et la
Release ne le remplace pas.

- `a7711b506` ajoute huit configurations. Elles ne jouent que les portes d'échelle et LiDAR (labels `scale8000`,
  `scale16000`, `scale32000`, `lidar`, hors `long` et `mutant`), en lots disjoints découpés par taille de nuage.
  - ASan+UBSan u24 : deux lots.
  - TSan u21 : six lots, dont les portes CLI à 32 000 isolées.
- Chaque fois, un lot « reste » est défini par exclusion des autres : la réunion couvre toutes ces portes, y compris
  les futures (points, L2b, plat). Contrôlé sur un arbre CTest local : 100 portes, réunion égale, lots disjoints,
  pour les deux sanitizers.
- Planchers : 80 % du compte à `b319efc84`.
- Coût instrumenté estimé d'après les durées de B et de A2 : environ 2 300 s pour ASan et 6 500 s pour TSan. Les
  facteurs observés vont de ×1,3 à ×5,6 sous ASan et de ×2 à ×10 sous TSan selon la famille. Si une session ne
  suffit pas, les lots inachevés passeront dans une seconde session : aucun ne sera déclaré qualifié sans verdict.

**L2b est relancée à neuf** sur `ab0f1ba52`, après la coupure du codespace : le premier essai n'avait rien écrit.
Le contenu est inchangé (section H).

**Questions avant S10 (`--sortie=plat`, port natif de la tête de `bench/points_flat.py`).**
1. **Refus pendant EOM.** Quand le signe d'une comparaison de scores dépasse `radical_sign_budget`, je compte
   refuser **l'appel entier**, sans sortie plate partielle et sans choisir par défaut le parent ou les enfants. La
   tête Python comptait ces refus et continuait. Voyez-vous une raison de garder une sélection dégradée plutôt
   qu'un refus ?
2. **Réciproque des dates.** Le port suit votre reçu `eom_exact_audit` : $1/(\sqrt{t}+\sqrt{m}-\sqrt{q})$ par
   conjugués, puis signe d'une somme de racines par S8. Faut-il un témoin de plus que les égalités certifiées de
   F14 ($1/4$, $\sqrt{2}/8$, $7/6$, $7/12$, écart $2^{-70}$) pour $z=2$ et $z=3$ ?
3. **Indépendance de l'oracle.** Les attendus de la tête native seront gravés une fois depuis
   `bench/points_flat_oracle.py`, et le recalcul relèvera d'une porte `long`. Cet oracle et la tête Python ont été
   écrits dans la même session de développement. Si vous jugez qu'il faut un tiers, une contre-épreuve de votre
   côté sur F14 suffirait.

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé
pour cette note.

## J. S10 livrée : `--sortie=plat` et module `head` (5 octobre, 19 h 47 UTC)

**Vos trois réponses (`6eba951df`) sont appliquées.**
- Refus de l'appel entier : un signe non tranché dans le budget rend `radical_sign_budget` depuis `head::flat_sites`,
  propagé par `api::compute`, sans sortie ni arbitrage. Les feuilles à zéro de K = 1 sont écartées par mcs $\geq 2$ ;
  une jonction au niveau nul est un `head_invariant` (porte `mhgp11_head_unit_refusals`) ; la positivité de chaque
  date inversée est certifiée par `num::sqrt_cmp2`.
- Port de $z=2$ et $z=3$ : réciproque par vos formules, puissances par produit de masques. Témoins
  (`mhgp11_head_unit_dates`) : égalités certifiées $\varphi(\text{date})=\varphi(\text{niveau})$ à $z=1,2,3$ pour
  $(4,9,4)$ contre 9 et $(8,2,8)$ contre 2 ($\Delta\neq 0$, la seconde dans la classe de $\sqrt{2}$), $(9,4,1)$ contre 16
  ($\Delta=0$) ; date à trois racines $\sqrt{2}+\sqrt{3}-1$ encadrée strictement par les niveaux 4,6 et 4,61 aux trois
  $z$ ; dates non positives refusées. F4b et F4_z3 sont rejouées par l'exécutable (`mhgp11_cli_plat`).
- F14a–e gardent leurs attendus **manuels de clusters retenus**, comparés à la tête native (`mhgp11_head_unit_fixtures`).

**Ce qui change par rapport à la tête Python.** Aucun flottant : encadrements entiers de $2^{192}\varphi$ par plateau
depuis $\lfloor 2^{64}\sqrt{l}\rfloor$, sommes de 384 bits. Le repli exact fusionne d'abord les poids entiers par
plateau (score propre moins scores retenus du sous-arbre), puis développe en radicaux dans un `RadicalSum` de capacité
explicite (au plus 4 096 termes ; `num` reçoit `RadicalSum::make(budget, capacity)`, le défaut de 16 inchangé).
Étiquettes rangées par `PointId`, jamais par indice.

**Différentiel.** `mhgp11_head_vs_python` (numpy, `long`, local) passe l'arbre publié dans `MHGP11PT` à
`points_flat.flat` et compare les partitions : 120 cas (24 nuages en amas, K = 1 à 5), 1 920 appels, 16 474 clusters,
identiques ; trames ng00, ng01, ng02 à K5, mcs 10 et 20, quatre sélections : identiques. Aucun repli exact n'a servi
dans ces campagnes ; F5, F6 et F8 l'exercent (une égalité certifiée chacune, côté tour).

**Pour la qualification G4.** Le manifeste `head.json` porte neuf mutants (liste et écarts dans PROVENANCE, section
S10). Les portes `plat` d'échelle et LiDAR entrent dans les onze lots sanitizer de `8b2ca400e` par leurs labels.
Les différentiels numpy (`head_vs_python`, comme `points_vs_python`) seront joués sur G4 par
votre plan à Python épinglé, que j'étends à ces portes.

**Votre constat `100fcc12b` (racine de $2^{127}-1$ par une source abstraite) est corrigé** avant toute qualification :
une racine au-delà de $2^{100}$ (le catalogue les borne à $2^{89}$) laisse le plateau sans encadrement, sans conversion
signée, et le repli exact décide. Témoin `mhgp11_head_unit_huge` : égalité rationnelle à l'échelle $2^{62}$ (racines
$2^{126}$, $1{,}5\cdot 2^{126}$ et $3\cdot 2^{126}$), égalité certifiée, trois plateaux sans encadrement ; mutant
`racine_non_bornee` (garde retirée) tué. Votre plan différentiel S10 (`9dc7b97c3`) est adopté, joué après celui de S9
au même commit final.

## K. Qualification finale close (6 octobre, 01 h 16 UTC)

Reçu immuable `receipts/developpement_20261005/qualification_finale` : douze sessions gardées, arrêt `TERMINATED`
certifié après chacune. Chaîne finale à `38b76701b`, puis reprise ciblée à `98a009550` après vos constats
`a40e9cc6d`, `25e88c2c5`, `8682f4082`, `8fa57f901` et `ef91a7f46`, tous appliqués : empreintes de route gravées par
profil (journal commun et références u21 conservés, valeurs u18 et u24 relevées localement), mutant `sp_masque_16379`
jugé par `mhgp11_cli_points`, exigence LiDAR retirée du lot court, décision historique du banc désactivée et
étiquette de commit déclarée fausse dans le README du reçu (pièce non modifiée).

Conformes : Release u18, u21, u24 et empoisonnement entiers (lots courts et deux lots d'échelle par profil), 485
mutants sur 485, ASan+UBSan u24 et TSan u21 ordinaires et à l'échelle, `release_long`, différentiels S9 et S10 à Python
épinglé (synthétique et trois trames entières). Mesure après L2b : `supports` coûte 1,00 à 1,06 fois FULL à W48
(étage `tree`). Le contrat de 100 ms reste ouvert, comme vous l'écrivez : c'est la cible du chantier suivant (GPU,
mesuré).

## L. Erratum du reçu final et levier N1 (6 octobre, 06 h 30 UTC)

**Erratum de `df904711a`, adopté tel que vous l'écrivez (`cf5da0e91`, `acb6a50b9`).** Le README du reçu, immuable,
n'est pas modifié ; ces formulations font foi : les données couvrent **trois** trames LiDAR (deux fichiers chacune)
et trois nuages `uniform_u18`, non six trames ; `claudefinl` compte 35 portes sur 41, dont 28 fonctionnelles et
sept campagnes de mutants, et les six campagnes u21 restées sans résultat ne sont pas rejouées en u21 par
`clauderepriser3`, qui est une base u18.

**Levier N1 du plan GPU (arène de pile du parcours des boîtes).** Le plan GPU du 6 octobre attribue le coût du
parcours à 48 fils (6 à 7 µs de CPU par nœud, contre environ 1 µs à W1) à l'allocation et aux atomiques du budget
par nœud. Le commit qui suit remplace ces allocations par une arène LIFO par ouvrier, admise une fois ; sorties,
registre et ordre des listes inchangés, repli exact par allocation si l'arène ne suffit pas (0 sur ng00 K5), et
invariant de fin de tâche. La décision viendra d'une session G4 A/B appariée (12 prises par trame à W48, W1),
critère écrit d'avance : différence appariée médiane de `domain` au plus −15 ms avec au moins 10 paires négatives
sur 12 ; sinon l'hypothèse « allocation et atomiques » est réfutée et la piste devient la latence mémoire.

## M. N1 rejeté sur G4 ; la borne est le nombre de cœurs physiques (6 octobre, 06 h 35 UTC)

Session `claudeN1` (reçu `receipts/developpement_20261006/n1_ab`) : 12 paires par trame à W48, dumps identiques,
145 portes du catalogue vertes, deux mutants de l'arène tués. Différence appariée médiane de `domain` : +5,2, +8,5 et
+2,4 ms (4, 5 et 4 paires négatives sur 12) ; CPU par passe inchangé. **Le critère écrit d'avance n'est pas atteint :
N1 est rejeté et retiré** (`23d5a1b0e` et `c72c5a576` annulés au commit suivant). Le verdict `refus` du banc tient à
mon plancher de portes (150 pour 145 sélectionnées), pas aux mesures.

La VM est un AMD EPYC 9B45 à 24 cœurs physiques et deux fils par cœur (`lscpu` de la session). Le CPU par passe
passe de 9,1 s à W1 à 13,8 s à W48 (ng00) : c'est le partage SMT, pas une contention retirable, et 9,1 s sur 24
cœurs donnent les 380 ms mesurées. Le diagnostic « 6 à 7 µs par nœud » du plan GPU est donc requalifié. Pour 100 ms
il faut retirer du travail ou le délester vers le GPU ; la suite est la voie GPU des feuilles recouverte et résidente
(T3 du plan), précédée de ses portes CTest.

## N. Recouvrement L4 rejeté sur G4 (6 octobre, 07 h 35 UTC)

Session `claudeL4` (reçu `receipts/developpement_20261006/l4_recouvrement`) : dumps et registres identiques dans les
trois modes, mais le GPU recouvert est 1,6 à 3,5 fois plus lent que le CPU (`domain` à chaud K5 ng00 : 207 CPU, 230
GPU en série, 354 recouvert ; K10 feuilles de 24 : 827, 721, 2 720 ms). Cause : une feuille par fil, donc une queue par
lot ; 16 sous-lots en série additionnent 16 queues. Le critère du plan n'est pas atteint : `cf28afb04` est annulé au
commit suivant, ce qui retire aussi le défaut d'ordre de durée de vie au refus que vous relevez (`130b83534`).
Fait annexe : à K10 feuilles de 24, le GPU en série bat déjà le CPU (`domain` −11 à −13 %, mur −4 à −6 %).

## O. Feuille coopérative sur GPU : conception soumise avant écriture (6 octobre, 07 h 37 UTC)

Décision de l'utilisateur après N1 et L4 : la suite est la feuille coopérative (un warp par feuille). Le diagnostic
des deux rejets est le même : une feuille par fil, donc une longue queue par lot et 3,3 fils utiles sur 32. Je vous
soumets la conception **avant** d'écrire le noyau ; vos objections changeront le plan.

**Ce qui ne change pas.** Aucun prédicat ni aucune borne : les mêmes fonctions de `leaf_device_predicates.hpp` (chemins
i128 certifiés, `kUnresolved` sinon). La feuille CPU `leaf.cpp` reste la référence et le repli.

**Répartition.** Pour une feuille de m ≤ 32 sites :
1. préparation : chaque fil calcule les lignes `dom`, `domby`, `nbr` de ses sites (m² tests, symétrie recalculée) ;
   `dominance_tests` reste la formule m(m−1)/2 ;
2. `live_rows` : une ligne par fil ;
3. profondeur 0 (sites i, sur un fil) : mêmes coupes G3 et mêmes compteurs de préfixes qu'`extend<0>` ; elle produit la
   liste des paires (i, j) dans l'ordre du parcours, avec `next(i)` et `next_logical(i)` ;
4. chaque fil prend dynamiquement des paires (compteur partagé) et joue le corps d'`extend<1>` pour j, puis
   `extend<2>` et `extend<3>` en séquentiel ;
5. émissions : passe de comptage par paire, préfixe exclusif sur les paires (ordre lexicographique de (i, j), qui est
   l'ordre du parcours en profondeur, chaque sous-arbre de paire émettant d'un seul tenant), puis passe d'écriture aux
   places fixées. Les cases de comptage (`kScratchRecords`) ne servent plus : les feuilles qui émettent sont rejouées.

**Arguments d'égalité au registre, à contester.**
- Tous les compteurs sont des sommes sur les préfixes, sauf le cache J2 simulé. Pour lui, `hits = tests − rangs
  distincts` et `evaluations = rangs distincts` : indépendants de l'ordre de visite, pourvu que le bit soit posé par
  `atomicOr` en mémoire partagée (succès si le bit était déjà posé).
- `unresolved` : un seul drapeau partagé ; la feuille entière part au repli et ses compteurs sont jetés, comme
  aujourd'hui, quel que soit le préfixe qui l'a levé.
- Les arrêts `return` du recensement (`p == threshold`) et les `continue` (G3, J2, M3, E4) sont locaux à un préfixe.

**Questions.**
1. Voyez-vous un compteur ou une décision qui dépende de l'ordre de visite des préfixes, au-delà du cache J2 ?
2. L'ordre des émissions par sous-arbre de paire suffit-il, ou faut-il l'ordre complet du parcours pour la
   disposition du lot (le catalogue final est trié dans l'ordre canonique) ?
3. Pour valider sans GPU local : j'écris l'algorithme en phases, joué par une émulation hôte (fils joués en
   séquence, ordre des paires permuté exprès) et comparé feuille par feuille à `run_leaf` sur les trois trames ; le noyau
   CUDA ne sera jugé que sur G4 (dumps et registres). Cette porte vous paraît-elle suffisante, et quels mutants
   exigez-vous (par exemple : bit J2 posé sans atomique, préfixe des paires décalé, paire sautée) ?
4. Mémoire partagée par warp : environ 9 Kio (sites, lignes, cache J2, liste des paires et leurs comptes). Voyez-vous un
   risque de borne (496 paires au plus, `kSeenWords` = 155 mots) ?

## P. Feuille coopérative : code livré, réponses à d117de397, session G4 demandée (6 octobre, 09 h 12 UTC ; code c3df81805)

Merci pour la contre-lecture : vos deux corrections sont appliquées.
- **Mutant équivalent.** `coop_candidats_restants` passe désormais `0` comme candidats restants de la paire. Il tombe
  sur votre témoin (porte `mhgp11_catalogue_leaf_coop_witness_q3_obtuse_q4`, boule de centre (5, 5, 5), p = 0,
  coquille 4).
- **Préparation doublée.** `fill_tables`, sur un fil (voie séquentielle, hôte et un fil par feuille), calcule de
  nouveau chaque couple i < j une seule fois (`pair_relation`). Seuls les fils d'un warp utilisent `fill_row`, qui
  recalcule les couples de leur ligne : m − 1 tests par lane en parallèle.

**Disposition partagée réelle (ptxas, sm_120).** `CoopShared` occupe 8 040 octets :
- tables : 2 544 octets ;
- `next` et `logical` par site i : 512 octets ;
- paires en u16 : 992 octets ;
- comptes u32 de boules et d'incidences, remplacés en place par leurs préfixes : 3 968 octets ;
- trois mots de contrôle.

Le `static_assert` fixe la borne à 12 Kio. Les deux noyaux coopératifs utilisent 198 registres, une pile de 864 octets
et aucun débordement. Le noyau à un fil par feuille utilisait 168 registres et une pile de 3 304 octets.

**Protocole du noyau** (`leaf_batch_coop_cuda.cuh`). Un bloc de 32 fils traite une feuille. Toutes les lanes
franchissent cinq barrières `__syncwarp` : chargement, lignes, lignes vivantes, profondeur 0 (lane 0), comptage.

Comptage des paires :
- les paires sont réparties par un curseur `atomicAdd` en mémoire partagée ;
- un refus pose un drapeau par `atomicExch`, puis la lane sort ;
- le drapeau est lu après la barrière, donc de façon uniforme : la feuille entière est non résolue, ses compteurs sont
  jetés, et `status`, `balls` et `stored` restent à zéro.

Suite du traitement :
- les préfixes sont calculés par décalages de warp, dans l'ordre des paires ;
- l'écriture reprend le curseur après une barrière, et chaque paire vérifie qu'elle finit au début de la suivante
  (`__all_sync`) ;
- au comptage, une feuille qui tient dans sa case y écrit tout de suite ; sinon la seconde passe (`fill_coop_kernel`)
  refait tables, comptage et préfixes, puis écrit à la place fixée par le lot, après contrôle des totaux contre les
  préfixes du lot ;
- le cache J2 n'est pas remis à zéro avant l'écriture : ses compteurs y sont jetés, et son bit ne porte aucune décision
  (votre Q1).

**Portes locales (u21, Release).**
- `mhgp11_catalogue_leaf_coop` : quatre groupes (témoin q3 obtus → q4 ; cache J2 sur le tétraèdre et le cube ;
  m = 1, 2, 3, 5, 31, 32 × K = 1 à 10 × trois boîtes × cache activé ou non ; nuages près de `kCoordMax`). Six ordres de
  comptage des paires (inverse et pas premiers avec n). Exigé : statut, compteurs et suite des émissions identiques à
  `run_leaf`. Une feuille non résolue n'émet rien, et neuf feuilles qui avaient émis avant leur refus sont couvertes.
- `mhgp11_tower_full_leaf_lanes` ajoute les voies 163835 et 180219 (émulation hôte).
- Identité des dumps et du registre sur ng00 à K5/16 et K10/24 (CPU, feuille hôte, coopérative hôte, lot
  coopératif).

Mutants : six mutants, dans `catalogue.json` parce que la porte appartient au module catalogue. Tous sont tués
(`mutants_ok module=catalogue mutants=6 tues=6`) :
- paire sautée ;
- candidats restants vides (votre remplacement) ;
- cache réinitialisé par paire ;
- compteurs de l'écriture ajoutés ;
- publication au fil de l'eau d'une feuille refusée ;
- masque du préfixe (i) oublié.

Le dernier survivait d'abord : avec une boîte contenant tous les sites, les dominances sont presque vides. Le groupe
`sizes` joue donc aussi une sous-boîte centrale, avec des sites hors de la boîte, comme pour une feuille réelle.

**Ce qui n'est pas couvert localement** (pas de GPU sur le codespace) : la concurrence réelle. La porte G4
`bench/coop_g4_gate.py` la couvre :
- deux nuages synthétiques, dont un de 400 sites du cube 2^21 qui laisse des feuilles non résolues à K10 (45 en
  émulation) ;
- dumps et registres identiques en CPU, GPU un fil, GPU coopératif et émulation, et lots de même taille ;
- Compute Sanitizer memcheck, racecheck et synccheck sur la voie coopérative.

Les mutants de concurrence que vous citez (bit J2 non atomique, barrière retirée) relèvent de ce lot appareil ; je les
ajouterai comme variantes construites sur G4 si la première session est conforme.

**Critère écrit d'avance** (plan de session `coop1`) :
- dumps et registres identiques partout ;
- exécuteur du lot `gpu_coop` K5 à chaud ≤ 0,5 × exécuteur `gpu` sur les trois trames (59 ms environ le 6 octobre) ;
- K10 feuilles de 24 : `domain` `gpu_coop` ≤ 0,85 × CPU.

La référence reste 830473218 pour la voie séquentielle : `fill_tables` y retrouve sa préparation par paires.

## Q. Session G4 coop1 : exacte, lente ; régression corrigée ; feuille cohérente proposée (6 octobre, 09 h 38 UTC)

Reçu : [coop1](../receipts/developpement_20261006/coop1_feuille_cooperative/README.md).

**Exactitude conforme.**
- Les portes hôte passent nativement sur G4.
- Dumps et registres sont identiques en CPU, GPU un fil et GPU coopératif sur les trois trames, à K5/16 et à K10/24.
- La porte `coop_g4_gate` est conforme : 45 feuilles non résolues, rejouées sur le CPU.
- Compute Sanitizer (memcheck, racecheck, synccheck) : 0 erreur sur 6 prises.

**Vitesse : le critère écrit d'avance n'est pas atteint.**
- K5 : l'exécuteur coopératif vaut 0,8 à 1,3 × le GPU un fil (critère : ≤ 0,5).
- K10 : `domain` vaut 0,93 à 0,94 × le CPU (critère : ≤ 0,85). L'exécuteur gagne tout de même 14 à 26 % sur le GPU
  un fil.

**Ma lecture.** Répartir les sous-arbres des paires laisse la divergence SIMT entière : chaque lane suit son propre
chemin q2/q3/q4. Le gain de J3 sur la v10 venait au contraire d'un parallélisme de données sur les sites.

**Deux défauts de mon refactoring, découverts par cette session.**
- La voie GPU un fil a régressé : écriture ×2, limitée par la latence. Cause : une référence aux tables en mémoire
  locale. Le SASS en témoigne (364 `LD` génériques contre 173). Corrigé : la feuille d'un fil reprend ses tables par
  valeur.
- `atomicOr` était appelé sur la mémoire locale (avertissement de ptxas). Il est désormais réservé aux tables
  partagées.

La session coop2 mesurera la correction (critère : exécuteur GPU un fil à moins de 10 % des valeurs L4) et profilera,
avec Nsight Compute, le noyau un fil à K10 sur ng00, ligne source par ligne source.

**Proposition, à contester avant écriture : la feuille cohérente.** Le warp entier parcourt le même préfixe, avec un
contrôle de flux uniforme.
- Les étapes scalaires (centre, faces J2, `lines_possible`) sont calculées de façon redondante par toutes les lanes :
  même instruction, aucune divergence.
- Recensement : la lane i juge le site i (`side` en i128), puis `__ballot_sync` donne les masques intérieur et
  coquille.
- Support canonique : les triplets et quadruplets de la coquille sont répartis sur les lanes, et le premier succès, dans
  l'ordre, est le plus petit indice gagnant (ballot puis `ffs`).
- Émission par la lane 0, ou listes écrites en parallèle.

**Questions.**
1. Le recensement séquentiel s'arrête au seuil (`p == threshold`), et `census_tests` compte les sites visités
   jusque-là. Je propose de calculer tous les sites, puis de déduire le compteur du rang du seuil-ième intérieur dans
   l'ordre (popc d'un préfixe de masque). De même, `judged` et les compteurs du support canonique se déduiraient de
   l'indice du premier succès. Voyez-vous un compteur dont la valeur dépendrait d'un calcul fait **après** l'arrêt
   séquentiel, en particulier un refus `kUnresolved` qu'un site au-delà du seuil lèverait dans la version parallèle
   mais jamais dans la séquentielle ? Je propose d'ignorer les refus des sites au-delà du point d'arrêt séquentiel.
2. Même question pour la recherche du support canonique : un refus non certifié au-delà du premier succès doit-il être
   ignoré ?
3. Quels mutants exigez-vous ? Je propose : seuil décalé d'un site, refus au-delà de l'arrêt non ignoré, premier
   succès pris au plus grand indice, ballot partiel (masque de lanes incomplet).

## R. Coop3, retrait de la feuille par paires ; supports réduits à l'arbre couvrant d'ordre K (6 octobre, 11 h 43 UTC)

**GPU.** Reçus [coop2](../receipts/developpement_20261006/coop2_correction_mesuree/README.md) et
[coop3](../receipts/developpement_20261006/coop3_reconvergence/README.md).
- Les tables par valeur ne suffisaient pas. La vraie cause de la régression était la reconvergence des warps : la
  boucle `extend` appelait `extend_one`, ce qui faisait passer le noyau d'écriture de 85 à 107 `BSSY`, avec 2,7 fois
  plus d'instructions exécutées et 1,39 fil actif par warp contre 3,47.
- Une fois la boucle d'origine rétablie, coop3 retrouve les valeurs L4 à 1 % près.
- La feuille coopérative par paires est alors plus lente partout. Elle est **retirée** (`d4228f5e5`), comme L4.
- À K10 avec des feuilles de 24, le GPU un fil bat le CPU de 10 à 15 % sur `domain`.
- La suite reste la question Q, le parallélisme sur les sites, qui sera d'abord jugée sur le profil source de coop2.

**Supports : décision de l'utilisateur** (texte exact) : « il ne faut surtout pas représenter tous les supports (pour
le niveau K) mais seulement ceux associés au minimum spanning tree de niveau K ». Choix retenu : les arêtes de Kruskal,
avec S\* seul.

Ce qui change :
- `build_support_hierarchy(..., Selection::spanning)` ne garde que les boules de rôle `birth` et `merge`. Une liaison
  interne relie des K-parties d'un même nœud et fermerait un cycle : elle est retirée. Chaque boule gardée porte son
  seul S\*, lu dans le catalogue.
- Plus d'énumération de Q_b, donc plus de plafond de 24 sites. Seul reste m ≤ 255 (colonne u8).
- Format `MHGP11SP` version 2 : sans `support_count`, et S = B.
- Le manifeste perd `internal`, `multi_support_balls`, `max_supports_per_ball`, `kparties_reliees` et `cofaces`.
- La sélection `all` reste dans la bibliothèque pour les portes S6 existantes.

Portes :
- `mhgp11_cli_supports_oracle` projette l'oracle borné S1 sur l'arbre couvrant : boules sans les internes, S\* au sens
  du catalogue (plus petite arité, puis SiteIdx de Morton).
- Le témoin sphere9 à 25 sites, refusé en version 1, est désormais admis.
- Les deux voies de route concordent toujours à l'octet.

Mesures à K5 (u21, relevé local ; empreintes regravées dans les trois profils) :
- nuages uniformes de 8 000 et 16 000 sites : une boule par nœud (`boules = noeuds`) et `branches = noeuds − 1`.
- ng00 : 576 482 boules pour 576 371 nœuds, contre 789 886 boules en version 1.

Ajout : § 10.10 de `MATHEMATIQUES.md`, qui montre que naissances et fusions suffisent à reconstruire T_K (lemmes B et C).
Mutants : 3 nouveaux, `sp_internes_gardees`, `sp_naissances_retirees` et `sp_etoile_permutee`, plus 2 repointés ; les
5 sont tués.

**Questions.**
1. Kruskal sur un hypergraphe : une fusion qui réunit c ≥ 3 composantes publie une seule boule, avec ses c branches
   dans `PRIOR`. Il ne s'agit donc pas d'un arbre couvrant au sens des graphes (c − 1 arêtes). Je considère cette
   lecture conforme à la demande ; voyez-vous un cas où une fusion porte plusieurs boules au même rang (plateau
   cosphérique) et où ne garder que la première serait plus juste ?
2. Les comptes de Q_b disparaissent de la sortie publiée. Faut-il garder une contre-épreuve du journal
   (`strict_traces`) en mode arbre couvrant, au prix de l'énumération de Q_b et du plafond, ou la contre-épreuve de la
   sélection `all`, déjà couverte par les portes S6, suffit-elle ?

## S. Kruskal intégré, versions du lecteur, u18 regravé ; GPU : J2 mémorisé (6 octobre, 12 h 45 UTC)

**Merci : le cycle de plateau (be8085ec1) était un vrai défaut de ma sélection.** C'était ma question R1, que vous
aviez déjà tranchée. Vos propositions sont intégrées telles quelles, et vos reçus sont cités dans le code et les portes.
- Sélecteur natif (`native/selector.patch`) : `Assembly::select`. Naissances gardées ; fusions retenues par un DSU sur
  les enfants de chaque multifusion, dans l'ordre des `BallIdx`, avec contrôle de connexion finale.
- Différentiel exact (`remaining_1054/proposed.patch`) : `mhgp11_cli_supports_oracle` sélectionne Kruskal sur l'oracle
  complet, puis compare la sortie native intacte.
- Lecteur : contrôle de la structure couvrante (`read_supports_spanning.patch`) et égalité des versions entre manifeste
  et fichier (`version_equality.patch`). Porte `mhgp11_cli_supports_spanning_reader` (cas3).
- Témoin triangle K1 ajouté à la porte CLI : deux supports, à W1 et W4.
- Mutants : `sp_selection_par_role` (retour au filtre de rôle), plus les trois `sp_*` repointés. Les 4 sont tués.
- u18 : vous aviez raison, mon script avait remplacé le mauvais bloc `EQUAL 18`. Les 18 empreintes des trois profils
  sont regravées depuis la sortie corrigée, chaque bloc contrôlé. Les deux voies concordent dans les trois profils, et
  comptes et journal sont identiques entre profils.
- La relecture des dossiers v1 n'est plus garantie : `read_supports` décode encore un binaire v1, mais `check_directory`
  exige désormais un manifeste v2 et la même version. Les dossiers v1 historiques restent attachés à leurs reçus.

R2 : d'accord, pas de réénumération de Q_b en mode arbre couvrant.

Nouvelles lignes (u21) :
- oracle de route : 11 917 boules pour 12 576 nœuds, 1 339 coquilles étendues ;
- ng00 : 576 388 boules pour 576 371 nœuds ; uniforme 32 000 : `boules = noeuds = 1 163 756`, et `branches = noeuds − 1`.

**GPU.** Reçu [j2memo](../receipts/developpement_20261006/j2memo_rangs_locaux/README.md). Le profil source de coop2
fixe les priorités :
- J2 ≈ 38 % du comptage, dont `center_line_meets` 23 %, avec 71 % de succès du cache à K10 ;
- recensement ≈ 15 % ;
- `local_rank` ≈ 7 %.

Deux leviers ont suivi :
- J2 mémorisé dans la feuille device : la même face donne la même issue, et les compteurs sont inchangés ;
- rangs locaux transmis aux puits.

Résultats :
- exécuteur ×0,82 à K5 et ×0,76 à K10 ;
- à K10, `domain` GPU inférieur de 16 à 21 % à celui du CPU ;
- critère écrit d'avance atteint.

La feuille cohérente (votre réponse à Q) ne vise qu'environ 15 % du noyau : je la mets en attente, derrière les étages
fixes de K5.

Votre constat P2 sur les arbres de points (fin de vie du bloc) est noté. Je l'intègre à la prochaine tranche, avec vos
gardes Python et C++.
