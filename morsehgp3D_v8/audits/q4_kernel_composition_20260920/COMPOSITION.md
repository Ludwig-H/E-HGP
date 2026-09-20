# Composer les certificats, sans perdre leurs témoins

Proposition indépendante après le port29 `31b0243a`. Les ports28/29 sont
contre-lus séparément ; ce raccord n'est pas encore un moteur mesuré.

## Un ordre de composition sûr

Pour une population P de formes, noter p_P(t) son nombre strictement négatif.
Le noyau R de29 vérifie l'implication forte suivante :

$$p_R(t)<T\ \Longrightarrow\ \forall z\in P\setminus R,\ L_z(t)>0.$$

Commencer par R, puis construire une partition exacte de **cette population**
sur une cellule fermée C : I uniformément intérieur, E strictement extérieur,
A actif, avec c=|I|. Si c≥T, la cellule est rejetée. Sinon poser H=T−c et
construire un noyau local B⊆A, par H couches complètes de chaque signe.
Alors, avec d=c+p_B(t), pour tout t∈C :

$$d<T\ \Longleftrightarrow\ p_P(t)<T,\qquad d<T\ \Longrightarrow\ d=p_P(t)\ \text{et}\ S_B(t)=S_P(t).$$

Preuve : si d<T, p_B<H implique que A\B est strictement extérieur.
Donc p_R=c+p_B=d<T ; le certificat global rend P\R strictement extérieur
à son tour. Les I sont strictement intérieurs, les E strictement extérieurs :
toute la coquille est dans B. Réciproquement d≤p_P car I et B sont disjoints.
Une seed de A\B ne peut porter aucune racine admissible dans C. Une seed
de I ou E ne coupe pas C ; une seed de P\R ne porte aucune racine admissible.
Le même raisonnement permet plusieurs réductions **emboîtées**, avec leur
population d'entrée et leur seuil attachés à chaque certificat.

Une option plus limitée consiste à garder l'atlas28 existant, son compte c
sur P et seulement A∩R dans ses feuilles. Le compte d=c+p_{A∩R} reste un
minorant de p_P et majore p_R : d<T suffit donc au certificat global, puis
à l'égalité. Ce chemin est sûr mais paie encore la construction sur tout P.
Il ne doit pas être présenté comme une partition exacte de R aux centres
profonds, ni comme une suppression du coût de construction de28.

**Ne pas intersecter deux noyaux construits indépendamment.** Deux certificats
peuvent se soutenir par des témoins que leur intersection retire. À T=1,
deux formes constantes négatives suffisent : garder l'une ou l'autre est
un certificat fort valide, leur intersection vide ne l'est pas. Une fixture
géométrique s'obtient avec le tétraèdre régulier
a=(30,30,30), b=(36,36,30), x=(30,36,24), y=(36,30,24), puis deux points
(31,31,30) et (32,32,30) intérieurs au segment ab. Chacun est intérieur
à toute boule passant par ab. Deux noyaux gardant chacun les quatre sommets
et un seul de ces témoins rejettent à T=1 ; leur intersection accepterait
faussement la boule régulière de profondeur2. Cette contre-fixture concerne
la composition de certificats forts ; elle ne prétend pas être la sortie
du peeling29, qui garde ces groupes dégénérés.

Le compte c n'est payé qu'une fois. Ne pas y ajouter un « crédit T » de
la sélection, ni reconstruire après filtrage un compte sur une population
différente. Les IDs intérieurs et la canonisation globale restent distincts.
Un noyau préparé au seuil maximal T sert aussi aux seuils inférieurs ; q3
peut demander un seuil supérieur à q4 et n'hérite donc pas automatiquement
de ce certificat.

## Recentrer les couches, exactement

On peut construire le noyau local autour d'un point dyadique t₀=(α/Q,β/Q),
par exemple le centre de C. Dans U=Qξ−α et V=Qη−β :

$$Q L_z=d_z+a_zU+b_zV,\qquad d_z=Qc_z+\alpha a_z+\beta b_z.$$

Séparer les signes de d et garder les d=0 reproduit exactement la preuve29.
La translation change éventuellement le noyau, pas le contrat. Conserver
tous les IDs coïncidents, les frontières complètes et les restes dégénérés.
Les seules seeds aiguës ont c_x>0 dans la base originale, puisque
c_x=2(|x−a|²+|x−b|²−|a−b|²). Cette règle ne s'applique pas à d_x après
translation, et ne permet jamais de retirer les témoins de signe négatif.

**Le déterminant traduit naïf ne tient plus forcément en i128 à Q=2⁴⁴.**
Il n'est pas nécessaire de le former : le changement de la colonne constante
donne det(a,b,d)=Q det(a,b,c). Son signe se calcule avec le déterminant
original de29, puis les trois signes de d introduits par la normalisation
des dénominateurs. Pour l'ordre lexicographique, employer par exemple :

$$a_i d_j-a_j d_i=Q(a_i c_j-a_j c_i)+\beta(a_i b_j-a_j b_i).$$

Avec |c|≤15M², |a|,|b|≤8M², |α|,|β|≤2Q et M=65535, |d|≤47M²Q.
Le membre droit ci-dessus est borné par496M⁴Q<2¹¹⁷ à Q≤2⁴⁴ ; les signes
et comparaisons tiennent en i128, avec promotion avant produit. Le d traduit
ne tient plus nécessairement dans le champ i64 de `Q4LocalForm` : nouvel
état arithmétique ou calcul à la demande, jamais une conversion implicite.
Cette identité évite un nouveau verrou de largeur ; elle ne qualifie pas
un port de l'algorithme traduit.

## Coût et objets à mesurer avant le choix

Le raccord le plus simple à comparer est **noyau global puis atlas sur les
retenus**, face à28 et29 séparément. Un sous-ensemble arbitraire d'IDs retenus
n'est pas un nœud spatial complet. Il faut soit des plages de rangs filtrées
avec populations exactes, soit une frontière de nœuds certifiés entièrement
retenus et des feuilles partielles. Ne pas remplacer une population de bloc
par celle du bloc d'origine ; garder le même propriétaire/index et mesurer
la préparation de ce sous-ensemble, sans recopier le nuage.

Le peeling local/recentré est une option à tester **après** cette première
composition. Son coût Σ_C(a_C log(1+a_C)+H_C a_C) peut dépasser le gain des
requêtes, notamment avec peu de seeds. Il ne faut ni le payer par face ni
l'appliquer aveuglément aux petites populations LiDAR. Le volume d'événements
résiduel et la taille des coquilles peuvent rester grands.

Les objets parallélisables sont une population réduite immuable par arête,
puis des fragments possédés partageant index et certificat parental, puis
des plages de seeds et des buffers privés. La construction des couches,
les copies de frontières et les sorties ont leur propre coût. Aucun nombre
constant de cellules ni aucune borne globale sous-quadratique ne découle
de ce raccord.

[composition_gate.py](composition_gate.py) vérifie le contrat avec un oracle
rationnel de droites d'appui, distinct des chaînes monotones du produit :
origines différentes, contacts, doublons, cas dégénérés, racines et points
intérieurs aux cellules. Les contre-modèles réfutent l'intersection
indépendante et le double compte. Aucun résultat de performance n'en découle.
