# S6 actuel et porte combinatoire pour coquille24

Relecture du 5 octobre 2026, sans natif/build/GCP/fit, sans modification du produit ou d'un reçu clos. Deux sources distinguées : contrat et oracle S1 publiés à `5adf6a59f3d3b99fdf947e676e8548d4102ab48f` (`published/`) ; S6 non suivi du worktree `v11-impl-s6`, base `f98aeed67d4030dd78e11d5faf7d8556c4d17aaf` (`wip/`). La question documentaire §E de `9290cf3bfe51147ede73d178dce8bf62e31c82d8` est capturée séparément (`published_delta/`). Les rapports `fix_s6`, `verif_s6`, `impl_s6` sont des déclarations du développeur, pas une qualification native de cet audit.

Verdict source : aucune nouvelle erreur matérielle trouvée dans les primitives Q_b/comptes relues. La factory `make_shape` garde p≤11 avant p+q, fermant le témoin UINT32_MAX ancien ; les aides d'incidence gardent l'arité2..4 avant les casts. Les supports minimaux sont écrits avant la fermeture zêta ; toutes les arités sont conservées, sans test cofaces>0. Un quadruplet utilise `strictly_inside` sur ses propres sommets, pas le drapeau de présentation de la sphère. Les tableaux de marques sont bornés avant les décalages. Les refus autorisent explicitement des buffers indéterminés, sans résultat ni ajout au registre. Le raccourci régulier s'appuie sur le Catalogue immuable certifié.

Les changements `fix_s6` relus sont présents : registre privé par fil, réduction `add` après jointure et nouvelle porte concurrency ; documentation des trois refus de `ball_shape` et témoin 0/2/4 hors Cat1 ; deux refus de fermeture étrangère à Shape (N_qmin nul et N_j non nul sous qmin) avec fixtures et mutants ciblés. Les gardes reposant sur les invariants déjà certifiés du Catalogue sont identifiées comme telles. Aucune exécution de ces tests/mutants n'est revendiquée ici ; les portes et nombres locaux des rapports ne deviennent pas une preuve G4.

## Réponse concrète à §E

La coquille entière `x²+y²+z²=5` contient24 sites. La translation commune(+2,+2,+2) donne des coordonnées entières admissibles ; l'homothétie entière positive conserve tous les résultats géométriques. La garde indépendante confronte les12926 parties de taille2..4 (276+2024+10626) à deux décisions exactes : déterminants/angles et poids de Gram Fraction des helpers S1 épinglés, extraits par AST sans importer/exécuter l'oracle intégral.

| Quantité |2|3|4|
|---|---:|---:|---:|
| supports minimaux Q_b par arité |12|24|792|
| parties fermées N_j |12|288|3906|

Donc828 supports minimaux. Exemples : paire `(2,1,0),(-2,-1,0)` ; triangle `(2,1,0),(-2,1,0),(1,-2,0)`, poids `(1/4,5/12,1/3)` ; tétraèdre `(2,1,0),(-2,1,0),(0,-1,2),(0,-1,-2)`, poids `(1/4)^4`, déterminant−32. Tous les sites appartiennent à la même sphère. Deux permutations de l'entrée préservent les supports comme ensembles de coordonnées et les comptes. Les coordonnées mises à l'échelle dans les domaines18/21/24 sont vérifiées en Fraction ; ce n'est pas une qualification des intermédiaires C++ de ces profils.

Avec p=0 :

|K|kparties_reliees=compressed_parts|strict_traces|cofaces=gabriel_cofaces|
|---|---:|---:|---:|
|1|24|24|12|
|2|276|264|288|
|3|2024|1736|3906|

Les792 tétraèdres restent dans Q_b à K1, avec0coface par support. Compter seulement les supports eux-mêmes après suppression de zêta donne24 au lieu de288 pour N3, et792 au lieu de3906 pour N4. Publier la fermeture comme liste minimale ajoute des parties non minimales : un triplet contenant un diamètre a un poids nul. Ces trois gardes sont distinguées dans le modèle.

**Porte conseillée :** une suite long dédiée aux primitives Q_b et N2/N3/N4, puis comparaisons natives des listes complètes et des comptes à K1..3. N_j se dénombre parmi les combinaisons de j sites en testant leurs sous-parties positives≤4. Aucun parcours2^24 n'est requis. Ne pas appeler `Supports.order/check` intégral à24 : `_minimal_nonseparable` de S1 prolonge également les sous-parties séparables jusqu'à m. Changer le seul calcul de N_j ne borne donc pas cet oracle complet. Une extension doit annoncer son domaine ou refuser explicitement une limite de ressource, jamais omettre des contrôles silencieusement.

Les quatre autres sphères proposées sont facultatives ici ; elles ont été calculées par déterminants exacts/containment, sans deuxième confrontation Gram de tous leurs supports :

|rayon carré|sites|Q2|Q3|Q4|N2|N3|N4|
|---|---:|---:|---:|---:|---:|---:|---:|
|6|24|12|8|918|12|272|3768|
|10|24|12|24|792|12|288|3906|
|11|24|12|0|990|12|264|3696|
|13|24|12|24|792|12|288|3906|

Les cinq sphères complètes ont un diamètre canonique (présentation2). Leur mélange de supports Q3/Q4 ne remplace pas les portes num sur sphères de présentation3/4.

**Frontière25 :** `x²+y²+z²=9` contient30 sites. Prendre les6 points des axes±3 et19 autres sites distincts (liste exacte dans `normal.json`), puis translater par(+3,+3,+3). L'antipodie conservée garantit la MEB de centre(3,3,3), rayon carré9, qmin2 et coquille complète25 dans ce nuage sélectionné. Attendu de la sortie supports : refus entier `support_shell_capacity`, sans export partiel, à qualifier nativement. Aucun plafond24 n'est ajouté à FULL. Notre modèle vérifie la géométrie et le refus de la factory ; il n'exécute ni le catalogue ni FULL.

## Compacité et admission future

`support_count` u16 est suffisant localement (majorant12926). Les comptes par boule tiennent en u32 sur Shape valide : maxima C(35,12)=834451800 et C(35,13)=1476337800 ; leurs sommes sur B≤UINT32_MAX tiennent en u64. Les compteurs globaux S et Z ainsi que leurs décalages restent u64 : S≤12926B et Z≤4S, ce n'est pas une capacité d'allocation garantie. Le manifeste agrège les cofaces par boule ; les incidences par support restent une autre quantité.

S6a n'alloue aucun grand tableau : spans out/scratch à la charge de l'appelant. L'assemblage count/fill, les allocations et la publication entière sont encore annoncés pour S6b/S7, pas présents dans ce snapshot. Admission à préserver avant les tâches : domaine et arbre résidents + métadonnées de compte + workspaces des fils actifs + sorties exactes, avec toutes coexistences. Un workspace comprend `8*closure_words(m_max)` et `support_capacity(m_max)*sizeof(Support)` si une liste temporaire est nécessaire. À m24, fermeture seule=2Mio par fil, soit96Mio pour48 fils ; majorant conditionnel, aucun pic mesuré ni sizeof(Support) natif. Pré-passe sur les seules boules sélectionnées pour borner m, admission checked des sorties/offsets, écritures à positions fixes, buffers et registres privés, refus de l'appel entier. La porte de faute doit atteindre cette allocation finale, pas un refus amont du catalogue.

## Rejeu et fermeture

13491 gardes PASS, sorties normal/−O strictement identiques. Un échec initial du vérificateur texte (majuscule/retour de ligne du commentaire) est conservé dans `history/01_preflight_text_guard/` ; aucun produit n'y a été exécuté. Les autres exécutions aboutissent à0 ; stderr final vide.

```sh
python3 -B -S check_followup.py > normal.json 2> normal.stderr
python3 -B -S -O check_followup.py > optimized.json 2> optimized.stderr
cmp normal.json optimized.json
sha256sum -c SHA256SUMS
```

BEFORE précède la lecture. AFTER recoupe les captures : toutes sources S6 WIP et rapports sont inchangés ; la réponse publiée5ad a évolué en929 pendant la coordination et les deux versions sont conservées. Aucune prétention de source LIVE unique non modifiée. Aucun nouveau défaut ouvert. Capsule close ; toutes les pièces sauf SHA256SUMS racine sont inventoriées.
