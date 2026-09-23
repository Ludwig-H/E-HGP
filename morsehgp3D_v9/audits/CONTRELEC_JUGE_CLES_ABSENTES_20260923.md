# Contrelecture de la porte échantillonnée des clés jamais émises

23 septembre 2026. Source publiée par **`683fa46e`** dans
`tests/chain/chain_absent_keys_gate.cpp` ; il s'agit d'une lecture du code
et d'une porte synthétique, pas d'un reçu LiDAR ni d'un verdict global
sur la chaîne.

La direction est utile : le juge forme des supports q2/q3/q4 à partir de
voisins de sites échantillonnés, calcule leur MEB et leur census exacts,
puis cherche leur `BallKey` dans le catalogue. Il peut donc trouver une
clé **jamais émise**, angle mort des contrôles Euler et des recensus de
clés émises. Sa conclusion reste celle d'un échantillon de supports, et
ses primitives MEB/census/coquille sont partagées avec le produit.

Quatre ajustements rendraient sa portée et ses mutants vérifiables :

1. `ball_census(..., interior_cap=Kmax+1, shell_cap=12)` renvoie soit
   `kInteriorOverflow`, soit `kShellOverflow`. Les deux statuts sont
   actuellement ignorés par la même ligne (« outside the window »),
   alors qu'une coquille de 13 sites peut avoir `p=0`, `q_min=2`, donc
   une fenêtre K5 admissible. Exemple entier u18 : centre `(5,5,5)`,
   rayon 5, les six points axiaux `centre±(5,0,0)`, `±(0,5,0)`,
   `±(0,0,5)` et sept des huit points `centre+(±3,±4,0)` ou
   `centre+(±4,±3,0)`. La paire antipodale donne `q_min=2` ; les
   13 points sont sur la même coquille. Le produit **refuse** le domaine
   `u>12`, donc ceci ne démontre aucun défaut du moteur dans son domaine
   déclaré. Compter séparément les deux débordements et annoncer que la
   porte ne juge que `u≤12` empêcherait cependant de présenter ce saut
   comme un rejet géométrique hors fenêtre.
2. Le commentaire dit retirer une clé « parmi celles que l'échantillon
   atteint », mais le code retire chaque 61e clé du **catalogue entier**,
   seulement pour K5, et exige une seule détection cumulée sur les trois
   familles. Une clé répétée dans les supports peut satisfaire ce plancher
   à elle seule. Recueillir les **clés admissibles distinctes** du juge,
   retirer au moins une clé de cet ensemble pour chaque famille et pour
   K5 **et K10**, puis exiger sa détection par cas rendrait la mutation
   causale. Un plancher d'admissibles par famille/K éviterait qu'un seul
   cas porte tout le total.
3. `run_tower=false` qualifie seulement la présence de clés. L'omission
   d'une clé de couche haute peut être acceptée par FULL tout en changeant
   son digest ; comparer séparément le payload FULL sain/amputé, champ par
   champ, reste nécessaire pour juger la tour et le témoin de première
   cofacette. Cette porte échantillonnée ne doit pas recevoir la portée
   d'un certificat global de complétude.
4. Le cas rapide 2k n'exerce **aucune** coquille étendue dans les six
   familles/K (rejeu local du binaire `build/v9-exp`, `extended=0` dans
   chaque ligne). Le développeur rapporte seulement **deux** cas étendus
   sur l'ensemble de la porte 8k. Ajouter une fixture ciblée `u≤12` avec
   `q_min<|U|`, une mutation de sa clé, et un plancher dédié permettrait
   de juger effectivement la branche `ShellTable`, séparément des
   familles synthétiques ordinaires.

Ces corrections gardent le juge léger et ciblé : elles ne demandent ni
génération exhaustive des supports ni déplacement de la priorité D5.
