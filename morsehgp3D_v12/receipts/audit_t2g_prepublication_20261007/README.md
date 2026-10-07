# T2 G — lecture avant publication et contre-épreuve du juge

**Le juge de déterminisme accepte des ordres manquants, même avec la ligne attendue par le wrapper CTest.**
Cette réserve prolonge le contrôle des juges de `CST-0018`, sans nouveau constat global. Elle ne démontre aucune
erreur géométrique du moteur ni falsification d'une prise historique. Sources en construction non commises à la
capture ; HEAD `59d604b87`, empreintes dans `sources.json`, copie du juge et du wrapper conservée.

Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `objet=full_pi0`,
`quantification=quantized_u21_input_only`, `public_status=not_claimed`. Aucun calcul natif, compilation, grand nuage,
GCP ou nouvelle mesure de vitesse. Les sondes sont de minuscules scripts qui impriment uniquement du JSON fourni.

## Contre-épreuve causale

`check.py` appelle le vrai CLI `g_determinism.py` épinglé avec des doubles de sonde retournant le code 0. Sept cas
passent en Python normal et `-O`, avec des sorties identiques : témoin à cinq ordres ; seul k1 ; k3 omis ; k2 dupliqué
à la place de k3 ; digest non hexadécimal ; digest de type liste ; et témoin calé sur la ligne CTest synthétique.
Les six premiers demandent `--k=5 --threads=1,48`, sans réellement créer de travailleurs du moteur.

Le cas CTest utilise `--uniform=8000,20261007,18 --k=5 --threads=1,8`. Il n'émet que k1, annonce 8 000 sites mais
376 649 naissances à cet ordre, recopie les totaux épinglés dans `tests.cmake`, et construit un digest de 64 caractères
avec le préfixe attendu suivi de zéros. Le lecteur rend code 0 et exactement cette ligne :

```text
g_determinism_ok cas=synth_u8000_k5 fils=1,8 empreinte=5304d1c8fe7c25ff naissances=376649 cellules=600630 representants=1780799 cibles_cellule=370896
```

Le script compare cette ligne à celle du wrapper ; il ne lance pas CTest ni sa sonde native. Le simple témoin k1
avec un total de naissance égal à 1 aurait été rejeté par cette ligne attendue : ce contrôle supplémentaire est
pris en compte ici. Les totaux ci-dessus viennent de la porte existante et ne sont pas de nouveaux résultats.

**Cause :** `run()` ignore la ligne `tour_g` ; `main()` n'exige qu'une liste d'ordres non vide ; `invariants()` saute
les identités de chaînes pour k1 et ne rattache pas ses naissances au nombre de sites. La liste d'ordres observée et
le digest auto-déclaré peuvent donc être également incomplets à deux nombres de fils. L'affichage final réduit le
digest à 16 caractères, seule partie épinglée par CTest.

## Remède uniforme proposé

Avant de comparer les régimes, admettre un schéma strict de sortie : une ligne `tour_g` réussie, K et nombre de fils
égaux à la commande, nombre de sites entier strict positif et cohérent avec l'entrée ; les ordres exactement
`1..min(K, sites)` une fois chacun, dans cet ordre ; `births(k1) == sites` ; compteurs entiers non négatifs, sans
booléens, champs nécessaires complets et histogramme de 16 cases. La quantité `out.orders()` peut être publiée
explicitement et doit alors être égale à `min(K, sites)`, jamais servir seule d'autorité.

Exiger exactement un digest de type chaîne SHA-256 complet (64 caractères hexadécimaux) ; comparer les 64 caractères
entre régimes **et** à la référence épinglée, même si l'affichage humain reste abrégé. Les JSON illisibles, phases
refusées, champs absents ou types invalides doivent donner un refus contrôlé.

Enfin, parser `--threads` strictement et exiger au moins deux nombres positifs **distincts** : actuellement `1,1`
compte comme deux régimes et les éléments non numériques sont silencieusement éliminés (lecture seule, sans
reproduction supplémentaire). Le test enregistré 1/8 apporte bien une comparaison parallèle ; le régime 1/48
contractuel reste à jouer sur G4. Aucun nouveau microbanc du moteur n'est demandé par ce reçu.

## Lecture du moteur G

Aucun contre-exemple causal trouvé dans le corps lu : `LEM-T1` teste les deux inclusions ; la table de populations
recompare toute la liste des SiteIdx ; un support minimal non canonique mène au census sans déclarer à tort la
sphère absente du catalogue ; le niveau est contrôlé avant les sauts, même saturés. Le census gardé reçoit un
`CertifiedBall`, et les requêtes exactes génériques incluent la requête dans leur budget numérique.

Les cibles sont écrites à des places exclusives, les espaces census et compteurs sont privés par travailleur, puis
réduits par sommes/maxima : aucune course de mémo visible. Les plafonds et refus typés sont déclarés. Cette lecture
ne remplace ni un test natif W1/W48 ni la preuve des étages T/M/V, absents de ce premier module G ; `LEM-T5` n'est
donc pas qualifié ici. Le double parcours count/fill respecte le protocole d'allocation ; aucun défaut de performance
non mesuré n'en est déduit.

Rejeu depuis ce dossier : `python3 -B -S check.py`, puis `python3 -B -S -O check.py`. Commandes, codes, sorties et
empreintes sont dans `normal.json` ; `verification.json` et `SHA256SUMS` ferment la capture.
