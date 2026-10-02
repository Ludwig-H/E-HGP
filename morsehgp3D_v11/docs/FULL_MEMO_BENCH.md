# Ablation FULL du mémo exact

Source de développement postérieure à `f425c5fe7`, non qualifiée à ce stade.
Le contrat et la preuve du mémo vivent dans [DESCENT_MEMO.md](DESCENT_MEMO.md).
Le banc est CPU sur G4 ; aucune accélération ni cible de 200 ms acquise.

`full_probe` conserve les modes 0..3 du catalogue et ajoute le bit 4 pour
une table de 65 536 parties. Cette signification est propre au banc FULL :
le bit 4 du banc catalogue désigne sa frontière adaptative. Ici la frontière
reste fixe ; aucune option adaptative n'est activée implicitement.
Le mode 3 emploie cache J2 et tri indirect ; le mode 7 ajoute le mémo.

`bench/full_memo.py` prévoit 18 processus frais K1..5 : trois trames entières
sans sol × u21/u24 × modes 3/7, puis les synthétiques 8k/16k/32k u21 dans
les deux modes. W48, feuille16, plafond natif8GiB, un essai par configuration,
ordre 3 puis 7. Les entrées LiDAR restent la grille1mm figée du domaine18 ;
évaluer en u21/u24 ne crée aucune précision d'entrée supplémentaire.
La défaillance d'un mode ne supprime pas son partenaire. Les omissions de
budget sont publiées, jamais remplacées par des trames partielles.

Le chrono FULL contient index, catalogue/recherche, forêts et verticales.
Lecture, Cloud, Pool, sérialisation, décodage et segmentation restent séparés.
La table est temporaire et coexiste avec les sorties de la tour avant son
retour : son coût est `memo_capacity * memo_slot_bytes`, relevé par le
binaire du profil. Les octets de table sont rendus avant déplacement du
domaine. Le pic doit couvrir sorties retenues et table vivante ensemble.
Aucune taille ABI estimée n'est présentée comme mesure.

Le flux `full_work.v4` publie les huit compteurs mémo, en plus du travail
MEB/census effectivement exécuté. Les hits n'ajoutent pas le travail du
calcul ancien. Par ordre actif : lookups = misses + hits, misses = steps,
queries = traces + vertical_descents, queries = insertions + hits − suffix_hits.
Les hits sont au plus les requêtes, les collisions au plus les misses,
les évictions au plus les insertions ; mode désactivé, tous sont nuls.
La comparaison entre modes exige sorties sémantiques égales et octets
égaux dans un même profil. Elle compare séparément les compteurs qui
restent invariants ; pas ceux des descentes évitées.

## Décodage du banc

L'option `--reuse-semantic` est inactive par défaut. Le plan d'ablation
l'active pour éviter de redécoder les mêmes sorties volumineuses.
Chaque essai ouvre son propre fichier et le parcourt intégralement pour
recalculer SHA256 et taille. La clé inclut format, version et empreinte
des sources du décodeur, profil, K, effectif et empreintes XYZ/IDs.
Seule une égalité de cette clé permet de copier un petit résumé déjà validé.
Il s'agit de l'hypothèse d'identité cryptographique déjà employée dans les
reçus, pas d'un nouvel oracle géométrique ni d'une comparaison littérale
avec un ancien fichier conservé.

Tous les événements, paramètres, compteurs, réservations et durées sont
rejugés à chaque essai, y compris leurs liens avec le résumé. Une entrée
n'est publiée qu'après validation complète et nettoyage réussi de l'artefact.
Un échec ou une interruption ne devient pas une origine validée. Le rapport
distingue `decoded` et `reused`, référence l'essai d'origine et publie les
coûts de relecture/hachage et de décodage réellement payés. Le cache possède
au plus64 petits résumés, jamais les payloads, événements ou chronos.
Ce mécanisme peut raccourcir la campagne ; il ne réduit pas le temps FULL.

Les modèles du collecteur couvrent la réutilisation avec véritables petits
payloads, les événements faux malgré un hit, le changement d'entrée, les
copies indépendantes, l'échec de nettoyage et l'interruption du décodeur.
Les portes natives du banc et l'ablation restent à exécuter sur G4.
