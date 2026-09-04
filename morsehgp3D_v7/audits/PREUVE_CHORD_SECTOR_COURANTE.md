# Prunes par corde et secteurs — preuve locale courante

Lecture indépendante du 4 septembre 2026. Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Aucun test supplémentaire exécuté, aucune source produit modifiée. GCP non utilisé.

**Conclusion : les deux prunes sont suffisants sur leur domaine d'appel.** Pour chaque boule admissible possédée par l'ancre, le verdict fournit au moins $h_q=s_{\max}-q+1$ sites distincts strictement intérieurs ; il implique donc $p+q>s_{\max}$. L'exclusion du futur sommet de support découle de la stricte négativité au centre réel. Il ne faut ni soustraire encore l'arité aux compteurs ni additionner deux certificats qui peuvent employer les mêmes sites.

## 1. Domaine et comptage

Le domaine est une boule de support minimal $S$ d'arité $q\in\{2,3,4\}$, dont $AB$ est une arête maximale, avec points distincts et coordonnées u16. L'existence d'une graine canonique aiguë en q4 et son accès par les covers font l'objet de la revue mathématique distincte ; la présente preuve s'applique à toute telle graine effectivement considérée.

Le seuil est fixé dans `src/spindle/spindle.hpp:46` et transmis sans crédit retranché aux tests d'ancre et de corde (`src/pipeline/generate.hpp:1236`, `1353`, `1378`, `1381`). En q2, ni secteurs ni corde ne sont appelés : la branche émet la boule diamétrale après les certificats de rectangle et d'extrémités (`generate.hpp:1342`). Les secteurs concernent q3/q4 ; la corde concerne q4 seulement.

Les positions dupliquées sont refusées avant génération et la taille est plafonnée à $2^{30}-1$ (`src/pipeline/run.hpp:415`, `438`). Les requêtes d'arbre visitent chaque feuille au plus une fois ; les handles sont une antichaîne et leurs plages de feuilles sont disjointes (`src/lanes/edge_cover.hpp:90`, `141`, `183`, `227`). Ainsi, les incréments par site désignent des identités distinctes et les compteurs u32 ne débordent pas dans le chemin public. Un appel direct avec un vecteur `CoverPoint` contenant plusieurs fois le même `u` ne satisfait pas cette prémisse.

Pour une boule donnée, tout point du support est sur sa sphère et sa puissance vaut zéro. Un certificat strict valable au centre de cette boule exclut donc automatiquement tous ses points de support, même s'ils figurent dans la liste scannée. Les exclusions explicites de $a,b$ dans les secteurs (`sector_kill.hpp:165`) et de $a,b,x$ dans la corde (`generate.hpp:944`) sont cohérentes avec cette propriété. Le quatrième point $y$, encore inconnu, ne peut pas être un témoin strict sur le morceau contenant son propre centre.

## 2. Secteurs : un minorant pour chaque centre réel

Posons $m=(a+b)/2$, $D^2=\lVert b-a\rVert^2$ et $c=m+v$. La boule passe par $a,b$, donc $v\perp(b-a)$ et $R^2=D^2/4+\lVert v\rVert^2$. Pour q3, l'angle opposé à l'arête maximale d'un triangle aigu appartient à $[\pi/3,\pi/2)$ : $R^2\leq D^2/3$, donc $\lVert v\rVert^2\leq D^2/12$. Pour q4, le support bien centré détermine sa boule minimale ; Jung donne $R^2\leq3D^2/8$, donc $\lVert v\rVert^2\leq D^2/8$.

`bisector_basis` n'accepte la base que si les distances de l'origine aux quatre arêtes du losange $\mathrm{conv}(\pm u,\pm v)$ sont au moins le rayon du disque des centres (`sector_kill.hpp:65–88`). L'octogone choisi contient ce losange ; les huit triangles de sommets $(0,P_k,P_{k+1})$ le recouvrent (`153–158`). Un refus de construction de la base conserve l'ancre. Avec au plus 128 itérations et des différences u16, les produits de cette construction restent dans leurs types entiers.

Pour $w_2=2z-a-b$, l'intériorité stricte est exactement $4w_2\cdot v>\lVert w_2\rVert^2-D^2$. Les tests aux trois sommets d'un triangle (`sector_kill.hpp:167–179`) rendent cette inégalité stricte en tout point du triangle, par affinité. Pour le secteur contenant le centre réel, chaque site compté est donc strictement intérieur à la boule. Le minimum des huit comptes est un minorant uniforme de sa profondeur (`181–185`). Le même site peut témoigner dans plusieurs secteurs : aucun compte n'est additionné entre secteurs.

Le crédit `base=h_a(a)+h_b(b)` correspond à deux ensembles distincts de sites dans $A\setminus\{a\}$ et $B\setminus\{b\}$ (`generate.hpp:488–507`, `1350`). Pour une extrémité fixée et un site fixé, l'appartenance à $W_q$ s'écrit $H>\sqrt{\Xi/c_q}$, avec $c_3=3$, $c_4=2$ ; c'est un domaine convexe dans l'extrémité opposée. La vérification stricte aux coins de sa boîte suffit donc pour toute la boîte. Ces sites sont universels sur le disque des centres admissibles.

Pour un centre réel appartenant au secteur $k$, les deux nombres `cnt[k]` et `cnt_out[k]+base` sont des minorants de profondeur. Le second additionne des sites hors de $A\cup B$ à des sites dans $A\cup B$ ; il n'y a pas de double comptage. Ainsi $\min_k\max(\mathrm{cnt}_k,\mathrm{cnt}_{\mathrm{out},k}+\mathrm{base})$ est sûr (`sector_kill.hpp:176–198`). Le crédit n'a pas besoin d'être universel sur les sommets artificiels de l'octogone, situés hors du disque.

Les éliminations radiales n'ajoutent aucun témoin. Leur sûreté pour le prune ne dépend donc pas d'un cover exhaustif ; omettre un témoin affaiblit uniquement ce certificat. Pour l'équivalence des deux routes d'ancre, les témoins sectoriels sont dans la boule diamétrale ouverte. Les candidats de rectangle la contiennent, avec `dist2q` égal à une distance minorante à la boîte des sommes ; `radially_sorted=false` supprime le `break`. Dans la route cover, le tri utilise des classes de coefficient 3 ou 4. En q4, les intérieurs diamétraux sont tous dans les classes 0 à 7 ; la coupure utilisant le coefficient 3 ne peut commencer que dans une classe ultérieure. Elle ne perd donc pas de témoin diamétral.

## 3. Corde q4 : couvrir tous les paramètres admissibles

Pour une graine aiguë $(a,b,x)$, posons $G=\lVert(b-a)\times(x-a)\rVert^2>0$, $n=(b-a)\times(x-a)$, $E=\lVert x-a\rVert^2$ et $X=\lVert x-b\rVert^2$. Le centre de sa circumboule est $c_3$ et $R_3^2=D^2EX/(4G)$. Tout centre de boule passant par ces trois points s'écrit $c_\mu=c_3+\mu n/(2G)$, avec $R_\mu^2=R_3^2+\mu^2/(4G)$. Jung impose donc $\mu^2\leq J/2$, où $J=D^2(3G-2EX)$ ; l'acuité et l'arête maximale donnent même $J\geq GD^2/3>0$ (`generate.hpp:922–930`).

L'identité de puissance est $G(\lVert z-c_\mu\rVert^2-R_\mu^2)=P(z)-\mu B(z)$, avec $B(z)=n\cdot(z-a)$ et $P(z)=L(z)/4$. Le code forme cette même valeur affine (`generate.hpp:750–770`, `946–954`). La divisibilité de $L$ par quatre est une identité algébrique, pas un arrondi géométrique.

`ChordPieces::init` calcule $\widehat\mu=\lfloor\sqrt{\lfloor J/2\rfloor}\rfloor+1>\sqrt{J/2}$. Les quatre intervalles fermés de sommets $\mu_j=(2j-4)\widehat\mu/4$, $j=0,\ldots,4$, recouvrent la corde admissible. La correction de la racine carrée par carrés entiers rend la valeur exacte malgré la proposition initiale flottante (`chord_kill.hpp:41–58`).

Un site est compté sur l'intervalle $i$ lorsque $L-(2i-4)\widehat\mu B<0$ et $L-(2i-2)\widehat\mu B<0$. Une fonction affine strictement négative aux deux extrémités l'est sur tout l'intervalle fermé (`chord_kill.hpp:72–88`). Au paramètre réel $\mu_y$ d'une completion admissible, le compte de son intervalle est donc un minorant d'intérieurs distincts. `dead(h4)` exige ce seuil sur les quatre intervalles (`90–93`). L'égalité de puissance au point $y$ empêche précisément de compter $y$ dans cet intervalle.

La branche d'appel enregistre aussi les sites de $P>0$ dans la corde avant le saut réservé au cœur universel, et vérifie le verdict même sur cette branche (`generate.hpp:951–963`). Les comptes de cœur et de corde sont réunis par un OU (`998`), jamais additionnés. Leurs témoins peuvent donc se recouvrir sans risque.

## 4. Prémisse arithmétique et précisions documentaires

Les tests sectoriels sont entiers stricts. La corde utilise un filtre flottant, puis `L-c*mu_hat*B` exact en i128 dans les cas indécidables (`chord_kill.hpp:62–84`). Son contrat arithmétique est celui de `src/pipeline/float_filter.hpp` : binaire64, arrondi au plus proche, séquence sans réassociation et borne affine conservatrice ; `generate.hpp:759` transmet une borne infinie quand le filtre est désactivé. Sous le profil u16, les magnitudes déclarées de $L$, $\widehat\mu$ et $B$ laissent la comparaison exacte sous $2^{110}$. La garde relative de produit $2^{-40}$ couvre les conversions et produits binaire64, avec une marge supérieure aux arrondis concernés. La présente lecture ne constitue pas une qualification de compilateurs ni de GPU.

Trois formulations peuvent être précisées sans modifier le verdict :

- `chord_kill.hpp:15` : remplacer « minimum » par « maximum », ou simplement « négativité aux deux extrémités ». C'est bien le maximum affine qui doit être strictement négatif.
- `sector_kill.hpp:136–144` : les témoins du crédit sont universels sur les **centres admissibles** de chaque secteur. Leur validité sur tout l'octogone n'est ni prouvée ni nécessaire.
- `sector_kill.hpp:224–228` : la distance de boîte n'est pas recalculée dans cette fonction ; elle reste un minorant sûr pour le `continue`, tandis que les tests géométriques utilisent les coordonnées exactes. Seule la sortie `break` est désactivée.

## Sources épinglées

Hashes SHA-256 inchangés entre les lectures de contrôle. Les numéros de ligne se rapportent à cet état, pas à une future modification du constructeur.

| Source sous `morsehgp3D_v7/` | SHA-256 |
| --- | --- |
| `src/lanes/chord_kill.hpp` | `f5a4ca9b6e2fbc0ea94e8adf92f64322ab2ef5c461c588efcedf1bc9e4faa039` |
| `src/lanes/sector_kill.hpp` | `4742ccccbcfdec847ea2e6bdded9f73b78d62e32585058b09d4b45b5951b7e06` |
| `src/pipeline/generate.hpp` | `ee2a4a1f96875c7db1fbd054700a22db6eabb8f62379c71c0ed6728f1b18de59` |
| `src/lanes/edge_cover.hpp` | `3186e5b67002a2d29a50e090bc465c86690224f78eab30088c5f33595d2e4836` |
| `src/pipeline/float_filter.hpp` | `52e07b4e4dfb1ca66e2a6218fc9c31afc9586e0a81ca5a4a846ed7727e44b7d9` |
| `src/spindle/spindle.hpp` | `f64af67ff1b6aa7a70f10e410bd53841600b44722b7d035fe86b51c25b808577` |
| `src/lanes/q3.hpp` | `4155a1c39193b68c47504e247a36e1bbf28b2c9ecbeeb50d6285d974519563fe` |
| `src/pipeline/run.hpp` | `885348a92f48658642e3783027cb7c4f239f1c8e1a0b91c66a698f3be6b29762` |
