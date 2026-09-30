# MMt : une vraie masse de branche excède i128 sur u18

Petit contrôle autonome Fraction, sans import du moteur ni exécution native.
Quatre sites positifs u18 forment un tétraèdre dont les quatre poids
barycentriques du centre circonscrit sont strictement positifs. Le script
vérifie les poids, les quatre rayons égaux et les coordonnées. Cette boule
est donc la MEB : la moyenne pondérée des distances carrées à tout autre
centre vaut β plus la distance carrée entre les centres, et ne peut être
strictement inférieure à β pour tous les sommets.

Son niveau réduit β a un numérateur de 142 bits et un dénominateur de
107 bits. Il n'est pas entier/4. Pour K=4 sur ces quatre sites, Gamma_4 a
un seul sommet, et FULL_4 une seule branche racine. Elle couvre chaque
point dès A=β. Avec η=2/3, la vraie masse MMt de cette branche est
W=ηA : son numérateur réduit a également 142 bits, son dénominateur
108 bits. Il ne s'agit pas d'une somme artificielle de rayons indépendants.
L'obstruction apparaît avant toute accumulation multi-branche.

Le test contrôle aussi T_half=A+W/2 et que le point critique pour κ=4
est avant la bande. La date de cette branche seule est sqrt(T_half).

Portée : réfutation géométrique de « niveaux entiers/4 » et « masses en
i128 suffisant sur u18 » dans le plan MMt. Ce n'est ni une corruption
observée du moteur publié, ni une mesure LiDAR/G4, ni une évaluation
statistique de MMt. Le stockage de Level actuel utilise déjà des entiers
plus larges ; le futur port des masses doit conserver cette exactitude.
