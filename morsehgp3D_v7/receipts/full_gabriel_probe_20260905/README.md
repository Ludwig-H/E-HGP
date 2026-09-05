# Admission micro de la sonde FULL horizontale — 5 septembre 2026

Statut : `completed`, 13 commandes closes, sources stables. Ce reçu valide
uniquement la construction de la sonde et six exécutions à **huit points**,
puis six rejets de parsing. Aucun essai à 8 000 points ou plus, aucune campagne
de latence, aucun GPU et aucun sanitizer n'appartiennent à ce reçu.

Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Résultat et portée

- Compilation neuve C++20, `-O3 -DNDEBUG -Wall -Wextra -Wpedantic -Werror
  -pthread`, sans `MHGP7_TESTING` : code 0, stdout/stderr vides.
- Six positifs : `n=8`, `s=8/10/12`, `Kmax=5/10`, codes 0. Les demandes
  `Kmax=10` ferment effectivement les ordres 1 à 8 : le dernier ordre possède
  une feuille, un nœud, aucun parent et aucune connexion de rang supérieur.
- Six rejets : option manquante, doublon, `n=9`, `s=9`, `kmax=0`, option
  inconnue ; codes 2, terminal négatif unique, aucun ordre construit.
- Le juge vérifie le JSONL sans doublons de clés, le terminal unique en
  dernière position, la séquence des ordres, les signatures de budgets,
  les trois grands-livres de masse 28, les compteurs mono-thread, le transfert
  des catalogues et les sentinelles de racine finale/couverture des huit IDs.

Les nœuds par ordre observés sont `[15,22,26,23,19]` pour `Kmax=5` et
`[15,22,26,23,19,12,5,1]` pour `Kmax=10`. Ces cardinalités et les sentinelles
ne prouvent **pas** l'égalité des forêts entre valeurs de `s`. Il n'existe ici
aucun digest d'entrée ou de certificat. Les comparaisons autorisées par cette
sonde portent sur coûts et volumes, pas sur l'égalité des objets.

L'autorité demeure horizontale et relative aux catalogues Gabriel exacts,
complets et réguliers fournis au constructeur. Les contrôles de frontière
réutilisent le chemin produit, mais ne constituent pas un oracle indépendant
de son générateur. Ni tour inter-K intégrée, ni masses, ni archive ne sont
produites. Les lignes d'ordre sont toujours provisoires ; seul le terminal
complet accompagné du code 0 clôt la demande entière.

## Limites et temps

Les plafonds sont fixés avant exécution pour toute la famille : 16 millions
de candidats bruts ; proxy de payload nommé de 8 GiB ; par ordre 8 millions
de records minima+connexions et 8 millions d'aliases, 4 millions de nœuds/lots,
8 millions de références parents, 128 millions de visites de faces et
d'opérations successor, 8 millions de requêtes portail, 2 millions de pas de
chaîne, 4 millions de MEB et 1 milliard de supports MEB/nœuds de requête.
La configuration brute conserve toutes les valeurs et tailles effectives.

Le proxy garde notamment la fusion 2E, le tri, le census et la coexistence
`census + minimum + 2*direct`. Il ne borne pas les capacités STL, les maps,
l'allocateur ou le RSS. La limite distincte `RLIMIT_AS <= 26 GiB` est abaissée
et vérifiée, jamais relevée. Chaque enfant du runner est borné à 60 secondes,
puis son groupe est drainé par TERM/KILL. ASan est exclu de cette sonde parce
que sa réservation d'espace virtuel est incompatible avec cette garde.

Le temps de référence `elapsed_before_terminal_ms` inclut génération de
l'entrée, index, calculs, lecture du résultat, destructions et lignes
provisoires ; il exclut la configuration initiale et l'émission du terminal.
La différence après soustraction des émissions provisoires est explicitement
diagnostique, pas un second chronométrage indépendant. Les temps micro ne
constituent ni un gain attribuable ni une validation du contrat 50k/1s/100ms.

## Reproduction et conservation

La commande exécutée est `python3 -B
build/v7_full_gabriel_probe_20260905/capture.py`. Le script complet figure
dans [capture.py](capture.py), les 13 argv/codes dans [receipt.json](receipt.json).
Le script crée son sous-répertoire `micro` exclusivement : ne jamais rouvrir
ou modifier le reçu fermé. Un rejeu nécessite un nouveau chemin `BASE`
explicitement déclaré et sa propre capture ; ce reçu historique ne change pas.

[raw_streams.json](raw_streams.json) conserve exactement les 26 flux bruts
stdout/stderr sous forme de chaînes UTF-8, y compris les flux vides. Chaque
chaîne réencodée en UTF-8 doit reproduire les octets et le SHA-256 du flux
correspondant dans le reçu. Aucun flux n'a été projeté ou normalisé.
Leur contrôle de copie est conservé dans [copy_check.json](copy_check.json).

[full_gabriel_probe.cpp](full_gabriel_probe.cpp) est la copie exacte exécutée.
Les 51 sources avant/après et les 39 dépendances utilisateur réellement
émises par `-MMD` sont épinglées ; le `.d` exact est conservé. Les en-têtes
système ne sont pas inclus par `-MMD`. L'observation du compilateur et de
l'environnement est honnêtement datée après les commandes, pas rétroactivement
présentée comme une capture avant exécution. Le binaire n'est pas publié ; son
hash dans le reçu identifie le fichier neuf sous `build/.../micro/`.

GCP non utilisé. Aucune branche, aucun commit ni push par cette sous-tâche.
