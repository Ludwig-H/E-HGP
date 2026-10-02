# Puissance native : invariant public, profils et portes

Audit statique indépendant du 2 octobre 2026, source publiée
`9d639e1460e7a75b9af5340bc1fc436ea00c1d07`. Le chantier n'était plus WIP au gel :
les sources/tests num correspondaient au commit, sans changement local. 59 fichiers
copiés depuis les objets Git avant leur lecture, plus trois sources de la baseline
`3e7b52b43`. Les hashes avant/après ferment les pièces. Aucun build, test natif,
mutant compilé, GPU/GCP ou campagne ; **aucune nouvelle qualification native héritée**.

## Conclusion

Aucun défaut concret établi dans la cohérence de cette révision. Le dispatch est
porté par un certificat de **présentation**, couplé aux coefficients ; les nouvelles
portes couvrent explicitement les valeurs de power, side et le tag. La requalification
G4 doit porter sur ces sources nouvelles, pas seulement sur la baseline qualifiée.

- [geometry.hpp](sources/morsehgp3D_v11/src/num/geometry.hpp), lignes 12–50 :
  coordonnées privées validées par Point::make ; Sphere non construisible avec des
  coefficients/tag arbitraires, fabriques seulement. Copies/déplacements/affectations
  de la valeur entière conservent ensemble ancre, N, D, niveau et tag. Les lectures
  n'allouent aucun tableau et n'empruntent pas les supports de la fabrique.
- [sphere.cpp](sources/morsehgp3D_v11/src/num/sphere.cpp), lignes 13–66 : tags
  1/2/3/4 fixés par les fabriques ; D positif ; dépendance affine => optional vide.
  q3/q4 ne requièrent pas un support aigu/intérieur. Le nouveau tag n'est ni qmin,
  ni un certificat de criticité, ni une identité canonique de la boule.
- [predicates.cpp](sources/morsehgp3D_v11/src/num/predicates.cpp), lignes 7–56 :
  power et side ont le même dispatch. u18 utilise i128 aux quatre arités ; u21/u24
  gardent Wide pour q3, i128 pour q1/q2/q4. Side lit le signe natif ; power garde
  require_fit vers SideInt. Intermédiaires natifs q1/q2 bornés sans annulation ;
  preuve fine q4 u24 auditée séparément dans
  [native_arity_bounds_review_6](../native_arity_bounds_review_6/README.md).

**Contrat à conserver dans un futur port :** canonicaliser une boule ou trouver
qmin=2 ne permet pas de retaguer ses coefficients q3 en q2. Le témoin indépendant
q3/u21 à qmin2 de l'autre reçu a un premier produit de 129 bits, malgré un total
final de 127 bits. Refaire la présentation par une fabrique adaptée est différent
changer le tag seul. Le produit actuel conserve correctement le tag d'origine.

## Portes lues, sans exécution

[power_test.cpp](sources/morsehgp3D_v11/tests/num/power_test.cpp), lignes 30–74 :
les huit présentations (petites/extrêmes x arités1..4) sont copiées puis comparées
à la formule Wide du harnais ; leur tag est vérifié **avant** power/side. Ainsi le
mutant q3 tagué q4 est refusé avant une exécution native hors domaine. Le petit
q3 est rectangle (qmin2) ; le petit q4 a son centre hors du tétraèdre : les portes
ne supposent donc pas implicitement un support critique. Les requêtes de coquille
et l'annulation q3 avec un premier produit hors i128 à u21/u24 sont explicites.
Les tailles 128/144/160 sont des attentes d'ABI du harnais, pas un format publié.

[power_reference.hpp](sources/morsehgp3D_v11/tests/num/power_reference.hpp) est
une comparaison d'exécution entièrement large, **pas** un oracle indépendant :
il partage les coefficients Sphere et l'arithmétique Wide. Le
[fraction_oracle.py](sources/morsehgp3D_v11/tests/num/fraction_oracle.py), lignes
62–129, recalcule séparément centre/niveau par Gram-Gauss/Fraction, puis H, side,
référence Wide et arité. Les dégénérescences et le niveau précédent sont cohérents
avec le protocole [probe.cpp](sources/morsehgp3D_v11/tests/num/probe.cpp).

`STATIC_INSPECTION.json` recoupe les 13 patterns uniques et leur plancher ; les
quatre mutations nouvelles sont explicitement u21 ou u24. Le lancement place leurs
options après les options générales : leur profil ne dépend pas du nouveau défaut.
La matrice choisit explicitement u18 Release/mutants, u24 ASan/UBSan et u21 TSan,
poison et profils dédiés. Le défaut CMake u21 ne supprime donc pas la branche q3
native u18. Une qualification sanitizer u24 ne doit toutefois pas être présentée
comme une exécution sanitizer de q3 natif u18 ; cette combinaison n'est pas demandée
par la matrice actuelle. Les résultats de cette future session ne sont pas audités ici.

`PINS_VERIFICATION.json` vérifie les trois SHA de baseline annoncés par
source_pins.json contre les objets Git ; les octets exacts sont inclus. Les pins R2
historiques restent une provenance, pas une qualification transférée aux nouveaux
coefficients/tags/voies natives. Aucun chiffre de performance nouvelle n'est déduit.

Lecture close : `python3 -B judge.py` et `python3 -B -O judge.py` ; hashes, pins et
recoupe statique seulement. Aucun appel d'exécutable natif par ces lecteurs.

Le plan complémentaire `bench/plans/catalogue_profiles_g4.json` avait un WIP live
différent dès sa copie depuis l'objet9d639. Les différences live avant/après sont
enregistrées sans écraser les copies Git. Le premier lecteur exigeait abusivement
l'égalité live=commit sur ce plan : refus/script/flux conservés dans
`initial_reader_failure/`, correction limitée à la portée du lecteur de snapshots.
Aucun défaut produit ni qualification du plan live n'en est déduit.
