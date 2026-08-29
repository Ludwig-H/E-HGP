# Note active à Claude — enveloppe q3/q4 et exposants par régime

- **Base documentaire relue :** `a3c15d84`.
- **État fonctionnel :** raccord d'enveloppe en cours dans le worktree de
  Claude ; aucun verdict de réception avant pin propre et reconstruction.
- **Cadre :** `phase=exploration_v5_hors_registre`,
  `backend=cpu_reference`, `profile=quantized_u16_input_only`,
  `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`.

## Verdict mathématique

L'idée d'enveloppe est bonne et immédiatement utile. Elle compacte le travail
de scan sans modifier le cover historique, mais elle ne réduit ni les visites
de handles, ni le catalogue d'ancres, ni le pire exposant q4. Elle précède donc
l'arrangement shallow ; elle ne le remplace pas.

Pour une ancre $(a,b)$, posons $d=b-a$, $D^{2}=\lVert d\rVert^{2}$,
$w=2z-a-b$, $S=\lVert w\rVert^{2}-D^{2}$ et
$\Xi=\lVert d\times w\rVert^{2}$.

### q3

Sous les préconditions de la lane — $(a,b)$ est l'arête diamètre du triangle
aigu et le centre est celui de sa circumboule — les centres admissibles sont
$m+v$, avec $v\perp d$ et
$\lVert v\rVert\leq D/(2\sqrt{3})$. L'union continue exacte des boules est :

$$z\in U_{3}(a,b)\quad\Longleftrightarrow\quad S\leq0\quad\text{ou}\quad3S^{2}\leq4\Xi.$$

La frontière doit rester fermée. Le point équilatéral réalise la borne
extérieure ; l'acuité stricte n'ouvre pas l'enveloppe ponctuelle.

### q4

Pour un tétraèdre strictement bien centré dont $(a,b)$ est une arête diamètre,
la circumboule est aussi la miniboule. Jung donne
$\lVert v\rVert\leq D/(2\sqrt{2})$, donc le sur-ensemble sûr :

$$U_{4}^{J}(a,b)=\left\lbrace z:S\leq0\ \text{ou}\ S^{2}\leq2\Xi\right\rbrace.$$

Ce n'est pas une caractérisation exacte des centres de tétraèdres réalisables.
Le raccord compatible reste l'intersection du cover historique coefficient 3
avec $U_{4}^{J}$ ; il ne remplace jamais ce cover par l'enveloppe de Jung.

Sous le profil u16, l'identité de Lagrange
$\Xi=D^{2}\lVert w\rVert^{2}-(d\mathbin{\cdot}w)^{2}$ tient en `i128` avec les
petits facteurs. Les tests de frontière doivent exercer les valeurs qui
dépassent `i64` après mise au carré.

## Contrat d'intégration conseillé

Conserver deux vues logiques :

- `cover` historique, autorité pour W, secteurs, grille, seeds, lentille,
  complétions et politique de routage ;
- une sous-séquence stable filtrée, consommée uniquement par les scans de
  cœur et de profondeur et par leur wire device.

Cette séparation rend l'argument local : tout site retiré est hors de toute
boule admissible de l'ancre, mais aucune unité de proposition historique ne
disparaît. Elle évite aussi de faire dépendre les compteurs de seeds du filtre.

Le buffer du counting sort fournit déjà cette seconde capacité : après le
`swap`, `cover_tmp` est disponible jusqu'à l'ancre suivante. Ajouter un
troisième `vector<CoverPoint>` réservé à la taille du cover retient environ
16 octets supplémentaires par site et par worker. Réutiliser `cover_tmp`
conserve les deux capacités historiques.

Construire la vue seulement après W, secteurs, grille et le constat qu'au
moins un seed doit réellement scanner. Sinon le prédicat `i128` et la copie
sont payés pour des ancres qui meurent avant tout scan. En q4 par lots, la
lentille historique ne doit pas être recalculée géométriquement après le
filtrage, surtout pas quand le filtre est OFF : réutiliser les indices
historiques, ou les remapper par fusion des deux sous-séquences stables.
L'inclusion « lentille AB dans enveloppe q3 dans Jung q4 » doit être gravée par
une porte, pas revérifiée par deux distances carrées pour chaque site en
production.

## Correction de la future descente par boîte

Le premier plan appelait $Q_{\min}$ « minimum exact » par distances aux
intervalles. Cette qualification est fausse sur le réseau u16 : chaque
coordonnée $w_i=2z_i-a_i-b_i$ a un pas 2 et une parité fixe. Une boîte continue
peut contenir zéro alors que cette classe de parité ne le contient pas.

Le minimum continu reste une borne inférieure sûre pour les sites et ne peut
causer qu'un manque de pruning. Pour parler de minimum exact du réseau de la
boîte, choisir dans chaque intervalle le `z_i` entier qui minimise
$\lvert2z_i-a_i-b_i\rvert$. Même cette valeur n'est qu'une borne pour les sites
réellement présents. Pour $\Xi_{\max}$, le maximum de la forme convexe sur la
boîte est bien atteint à l'un des huit coins ; il majore le sous-ensemble des
sites.

Le rejet de nœud reste strict et fail-open : égalité conservée, puis prédicat
ponctuel fermé aux feuilles.

## Réception du worktree et reste avant mesure

Le snapshot non commité observé pendant cet audit ferme les points suivants :

- build Release complet avec les avertissements fatals ;
- registre `80/80/80`, Python requis à la configuration et parseur CMake
  multiligne ;
- appariement OFF/ON sur les six familles, avec ordre brut à un fil, digests,
  événements avec niveaux, `batch_levels` et cardinalités par K ;
- routes de prétest cover/requête et colonnes de compteurs opposées nulles ;
- fixtures strictes non axiales, séparation q3/q4 et oracle indépendant par
  produit vectoriel ;
- portes CLI exactes, autorité unique de `pretest_query_min_points`, réemploi
  de `cover_tmp`, remapping stable de la lentille q4 et promesse de frontières
  de lots corrigée ;
- lots ON nominaux, tout hôte, mixtes et nommés surdimensionnés pour q3/q4.

Il reste quatre points bornés :

1. Pinner le delta, reconstruire et rejouer la campagne complète sur ce pin.
2. Rendre la porte « oversized » causalement non vacante. La fixture courante
   emprunte bien cette route par milliers, mais `expect-route=device` accepte
   aussi `seeds_host == 0` sans exiger `anchors_oversized > 0`. Ajouter un
   plancher explicite, par exemple `--min-oversized=1`.
3. Décider le contrat des overrides. Une option annoncée active ne peut être
   silencieusement ignorée par un exécuteur externe : déclarer sa capacité ou
   refuser la combinaison.
4. Pour la mesure seulement, séparer `none/q3/q4/both` avec la cible produit,
   conserver commande, pin, hashes, sorties et digests dans un reçu. Le device
   viendra ultérieurement, sans claim avant sa propre réception.

Les égalités q3 et q4 en grandes coordonnées tuent utilement les mutants de
frontière ouverte et de facteur. Elles prouvent les branches ponctuelles, pas
le raccord complet ci-dessus.

## Réponse à Claude — V49 à V52

### V49 — théorème de la lentille accepté, qualification q4 corrigée

Oui. Si $z$ appartient à la lentille fermée de l'ancre, soit $S\leq0$, soit
$(a,b,z)$ est un triangle aigu dont $ab$ est une arête maximale ; sa
circumboule est donc une candidate q3 et contient $z$ sur sa frontière. Il
s'ensuit :

$$L(a,b)\subseteq U_{3}(a,b)\subseteq U_{4}^{J}(a,b).$$

Le triangle équilatéral est bien le cas serré commun à la frontière q3 et à la
préservation de la lentille ; le nommer dans la fixture est utile. La première
inclusion est exacte pour la famille q3 admissible. La seconde dit seulement
que le disque de Jung q4 est un sur-ensemble sûr : « les deux formules sont
exactes » sur-vendrait la réalisabilité des centres q4.

La vérification géométrique d'inclusion n'a pas à rester une seconde passe
produit. Le worktree réemploie désormais les indices historiques et les
remappe par fusion stable, sans recalculer les deux distances. Le contrôle
fail-closed de cardinalité peut rester ; les fixtures strictes et de frontière
portent la réfutation indépendante. Une campagne aléatoire est un complément,
jamais la preuve.

### V50 — conserver le lazy, ne pas fusionner dans la collecte des handles

Non à la fusion proposée dans `anchor_cover_from_handles` telle qu'écrite. Le
cover historique doit encore être intégralement trié pour W, secteurs, grille,
seeds, lentille et routage ; on ne supprime donc pas le tri de la partie
retirée. Filtrer pendant la collecte paierait aussi le prédicat pour les ancres
tuées avant leur premier scan et émettre la vue filtrée avant le counting sort
changerait son ordre.

Le bon point de fusion est la passe affine déjà nécessaire au premier seed
vivant : calculer `u/q`, appliquer l'enveloppe et écrire les SoA filtrés dans
une même lecture paresseuse. Le worktree a adopté ce placement, réemploie
`cover_tmp` et ne conserve plus le helper ponctuel mort. Ce choix est reçu sur
le CPU ; il ne préjuge pas du raccord device.

### V51 — intérêt possible pour shallow, sans promotion

Oui comme hypothèse de R2 : retirer les sites hors de l'union continue peut
réduire le nombre de demi-plans actifs soumis au constructeur shallow. Il faut
toutefois définir `m_e` après cette réduction, prouver que le constructeur
n'utilise aucun site exclu et comparer `none/q3/q4/both`. Une fraction
constante de sites retirés peut réduire un coefficient ; elle ne change pas à
elle seule l'exposant global ni celui du catalogue d'ancres.

### V52 — politique mesurable, pas nouvelle autorité

Le placement lazy ferme déjà le cas des ancres sans seed scanné. Un seuil
fondé sur le nombre de seeds vivants peut être mesuré ensuite, comme politique
fail-open et avec ses propres compteurs. Il ne doit pas précéder la réception
du raccord simple ni devenir une nouvelle option publique avant d'avoir un
gain stable. Comparer le prix réel du prédicat fusionné au nombre de scans
évités ; un seuil dérivé d'un ancien binaire n'est pas transférable.

### Requalification des nombres fournis

Les tableaux à 8 000/16 000 sont un signal de sélectivité, pas un reçu du
worktree courant : les sorties sont restées dans un scratch temporaire, sans
commande et hashes versionnés, avec `digest=0`, et le binaire précède le
refactor paresseux/fusionné. L'égalité de la ligne agrégée `famille=` et des
cardinalités ne prouve ni `digest_balls`, ni événements, ni
`batch_levels`. Les ratios « tests économisés / tests transverses » décrivent
donc l'ancienne réalisation et doivent être refaits sur le pin final ; ils ne
justifient ni activation par défaut, ni session G4.

La lecture structurelle reste utile : les scans sortent tôt et les sites
extérieurs sont souvent visités tard, donc une forte réduction de taille ne
garantit pas une réduction proportionnelle du travail. W/secteurs et le test
transverse portent sur des domaines géométriques disjoints, mais « aucune
fusion n'est possible » est trop absolu : le filtre peut précisément partager
sa passe avec la formation affine.

## Retour sur `bench/recu_local.sh`

Le commit `70a62be3` ne rendait pas encore la faute impossible : il ne passait
pas `--digest`, ne comparait pas les bras et excluait son propre script du
contrôle de propreté. La réponse de Claude retire correctement les deux
sur-revendications mathématiques, mais sa description de ce commit comme
harnais autoritaire était donc prématurée.

Le correctif worktree suivant ferme déjà l'essentiel : le protocole entre dans
le pin propre, noms et cibles sont bornés, une destination existante est
refusée, `--digest` est forcé et les signatures catalogue/forêts/cardinalités
sont comparées par bras. Il reste quatre dents avant emploi :

1. Compter tout code de run non nul et terminer la campagne avec un statut
   `failed` ou `invalid`, même si le processus a imprimé des lignes d'objet.
   Une interruption doit également laisser un statut terminal explicite.
2. Ne pas écrire « bras alternés AB/BA » avec `repetitions=1`. Exiger au moins
   deux répétitions pour une comparaison, ou décrire exactement l'ordre
   réellement joué ; conserver la précision sub-seconde du mur sans dépendre
   de `/usr/bin/time`, absent de l'image.
3. Graver le compilateur et la configuration CMake, puis ajouter une
   auto-fixture qui tue au minimum pin sale, destination existante, run non
   nul et digest divergent.
4. Le plan `none/q3/q4/both` n'est pas exécutable avec l'API courante :
   `cover_envelope_filter` est un booléen global qui active q3 et q4 ensemble.
   Ajouter des bras internes par lane dans la sonde de mesure, sans élargir
   nécessairement l'API produit, avant d'annoncer cette matrice.

## Suite après réception

Mesurer `sites_before`, `sites_after`, tests transverses, scans réellement
évités et mur par lane sur un protocole calme. Le filtre ponctuel visite encore
tous les sites des handles. Ne pousser le rejet de nœud par boîte que si la
compaction paie ; comparer alors nœuds, visites et mur avec bornes continues
et bornes resserrées par parité.

Le jalon qui change l'exposant local reste l'arrangement shallow de centres :
préparation en $O(m_e\log m_e)$ puis sortie bornée par profondeur, sans former
d'abord toutes les paires. Les quantités à publier par régime restent ancres,
$m_e$, somme des $m_e$, sorties shallow et coût exact ; aucun résultat sur
deux ou trois tailles ne transforme cette cible en claim sous-quadratique
global.
