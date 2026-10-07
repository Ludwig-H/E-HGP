# Contre-lecture des lemmes T1, T3 à T7 de la conception de la tour

7 octobre 2026. Claude, à la demande du développeur v12 ; rôle : auditeur de contre-lecture, ponctuel (la même
session tient par ailleurs les démos `Zoltan/demos`). Jugé :
[`CONCEPTION_TOUR.md`](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md)
(blob `8e7432dcc680`), annexe A et § 2.2, 3.2, 3.4, 4.1 à 4.3, 5, lu sur `13c52bc60`. Fondations relues :
[L02](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/morsehgp3D_v11/receipts/conception_v11_20261002/audit_v10/L02_MATH_TOUR.md) § 4 (lemmes 1 à 4, théorèmes
B à F et J ; blob `2edac5240eb3`) et [`MATHEMATIQUES.md`](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/morsehgp3D_v11/docs/MATHEMATIQUES.md) de la v11 (M1, M2,
T2 à T6, lemmes A, C, D, P, témoins D2 et mémo ; blob `fc76c5840a01`). Aucun code du moteur exécuté ; deux témoins
vérifiés en arithmétique exacte. Constats au [registre](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/morsehgp3D_v12/audits/CONSTATS.md), bloc `CST-0101` à `CST-0199`.

```text
phase=exploration_v12_hors_registre
public_status=not_claimed
GCP non utilisé
```

| Énoncé | Verdict | Réserve | Constats |
| --- | --- | --- | --- |
| `LEM-T1` | preuve juste, pour tout support $S$ de $b$ | le test $S \subseteq F$ manque au pseudo-code (§ 3.2) et à la règle de décision (§ 3.4) | `CST-0101` |
| `LEM-T3` | juste | compatible avec `WIT-D2` et `WIT-MEMO`, à graver dans sa porte | `CST-0104` |
| `LEM-T4` | juste, sans dépendre du théorème J | à énoncer comme le lemme P.3 ; `REG:243` caduque plutôt que prouvée ; clé de numérotation | `CST-0102`, `CST-0103`, `CST-0107` |
| `LEM-T5` | juste | hypothèse $\mathrm{rang}(\ell) \leq r$ absente de (iii) | `CST-0105` |
| `LEM-T6` | juste | — | — |
| `LEM-T7` | juste | la famille contient les séparables maximaux sans s'y réduire ; (iv) majore l'apport | `CST-0106` |

## T1 — certificat combinatoire

1. La preuve A.1 vaut pour tout support $S$ de $b$, pas seulement $S^{*}$ : $S \subseteq F \subseteq P_b$ entraîne
   $B(F) = b$ (lemme 1 de L02, M2 de la v11).
2. **Domaine.** La justesse n'exige que la moitié « aucun faux site » de (H2) pour l'enregistrement lu : $I \cup U$
   sans site hors de la boule fermée, $S^{*}$ vrai support. La complétude du catalogue n'y entre pas. Une boule hors de
   $\mathrm{Cat}_K$ (20 à 23 % des plus petites boules des descentes) ne peut donner aucun faux positif : un support
   détermine sa boule ($B(S) = b$ pour tout support $S$ de $b$), donc une clé $S$ égale au $S^{*}$ d'une boule $b'$ du
   catalogue force $b' = B(S)$. Si $B(F)$ est absente, la recherche ou une inclusion échoue, et le chemin exact reprend.
3. **« $S = S^{*}(b)$ et $F \subseteq P_b$ » ne suffit pas** : il faut aussi $S \subseteq F$. Témoin exact : carré
   $A=(0,0,0)$, $B=(2,0,0)$, $C=(2,2,0)$, $D=(0,2,0)$. La boule $b$ du cercle circonscrit a $c=(1,1,0)$, $\lambda=2$,
   $I=\emptyset$, $U=\lbrace A,B,C,D\rbrace$, $q=2$, $S^{*}=\lbrace A,C\rbrace$ ; $F=\lbrace A,B\rbrace \subseteq P_b$
   mais $\beta(F)=1<2$. Une proposition qui rend $S^{*}(b)$ hors de $F$ certifierait la mauvaise boule ; la marche
   depuis la sphère précédente (§ 3.4, seconde candidate) peut le faire. M2 de la v11 exigeait déjà les deux
   inclusions ; le § 3.2 n'écrit que $F \subseteq P_b$, et le § 3.4 parle de « deux égalités d'entiers ». Coût du test :
   au plus quatre identifiants. Mutant : test retiré, proposition forcée à $S^{*}(b)$.
4. **Coquille étendue, support non canonique.** La clé ne trouve rien ; le chemin exact recense la sphère, calcule
   $S^{*}$ et retrouve la boule. Aucun faux positif, seulement du travail (0,02 à 0,04 % des boules). Fixture : le
   même carré, $F=\lbrace B,D\rbrace$, proposition $\lbrace B,D\rbrace$ ; attendus : recherche en échec, chemin exact,
   arrêt sur la jonction à quatre morceaux $(b,2)$.

## T3 — arrêt sur la première cellule, pointeurs

Juste. Le pas $F_s \to r'$, avec $r' = I_{b'} \cup A_1$ et $A_1$ séparable, est un pas valide du théorème D (en
fenêtre, $p_{b'} < k$) : toute la chaîne en est une. Et $\lambda_{b'} = \beta(F_s) \leq \beta(F_0) < \lambda$ donne
le rang strictement inférieur.

- `WIT-D2` : la trace $AB$ de $ABC$ naît au niveau 64, après $\ell(r_b-1)=41$. Sa boule ($p=2 \geq k$) est hors de
  $\mathrm{Cat}_2$, son recensement est saturé, et le saut mène à la naissance $ZW$. T3 n'impose pas
  $\beta(F_0) \leq \ell(r_b-1)$, et aucune garde ne doit l'ajouter, ni la garde gratuite par rangs ni une porte.
- `WIT-MEMO` ($\lbrace 0,2,4,6\rbrace$, $k=2$) : un pointeur de cellule vaut à partir de $\lambda_{b'}$, date
  initiale de $F_s$, et la cible finale à partir de $\beta(F_0)$, jamais depuis la date terminale. La paire
  $\lbrace 0,6\rbrace$ saute vers $\lbrace 2,4\rbrace$, née au niveau 1, mais elle n'est un sommet qu'au niveau 9. La
  « date d'usage » de A.3 le dit déjà.

## T4 — noyau sans lots

1. La preuve A.4 est une récurrence autonome qui n'utilise pas le théorème J. « Par le théorème J » (PO-T4, § 4.2)
   désigne une formulation parallèle (contiguïté par concaténation), pas une prémisse.
2. **Égalité avec le lemme P** (v11 § 10.3). Elle tient sous trois ponts, à écrire dans l'énoncé de `LEM-T4` :
   - (a) (H2) et les cibles de `LEM-T3` : après le rang $t-1$, les classes de naissances sont les composantes strictes
     du niveau $\ell(t)$ (théorème E de L02, T6 de la v11).
   - (b) (H4) : un représentant par morceau atteint chaque composante que la cellule touche (lemme C.1 de la v11,
     lemme 4 (c) de L02).
   - (c) La liste des jonctions de l'ordre $k$ est celle des cellules de $W_k$ : jonctions régulières, **boules
     faibles comprises** ($k=p+q-1$), et cellules étendues de fenêtre non naissance, **inertes comprises** (un
     représentant). Les boules hors de $W_k$ n'y figurent pas et n'ont pas à y figurer (lemme P.2).

   Alors les classes d'événements liés de rang $t$ sont les composantes de $H_{\ell(t)}$ qui contiennent au moins deux
   composantes strictes, et leurs opérandes de rang inférieur ou feuilles en sont les enfants. **Continuation sans
   nœud** (une seule composante touchée, ou déjà réunie plus tôt dans le rang) : aucun événement. Cellule
   « passagère » : aucun événement, et son `jtop` relève de `LEM-T6`.
3. **`REG:243`.** La ligne vient de la [spécification d'origine](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/docs/SPECIFICATION_MORSEHGP3D.md) (l. 729 :
   « graphe de successeurs multivalué traité par composantes fortement connexes »). `LEM-T4` ne prouve pas ce quotient,
   il le rend inutile :
   - les descentes ne restent jamais sur un plateau (théorème D, `OBJ-T2`) ;
   - l'atomicité du plateau est le lemme P ;
   - la contraction du noyau est `LEM-T4` ;
   - les coquilles étendues relèvent de `LEM-T7`.

   La clore comme **caduque, remplacée par** ces énoncés, avec cette portée, et non comme prouvée. Dans un rang, la
   relation « liés » est une forêt orientée du producteur vers le consommateur. Ses composantes fortement connexes sont
   donc des singletons : c'est de composantes connexes qu'il s'agit.
4. **Clé de numérotation.** La clé (rang, plus petite naissance) doit lire la numérotation canonique de la v12
   (naissances par niveau puis centre exact, [contrat](https://github.com/Ludwig-H/E-HGP/blob/fc1f913ce325b04922b897137a9507153b129894/morsehgp3D_v12/docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md) § 1). L'ordre du
   catalogue (niveau, $S^{*}$) de la conception du 2 octobre (§ 2.1, § 4.2) donnerait d'autres empreintes.

## T5, T6 — historique d'attache, image d'une naissance

- **T5** est juste, mais (iii) suppose $\mathrm{rang}(\ell) \leq r$ : sinon la fonction rend une naissance qui n'est
  pas encore vivante.
  - Les appelants du document respectent cette hypothèse (verticales : `rep_node` de rang inférieur ; `core` :
    naissance de niveau au plus $\beta(N_k(x)) \leq D_k(x)$).
  - Il faut l'écrire, et garder les listes d'événements par survivant dans l'ordre de traitement (tri par comptage
    stable).
- **T6** est juste, y compris dans le cas suggéré par l'auteur : une jonction qui n'unit rien, puis, au même rang, une
  jonction qui absorbe sa composante. Le premier événement postérieur a $w_0$ pour opérande, et sa classe est le
  parent de rang $r$.
  - Naissance étendue : la cellule $(b,k-1)$ est en fenêtre, puisque $t \geq q$. Si c'est une naissance, on prend
    son nœud ; sinon, `jtop` et T6.

## T7 — quotient local

Juste. La preuve tient par la séparation de Gordan, la lecture d'une cellule de l'arrangement depuis un sommet, et
les fenêtres semi-ouvertes. Les cas limites demandés :
- **Antipodes** : une paire $u$, $-u$ ne définit pas de plan (c'est le même grand cercle). La fenêtre semi-ouverte
  exclut l'antipode de son origine, et $m=2$ n'arrive qu'en paire antipodale, d'où la clause finale.
- **Coquille coplanaire au centre** : un seul plan.
- **Plans partageant une droite** : un plan par classe de paires non parallèles, et les marques `fait` ne perdent
  rien.
- **Sous la fenêtre** ($t \leq q-2$) : le quotient rendrait un seul morceau. `resolve1` ne l'appelle pas, ce qui est
  cohérent.

Deux précisions :
1. **La famille n'est pas réduite aux séparables maximaux.** Elle les contient tous, plus des ensembles non maximaux.
   Exemple : cercle $x^{2}+y^{2}=25$ du plan $z=0$, avec $E=(5,0,0)$, $G=(4,3,0)$, $H=(-3,4,0)$, $J=(-3,-4,0)$. La
   fenêtre issue de $G$ est $\lbrace G,H\rbrace$, contenue dans le maximal $\lbrace E,G,H\rbrace$. C'est sans effet sur
   (i) à (iv), mais tout usage qui suppose la maximalité serait faux.
2. **(iv) majore l'apport à la couverture.** Elle donne les sites de $U$ qui ne sont dans aucune $k$-partie stricte de
   $P_b$ ; un tel site peut déjà être couvert par un autre sommet de la composante. `ext_cover` est donc exact pour
   l'union, pas pour un compte de sites nouveaux.

## Ce qui n'est pas établi ici

- Aucun contrôle du 2 octobre n'a été rejoué.
- PO-T2, T8 et I1 n'ont pas été contre-lus.
- Les deux questions de fond de l'auteur restent ouvertes : la décroissance jugée par les seules portes, et le
  contrôle croisé de l'index.
