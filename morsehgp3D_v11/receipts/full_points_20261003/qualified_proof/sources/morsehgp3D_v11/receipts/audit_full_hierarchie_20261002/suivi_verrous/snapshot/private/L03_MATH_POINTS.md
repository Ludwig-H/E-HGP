# L03 — De la tour à la hiérarchie de points : audit de la v10 pour la conception de la v11

Audit du 2 octobre 2026, rédigé entre 05 h 26 et 06 h 50 UTC (heures lues par `date -u`). Lentille L03 de
l'audit à fond de `morsehgp3D_v10` demandé à l'ouverture de `morsehgp3D_v11`.

```text
phase=audit_v10_pour_conception_v11_hors_registre
backend=cpu_reference (binaires v10 reconstruits depuis afb081774, hors source, sous /tmp/v11-audit/l03_math_points)
profile=quantized_u18_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
GCP non utilisé. Aucune commande git mutante. Dépôt et dossiers privés lus seulement.
```

Vocabulaire. **lu** : constaté à la lecture du code ou d'un document. **exécuté** : rejoué par moi (binaire, script,
oracle). **mesuré** : nombre produit par un calcul fait pour cet audit. **non vérifié** : repris d'un document sans
contrôle. Pour les énoncés : **prouvé** (argument complet relu ou refait ici), **mesuré**, **conjecturé**.
Les audits, la thèse, HGP-old et les notes de mémoire ont servi de pistes, jamais de preuves.

## 0. Réponse courte

1. **La v10 n'a pas de hiérarchie de points « retenue » dans son produit.** Le dépôt publié (afb081774) ne contient
   que trois entrées natives : `core` (C ∩ X), `cover` (première couverture) et `coverE` (boule de K + E sites),
   suivies d'une tête de 172 lignes dont la condensation n'est pas celle de HDBSCAN quand un cluster ne tient que
   par des points à entrée différée (cas de `core`). Tout le reste (majorités, ER0h, vote sur tour condensée, tête
   de la thèse, existence mûre) vit en Python exact dans des dossiers privés, hors dépôt, sans une seule fixture
   gravée dans les portes CTest.
2. **Ce qui est tranché, par preuve ou par mesure recoupée.** Aucune projection ne réunit laminarité, fidélité aux
   amas discrets, précocité, stabilité, respect du cœur et verticalité : registre d'impossibilités à fixtures
   exactes. `core` est exacte, stable (2 ε en rayon), verticale, et tardive : au meilleur bloc elle est sous la
   hiérarchie de HDBSCAN dès K = 2 (0,770 contre 0,811). `cover` garde presque toute la tour au meilleur bloc
   (−0,006 à −0,017 d'IoU en synthétique, −0,002 sur LiDAR) ; elle est discontinue, non verticale, et son départage
   des égalités exactes n'est pas canonique dans le produit. La vérité terrain n'est qu'en partie dans la tour (IoU
   moyen 0,84 en synthétique à K = 5, 24 % des groupes sans amas au-dessus de 4/5).
3. **Ce que les cibles de l'utilisateur imposent.** Les quatre réponses (triangles, voisin proche, filament, chaîne)
   ne sont rendues ensemble ni par `core`, ni par `cover` (74 jugements ancrés sur 125), ni par `cover1` (lecture
   « masse », mesurée ici), ni par la tête de la thèse (50 sur 125). La seule règle à 125 sur 125 est ER0h(1, 12),
   rejouée ici à l'identique. Sa continuité est une conjecture, elle n'est pas lipschitzienne (rejoué), ses deux
   paramètres sont choisis sur ces mêmes 125 jugements, elle perd 0,016 à 0,058 d'IoU au meilleur bloc contre
   `cover` à K = 5 et 10, et elle n'a ni noyau natif ni coût mesuré aux tailles d'intérêt.
4. **Trois faits mesurés ou exécutés dans cet audit.**
   - À entrée égale, **l'avantage de `cover` sur la hiérarchie de HDBSCAN au meilleur bloc est reproduit par une
     hiérarchie d'atteignabilité mutuelle à entrée « bord » et α = 2**, sans tour : écart `cover` − MR₂-bord de
     −0,004 [−0,011 ; +0,002] à K = 5 sur 32 scènes de 2 000 points, −0,011 [−0,025 ; +0,005] sur 16 scènes de 8 000
     points (constat 02). La batterie du 1er octobre n'avait pas ce témoin.
   - **Sur les sept objets des démos Zoltan où HDBSCAN échoue, la tour à K = 5 et 10 échoue aussi** (acquis v10,
     rejoué ici à K = 5). Aux ordres 2 à 4, mesurés ici, **un seul est rattrapé** : le vélo C de la démo 04 à K = 3
     et 4 (IoU 0,57 et 0,63, contre 0,36 au mieux pour HDBSCAN) ; MR₂-bord le rattrape aussi ; et la tour perd à ces
     ordres un objet que HDBSCAN rend (02 C). Au meilleur ordre par objet : 9 objets sur 15 contre 8.
   - **Le principe « un point de bord ne fait pas exister un cluster » n'est tenu par aucune chaîne retenue** dès
     qu'on retire le filament de la fixture : `cover`, `cover1` et ER0h + cohortes créent {amas, x} à 453,471
     (exécuté ici ; déjà relevé par le vérificateur du vote et par la conception de l'existence mûre). L'existence
     des clusters (taille de cœur, sites couverts, maturité) est le vrai verrou restant.
5. **Recommandation.** Implanter d'abord dans le produit v11 la paire exacte et bon marché `core` + `cover`
   canonique (égalités exactes renvoyées à l'ancêtre commun), la publication de la relation de couverture, et la
   condensation par cohortes ; graver les fixtures listées au § 12.4. Garder ER0h comme règle cible derrière des
   obligations de preuve et de coût ; garder l'existence mûre, le vote sur tour condensée, la tête de la thèse et
   toute sélection hors du moteur. Publier toujours les témoins MR-bord à côté de la tour.

## 1. Périmètre lu

| Lieu | Contenu lu |
| --- | --- |
| `/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/` (afb081774) | `docs/CLUSTERING_DEPUIS_LA_TOUR_20260929.md`, `docs/conception/CLUSTER_v2.md` (entier), `PASSATION.md`, `src/tower/tower.hpp` et `tower.cpp` (attaches, lignes 1500 à 1640 et 1753 à 1790), `src/points/`, `src/head/`, `cli/mhgp10_cluster.cpp`, `tests/points/`, `tests/head/`, `tests/regression/test_mreach_border.py`, `bench/frontier/`, `bench/synthetic/scenes.py`, `CMakeLists.txt` |
| mêmes sources, `audits/` | `AUDIT_LAMINARITE_POINTS_20260929.md` (entier), `ANCRAGE_AMBIGUITES.md`, `CONTRE_AUDIT_FIXTURES_PROJECTION_20260929.md`, `AUDIT_HIERARCHIE_KNN_20260929.md`, `AUDIT_ETAT_COURANT.md` (tête), les quatre `REPONSE_CLAUDE_*` des 1er et 2 octobre, `receipts/audit_continu_20260929/point_condensation_20260930/README.md` |
| `/workspaces/E-HGP/build/v10-verrou-points/` | `juge_final/VERDICT_FINAL.md` (entier), `PORTES_ET_REGISTRE.md`, `QUESTIONS_RESTANTES.md`, `REPONSES_UTILISATEUR_20261001.md`, `cibles/CIBLES_REVISEES.md`, `revision_cible/NOTE_REVISION_CIBLE.md`, `revision_cible/QUESTIONS_UTILISATEUR.md`, `SYNTHESE/VERROU_FULL_POINTS.md` (§ 0 à 2), code `juge_final/verdict/` et `verif_echelle_relative/` |
| `/workspaces/E-HGP/build/v10-tour-vers-points/` | `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` (réponse courte, § 1.7, § 2, § 3, § 4, § 5) et ses tables ; `regles2/vote_condense/CONCEPTION.md` et `VERIFICATION.md` ; `regles2/er0h/README.md`, `RAPPORT.md` (§ 1, § 2, § 7.2), `VERIFICATION.md` ; `regles2/these91/README.md`, `RAPPORT.md` (§ 1 à 6) ; `regles2/existence_mure/CONCEPTION.md` (§ 1 à 5) ; `mesure_vc/VERIFICATION.md` ; `selection/RAPPORT.md` ; `DIAGNOSTIC/RAPPORT_DIAGNOSTIC.md` (§ 0) ; `lidar64/` (état) |
| `/workspaces/E-HGP/build/v10-lidar-demos/` | `harnais/RAPPORT_HARNAIS.md`, `regles/resultats/tableaux_regles.md`, `_cache/scenes/LISEZMOI.md` |
| `/workspaces/E-HGP/build/v11-worktree/Zoltan/demos/` | `README.md`, `04_velos_en_rang_avec_sol/resultats_hdbscan_K5.json` |
| thèse | `docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`, pages PDF 47 (définition 8) et 122 à 123 (§ 9.1), relues par `pdftotext` |
| mémoire du développeur | `verrou-full-vers-points`, `clustering-depuis-la-tour`, `hdbscan-echoue-deja-k2`, `ordre-tour-hierarchie-puis-z`, `exposant-z-ponderation`, `sauvegarde-coupure-20261001`, `raccord-audits-v10`, `auditeurs-v10` |
| arrivé pendant l'audit (commits `aa8f0807a` à 05 h 42 et `52687f8e5` à 06 h 11 UTC ; aucun fichier de code, de test ni de documentation de la v10 n'y change : `git diff --stat afb081774 HEAD` vide sur `src`, `cli`, `tests`, `bench`, `reference`, `docs`) | `morsehgp3D_v11/README.md` et `docs/ARCHITECTURE.md` (ouverture de la v11) ; `morsehgp3D_v10/receipts/audit_independant_20261002/maturity_review/README.md` |
| `/workspaces/E-HGP/build/v10-audit-full-hierarchy-20261002/` (audit parallèle de l'auditeur indépendant, même source afb081774, hors dépôt) | `audits/audit_full_hierarchie_20261002/README.md`, `projection/AUDIT_PROJECTION_MATURITE_20261002.md`, `clustering/AUDIT_CLUSTERING_20261002.md`, `receipts/…/cohort_repair/README.md` (tête) : lus comme pistes, reçus non rejoués sauf mention |

Non lu en entier, donc cité seulement par ses résumés : les mémos des trois approches du juge final (`parametres`,
`echelle_relative`, `principe_libre`), `VERIFICATION_mesures.md` et `VERIFICATION_references.md` de la batterie,
`QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md` (404 Ko), les rapports complets `these91` et `er0h` au-delà
des sections citées.

## 2. Méthode

| Moyen | Ce qu'il établit | Où |
| --- | --- | --- |
| Oracle exact indépendant, écrit pour cet audit depuis les seules définitions : Γ_K exhaustif par K-parties et (K + 1)-parties, boules minimales en `Fraction`, amas discrets, entrées `core` et `cover` avec toutes les égalités | la vérité sur les petites fixtures (n ≤ 14) ; aucun import du dépôt ni des dossiers privés | `preuves_l03_math_points/oracle_l03.py` |
| Binaires v10 reconstruits depuis l'arbre de lecture (cmake Release, `-j3`, hors source) | ce que fait le produit publié : entrées natives, tête, départages | `/tmp/v11-audit/l03_math_points/build_v10/` |
| Rejeu en lecture seule du code privé (`-B`, sorties sous `/tmp`) | reproductibilité des reçus du juge final et de la vérification tierce d'ER0h | `preuves_l03_math_points/juge/` |
| Recalcul depuis les tables fusionnées de la batterie | exactitude des agrégats publiés | `recalcul_batterie.py`, `recalcul_lidar.py` |
| Mesures nouvelles : niveau B de la tour contre le témoin d'atteignabilité mutuelle du dépôt (entrées cœur et bord, α = 1 et 2) ; tour et hiérarchies sur les cinq démos à K = 2, 3, 4 | deux questions laissées ouvertes par la v10 | `niveau_b.py`, `lidar_mr.py`, `tab_lidar.py` |
| Port Python des deux condensations (sémantique publiée, cohortes), contrôlé contre la tête réelle et contre scikit-learn sur les fixtures | portée du défaut de la tête sur des hiérarchies produites par le moteur | `impact_condensation.py`, `fix_cover_interne.py` |
| Différentiel du binaire contre l'oracle sur 300 petits nuages (K = 2, 3, 4) | conformité des hiérarchies `core` et `cover` exportées, hors égalités | `differentiel_natif_oracle.py` |
| Extraction des archives embarquées dans les rapports des vérificateurs | ce qui reste rejouable après la vidange de `/tmp` | constat 10 |

Limites de la méthode. L'oracle est borné : il établit la vérité sur les fixtures, il ne re-parcourt aucun théorème.
Les mesures nouvelles sont de petite taille (32 scènes de 2 000 points et 16 scènes de 8 000 points, graines d'audit
9020261002001 et suivantes, hors de tout plan ; cinq trames) : ce sont des indices appariés avec intervalles, pas
des qualifications. Machine partagée, charge 7 à 17 : aucun temps de ce rapport n'a valeur de mesure.

## 3. Définitions exactes des projections candidates

### 3.1 Objets communs

- X : n sites distincts de la grille entière. $L_K(r)=\left\lbrace y:\lvert X\cap\bar{B}(y,r)\rvert\geq K\right\rbrace$. FULL_K : arbre de fusion des composantes de $L_K(r)$ ; nœud v de naissance $b_v$ et de mort $d_v=b_{\mathrm{parent}(v)}$ ; niveaux en rayon carré, rationnels exacts ; coupes fermées ; fusions N-aires.
- Amas discret (thèse, définition 8, relue page PDF 47) : $D_r(C)=X\cap\delta_r(C)=\left\lbrace x\in X:\mathrm{dist}(x,C)\leq r\right\rbrace$. La thèse écrit elle-même que poser $C\cap X$ « aurait été une erreur » et que, hors K = 1, ces amas ne forment pas une partition.
- Couverture : v couvre x au niveau r si $\mathrm{dist}(x,C_v(r))\leq r$. $c_x(v)$ : premier niveau de la vie de v où v couvre x. L'ensemble $V_x$ des nœuds couvrants est clos vers le haut.
- $d_K(x)$ : distance de x à son K-ième site le plus proche, x compté. $\alpha_K(x)$ : rayon de la plus petite boule fermée contenant x et K − 1 autres sites. $d_K/2\leq\alpha_K\leq d_K$ ; à K = 2, $\alpha_2=d_2/2$.
- Caractérisation exacte utilisée par l'oracle (prouvée ici) : les composantes de $L_K(r)$ sont celles du graphe Γ_K (K-parties de rayon ≤ r, reliées par les (K + 1)-parties de rayon ≤ r), et $D_r(C)$ est la réunion des K-parties de la composante. Argument : $L_K(r)$ est la réunion des régions témoins $T_r(F)=\bigcap_{f\in F}\bar{B}(f,r)$, convexes, non vides si et seulement si le rayon de F est ≤ r ; deux régions se coupent en y si et seulement si tous les sites de $F\cup F'$ sont à distance ≤ r de y, et l'on passe alors de F à F′ en échangeant un site à la fois sans quitter y ; si x est à distance ≤ r de $y\in C$, la K-partie formée de x et de K − 1 sites de $\bar{B}(y,r)$ a y dans sa région témoin.
- **Règle ancrée** : chaque point reçoit une date t(x) et un propriétaire o(x), nœud vivant à t(x). x est seul avant t(x), puis suit les ancêtres de o(x). Hauteur de réunion : $u(x,y)=\max\left(t(x),t(y),b_{\mathrm{lca}(o(x),o(y))}\right)$. Toute règle ancrée donne des partitions emboîtées.
- **Condensation par cohortes** à mcs : à chaque rayon, les clusters sont les blocs d'au moins mcs points ; le reste est du bruit à ce rayon. C'est la sémantique de HDBSCAN sur l'ultramétrique de points.

### 3.2 Les candidates

| Nom | Date t(x) | Propriétaire o(x) | Paramètres | Lit mcs ? | Où elle existe |
| --- | --- | --- | --- | --- | --- |
| `core` (C ∩ X) | $d_K(x)$ | composante de $L_K(d_K(x))$ qui contient x | aucun | non | produit : `src/tower/tower.cpp:1600` à 1626 |
| `cover` (première couverture) | $\alpha_K(x)$ | composante du centre de la première boule couvrante ; égalités : plus petit indice du catalogue | aucun | non | produit : `src/tower/tower.cpp:1531` à 1598 |
| `cover1`, `coverE` | $\alpha_{K+E}(x)$ (boule de K + E sites) | composante de $L_K$ qui contient le centre de cette boule | E | non | produit : `TowerParams::cover_extra`, `cli/mhgp10_cluster.cpp` |
| A5 (cover canonique) | $\alpha_K$ si une seule composante couvre x à cette date, sinon naissance de l'ancêtre commun | cette composante, sinon l'ancêtre commun | aucun (η = 0 de la bande) | non | dépôt, hors produit : `bench/frontier/cover_band.py` ; privé : `A5_U1` |
| bande-LCA (η) | max(α², naissance de l'ancêtre commun des témoins forts de rayon ≤ (1 + η) α) | cet ancêtre | η | non | dépôt, hors produit : `bench/frontier/cover_band.py` |
| P_κ (ancrage persistant) | $\max\left(\alpha,\max_j\left(m_j-\kappa(c_j-\alpha)\right)\right)$ sur le code-barres couvrant de x | lignée de première couverture | κ | non | privé : `v10-verrou-points/SYNTHESE` |
| majorités à dénominateur figé (uniforme, 1/β, de bande, de paires, MMg, MMp) | premier rayon où une composante porte plus de θ W_x des poids de témoins de x (W_x compte les témoins futurs) | cette composante | univers, poids, θ, η | non | privé ; audit continu § 8 à 10 |
| MMt(κ, η) | cône $\sup_{\theta}\left(\sqrt{T(\theta)}-\kappa\alpha(2\theta-1)\right)$ sur la masse « temps de couverture écoulé » dans la bande $[\alpha^{2};(1+\eta)\alpha^{2}]$ | lignée majoritaire | κ, η | non | privé : `revision_cible`, `juge_final` |
| ER0(η, κ) | même cône, poids prépayés $\omega_v=\min(d_v,(1+\eta)\alpha^{2})-c_x(v)$ comptés dès le début du nœud | composante de majorité stricte $2m>W$, ancêtre vivant à la date | η, κ | non | privé : `juge_final/verdict` |
| **ER0h(η, κ)** | la même, après héritage proportionnel des poids vers les feuilles de vote (lemme M : $M(c)=M(p)S(c)/(S(p)-\omega_p)$) | idem | η = 1, κ = 12 retenus | non | privé : `juge_final/verdict/er0h.py`, `regles2/er0h/er0h_texte.py` |
| ER0hC, ER0hA | ER0h avec admission à mcs sites couverts : absorption (C) ou admission stricte (A) | idem | η, κ, mcs | **oui** | privé : `juge_final/verdict/er0hc.py` |
| VC[taille, votes, …] (vote sur tour condensée) | jonction du point au cluster de la tour condensée ; dates `un`, `maj`, à marge `+cκ` | routage descendant par majorité ou pluralité à chaque vraie scission | taille (`coeur`, `couv`, `poss`), univers de votes (W, G, T~η, A1, C0), dénominateur, date | **oui** | privé : `regles2/vote_condense`, `vote_condense_v2` |
| tête de la thèse § 9.1 (`these91`) | pas de date de point : masses $m_\tau=S_\tau\sum_{x\in\tau}1/T_x$ sur les facettes, condensation par masse, puis vote $V_x(c)=\sum S_\tau/T_x$ | `vote` : argmax par sélection ; `engage` : facette de plus grande part, fixée une fois | univers (`alg1`, `ferme`), z, seuil de masse | oui (seuil sur la masse) | privé : `regles2/these91` ; texte relu pages PDF 122 à 123 |
| existence mûre M[θ, proj], R[θ, proj] | $\max(e(x),a(g(x)))$ : entrée de la projection, retardée jusqu'à ce que la lignée tienne mcs sites mûrs ; maturité $m(x)=(1-\theta)\alpha_K+\theta d_K$ | premier ancêtre « grand » du propriétaire de la projection | θ, projection | **oui** | privé : `regles2/existence_mure` (conception du 2 octobre, non mesurée) |
| `cdelay[θ]` | $(1-\theta)\alpha_K+\theta d_K$ | lignée de `cover` | θ | non | privé : diagnostic de campagne |

Précisions vérifiées dans le code (lu) :

- `cover` du produit prend, pour chaque site, **la première boule du catalogue dans l'ordre canonique** dont la
  population fermée vaut au moins K + E (`tower.cpp:1561` à 1572, minimum atomique d'indice), puis la composante
  d'une K-partie quelconque de cette boule (`tower.cpp:1551` à 1559). C'est exact pour la date ; le propriétaire
  dépend de l'ordre du catalogue quand plusieurs composantes couvrent x au même niveau (constat 04).
- `cover1` rattache à l'ordre K une boule de K + 1 sites : c'est une lecture « masse » de la couverture, pas la
  définition 8 à l'ordre K.
- La hiérarchie exportée (`PointDendrogram`, `src/points/dendrogram.hpp:17` à 29) est **toute la forêt FULL_K avec
  les points attachés**, niveaux en `double` : 1 065 543 nœuds pour 67 114 sites à K = 5 (démo 01, exécuté). La
  construction par graphe-chemin de `CLUSTER_v2.md` § 4.1 (`jrank`, `entry_node`) n'existe pas dans le code
  (`grep` : aucune occurrence sous `src/` ni `cli/`).

## 4. Propriétés : ce qui est prouvé, ce qui est seulement observé

Axiomes (repris de `SYNTHESE/VERROU_FULL_POINTS.md` § 2, relus) : **L** laminarité ; **NP** tout bloc est dans
l'amas discret de son nœud ; **Can** équivariance par isométrie et renumérotation, aucun départage ; **TI** la règle
ne lit que FULL_K et la relation de couverture ; **Stab** $\lvert u_X(i,j)-u_Y(i,j)\rvert\leq\Lambda\varepsilon$
pour un déplacement apparié d'au plus ε ; **CR** respect du cœur (chaque C ∩ X dans un bloc) ; **V** verticalité
entre ordres ; **E+** entrée immédiate à α.

### 4.1 Tableau des propriétés

| Règle | L | NP | Can | Stab | CR | V | Jugements ancrés (sur 125) | Mode de vérification dans cet audit |
| --- | --- | --- | --- | --- | --- | --- | ---: | --- |
| `core` | prouvé | prouvé | prouvé (C1 : la composante ne dépend pas du départage des ex æquo) | **prouvé**, Λ = 2, constante optimale | prouvé (définition) | prouvé (C3) | 45 | preuves refaites ici ; 45 exécuté (juge natif) |
| `cover` du produit | prouvé | prouvé | **réfuté** aux égalités exactes (constat 04) | **réfuté** : saut (L + δ)/2 pour un déplacement 2 δ | réfuté (`cx_firstcov_k3_n6`) | **réfuté** ((0,1,2,6,9), K = 2 et 3) | 74 | exécuté (oracle et binaire) |
| A5 (cover canonique) | prouvé | prouvé | prouvé par construction | réfuté (même fixture F2) | réfuté | réfuté | 74 | 74 exécuté (juge natif) |
| `cover1` | prouvé | prouvé (le centre de la boule est dans $L_K$) | non, même départage | non étudié | — | — | non jugée | fixtures de base exécutées : T0 et Q1 oui, Q2, Q3, Q4 non |
| P_κ | prouvé | prouvé | prouvé | Λ entre 2 κ et 1 + 2 κ (premier juge) | oui à K = 2 | non | 70 (P_2) | 70 exécuté ; bornes non vérifiées |
| majorité uniforme ou 1/β, dénominateur figé | prouvé (audit continu § 8, relu) | prouvé | prouvé | **réfuté** (contact coquille/intérieur ; frontière de majorité) | réfuté dès K = 2 (I14) | réfuté (I17) | 100 (`maj_bande_unif[1/4]`) | 100 exécuté ; contre-exemples lus |
| MMt(κ, η) | prouvé | prouvé | prouvé | lipschitzienne à constante locale pour κ ≥ √(1 + η) (théorème S_t du dossier privé) ; pas de constante uniforme | réfuté (FX-MC12) | réfuté | 110 (MMt(8, 4/5)) | 110 exécuté (juge natif) ; théorème de continuité lu, non vérifié |
| ER0(η, κ) | prouvé | prouvé | prouvé | **réfuté** : `bascule_repli_thales`, saut 50,683 par unité, proportionnel à l'échelle | réfuté | réfuté | 125 | 125 et saut exécutés (juge natif, script du tiers) |
| **ER0h(1, 12)** | prouvé | prouvé | prouvé | continuité **conjecturée** ; **non lipschitzienne** : saut 36,9 ; 120,8 ; 385,3 par pas de grille aux échelles 10³, 10⁴, 10⁵ | réfuté (voulu : réponse (b) de Q1) | réfuté | **125** | 125 exécuté deux fois (code du juge, reçu identique octet pour octet ; réécriture du tiers, même empreinte) ; sauts exécutés |
| ER0hC (absorption) | prouvé | prouvé | prouvé | comme ER0h | — | — | 125 | proposition E : contrôle exécuté ici, 0 violation (constat 10) |
| ER0hA (admission stricte) | prouvé | prouvé | prouvé | — | — | — | 100 | lu |
| `VC[couv,T~1,abs,maj,sc,maj+c12]` | prouvé | prouvé | oui | **réfuté** par son vérificateur : sauts de 486, 256,5 et 250 pour 1 mm | — | — | 125 | lu ; scripts de ce vérificateur non conservés (constat 10) |
| `VC[coeur,W1]` | prouvé | prouvé | oui | réfuté (frontière de majorité : 5 631,5 → 10 240) | — | — | 20 | lu |
| thèse § 9.1, `vote` | **réfuté** (argmax par sélection) | — | départage déclaré | — | — | — | 50 | texte relu ; témoin abstrait refait à la main |
| thèse § 9.1, `engage` | prouvé (proposition 7 de `these91`) | — | — | — | — | — | 25 | lu |
| M[θ, proj] (existence mûre) | prouvé | prouvé | hérite de la projection | « aussi continue que sa projection » (théorème C du dossier, non vérifié) | — | — | 125 pour θ ≤ 1/16 (`er0ha`), 45 à θ = 1 | non vérifié (conception du 2 octobre, non mesurée) |

Preuves refaites pour cet audit (courtes) :

- **Ancrage ⇒ L.** Deux blocs à un même rayon sont des images réciproques, par $x\mapsto\mathrm{anc}_r(o(x))$, de nœuds vivants distincts : ils sont disjoints ; quand r croît, un ancêtre réunit des blocs et un point entre dans un bloc, aucun bloc ne se scinde.
- **NP pour `cover` et A5.** Le propriétaire couvre x à sa date ; si v couvre x à ℓ, l'ancêtre de v vivant à ℓ′ ≥ ℓ contient $C_v(\ell)$ et couvre donc x à ℓ′.
- **Stabilité de `core`.** Si $\lvert x_i-y_i\rvert\leq\varepsilon$, alors $L_K^X(r)\subseteq L_K^Y(r+\varepsilon)$ (on transporte les K témoins d'une boule) et le segment $[x_i,y_i]$ est dans $L_K^Y(r+2\varepsilon)$ dès que $x_i\in L_K^X(r)$ ; d'où $u^Y\leq u^X+2\varepsilon$, puis la symétrie. Borne en rayon, à effectif et identifiants appariés, K fixé.
- **Encadrement de HDBSCAN (C5, relu et refait).** Avec $\mathrm{MR}^{1}_K(\varepsilon)$ le graphe $\max(r_K(x),r_K(y),d(x,y))\leq\varepsilon$ : $\Pi_K(r^{2})\sqsubseteq\mathrm{MR}^{1}_K(2r)$ et $\mathrm{MR}^{1}_K(\varepsilon)\sqsubseteq\Pi_K(9\varepsilon^{2}/4)$. À K = 2 l'atteignabilité mutuelle est la liaison simple (L2).
- **Vote non laminaire.** x porte 0,3 ; 0,3 ; 0,4 dans les classes de c1, c2 (frères, de parent c) et d. Sélection {c1, c2, d} : x vote d. Sélection {c, d} : x vote c. Les blocs « d » et « c » se croisent.

### 4.2 Contre-exemples et fixtures : état et rejeu

Colonne « rejeu » : **O** oracle indépendant de cet audit, **B** binaire v10 reconstruit, **J** code privé rejoué,
**—** lu seulement.

| Fixture (coordonnées exactes) | Ce qu'elle établit | Rejeu | Gravée dans une porte CTest du dépôt ? |
| --- | --- | --- | --- |
| F1 : {0, 2, 4} sur une droite, K = 2 | le site 2 est couvert au même niveau par deux composantes : toutes les couvertures, l'affectation exclusive immédiate et la laminarité sont incompatibles (I1) | O, B : le binaire rend {0, 2} à r = 1, jamais {2, 4} | non (reçu d'auditeur) |
| F2 : {0, 999, 2000} et {0, 1001, 2000}, K = 2 | discontinuité de `cover` : u(gauche, milieu) passe de 499,5 à 1000 ; `core` de 999 à 1001 | O, B | non |
| F3 : {0, 20, 22, 50, 52} | K = 1 à r = 10 donne {0, 20, 22} ; K = 2 à r = 15 donne {20, 22, 50, 52} : la réunion libre des ordres n'est pas laminaire | O | non |
| (0, 1, 2, 6, 9), K = 2 et 3, r = 3 | `cover` n'est pas verticale : K = 2 donne {0,1,2} \| {6,9}, K = 3 donne {0,1,2,6} | O (η = 0) | non |
| deux triangles P1 : A(268,3000,0) B(268,1000,0) C(2000,2000,0) D(4000,2000,0) E(5732,3000,0) F(5732,1000,0) | FULL_2 : AC, BC, DE, DF à 999,978 ; sept paires à 1000 ; ABC \| CD \| DEF à 1154,684 ; fusion à 1931,827. `core` : rien avant 1999,956 | O, B | non |
| P2 (pont 1998) et T1_1700 (pont 1700 : D(3700,…), E(5432,…), F(5432,…)) | `cover` rend AB \| CD \| EF ; `core` rend CD seule ; scikit-learn HDBSCAN (K = 1 ou 2, mcs 2 ou 3) rend tout en bruit | O, B, scikit-learn 1.9.1 | non |
| Q2 : x(1000,1000,1000) a(1100,1000,1000) b1(1010,1120,1000) b2(1010,1119,1016), K = 2 | `cover` : {x, a} dès 50 ; `cover1` : {x, b1, b2} à 60,36 ; `core` : tout à 100 | B | non |
| Q3 (filament de pas 700 contre amas de 8 points à 900), K = 2 | `cover` et `core` : x avec le filament ; `cover1` : x dans l'amas à 453,471 | B | non |
| Q4 (chaîne C, m, D entre deux tétraèdres), K = 3 | `cover` : chaîne dès 692,820 ; `cover1` : CPQR \| DP2Q2R2 dès 866,025 ; `core` : aucun bloc avant 1385,641, après la racine de FULL_3 (1334,166) | B | non |
| `cx_firstcov_k3_n6` : (2,4,4)(2,8,5)(2,9,1)(3,7,0)(6,9,6)(8,10,7), K = 3 | à a = 18, x₄ est seul dans C ∩ X (facette isolée {1,4,5}, β = 11) alors que sa première couverture {1,2,4} (β = 41/4) le met avec {1,2} : `cover` ≠ C ∩ X et viole CR | O | non |
| `cx_E5`, `cx_four_L11F1`, `cx_square_K2`, `line5`, `cx_fold_k2_n6`, `cx_fold_k3_n5` (`CLUSTER_v2.md` § 12.2) | niveaux de fusion de C ∩ X : 18, 36, 62 ; 225, 468 ; 4 ; 1, 16 ; 190/7 ; 54 | O : valeurs du document retrouvées pour ces six fixtures et pour `cx_firstcov_k3_n6` | **non** (constat 09) |
| `line5` : (0,0,0)(1,0,0)(5,0,0)(9,0,0)(10,0,0), mcs = 2 | scikit-learn n'est pas équivariant par permutation : le point du milieu va à gauche ou à droite selon l'ordre d'entrée | scikit-learn 1.9.1, trois algorithmes | non |
| A : trois points admis à β = 1, 4, 9 ; B : deux points à 16 ; fusion à 25 ; mcs = 2, z = 1 | condensation : stabilité de A = 11/15 ; la tête publiée rend 37/30 et, racine sélectionnable, rend A et B là où scikit-learn rend un seul cluster | B (`driver_condense.cpp`), scikit-learn | non (reçu d'auditeur) |
| `internal_k3` : (15,4,0)(5,4,0)(7,8,0)(7,0,0)(1,4,0)(0,4,1), K = 3 | entrée interne de `cover` : le site (15,4,0) est couvert à β = 25 par une boule de quatre sites cocycliques et entre dans la racine née à 169/9 ; à mcs = 6 la tête publiée rend 88/65 au lieu de 6/5 | O, B | non (reçu d'auditeur ; `tests/points/test_cover_band_native.py` non câblé) |
| 0, 1, L, L + 1 (L = 4 à 100), K = 2 | majorité uniforme à dénominateur figé : aucune des deux paires avant la fusion ; poids 1/β : les deux paires dès β = 1/4 | — (raisonnement refait à la main pour L = 100) | non |
| tétraèdre C(0,0,0) A(4M,0,0) B(0,5M,0) D(0,0,100M), C déplacé en (1,1,1) | majorité 1/β : saut de 6,557 m à 4,096 m pour 1,7 mm (contact coquille/intérieur) | — | non |
| cinq points collinéaires 0, 1024, 11263 \| 11264, 21504, 22528, K = 2, mcs = 2 | condenser puis affecter une fois : réunion de 5 631,5 à 10 240 pour 1 mm ; la date à marge d'ER0h ramène l'écart à κ/η unités | J : ER0h 12,01 par unité, mutant sans marge 4 608,5 | non |
| `bascule_repli_thales` : y(1000,1000,0) z(3000,1000,0) q(1200,1600,0) → (1200,1601,0), K = 2 | ER0 est discontinue ; ER0h saute de 0,5 aux trois échelles | J | non |
| famille à cinq points : x(1155,1732,0) y(0,0,0) y′(155,0,0) z(2155,0,0) z′(2310,0,0), x → (1154,1732,0) | ER0h : réunion de x et y′ de 1 077,5 à 1 040,618 pour un pas de grille ; loi $\sqrt{\kappa a\delta/8}$ | J | non |
| amas de 8 points + x à 900 + groupe lointain, K = 2, mcs = 9 | un point de bord fait exister un cluster de 9 points à 453,471 pour `cover`, `cover1`, ER0h + cohortes ; à 900 pour `core` | B, J (constat 06) | non, et absente des 125 jugements |
| trois contre-fixtures du vérificateur du vote (six à huit sites, K = 2, mcs = 3 ; coordonnées dans `regles2/vote_condense/VERIFICATION.md` § 9) | sauts de 486, 256,5 et 250 de la date à marge du vote pour 1 mm | — : scripts de ce vérificateur non conservés | non |
| témoin de contact à cinq points (0,0,0), (10000,0,0), (130000,0,1), (140000,0,0), (70000,104000,0), K = 2 | noyau flottant d'ER0h : propriétaire faux à une marge exacte de 2,0 · 10⁻¹⁰, meilleur bloc 2/3 au lieu de 3/3, coupe plate changée à mcs = 2 ; le chemin certifié rend l'exact | J : script `t9_temoin_contact.py` extrait de l'archive du vérificateur | non |

## 5. Cibles fixées par l'utilisateur

Sources : `revision_cible/QUESTIONS_UTILISATEUR.md` (fixtures et options), `juge_final/REPONSES_UTILISATEUR_20261001.md`
(réponses recopiées), notes de mémoire `verrou-full-vers-points` et `ordre-tour-hierarchie-puis-z`. Les réponses du
30 septembre ne sont consignées que dans la mémoire du développeur et dans `CIBLES_REVISEES.md` : il n'existe pas
de fichier daté qui les recopie comme celles du 1er octobre.

| Cible | Fixture | Réponse de l'utilisateur | Ce qu'elle exclut |
| --- | --- | --- | --- |
| T0 (30 septembre) | deux triangles de la thèse § 6.1, K = 2 | « d'abord les deux triangles avant la fusion » : ABC \| DEF | lue aussi sur le pont plus court de 0,1 % (P2) : exclut la fidélité non ambiguë E_u, toute lignée de première couverture (`cover`, A5, P_κ), `core` |
| Q1 (b) | pont de 1700 contre côtés de 2000 | les triangles, « en fait cela dépend de K », puis : « cela dépend de min_cluster_size. On ne doit faire apparaître les clusters qu'à partir de min_cluster_size » | le respect du cœur sur [1700 ; 1787,4[ |
| Q2 (a) | voisin a à 100 contre paire serrée à 120 | x avec a | les votes qui comptent les faces (W, G, masse des sites) |
| Q3 (a) | bout de filament contre amas dense | x dans le filament | les majorités de masse à bande large |
| Q4 (a) | chaîne serrée entre deux tétraèdres, K = 3 | attendre, puis la chaîne CmD avant la racine | la transposition « masse » des triangles (`cover1`, MMg) |
| Q1bis (1er octobre) | T1_1700 à mcs = 2 | les triangles, comme à mcs ≥ 3 | toute règle à filtre d'admissibilité par mcs (ER, MMtA) |
| Q-Π2 (1er octobre) | « un point de bord peut-il, à lui seul, faire exister un cluster ? » | **non, aucun cluster**, « cela dépend de K et de min_cluster_size » | la complétion d'un amas de mcs − 1 points par un point couvert |
| Q2bis, Q3bis | cellules à mcs plus grand | pas d'option : « je t'ai dit de faire des tests par rapport au Lidar dans Zoltan/ » | les nouvelles questions synthétiques |
| ordre de travail (1er octobre, 18 h 20) | — | la tour, puis la hiérarchie contre celle de HDBSCAN, z en dernier | les coupes plates réglées présentées avant A et B |
| exigences de test | `build/workflows/EXIGENCES_TESTS_UTILISATEUR.md` | beaucoup de synthétique (n, difficulté, nombre de groupes), vérité et MAP, précision et rappel, LiDAR Zoltan ; « il est impératif d'être meilleur que HDBSCAN » | — |
| proposition (1er octobre, 14 h 25) | — | propriétaire calculé sur la tour FULL condensée « au sens analogue de HDBSCAN », par vote pondéré sur les faces de x, « seulement à partir du moment où le cluster contient au moins min_cluster_size points » | — |

Rôle de `min_cluster_size` qui s'en déduit : la cible est une hiérarchie **condensée** (un groupe n'est un cluster
qu'à partir de mcs points) ; la projection, elle, ne doit pas dépendre de mcs (Q1 et Q1bis donnent la même réponse
à mcs 2 et 3). C'est le pipeline du juge final : FULL_K, projection sans mcs, condensation à mcs.

Deux tensions que les réponses ne lèvent pas :

1. Q1 (b) viole le cœur et coûte les petits groupes : c'est la même décision appliquée à un groupe de K points
   voisin d'un grand amas (mesuré par le juge : 0,26 contre 0,41 pour `cover` à K = 5).
2. « Tour condensée au sens analogue de HDBSCAN » se lit par la taille de cœur |C ∩ X| ≥ mcs. Avec cette taille,
   les deux triangles n'existent jamais (à K = 2 aucun sommet n'est dans la composante avant la fusion : 20
   jugements sur 125). Avec la taille « sites couverts », les triangles existent, mais un point de bord fait
   exister un cluster (Q-Π2 violée hors de la cellule gravée, constat 06). L'utilisateur a fixé les deux exigences.

## 6. Résultats mesurés et leur degré de vérification

Trois niveaux, dans l'ordre fixé par l'utilisateur. **A** : meilleur amas discret de FULL_K par cible. **B** :
meilleur bloc d'une hiérarchie de points par cible. **C** : coupe plate. Trois lectures : présence exacte,
approximation (parts au-dessus de 1/2 et 4/5, IoU moyen), compatibilité (antichaîne). Tout est sur graines de
développement et trames choisies : **aucune confirmation scellée n'existe pour les règles du 1er et du 2 octobre**.
Les seuls tests préenregistrés sont les lots A et C des 28 et 29 septembre, joués avec la tête publiée, dont la
condensation s'écarte de celle de HDBSCAN pour l'entrée `core` (constat 03 : écart sans effet mesuré sur les
étiquettes d'un échantillon).

### 6.1 Niveau A : la tour

Source : `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` (sha256 `1640fa47…`, deux vérificateurs
`confirme_avec_reserves`). Les lignes marquées † sont **recalculées ici** depuis `tables/groupes_A_B.csv.gz` et
`tables/lidar_instances.csv.gz` : valeurs identiques.

| Cibles | Population | Exactes | Part > 4/5 | IoU moyen |
| --- | --- | ---: | ---: | ---: |
| groupes du générateur, K = 2 † | 14 400 groupes, 1 728 scènes | 3 000 | 0,730 | 0,823 |
| groupes du générateur, K = 5 † | idem | 2 930 | 0,756 | 0,839 |
| groupes du générateur, K = 10 † | idem | 2 801 | 0,763 | 0,847 |
| classes MAP, K = 5 | 12 794 classes, n ≤ 8 000 | 4 269 | 0,789 | 0,870 |
| bloc iid (MAP exactement optimal), meilleur K | 488 classes | 2 | 0,412 | 0,715 |
| instances LiDAR ≥ 50 points, K = 5 † | 728 instances, 64 trames | 314 | 0,865 | 0,923 |
| instances LiDAR, K = 10 † | idem | 297 | 0,843 | 0,918 |

Ce que la tour ne contient pas (lu, vérifié par les deux vérificateurs de la batterie) : niveaux `hard` et
`extreme` (0,663 et 0,450 des groupes au-dessus de 4/5), familles `unbalanced` et `heteroscedastic`, mélanges iid à
3 écarts-types (aucune classe au-dessus de 4/5 : la tour sort le cœur de chaque composante), objets LiDAR en contact
(à moins de 0,1 m : 0 exacte sur 42), vélos (IoU 0,687). À K fixe, l'IoU baisse quand n monte : −0,040
[−0,069 ; −0,016] de 500 à 32 000 points à K = 5. Le moteur est borné à K ≤ 10.

### 6.2 Niveau B : les hiérarchies de points natives contre celle de HDBSCAN

IoU moyen du meilleur bloc, 14 400 groupes par ordre († recalculé ici) :

| K | tour (A) | `cover` | `cover1` | `core` | HDBSCAN | `cover` − HDBSCAN apparié par scène † | scènes + / = / − † |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 2 † | 0,823 | 0,817 | 0,820 | 0,770 | 0,811 | +0,0053 | 976 / 180 / 572 |
| 3 † | 0,830 | 0,822 | 0,824 | 0,757 | 0,797 | +0,0238 | 1 185 / 156 / 387 |
| 5 † | 0,839 | 0,827 | 0,828 | 0,747 | 0,776 | +0,0495 | 1 241 / 140 / 347 |
| 10 † | 0,847 | 0,830 | 0,831 | 0,739 | 0,751 | +0,0767 | 1 260 / 125 / 343 |

LiDAR, 728 instances († recalculé ici) : K = 5 : tour 0,9227, `cover` 0,9205, HDBSCAN 0,9134, `core` 0,8997 ;
K = 10 : 0,9178, 0,9143, 0,8990, 0,8848. Par rôle de trame à K = 5 : `cover` − HDBSCAN = +0,0104 sur les 39 trames
choisies sur les échecs de HDBSCAN (29 trames pour, 10 contre), −0,0014 sur les 20 trames témoins (10 / 3 / 7).
Pondéré sur les 299 trames du criblage : +0,0002 [−0,005 ; +0,004] à K = 5 (lu). Échecs enregistrés de HDBSCAN
(49 instances des trames « échec ») : la tour en passe 30 au-dessus de 1/2, `cover` 26, `cover1` 25, HDBSCAN
recalculé 18, `core` 13 ; au même ordre que l'échec : 35, 27, 27, 0 et 7 couples sur 80 († recalculé ici).

Lectures établies :

- `cover` et `cover1` gardent la tour à 0,02 près ; `core` perd 0,05 à 0,11 et reste **sous** la hiérarchie de
  HDBSCAN à chaque ordre mesuré, de 2 à 10 (loi de la demi-lacune : ses points entrent tard par rapport aux fusions).
- HDBSCAN reste devant dans `shells` et `hierarchical` bruitées (jusqu'à −0,014, par la précision) et en présence
  exacte sous 20 % de bruit : `cover` prend dans son bloc un point de bruit couvert tôt (à bruit ignoré, `cover`
  retrouve 932 groupes exacts sur 1 800 contre 798 pour HDBSCAN, lu).
- Les niveaux de difficulté du banc sont calibrés sur l'échec de HDBSCAN (ARI de sa coupe plate 0,95 / 0,70 /
  0,40 / 0,15) : `hard` et `extreme` sont par construction ses mauvais cas.
- Sur LiDAR à K = 5, hors des trames choisies sur ses échecs, la hiérarchie de HDBSCAN n'est pas derrière.

### 6.3 Les règles candidates au niveau B (toutes lues ; aucune dans la batterie vérifiée)

| Règle | Écart au meilleur bloc contre `cover` | Source et degré de vérification |
| --- | --- | --- |
| ER0h(1, 12) | −0,001 ; −0,004 ; −0,016 à K = 2, 3, 5 (n = 300 et 2 000) ; −0,030 à K = 10 ; −0,019 et −0,043 à n = 8 000 (4 scènes) ; trames : −0,029 et −0,058 à K = 5 et 10 ; groupes de moins de 2 K points : 0,24 contre 0,40 | `regles2/er0h/RAPPORT.md` ; vérification adverse restée à l'**état intermédiaire** (1 810 cellules de tableaux recalculées par elle, 0 écart) |
| `VC[couv,T~1,…,maj+c12]` | vaut `cover` à mcs = K ; −0,01 à −0,03 quand mcs monte | `regles2/vote_condense/VERIFICATION.md` (mesure du vérificateur, 208 unités) |
| thèse § 9.1, engagement, z = 2 | −0,001 à −0,009 (n = 300) ; +0,012 / +0,037 / +0,068 contre HDBSCAN | `regles2/these91/RAPPORT.md` ; vérification non relue ici |
| majorités de bande, MMt, P_2, ER | aucune ne dépasse `cover` de plus de 0,005 ; plusieurs sont 0,01 à 0,05 dessous | `DIAGNOSTIC/RAPPORT_DIAGNOSTIC.md` § 4.3, n = 300 |

Conclusion mesurée, que je reprends : **au niveau B, la projection n'est pas un levier** ; la raison d'être des
règles à majorité est la conformité aux cibles de l'utilisateur, que `cover` échoue.

### 6.4 Niveau C : coupes plates (mises de côté par l'utilisateur ; lues)

- Étage sélection (768 scènes dev, chaque côté choisit sa configuration sur une grille de 120, intervalles hors
  sac) : `cover` 0,758 à 0,761 contre HDBSCAN 0,703 à 0,716 de mIoU. Sur les cinq trames Zoltan : `cover` −0,018
  et −0,023. Le témoin `mreach` (arbre de HDBSCAN, tête de la tour à exposant z) rattrape +0,039 des +0,062 :
  l'essentiel du gain est un effet de tête.
- À z = 1 fixe, sans epsilon ni remplissage (mesure du vote, vérifiée) : n = 2 000, K = 5, mcs = K / 10 / 20 / √n :
  `cover` 0,447 / 0,656 / 0,709 / 0,727 ; HDBSCAN 0,580 / 0,642 / 0,648 / 0,658 ; `VC[coeur,W1]` 0,690 / 0,721 /
  0,711 / 0,703. Sur les cinq trames à mcs = K : `cover` 0,450 avec 5 732 clusters par trame, HDBSCAN 0,580 avec
  867, `VC[coeur,W1]` 0,601 avec 1 223.
- Mécanisme de la sur-segmentation de `cover` à mcs ≤ K (prouvé en position générale, relu) : la première boule
  couvrante d'un point est une naissance d'ordre K qui contient exactement K sites ; chaque naissance dont les
  K sites l'ont pour première couverture est une feuille de masse K, admise dès que mcs ≤ K.
- La condensation garde l'information : oracle d'antichaîne 0,914 sur l'arbre brut de `cover` et 0,912 à 0,914
  après condensation (F1 objets, K = 5). La perte à la coupe EOM (0,05 à 0,10 de mIoU à n = 300) est dans une autre
  métrique : l'auditeur a demandé de ne pas soustraire les deux nombres, et le développeur a retiré la comparaison.
- scikit-learn n'est pas déterministe d'une machine à l'autre à K ≥ 3 : tri non stable des arêtes ex æquo dans
  `_process_mst` ; 68 à 87 % des configurations d'étiquettes changent selon l'ordre des égalités, les moyennes
  bougent de 0,003. Vérifié ici sur `line5` : le point du milieu change de côté avec l'ordre d'entrée.

### 6.5 Mesures nouvelles de cet audit

**(i) Qui fait le gain de `cover` sur HDBSCAN : la tour, ou l'entrée ?** Même évaluateur de meilleur bloc
(`niveau_b.py`), mêmes sites, binaire v10 de afb081774 pour la tour, témoin d'atteignabilité mutuelle du dépôt
(`tests/head/mreach.cpp`, entiers exacts, plateaux N-aires) pour HDBSCAN : MR₁-cœur est la hiérarchie de HDBSCAN,
« bord » est la règle des points-bord (analogue de `cover`), α = 2 divise les distances de paire par deux.
32 scènes (8 familles, medium et hard, bruit 0 et 0,1), n = 2 000, 8 groupes, 256 groupes par ligne. Contrôle : à
K = 1 les six hiérarchies donnent les mêmes meilleurs blocs (8 scènes, identité exacte).

| K | `cover` (tour) | `core` (tour) | MR₁-cœur (HDBSCAN) | MR₁-bord | MR₂-bord | `cover` − HDBSCAN [IC 95 %] | MR₁-bord − HDBSCAN | `cover` − MR₂-bord [IC 95 %], scènes + / = / − |
| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- |
| 2 | 0,819 | 0,757 | 0,807 | 0,807 | 0,823 | +0,012 [+0,005 ; +0,021] | 0 (identiques à K = 2) | −0,004 [−0,010 ; +0,003], 10 / 4 / 18 |
| 5 | 0,837 | 0,737 | 0,770 | 0,817 | 0,841 | +0,067 [+0,049 ; +0,085] | +0,047 [+0,035 ; +0,059] | −0,004 [−0,011 ; +0,002], 14 / 4 / 14 |
| 10 | 0,846 | 0,731 | 0,747 | 0,826 | 0,839 | +0,099 [+0,073 ; +0,126] | +0,079 [+0,059 ; +0,100] | +0,007 [−0,001 ; +0,017], 14 / 4 / 14 |

À une taille d'intérêt, n = 8 000, K = 5, 16 scènes `hard` (bruit 0 et 0,1), 128 groupes : `cover` 0,787 ; `core`
0,692 ; HDBSCAN 0,720 ; MR₁-bord 0,767 ; MR₂-bord 0,799. `cover` − HDBSCAN = +0,067 [+0,042 ; +0,091] ;
`cover` − MR₂-bord = −0,011 [−0,025 ; +0,005], MR₂-bord devant dans 12 scènes sur 16.

Lecture : l'écart `cover` − HDBSCAN de la batterie (+0,05 à K = 5, +0,08 à K = 10) se retrouve ici (+0,07 et +0,10
sur des scènes medium et hard). Il se décompose en « entrée bord » (+0,05 et +0,08), « α = 2 » (+0,02 et +0,01) et
« connexité exacte de la multicouverture » : **rien de mesurable** à K = 2 et 5, +0,007 non séparé de zéro à
K = 10. C'est le résultat du lot C préenregistré du 29 septembre (famille « objet », coupes plates à même tête :
aucune différence à K = 2, 3, 5, 8 ; +0,010 à K = 10, `receipts/test_cover_C_20260929/README.md`), retrouvé ici
au niveau de la hiérarchie, sans tête. La batterie du 1er octobre ne portait plus ce témoin.

**(ii) Les démos Zoltan aux ordres 2 à 4.** Outil natif du harnais (`harnais/natif/lidar_iou.cpp`) recompilé contre
la bibliothèque de afb081774 ; scènes préparées du cache (hors dépôt, lues seulement) ; IoU au sens de la PQ.
Contrôles : à K = 5, les cinq démos rejouées donnent les valeurs publiées par le harnais (démo 01 : 0,707 / 0,422 /
0,856 au plafond et 0,698 / 0,412 / 0,846 pour `cover` ; démo 04, objet C : 0,462 et 0,458).

| Démo (objets A / B / C) | K | plafond de la définition 8 | `cover` | `core` | HDBSCAN publié (Zoltan) |
| --- | ---: | --- | --- | --- | --- |
| 01 vélos en rang | 2 | 0,604 / **0,415** / 0,657 | 0,594 / **0,413** / 0,656 | 0,580 / 0,405 / 0,641 | K = 5 : 0,67 / **0,41** / 0,75 |
| | 3 | 0,649 / **0,476** / 0,841 | 0,642 / **0,478** / 0,832 | 0,577 / 0,425 / 0,734 | |
| | 4 | 0,709 / **0,424** / 0,856 | 0,683 / **0,421** / 0,837 | 0,663 / 0,416 / 0,705 | |
| 02 vélos contre façade | 2 | **0,390** / **0,245** / 0,696 | **0,362** / **0,224** / 0,673 | 0,286 / 0,204 / 0,551 | K = 5 : **0,31** / **0,18** / 0,60 |
| | 3 | **0,352** / **0,337** / 0,442 | **0,293** / **0,296** / 0,443 | 0,132 / 0,286 / 0,383 | |
| | 4 | **0,393** / **0,357** / 0,445 | **0,414** / **0,306** / 0,447 | 0,212 / 0,286 / 0,387 | |
| 03 piéton contre façade | 2 | **0,444** / 1,000 / 0,980 | **0,450** / 1,000 / 0,980 | 0,444 / 1,000 / 0,980 | K = 5 : **0,44** / 1,00 / 0,98 |
| | 3 | **0,452** / 1,000 / 0,980 | **0,459** / 1,000 / 0,980 | 0,444 / 1,000 / 0,980 | |
| | 4 | **0,460** / 1,000 / 0,980 | **0,459** / 1,000 / 0,980 | 0,426 / 1,000 / 0,980 | |
| 04 vélos en rang, sol conservé | 2 | **0,460** / **0,269** / **0,386** | **0,451** / **0,262** / **0,382** | 0,398 / 0,234 / 0,361 | K = 1 à 10 : au plus 0,30 / 0,31 / **0,36** |
| | 3 | **0,324** / **0,343** / 0,566 | **0,304** / **0,327** / 0,565 | 0,261 / 0,287 / 0,495 | K = 3 : 0,29 / 0,30 / 0,35 |
| | 4 | **0,298** / **0,308** / 0,629 | **0,271** / **0,310** / 0,630 | 0,211 / 0,252 / 0,485 | K = 4 : 0,28 / 0,31 / 0,34 |
| 05 témoin voitures | 2 à 4 | ≥ 0,866 / 0,983 / 0,997 | ≥ 0,864 / 0,983 / 0,994 | ≥ 0,859 / 0,983 / 0,994 | K = 5 : 0,86 / 0,98 / 0,99 |

En gras : les sept objets en échec enregistré de HDBSCAN quand ils restent ≤ 1/2. « Plafond » est le nom du
harnais pour le meilleur amas discret de la définition 8 ; ce n'est pas un majorant des blocs d'une hiérarchie (un
bloc inclus dans un amas discret peut avoir un meilleur IoU : 02 A à K = 4), il ne borne que leur rappel.
**Un seul est rattrapé** : le
vélo C de la démo 04 à K = 3 (181/320 au plafond, 178/315 pour `cover`) et à K = 4 (175/278 et 174/276), alors que
la courbe publiée de HDBSCAN reste sous 0,37 pour K = 1 à 10. À K = 5 il retombe à 0,46, à K = 10 à 0,38 (harnais).

**Dans l'autre sens**, aux mêmes ordres 3 et 4, la tour **perd** le vélo C de la démo 02 (plafond 0,442 et 0,445,
`cover` 0,443 et 0,447) que HDBSCAN rend à ces ordres (0,667 et 0,619) et que la tour rend à K = 2 (0,696) et à
K = 5 (0,701). Bilan au seuil 1/2 sur les 60 couples (objet, ordre de 2 à 5), `cover` contre la courbe publiée de
HDBSCAN au même ordre : 30 réussis par les deux, 26 par aucun, 2 gains de `cover`, 2 pertes. Au meilleur ordre par
objet : `cover` (K = 2 à 5) passe 1/2 pour 9 objets sur 15, HDBSCAN (K = 1 à 10) pour 8 ; le meilleur IoU de
`cover` est au moins celui de HDBSCAN pour 14 objets sur 15 (02 C : 0,687 contre 0,708)
(`preuves_l03_math_points/lidar/lidar_demos_K2_K5.txt`).

Le même objet dans la hiérarchie d'atteignabilité mutuelle (témoin du dépôt, évaluateur `lidar_mr.py` ; contrôle :
MR₁-cœur redonne les valeurs publiées de Zoltan sur la démo 03 à K = 5 et sur la démo 04 à K = 3 et 4) :

| Démo 04, objet C | HDBSCAN (MR₁-cœur) | MR₁-bord | MR₂-cœur (`alpha` = 2 de scikit-learn) | **MR₂-bord** | `cover` (tour) | plafond (tour) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| K = 3 | 0,348 (79/227) | 0,361 (82/227) | 0,437 (131/300) | **0,568** (176/310) | 0,565 (178/315) | 0,566 (181/320) |
| K = 4 | 0,341 (77/226) | 0,357 (81/227) | 0,396 (105/265) | **0,619** (169/273) | 0,630 (174/276) | 0,629 (175/278) |

Lecture : sur le seul objet rattrapé, il faut les deux ingrédients de `cover`, la fusion à la demi-lacune et
l'entrée par couverture. HDBSCAN standard échoue, avec `alpha` = 1 comme avec `alpha` = 2 ; une hiérarchie
d'atteignabilité mutuelle à α = 2 **et** entrée bord, sans tour, fait jeu égal avec la tour.

### 6.6 Ce qui est vérifié, et par qui

| Famille de résultats | Vérification existante | Vérification de cet audit |
| --- | --- | --- |
| batterie A et B (synthétique, MAP, LiDAR) | deux vérificateurs adverses, `confirme_avec_reserves` ; scripts de l'un archivés dans son rapport, ceux de l'autre non conservés | agrégats de tête recalculés depuis les tables fusionnées : identiques ; les cinq démos rejouées à K = 5 par exécution |
| 125 jugements ancrés | juge (deux chemins), tiers, vérificateur | rejoués : chemin Γ_K (reçu identique), chemin natif, réécriture du tiers ; témoins `cover` 74, A5 74, `core` 45, P_2 70, MMt(8, 4/5) 110, `maj_bande_unif[1/4]` 100 |
| proposition E (tour condensée = projection puis cohortes) | preuve dans `VERDICT_FINAL.md` § 2.6 ; contrôle du juge **jamais enregistré** (marqueur non rempli) ; tiers et vérificateur l'ont rejouée | contrôle exécuté ici : 71 scènes, 408 couples, 0 violation |
| non-lipschitzianité d'ER0h, discontinuité d'ER0 | tiers ; vérificateur (rapport intermédiaire) | rejouées par les scripts du tiers |
| mesure du vote sur tour condensée (niveau C) | vérificateur `confirme_avec_reserves` | non vérifié |
| sélection, diagnostic, tête de la thèse, existence mûre | internes ; existence mûre : conception seule | non vérifié |
| fixtures de `CLUSTER_v2.md` § 12.2 | oracle du critique (hors dépôt) | sept fixtures retrouvées par l'oracle indépendant |
| défaut de condensation de la tête | reçu d'auditeur (fixture, scikit-learn, oracle `Fraction`) | fixture rejouée sur la tête de afb081774 ; portée mesurée sur 128 configurations |
| hiérarchies `core` et `cover` du binaire | porte `mhgp10_tower_oracle` (C ∩ X), `mhgp10_points_cover` (α seulement) | différentiel contre l'oracle indépendant : 300 nuages de 6 à 11 sites (génériques et dégénérés), K = 2, 3, 4, suites des partitions à toutes les coupes : `core` 900 cas, 0 écart ; `cover` sans égalité 786 cas, 0 écart ; 114 cas à égalités exactes, dont 40 où le départage du produit diffère d'un départage lexicographique |

## 7. Constats numérotés

Gravité : **bloquant** (rend faux ou invalide un résultat ou un contrat), **majeur** (à traiter dans la conception
v11), **mineur**, **info**.

### L03_MATH_POINTS-01 — Sur les démos Zoltan désignées, la tour aux ordres du contrat ne réussit pas là où HDBSCAN échoue (bloquant pour le contrat 3 lu sur ces démos à K = 5 et 10)

- **Fait.** Les sept objets suivis où la hiérarchie de HDBSCAN échoue (01 B ; 02 A et B ; 03 A ; 04 A, B, C) restent
  ≤ 1/2 à K = 5 et à K = 10 pour `core`, `cover` **et pour le plafond de la définition 8** : FULL_K ne contient
  aucun amas discret qui les isole, et aucune des règles exactes mesurées sur recadrages par le harnais (A5, P_2,
  majorités de bande, MMt) ne répare un échec. Aux ordres 2, 3 et 4, mesurés ici, un seul est rattrapé : 04 C (0,566
  et 0,629 au plafond ; 0,565 et 0,630 pour `cover`), et un objet est perdu aux mêmes ordres : 02 C (0,44 contre
  0,67 et 0,62 pour HDBSCAN). 01 B plafonne à 0,478. Au même ordre, de K = 2 à 5 : 2 gains, 2 pertes sur 60 couples
  ; au meilleur ordre par objet : 9 objets sur 15 contre 8.
- **Preuve.** `/workspaces/E-HGP/build/v10-lidar-demos/harnais/RAPPORT_HARNAIS.md:76` (K = 5 et 10) ; § 6.5 (ii) de
  ce rapport (K = 2 à 5, `preuves_l03_math_points/lidar/lidar_demos_K2_K5.txt`) ; contrôles à K = 5 identiques au
  harnais sur les cinq démos ; table de la batterie recalculée : sur les sept instances en échec enregistré des
  trois trames de démonstration sans sol, deux passent 1/2 à K = 5 ou 10, une seule au même ordre que l'échec.
- **Vérification.** exécuté (K = 5, cinq démos) et mesuré (K = 2 à 4, cinq démos).
- **Conséquence v11.** Aucune hiérarchie mesurée ne fournit, sur ces démos à K = 5 ou 10, un « exemple où la
  hiérarchie HGP réussit ». Le meilleur amas discret n'est pas un majorant mathématique (un bloc inclus dans un amas
  discret peut avoir un meilleur IoU : 02 A à K = 4, 0,414 contre 0,393), mais rien de mesuré ne franchit 1/2. La
  demande de l'utilisateur recopiée dans `morsehgp3D_v11/README.md` admet d'autres cas (« on peut en trouver
  d'autres ») : les exemples existent ailleurs, 27 couples (instance, K) sur 80 dans les 39 trames « échec » du
  criblage (`cover`, niveau B, trames choisies sur les échecs de HDBSCAN), et 04 C à K = 3 et 4. La v11 doit (a)
  évaluer **tous** les ordres 1 à Kmax que la tour FULL fournit déjà, (b) graver la liste des instances gagnées et
  perdues, (c) ne rien promettre sur les objets en contact (à moins de 0,1 m : 0 exacte sur 42) tant qu'une idée
  nouvelle (multi-K, autre notion de voisinage) n'est pas mesurée.

### L03_MATH_POINTS-02 — Le gain de `cover` sur HDBSCAN vient de l'entrée « bord » et du facteur α = 2, pas de la connexité exacte (majeur)

- **Fait.** Au meilleur bloc, la hiérarchie d'atteignabilité mutuelle à α = 2 et entrée bord (MR₂-bord, sans tour)
  égale `cover` : −0,004 [−0,010 ; +0,003] à K = 2, −0,004 [−0,011 ; +0,002] à K = 5, +0,007 [−0,001 ; +0,017] à K =
  10 (32 scènes de 2 000 points) ; −0,011 [−0,025 ; +0,005] à K = 5 sur 16 scènes de 8 000 points. L'entrée bord
  seule explique +0,047 des +0,067 de `cover` sur HDBSCAN à K = 5, et +0,079 des +0,099 à K = 10. Sur la démo 04,
  MR₂-bord rattrape aussi le vélo C (0,568 à K = 3 et 0,619 à K = 4, contre 0,565 et 0,630 pour `cover`) ; HDBSCAN y
  vaut 0,348 et 0,341, MR₁-bord 0,361 et 0,357, MR₂-cœur 0,437 et 0,396.
- **Preuve.** § 6.5 (i) ; `preuves_l03_math_points/mrbord/niveau_b_resume.txt`, `mrbord/lidar_mr_04.txt` ; contrôles
  : K = 1 identique pour les six hiérarchies ; MR₁-cœur redonne les valeurs publiées de Zoltan (démo 03 à K = 5 :
  0,4444 / 1 / 0,9804 ; démo 04 à K = 3 et 4 : valeurs de la courbe publiée). Antécédent dans le dépôt :
  `receipts/test_cover_C_20260929/README.md:66` (famille « objet », graines scellées : aucune différence à K = 2, 3,
  5, 8 ; +0,010 à K = 10) et `audits/audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md:89` à 91.
- **Vérification.** mesuré (niveau B, sans tête) ; lu (lot C, coupes plates à même tête).
- **Conséquence v11.** Ne jamais écrire « la tour bat HDBSCAN » sans le témoin MR-bord à côté : la batterie du 1er
  octobre ne le portait pas. Ce qui est établi : la sémantique des amas discrets (entrée par couverture) fait mieux
  que la condition de cœur de HDBSCAN standard. Ce qui ne l'est pas : un apport statistique de la multicouverture
  exacte à K fixé. L'atout propre de la tour (tous les ordres, verticales, exactitude, facettes pour Zoltan) reste à
  exploiter et à mesurer.

### L03_MATH_POINTS-03 — La tête publiée ne fait pas la condensation de HDBSCAN quand un cluster ne tient que par des points à entrée différée (majeur)

- **Fait.** `condense` fait sortir chaque point attaché à son propre niveau d'entrée et ne teste `min_cluster_size`
  qu'aux fusions géométriques. Sur la fixture A (trois points admis à β = 1, 4, 9), B (deux points à 16), fusion à
  25, mcs = 2, z = 1 : stabilité de A = 1,2333 (37/30) au lieu de 11/15 ; avec racine sélectionnable la tête rend A
  et B, scikit-learn rend un seul cluster. **Racine exclue**, en ajoutant C (deux points à 100) et une racine
  globale à 1600 : la tête rend A \| B \| C, scikit-learn 1.9.1 rend R \| C sur la même ultramétrique (la tête
  compte A + B = 4/3 > 7/8 = R, la valeur juste est 5/6 < 7/8 ; rejoué ici ; à z = 2 la sélection coïncide, la
  stabilité de A reste fausse). Le défaut se déclenche quand un nœud est grand par ses points attachés à dates
  différées, sans enfant grand. **Portée mesurée ici** (8 scènes de 2 000 points, K = 2 et 5, mcs 10 et 45, z = 1 et
  3, soit 128 configurations ; le port Python de la sémantique publiée redonne les étiquettes du binaire dans les
  128) : avec l'entrée `core`, une cohorte de départs fait passer un cluster sous mcs dans 60 configurations sur 64
  et les stabilités changent dans 45, mais **aucune étiquette EOM ne change** ; avec `cover`, le défaut ne se
  déclenche dans aucune des 64 configurations. Raison, prouvée : en position générale la première boule couvrante de
  x contient exactement K sites, c'est une naissance d'ordre K, et x est attaché à cette feuille à sa date de
  naissance. Hors position générale `cover` a des **entrées internes** : sur le témoin de l'auditeur continu
  (`internal_k3`, six sites dont quatre cocycliques, K = 3), rejoué ici, le site (15,4,0) entre à β = 25 dans la
  racine née à 169/9, et à mcs = 6 la tête publiée rend une stabilité de 88/65 au lieu de 6/5 (z = 1) et de
  1294/4225 au lieu de 6/25 (z = 2), sans changement d'étiquette.
- **Preuve.** `morsehgp3D_v10/src/head/head.cpp:73` à 80 et 81 à 103 ; `preuves_l03_math_points/driver_condense.cpp`
  et `driver_condense_racine_exclue.cpp` liés à la bibliothèque de afb081774 (sorties `sorties/driver_condense.txt`
  et `sorties/driver_condense_racine_exclue.txt`) ; scikit-learn 1.9.1 sur la même ultramétrique ; fixtures de
  l'auditeur continu (`receipts/audit_continu_20260929/point_condensation_20260930/README.md`) ;
  `impact_condensation.py` et `sorties/impact_condensation.txt` ; `fix_cover_interne.py` et
  `sorties/fix_cover_interne.txt` (oracle de cet audit, binaire de afb081774 et les deux condensations ; le reçu de
  l'auditeur `receipts/audit_continu_20260929/point_condensation_cover_r2_20260930` passe aussi son propre
  `verify.py`, rejoué sur une copie). Défaut déclaré ouvert : `morsehgp3D_v10/PASSATION.md:13` à 22 et 91 ; non
  corrigé dans le raccord privé R2. `docs/SPEC_V10.md:70` (« la tête condense exactement comme HDBSCAN ») et le
  tableau d'état du README (« condensation exacte ») disent encore le contraire pour la hiérarchie C ∩ X qu'ils
  décrivent.
- **Vérification.** exécuté (fixture) ; mesuré (portée).
- **Conséquence v11.** Le défaut est réel, il renverse l'EOM sur des hiérarchies construites à la main, et se
  corrige par une condensation par cohortes de rang exact (plateaux contractés, égalité à mcs conservée), fixtures
  gravées ; l'auditeur indépendant a déposé le 2 octobre une proposition C++ isolée, à limites déclarées
  (`build/v10-audit-full-hierarchy-20261002/…/cohort_repair/`, lue, non rejouée ici). Sur les hiérarchies produites
  par le moteur, son effet sur les étiquettes est nul dans l'échantillon mesuré : il ne suffit pas à invalider les
  lots préenregistrés A (`core`) et C (`cover`), qui restent à rejouer avec la tête corrigée avant d'être cités. La
  porte `mhgp10_head_condensation_vs_sklearn` ne teste qu'une hiérarchie sans entrée différée (atteignabilité
  mutuelle à K = 1 et 2) : une porte verte ne disait rien de ce cas.

### L03_MATH_POINTS-04 — `cover` du produit : départage non canonique des égalités, discontinuité, 51 jugements ancrés perdus (majeur)

- **Fait.** (a) Quand plusieurs composantes couvrent x au même niveau α, le produit prend la boule de plus petit
  indice du catalogue : {0, 2, 4} rend {0, 2} et jamais {2, 4} ; le carré rend {P, Q} ; la fixture Q4, symétrique
  par rapport à m, rend {P, Q} \| {P2, Q2, R2} et son image par cette symétrie rend {P, Q, R} \| {P2, Q2}. (b)
  Déplacer un point de 2 unités déplace une hauteur de 500,5 (F2). (c) `cover` et sa variante canonique A5 passent
  74 jugements ancrés sur 125 : elles rendent AB \| CD \| EF dès que le pont est plus court (P2, T1_1700) et la
  chaîne dès sa naissance (Q4). (d) `cover` n'est ni verticale entre ordres ni respectueuse du cœur.
- **Preuve.** `morsehgp3D_v10/src/tower/tower.cpp:1561` à 1572 ;
  `preuves_l03_math_points/sorties/fix_egalites_cover.txt`, `fix_discontinuite.txt`, `fix_natif_triangles.txt`,
  `fix_natif_questions.txt`, `fix_cluster_v2.txt` ; juge natif rejoué (`juge/cel_natif_temoins.json`).
- **Vérification.** exécuté (oracle indépendant et binaire). Recoupements : dans le différentiel de cet audit, 40
  des 114 cas à égalités exactes diffèrent d'un départage lexicographique (constat 14) ; l'auditeur indépendant
  rapporte le même jour un juge historique qui rejetait à tort le binaire sur quatre sites à K = 3 pour ce motif
  (lu).
- **Conséquence v11.** Une règle du produit ne départage jamais par indice : aux égalités exactes entre composantes
  distinctes, le point est attaché à leur ancêtre commun (règle A5). La discontinuité et l'échec des cibles sont des
  propriétés de la première couverture, à déclarer et à graver, pas à corriger en silence.

### L03_MATH_POINTS-05 — ER0h(1, 12) : seule règle à 125 sur 125, mais ni prouvée continue, ni lipschitzienne, ni hors échantillon, ni portée (majeur)

- **Fait.** Les 125 jugements sont reproduits (trois chemins). Mais : (a) la continuité est une conjecture
  (`VERDICT_FINAL.md:669`) ; (b) la règle n'est pas lipschitzienne : un pas de grille déplace une réunion de 36,882
  ; 120,753 ; 385,297 unités aux échelles 10³, 10⁴, 10⁵ (loi en racine) ; (c) η = 1 et κ = 12 sont pris dans la
  fenêtre trouvée sur ces mêmes 125 jugements (κ de 5,16 à 28,26, η de 0,79 à 1,13) ; (d) au meilleur bloc elle perd
  contre `cover` : −0,016 à K = 5, −0,030 à K = 10, −0,029 et −0,058 sur les trames, et 0,24 contre 0,40 sur les
  groupes de moins de 2 K points ; (e) le noyau flottant rend un propriétaire faux à une marge de 2,0 · 10⁻¹⁰
  (témoin à cinq points, rejoué), le chemin certifié est en Python, les dénominateurs atteignent 4 790 bits, et
  aucun coût natif n'existe aux tailles d'intérêt (Python : 49 à 66 s à n = 8 000 et K = 10 ; 288 s et 5,8 Go à n =
  32 000) ; (f) sa vérification adverse est restée à l'état intermédiaire. L'auditeur indépendant a établi le 2
  octobre un lemme **conditionnel** (lu, non rejoué) : à arbre daté, activations et α fixés, l'écart de réunion de
  deux attaches est au plus 2 κ α fois la variation totale des crédits normalisés ; l'héritage des crédits peut
  amplifier une petite variation des poids propres, et la stabilité sous déplacement des coordonnées n'en découle
  pas.
- **Preuve.** `preuves_l03_math_points/juge/` (reçu `cel_ER0h_1_12.json` de sha256 égal à celui du juge ;
  `cellules_tiers_1_12.json`, empreinte `bd7a1cbf0083cc95` ; `er0h_famille.json`) ;
  `/workspaces/E-HGP/build/v10-tour-vers-points/regles2/er0h/RAPPORT.md:41` et § 7.2 ; `VERIFICATION.md:7`, 44, 49 ;
  `/workspaces/E-HGP/build/v10-verrou-points/juge_final/VERDICT_FINAL.md:126` à 170 et § 3.7 ;
  `juge/er0h_t9_temoin_contact.json` ;
  `build/v10-audit-full-hierarchy-20261002/…/projection/AUDIT_PROJECTION_MATURITE_20261002.md` § 5.
- **Vérification.** exécuté (jugements, sauts, 28 tests du tiers) ; lu (mesures de niveau B, coût).
- **Conséquence v11.** ER0h est la règle cible des cellules de l'utilisateur, pas une règle prête pour le moteur.
  Avant tout port : décision écrite sur la classe de stabilité acceptée, preuve ou contre-exemple de continuité,
  fixtures hors échantillon pour (η, κ), noyau certifié natif avec compteurs à 30 000 et 60 000 points, et mesure de
  non-infériorité contre `cover` aux tailles d'intérêt.

### L03_MATH_POINTS-06 — « Un point de bord ne fait pas exister un cluster » : principe non tenu ; l'existence des clusters est le verrou ouvert (majeur)

- **Fait.** Sur l'amas de 8 points avec x à 900 et sans filament (K = 2, mcs = 9), `cover`, `cover1` et ER0h(1, 12)
  suivie des cohortes font naître le cluster {amas, x} à 453,471, dès que l'amas couvre x ; `core` à 900. La cellule
  gravée Q-Π2 passe seulement parce que x y vote pour son filament. Les trois notions de taille étudiées s'excluent
  : taille de cœur (20 jugements sur 125 : les triangles n'existent jamais), sites couverts (les triangles existent,
  le point de bord complète un amas, et toute naissance de K points est une feuille admise à mcs ≤ K : 5 732
  clusters par trame contre 867 pour HDBSCAN), maturité intermédiaire (conception du 2 octobre, non mesurée ; le
  principe y reste réfuté pour θ < 1). Deux limites de la maturité, établies le même jour par l'auditeur indépendant
  : (i) à projection fixée, elle ne fait que remplacer des blocs par leurs singletons, donc elle ne peut améliorer
  que la sélection, jamais le meilleur bloc (argument refait ici à partir de la forme close du dossier) ; (ii) une
  taille géométrique n'est pas exclusive : sur {0, 2, 4} à K = 2, τ = 1/2, deux composantes ont chacune deux sites
  mûrs à r = 4/3 pour trois sites, donc une admission par taille non exclusive ne garantit pas mcs membres après
  projection (refait à la main).
- **Preuve.** `preuves_l03_math_points/sorties/fix_point_de_bord.txt` ;
  `/workspaces/E-HGP/build/v10-tour-vers-points/regles2/vote_condense/VERIFICATION.md:60` (m9) et 46 (B2) ;
  `regles2/existence_mure/CONCEPTION.md` § 1 ; `mesure_vc/VERIFICATION.md` ;
  `morsehgp3D_v10/receipts/audit_independant_20261002/maturity_review/README.md` (commit `aa8f0807a`) ;
  `build/v10-audit-full-hierarchy-20261002/…/projection/AUDIT_PROJECTION_MATURITE_20261002.md` § 1 et 2.
- **Vérification.** exécuté (figure sans filament) ; lu (mesures, limites de la maturité).
- **Conséquence v11.** Graver la figure sans filament comme fixture **ouverte** et la soumettre à l'utilisateur avec
  des mesures LiDAR (il a demandé de ne plus trancher par questions synthétiques). Dans le produit : cohortes
  exactes, et avertissement documenté qu'avec une entrée par couverture mcs ≤ K sur-segmente par construction.

### L03_MATH_POINTS-07 — Aucune règle ne réunit toutes les propriétés ; les réponses de l'utilisateur fixent les sacrifices (majeur)

- **Fait.** Registre d'impossibilités à fixtures exactes (I1 à I24) : laminarité + toutes les couvertures +
  affectation immédiate (F1) ; fidélité non ambiguë + laminarité ⇒ discontinuité ; cible des triangles sur P2 ⇒
  exclusion de toute lignée de première couverture ; majorité + respect du cœur incompatibles dès K = 2 ; cible
  étendue + cœur incompatibles pour un pont plus court que 1868,30 ; majorités non verticales ; réunion libre des
  ordres non laminaire. Les réponses (b, a, a, a) sacrifient le cœur, l'entrée immédiate, les petits groupes de
  taille proche de K et la verticalité.
- **Preuve.** `/workspaces/E-HGP/build/v10-verrou-points/revision_cible/NOTE_REVISION_CIBLE.md` § 2.4 ;
  `juge_final/VERDICT_FINAL.md` § 5 ; rejeux de cet audit pour F1, F2, F3, (0,1,2,6,9), `cx_firstcov_k3_n6`, P2,
  T1_1700.
- **Vérification.** exécuté pour les fixtures citées ; lu pour le reste du registre (preuves non relues une à une).
- **Conséquence v11.** La spécification v11 nomme la règle **et** ses sacrifices, fixture à l'appui. Un seul ordre K
  à la fois ; le multi-K reste une recherche.

### L03_MATH_POINTS-08 — Une part de la vérité n'est pas dans la tour à K ≤ 10 ; aucune projection mesurée ne la rend (majeur)

- **Fait.** À K = 5, 24 % des groupes synthétiques n'ont aucun amas discret au-dessus de 4/5 (IoU moyen 0,839) ; aux
  quatre ordres mesurés, 1 026 groupes sur 12 800 (8,0 %) ont une classe MAP au-dessus de 4/5 et aucun amas
  au-dessus de 4/5 ; la tour baisse de 0,040 quand n passe de 500 à 32 000 à K fixe ; mélanges iid à 3 écarts-types
  : aucune classe au-dessus de 4/5 ; LiDAR en contact : 0 exacte sur 42.
- **Preuve.** `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` § 1 ; agrégats recalculés
  (`sorties/recalcul_batterie.txt`, `recalcul_lidar.txt`).
- **Vérification.** exécuté (agrégats), lu (décompositions).
- **Conséquence v11.** Publier le niveau A à côté de toute hiérarchie ; lier K à n (mesure à K = 20 et 40 bloquée
  par la borne du moteur) ; ne pas attendre de la projection ce que la tour n'a pas.

### L03_MATH_POINTS-09 — Rien n'est gravé : ni fixtures, ni registre des preuves, ni la construction conçue (majeur)

- **Fait.** Aucune des fixtures de `CLUSTER_v2.md` § 12.2, aucune cible de l'utilisateur, aucun contre-exemple des
  auditeurs n'est une porte CTest (treize portes, aucune sur une fixture de projection nommée ;
  `tests/points/test_cover_band_native.py`, qui porte les deux fixtures d'entrée interne K = 3 et K = 5, n'est pas
  câblé dans `CMakeLists.txt`). Le registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` ne contient aucune ligne
  v10 alors que `CLUSTER_v2.md` § 14 listait vingt obligations (OP1 à OP15 et leurs variantes) « à inscrire avant le
  code ». La construction par graphe-chemin (théorème PG), les clés unifiées exactes, la condensation N-aire à
  plateaux sûrs et les têtes T2 et T3 de cette conception ne sont pas dans le code : la hiérarchie exportée est la
  forêt entière, niveaux en `double`, avec un raccourci à 10⁻⁹.
- **Preuve.** `grep` sans résultat de `line5`, `firstcov`, `5732`, `jrank` sous `morsehgp3D_v10/tests`, `reference`,
  `src`, `cli` ; `grep -c 'v10\|mhgp10' docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` = 0 ;
  `morsehgp3D_v10/CMakeLists.txt:68` à 132 ; `docs/conception/CLUSTER_v2.md:247`, 843, 1037 ;
  `src/points/dendrogram.hpp:18` ; `src/tower/tower.cpp:1774`.
- **Vérification.** lu, exécuté (`grep`).
- **Conséquence v11.** Toute fixture citée dans un document de la v11 existe d'abord comme porte à code exact. Les
  sept fixtures de `CLUSTER_v2.md` sont justes (retrouvées par l'oracle indépendant) : elles se portent telles
  quelles.

### L03_MATH_POINTS-10 — Durabilité inégale des vérifications ; un contrôle annoncé n'avait jamais été enregistré (mineur)

- **Fait.** Les sept vérifications adverses des 1er et 2 octobre ont travaillé sous `/tmp`, vidé au redémarrage du 2
  octobre vers 05 h 04 UTC. Quatre ont embarqué leurs scripts dans leur rapport, en archive encodée, extraites ici :
  ER0h (90 fichiers, scripts, reçus et journaux ; sha256 `1de5a172…` conforme ; le témoin de contact en est rejoué),
  tête de la thèse (74 fichiers), correctif du vote (34 scripts, sha256 `d528a19a…` conforme, reçus non archivés),
  mesures de la batterie (28 scripts, empreinte `b489f4c9…` conforme). Trois n'ont laissé que des empreintes : vote
  sur tour condensée (celle qui porte les constats B1 et B2), mesure du vote, références de la batterie. La
  vérification d'ER0h porte « état intermédiaire » et n'a pas été close. `VERDICT_FINAL.md:302` et
  `PORTES_ET_REGISTRE.md:162` gardent des marqueurs non remplis (`@@EQUIV@@`, `@@NEQ@@`, `@@EQUIVP@@`) à la place du
  contrôle exact de la proposition E ; aucun reçu `equiv*` n'existe dans `juge_final/verdict/recus/`. Les réponses
  de l'utilisateur du 30 septembre, qui fondent 110 des 125 jugements, n'ont pas de fichier daté.
- **Preuve.** `ls /tmp/mhgp10-batterie` : absent ; archives extraites des `VERIFICATION.md` de `regles2/er0h`,
  `regles2/these91`, `regles2/vote_condense_v2` et de `batterie_ab/VERIFICATION_mesures.md` ; aucun bloc encodé dans
  les trois autres ; `grep -n '@@'` ; contrôle exécuté ici : `juge/equivalence_condense_1_12_n9.json` (71 scènes,
  408 couples scène × mcs, 2 956 points, 2 186 rayons, 0 violation de E1 à E4) ; `juge/er0h_t9_temoin_contact.json`.
- **Vérification.** exécuté.
- **Conséquence v11.** Garder la pratique de l'archive embarquée, la rendre obligatoire, et y joindre les reçus. Un
  verdict n'est cité qu'une fois clos. Les réponses de l'utilisateur sont recopiées dans un fichier daté le jour
  même.

### L03_MATH_POINTS-11 — La référence HDBSCAN demande un protocole : fixture P1 trompeuse, ordre des égalités (mineur)

- **Fait.** Sur P1 (pont de 2000, arêtes obliques de 1999,956 sur la grille), scikit-learn 1.9.1 rend ABC \| DEF à
  `min_samples` 1 ou 2 et mcs 2 ou 3 : l'échec de HDBSCAN à K = 2 ne se voit que sur P2, T1_1700 ou la figure
  idéale. Sur `line5`, le point du milieu change de cluster avec l'ordre d'entrée (trois algorithmes). D'une machine
  à l'autre les étiquettes plates changent à K ≥ 3 (tri non stable des ex æquo). Avec `allow_single_cluster=True`,
  scikit-learn rend un cluster de trois points à mcs = 5, et la tête publiée aussi (exécuté) : l'auditeur
  indépendant propose de l'interdire, ce qui s'écarte de la référence ; la convention de racine est à fixer.
- **Preuve.** `preuves_l03_math_points/sorties/sklearn_triangles.txt`, `line5_sklearn.txt`,
  `driver_racine_sous_mcs.txt` ; `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` § 2.8.
- **Vérification.** exécuté.
- **Conséquence v11.** Graver P2 et T1_1700 (pas P1) pour « HDBSCAN échoue à K = 2 » ; comparer les hiérarchies par
  leurs blocs à plateaux contractés ; pour toute coupe plate de scikit-learn, publier l'enveloppe sur les ordres
  d'égalité.

### L03_MATH_POINTS-12 — Toutes les mesures des 1er et 2 octobre sont exploratoires (majeur)

- **Fait.** Graines de développement seulement ; aucune confirmation scellée (la campagne a été arrêtée avant) ;
  niveaux `hard` et `extreme` calibrés sur l'échec de HDBSCAN ; 39 des 64 trames choisies sur ses échecs ; hors de
  ces trames l'écart LiDAR à K = 5 n'est pas séparé de zéro ; les règles candidates ne sont pas dans la batterie
  vérifiée ; l'étage sélection montre un gain en synthétique et une perte sur les cinq trames. Le décideur des tests
  préenregistrés indexe ses lignes par (unité, méthode) et ne cherche les manquants que parmi les unités présentes :
  un fichier amputé ou à doublons y passe (lu dans le code ; contre-épreuve de l'auditeur indépendant, qui a aussi
  contrôlé que le vrai reçu du lot A porte bien 960 unités × 8 méthodes ; non rejoué ici).
- **Preuve.** `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` § 4.1 et § 5.1 ; `selection/RAPPORT.md` § 1 ; note de
  mémoire `sauvegarde-coupure-20261001` ; `morsehgp3D_v10/bench/synthetic/decide.py:33` à 45 et 136 à 143.
- **Vérification.** lu.
- **Conséquence v11.** « Meilleur que HDBSCAN » reste à établir par un protocole scellé : espace de graines neuf,
  strates de trames par rôle, témoins MR-bord, règle de décision écrite avant.

### L03_MATH_POINTS-13 — `cover1` est une lecture « masse » de la couverture, jamais jugée sur les cibles (info)

- **Fait.** `--entry=cover1` rend ABC \| DEF de 1154,684 à la fusion sur P1, P2 et T1_1700 (mcs 2 et 3), mais met x
  avec la paire serrée en Q2, x dans l'amas en Q3, et rend les tétraèdres en Q4 : réponses (b, b, b, b), celles de
  la masse des sites. Elle n'apparaît dans aucun bilan des 125 jugements.
- **Preuve.** `preuves_l03_math_points/sorties/fix_natif_triangles.txt`, `fix_natif_questions.txt`.
- **Vérification.** exécuté (variantes de base seulement).
- **Conséquence v11.** Ne pas la prendre pour une solution des triangles ; la garder comme témoin de la sémantique
  de HGP-old.

### L03_MATH_POINTS-14 — Ce qui se reproduit exactement (info)

- **Fait.** Rejoués à l'identique : reçu des cellules d'ER0h (sha256 égal), réécriture tierce (même empreinte, cinq
  mutants tués), 28 tests du tiers, bilans natifs des témoins ; agrégats de la batterie (synthétique et LiDAR)
  recalculés sans écart, 20 520 lignes en double sans conflit ; valeurs LiDAR du harnais à K = 5 ; valeurs publiées
  de HDBSCAN par le témoin MR₁-cœur ; sept fixtures de `CLUSTER_v2.md` ; FULL_2 des deux triangles et des quatre
  questions. Différentiel du binaire contre l'oracle indépendant (300 nuages de 6 à 11 sites, génériques et
  dégénérés, K = 2, 3, 4, suites de partitions à toutes les coupes) : `core` 900 cas sans écart, `cover` sans
  égalité 786 cas sans écart ; les 114 cas à égalités exactes relèvent du constat 04 (40 diffèrent d'un départage
  lexicographique).
- **Preuve.** `preuves_l03_math_points/` (liste au § 13).
- **Vérification.** exécuté.
- **Conséquence v11.** Les fondations exactes (oracles Γ_K, juge condensé, bibliothèque de fixtures, harnais LiDAR,
  tables de la batterie) sont fiables et se portent.

## 8. Ce qui est tranché, conjecturé, ouvert

### 8.1 Tranché (preuve, ou mesure recoupée par au moins deux chemins)

1. La tour d'ordre K est l'arbre exact des composantes de $L_K(r)$ ; ses amas discrets forment un recouvrement, pas
   une partition (thèse, définition 8, remarque 3). Une hiérarchie de points est donc un **choix** ; la thèse n'en
   fournit pas : son § 9.1 donne une partition par vote pour une sélection fixée (proposition 7), pas des
   partitions emboîtées.
2. Toute règle ancrée (une date, un propriétaire, puis les ancêtres) est laminaire. Un argmax réévalué à chaque
   coupe ne l'est pas.
3. `core` : restriction exacte, canonique, verticale, stable à 2 ε ; tardive ; au meilleur bloc, sous la
   hiérarchie de HDBSCAN dès K = 2 ; 45 jugements ancrés sur 125.
4. `cover` : meilleure fidélité native à la tour ; discontinue ; non verticale ; viole le cœur ; 74 sur 125 ;
   sur-segmente à mcs ≤ K ; prend des points de bruit dans ses blocs.
5. La projection ne doit pas lire mcs (réponses Q1 et Q1bis ; échec des règles à admission stricte) ; mcs agit dans
   la condensation par cohortes. Pour les votes de temps de couverture avec absorption et taille « sites
   couverts », « propriétaire sur la tour condensée » et « projection puis cohortes » rendent les mêmes clusters
   (proposition E, contrôle exécuté ici).
6. L'existence par taille de cœur (analogue littéral de HDBSCAN) échoue les deux triangles par construction.
7. ER0 est discontinue ; la majorité immédiate aussi ; la majorité uniforme perd deux paires bien séparées ; la
   majorité 1/β saute au contact coquille/intérieur ; un dénominateur recalculé sur les seuls témoins actifs casse
   la laminarité.
8. Au meilleur bloc, aucune règle ne dépasse `cover` de plus de 0,005 ; la perte entre la tour et la coupe plate
   est dans la sélection, pas dans la projection ni dans la condensation.
9. Le gain de `cover` sur la hiérarchie de HDBSCAN standard est un effet d'entrée (points-bord) et de facteur 2 sur
   les distances de paire ; à entrée et tête égales, la tour et MR₂-bord sont à parité (lot C scellé ; niveau B
   mesuré ici).
10. Sur les sept objets des démos où HDBSCAN échoue, la tour à K = 5 et 10 échoue aussi ; aux ordres 3 et 4 elle
    en rattrape un et en perd un autre.

### 8.2 Conjecturé

| Énoncé | État | Ce qui le fermerait |
| --- | --- | --- |
| ER0h est continue | aucun saut sur 29 750 + 8 634 déplacements adverses ; loi en racine sur une famille | preuve du transport entre strates, ou contre-exemple exact |
| le saut d'ER0h est borné par $\sqrt{\kappa a\delta/8}$ | argument, pas preuve générale | preuve |
| la région (η, κ) des 125 jugements est connexe | grille et dichotomie | formes closes sur T1_1700 et T6 |
| lemme L0 du vote condensé, points 2 et 3 (amas discret constant sur la vie d'un nœud, admission à la naissance) | argument ; contrôle sur deux scènes ; le point 1 (un témoin fort est une naissance, en position générale) est prouvé | preuve relue ou contre-exemple |
| l'étage d'existence mûre n'ajoute aucune discontinuité (théorème C) | découle de la forme close F du dossier (le maximum et la statistique d'ordre sont 1-lipschitziens : argument refait ici) ; F relue par l'auditeur indépendant, non revérifiée ici | preuve de F inscrite au registre, fixture ; la variante rétroactive R est discontinue (saut de 345 pour 1 mm, lu) |
| monter K avec n rattrape la baisse de la tour | observé : la part des groupes dont le meilleur ordre est K = 10 monte de 0,228 à 0,532 | mesure à K = 20 et 40 (borne du moteur à lever) |

### 8.3 Ouvert

1. **L'existence des clusters.** Quelle taille fait exister un cluster : les deux exigences de l'utilisateur (les
   triangles existent à mcs ≤ 3 ; un point de bord ne fait pas exister un cluster) ne sont réunies par aucune des
   trois notions étudiées (constat 06).
2. **Une règle qui passe les cibles sans sacrifier les petits groupes.** La tension est une propriété de la famille
   des majorités de bande (elle se ferme à η ≤ 1/4, hors de la région qu'impose Q1).
3. **Le point de bruit entré dans le bloc** de `cover` : aucun critère sans constante posée à la main ne sépare un
   point de bord d'un point de bruit voisin.
4. **L'axe K.** Les hiérarchies de deux ordres se croisent ; aucune tête multi-K n'a gagné 0,02 ; c'est pourtant le
   seul endroit où la tour peut différer d'un témoin d'atteignabilité mutuelle.
5. **Les objets en contact sur LiDAR** (vélos garés, piéton contre façade) : hors d'atteinte à K = 2 à 10, sauf un.
6. **Le coût natif** de toute règle à votes aux tailles du contrat (30 000 à 60 000 points, part des 100 ms).
7. **La sélection** (z, EOM, remplissage), remise à plus tard par l'utilisateur.
8. **La confirmation scellée** de toute supériorité sur HDBSCAN.
9. Mesures lancées et non rendues à la coupure du 2 octobre : découpage plat et ordres 2 et 3 sur les 64 trames,
   MAP à 16 000 et 32 000 points (`lidar64/tables/` est vide), mesure de l'existence mûre, vérification finale
   d'ER0h.

## 9. Ce qui est solide et mérite un port explicite en v11

| Acquis | Pourquoi il est solide | Source à porter |
| --- | --- | --- |
| Définitions exactes : $L_K$, FULL_K à coupes fermées et fusions N-aires, amas discrets, $\alpha_K$, $d_K$, relation de couverture | relues dans la thèse et dans le code ; rejouées par un oracle indépendant | `docs/SPEC_V10.md`, `src/tower/tower.hpp:125` à 133 |
| Calcul de `cover` : première boule du catalogue de population ≥ K contenant x, une résolution par boule | prouvé (la K-partie minimisante a une boule critique de $p+q_{\min}\leq K$) ; α contrôlé par force brute | `src/tower/tower.cpp:1531` à 1598, `tests/points/test_cover_entry.py` |
| Contrat de règle ancrée (date, propriétaire vivant et couvrant) et ses validateurs | laminarité et NP par construction | `SYNTHESE/VERROU_FULL_POINTS.md` § 1.2 ; `verif_echelle_relative/ver.py` (`Hier.valider`) |
| `core` et ses énoncés C1 à C5, borne 2 ε | preuves courtes, refaites ici | `CLUSTER_v2.md` § 3.3 ; audit continu § 2.2 |
| Condensation par cohortes, plateaux N-aires atomiques, lemme des plateaux sûrs | lemme recoupé contre l'oracle de l'auditeur (264 cas) ; fixtures A/B/R, A/B/C sous racine exclue et `internal_k3` rejouées | `juge_final/cibles/condense.py` ; `CLUSTER_v2.md` § 5.3 ; reçu `point_condensation_20260930` ; proposition C++ de l'auditeur indépendant (`cohort_repair/head_cohorts.patch`, limites déclarées, non rejouée ici) |
| Les sept fixtures C ∩ X de `CLUSTER_v2.md` § 12.2 et les cibles de l'utilisateur avec leurs variantes | valeurs retrouvées par l'oracle indépendant | `CLUSTER_v2.md:843` ; `PORTES_ET_REGISTRE.md` § 2 |
| Registre des impossibilités I1 à I24 avec fixtures | chaque ligne a une fixture exacte et un vérificateur | `revision_cible/NOTE_REVISION_CIBLE.md` § 2.4 |
| ER0h comme **référence exacte de recherche** : définition § 2.2, lemme M, lemme d'encadrement, proposition E, 28 tests, mutants | trois implémentations concordantes ; rejouée ici | `juge_final/verdict/er0h.py`, `regles2/er0h/er0h_texte.py`, `er0h_certifie.py` |
| Protocole de mesure à trois niveaux et trois lectures, règles de rédaction de l'auditeur, bootstrap apparié par scène, pondération LiDAR par rôle de trame | a résisté à deux vérifications adverses ; agrégats recalculés ici | `batterie_ab/RAPPORT_TOUR_PUIS_HIERARCHIE.md` (règles de lecture) |
| Témoin d'atteignabilité mutuelle exact, entrées cœur et bord, α = 1 et 2 | entiers exacts ; redonne les valeurs publiées de HDBSCAN | `tests/head/mreach.cpp`, `mreach.hpp` |
| Harnais LiDAR : scènes préparées, IoU au sens de la PQ, outil natif du plafond de la définition 8 | reproduit les 13 exécutions publiées des démos ; rejoué ici | `v10-lidar-demos/harnais/` |
| Référence MAP du plan étendu | redérivée par un vérificateur (8 256 000 sites, 0 désaccord) | `banc2/plan_map/` (non relu ici) |

## 10. Ce qu'il ne faut pas refaire

1. Une tête dite « condensation HDBSCAN exacte » dont la seule porte teste une hiérarchie sans entrée différée :
   le défaut des départs de points n'a été vu que par un auditeur, après deux tests préenregistrés.
2. Départager une égalité exacte par indice de catalogue ou rang de Morton.
3. Des niveaux en `double` et un raccourci à 10⁻⁹ dans l'ordre des événements de la hiérarchie de points ; exporter
   toute la forêt (un million de nœuds pour 67 000 points) au lieu des seuls événements de points.
4. Une conception de 80 Ko dont ni les fixtures, ni les portes, ni les lignes de registre ne sont gravées, et dont
   la construction centrale n'est pas implantée.
5. Des vérifications adverses dont le code et les reçus vivent sous `/tmp` sans archive (trois sur sept) ; des
   documents de verdict avec des marqueurs non remplis ; un verdict cité alors qu'il est encore « intermédiaire ».
6. Régler les paramètres d'une règle sur les fixtures qui servent ensuite à la qualifier ; écrire des cibles
   « extrapolées » après avoir vu des règles (catalogue v2 : une seule cible confirmée, statut « forcée » faux sur
   des fenêtres).
7. Comparer à HDBSCAN standard sans les témoins MR-bord ; calibrer la difficulté sur l'échec de HDBSCAN ; choisir
   des trames sur ses échecs sans publier l'écart par rôle.
8. Présenter une coupe plate réglée (z, mcs, remplissage) avant d'avoir établi les niveaux A et B ; publier un
   « meilleur z ».
9. Une projection qui lit mcs (admission stricte) ; un vote dur qui compte les faces (W, G) ; une majorité à poids
   uniformes ; un dénominateur recalculé.
10. Un chemin rapide qui décide une majorité ou une date en flottant sans borne certifiée ni repli exact.
11. Des règles Python qui ne passent pas les tailles d'intérêt (votes T à n = 8 000 et K = 10 : échec mémoire).
12. Reposer à l'utilisateur des questions synthétiques de préférence : il a demandé des mesures LiDAR.

## 11. Questions ouvertes pour la v11

1. Quelle notion de taille fait exister un cluster, pour que les triangles existent et qu'un point de bord ne
   complète pas un amas ? (figure sans filament à trancher par l'utilisateur, mesures LiDAR à l'appui)
2. La classe de stabilité exigée d'une règle du produit : continuité seule (ER0h, pente en racine), ou constante
   locale prouvée (MMt, 110 jugements sur 125, deux retards de 4,5 % et 10,8 %) ?
3. Le contrat 3 se juge-t-il sur les cinq démos (un seul objet rattrapé, à K = 3 et 4) ou sur un jeu de trames
   stratifié par rôle ? À quels ordres ?
4. « Meilleur que HDBSCAN » : contre scikit-learn standard seulement, ou aussi contre HDBSCAN muni de l'entrée bord
   et de α = 2, qui égale la tour à un ordre ?
5. La réponse (b) de Q1 vaut-elle son coût mesuré sur les petits groupes (0,24 contre 0,40) ?
6. Le produit publie-t-il une hiérarchie par ordre, ou une seule ; et quel ordre pour le LiDAR ?
7. La tête de la thèse (masses $S_\tau/T_x$ sur les facettes) est-elle requise par Zoltan indépendamment de la
   hiérarchie de points ? Si oui, c'est un second consommateur de la relation de couverture, hors moteur.
8. La doctrine F1 de la v11 (aucune décision en flottant) couvre-t-elle la sélection EOM, qui compare des sommes de
   $r^{-z}$ ? Si oui, quel domaine de z et quel repli exact ; sinon, où passe la frontière du moteur ?
9. Convention de racine : `allow_single_cluster` peut-il rendre un cluster de masse inférieure à mcs, comme
   scikit-learn, ou la règle « un cluster n'apparaît qu'à partir de mcs » s'applique-t-elle aussi à la racine ?

## 12. Recommandation pour la v11

### 12.1 Découper en cinq étages à contrats

| Étage | Entrée | Sortie | Exactitude | Dans le moteur ? |
| --- | --- | --- | --- | --- |
| 1. tour | sites | FULL_1 à FULL_Kmax, rangs exacts | exact | oui |
| 2. couverture | tour, catalogue | par ordre : entrée cœur ($d_K$, nœud) ; **toutes** les composantes de première couverture à α ; incidences des témoins forts (boule, niveau, nœud) | exact | oui (publication) |
| 3. projection | étages 1 et 2 | date et propriétaire par site, en rangs exacts ; hiérarchie réduite aux événements de points | exact relativement à la tour | oui pour `core` et `cover` canonique ; non pour le reste |
| 4. condensation | étage 3, mcs | arbre condensé par cohortes | exact | oui |
| 5. sélection | étage 4 | étiquettes | flottant déclaré | non : bibliothèque de tête, hors moteur |

L'étage 2 est la clef : toutes les règles étudiées (sauf la tête de la thèse et `cover1`) ne lisent que FULL_K et
la relation de couverture. Son volume est mesuré : 633 480 boules témoins et 3 167 616 incidences pour 67 114
sites à K = 5 (démo 01).

### 12.2 Première hiérarchie du produit : `cover` canonique, avec `core` en témoin

**Définition** (dates en niveaux, c'est-à-dire en rayons carrés). Pour un ordre K et un site x, soit $A(x)=\alpha_K(x)^{2}$ et $E(x)$ l'ensemble des nœuds de FULL_K
vivants à la coupe fermée A(x) dont la composante contient le centre d'une boule fermée de niveau A(x) contenant x
et au moins K sites. On pose $o(x)=\mathrm{lca}(E(x))$ et $t(x)=\max\left(A(x),b_{o(x)}\right)$. Avant t(x), x est
seul ; ensuite il appartient au bloc de l'ancêtre vivant de o(x). Si E(x) a un seul élément, c'est la première
couverture ; sinon x attend la fusion de ses composantes de première couverture. `core` : $t(x)=d_K(x)^{2}$, o(x)
la composante qui contient x.

**Pourquoi elle d'abord.** (1) C'est la sémantique des amas discrets de la définition 8, avec un seul choix déclaré.
(2) C'est la meilleure hiérarchie native au meilleur bloc, la seule mesurée aux tailles d'intérêt et sur 64 trames.
(3) Elle coûte au plus n résolutions par ordre. (4) Toutes les règles à votes se réduisent à elle pour un point qui
n'a qu'une entrée de bande (prouvé pour ER0h). (5) Ses défauts sont connus, exacts et gravables.

**Obligations de preuve** (à inscrire au registre avant le code) :

| Id | Énoncé | Statut visé |
| --- | --- | --- |
| P1 | la première boule du catalogue de population ≥ K contenant x a pour niveau $\alpha_K(x)^{2}$, et l'énumération des boules de ce niveau contenant x donne E(x) | prouvé ci-dessous, relativement à la complétude du catalogue servant K et au lemme de couverture |
| P2 | laminarité (règle ancrée) | prouvé |
| P3 | tout bloc est dans l'amas discret de son nœud | prouvé |
| P4 | équivariance par isométrie de la grille et par renumérotation ; aucun départage | prouvé par construction ; fixtures F1, carré, Q4 et son image |
| P5 | $\alpha_K\leq t$ ; $d_K/2\leq\alpha_K\leq d_K$ | prouvé |
| P6 | `core` : C1 (indépendance du départage), C2 et C3 (emboîtements), stabilité 2 ε | prouvé |
| P7 | condensation par cohortes = sémantique de HDBSCAN sur l'ultramétrique de points ; plateaux atomiques | prouvé (lemme de condensation) ; fixture A/B/R |
| N1 | `cover` est discontinue | `false_in_general` pour « cover est stable » ; fixture F2 |
| N2 | `cover` n'est pas verticale ; `cover` ne respecte pas le cœur | fixtures (0,1,2,6,9) et `cx_firstcov_k3_n6` |
| N3 | `cover` ne rend pas les cibles de l'utilisateur | fixtures P2, T1_1700, Q4 : 74 sur 125 |
| N4 | avec `cover`, mcs ≤ K admet toute naissance de K points | fixture à écrire ; avertissement dans l'interface |

*Preuve de P1.* Soit F une K-partie contenant x de rayon minimal ρ = $\alpha_K(x)$ et B sa boule minimale. Si un site s
strictement intérieur à B n'est pas dans F, on remplace par s un site u ≠ x de F situé sur la coquille : la nouvelle
K-partie contient x et tient dans B, donc son rayon vaut ρ par minimalité, et sa boule minimale est B par unicité.
Le centre reste donc dans l'enveloppe convexe des sites de coquille restants. On répète jusqu'à ce que tous les
sites strictement intérieurs soient dans la partie : alors $p+q_{\min}\leq K$, B est un témoin fort, présent au
catalogue. Réciproquement toute boule de population ≥ K qui contient x a un rayon ≥ ρ. Enfin, par le lemme de
couverture, toute composante qui couvre x au niveau ρ porte le centre d'un témoin fort de rayon ≤ ρ contenant x,
donc de rayon exactement ρ : l'énumération des boules de ce niveau donne E(x). ∎

**Juges.** Oracle borné Γ_K (n ≤ 12 à 14, tous les niveaux critiques, nuages génériques et dégénérés) : il établit
la vérité. À l'échelle : invariants globaux (propriétaire vivant à la date, couvrant par une boule témoin, date entre
α et $d_K$, racine unique, masses sommées) et juge d'échantillon ; jamais de vérification exhaustive.

### 12.3 Variantes à garder comme options de recherche (référence exacte, hors moteur)

| Option | Rôle | Condition d'entrée dans le moteur |
| --- | --- | --- |
| **ER0h(η, κ)** | règle cible des cellules de l'utilisateur | décision écrite sur la stabilité acceptée ; preuve ou contre-exemple de continuité ; (η, κ) validés hors échantillon ; noyau certifié natif mesuré à 30 000 et 60 000 points ; non-infériorité contre `cover` au niveau B aux tailles d'intérêt et sur trames ; mesure R9 (petits objets) |
| MMt(8, 4/5) | repli continu prouvé (110 sur 125, retards seulement) | mêmes mesures ; décision de l'utilisateur sur les deux retards |
| existence mûre M[θ] et R[θ] ; `VC[coeur,W1]` | étage d'existence | question 1 du § 11 tranchée ; mesure aux tailles d'intérêt |
| `cover1` | témoin de la sémantique de HGP-old | aucune : témoin |
| bande-LCA (η > 0), P_κ | témoins stables | aucune : témoins |
| tête de la thèse § 9.1 (`engage`, `compte`) | masses fractionnaires ; besoin éventuel de Zoltan | consommateur séparé de l'étage 2 |
| têtes multi-K | seul lieu d'un avantage propre de la tour | une hypothèse nouvelle et une mesure contre MR₂-bord |

### 12.4 Fixtures à graver dès l'ouverture (portes à code exact, coordonnées du § 4.2)

1. **Tour** : P1, P2, T1_1700 (niveaux de FULL_2 : 999,978 ; 1000 ; 1154,684 ; 1931,827 ou 1930,861 ou 1787,360),
   Q2, Q3, Q4.
2. **`core`** : les sept fixtures de `CLUSTER_v2.md` § 12.2 ; `core` sur les six fixtures de l'utilisateur.
3. **`cover` canonique** : F1 (le site 2 attend r = 2), carré, Q4 et son image par symétrie centrale (sorties
   images l'une de l'autre) ; F2 (saut déclaré) ; (0,1,2,6,9) ; `cx_firstcov_k3_n6` ; sorties sur P1, P2, T1_1700,
   Q2, Q3, Q4 (dont les échecs, gravés comme tels).
4. **Condensation** : A/B/R (stabilité 11/15, sélection de scikit-learn) ; A/B/C sous racine exclue (R \| C, pas
   A \| B \| C) ; `internal_k3` à mcs = 6 (6/5, pas 88/65) ; plateau contracté ; égalité à mcs ; racine de masse
   inférieure à mcs (convention à fixer d'abord) ; deux triangles à mcs 2, 3, 4.
5. **Référence HDBSCAN** : P2 et T1_1700 (tout en bruit), `line5` (non-équivariance déclarée), K = 1 : identité de
   la tour, de MR₁ et de MR₂.
6. **Recherche (hors moteur)** : les 125 jugements ; `bascule_repli_thales` ; famille à cinq points d'ER0h ; fixture
   Q8 ; témoin de contact à marge 2,0 · 10⁻¹⁰ ; trois contre-fixtures du vote à marge ; figure du point de bord
   sans filament, statut **ouvert**.
7. **LiDAR** (comptes et IoU seulement, aucune coordonnée) : les sept objets des démos avec leur meilleur IoU par
   ordre 1 à Kmax ; la liste des 80 couples (instance, K) en échec enregistré avec gagné ou perdu par source.

### 12.5 Ce qui reste hors du moteur

La sélection (EOM, feuilles, exposant z, epsilon), le remplissage et la complétion des queues ; les règles à votes
tant que leurs obligations ne sont pas levées ; la tête de la thèse ; les références MAP et modales ; HDBSCAN, qui
reste scikit-learn appelé tel quel, avec le témoin d'atteignabilité mutuelle du banc pour l'attribution.

Raccord avec l'architecture ouverte le 2 octobre (`morsehgp3D_v11/docs/ARCHITECTURE.md`, modules `points` puis
`head`, règle 9 et doctrine F1 : aucune décision en flottant). La projection et la condensation par cohortes ne
comparent que des rangs exacts : elles tiennent F1 sans effort. La sélection EOM, elle, compare des sommes de
$\lambda=r^{-z}$ : dans la v10 ce sont des `double` (`src/head/head.cpp:11` à 13 et 117 à 119), donc une décision
flottante. Pour z pair les stabilités sont des rationnels exacts ; pour z impair ce sont des sommes de racines
carrées. La v11 doit dire laquelle des trois voies elle prend : z entier pair seulement dans le moteur ; filtre
certifié avec repli exact ; ou sélection déclarée hors du périmètre F1, dans une couche nommée. L'auditeur
indépendant signale quatre quasi-égalités EOM laissées hors qualification par sa proposition de réparation (lu).

### 12.6 Protocole de comparaison à HDBSCAN (contrat 3)

1. Trois niveaux séparés (A, B, C) et trois lectures ; z et la sélection en dernier.
2. À chaque comparaison, cinq sources sur les mêmes sites : tour (A), `cover`, `core`, HDBSCAN (scikit-learn, blocs à
   plateaux contractés), **MR₁-bord et MR₂-bord**.
3. Tous les ordres 1 à Kmax, et le meilleur ordre par cible, publiés.
4. LiDAR par rôle de trame, pondéré par le plan de sondage ; les démos à part.
5. Une confirmation scellée (graines neuves, règle de décision écrite avant) avant toute phrase de supériorité.

## 13. Preuves et reproduction

Dossier durable : `/workspaces/E-HGP/build/v11-persist/audit_v10/preuves_l03_math_points/` (moins de 1 Mo ; aucune
coordonnée dérivée de KITTI : comptes, fractions d'IoU et empreintes seulement). Dossier de calcul, non durable :
`/tmp/v11-audit/l03_math_points/`.

| Fichier | Contenu | Commande |
| --- | --- | --- |
| `oracle_l03.py` | oracle Γ_K exact indépendant (boules minimales, composantes, amas discrets, entrées `core` et `cover`, hiérarchie d'atteignabilité mutuelle en convention de la thèse) | importé par les scripts ci-dessous |
| `natif.py` | lance `mhgp10_cluster --tree` (binaire reconstruit) et lit la hiérarchie exportée | idem |
| `fix_triangles.py`, `sorties/fix_triangles.txt` | P1, P2, T1_1700 : FULL_2, amas discrets, `cover` (deux départages), `core`, HDBSCAN en convention de la thèse | `python3 -B fix_triangles.py` |
| `fix_natif_triangles.py`, `fix_natif_questions.py` et leurs sorties | les mêmes fixtures et Q2, Q3, Q4 par le binaire, entrées `core`, `cover`, `cover1` | `python3 -B fix_natif_triangles.py` |
| `fix_egalites_cover.py`, `sorties/fix_egalites_cover.txt` | départage du produit : {0,2,4}, carré, Q4 et son image | `python3 -B fix_egalites_cover.py` |
| `fix_discontinuite.py` | F2 par l'oracle et par le binaire | `python3 -B fix_discontinuite.py` |
| `fix_cluster_v2.py` | les sept fixtures C ∩ X de `CLUSTER_v2.md` par l'oracle ; croisement (0,1,2,6,9) dans `sorties/` | `python3 -B fix_cluster_v2.py` |
| `fix_point_de_bord.py` | amas + x sans filament, mcs = 9 : binaire et ER0h du juge | `python3 -B fix_point_de_bord.py` |
| `driver_condense.cpp`, `sorties/driver_condense.txt` | défaut de condensation sur la tête publiée | `g++ -std=c++20 -O2 -I<v10>/src driver_condense.cpp <build>/libmhgp10_core.a -lpthread` |
| `driver_condense_racine_exclue.cpp`, `driver_racine_sous_mcs.cpp` et leurs sorties | renversement EOM avec racine exclue (tête A \| B \| C, scikit-learn R \| C) ; cluster de trois points à mcs = 5 avec racine autorisée (tête et scikit-learn) | même commande |
| `impact_condensation.py`, `sorties/impact_condensation.txt` | portée du défaut : port Python de la sémantique publiée (égal au binaire) contre cohortes, 128 configurations | `python3 -B impact_condensation.py` |
| `fix_cover_interne.py`, `sorties/fix_cover_interne.txt` | entrée interne de `cover` hors position générale (`internal_k3`) : oracle, binaire, deux condensations | `python3 -B fix_cover_interne.py` |
| `sorties/sklearn_triangles.txt`, `sorties/line5_sklearn.txt` | scikit-learn 1.9.1 sur P1, P2, T1_1700 et `line5` | scripts en ligne, reproduits dans les sorties |
| `differentiel_natif_oracle.py`, `sorties/differentiel_natif_oracle.txt`, `sorties/differentiel_natif_oracle_300.json` | binaire contre oracle sur 300 petits nuages, K = 2, 3, 4 : suites de partitions `core` et `cover` | `python3 -B differentiel_natif_oracle.py 2026100201 300 R.json` |
| `recalcul_batterie.py`, `recalcul_lidar.py` et leurs sorties | agrégats de tête de la batterie recalculés depuis les tables fusionnées | `python3 -B recalcul_batterie.py` |
| `juge/cel_ER0h_1_12.json` | rejeu de `juge_final/verdict/cellules.py` : sha256 `598b6ba0…`, égal au reçu du juge | `python3 -B cellules.py --regle ER0h --eta 1 --kappa 12 --out R.json` |
| `juge/cellules_tiers_1_12.json` | rejeu de la réécriture tierce et de ses cinq mutants | `python3 -B cellules_tiers.py --eta 1 --kappa 12 --mutants --out R.json` |
| `juge/cel_natif_temoins.json`, `juge/cel_natif_majorites.json` | bilans natifs : `cover` 74, A5 74, `core` 45, P_2 70, `maj_bande_unif[1/4]` 100, ER0h 125, ER0 125, MMt(8, 4/5) 110, MMt(4, 2/3) 100 | `python3 -B cellules_natif.py --regles '…' --out R.json` |
| `juge/equivalence_condense_1_12_n9.json` | contrôle de la proposition E jamais enregistré par le juge : 0 violation | `python3 -B equivalence_condense.py 1 12 9 9 R.json` |
| `juge/er0h_famille.json`, `er0h_thales.json`, `er0h_q8.json` | non-lipschitzianité d'ER0h, discontinuité d'ER0, fixture Q8 | `python3 -B attaque.py famille R.json` ; `attaque_complements.py thales` et `q8` |
| `juge/er0h_t9_temoin_contact.json` | témoin de contact du vérificateur d'ER0h (noyau flottant faux à une marge de 2,0 · 10⁻¹⁰, chemin certifié juste), script extrait de l'archive embarquée dans `regles2/er0h/VERIFICATION.md` | `python3 -B t9_temoin_contact.py --out R.json` |
| `mrbord/export_mreach_tree.cpp`, `niveau_b.py`, `resume_nb.py`, `niveau_b_resume.txt`, `nb_*.json` | niveau B de la tour contre MR à entrées cœur et bord (§ 6.5 i) | `python3 -B niveau_b.py R.json 2000 2,5 medium,hard 0,0.1 1` |
| `mrbord/lidar_mr.py`, `lidar_mr_04.txt` | hiérarchie MR sur les démos 03 et 04 (§ 6.5 ii) | `python3 -B lidar_mr.py 04_velos_en_rang_avec_sol 3,4 mr1_coeur,mr1_bord,mr2_bord 2` |
| `lidar/lidar_demos_K2_K5.txt`, `tab_lidar.py` | démos Zoltan, K = 2 à 5, plafond, `cover`, `core` | `lidar_iou SITES OBJ --k=K --out=R.json --threads=2` |

Variables d'environnement des rejeux du code privé : `MHGP10_FIXTURES_SCRATCH` et `MHGP10_ER0H_SCRATCH` pointés sous
`/tmp/v11-audit/l03_math_points/`, `PYTHONDONTWRITEBYTECODE=1`, `python3 -B` : aucun fichier écrit dans les dossiers
privés ni dans l'arbre de lecture.
