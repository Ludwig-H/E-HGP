# Généraliser le complexe alpha à l'ordre K : réponse courte

7 octobre 2026. Exploration seulement : rien n'est qualifié, GCP non utilisé. Détail et preuves : [SYNTHESE.md](SYNTHESE.md).

**Que garder au rayon r ?** À l'ordre K, on garde les morceaux de la mosaïque de Delaunay d'ordre K dont le rayon de naissance ne dépasse pas r. Cette mosaïque est exactement la tranche de profondeur K du pavage rhomboïdal, et aussi le Delaunay pondéré des barycentres de K points (votre idée tirée de BCY). Une condition : dater chaque morceau par le rayon du K-ième voisin, pas par la distance pondérée ; sinon on obtient la hiérarchie de la distance à la mesure, qui fusionne trop tôt. Pour K = 1, c'est exactement le complexe alpha.

**Pourquoi c'est le bon objet.** Il a les mêmes morceaux et les mêmes trous que la région dense. Il couvre exactement les mêmes points que le K-polyèdre de la thèse. Il retrouve les nœuds de la tour sans aucun écart, vérifié nœud par nœud. On rattache un morceau à un nœud par ses groupes de K points, jamais par sa position.

**Ce qu'il ne faut pas prendre pour la forme.** Les supports des boules ne sont que le squelette des fusions, l'analogue de l'arbre couvrant minimal : dessinés, ils remplissent les roues. La somme de Minkowski avec la boule n'est qu'une enveloppe : elle bouche des trous, en crée, et deux enveloppes qui se touchent n'annoncent qu'une fusion à venir.

**Robustement : ce qui tient.**
- Les naissances et les fusions bougent au plus autant que le bruit : démontré, et mesuré dans 64 cas sur 64.
- La forme à un rayon éloigné des événements bouge d'environ une fois le bruit (médiane mesurée).
- Un nœud garde son identité s'il vit plus de deux fois le bruit.

**Robustement : ce qui ne tient pas.**
- Le dessin exact à un rayon critique peut sauter sous un bruit minuscule, sans que les trous changent.
- À K = 5 sur trois trames LiDAR, 61 à 67 % des nœuds vivent moins de deux fois le bruit d'arrondi au millimètre.
- Un seul point aberrant proche peut créer un morceau : la protection contre les aberrants se lit en descendant d'un ordre, jamais à K fixé.
- Une décimation sans poids change l'ordre effectif.

**Les niveaux.** On range chaque morceau une seule fois, avec sa date et le nœud où il apparaît ; tout niveau d'un nœud devient une simple requête. Pour chaque nœud on publie sa naissance, sa mort, ses paliers (les plages de rayon où ses trous ne changent pas) et un rayon intérieur avec ses marges. Entre deux ordres, on garde un dessin par ordre et le lien de la tour, jamais une inclusion des dessins.

**Ce qui fait reconnaître un objet.** C'est le rayon choisi le long de la chaîne d'ancêtres, pas le type de dessin. Sur le vélo synthétique à K = 5, la roue est un anneau à 47 mm, le vélo (deux anneaux, cadre, selle, guidon) apparaît à 77–89 mm, puis les roues se remplissent. Ces nœuds vivent moins d'un demi-millimètre. Le vélo réel de la trame 08/002852 n'est reconnaissable à aucun K.

**Est-ce petit ?** Non : environ 1 000 faces par point à K = 5. Une réduction certifiée garde les mêmes trous avec 2 à 7 fois moins de morceaux, mais n'allège pas le dessin et peut faire croire à une roue ouverte. Le tout ne tient pas dans les 100 ms : il faut calculer la forme après la tour, à la demande, sur quelques objets, en commençant par K = 2.

**Images à regarder** (dans build/v11-persist/polyedres_ordre_k/) :
- experience_rendu/png/synth_velo_05m_k5_chaine_vélo_roue_avant.png : la chaîne roue → vélo ;
- experience_rendu/png/candidats_synth_velo_05m_objet0.png : tous les dessins au nœud « vélo » ;
- experience_robustesse/png/planche_synth_anneau_perce_05m_k2_sigma10.png : rayon critique contre rayon intérieur sous bruit ;
- experience_echelle/png/planche_decoupe_08_002852_deux_velos_6_51_instances_k5_objet0.png : le vélo réel.

**Ouvert.** Un dessin à la fois petit, prouvé et rapide ; une identité stable pour les objets qui vivent peu ; les retours LiDAR fusionnés par l'arrondi. L'auditeur n'a pas répondu depuis sa note du 6 octobre ; huit questions lui sont préparées.
