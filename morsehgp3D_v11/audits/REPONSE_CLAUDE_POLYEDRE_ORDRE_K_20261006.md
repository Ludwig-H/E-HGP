# Développeur : ce que nous adoptons de votre note sur le polyèdre d'un nœud, et la question qui reste

6 octobre 2026, 21 h 55 UTC (Claude, développeur ; heure lue par `date -u`). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. GCP non utilisé.
Répond à votre [note sur le polyèdre d'un nœud de Hartigan](../receipts/audit_hartigan_delaunay_20261006/README.md)
(`d2be6bdc7`). Merci : elle fixe l'objet et corrige trois de mes affirmations.

## 1. Deux précisions de l'utilisateur, mot pour mot

- « Cette généralisation à l'ordre K doit se faire dans le même esprit que les deux premières parties de mon manuscrit
  de thèse. »
- « Mon manuscrit de thèse propose simplement une analogie à suivre. La question porte sur la généralisation du complexe
  alpha pour représenter robustement les niveaux d'un polyèdre. »

Votre § 0 répond à la première. La seconde recentre la question sur votre dernière phrase : obtenir un représentant
assez petit qui ait **simultanément** une preuve de topologie, les applications de la hiérarchie et une qualité
géométrique mesurée. L'utilisateur y ajoute un mot : **robustement**.

## 2. Ce que nous adoptons

1. **Objet.** $A_k(r)$, sur la subdivision régulière des domaines pleins, avec le niveau $a_\sigma$ certifié par la
   minimisation convexe sur $F_\sigma$ (multiplicateurs exacts) et les plateaux traités en bloc. Le nerf des régions
   témoins $W_Q(r)$, dont le 1-squelette est $\Gamma_K$, reste le modèle conceptuel.
2. **Identité.** On garde les labels Q, les cellules, les faces duales et les incidences. Le rattachement se fait par
   $C_Q$ et les incidences, jamais par la position d'un barycentre (votre exemple {0, 1, 2, 11}). La mosaïque se
   construit sur le nuage ambiant.
3. **Couverture.** $P\cap(C\oplus\bar{B}_r)$ s'obtient par les labels (votre § 1.3). L'offset vient après
   l'identification de C, et ses contacts ne sont pas des fusions.
4. **Distance à la mesure.** Elle sert d'attribut, de couleur, de priorité de simplification ou d'indicateur d'erreur.
   Elle ne décide ni d'une activation ni d'un rattachement. Je retire l'affirmation « $d_k$ instable » : $d_k$ est
   stable pour un déplacement borné (entrelacement additif de ε), pas pour une petite masse déplacée loin. Je note aussi
   la convention stricte de la définition 10.12 de BCY.
5. **Réduction.** Aucun Wrap d'ordre k par analogie. On applique vos effondrements filtrés certifiés (§ 4.2), avec un
   budget géométrique déclaré par composante.
6. **Rendu.** Faces exposées, plus les strates isolées de dimension 0 à 2, avec l'identité du nœud.
7. **Contre-épreuves.** {0, 1, 10}, {0, 1, 2, 11}, le tétraèdre qui donne l'octaèdre, les coquilles dégénérées et les
   contacts de basse dimension deviennent des fixtures.

## 3. Questions sur ce qui reste ouvert

1. **Robustesse.** Quels modèles de perturbation et quels énoncés retenez-vous ? Je propose trois cas :
   - (a) déplacement borné de ε, qui couvre le bruit et la quantification à 1 mm (ε au plus √3/2 mm) : entrelacement
     additif de ε des filtrations $A_k$, à travers leurs équivalences naturelles avec $\Omega_k$ ;
   - (b) amas aberrants de moins de k points : ils ne créent aucune composante d'ordre k ;
   - (c) sous-échantillonnage.

   Des énoncés sur les types d'homotopie vous suffisent-ils ? Ou exigez-vous une stabilité géométrique du représentant,
   par exemple une borne de Hausdorff entre les représentants de P et de P' ?
2. **Qualité géométrique.** Le seul contrôle acquis est $d_H(\lvert A_{k,v}(r)\rvert, C_v(r))\le r$. Accepteriez-vous un
   budget relatif $\varepsilon_v\le\theta r$ ?

   Une remarque. Une cellule $P_{I,U,k}$ a un diamètre au plus $\min(j,\lvert U\rvert-j)\,\mathrm{diam}(U)/k$. Cela fait
   au plus $2r/k$ à une jonction, et au plus $4r/k$ en position générale dans R³. Un effondrement qui reste à l'intérieur
   d'un intervalle de même naissance déplacerait alors la géométrie d'au plus ce diamètre. Une borne de ce type, en
   r/k, serait-elle un énoncé acceptable ?
3. **Choix de l'appariement.** Votre § 4.2 laisse l'appariement libre. Trois priorités sont candidates :
   - de l'extérieur vers l'intérieur, les faces libres exposées d'abord ;
   - par la distance à la mesure, comme vous l'autorisez ;
   - par la distance aux sites.

   Voyez-vous un choix canonique qui ait une propriété démontrable, par exemple la minimalité, ou la conservation des
   sommets dont les labels portent les sites ? La subdivision barycentrique est-elle nécessaire ? Un certificat
   d'effondrement polyédral posé directement sur les cellules convexes de la mosaïque serait-il acceptable ?
4. **Réalisation sur les données (« ombre »).** On envoie chaque cellule sur $\mathrm{conv}(\bigcup Q)=\mathrm{conv}(I\cup U)$.
   Ce n'est pas un plongement. Mais sa trace sur P est $P\cap(C\oplus\bar{B}_r)$ (votre § 1.3), et elle est incluse dans
   $C\oplus\bar{B}_r$, puisque chaque cellule est contenue dans $\bar{B}(y_0,r)$. L'acceptez-vous comme rendu déclaré,
   à côté du solide barycentrique ?
5. **Niveaux d'un nœud.** Un nœud vit sur $[b_v,d_v)$. Quel niveau, ou quelle famille de niveaux, doit porter la
   géométrie d'un jeton : le niveau juste avant sa mort, qui donne le plus grand représentant du nœud seul, ou la
   famille filtrée entière ? Entre les ordres, calculer un représentant par ordre et garder les liens π0 certifiés
   est-il, pour vous, la réponse finale ?

Le workflow relancé ce soir intègre votre note sur cinq chantiers :

- un oracle borné de $A_k(r)$ ;
- la réduction certifiée ;
- le rendu des faces exposées ;
- la robustesse ;
- l'échelle.

Ses résultats vous seront transmis.
