# Note de Claude — le sweep à deux côtés est livré (contre-audit 63d364a)

Date : 17 août 2026. Base : `a524020` + le commit portant cette note.
Reçu détaillé :
`receipts/forest_20260817/ADDENDUM_SWEEP_DEUX_COTES_20260817.md`.

J'avais séquencé ce chantier après le dépouillement G4 ; la campagne
locale n=8000 l'a fait remonter en priorité 1 (eight_clusters est LE
cas dur, et son coût est exactement là où votre identité frappe). Il
est maintenant en place, tel que spécifié :

1. $d_{cover}(\mu) = p + P_<(\mu) + N_>(\mu)$ est l'unique lecture de
   profondeur du chemin axial ; le scan `depth_dead` par groupe est
   SUPPRIMÉ et remplacé par votre assertion de réception (`kAxialVerify`
   dans la porte appariée : égalité du COMPTE avec le scan complet sur
   chaque groupe émis, pas du seul verdict).
2. Table unique de groupes de $\mu$ exacts fusionnant les deux signes
   dans $[L,U]$, ties de frontière inclus, préfixes/suffixes en $O(1)$
   par groupe, mort AVANT `valid_completion`/`q4_form`.
3. Votre fixture entière § 5 (sphère $1513/49$, trois intérieurs hors
   $W_4$ et tous du côté opposé au complèteur) est gravée dans la porte
   appariée : clé absente à smax=6 avec mort bilatérale comptée,
   présente à smax=7 au bon niveau, baseline/axial identiques post-RLE
   aux deux caps.
4. Six mutants axiaux tués à code exact 4, dont les trois nouveaux :
   `ignore-opposite-side`, `reverse-negative` (survie indue de la
   fixture à smax=6), `depth-nonstrict` (mort indue à smax=7). 96
   portes CTest vertes.

Un point d'honnêteté : votre fixture a corrigé ma première version en
une exécution — le compteur bilatéral ne voyait que l'étage $d_j$,
alors que sur la fixture la mort se produit à la CLASSIFICATION (racine
positive sous $L$, seuil issu de l'ordre négatif : $\geq k$ négatifs
strictement au-dessus, $d \geq h_4$ exact). Le compteur couvre
maintenant les deux étages ; les émissions, elles, étaient justes dès
la première version (la porte appariée l'atteste).

Mesure sur le cas dur (eight_clusters n=1000, smax=11) : `t_gen`
135,9 s (baseline) → 96,4 s (par côté) → **87,5 s** (deux côtés,
−36 % cumulés), plus aucune évaluation q4 tuée au scan (877 M de morts
bilatérales sans scan ni complétion), sorties identiques (219 653
événements). Le poste dominant restant est le balayage $A,B$ des
4,4 M de seeds — la borne CPU naturelle de cette étape, et le noyau
régulier que vous destiniez au GPU.

Rien de tout cela ne promeut l'axial sur CPU : opt-in, apparié, muté ;
la production reste la baseline et la campagne G4 décidera. Restent en
file : votre audit « borne de Poisson q2 sur la masse de sortie »
(180n facettes dès q2 ; 5,4 G à 30 M) et le contre-audit « étendre la
borne q2 aux facettes de la forêt » — je les traite après le lancement
de la campagne.
