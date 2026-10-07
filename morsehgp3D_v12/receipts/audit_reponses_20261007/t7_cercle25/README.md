# WIT-T7-CERCLE25 : oracle exact proposé, 7 octobre 2026

`phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Modèle Python borné seulement ; aucun moteur natif, GPU ou GCP exécuté. Aucun produit,
prototype, registre ou état de constat modifié. **CST-0106 reste ouvert**.

Le témoin contractuel à quatre sites est maintenant exploitable dans ce reçu :
E=(5,0,0), G=(4,3,0), H=(-3,4,0), J=(-3,-4,0), centre nul, rayon carré 25.
La fenêtre commençant en G est GH, strictement incluse dans EGH ; (1,1,0) sépare
EGH du centre avec les produits scalaires 5,7,1. Cela réfute l'ancienne assertion
« toutes les fenêtres énumérées sont maximales ». **Cela ne réfute pas le quotient.**

La recette historique parcourant les deux orientations donne cinq ensembles
distincts : EGH, EGJ, EJ, GH, HJ. Les trois maximaux sont exactement EGH, EGJ, HJ.
Avec une seule orientation positive, on obtient quatre fenêtres, sans EJ ; ne pas
confondre ce parcours spécialisé avec la double orientation du pseudocode.

| t | Partition exacte des t-parties séparables |
| --- | --- |
| 1 | {E,G,H,J} |
| 2 | {EG,EH,EJ,GH,GJ} ; {HJ} |
| 3 | {EGH} ; {EGJ} |
| 4 | aucune, naissance |

Le disque critique a p=0, m=4, q_min=3 : sa fenêtre réelle est k=2..4. La ligne
t=1 vérifie le quotient sous la fenêtre. EHJ et GHJ sont deux supports de taille 3
contenant strictement le centre, de poids respectifs (3/8,5/16,5/16) et
(3/7,1/8,25/56). Aucune paire n'est antipodale.

`check.py` énumère les 15 parties non vides et leurs disques minimaux par centres
de supports de taille 1..3, en `Fraction`. Il recoupe chaque résultat avec un test
indépendant d'enveloppe convexe (segments et triangles). Sur une sphère de rayon R,
le centre est hors de conv(A) exactement quand le rayon minimal de A est <R : un
séparateur strict permet de déplacer légèrement le centre et de diminuer toutes
les distances ; réciproquement, une combinaison convexe centrée donne la borne
r²≥R²+||c||² pour toute boule contenant A. Aucun angle ni flottant n'intervient.

Le graphe géométrique brut relie A,A' si A∪A' est séparable. Ses partitions sont
comparées, ensemble par ensemble, au quotient des cinq fenêtres, puis à celui des
trois maximaux. Les deux coïncident pour t=1..4. La suppression d'une fenêtre
M⊂N conserve les t-parties couvertes ; chaque arête M—X est relayée par N—X car
|M∩X|≥t implique |N∩X|≥t. Elle ne peut donc changer ce quotient si N est conservé.
Ce témoin ne démontre pas un surcompte de sites de la clause (iv) : ici le
complément de l'union est vide à t≤3, puis EGHJ à t=4.

La traduction entière (+3,+4,0) donne (8,4,0),(7,7,0),(0,8,0),(0,0,0), centre
(3,4,0), même rayon carré. Les 15 niveaux et séparabilités sont revérifiés sur
cette entrée positive u21. Les lettres sont des identités de fixture : un futur
test natif doit normaliser sa permutation de SiteIdx/Morton avant comparaison.

Point d'intégration proposé : dans le prototype TMVR `tests/tower/oracle_tour.py`,
après `carre_k4` à la ligne 43 et avant la fermeture de `WITNESSES` à la ligne 44 :

```python
('wit_t7_cercle25', [(8,4,0), (7,7,0), (0,8,0), (0,0,0)], 4),
```

Cet ajout exercerait le raccord TMVR existant. Il ne prouverait pas que la future
route LEM-T7 est jouée : celle-ci devra comparer sa famille et son quotient aux
attendus de `fixture.json`, en conservant la distinction non maximal/maximal.
Le contrat exige ce témoin quand LEM-T7 est activé ; `src/tower/tower.hpp:13-14`
le décrit encore comme amélioration future. Le cinq-sites `circle25_pair` ne
remplace pas ce quatre-sites. Aucun défaut FULL actuel n'est déduit de ce manque.

Rejeu autonome, depuis ce dossier :

```sh
python3 check.py
python3 -O check.py
```

Avec `--repo /chemin/E-HGP`, le script vérifie le hash du modèle historique puis
ne lui soumet que ces quatre sites (famille, quotient et géométrie brute). Cette
option produit le `results.json` archivé : code 0 normal et -O, sorties identiques,
sources stables avant/après. Sa suite historique complète n'est pas appelée.

Sources : main `7e87b58d2fa0abb57d10f031bf39d6c9bd82764c`,
`docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md:99,132`, `docs/CONTRAT_TOUR.md:244` ;
ancienne conception v11 `CONCEPTION_TOUR.md:138-171,760-775` et son
`preuves_tour/quotient_check.py:98-161` ; contre-lecture archivée `:108-127`.
Prototype TMVR repo3 : seules les deux sources lues sont épinglées, aucune
livraison déduite. Les sept SHA-256 et le hash du résultat sont dans `capture.json`.
