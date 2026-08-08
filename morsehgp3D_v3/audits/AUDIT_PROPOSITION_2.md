# Second audit actualisé et journal continu — A2pe, peeling local et oracle M1

> [!IMPORTANT]
> Base historique de cet audit : commit `8ac683ad1e167937fe7f9e964860f6be374a48d0`, 700 lignes, SHA-256 `af94886fec1d83bc671eefdd4c605ca6f4d939ff3a1ce0298e49c719aed03386`. Réaudit différentiel courant : commit `ae08c9fbfa33246187e95a87977c8c671601f040`, 738 lignes, 42 665 octets, SHA-256 `615935ad798ce5afb3eb3280a54a3bfd8306eed9d7570ff474866c7a3255d912`. Les sections historiques sont conservées comme justification des corrections; le verdict sur le texte courant est donné au §0 ci-dessous. Le suivi de l'oracle et du prototype est séparé dans [`AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md`](AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md).

> [!NOTE]
> Contexte : `phase=exploration_v3_hors_registre`, `backend=cpu_reference_oracle_under_audit`, `profile=quantized_u16_input_only`, `mode=a2pe_and_oracle_reaudit_v3`, `public_status=not_claimed`. Aucune porte de produit, aucun SLO et aucun statut exact ne sont ouverts ici.

> [!CAUTION]
> **Verdict courant : GO pour formaliser et falsifier A2pe; NO-GO pour en faire une architecture produit ou publier un statut exact.** Le texte `ae08c9f` a intégré les principales corrections mathématiques de cet audit. Les obligations décisives PEL-1 à PEL-4 restent explicitement ouvertes; aucun constructeur sparse, certificat de localité ou coût produit n'est encore établi.

## 0. Réaudit différentiel du texte `ae08c9f`

La proposition courante ferme correctement les contradictions textuelles suivantes :

- l'objet est le sous-complexe shallow stratifié, jamais l'ensemble sous-jacent d'un unique $V_K(p)$;
- une strate fournit au plus la projection de $p$ sur son enveloppe affine, suivie des tests de centrage, shell, profondeur, rang et propriétaire;
- les budgets de profondeur sont séparés pour supports deux, trois et quatre;
- une 2-face vérifie seulement $mathrm{aff}(F)=H_u$ et A2e s'exécute une fois sur le plan canonique;
- PEL-3 est déclarée fausse dès deux points et les composantes non bornées restent une obligation de terminaison;
- le coût publié est maintenant un worst-case par ancre, potentiellement quadratique sur toutes les ancres sans arrêt certifié;
- le masque carrier est fail-open et ne filtre jamais les témoins de profondeur;
- la descente réinterroge la miniboule courante;
- les runs sont triés et les niveaux rationnels égaux groupés avant réduction;
- les gates distinguent générateur géométrique, source HGP, couverture, incidences silencieuses et verticales.

Les verrous actuels ne sont donc plus des erreurs de formulation, mais les quatre obligations reconnues par le document : complétude des plans porteurs, sensibilité à la sortie avec terme d'entrée, traitement exact des strates non bornées et coût réel du prédicat 3D face à A2e. Tant qu'elles sont ouvertes, A2pe reste une hypothèse de recherche.

Le scan KaTeX du snapshot courant ne trouve ni bloc `$$` sur plusieurs lignes physiques, ni macro `\operatorname`, ni délimiteur interdit, ni commande à accolades manquantes parmi les motifs imposés par `AGENTS.md`.

## 1. Progrès réels de la nouvelle révision

La révision courante a correctement intégré l'essentiel des audits précédents :

- l'autorité reste la spécification et le registre des preuves;
- les mesures sans reçu sont explicitement diagnostiques;
- le census tangentiel n'est plus utilisé pour certifier $R$;
- $R$ est fini et distinct de la tangente non contrainte;
- carriers et témoins de profondeur sont séparés;
- la boule $\bar B(c,2r)$ est limitée au premier pas de descente;
- le tri global exact et les lots de niveaux égaux sont réintroduits;
- `exact_dyadic_input` et `quantized_u16_input` sont séparés;
- `RelevantGP` est référencé normativement;
- les largeurs u16 ne sont plus présentées comme celles du profil dyadique;
- la forêt v2 et ses anciens oracles ne sont plus qualifiés;
- l'objet HGP aval, les incidences silencieuses, la couverture et les verticales ne sont plus confondus avec le seul catalogue.

Le saut A2e reste la contribution décisive de la proposition. Pour une paire diamétrale $e=pq$, le rang support quatre devient la profondeur d'un arrangement de droites dans le plan médiateur, clippé par l'ellipse de Jung. Cela respecte l'invariant produit : aucune mosaïque de Delaunay d'ordre supérieur, aucun $\Gamma$ global persistant et aucune matrice paire--point.

Ces corrections changent le débat : il ne s'agit plus de rejeter A2e, mais de définir exactement l'objet A2pe qui doit fournir ses ancres sans reconstruire sous un autre nom une structure globale dense.

## 2. Audit de la nouvelle voie A2pe

### 2.1 P0 — l'ensemble $V_k(p)$ efface les strates dont A2pe a besoin

La proposition définit $V_k(p)$ comme l'ensemble des centres où $p$ possède au plus $k$ points strictement plus proches, puis parle de « ses 2-faces ». Pris comme simple sous-ensemble de $\mathbb{R}^3$, cet objet ne conserve pas les hyperplans internes séparant deux cellules qui sont toutes deux de profondeur au plus $k$.

Contre-exemple minimal permanent : avec $X=\left\lbrace p,u\right\rbrace$ et $k=1$, on a $V_1(p)=\mathbb{R}^3$. Sa frontière ne possède aucune 2-face. Pourtant le plan médiateur $H_u$ existe et son milieu porte la sphère critique de support $\left\lbrace p,u\right\rbrace$, de rang fermé 2. Avec davantage de points placés loin du milieu, une plaque de $H_u$ reste intérieure à $V_K(p)$ tout en portant le même événement utile.

Le bon objet n'est donc pas la frontière d'un unique $V_K(p)`. Il faut définir l'un des objets équivalents suivants :

- le **sous-complexe shallow stratifié et étiqueté** de l'arrangement des $H_u$;
- la tour des frontières $V_0(p),\ldots,V_K(p)$ en conservant toutes les incidences internes;
- une structure de faces dont chaque strate porte explicitement sa profondeur stricte et ses hyperplans porteurs.

Cette correction est une condition de complétude, pas un détail d'implémentation. La fixture à deux points doit rester dans l'oracle et échouer dès qu'une construction ne conserve que la frontière de l'union.

### 2.2 P0 — une face porte un candidat unique, pas une sphère en chacun de ses points

Sous arrangement simple, une strate de dimension $j$ possède $3-j$ hyperplans porteurs en plus de l'ancre $p$, donc un support candidat de taille $q=4-j$. Elle ne définit pas une infinité de sphères critiques. Elle définit un seul centre candidat : la projection métrique de $p$ sur l'intersection affine de ses hyperplans porteurs.

| dimension $j$ | taille $q$ | candidat |
| ---: | ---: | --- |
| 3 | 1 | $c=p$ dans la cellule contenant $p$ |
| 2 | 2 | milieu de la paire |
| 1 | 3 | pied métrique, donc circumcentre du triangle |
| 0 | 4 | sommet de l'arrangement |

L'émission exige encore, dans cet ordre :

1. projection appartenant à la strate pertinente;
2. centre dans $\mathrm{relint}\,\mathrm{conv}(U)$;
3. indépendance affine et shell complet;
4. profondeur stricte exacte;
5. propriétaire diamétral canonique;
6. rang fermé et niveau exacts.

Sous `RelevantGP`, la formule est $\mathrm{rang}_{\mathrm{ferme}}=(4-j)+\mathrm{profondeur}$. En dégénérescence, elle devient $\lvert\mathrm{shell}\rvert+\mathrm{profondeur}$ après reconstruction du shell complet; $4-j$ n'est plus une autorité.

Pour $s_{\max}=11$, les profondeurs maximales admises sont 7, 8 et 9 pour les supports quatre, trois et deux. Les seuils de réfutation sont donc respectivement 8, 9 et 10 témoins stricts. Le support singleton doit rester un chemin séparé.

### 2.3 P0 — A2e est l'arrangement sur tout $H_u$, pas « une 2-face »

Une 2-face portée par $H_u$ est une région polyédrique dont l'enveloppe affine est $H_u$; elle n'est pas le plan entier. A2e est la restriction de **tout l'arrangement** à $H_u$, puis son clipping par l'ellipse de Jung et son filtre de diamètre.

Il faut donc choisir une architecture non circulaire :

- soit construire directement le sous-complexe A2p stratifié et extraire ses événements par dimension;
- soit utiliser ce sous-complexe uniquement pour produire une liste canonique de paires, dédupliquer cette liste, puis exécuter A2e une fois par paire;
- soit conserver A2e avec une autre source complète d'ancres.

« Exécuter A2e sur chaque 2-face » mélange les objets et multiplie le travail. Un support quatre générique peut être incident à trois plans porteurs pour chacun de ses quatre sommets, donc être revu jusqu'à douze fois avant le filtre propriétaire. Ce coût doit apparaître dans le ledger.

PEL-1 doit prouver une inclusion de complétude — chaque paire diamétrale canonique utile apparaît parmi les plans porteurs produits — et non une égalité entre toutes les 2-faces et les ancres utiles. La réciproque est fausse sans projection, bon centrage, condition de diamètre et clipping de Jung.

### 2.4 P0 — l'arrêt local par distance ne couvre pas les composantes non bornées

Le certificat proposé n'est sûr qu'avec la quantité exacte $\rho_p=\sup_{c\in T_{\mathrm{courant}}}\lVert c-p\rVert$, où $T_{\mathrm{courant}}$ désigne **tout** le sous-complexe actuellement admissible. Si une composante est non bornée, $\rho_p=+\infty$ et aucun point restant ne peut être écarté par distance.

Ce cas n'est pas marginal. Si $p$ est un sommet exposé de $\mathrm{conv}(X)$, il existe une direction extérieure $v$ telle que $v\cdot(u-p)<0$ pour tout $u\neq p$. Le rayon $p+tv$, $t>0$, reste de profondeur zéro; $V_K(p)$ est donc non borné pour tout $K$. Un nuage en position convexe peut avoir $\Theta(n)$ sommets exposés tout en évitant les dégénérescences pertinentes. La stratégie peut alors insérer les $n-1$ plans pour $\Theta(n)$ ancres.

PEL-3 n'est pas seulement « probablement faux » : l'implication « face non bornée donc aucune sphère critique finie » est réfutée par deux points. Le plan médiateur est non borné et contient néanmoins leur milieu critique fini.

Des réparations sont possibles, mais elles doivent être prouvées séparément : clipping sûr par une région de centres bien centrés, certificat par feature plutôt que par union entière, traitement directionnel des strates non bornées, ou repli dense explicite. Le clipping par $\mathrm{conv}(X)$ conserve la correction puisque tout centre critique appartient à $\mathrm{conv}(U)\subseteq\mathrm{conv}(X)$; il ne prouve toutefois aucune sélectivité et ne doit pas introduire une structure globale non budgétée.

### 2.5 P1 — la complexité annoncée compare des objets incompatibles

La borne des premiers niveaux de $m_p$ plans donne un coût worst-case en $O(m_pK^2)$ pour une ancre, sous le modèle et les hypothèses de l'algorithme choisi. Sans arrêt certifié, $m_p=n-1$ et la somme sur les ancres peut atteindre $O(n^2K^2)$. Rien ne permet encore d'écrire $\Theta(nK^2)$ pour le chemin produit.

Les deux diagnostics invoqués ne mesurent pas $m_p$ :

- 450 à 510 est un nombre de sphères critiques v2 par point, issu d'un catalogue incomplet;
- 175 est une taille de voisinage A2e/v2, pas le nombre de plans insérés par A2p.

Le census A2pe doit publier, par ancre et par dimension : plans examinés et insérés, composantes non bornées, strates visitées, candidats projetés, rejets de centrage/shell/rang, événements uniques et bytes de transcript. La sensibilité à la sortie doit avoir un terme d'entrée, par exemple $O(m_p\,\mathrm{polylog}(m_p)+Z_p)$; elle ne peut être `O(sortie)` quand la sortie est vide.

### 2.6 P1 — la profondeur seule ne fournit pas le payload HGP

Un niveau numérique ne suffit pas. Chaque événement doit transporter les identifiants strictement intérieurs, le shell complet, le support minimal, le centre, le niveau rationnel, le propriétaire et les incidences nécessaires aux bras. Il faut donc soit des listes de conflits shallow bornées, soit un range-report exact et sensible à la sortie au centre candidat.

Le programme M2 devra comparer ce transcript complet à l'oracle, pas seulement les nombres de cellules et leurs profondeurs. Il devra également rester éphémère par ancre : persister l'union des complexes A2p sur les $n$ points reconstruirait une mosaïque globale contraire à l'invariant du projet.

### 2.7 P1 — un prototype ne « règle » pas à lui seul PEL-1 à PEL-4

Le plan en cinq lignes réintroduit les noms M1/M2/M3 alors que le document vient de retirer M1–M5 pour ne pas heurter l'obligation normative M.1. Il faut employer `V3-O`, `V3-P` et `V3-C`, ou des noms sans collision.

Un prototype exact sur une ancre peut :

- falsifier une double inclusion;
- fournir les fixtures minimales;
- mesurer $m_p$, les strates et le coût des prédicats;
- comparer deux représentations sur un corpus fini.

Il ne peut pas, par une campagne finie, démontrer une complétude universelle, une terminaison sur les strates non bornées ou une borne asymptotique. Il ne « mesure pas A1-source gratuitement » : il mesure sur corpus un sur-ensemble de paires dont le coût de construction est précisément la question ouverte.

## 3. Obligations encore ouvertes dans A2e et dans le pipeline

### 3.1 Supports deux, trois et quatre

Le support quatre emploie $t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq D^2/8$. Le support trois doit porter explicitement sa région $t^{\mathsf{T}}(B^{\mathsf{T}}B)t\leq D^2/12$, ainsi que ses propres $c_e^{(3)}$, $\delta_e^{(3)}$ et $\kappa_e^{(3)}=s_{\max}-3-c_e^{(3)}$. Le support deux est le point $t=0$ et exige dix témoins stricts pour être réfuté au rang 11.

Un constructeur des seuls sommets shallow couvre support quatre, pas les pieds situés sur les arêtes ni $t=0$ dans les faces. Il faut le complexe utile faces--arêtes--sommets, ou un calcul batched de profondeur aux pieds et à l'origine sans coût quadratique.

### 3.2 Le masque carrier est fail-open

La règle sûre n'est pas « inclure seulement si $2R(z)\geq D$ est prouvé ». Elle est : **conserver comme carrier tant qu'un majorant certifié n'a pas prouvé $2R_{\mathrm{hi}}(z)<D$**. Une valeur non finie, sous-normale, débordée ou un intervalle traversant le seuil reste éligible.

Les formes de tous les témoins restent présentes pour le rang, y compris lorsque leur point est exclu comme carrier. La proposition dit cela correctement au §7, mais son tableau GPU conserve encore `max_tau_hi`; il doit utiliser `max_two_R_upper_hi`. La finitude mathématique $R\leq\mathrm{diam}(X)$ ne fournit ni un majorant algorithmique sélectif ni son coût.

### 3.3 La recommandation synthétique contredit encore le corps du document

Le bloc A2pe recommande encore une descente dans une boule fixe $\bar B(c,2r)$ et une consommation directe « en flux ». Il doit dire : requête dans la miniboule **courante** à chaque remplacement, puis runs bornés, tri par niveau rationnel exact, merge déterministe et groupement atomique des niveaux égaux avant réduction.

Gate F doit reprendre cette chaîne entière. Gate G doit devenir une vraie porte avec critères : profil `hgp_reduced` ou `full_pi0`, source $\Gamma_k$, facettes/cofaces, incidences silencieuses, M.1 si applicable, `coverage_delta`, `coverage_log`, lots, verticales et carrés de naturalité. Le census de charge doit compter ces objets aval; une sortie géométrique sparse ne garantit pas une source HGP sparse.

### 3.4 Qualité documentaire

Quelques corrections mécaniques restent nécessaires :

- `77 à 214 ans` doit être `77 à 215 ans` avec les valeurs affichées;
- le bloc de $h_x$ occupe deux lignes physiques entre `$$`, interdit par `AGENTS.md`;
- l'étiquette accentuée `théorème` est placée directement dans `\textbf` en mode mathématique;
- Gate A doit inclure A2p, A2pe et PEL, puis apparaître dans l'ordre des travaux;
- « la seule mesure qui compte est $R$ » n'est vrai que pour la coupe carrier; les témoins, strates et objets HGP ont leurs propres distributions.

## 4. Résultat concurrent du center-cover sur G4

Ce résultat concerne `morsehgp3d`, pas l'oracle v3, mais il informe directement l'alternative A2e + A1-source.

Contexte exact : `phase=15`, `backend=cuda_g4_plus_reference_cpu_oracle`, `profile=hgp_reduced`, `mode=proposal_only_center_cover_prune_mass_falsifier_v1`.

- builds CUDA Release et Audit réussis, AOT `sm_120` sans PTX;
- n=32 Release et Audit : oracle, contrat d'audit et identité de masse verts;
- Compute Sanitizer : zéro erreur et zéro fuite;
- `uniform_latin` 50 k : timeout unique après 600 s, aucun JSON;
- `eight_clusters` 50 k : interrompu sur instruction utilisateur, aucun retry.

Ce n'est pas une réfutation du théorème center-cover. C'est une falsification nette de l'implémentation actuelle comme chemin produit 50 k. La porte P1a reste fermée et aucun claim scientifique/public n'est permis. Cela renforce l'intérêt d'A2pe, mais ne transforme pas ses obligations ouvertes en preuves.

## 5. Audit de l'oracle M1 en cours de développement

### 5.1 Snapshot et bons choix

| fichier | SHA-256 |
| --- | --- |
| [`bigint.hpp`](../oracle/bigint.hpp) | `ce6227b962d39fdc680adb123c3d44a81acf5ee2f8862ba396634a9e4fa00a05` |
| [`rational.hpp`](../oracle/rational.hpp) | `51e30daeb0f00db2b5ee98c3b1bd212246287c27bbf32244134572f619b0f71e` |
| [`bigint_selftest.cpp`](../oracle/bigint_selftest.cpp) | `4ede41cd234c47eb9f8da02ff94763086d4c8d0f7083318ae04874140f3f1727` |
| [`oracle_main.cpp`](../oracle/oracle_main.cpp) | `7787b24804ce79d5f1fa4013e12dff46e2f062c00c5692bdf26e9ad4f4c14a7d` |
| [`anchored_catalogue.hpp`](../prototype/anchored_catalogue.hpp) | `ba5ef6aeb7e5384c9a825d0138fd37967bf9559457347164c78229013b00eeb5` |

Trois choix sont excellents : entier signe--magnitude à taille variable au lieu du complément à deux fixe du sujet; sphères résolues par élimination de Gauss au lieu de Cramer; forêt reconstruite depuis tous les $k$- et $(k+1)$-sous-ensembles de $\Gamma_k$. La campagne utilise enfin toute la grille u16, publie l'identité `attempted = decided + rejected_domain` et a trouvé un vrai défaut de tri des membres dans la v2.

Ces qualités en font une bonne base de juge. Elles ne suffisent pas encore à faire de son `OK` une porte.

> [!WARNING]
> Les §§5.2 à 5.7 conservent les défauts observés sur le premier snapshot de M1 afin de documenter les régressions qui doivent rester fermées. Le delta live a depuis corrigé les planchers vacus, les rationnels nuls, le centre exact, plusieurs champs de forêt et l'absence de GMP. Le statut courant et les défauts résiduels sont synthétisés au §5.11 et détaillés dans l'[audit autonome M1/M2.1](AUDIT_ORACLE_M1_ET_PROTOTYPE_M2_1.md).

### 5.2 P0 — une campagne entièrement vide peut rendre `OK`

Les arguments `--min-decided` et `--min-nodes` ne sont pas validés. Avec des planchers nuls ou négatifs, une campagne dont tous les nuages sont rejetés satisfait l'identité de fermeture et sort avec le code 0.

Reproduction sur le snapshot audité :

```text
mhgp3v_oracle --clouds 1 --seed 4242 --min-points 8 --max-points 8 --max-order 1 --coord-max 1 --min-decided 0 --min-nodes 0
attempted=1 decided=0 rejected_domain=1 | spheres=0 forets=0 noeuds=0
OK : campagne fermee, structure complete comparee sur la grille declaree
```

Les planchers doivent être strictement positifs, validés par un parseur sans suffixe ni overflow et inscrits dans le reçu. Une campagne positive devrait exiger son quota décidé par famille; les dégénérescences attendues doivent vivre dans une campagne négative distincte avec leurs propres quotas. `identity_closed=true` signifie seulement « rien n'a disparu », jamais « assez de décisions ont été exercées ».

### 5.3 P0 — l'invariant « dénominateur strictement positif » n'est pas imposé

`Rational(BigInt n, BigInt d)` accepte $d=0$. Le chemin de normalisation transforme même silencieusement $0/0$ en $0/1$, parce que le cas numérateur nul force le dénominateur à un. $1/0$ reste un objet dont le dénominateur a le signe zéro. `operator/` repose seulement sur un commentaire demandant à l'appelant de tester le diviseur.

Un juge doit échouer fermé ici : constructeur contrôlé, statut explicite ou exception; garde du diviseur dans `operator/`; validation de `Sphere.den>0` avant `exact_level_of`; fixtures permanentes $1/0$, $0/0$ et division par zéro. Sans cela, une sphère sujet malformée de numérateur et dénominateur nuls peut être lue comme un niveau zéro valide.

### 5.4 P0 — le catalogue compare le rayon, pas le centre exact

La clé support, le rang, les membres et $\lVert\mathrm{num}\rVert^2/\mathrm{den}^2$ sont comparés. En revanche, le juge ne compare pas le centre rationnel du sujet au centre de Gauss. Un numérateur tourné ou une base corrompue peut conserver la même norme et donc le même niveau tout en décrivant une autre sphère.

Avant toute lecture, il faut aussi valider :

- `1 <= n_support <= 4`;
- IDs de support distincts, triés et dans les bornes;
- `rank`, `members_begin` et toute la tranche dans le pool;
- `sph.support == n_support`, `den>0` et `base` cohérente;
- centre exact, shell, côtés de tous les membres et non-membres.

Ces gardes empêchent un sujet fautif de faire sortir le juge de ses propres tableaux. L'oracle ne doit jamais faire confiance à la structure mémoire de ce qu'il juge.

### 5.5 P0 — la « comparaison structurelle complète » ignore des champs publics

La fermeture canonique des minima est une bonne clé de généalogie, mais les champs suivants ne sont pas comparés :

- `Forest.roots`;
- `first_child`, `next_sibling` et `n_children`;
- `Forest.order`, `births`, `merge_events`, `killed`, `unresolved_arms`, `censored_events` et `authoritative`;
- `Result.censored_orders`;
- sémantique de la source d'une multifusion : rang $k+1$, participation au lot et plus petit index contractuel.

Le code reconstruit l'arité depuis les parents; un `n_children` faux peut donc passer. Il déduit les racines de `parent<0`; une liste `roots` fausse peut passer. Une source de fusion sans rapport mais de même niveau peut passer. Il manque en outre une garde `parent < nodes.size()` avant indexation.

La porte doit comparer les deux représentations d'adjacence entre elles, les compteurs recalculés, les sources canoniques et tous les drapeaux de censure. Pour la v3, elle devra ensuite comparer lots, couverture et verticales selon le profil choisi.

### 5.6 P1 — l'indépendance et la couverture ont encore des limites explicites

- L'oracle cible aujourd'hui `morsehgp3D_v2` et le seul profil `quantized_u16_input`; il ne lit pas les binary64 comme dyadiques exacts.
- La campagne aléatoire courante fixe `threads=1`; elle ne couvre ni permutations, ni nombres de fils, ni ordonnancements, ni fixtures sanctionnées.
- Le reçu [`oracle_campaign_20260808.json`](../receipts/oracle_campaign_20260808.json) ne contient ni commit/binaire/compilateur, ni digests d'inputs, ni planchers, ni manifeste brut.
- Le selftest GMP est optionnel : sans GMP, il imprime `temoin gmp : NON` puis `OK` et retourne 0. Une qualification de largeur arbitraire ne doit pas devenir verte sans son témoin large.
- Les rationnels sont testés par identités internes sur de petits entiers, pas différentiellement contre `mpq_class`.
- Les compteurs de selftest sont incrémentés même lorsqu'une vérification conditionnelle est sautée; `checks` n'est pas une identité de campagne.

Il faut deux statuts : selftest minimal de développement, éventuellement sans GMP, et gate arithmétique qualifiant qui exige GMP ou un second témoin large équivalent. Les profils u16 et dyadique auront des campagnes et reçus distincts.

### 5.7 Vérifications dynamiques de cet audit

Sans modification du dépôt :

- selftest GCC 13, ASan/UBSan, GMP, 3 000 tours : 37 208 vérifications annoncées, sortie `OK`, aucun diagnostic sanitizer;
- même selftest sans GMP, 100 tours : code 0 et `OK`, ce qui confirme le fail-open du témoin large;
- oracle Release, trois nuages sur la grille complète : 3/3 décidés, 200 sphères, 8 forêts, 218 nœuds, largeur maximale 155 bits, sortie `OK`;
- campagne vacue ci-dessus : code 0, 0 décision et 0 nœud.

Le reçu commité 40/40, 1 850 sphères et 1 909 nœuds est un résultat encourageant. Il reste une régression observée, pas une certification structurelle complète.

### 5.8 P0 — le certificat de localité M2.1 est faux

Le prototype [`anchored_catalogue.hpp`](../prototype/anchored_catalogue.hpp) énumère les tuples contenant l'ancre dans une fenêtre de voisins, calcule le plus grand rayon parmi les supports déjà émis, puis arrête si son diamètre n'atteint pas le premier voisin exclu.

L'implication est invalide. Le maximum des supports **déjà trouvés** n'est pas un majorant du rayon d'un support encore inconnu qui utilise un point exclu. La propriété « tout membre d'une sphère de rayon $r$ passant par $p$ est à distance au plus $2r$ de $p$ » certifie le rang d'un candidat déjà trouvé; elle ne certifie pas l'absence d'un candidat plus grand.

Contre-exemple entier u16 exécuté sur le snapshot audité :

- ancre $p=(1000,1000,1000)$;
- vingt voisins proches du côté $x<1000$;
- point lointain $q=(2000,1000,1000)$;
- `s_max=3`, fenêtre initiale 16.

Le prototype publie `certified=1`, `exhausted=0`, `neighbourhood=16`. Pourtant l'oracle exhaustif contient le support critique $\left\lbrace0,21\right\rbrace$ et le catalogue ancré ne l'émet pas : 63 sphères de référence contre 61, aucune dégénérescence et zéro ancre déclarée non certifiée. L'ancre $q$ ne répare pas l'omission, car le propriétaire choisi est le plus petit ID, donc $p$. Les inégalités sont ouvertes; une petite perturbation transverse conserve le contre-exemple tout en supprimant les alignements accidentels.

Le défaut apparaît aussi sans fixture façonnée : avec `--subject anchored --clouds 1 --seed 4242 --min-points 22 --max-points 22 --max-order 1 --seed-neighbours 16`, l'oracle trouve 70 sphères contre 69 et le support $\left\lbrace6,10\right\rbrace$ manque, tandis que les 22 ancres sont annoncées certifiées, aucune épuisée, avec voisinage moyen et maximal 16. Le CTest courant ne lance aucun sujet `anchored`; ses nuages de 8 à 10 points sont plus petits que la fenêtre 16 et n'exercent donc jamais ce certificat.

Un second faux diagnostic se trouve dans le chemin d'épuisement : `certified` est initialisé à vrai et n'est recalculé que si la fenêtre n'est pas épuisée. Une fenêtre absorbant tout le nuage ressort donc `certified=true`; la branche documentée comme retour faux devient inatteignable. C'est la même confusion entre « complet par épuisement » et « certifié par borne » que celle déjà reprochée à la v2.

Le prototype doit retirer ce certificat. Une fermeture sûre exige soit un majorant indépendant couvrant **tous** les supports possibles, soit l'exhaustivité. Il peut conserver un reçu par candidat déjà stabilisé, mais pas arrêter la source entière avec le maximum observé.

### 5.9 P1 — M2.1 réimplémente la cascade et sous-compte son coût

Le fichier ne construit ni $V_K(p)$, ni sous-complexe stratifié, ni peeling. Il énumère tous les supports de taille au plus quatre dans la fenêtre : c'est précisément la cascade locale en $\sum_p\binom{\lvert W_p\rvert}{3}$ que la proposition condamne. Le compteur `two_faces` désigne seulement les paires ancre--membre rencontrées dans des supports trouvés; ce ne sont pas des 2-faces A2p.

En cas de croissance, `candidates`, `witness_tests` et `degenerate_shells` sont remplacés par les compteurs du **dernier** tour au lieu d'être cumulés. Le ledger sous-estime donc le travail réellement payé. L'ordre des voisins ne départage pas les distances égales par `PointId`, ce qui affaiblit le déterminisme.

Ce code reste utile comme sujet différentiel borné, jamais comme prototype du peeling ni comme mesure gratuite de PEL. Il doit être renommé en conséquence et conserver séparément travail cumulé, travail du dernier tour et type exact de certification.

### 5.10 P1 — l'intégration live n'est pas encore bâtissable ni reproductible

Sur les hashes indiqués en tête, le build CMake échoue : `oracle_main.cpp` inclut `prototype/anchored_catalogue.hpp`, mais la cible n'ajoute que le dossier `oracle/` à ses includes. L'erreur est `fatal error: prototype/anchored_catalogue.hpp: No such file or directory`.

Le nouveau mode `--measure-only` ignore aussi `--receipt` et retourne 0 même en présence d'ancres non certifiées ou de dégénérescences; cela peut convenir à un diagnostic seulement s'il publie explicitement `status=diagnostic_only` et une identité de campagne. Dans le mode `--subject anchored`, le JSON conserve la chaîne codée en dur `morsehgp3D_v2 build_catalogue + run`, donc un reçu peut annoncer le mauvais sujet.

Enfin, les forêts du sujet `anchored` sont encore construites par `mhgp::build_forest` de la v2. C'est acceptable pour isoler le catalogue dans un différentiel, mais cela ne qualifie ni une forêt v3 ni le pipeline HGP proposé.

### 5.11 Delta live — corrections acquises et verrous restants

Le delta courant retire explicitement le faux certificat, sépare `exhaustive` de `assumed_window`, départage les distances égales par `PointId`, corrige la mesure fermée $d^2\leq4r^2$, répare l'include CMake et ajoute un CTest ancré exhaustif. Les planchers de campagne, le dénominateur rationnel, le centre exact et les champs publics majeurs de la forêt sont également contrôlés. Ces corrections sont substantielles.

Quatre verrous demeurent prioritaires :

1. une source de multifusion étrangère mais de même rang et niveau passe encore;
2. un record catalogue illisible fait échouer `compare_catalogues`, mais le `main` poursuit la lecture de la forêt au lieu d'échouer atomiquement;
3. le CTest ancré ne couvre que `s_max=2`, donc pas les supports trois et quatre;
4. les reçus ne scellent ni le binaire, ni la toolchain, ni les digests des nuages, et `measure-only` ignore encore `--receipt`.

Le nouveau `sufficient_neighbours` est une mesure a posteriori honnête uniquement sous régime exhaustif. Il ne constitue pas une coupure en ligne et son calcul exhaustif conserve la cascade combinatoire. Sous `assumed_window`, il est seulement un minorant diagnostique.

## 6. Portes recommandées avant M2

### V3-O0 — arithmétique du juge

- dénominateur nul impossible par construction;
- division nulle fail-closed;
- différentiel entier et rationnel contre un témoin large obligatoire;
- ASan/UBSan, cas extrêmes, tailles croissantes et compteurs exacts;
- deux profils d'entrée nommés.

### V3-O1 — lecture hostile du sujet

- validation de tous les indices, tailles, tranches, enums, dénominateurs et liens avant déréférencement;
- mutations négatives de chaque champ public;
- aucune corruption ne doit crasher le juge ou être normalisée en donnée valide.

### V3-O2 — campagnes fermées et reproductibles

- campagnes positives et négatives séparées;
- quotas strictement positifs par famille et identité complète;
- permutations, fils, égalités, dégénérescences et fixtures des warnings;
- reçu avec commit, binaire, compilateur, options, digests, planchers et manifeste.

### V3-A0 — objet mathématique A2pe

- définir le sous-complexe stratifié, pas seulement $V_K(p)$ comme ensemble;
- fixture permanente à deux points;
- dictionnaire strate--projection--support--rang démontré;
- PEL-3 retiré ou remplacé par un traitement exact des strates non bornées.

### V3-A1 — prototype CPU borné

- oracle exhaustif par ancre sur petits $n$;
- faces, arêtes, sommets et supports 1/2/3/4;
- transcript de conflits, shell, owner et niveau;
- aucun arrêt par le maximum des supports déjà observés; fixture du support lointain et campagne aléatoire à 22 points obligatoires;
- ledger cumulé sur tous les tours de croissance et départage des distances égales par `PointId`;
- aucune affirmation asymptotique tirée d'une campagne finie.

### V3-A2 — census de décision

- $m_p$, strates par dimension/profondeur, non-borné, high-water et temps;
- comparaison directe A2p, A2pe et A2e + source;
- objets HGP aval, tri et lots inclus dans le ledger;
- no-go architecture si sortie sparse mais intermédiaires denses.

### V3-H — source et réduction HGP

- profil explicite;
- incidences actives/silencieuses, attaches, couverture, M.1 si applicable;
- runs triés, merge exact, lots atomiques;
- descente par miniboule courante;
- verticales et naturalité avant toute publication.

## 7. Décision finale

La proposition actuelle contient probablement la meilleure intuition apparue jusqu'ici : **utiliser le complexe shallow ancré par point pour rendre les paires visibles, puis exploiter la réduction 2D sur les plans médiateurs**. Mais l'identité géométrique « A2e vit dans $H_u$ » ne fournit pas encore l'algorithme A2pe.

La décision est :

- **GO** pour formaliser le sous-complexe stratifié et construire un prototype CPU exact, éphémère par ancre;
- **GO** pour renforcer M1 avant tout autre code produit;
- **GO** pour confronter A2p/A2pe/A2e sur les mêmes petits oracles et les mêmes ledgers;
- **NO-GO** pour dire que les 2-faces de l'ensemble $V_K(p)$ constituent déjà la source complète;
- **NO-GO** pour fonder la localité sur un rayon fini sans traiter les strates non bornées;
- **NO-GO** pour présenter le résultat 40/40 de M1 comme une certification tant que les voies vacues et les champs ignorés restent ouverts;
- **NO-GO** pour M2.1 dans son état `ba5ef6ae…` : certificat faux, support omis et build CMake cassé;
- **NO-GO** pour le center-cover actuel à 50 k, le contrat 600 s étant déjà manqué;
- **NO-GO** pour tout statut exact, SLO ou claim public.

Le prochain livrable décisif est un couple, pas un gros catalogue : **un oracle M1 rendu hostile et fail-closed**, puis **un constructeur CPU du complexe shallow stratifié pour une ancre**, comparé exhaustivement et instrumenté. Ce couple dira si A2pe est une vraie voie produit ou seulement un excellent oracle 3D pour A2e.

## 8. Références

- [`PROPOSITION.md`](../PROPOSITION.md), snapshot audité.
- [`AUDIT_PROPOSITION.md`](AUDIT_PROPOSITION.md), premier audit historique.
- [`README.md`](../README.md), statut actuel de M1.
- [`oracle/oracle_main.cpp`](../oracle/oracle_main.cpp), juge structurel.
- [`oracle/rational.hpp`](../oracle/rational.hpp), rationnels exacts du juge.
- [`prototype/anchored_catalogue.hpp`](../prototype/anchored_catalogue.hpp), sujet M2.1 encore incomplet.
- [`receipts/oracle_campaign_20260808.json`](../receipts/oracle_campaign_20260808.json), premier reçu M1.
- [`receipts/census_tukey_shallow_20260808.json`](../receipts/census_tukey_shallow_20260808.json), census diagnostique.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md), réduction A2e et center-cover.
- [`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md), objet et domaine normatifs.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md), autorité de statut des preuves.
