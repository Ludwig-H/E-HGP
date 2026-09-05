# Poids du manuscrit : supplément d’incidence

Les formules du §9.1 et l’Algorithme 1 (PDF 122–126) portent sur les facettes des cofaces contributrices **avant** réduction par arbre couvrant. Elles ne se déduisent ni des seuls parents FULL ni des seules feuilles minima. Le [dossier principal](../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md#4-tour-et-poids--deux-réserves-différentes) expose maintenant cette distinction. `public_status=not_claimed`.

## Univers et conservation

Fixer le catalogue C de cofaces, ses identités sans doublons, F l’univers des facettes pondérées, la fonction ψ positive et finie sur les rayons contributifs, le rayon/unité, l’horizon et les conventions zéro/bruit/départage. Le profil direct du manuscrit prend F=∂C, avec C Gabriel ; ce n’est pas Čech exhaustif ni un sous-flot auxiliaire de portails. Une arête non-Gabriel peut appartenir à F : le triangle obtus du [reçu rationnel](receipts_vertical_20260905/masses/normal.json) l’impose.

$$S_\tau=\sum_{\sigma\in C,\ \tau\subset\sigma}\psi(\rho(\sigma)),\qquad T_x=\sum_{\tau\in F,\ x\in\tau}S_\tau,\qquad w_{x\tau}=S_\tau/T_x\quad(x\in\tau,\ T_x>0).$$

Le poids vaut zéro sinon. La masse d’une facette est `mτ=Σx∈τ wxτ`. En échangeant les sommes finies, `Στ wxτ=1` sur `X+={x:T_x>0}` et `Στ mτ=|X+|`. Si F=∂C, chaque coface contenant x possède K facettes contenant x, donc :

$$T_x=K\sum_{\sigma\in C,\ x\in\sigma}\psi(\rho(\sigma)).$$

Cela évite un second index global point–facettes pour T. Les scores S restent nécessaires. Une restriction supplémentaire de F invalide cette simplification.

Les masses de nœuds s’additionnent sur des feuilles représentant F ou sur une affectation certifiée de ces facettes ; les seuls minima FULL ne fournissent pas cette affectation. Une antichaîne ne conserve toute la masse que si elle couvre toutes les feuilles positives ; son complément doit rester en réserve ou en bruit. Recalculer T à chaque coupe définit une autre mesure : les anciennes masses changent. K1 peut attribuer directement masse 1 à ses racines sans coface, sous une convention explicite.

## Payload suffisant et décisions encore distinctes

Un histogramme `Hτ,λ` des incidences par facette et niveau carré suffit aux scores pour toute ψ ultérieure. À ψ figé, S peut suffire. Il faut agréger les directes avant les chaînes silencieuses, ou certifier leur origine : `build_render(events)` agrège le multiensemble reçu et ne certifie ni sa complétude ni sa déduplication. Les callbacks provisoires ne deviennent pas une archive de mesure.

La [contre-fixture des deux tétraèdres](receipts_vertical_20260905/README.md) donne, avec ψ=ρ^-2, les mêmes tokens horizontaux et des votes opposés ; cette fixture p2 est distincte du comparateur p3. Elle interdit d’inférer les incidences depuis le quotient de connectivité. L’affectation temporelle est également distincte : dans E5, reporter la masse d’AC de 33/2 à 83886/3563 change une coupe pondérée sans changer sa couverture. La « première incidence Gamma » n’est pas une prescription explicite de l’Algorithme 1.

Pour `T_x>0`, le vote souple est `p(x)=Στ wxτ pτ` ; le vote dur compare les sommes par classe et n’applique le départage qu’en cas d’égalité certifiée. La classe bruit et `T_x=0` doivent être déclarés. L’[autorité p3](AUTORITE_VOTE_P3_COURANTE.md) ferme les **numérateurs** ; les quotients de masses, la condensation et les probabilités exportées conservent leur propre problème numérique.

L’intégration pondérée est différée ; ses choix encore ouverts sont regroupés dans [les questions secondaires](QUESTIONS_SECONDAIRES.md). Les preuves et résultats négatifs restent dans les [reçus originaux](receipts_vertical_20260905/README.md).
