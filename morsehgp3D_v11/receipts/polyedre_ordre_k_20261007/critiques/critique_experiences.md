# Critique expérimentale adverse : complexe alpha d'ordre K, rendu, réduction, robustesse, échelle

Date : 7 octobre 2026. Travail de 00 h 42 à 01 h 23 UTC (heures lues par `date -u`). Rôle : critique expérimentale
adverse du workflow « généraliser le complexe alpha à l'ordre K pour représenter robustement les niveaux d'un
polyèdre ».

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Aucun commit, aucun push. Toutes les écritures sont dans ce dossier (environ 0,1 Mo). Calculs sous `nice -n 10`, sur
un fil. Statuts employés : **[P]** prouvé (ailleurs, invoqué), **[V]** vérifié borné (oracle exact), **[M]** mesuré,
**[J]** jugement visuel, **[C]** conjecture.

## 0. Verdict en bref

1. **Les quatre expériences tiennent sur ce que j'ai pu rejouer.** Une mesure clé de chacune a été refaite
   indépendamment, sur l'oracle exact quand c'était possible. Aucun désaccord, aucune contradiction mathématique.
   - **Rendu** : 228 couples (nœud, niveau) sur 228. Mêmes comptes par dimension, mêmes faces exposées et strates
     isolées, même amas discret. **[V]**
   - **Réduction** : le journal est transporté sur l'oracle par les clés (I', U'). Il est accepté par mon juge et
     par celui de l'oracle. χ et π0 sont égaux à tous les niveaux. L est identique au tableau publié. **[V]**
   - **Robustesse** : d_B(H0) ≤ δ dans 4 cas sur 4. Un site ajouté crée une composante (3 cas sur 3). Les vies des
     nœuds FULL sont reproduites à la 4e décimale. **[V]**
   - **Échelle** : à 8 000, 16 000 et 32 000 sites, les comptes sont recomptés par un autre outil, sans écart. **[M]**
2. **Plusieurs énoncés des résumés sont plus forts que leurs preuves.**
   - « 0 écart π0 avec FULL (32/32) » du rendu est une **égalité de comptes** (`jeton.bijection`), pas une
     identification.
   - Les certificats de réduction à 16 000 et 32 000 sites, k = 2, ne vérifient qu'un **échantillon** de dates
     (3,3 % et 1,5 %). Or la même expérience montre qu'un échantillon laisse passer un mutant de dates.
   - Les « 1 003 à 1 086 faces par site à K = 5 » sont des densités de **mosaïques locales**, prises sur 400 à 2 470
     sites. Les « 48 M faces » d'une trame sont une **extrapolation**.
   - La quantification n'a **jamais exercé la fusion de sites**, qui est précisément le cas signalé par l'auditeur.
3. **Reconnaissabilité [J]** :
   - Le **vélo synthétique** se reconnaît pleinement au dual A_5 des **ancêtres intermédiaires** (77 à 89 mm).
     Au nœud « objet », les roues sont des disques pleins : ce n'est plus qu'une silhouette.
   - **L'anneau**, la **roue** à son nœud, le **piéton** et les **six points** se reconnaissent.
   - Les **deux nappes** se séparent jusqu'à K = 3, mais pas à K = 5. Le meilleur nœud de la nappe arrière y est le
     nuage entier (IoU 0,43).
   - **Le vélo réel n'est reconnaissable dans aucun candidat**, pas même à K = 1, et cela malgré une sélection du nœud
     par la vérité terrain.
   - L'**offset** n'est jamais une forme reconnaissable, sauf pour l'anneau.
   - À K = 5, les nœuds les mieux reconnus vivent de 0,1 à 0,4 mm, sous le bruit de quantification (√3/2 mm). La
     forme reconnaissable appartient donc à un **rayon de la chaîne**, pas à l'identité d'un nœud.
4. **Auditeur** : aucune réponse après 28d70f8ab. Le dépôt a été relu à 00 h 43 et à 01 h 21 UTC ; seuls des commits
   « v11 » du développeur ont suivi, sans rapport avec les polyèdres.

## 1. Les prédictions précèdent-elles les mesures ?

| expérience | PREDICTIONS.md (mtime UTC) | sha256 | première sortie (mtime UTC) | verdict |
| --- | --- | --- | --- | --- |
| rendu | 22:19:52 | 10f81674… = fichier .sha256 | 22:34:25 (`resultats/sortie_campagne.log`) | précède |
| réduction | 22:54:17 | d3a471de… = fichier .sha256 | 23:10:53 (`journal_reference.npy`) | précède |
| robustesse | 22:59:46 | 9dbaf9c9… = fichier .sha256 | 23:06:25 (cache des entrées) | précède |
| échelle | 23:02:31 | 5e0041f2… = fichier .sha256 | 23:04:48 (entrées) | précède |

Limite : la preuve repose sur des mtimes et un sha256 écrit au même instant, pas sur un commit. C'est cohérent, mais
cela ne vaut pas un scellement externe.

Les paramètres sont fixés dans les prédictions avant toute mesure : θ = 1/2 (réduction), σ = 1, 3 et 10 mm,
h = 1, 2 et 5 mm, décimation 50 % (robustesse). **Rien n'est réglé sur la découpe réelle dans les méthodes.** En
revanche, **tous les nœuds montrés sont choisis par l'oracle**, c'est-à-dire par l'IoU contre la vérité, découpe
réelle comprise. Les jugements de reconnaissabilité sont donc des **bornes optimistes**. Toutes les expériences le
déclarent.

## 2. Rejeux indépendants

Scripts : `rejeu_oracle.py` (passes 1 et 2), `vies_oracle.py`, `budget_oracle.py`, `recompte_k2_8000.py`. Sorties :
JSON du même nom. Jeu commun pour l'oracle : `synth_anneau_perce_10m`, 52 sites, sous la borne n ≤ 60.

Les mosaïques de l'oracle ont 52/285/437/203 faces à k = 1, 285/1 508/1 943/719 à k = 2 et 634/3 502/4 518/1 649 à
k = 3. Ces comptes sont identiques à ceux publiés par le rendu et par la réduction.

### 2.1 Rendu

- **Rejeu [V].** Je prends tous les couples (nœud, niveau) mesurés par `experience_rendu` sur ce jeu : 228, pour
  k = 1, 2 et 3. Pour chacun, une composante de A_k(r) de l'oracle a exactement :
  - les mêmes comptes (sommets, arêtes, 2-faces, 3-cellules) ;
  - le même nombre de « faces exposées », c'est-à-dire 2-faces bordant une seule 3-cellule active plus strates
    maximales de dimension 0 à 2 ;
  - le même amas discret, compté par les labels.

  Résultat : **228 sur 228**.

  Ma première passe donnait 210 sur 228. La faute était **la mienne** : j'approchais les niveaux flottants par
  `limit_denominator`. Une fois les niveaux recalés sur le niveau exact de l'oracle, le rejeu donne 228 sur 228
  (`rejeu_oracle_passe1.json`, puis `rejeu_oracle.json`).
- **Contre-épreuve.** Le résumé dit « 0 écart π0 avec FULL sur les 32 couples » et « s'identifie sans écart aux nœuds
  FULL par les labels ». Or le champ `bijection` vient de `jeton.bijection`. Cette fonction compare, à chaque niveau
  critique, le **nombre** de composantes au **nombre** de nœuds vivants. L'oracle l'avait déjà signalé : elle
  « compare des comptes, pas des identités ».
  - L'identité n'est vérifiée **que pour les nœuds mesurés**, tous choisis par l'oracle : naissances dans une seule
    composante, aucune absente.
  - L'identification nœud par nœud existe ailleurs : dans l'oracle (91 ordres) et dans l'échelle (417 nœuds). Elle
    n'existe pas dans le rendu.
- **K = 1 [V].** L'oracle et gudhi donnent les mêmes 977 simplexes. Les niveaux diffèrent d'au plus 7 × 10^-12 en
  relatif, ce qui est le flottant de gudhi sur des coordonnées non centrées d'environ 7 × 10^4 mm.
  - Le rendu signale honnêtement 3 jeux sur 8 où gudhi triangule des cellules cosphériques que la mosaïque garde en
    polytopes. Les niveaux y sont égaux.
  - L'énoncé juste est donc : **« K = 1 redonne le complexe alpha polyédral (subdivision de Delaunay filtrée) »**,
    égal à l'alpha simplicial hors des cellules dégénérées.

### 2.2 Réduction

- **Rejeu [V], k = 2 et k = 3.** Je relance `noyau.effondrer` (priorité de référence, tous sommets protégés). Je
  transporte le journal sur la mosaïque de l'oracle par les clés (I', U').
  - Le transport est une bijection : 4 455 faces sur 4 455 à k = 2, 10 303 sur 10 303 à k = 3.
  - Deux juges l'acceptent, 1 807 et 4 278 paires :
    - **mon juge**, écrit dans ce dossier : niveaux exacts de l'oracle, facette, liberté dans le complexe **entier**
      toutes dimensions, sommets protégés, fermeture ;
    - **`oracle_ak.verifier_effondrements`**.
  - χ(L_r) = χ(A_r) et les partitions des sommets en composantes sont égales à **tous** les niveaux (1 109 et 2 199).
  - L = 285/420/136/0 et 634/873/240/0, soit L/K = 0,189 et 0,170 : **identiques à `TABLEAUX.md`**.
  - Un mutant (paire libre de niveaux exacts différents) est refusé par les deux juges.
- **Échelle, recompte par un autre outil [M].** J'utilise qhull et un test de sphère exact en entiers ; il ne trouve
  **aucune** adjacence cosphérique, donc la triangulation est ici la subdivision.

  | sites | arêtes de Delaunay | sommets de la mosaïque d'ordre 2 publiés | tétraèdres | 3-cellules d'ordre 1 publiées |
  | --- | --- | --- | --- | --- |
  | 8 000 | 59 221 | 59 221 | 51 095 | 51 095 |
  | 16 000 | 119 323 | 119 323 | 103 212 | 103 212 |
  | 32 000 | 239 215 | 239 215 | 207 125 | 207 125 |

  Euler vaut 1 pour les six mosaïques d'échelle et pour leurs L. Cela confirme le 0-squelette d'ordre 2 et la
  mosaïque d'ordre 1, **pas** les 3-cellules d'ordre 2.
- **Certificats à l'échelle : incomplets à 16 000 et 32 000 sites, k = 2.**
  - Les dates sont vérifiées en entier à k = 1 (8 000, 16 000 et 32 000 sites) et à k = 2 (8 000 sites).
  - À k = 2, 16 000 et 32 000 sites, seules 20 000 dates sont vérifiées, sur 614 247 et 1 316 072 paires.
  - Or l'expérience montre elle-même qu'un échantillon de 50 dates laisse passer un mutant.
  - Ces deux certificats ne sont donc **pas établis**. Le README le dit ; le résumé (« persistance identique jusqu'à
    32 000 sites ») vaut **à k = 1 seulement**.
- **Indépendance du vérificateur.** `Verificateur` est dans le même fichier (`noyau.py`) et a le même auteur. Il
  recalcule cependant les sphères en entiers. Les paires sont proposées par égalité de **flottants** (`b[s] == b[t]`) ;
  l'exactitude ne vient que de la vérification des dates. C'est acceptable seulement si toutes les dates sont
  vérifiées.
- **Budget par composante.** D_v est exact et calculé par composante. En revanche, d_H(L, A) et d(C, L) sont
  **échantillonnés**, donc ce sont des bornes inférieures : « 0 dépassement » est un filet, pas une preuve. La borne
  elle-même est un théorème de l'auditeur, invoqué. Mon recalcul de D_v/r sur toutes les composantes à tous les
  niveaux est au § 2.5.
- **Forme [J].** Le « L ouvre des roues pleines » est un **artefact d'ordre d'effondrement** : la planche
  `planche_synth_velo_10m_k5_objet0` montre un filet creux, sans changement de H1. C'est exactement l'avertissement de
  l'auditeur : « homotopie et emboîtement ne contrôlent pas la forme ». Cela ne doit jamais compter comme un gain de
  reconnaissabilité.

### 2.3 Robustesse

- **Positions appariées [V].** Déplacement entier |d| ≤ 3 mm (δ = 3 mm), deux tirages, k = 1 et 2. d_B(H0) vaut
  1,485 / 1,498 / 2,291 / 2,291, soit 0,50 à 0,76 δ, avec les barres infinies comprises. Le résultat est conforme au
  théorème d'entrelacement. **Quatre cas seulement, H0 seulement : H1 et la DTM ne sont pas rejoués.**
- **Population [V].** J'ajoute **un** site à environ 2 espacements, vers l'extérieur, comme le genre `groupe_proche`
  (m = 1 < k = 2).
  - β0 augmente à 31 à 63 niveaux dans **3 cas sur 3**. La première hausse est au demi-écart au plus proche voisin
    (par exemple 985/2 mm², soit 44,38/2 mm).
  - Un site lointain laisse le diagramme identique sous r* = dmin/2.
  - L'énoncé « moins de k aberrants ne créent rien » est donc faux ici aussi. Mais « 33 sur 33 » découle **du
    protocole** (aberrants posés à 2 espacements) : ce n'est pas une fréquence sur données réelles.
- **Vies des nœuds [V].** Mon arbre de fusion, recalculé sur l'oracle (naissances, fusions par plateaux d'un bloc),
  donne 103, 122 et 110 nœuds. Ce sont **exactement** les nœuds FULL de `vies.json`. Les parts de vies supérieures à
  2δ (δ = 3,3, 12,3 et 33,6 mm) sont identiques à la 4e décimale. Vie médiane : 15,5 / 3,1 / 7,7 mm (k = 1 / 2 / 3).
- **Critiques.**
  1. **Quantification : la fusion de sites n'est jamais exercée.** On a 0 fusion sur 48 cas, parce que l'espacement
     des jeux synthétiques est bien supérieur à 5 mm. Le cas signalé par l'auditeur (fusion de retours, rupture de la
     bijection) reste donc **non testé**. Ce sont les trames LiDAR à 1 mm qui l'exerceraient.
  2. δ est le **déplacement maximal** observé, environ 4σ pour un bruit gaussien. La borne d_B ≤ δ est un théorème :
     64 sur 64 teste l'implémentation, rien de plus.
  3. Le contrat de masse (transport pondéré) **n'est pas fait** : k' = arrondi(kρ) est une heuristique.
  4. L'expérience n'a **pas de README** : ses résultats ne sont que dans `resultats/tables.md` et `synthese.json`.
  5. Le programme « deux nappes » n'est pas fait.

### 2.4 Échelle

- **Tailles [M].** `points.u32le` / 12 donne exactement 8 000, 16 000 et 32 000 sites, plus les tuiles 8 027,
  16 054 et 32 108.
- **Nœuds par site.** Recalculés depuis les manifestes FULL : 4,65 à 4,80 à K = 2 et 13,63 à 16,08 à K = 5 sur les
  sous-ensembles KITTI. Les tuiles donnent 4,01 et 9,34 à 9,36. Le tableau du README est juste. Les fourchettes du
  texte (« 4,5–4,8 » et « 13,3–16,1 ») ne correspondent à aucune ligne publiée et omettent les tuiles. C'est un écart
  mineur.
- **Densités.** Les 132 à 138 (K = 2) et 1 003 à 1 086 (K = 5) faces par site sont mesurés sur des **mosaïques locales
  Y** de 400 à 2 470 sites, pas sur les nuages de 8 000 à 32 000 sites.
  - À K = 2, la réduction fournit des nuages entiers : 142 à 144 faces par site, ce qui est cohérent.
  - À K = 5, **aucun nuage entier n'est mesuré** (« trame entière à K = 5 non mesurée »). Les « 6,2 M et 48 M faces
    pour 45 845 sites » sont une densité multipliée par n, donc une extrapolation. Le résumé les présente comme un
    résultat (« donne »).
- **Identité à FULL.** Pour 417 nœuds sur 417, les naissances du sous-arbre sont toutes dans la composante, aucune
  naissance extérieure n'y est, et il y a une seule composante. C'est une **vraie identification** par les labels.
  À K = 5, les nœuds « oracle » d'objets vivent de 0,2 à 0,4 mm (258,2 → 258,4 ; 151,1 → 151,5 ; 134,5 → 134,8 mm),
  sous le bruit de quantification. À K = 2, leur vie va de 0,2 mm (83,1 → 83,3) à 827 mm (144,6 → 971,5).
- **Vérificateur local (`mesure_locale.verifier`).** Les dates sont recalculées par l'étoile depuis les niveaux propres
  **de la construction** (`O.own`), en exact seulement en cas d'égalité de flottants. Le journal est jugé sur le
  **masque** du nœud. Ce certificat est donc **nœud par nœud**, ce que l'auditeur juge insuffisant. L'expérience le
  confirme elle-même (47 suites sur 57 non exécutables sur la vie du parent) et propose de certifier au sommet de la
  chaîne puis de restreindre. Cette restriction est valide : un effondrement reste dans sa composante.
- **Coût.** Les 100 ms ne sont jugés que par extrapolation, sous l'hypothèse H (un constructeur natif au coût par
  élément de la tour) : c'est **non mesuré**.

### 2.5 Budget géométrique par composante (oracle)

Voir `budget_oracle.json` : maximum de D_v(r)/r sur toutes les composantes de A_k(r), à tous les niveaux, anneau
10 m, k = 2 et 3. Il est confronté à la borne 4/k (position générale) et au maximum publié par la réduction (1,70 à
k = 2, sur 27 couples choisis).

**§ 2.5 bis, résultat [V].** À k = 2, sur 4 119 couples (composante, niveau) :

- max D_v/r = **1,876**, sous la borne 4/k = 2 : aucun dépassement ;
- les 27 couples choisis par la réduction (max 1,70) **sous-estiment** le maximum réel, ce qui est attendu d'une
  sélection ; la borne tient tout de même.

À k = 3, sur 6 072 couples : max D_v/r = **1,093**, sous 4/k ≈ 1,333 ; aucun dépassement.

La première version du script rappelait `diametres2_composantes` à chaque niveau ; elle était trop lente à k = 3 et
a été interrompue (`budget_oracle_v1_interrompu.log`). La version incrémentale redonne exactement son résultat à
k = 2 (4 119 couples, 1,8762).

Ces sous-ensembles LiDAR ne contiennent aucune adjacence cosphérique (§ 2.2) ; la position générale y tient donc
empiriquement à l'ordre 1. Les jeux synthétiques réguliers en ont : nappes, piéton, vélo occulté. Là, seule la borne
exacte min(j, m − j) diam(U)/k de l'auditeur s'applique, et 4r/k peut être dépassé. **[C]** Ce n'est pas mesuré
ici.

## 3. Contrôles transversaux demandés

| contrôle | rendu | réduction | robustesse | échelle |
| --- | --- | --- | --- | --- |
| prédictions avant mesures | oui | oui | oui | oui |
| complétude contrôlée (stricte) | 32/32 (après correction d'un dépassement int64 de SON contrôle) | oui, toutes, 8k–32k compris | 460/460 | 417/417 |
| attribution par labels | nœuds mesurés : oui ; π0 global : comptes seulement | oui (clés, sommets) | oui (IoU des couvertures) | oui (naissances du sous-arbre) |
| K = 1 = alpha | oui (polyédral ; 3 jeux à cellules cosphériques) | oui (1 ulp) | oui (3e-14) | oui (1 ulp) |
| certificats d'effondrement vérifiés indépendamment | sans objet | même module ; rejoué par moi (2 juges) : oui | sans objet | même module, dates de la construction, nœud par nœud |
| budget par composante | d_H(A, C) par ε-net non certifié | D_v exact, d_H échantillonné | Hausdorff échantillonné | θ = D_v/r exact |
| échelle aux bonnes tailles | non (52–1 139) | k ≤ 2 : oui ; k ≥ 3 : non | non (52–649) | oui pour la tour et les nœuds ; K = 5 entier : non |
| rien réglé sur la découpe réelle | méthodes : oui ; nœuds : oracle | idem | idem | idem |

## 4. Reconnaissabilité, candidat par candidat [J]

J'ai regardé les planches (même caméra par jeu). Mon jugement n'est pas aveugle et les nœuds sont choisis par l'oracle.

| objet | dual A_K (nœud oracle) | ancêtres intermédiaires | ombre | offset | supports S* | L réduit | verdict |
| --- | --- | --- | --- | --- | --- | --- | --- |
| six points (§ 6.1, 2D, K = 2) | exact, 7 → 3 → 1 | sans objet | ABC, CD, DEF se touchent en C et D sans fusion | les offsets de composantes distinctes se chevauchent | sans objet | 7 → 5 cellules | fixture fidèle : l'ombre et l'offset ne sont jamais des identités |
| anneau (5 et 10 m) | anneau ouvert, K = 1 à 5 | sans objet | anneau | tore épais, ouverture gardée | anneau | anneau | reconnu par tous ; cas facile |
| roue (dans le vélo, K = 5) | anneau à son nœud (r = 46,6 mm) | sans objet | anneau empâté | sans objet | sans objet | sans objet | reconnue, mais le nœud vit 0,2 mm |
| vélo synthétique (5 et 10 m) | au nœud objet : silhouette, **roues pleines** | **r = 77 à 89 mm : roues en anneaux, cadre, selle, guidon : clairement un vélo** | silhouette empâtée, roues pleines à K = 5 | deux cercles : non | cadre lisible, roues pleines | filet trompeur | reconnu **le long de la chaîne**, pas au nœud objet |
| piéton (5 m) | silhouette en marche, K = 1 à 5 | sans objet | silhouette | bloc : non | silhouette | sans objet | reconnu (profil plan favorable) |
| deux nappes (10 m) | séparées jusqu'à K = 3 (IoU 1), en fil de fer ou lignes à K = 1–2 ; **K = 5 : avant IoU 0,58 (1 111 sites), arrière IoU 0,43 (1 139 sites, le nuage entier)** | sans objet | fusion à K = 5 | un seul bloc | fusion | sans objet | **échec à K = 5 : aucune nappe séparée** |
| vélo réel (08/002852) | bloc irrégulier, IoU 0,81 à 0,88 | sans objet | bloc | bloc | bloc | sans objet | **non reconnaissable, même à K = 1** |

Ce que je retiens :

1. Le meilleur rendu est le **dual A_K à un rayon intermédiaire de la chaîne**. Ce n'est pas un nœud « objet ».
2. Les nœuds qui donnent ces images ont des vies inférieures au bruit de quantification : 0,1 mm (nœud 3579,
   77,4 → 77,5 mm) et 0,2 mm (nœud 3129).
3. Les planches de robustesse le confirment. Sous σ = 10 mm, la composante appariée par les labels a une IoU de 1 et
   l'image ne change pas, mais ce n'est pas « le même nœud ».
4. Un jeton de forme robuste doit donc être indexé par (chaîne, rayon), avec marges, et non par l'identité d'un nœud
   de vie courte. **[C]**

## 5. Auditeur

Le dépôt a été relu à 00 h 43 puis à 01 h 21 UTC. Aucun commit « audit » après 28d70f8ab. Après ce commit, les seules
modifications sous `morsehgp3D_v11/audits/` sont les notes « section X » et « section Y » du développeur (6f37e4e54 et
c0f05d649, vitesse de la tour, sans rapport). Aucun nouveau `receipts/audit_*` non plus. Réponse de l'auditeur :
**aucune**.

Confrontation avec la liste de 28d70f8ab :

| demande de l'auditeur | état |
| --- | --- |
| oracle exact et tests des labels | fait |
| réduction à sommets protégés, journal, vérificateur global indépendant | fait ; même module ; rejoué indépendamment ici |
| diamètres certifiés par composante | fait (D_v exact) ; d_H seulement échantillonné |
| deux rendus nommés (dual réduit, ombre de couverture) | faits |
| épreuve 1 : positions | faite |
| épreuve 2 : population | faite |
| épreuve 3 : échantillonnage avec contrat de masse | **non faite** : décimation sans poids et heuristique k' = arrondi(kρ) |
| certificat couvrant toute la plage de filtration | respecté dans la réduction ; nœud par nœud dans l'échelle, invalidé sur la vie du parent (47/57), comme l'auditeur l'avait prédit |
| piste « Sparse Multicover » (Alonso, SoCG 2025) | non explorée expérimentalement |

Les avertissements de l'auditeur sont tous confirmés par les mesures :

- le dessin n'est pas stable à rayon fixé : 44 nœuds sur 141 sautent, et le triangle donne H/δ = 22,5 ;
- l'ombre n'est pas homotopique ;
- moins de k aberrants peuvent créer des composantes ;
- les réductions nœud par nœud ne suffisent pas.

## 6. Manques

1. **Pas de coût mesuré** d'un constructeur local depuis FULL. Tout est en Python, en aval. L'objectif de 100 ms n'est
   jugé que par extrapolation (hypothèse H).
2. **K = 5 sur un nuage entier** de 8 000, 16 000 ou 32 000 sites : non mesuré. Les 3-cellules d'ordre 2 ne sont pas
   recomptées par un outil indépendant.
3. **Certificats de réduction à k = 2, 16 000 et 32 000 sites** : dates seulement échantillonnées.
4. **Robustesse sur LiDAR réel** non faite : quantification avec fusions et modèle pondéré des multiplicités. Le
   contrat de masse non plus. H1 et la DTM ne sont pas rejoués par moi.
5. **Identité π0 globale avec FULL dans le rendu** : seulement des comptes.
6. **Reconnaissabilité** sans protocole aveugle ni sélection non oracle : aucun nœud n'est choisi sans la vérité
   terrain.
7. **README de la robustesse** absent.
8. **Jeton de forme robuste** indexé par (chaîne, rayon, marges) : proposé ici [C], non mesuré.

## 7. Fichiers

| fichier | contenu |
| --- | --- |
| `rejeu_oracle.py`, `rejeu_oracle.json`, `rejeu_oracle.log` | passe 2 : rendu 228/228, robustesse, K = 1 |
| `rejeu_oracle_passe1.json`, `rejeu_oracle_passe1.log` | passe 1 : réduction k = 2 et 3, mutants, rendu 210/228 (ma faute de niveaux), bruit |
| `vies_oracle.py`, `vies_oracle.json` | arbre de fusion recalculé sur l'oracle contre `vies.json` |
| `budget_oracle.py`, `budget_oracle.json`, `budget_oracle.log` | D_v/r sur toutes les composantes à tous les niveaux (k = 2 : 1,876 ; k = 3 : 1,093) |
| `recompte_k2_8000.py`, `recompte_k2_{8000,16000,32000}.json` | arêtes et tétraèdres de Delaunay (qhull + test exact) contre les mosaïques d'échelle |
