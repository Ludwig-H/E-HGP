# Frontière et hiérarchie de points — choix mathématiques actuels

2 octobre 2026. Produit u18 inchangé ; lecture des réponses jusqu'à
`afb081774` et des prototypes locaux actuels. Aucun moteur modifié/GCP.
`public_status=not_claimed`. Les preuves anciennes restent datées de leur
[lecture](../../receipts/audit_independant_20260930/notes_avant_synthese/README.md).

## Sens de la frontière dans la thèse

Parties I/II relues, [trace](../../receipts/audit_independant_20260929/lecture_these/receipt.json).
Théorème 2 pp.60–61 : C_discret(r)=X∩δ_r(C). Un point de frontière
peut couvrir plusieurs composantes sans être core. Une couverture commune
ne signifie pas une fusion spatiale. Le chapitre 9 normalise les
contributions par observation avant condensation ; un remplissage final
ne rétablit pas une branche déjà éliminée pour masse insuffisante.
FULL exact ne détermine donc pas seul une partition exclusive des points.

[Deux triangles, thèse §6.1](../../receipts/audit_independant_20260930/thesis_fixtures/thesis_geometry/README.md) :
côtés=CD=2r₀. Entre 2r₀/√3 et r₀√(2+√3), FULL possède ABC, CD, DEF,
avec couvertures recouvrantes. Cible utilisateur : **ABC | DEF avant
fusion**, C/D attribués aux triangles, pont CD conservé dans FULL.
Antichaîne-LCA diffère ces deux points jusqu'à la fusion : garanties
formelles correctes, cible manquée. Majorité de bande retrouve la cible
sur l'idéal exact et les deux variantes entières ; cela ne la rend pas
robuste en général. [Oracle et captures](../../receipts/audit_independant_20260930/thesis_fixtures/README.md).

Après projection dure, deux masses de 3 ; avant durcissement, la bande
uniforme à activation par marches donne 8/3, 2/3, 8/3. Un même nombre
mcs ne représente pas le même seuil dans ces deux modèles. Cette bande
n'est pas la mesure complète de cofaces de la thèse.

## Ce que les nouveaux essais ont tranché

[Réponse développeur du 2 octobre](../REPONSE_CLAUDE_BATTERIE_TOUR_HIERARCHIES_20261002.md) :
la variante par taille couverte passe 125 jugements mais conserve la
fragmentation à mcs=K ; celle par taille de cœur améliore ce régime mais
échoue les triangles. Trois nouvelles fixtures réfutent la continuité
universelle du vote avec date à marge. Une réussite de fixtures ou une
moyenne favorable ne choisit pas encore une règle statistique générale.

La comparaison demandée reste : cibles présentes dans FULL → blocs
présents dans la projection → compatibilité simultanée. Distinguer
antichaîne et coupe horizontale commune. L'IoU maximal dépend de la
richesse de la famille, et une classe MAP n'est pas nécessairement une
composante d'un niveau de densité. [Univers des nouveaux tableaux](../../receipts/audit_independant_20261002/battery_review/README.md).

## Maturité : contrat à garder explicite

Pour une projection P1 fixée, u_P(x,y)=max(e(x),e(y),b_LCA(o(x),o(y))).
Si m≥e et n≥mcs, t(x) est la mcs-ième valeur de max(m(y),u_P(x,y)),
et u_M(x,y)=max(u_P(x,y),t(x),t(y)). Max et statistique d'ordre sont
1-lipschitziens en norme sup : stabilité CONDITIONNELLE aux hauteurs de
P et à m. Cela ne répare ni un propriétaire discontinu ni une entrée
instable. Le carré K2 peut perdre ses deux blocs après retard de maturité.
[Preuve et réserve déjà transmises](../audit_continu_20260929/QUESTION_RACCORD_CORRECTIFS_ET_FRONTIERE_20260929.md#correctifs-courants-et-réponses-q13-q14-q15).

La version géométrique utilise le continuum C_r, union des intersections
fermées des K-boules, avec dist(x,C_r)≤τr. Elle ne se réduit pas aux
centres MEB. Un point peut être mûr dans deux composantes ; leur taille
mûre ne garantit pas mcs membres exclusifs après attribution. **Trois
points 0,2,4 suffisent** : K2, τ=1/2, r=4/3, deux composantes mûres de
2 sites, impossibles à transformer en deux blocs disjoints de2.
Plancher m≥e nécessaire si l'entrée de P est retardée.
[Petit témoin exact et implications](../../receipts/audit_independant_20261002/maturity_review/README.md).

La validation rétroactive R garde des dates précoces après avoir regardé
la vie future d'une branche. Elle reste définissable hors ligne, mais
non causale et potentiellement discontinue. Ses gains de sélection ne
répondent pas à eux seuls à la priorité actuelle sur la hiérarchie.

Le calendrier ajouté par M doit aussi être exact : niveaux, dates
interpolées et dates de cône ont leurs identités/rangs, même si leurs
valeurs double coïncident. [Helper capturé contre un oracle rationnel](../../receipts/audit_independant_20261002/date_order_review/README.md) :
2 880 clés conformes normal/−O ; portée scalaire, pas conformité FULL.

## Invariants conservés pour v11

Un K fixé reste la cible. [Les croisements inter-K](../../receipts/audit_independant_20260930/cover_band_followup/README.md)
limitent une future combinaison des ordres, sans bloquer cette cible.
Conserver toutes les incidences I/U et leurs entrées internes K3/K5 ;
les fusions FULL à p+q_min=K+1 ne sont pas les seuls témoins de couverture.
Univers et dénominateur de vote figés, plateaux atomiques, égalité sans
vainqueur, puis propriétaire engagé une fois et ancêtres : laminarité.
La bande K2 exige aussi des paires non critiques.

[Condensation par cohortes](../../receipts/audit_independant_20260930/developer_rebound/condensation_reference/README.md) :
terminer à la cohorte où la masse restante passe sous mcs, même sans
scission géométrique ; aplatir les événements simultanés au plateau parent.
Pour une masse progressive, préciser si le seuil est testé aux splits
seuls ou pendant toute la vie : un franchissement intérieur existe.
[Cas exact](../../receipts/audit_independant_20260930/developer_rebound/fractional_review/README.md).

Arrondi au plus proche de pas h : déplacement≤√3·h/2, à observations
et poids conservés. FULL est interlacé en rayon ; supports, coquilles,
projection dure et EOM peuvent changer. Même arbre simplement changé
d'unité : β_phys=h²β_grille et λ_phys=h^(−z)λ_grille, EOM idéal invariant.
Requantifier change l'entrée. [Contrat précision](../AUDIT_MASSIF_LIDAR_20260930.md).
