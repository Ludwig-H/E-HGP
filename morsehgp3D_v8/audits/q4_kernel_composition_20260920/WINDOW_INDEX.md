# Fenêtre exacte et objets d'un futur index de couches

Réponse indépendante à la question constructeur30, après31b0243a. La fenêtre
est démontrée ci-dessous ; l'index est une proposition d'architecture, sans
implémentation ni coût de recherche qualifié dans cet audit.

## Fenêtre fermée, rangs pondérés et coquilles

Sur une famille affine, noter E les racines de sortie, I les racines d'entrée,
avec une occurrence par ID ; c compte seulement les formes constamment
strictement intérieures. Les constantes nulles appartiennent à toute coquille.

$$p(\mu)=c+\#\{e\in E:e>\mu\}+\#\{i\in I:i<\mu\}.$$

Poser T=K−2. Si c≥T, rejeter la famille. Sinon H=T−c≥1 ; L est la H-ième
sortie décroissante, U la H-ième entrée croissante. Prendre L=−∞ si |E|<H
et U=+∞ si |I|<H, y compris lorsqu'un type est totalement absent.
Si μ<L, H sorties sont encore strictement intérieures ; si μ>U, H entrées
le sont déjà. Donc tous les centres admissibles sont dans [L,U]. L>U rejette,
mais **L=U doit être conservé** : il peut porter la seule racine admissible.

Il y a au plus H−1 sorties strictement au-dessus de L, et au plus H−1 entrées
strictement au-dessous de U ; les cas de borne infinie satisfont la même
conclusion par leur population totale. Il reste donc au plus2H−2 **IDs**
d'événements dans ]L,U[. Ce n'est une borne ni sur les coquilles aux bornes,
ni sur les constantes de coquille. Une borne peut réunir arbitrairement de
nombreux IDs, des deux types. Tous doivent être retrouvés et groupés par
égalité exacte, indépendamment du départage utilisé dans les tas de sélection.

Les groupes coïncidents portent des poids : le rang H est un rang d'IDs,
pas de groupes ou de valeurs distinctes. Des quantiles non pondérés peuvent
seulement élargir la fenêtre ; ils ne réfutent pas nécessairement son certificat
extérieur, mais détruisent la borne2H−2 sur les IDs strictement entre les bornes.
Le gate réfute précisément cette propriété, sans leur attribuer une fausse
perte de complétude. Une déduplication qui perd les poids dans le census
serait, elle, une erreur d'exactitude.

Pour balayer la fenêtre seule, les entrées strictement sous L et les sorties
strictement au-dessus de U deviennent des contributions constantes intérieures.
Les autres événements extérieurs ne contribuent pas. Initialiser avec ces
constantes et les sorties retenues, puis retirer sorties, lire profondeur
stricte/coquille, ajouter entrées à chaque groupe. La profondeur exacte reste
à vérifier dans la fenêtre : sa présence seule ne signifie pas p<T.

## Ce que l'index doit posséder

Garder chaque frontière convexe de29 dans son ordre cyclique, tous les points
colinéaires de bord, les coordonnées dupliquées et leurs listes d'IDs.
Des poids/préfixes permettent des rangs d'IDs sans développer chaque doublon.
Le reste dégénéré conservé par29 a sa propre représentation de point ou segment
ordonné. Toutes les couches des deux signes interviennent ; les voisins de la
seule couche contenant la seed ne décrivent pas les autres populations.

Dans le diagramme dual fini, écrire p_x=(u₀,v₀), p_z=(u,v), avec v₀≠0 après
échange éventuel des axes dans toutes les formes. Le paramètre suivant est ξ ;
le passage au paramètre μ du moteur doit conserver son orientation affine.

$$r(p_z)=\frac{v-v_0}{uv_0-vu_0},\qquad \text{pente sur la seed}=\frac{c_z(uv_0-vu_0)}{v_0}.$$

Cette projection fractionnaire est vue depuis p_x. Pour découper la frontière,
les événements géométriques utiles sont les tangences depuis p_x, la droite-pôle
passant par0 et p_x, et p_z=p_x. Entre ces coupures, les arcs convexes fournissent
des suites monotones ; le signe de pente fixe le type entrée/sortie. Un bord
tangent peut être un plateau complet. Si la seed est sur la frontière ou si
la couche est colinéaire, traiter explicitement le point confondu, les contacts
et les deux côtés ; aucune perturbation ne doit supprimer leurs égalités.

Au pôle, la restriction est constante : p_z=p_x donne une coquille constante,
les autres contacts donnent une constante intérieure ou extérieure selon son
signe exact. Ces populations déterminent c avant de connaître H. Les formes
c_z=0 ne sont pas des points duaux finis : conserver un flux séparé, avec repli
exact ou futur index angulaire des normales **orientées**. Les formes globalement
nulles, dont a,b, donnent la coquille commune. Aucun signe opposé n'est annulé.

La requête envisagée fusionne les extrêmes pondérés de toutes les chaînes,
puis récupère les plateaux complets aux bornes L,U et les constantes de coquille.
Les égalités peuvent traverser plusieurs chaînes et couches ; le regroupement
numérique exact reste global à la seed. Cet objet peut éviter les scans par
face, mais recherches de tangences/pôles, comptage pondéré, extraction des
plateaux, tri final des IDs, coquilles et construction partagée sont à payer.
Aucune recherche logarithmique ni borne globale n'est revendiquée.

[window_gate.py](window_gate.py) confronte les fenêtres à des événements
rationnels avec multiplicités, constantes et coquilles, dont L=U, L>U et
bornes infinies. Il vérifie aussi la formule duale et le signe des pentes.
Il ne construit ni index de frontières ni géométrie q4 issue de nuages réels.
Les contrôles de ce modèle ne sont pas ceux du port constructeur30.
