# Bord de bande dure — preuve privée Fraction

Périmètre : minuscule contrôle mathématique CPU/Python, aucune importation du
moteur, aucune exécution native ou GCP, aucune modification des sources partagées.
Tous les sites sont collinéaires en 3D ; on utilise leurs abscisses rationnelles.
Un facteur entier commun et une translation les placent, séparément pour chaque
fixture, sur une grille entière u18. Le passage à la limite e→0 est géométrique,
pas une affirmation d’instabilité au-dessous du pas d’une grille fixe.

Les cinq sites sont x=0, C1=100, C2=110, D1=-110, D2=-125+e.
K=2, eta=1/4, alpha(x)=50. Chaque paire vote à la moitié de sa
distance ; la bande est fermée, et la majorité strictement supérieure à W/2.
Si e<0, W=3 et x rejoint C à 55. Si e>=0, W=4 et x attend 105.
Les composantes FULL2 et leur fusion 105 sont recoupées par deux voies exactes :
Gamma2 exhaustif (toutes paires et toutes cofaces triples) et union des intervalles
fermés où au moins deux sites sont à distance <=r.

La fonction réelle majorite_virtuelle, _exiger et ErreurPaires sont extraites AST
sans modification du snapshot intégral épinglé paires_snapshot.py. Seul son
contexte de forêt est un adaptateur indépendant Fraction, pas la forêt native.
Les votes sont préparés indépendamment de PairesK2.lignes/PairResolverK2 : cela
qualifie la règle réelle de majorité sur ces votes, pas le raccord natif.

Le contrôle compare les dates, toutes les hauteurs de réunion et les composantes
à toutes les coupes événement et intermédiaires. Fixtures : e=0 et e=±1/m,
m=1,2,10,100,1024 ; contrôle positif eta=1/8 (pas de saut). Deux mutants causaux
sont rejetés : >= au lieu de > pour la majorité, et bande ouverte au lieu de fermée.
Aucun assert : les contrôles fonctionnent aussi avec Python -O.

record.py capture les pins partagés avant/après, les sorties et codes normal/-O,
puis un manifeste. read.py refuse des captures incomplètes/modifiées et rejoue
le code autonome épinglé. Le manifeste protège les fichiers, pas sa propre
authenticité cryptographique ; le SHA du manifeste est communiqué séparément.
Le lecteur exige --manifest-sha256 SHA_ATTENDU et le contrôle avant le chargement
du manifeste et avant tout rejeu. Il vérifie les sept fichiers réels du paquet,
puis tous leurs pins après les replays, pour garantir une lecture sans modification.
Cette version r2 préserve intégralement le premier paquet fermé dans
/tmp/hard-band-border.1hSd9lO2 ; seuls README/record/read sont renforcés ici.

Conclusion : le contact particulier déjà passé par les paires ne prouve pas une
stabilité générale à univers de votes figé. Ce paquet n’est pas un test de MMt,
ne contredit pas ses théorèmes sous leurs hypothèses et ne tranche pas entre la
cible statistique de masse et un objectif de continuité globale.

La version u18 prend M=1024, e=±1/M et translate de 125M+1.
Elle déplace le seul dernier site de 0 à 2 ; tous les sites sont <=240641.
La réunion de x avec C1 passe de 56320 à 107520 (saut51200).
Le site C1 est déjà attaché à C2 dès5120. À55M, le vote x,D1 reste
dans sa propre composante, non encore réuni à D1,D2 avant(125−e)M/2.

Deux essais de préflight non qualifiés ont échoué sur le garde de mutation :
il attendait deux puis trois occurrences textuelles, mais le snapshot intégral
en a quatre (dont deux fonctions non compilées par ce paquet). Le garde a été
corrigé à quatre ; seules les deux occurrences de la fonction AST extraite
ont un effet. Ces erreurs du contrôleur ne sont pas des défauts géométriques
ni des essais normaux/-O conservés dans la capture faisant autorité.
