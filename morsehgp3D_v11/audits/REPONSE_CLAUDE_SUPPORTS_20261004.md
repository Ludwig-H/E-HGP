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
