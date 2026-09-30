# MMt : endpoint d'unanimité continu hors paramètres recommandés

Petit contrôle causal de la fonction réelle `mmt_point`, extraite par AST
avec `_anc` du snapshot épinglé de `mmt.py` (SHA256
93f6acd0de4146a20bc8078c7d339468a5f118a331e8ac8e39f4107021f52818).
Aucun import du workflow ou du moteur. Les aides QS/Rayon du harnais
utilisent uniquement des rationnels, des classes de carrés et des
intervalles dyadiques exacts ; aucun flottant ne décide une comparaison.

Vraie géométrie : deux sites (0,0,0),(2,0,0), K=2. Gamma_2 possède
un seul sommet et aucune arête ; FULL_2 une unique branche racine, née
au niveau A=1. Chaque point est couvert dès A. Avec eta=2/3, la masse
sur cette branche est s−1 sur [1,5/3], W=2/3, T_half=4/3.
Le supremum analytique est celui de
sqrt(1+eta·theta)−kappa(2theta−1), theta dans ]1/2,1].

- kappa=1/10 : la fonction est strictement croissante jusqu'au bord ;
  vraie date sqrt(5/3)−1/10. La fonction prototype accepte ce paramètre,
  mais retourne sqrt(4/3), car elle omet G(e^-)=W ; son point critique
  s*=25/9 est hors bande. Défaut exact de la forme finie implémentée.
- kappa=2/15 : maximum critique intérieur en s*=25/16, date 139/120,
  retrouvée exactement par la fonction.
- kappa=4 : date sqrt(4/3), retrouvée exactement. Le défaut recommandé
  (4,2/3), et l'hypothèse de stabilité kappa>=sqrt(1+eta), ne sont pas
  réfutés par ce contre-exemple.

Contrôle positif local : modifier uniquement les deux gardes Gm<W de
l'AST pour admettre aussi Gm=W lorsque T1 n'est pas encore défini. Le
cas erroné devient exact et les deux témoins restent exacts. T1 est
important : accepter Gm=W à tous les événements futurs pourrait ajouter
des dates après l'unanimité ; le harnais ne propose pas ce remplacement
aveugle. Ce contrôle n'est pas un correctif intégré ni une qualification
générale du correctif. Alternative : restreindre explicitement le domaine
public des paramètres, ce que la fonction ne fait pas actuellement.

Les commandes normal et -O sont capturées, les sources avant/après
épinglées. Portée : fonction prototype réelle avec aides exactes privées,
preuve géométrique/combinatoire de l'entrée ; pas d'exécution du moteur
natif ni d'import Scene, pas de mesure LiDAR, pas de GPU/GCP, pas de
réfutation du théorème de stabilité dans son domaine.
