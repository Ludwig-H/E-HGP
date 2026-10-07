# Contre-lecture des contrats de mesure et de différentiel

7 octobre 2026. Auditeur Codex, volet mesure. Pin documentaire examiné :
`fc1f913ce325b04922b897137a9507153b129894`. Le contrat numérique est celui introduit par
`e264de6f2`, SHA-256 `40baf861e474c89e3155513cdbfa3f7a61896412a85948029c776bd1ad03af0e`.
Il remplace le brouillon antérieur `39c19a00…` pour cette note. Les lectures viennent de
`git show` au pin ; le worktree de rédaction reste à `13c52bc60`.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
D1 à D15 sont enregistrées, avec D10/D13 reportées et D14 réservée à l'utilisateur : aucune décision n'est rouverte.
Ce reçu ne qualifie aucun moteur v12. GCP non utilisé. Aucun code produit modifié.

Deux incohérences doivent être corrigées avant de figer les portes : l'empreinte v11 choisie n'est pas invariante par
translation ; le coût de la translation dans un binaire n'est pas le coût d'élargissement du profil demandé par D6.
Le premier témoin complète CST-0113, ouvert pendant le travail ; le second devient CST-0207.

## CST-0113 — l'empreinte de référence hache le repère absolu

**Classe : contrat de différentiel ; gravité majeure, latente avant code.**

`docs/CONTRAT_NUMERIQUE.md:124` nomme l'empreinte sémantique du lecteur strict v11 ; les lignes 154–155 exigent
qu'elle soit inchangée par toute translation entière admissible. Or `morsehgp3D_v11/bench/full_semantic.py:116`
hache les XYZ absolus des sites, et les lignes 151–152 les centres absolus des naissances.

Le script joint fabrique un FULL synthétique minimal exact : un site, PointId 7, K=1, une naissance au site,
niveau zéro, aucune arête, racine zéro. Les deux vidages de 290 octets sont acceptés par le lecteur original
épinglé, sans modification ni remplacement de ses contrôles.

| Entrée synthétique | Profil | Empreinte sémantique v11 |
| --- | --- | --- |
| (0,0,0), ID 7 | u21 | `537e27c5d0d486344490305ae5dbc43ab7b034aaae2845aa7cadd1c6d862640b` |
| (1,0,0), ID 7 | u21 | `e7049e99cedfd4d2201e88306ff550899b7593034af1bd5fe4cbcca07be99a43` |
| (0,0,0), ID 7 | u24 | `537e27c5d0d486344490305ae5dbc43ab7b034aaae2845aa7cadd1c6d862640b` |

La topologie et les niveaux sont identiques. Le contrôle positif u21/u24 confirme ce que fait réellement le lecteur :
il efface le profil et le bourrage des entiers pour une **même entrée absolue**, comme annoncé par `MESURE.md:100–102`.
Il ne quotient pas les translations. Le témoin viole la nouvelle obligation même dans u21 ; u32 n'est pas nécessaire.

**Correction attendue.** Garder deux contrats explicitement distincts :

- comparer la v12 et la v11 sur les mêmes coordonnées absolues par une représentation sémantique commune ;
- comparer une entrée et sa translation par une application explicite de la translation aux sites et centres,
  ou définir une seconde empreinte normalisée par un repère commun, avec ordre canonique indépendant de Morton.

Si la seconde option est choisie, soustraire le minimum global des coordonnées à tous les objets géométriques
publiés concernés et recanoniser leurs références ; ne pas confondre cette empreinte de test avec l'export absolu.
Le témoin et son contrôle positif doivent devenir des portes de l'adaptateur `MES-M0`.

Le lecteur v11 limite aussi `expected_bits` à 18/21/24 (`full_semantic.py:64`) et impose un ordre Morton strict
aux sites (lignes 110–112). Le témoin u32 est refusé par `expected header`, résultat conservé au JSON. Ce sont des
limites d'un lecteur ancien à adapter, pas des défauts du moteur v12 non écrit. `ARCHITECTURE.md:18–19` prescrit
l'ordre lexicographique publié ; cette adaptation doit garder le même objet mathématique et séparer identité
des IDs, ordre de sérialisation et origine des coordonnées. La présence d'un repli sémantique dans `MESURE.md`
évite d'exiger inutilement les mêmes octets après un changement de format.

## CST-0207 — la mesure proposée ne décide pas D6

**Classe : mesure/contrat ; gravité majeure pour l'adoption du profil, avant banc.**

`DECISIONS.md:27` demande un produit à un seul profil, **le plus large qualifié dont le coût sur LiDAR reste à moins
de 3 % de u21**. `CONTRAT_NUMERIQUE.md:35` fixe déjà le binaire à Bmax=32, tandis que les lignes 164–165 mettent le
seuil de 3 % sur une même trame originale puis translatée dans ce binaire. Ces deux ratios sont différents :

| Contre-modèle synthétique, en ms | Binaire u21, original | Binaire candidat, original | Même candidat, translaté | Surcoût du profil | Surcoût de translation |
| --- | ---: | ---: | ---: | ---: | ---: |
| A | 100 | 120 | 120 | +20 % : échoue D6 | 0 % : passe §8 |
| B | 100 | 100 | 104 | 0 % : passe D6 | +4 % : échoue §8 |

Ce sont des contre-modèles algébriques, **aucune mesure de moteur ou prédiction G4**. Ils prouvent que l'obligation
du §8 ne suffit pas à D6 et ne lui est pas équivalente. Un coût constant de registres, de disposition mémoire,
d'export ou de repli accru par compilation serait invisible dans le premier ratio si les deux entrées le paient.

**Correction attendue.** Déclarer Bmax=32 comme candidat de conception jusqu'à qualification. Pour D6, conserver
une base u21 et comparer les candidats u24/u32 sur les mêmes trames et dans le régime D1–D3, avec la même logique
et des sources/builds épinglés. Le produit retenu demeure un seul profil. Mesurer séparément, dans ce candidat,
les translations jusqu'aux bords pour leur correction et leur coût. Ce second banc est utile ; il n'est pas une
preuve du premier. La règle statistique décidant les 3 % doit être écrite avant les prises et appliquée à ce ratio.

Mettre les documents d'exécution en cohérence avec D6 : `ARCHITECTURE.md:14` et `PLAN.md:85–87` restent figés sur
u21 avec u24 en matrice. `PLAN.md:18` exige déjà une qualification au **profil produit** : cette règle est la bonne
pour le jalon final, sans transformer les profils candidats en plusieurs chemins produits.

## Identification des références et suite du banc

Le reçu joint extrait les métadonnées des trois rapports `claudeg1` mentionnés dans `MESURE.md:50`. Une correction
documentaire mineure est certaine : la ligne 41 donne le mode GPU `868347:400` aux colonnes K5 et K10 ; le rapport
K10 porte `868347`, tandis que K5 GPU porte bien `868347:400`. Les temps n'ont pas été recalculés dans ce suivi et
aucune nouvelle conclusion de performance n'en découle.

| Rapport de référence | K | Feuille | Mode des deux bras | Processus/bras/trame | Passes chaudes/processus |
| --- | ---: | ---: | --- | ---: | ---: |
| `gpu_ab_report_ab_k5_16_cpu.json` | 5 | 16 | `802811` | 6 | 10 |
| `gpu_ab_report_ab_k5_24_gpu.json` | 5 | 24 | `868347:400` | 6 | 10 |
| `gpu_ab_report_ab_k10_24_gpu.json` | 10 | 24 | `868347` | 3 | 10 |

Les trois déclarent `verdict=conforme`, W=48, binaires `new=d203b3a3…` et `base=d9123209…`.
Les chemins, SHA-256 complets des rapports et binaires figurent dans `result.normal.json`.
`PROVENANCE.md:3–6` pose correctement la requalification sans héritage. Il faudra relier, dans le reçu de chaque
nouveau banc, le SHA du binaire exécuté au commit, aux éventuelles modifications, au profil, aux options de
compilation et au manifeste source de chaque bras. Le hash d'un exécutable réutilisé ne suffit pas seul à ce lien.

Les défauts des juges v11 sont déjà recensés dans `audits/AUDIT_CODEX_20261007.md:19` et dans le contre-audit v11 :
pas de nouveau constat identique ici. Au port du juge unique demandé par `MESURE.md:141–142`, rejouer au minimum
les injections « verdict refus malgré chronos favorables », « une seule prise », « preuve/hash manquant » et
« cellule sans paire », en Python normal et optimisé ; chacune doit interdire l'adoption. Un composant v12 encore
absent n'est pas traité comme un défaut de code.

Pour D7, publier le maximum demandé à côté des médianes : une médiane de passes par processus ne constitue pas
une borne de latence. Préciser avant la campagne l'ensemble sur lequel porte le maximum (trames, processus,
passes), sans rouvrir la décision déjà prise. Réserver les ports de qualification au pin effectivement mesuré et
jugé : une qualification par union de plusieurs pins ne devient pas celle du gel final.

## Reproduction et limites

Depuis la racine d'un worktree qui possède le pin :

```sh
python3 morsehgp3D_v12/receipts/audit_contrats_20261007/mesure/check_measure_contracts.py
python3 -O morsehgp3D_v12/receipts/audit_contrats_20261007/mesure/check_measure_contracts.py
```

Les deux exécutions terminent avec code 0 et produisent des JSON identiques octet pour octet. Le script lit les
sources au pin par Git, charge les deux lecteurs d'origine et n'emploie aucun `assert` pour juger ses témoins.
`result.normal.json` et `result.optimized.json` sont les sorties conservées ; `SHA256SUMS` ferme le reçu.
Le FULL est construit en mémoire avec les coordonnées synthétiques affichées ci-dessus. Aucune donnée réelle,
aucun contenu personnel, aucune compilation moteur, aucun accès GCP. La portée est une contradiction de
contrats et de tests projetés, pas une erreur géométrique de FULL ni une mesure du futur moteur.
