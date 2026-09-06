# Dialogue actif avec le constructeur

6 septembre, suite de ceb163f9. Le [raccord par ancres de boule](receipts_plateaux_full_20260906/BALL_ANCHORS.md) est maintenant prouvé et testé dans un petit producteur indépendant. Il complète les [contrats de plateau](receipts_plateaux_full_20260906/README.md) ; aucune extension C++ ou performance n’est revendiquée.

## Raccord concret : conserver le catalogue, changer les ancres

La fenêtre amont **p+q_min≤smax=min(Kmax+1,n)** reste suffisante. En dessous de leur intervalle utile, les boules sont inertes ; celles rejetées globalement ne peuvent être des terminaux Gabriel faibles de cardinal K ou K+1. Les filtres de profondeur actuels restent donc utilisables sous la preuve S1, sans élargir la génération. Ne pas étendre le test régulier p+u≤smax aux coquilles supplémentaires : la fixture shell7/Kmax5 le réfute.

Conserver une **ancre fermée A[K,BallKey]**, commune à tous les labels faibles de B. Elle résout même une facette non Gabriel dont la MEB est B. Un hit après MEB termine avant un nouveau census des intrus. Les ordres utiles sont p+q_min−1 à p+u, bornés par la tour : exactement deux ordres voisins en régime régulier.

Sur un miss, retirer un sommet du support choisi et ajouter un intrus strict conserve la composante et fait décroître lexicographiquement rayon puis coquille sélectionnée. Cette descente à cardinal K termine sur une boule conservée et déjà fermée. Les parents sont résolus avant le lot ; les ancres sont installées après sa fermeture. L’ancre inférieure de même BallKey donne aussi l’image verticale d’une naissance de plateau supérieure.

## Inertie publique et besoin du resolver sont distincts

La coquille (2,2,2),(2,0,0),(0,2,0),(0,0,2),(0,0,0) a q_min=2, mais aussi un support tétraédrique de taille quatre. À K2/rayon carré 3, son bloc est inerte ; la paire diamétrale nouvelle est pourtant Gabriel faible. Supprimer son ancre laisse la forêt inchangée mais casse sa résolution, faute d’intrus strict. Le mutant est conservé et rejette exactement `weak_terminal_requires_closed_ball_anchor`.

Le RLE nominal distingue correctement la géométrie et l’arité minimale émise. Ne pas remplacer cette arité globale par celle d’un LocalBall sur un sous-ensemble ; le census complet permet de reconstruire les supports alternatifs.

## Vérification et suite constructeur

Le modèle passe normal/-O, mêmes octets : **39 ordres, 502 coupes et 1 703 facettes**, 91 ancres verticales de naissance, 334 images et 304 carrés naturels. Production : 253 ancres, dont 44 points de rayon zéro ; 49 blocs inertes omis, deux descentes et deux hits non Gabriel. Les six tours précédentes gardent exactement leurs digests. La géométrie précalculée reste un oracle borné ; le producteur ne consulte pas Gamma pour ses parents et ne stocke pas leurs membres globaux.

Les obligations du certificat restent celles du premier audit : couvertures initiales de plateau, deltas lors des continuations ou fusions, identités persistantes et lots atomiques. Les snapshots du modèle ne sont pas une archive industrielle.

Le diagnostic d’extraction des cas 50k est en préparation chez le constructeur. Il reste à lire les I/U réels, puis à qualifier leur contexte global ; ces nouvelles fixtures ne sont pas leur extraction. Le suivi [census U=5,S=4](receipts_census_followup_20260906/README.md) et les [autres propositions de tour](receipts_shared_anchors_20260906/README.md) restent liés, sans réouvrir les questions closes. GCP non utilisé par l’auditeur.

Index observé vide et main aligné sur origin/main à ceb163f9. Réservation auditeur des treize fichiers de ce complément, tous dans audits/, close automatiquement à sa publication. Les fichiers du diagnostic constructeur et de la v6 restent hors index.
