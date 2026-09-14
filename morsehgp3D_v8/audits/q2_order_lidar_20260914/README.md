# Ordre des témoins : comparaison sur trois scans LiDAR réels

14 septembre 2026 — auditeur A, sources produit **e3af11a7** extraites
de Git, sans modification du moteur. Cadre `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`. GCP non utilisé.

**Le nouvel ordre améliore modestement le temps q2 dans ces captures
LiDAR.** Sa baisse de tests géométriques est en grande partie remplacée
par des descentes structurelles. Il ne ferme ni P0 ni le contrat de tour.
Les sources conjointes A×B en chantier ne participent pas à ces mesures.

## Mesure appariée et portée

36 appels, 32 configurations : Global/Complement × none/sibling,
Kmax10, Samples/Shared, masque q2 seul. Les
[entrées déjà préparées](../lidar08_20260914/README.md) sont les scans
KITTI08 0, 100 et 200 à 8k, puis le scan 0 à 16k/32k/50k. s8 partout,
s10/12 en plus sur scan 0 à 8k. Une répétition du pilote inverse l'ordre
des quatre variantes. Aucun nouveau téléchargement, alignement de
points, accumulation temporelle ou modification de quantification.

Chaque appel utilise un seul CPU autorisé, fixé par affinité ; l'hôte
reste partagé. Pas de warmup ni distribution de latence. Les valeurs
temporelles sont des observations appariées, sans attribution statistique.
Le total comprend lecture du fichier, propriétaire, index, front, census,
collecte, callback canonique, vérifications et destructions. Il porte
sur un flux de supports q2 ; les K1..10 de la tour FULL ne sont pas calculés.

Temps **total** en secondes, premier passage à s8 :

| Entrée | Global/none | Complement/none | Global/sibling | Complement/sibling |
|---|---:|---:|---:|---:|
| Scan 0, 8k | 1,692 | 1,575 | 1,666 | 1,555 |
| Scan 100, 8k | 1,444 | 1,336 | 1,438 | 1,305 |
| Scan 200, 8k | 1,515 | 1,438 | 1,572 | 1,461 |
| Scan 0, 16k | 3,620 | 3,365 | 3,626 | 3,426 |
| Scan 0, 32k | 7,580 | 7,051 | 7,521 | 7,073 |
| Scan 0, 50k | 13,881 | 13,450 | 13,755 | 13,086 |

L'ordre Complement réduit le temps dans chaque comparaison à mode frère
fixé de ce tableau. Le frère ajouté à Complement n'améliore pas toujours
le temps : scan 200/8k, 16k et 32k gardent le signe contraire.
La répétition scan 0/8k donne respectivement 1,639 / 1,529 / 1,640 /
1,559 s : travail discret inchangé, mais le classement avec/sans frère
dans Complement s'inverse. Ne pas promouvoir automatiquement leur combinaison.

À 8k, Complement/sibling prend 1,555 / 1,551 / 1,668 s pour s8/10/12 ;
Complement/none 1,575 / 1,607 / 1,634 s. Cette capture ne désigne pas
d'optimum universel de séparation.

## Travail payé et croissance

Les visites géométriques du compte sont `count_bound_tests + count_point_tests`.
Les quatre compteurs d'ordre sont séparés : descentes structurelles,
sauts de B original, sauts d'ancre et transitions. Leur somme est un
décompte d'opérations de contrôle du parcours, pas leur équivalent en
instructions ou en temps. La collecte et les tests frère sont encore
d'autres postes ; ne pas les omettre ni compter deux fois `cursor_advances`.

| Scan 0, s8 | Visites Global/none | Visites Complement/sibling | Opérations d'ordre Complement/sibling |
|---|---:|---:|---:|
| 8k | 59 226 897 | 44 554 779 | 12 448 846 |
| 16k | 135 805 996 | 102 671 744 | 26 768 823 |
| 32k | 280 997 181 | 210 821 520 | 56 852 596 |
| 50k | 579 839 306 | 451 981 906 | 96 257 218 |

À 50k, les 96,26 millions d'opérations comprennent 89,54 millions de
descentes, 2,65 millions de sauts B, 3,10 millions de sauts d'ancre et
0,969 million de transitions. Les 1 134 116 tests frère restent payés
séparément. Une baisse de 22,05 % des visites géométriques ne représente
donc pas une baisse de 22,05 % du travail total. Le total temporel
Global/none → Complement/sibling baisse ici d'environ **5,73 %**.

Les doublements 8k→16k→32k multiplient les visites Global/none par
2,293 puis 2,069 ; Complement/sibling par 2,304 puis 2,053. Les deux
conservent donc des croissances voisines sur cette série. Ni trois tailles
ni le gain local ne donnent une borne asymptotique. Les temps des amas
synthétiques du constructeur restent une autre série, non requalifiée ici.

## Ce que cela apporte au census conjoint

Sur les scans 8k à s8, remplacer les Q démarrages d'ancres par R démarrages
de rectangles retire **22,5–25,6 %** des racines initiales. À 50k,
Q=3 829 093 et R=2 657 844 : différence 1 171 249, soit **30,59 %**.
Ces valeurs se recalculent dans les reçus sans nouveau recensement.
Elles décrivent les démarrages, pas une borne du gain de visites ou de temps.
Les produits pourront encore se diviser avant d'obtenir un crédit commun.

Le nombre de rectangles dont le petit facteur contient au moins deux
sites est au plus Q−R. À 50k, au moins 55,93 % des rectangles ont donc
déjà une ancre singleton et passeront immédiatement au chemin existant.
Cela justifie le relais avant préparation des 96 octets, déjà présent
dans le chantier relu. La [preuve du raccord et ses limites](../q2_product_20260914/README.md)
reste distincte de ces mesures d'ordre à ancres individuelles.

## Preuves et reproduction

[r1_BUILD.json](r1_BUILD.json) ferme les sources extraites de e3af11a7,
l'[archive](r1_sources.zip), les commandes et les binaires. L'adaptateur
[LiDAR](lidar_order_probe.cpp) dérive explicitement de celui du précédent
audit ; seuls les options publiques et leurs compteurs sont ajoutés.
Le callback canonique et ses contrôles sont conservés.

Le gate constructeur reconstruit dans ce snapshot passe : 1 798 appels,
48 922 supports complets, 436 117 contrôles, cinq mutants **modèles**.
C'est un rejeu Release de ce gate, pas une nouvelle suite ASan/UBSan
ni une qualification de toute la v8. Le défaut CLI incorrect est rejeté.

Les campagnes [pilote](campaign_pilot/COMPLETION.json),
[répétition](campaign_repeat_pilot/COMPLETION.json),
[autres scans](campaign_other_scans/COMPLETION.json),
[croissance](campaign_growth/COMPLETION.json),
[50k](campaign_check50k/COMPLETION.json) et
[séparation](campaign_separation/COMPLETION.json) conservent commandes,
sorties brutes, hashes d'entrée, affinité et charge hôte.
Le lecteur final [verify.py](verify.py) ferme ces comparaisons et exerce
huit mutants de reçus, dont une ligne échouée et une durée non finie.
[r1_VALIDATION.json](r1_VALIDATION.json) rapporte les lectures normal/−O,
les contrôles documentaires et le registre inchangé.

Les empreintes des supports complets, masses, front et collecte sont
identiques entre les variantes appariées. Les neuf nouvelles baselines
Global/none retrouvent aussi exactement le travail et les empreintes
des captures antérieures, y compris s10/12 issus du raccord q2 initial.
Ces sommes de contrôle ne remplacent pas un oracle géométrique à 50k.
Les capacités retenues publiées ne sont pas un pic RSS ni une qualification
de résidence à plusieurs dizaines de millions de points.

Rejouer le lecteur sans écraser sa clôture :

```bash
python3 -B morsehgp3D_v8/audits/q2_order_lidar_20260914/verify.py --name replay
```

Pour reconstruire, `build_snapshot.py --name nouveau` crée un snapshot
et des binaires neufs sous ce seul audit. Le runner `measure.py` prend
explicitement `--binary`, `--build-receipt` et `--plan` ; il refuse de
remplacer une campagne déjà présente. Les plans existants restent figés.
