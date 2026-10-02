# Q4 sans Level prématuré : contrat du prochain port

Revue de conception du 2 octobre 2026. Source actuelle figée
`9df77494732b03ddf11dbcf1dcb11d96bef54a3b` ; doc DEVELOPPEMENT WIP copiée séparément
(SHA avant : `5a5e64e54af52aa7e9a141060b114ed0ab90b46e302ccbac0636a2a57b78c773`).
Les 66 dépendances et leur état live avant/après sont conservés. **Aucun code candidat
n'existe dans cette source** ; aucun build/natif/GCP, gain ou qualification du port.

## Interface minimale proposée

[support.cpp](sources/morsehgp3D_v11/src/catalogue/support.cpp) ne lit jamais Level :
paires => is_midpoint, triplets => orientation du centre, tétraèdres => strictly_inside.
[catalogue.cpp](sources/morsehgp3D_v11/src/catalogue/catalogue.cpp), lignes 66–76, lit
ancre/N/D pour la propriété ; [leaf.cpp](sources/morsehgp3D_v11/src/catalogue/leaf.cpp),
lignes 68–102, utilise side puis seulement le Level au moment de Collector.accept.

- Un type q4 fermé possède les valeurs ancre/N/D, avec D>0, issu uniquement de la
  fabrique à quatre Point validés. Pas de constructeur public tuple de coefficients,
  tag arbitraire ou « Sphere avec Level zéro ». Il ne conserve aucun pointeur vers
  les supports/workspaces. Dépendance affine => optional vide, comme aujourd'hui.
- Un noyau de centre certifié, construit seulement par Sphere et le type q4, partage
  side, orientation, inside et midpoint ; centre_in_box et canonical_support reçoivent
  cette interface readonly. Deux surcharges fermées peuvent déléguer au même noyau.
  Ne pas exposer un concept public de getters forgeables : la borne native dépend
  aussi du certificat de présentation. Les vues éventuelles restent synchrones et
  ne survivent pas à leur propriétaire.
- À la frontière num→catalogue, exposer le type opaque et les opérations nécessaires
  via num.hpp ; catalogue ne construit pas le centre en recopiant les formules d'un
  en-tête privé num. La représentation et ses constructeurs restent privés.
- Sphere::through4 conserve son contrat complet : centre candidat puis matérialisation
  immédiate du Level, avec les mêmes refus. Le catalogue utilise séparément la
  matérialisation explicite **q4 seulement**, sans cache mutable ni allocation.
  Q2/q3 gardent leurs Level actuels ; aucune méthode générique « carré de N » pour q3.

Le centre inclut l'ancre de coquille qui définit le rayon implicite ; un centre isolé
associé à une autre ancre n'est pas le même certificat de boule. Garder exactement
les coefficients/tag de présentation, même après calcul de qmin. Le Level q4 est
formé directement à partir de ces mêmes N/D : ni nouveau centre, ni réduction PGCD,
ni réancrage. [Level](sources/morsehgp3D_v11/src/num/level.hpp) est non réduit et le
[banc](sources/morsehgp3D_v11/bench/catalogue_probe.cpp), lignes 76–98, sérialise ses
entiers : une fraction égale obtenue autrement peut changer les octets publiés.

## Garde à ne pas perdre et ordre des étapes

**Fermé ne signifie pas support positif.** Conserver strictly_inside avant propriété,
judged et census ([leaf.cpp](sources/morsehgp3D_v11/src/catalogue/leaf.cpp), 58–63).
A=(0,0,0), B=(4,0,0), C=(0,4,0), D=(0,0,4) donnent une présentation q4 valide,
centre (2,2,2), niveau12, poids barycentriques (-1/2,1/2,1/2,1/2). Le centre appartient
à la boîte racine mais sort du tétraèdre. Sans cette garde, K3/p0/m=q4 saute la
canonicalisation et admettrait à tort le qmin initialisé à4. Le produit actuel rejette.

`TRACE.json` vérifie exactement la géométrie de cette fixture et d'un q4 positif,
avec deux traces de passe identiques ; son mutant abstrait sans inside émet la
mauvaise boule. **C'est un témoin du piège de port, aucune panne native reproduite.**

Après inside/propriété : mêmes seuils et même ordre du census (theta du q généré),
coquille complète, canonical_support, égalité S*=generated, admission p+qmin,
puis Level q4, puis Collector.accept. Ne pas materialiser le niveau des présentations
rejetées, ni déplacer l'incrément judged/census pour masquer leur travail.
Le cas m=q peut sauter la canonicalisation seulement parce que la positivité a
été vérifiée. Les préfixes q3 obtus continuent vers q4 comme aujourd'hui.

## Deux passes, refus et capacités

[assemble.cpp](sources/morsehgp3D_v11/src/catalogue/assemble.cpp), lignes 33–52, garde
le comptage puis le remplissage, et compare leur ledger. Les deux passes doivent
chacune matérialiser les seules émissions qmin4 : ne pas supprimer silencieusement
le Level de la passe filling=false. Ajouter un compteur clairement défini par passe,
inclus dans la comparaison des ledgers ; sur succès, il égale le nombre d'émissions
qmin4. Les anciens compteurs de géométrie doivent rester exactement identiques.

La matérialisation réussit avant Collector.accept ; son refus interne se propage en
arithmetic_invariant comme checked_level, jamais en Level zéro, rejet de candidat
ou paramètre invalide. Les gardes de capacité/counter et les écritures transactionnelles
de [Collector](sources/morsehgp3D_v11/src/catalogue/catalogue.cpp), 47–63, restent en
place. Les anciennes gardes de Level des candidats rejetés sont invariants inatteignables
sur le domaine certifié ; préserver les refus publics n'exige pas de calculer leurs Level.

Le candidat est local de taille constante : pas d'arène, memo, tableau par préfixe,
std::function allouante ni nouvel état TLS. Emission garde son Level complet ; les
tailles B/P/L, réservations Buffer et sites d'allocation restent inchangés pour ce
port isolé. Aucun pic/RSS/sizeof nouveau n'est déduit de cette lecture.
La métrique nouvelle est publiée par le banc sans entrer dans le fichier canonique ;
les lecteurs historiques ne doivent pas exiger artificiellement son absence.

Portes futures ciblées : q4 hors hull et q4 positif ; émissions qmin4/m5 étendues et
préfixe obtus existants ; rejet non canonique sans Level ; public Sphere toujours
complet ; mêmes fichiers canoniques/anciens compteurs/refus/pics Buffer, et compteur
Level q4 égal dans les deux passes aux émissions qmin4. Garder les budgets/profils
et injection d'allocation actuels ; ces portes ne sont pas exécutées ici.

Coordination math : le minorant familial « témoins coplanaires strictement dedans le
disque du triple => intérieurs de toutes extensions q4 » est une **autre** optimisation.
S'il est porté, son seuil q4 et son compteur sont séparés du q3 et du census final ;
aucun seed d'intérieurs n'en découle. Il modifierait le travail discret et ne doit
pas être mélangé à la seule matérialisation tardive avant son ablation propre.

Lecteur autonome : `python3 -B judge.py` et `python3 -B -O judge.py`.
