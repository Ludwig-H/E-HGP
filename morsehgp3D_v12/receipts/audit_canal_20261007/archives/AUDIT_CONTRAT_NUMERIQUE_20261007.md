# Relecture du contrat numérique en repère local (B ≤ 32)

7 octobre 2026. Claude, même rôle que pour la [contre-lecture des lemmes T](https://github.com/Ludwig-H/E-HGP/blob/2a7a5f34622c6a09198e241e6d137b281c8e62be/morsehgp3D_v12/audits/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md).
Ces deux notes sont les miennes ; l'audit d'ouverture et le reçu `13c52bc60` sont de l'autre auditeur (Codex).

- **Jugé** : [`CONTRAT_NUMERIQUE.md`](https://github.com/Ludwig-H/E-HGP/blob/2a7a5f34622c6a09198e241e6d137b281c8e62be/morsehgp3D_v12/docs/CONTRAT_NUMERIQUE.md) (blob `9b6a17bd61d1`, `e264de6f2`).
- **Confronté** au code de la v11 sur `fc1f913ce` : `src/num`, `src/index`, `src/catalogue`, `src/tower`,
  `src/api` et `bench/full_semantic.py`, lus sans exécution.
- **Constats** au [registre](https://github.com/Ludwig-H/E-HGP/blob/2a7a5f34622c6a09198e241e6d137b281c8e62be/morsehgp3D_v12/audits/CONSTATS.md), de `CST-0108` à `CST-0113`.

```text
phase=exploration_v12_hors_registre
public_status=not_claimed
GCP non utilisé
```

`NUM-REPERE` et la preuve de `NUM-GARDE` sont justes. Avec $c \in \mathrm{conv}(S)$, on a
$R \leq \mathrm{diam}(S) < \sqrt{3} \cdot 2^{s}$ et le pavé $(a_i - 2^{s+1}, a_i + 3 \cdot 2^{s})$. Pour un site qui
passe la garde, les écarts au support sont inférieurs à $3 \cdot 2^{s}$, et la projection d'une boîte qui touche le
pavé reste dans le pavé. Les réserves sont toutes du côté du **domaine** de la garde et de ce que la v11 fait hors
d'elle.

## Question 1 — domaine de `NUM-GARDE`

- **Boules certifiées : oui.** C'est le cas des boules du catalogue et des plus petites boules certifiées par les
  signes barycentriques exacts. Les coquilles étendues de `LEM-T7` en font partie : $c \in \mathrm{conv}(S^{*})$, et
  tous les sites de $U$ sont sur la sphère, donc dans le pavé.
  - Les prédicats du quotient (`LEM-T7`) et du support canonique portent sur des sites de $U$ hors du support. Selon
    la règle du contrat (budget du repère $s+2$), l'orientation avec centre vaut $7s+23$ (natif `i128` jusqu'à
    $s=14$, pas 16), et le côté d'un site gardé $6s+20$ (natif jusqu'à $s=17$).
  - Ce sont des majorants grossiers : un budget mixte, avec $N$ et $D$ au repère $s$ et les sites à $s+2$, ferait
    mieux. La table du § 3 doit le dire (`CST-0111`).
- **Proposition flottante non certifiée : non** (`CST-0108`). Le centre circonscrit d'un triangle obtus est hors du
  triangle, et d'un triangle presque aligné, arbitrairement loin.
  - Règle à écrire : aucun site ni aucune boîte extérieurs ne sont confrontés à une boule avant le certificat exact
    de son support.
  - `resolve1` respecte déjà cet ordre (recensement après `meb_exacte`). Le faire tenir par le type (une boule
    certifiée ne sort que du catalogue ou du certificat) et par un mutant.
- **Recensement de la v11 hors garde** (`CST-0109`). `LatticeSphere`, construit à chaque parcours sur le chemin chaud
  (`src/index/census.cpp:45`), fait deux choses que la garde ne couvre pas :
  - il calcule le centre **absolu** $a_j D + N_j$ ($5B+6$ bits, `static_assert` à 126, soit $B \leq 24$) ;
  - il évalue la puissance au **coin lointain** des boîtes (`bound_signs`), n'importe où dans le domaine.

  Correctifs : prendre le point entier le plus proche en local (plancher de $N_j / D$, puis $+a_j$) ; et une boîte
  non contenue dans le pavé ne peut pas être contenue dans la boule, son signe majorant est donc positif sans
  arithmétique. Le coin lointain n'est alors évalué que dans le pavé, au budget de $s+2$.
- **Requêtes à centre entier** (`core`, $k$ plus proches sur l'index, `CST-0110`). Il n'y a aucune boule avant d'avoir
  $k$ candidats. Les distances carrées font $2B+2$ bits sur l'étendue globale ; `u64` suffit jusqu'à $B=31$, pas à
  $B=32$ (la conception d'origine les donne en 64 bits). Il faut `i128`, ou un repère ancré au site interrogé avec
  élagage par rayon courant. Le contrat n'en parle pas.

## Question 2 — étendue de la liste d'une feuille

- **Correction : aucun plafond.** La voie large est exacte. Un refus par étendue serait une incomplétude sur des
  entrées légitimes, puisque l'étendue d'une liste vient des $K$ plus proches d'un centre de la boîte.
- **Coût** : un seul site lointain fait passer toute la feuille en voie contrôlée ou large, puisque la voie est
  uniforme par repère.
  - Mesurer d'abord (`MES-S`) la part des feuilles dont l'étendue vient d'une minorité de sites.
  - Remèdes, dans l'ordre : continuer la subdivision, puisque l'étendue de l'enfant est au plus celle du parent ;
    sinon choisir la voie par prédicat, d'après des décalages par site précalculés une fois par feuille.
- **Précision à `NUM-COUVERTURE`** (`CST-0112`). Le filtrage G1 d'un enfant lit les témoins et les candidats dans la
  liste du **parent** (`src/catalogue/boxes.cpp`, `filter` et `reservoir`), qui n'est pas incluse dans celle de
  l'enfant. Ce filtrage s'évalue donc dans le repère du parent, qui contient celui de l'enfant.

## Question 3 — la clé de Morton décide-t-elle quelque chose ?

Oui, deux choses publiées (`CST-0113`).

1. **Le support canonique.** $S^{*}$ est le support de cardinal minimal, puis premier dans l'ordre lexicographique
   des `SiteIdx` (`src/catalogue/support.cpp`), et un `SiteIdx` est un rang de Morton (`src/cloud/cloud.hpp`). Pour
   une coquille à plusieurs supports minimaux (le carré et ses deux diagonales), $S^{*}$ dépend donc de la clé.
   - Il ordonne le catalogue, (niveau, $S^{*}$) = ordre des `BallIdx`.
   - Il ordonne les lignes de la sortie `supports` (`src/api/write_supports.cpp`, l. 14), qui publie aussi $S^{*}$.
   - Il fixe la convention `cover_v10`.
2. **L'ordre des sections de sites.** Les lignes des sections SITES des sorties FULL, `points` et `supports` sont des
   `SiteIdx`. Le lecteur strict exige et hache l'ordre de Morton **absolu** sur `bits` bits
   (`bench/full_semantic.py`, l. 110 à 113).
   - Une clé prise sur (coordonnées − minimum) change cet ordre même à $B_{\mathrm{eff}} \leq 21$, car l'ordre de
     Morton n'est pas invariant par translation.
   - La porte d'invariance par translation du § 7 ne peut pas se juger avec ce lecteur tel quel : il hache des
     coordonnées et des centres absolus, et l'ordre de Morton absolu des sites. Elle doit comparer à translation
     près, avec les sites rangés par une clé invariante (coordonnées lexicographiques ou `PointId`). L'ordre des
     naissances par (niveau, centre) est, lui, invariant par translation.
   - Deux issues : écrire les sites dans l'ordre absolu de la v11 (tri à part, à l'export) et redéfinir $S^{*}$ par
     les coordonnées ; ou versionner le schéma (sites en ordre lexicographique des coordonnées) et comparer la v11 et
     la v12 par un lecteur qui retrie.

Ailleurs, le rang de Morton ne départage que des choix sans effet sur l'objet : ex æquo des plus proches,
représentants, propositions (théorème D). Les compteurs publiés changeront donc, pas les forêts.

## Question 4 — centres absolus

Dans le moteur, `compare_centers` n'est appelé que par `src/tower/forest_build.cpp`, l. 93 : tri des naissances de
même rang. Le lecteur strict ordonne de même les naissances par (niveau, centre). Les autres usages du centre absolu
sont l'export des centres des naissances et `LatticeSphere` (question 1). Aucun autre départage ne compare des centres.

## Non établi

Aucun code exécuté. Les seuils de la table du § 3 ont été relus sur leurs formules, pas recalculés un à un. Les
certificats de fabrique de la v11 n'ont pas été relus.
