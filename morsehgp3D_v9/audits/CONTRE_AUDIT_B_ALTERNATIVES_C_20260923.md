# Contrelecture B — portée des alternatives C pour le contrat LiDAR

23 septembre 2026. Lecture du [rapport C](AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md),
de ses pièces `c_alternatives_20260923/`, du reçu [G4 R8](../receipts/g4_tower_r8_20260923/README.md)
et de sa [contrelecture](CONTRE_AUDIT_B_G4_R8_20260923.md). Aucun calcul G4
nouveau, aucune modification du moteur. Cette note précise le statut des
chiffres ; elle ne retire pas l'intérêt des expériences proposées.

## D3 : une cible de travail, pas une attribution du temps G4

La fraction **66,26–70,15 %** désigne les fenêtres CPU instrumentées *arête
et filtre de rectangle* d'une **seule** trame locale sans sol,
08/000200/K5/W1 : 68,960/104,079 CPU·s après soustraction **modélisée**
de 1 µs par fenêtre, ou 91,028/129,765 CPU·s sans cette correction. Ce
n'est ni la part du q3/q4 complet sur G4, ni une borne sur d'autres scènes.
Les arêtes de plus de 1,6 m y représentent 21 333 739/22 722 345 arêtes
développées. Leurs 52 885 émissions sont **5,944 % des 889 656 émissions
q3+q4**, et non 5,9 % de toutes les boules : face aux 1 407 885 boules du
catalogue, la fraction est 3,756 %. La sonde archivée
`c_alternatives_20260923/experiences/d3_refut/attrib_scene_02_full_k5.json`
ne porte pas le hash de son entrée temporaire ; ses comptes principaux
recoupent le reçu local v12, ce qui n'en fait pas une attribution R8/G4.

Après la propre correction d'horloge D3, le critère secondaire « plus de
50 % sur les sept coupes à 0,5 m » n'est vrai que sur **5/7** : les deux
coupes 8k de 08/000200 valent 49,9 % à K5 et 48,4 % à K10. Le critère
d'abandon préannoncé « au moins deux scènes » reste satisfait. Les parts
K10 de trame entière 48,6/56,3 % issues de `fullframe_model.py` sont des
**estimations**, pas des minorants démontrés ni des mesures directes.
Le seuil « moins de 25 % du CPU q3/q4 » est un **critère expérimental
proposé** pour des certificats d'ancres longues, pas un résultat atteint.

## D5 : le gain FULL est conditionnel et change plusieurs postes

Le sidecar D5 compare 5 041 956 racines **pré-lot** sur les coupes
emboîtées 8k/K10, 8k/K5 et 16k/K5 d'une seule scène, 08/000100, et des
fixtures dégénérées. Il n'a pas mesuré une tour FULL G4 avec le payload
public identique. Son premier tampon de 13 racines déborde sur une
coquille à 12 sites/32 parents ; la règle 0 du saut et l'ordinal de
programme d'un groupe compact ont dû être corrigés. Les portes de sortie
exacte E1/E2 restent à exécuter. Le « FULL ×6–13 » et « K10 3,2 s vers
0,25–0,6 s » sont donc des **projections conditionnelles**, pas un levier
produit déjà mesuré. Voir aussi la [note D5 B](CONTRE_AUDIT_B_D5_FULL_MAIGRE_20260923.md).

R8/08/000100/K10 consacre déjà **1,05 s** à validation, populations,
images, banque et encodage, hors phase statique et lots. D5 doit réduire
ou justifier exactement chacun de ces postes pour réaliser son chiffre.
La tour compacte, son expansion mesurée séparément et le digest placé
hors chronomètre changent la **sortie et la frontière de mesure** de la
chaîne actuelle ; l'appel public R8 produit encore un digest synchrone,
même si `chain_total` l'exclut. Toute comparaison causale doit garder les
mêmes octets de sortie et le même périmètre de temps, ou publier
explicitement deux contrats distincts.

## Ce que les reçus ne permettent pas de conclure

Les **1,15–1,64 s** de résidu K10 quand on soustrait q3/q4 et FULL à R8
sont un coût du **chemin CPU actuel**. Ils prouvent que rendre ces deux
phases gratuites, sans rien changer d'autre, ne suffit pas sur ces cas.
Ils ne sont pas une borne inférieure pour un autre générateur, une autre
fusion, ni un GPU résident. « K10 sous 1 s hors de portée de toutes les
familles », les probabilités numériques des jurys et « émission par
niveau préalable à 100 ms » doivent donc être lus comme **jugements
conditionnels de conception**, non comme théorèmes ou seuils de contrat
renégociés. Aucune sortie GPU de tour v9 ne permet encore un verdict G4 GPU.

La grille 1 mm est bien le **profil v9 prioritaire** selon la dernière
décision utilisateur ; float32 est secondaire. Cela ne remplace pas le
contrat sur **trames brutes entières de plusieurs séquences** par les trois
trames **sans sol** de la seule séquence 08. Pour cette ligne sans sol,
publier aussi segmentation, préparation et temps total. Les sept morceaux
et les disques emboîtés servent à l'échelle, pas au contrat.

## Suite utile au développeur

1. Garder D3 comme instrument de localisation du coût, mais attribuer
   fenêtres, `core_form_sites`, chemins de rejet et émissions sur les
   mêmes trames et le même binaire que l'ablation G4. Juger le **travail
   aval et le mur**, pas seulement le nombre d'arêtes longues rejetées.
2. Porter D5 par une première tranche exacte (jointure graines/selles
   dans la résolution statique, repli actuel) ; exiger toutes les
   cibles/racines, le payload et le digest identiques avant de projeter
   un gain FULL. Décomposer ensuite la queue de 1,05 s.
3. Mesurer l'ordonnancement q3/q4 v14 séparément, puis comparer un port
   GPU au meilleur CPU sur **le même ledger**. Aucun pourcentage subjectif
   du rapport C ne doit fermer K10 ou 100 ms avant cette mesure.
