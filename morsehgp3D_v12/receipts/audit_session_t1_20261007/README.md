# Audit de la session A, des données et du contrat T1

7 octobre 2026, sources figées à `4147c546000b198b5239646063bfb1e3ed6d28fc`.
Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference` pour l'audit,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Le GPU est celui de la campagne historique du développeur ; aucun appel GCP/GPU,
téléchargement, gros build ni campagne lourde locale n'a été effectué par l'audit.

**La campagne M2 justifie le choix local J3 r168 ; les six défauts nouveaux concernent
les outils de données et de publication. Le contrat T1 a besoin d'un lecteur de transition
qui contrôle aussi les conventions globales de sélection.**

## Preuves et conséquences

| Volet | Résultat et portée | Preuves |
| --- | --- | --- |
| M2 réel | 45 processus, 4 725 durées, 315 identités GPU, 18 contrôles hôte ; J3 r168 choisi, moyenne géométrique 0,1763889694 et pire borne haute IC95 0,2026892599. Bootstrap exhaustif concordant. Noyau seul. | [campagne](campagne/README.md) |
| M6 réel et pilote | Neuf prises, 585 lignes ; premiers usages séparés. Environ 8 µs par lancement synchronisé spin/yield. Le gain yield au premier lancement n'est pas établi par les trois processus. Injection externe : neuf sorties vides donnent aussi `mes_m6_ok` (`CST-0018`). | [M6](m6/README.md) |
| Données | Grille : 28 030 comparaisons exactes réussies. Rejeu, manifeste et découpe : trois défauts reproductibles. Aucun nuage réel relu. | [données](donnees/README.md) |
| Session et publication | 70 contrôles ciblés simulés ; fonctions critiques du contrôleur inchangées après renommage. Fermeture historique cohérente, même génération, état final enregistré TERMINATED. Trois défauts de cache/publication. | [session](session/README.md) |
| Catalogue T1 | J3 commun à essayer sous MES-P et qualification numérique ; lecteur de transition à préciser. Deux modèles exacts complètent CST-0113, sans divergence native alléguée. | [contrat](contrat/README.md) |

Le raccord M2 vérifie les 73 fichiers du manifeste public, les sources de son paquet,
les commandes et les médianes des logs bruts locaux. Les ELF ne sont plus présents :
leurs empreintes déclarées ne sont pas indépendamment rehachées. Compute Sanitizer
couvre 3 000 feuilles d'un cas, six formes ; ce n'est pas toute la campagne.
Les contrôles des juges restent ouverts même quand une campagne réunit manuellement
les preuves manquantes dans leur logique. Aucun résultat faux n'est imputé à la session A.

## Six constats nouveaux

| ID | Défaut au pin | Correction attendue |
| --- | --- | --- |
| CST-0216 | `replay_all.sh` cherche encore ses outils sous `$ROOT/scripts`, avant leur déplacement dans `bench/data`. | Séparer emplacement des scripts et racine des sorties ; tester le pilote déplacé. |
| CST-0217 | Le vérificateur rend code 0 pour `cases=[]` ou un hash de coordonnées nul. | Schéma, sélection non vide et hashes obligatoires avant succès. |
| CST-0218 | `order[:N]` tranche des ex æquo XY : deux sites gardés sur quatre d'une colonne, malgré « toute la hauteur ». | Choisir un rayon puis conserver toute sa frontière ; publier l'effectif réel. |
| CST-0219 | L'identité synthétique dans un nom est publiée et réintroduite dans `SHA256SUMS` après le contrôle. | Contrôler les chemins et le manifeste final, avec collisions explicites si renommage. |
| CST-0220 | Le cache compte comme récupérables des blocs encore retenus par un lien dur ; il évince puis refuse une demande irréalisable. | Distinguer volume logique du cache et espace disque effectivement récupérable. |
| CST-0221 | Un refus d'expurgation supprime tout `--dest`, même préexistant. | Refus avant écriture ou construction dans un répertoire temporaire possédé, puis publication atomique. |

Les témoins ne touchent que leurs répertoires temporaires. Le dernier défaut ne démontre
aucune perte historique. Le cache conserve les octets du lien de build dans le témoin.
Les observations secondaires (plus petit ID source, destination via ancêtre symbolique,
premier essai après échéance du cache) sont bornées dans les rapports, sans nouveau ticket.

## Réponse au développeur pour T1

Accord pour une source J3 commune hôte/appareil, sous mesure du catalogue CPU entier
et des petites chaînes MES-P ; l'écart local ×1,4–1,9 ne décide pas seul. Le repli exact
hôte doit réellement élargir le domaine arithmétique et résoudre les feuilles refusées.
Conserver le DFS v11 figé et l'oracle borné indépendant comme témoins.

Accord pour un lecteur de transition, mais une tolérance limitée aux coquilles à plusieurs
supports ne suffit pas pour FULL. Un triangle équilatéral entier donne trois supports uniques
à même rayon : passer de Morton à l'ordre des positions change Kruskal et le choix `cover`.
Comparer les bijections géométriques exactes, puis recalculer les choix canoniques de chaque
convention ; ne pas accepter seulement une sélection possible. Un modèle G1 à six sites montre
aussi que Morton normalisé peut changer le réservoir et la partition en feuilles. MES-M5 doit
fixer l'ordre parent et distinguer changement de calendrier et changement de départage.
Ces modèles prolongent **CST-0113** ; ils ne démontrent aucun résultat natif faux.

Le contrat doit aligner son palier s17 avec les paliers numériques proposés, conserver le
comptage exact du repli avant admission, compter le front mémoire et publier séparément les
coûts physiques des deux passes. Le détail et les portes proposées sont dans le volet contrat.

## Clôture et rejeu

Chaque volet fournit sa commande légère, ses sources épinglées et ses résultats. Les sorties
données, M6, contrat et M2 sont identiques en normal/`-O`. Le lecteur session utilise `-O`
et contrôle les empreintes avant/après. Les lectures privées sont facultatives pour les témoins
session ; M2 requiert ses petites archives ou le mode explicitement limité `--published-only`.
Aucun contenu de jeu sous licence ni identité réelle n'est ajouté.

`verification.json` conserve les contrôles d'intégration ; `SHA256SUMS` couvre les fichiers
du reçu. Le registre actif porte les six nouveaux constats et les compléments CST-0018/0113.
Les clôtures CST-0209/0210/0212 antérieures sont préservées ; aucun autre constat n'est clos ici.
La session B (`f1ea04c40`) et son addendum (`abe9df451`), postérieurs au pin, sont hors de cette
tranche et restent à contre-lire. Aucun contrat de catalogue, FULL, 100 ms ou multi-séquences acquis.
