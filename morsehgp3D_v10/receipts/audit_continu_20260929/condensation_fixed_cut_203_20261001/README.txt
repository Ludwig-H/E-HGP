Condensation a coupe fixe : obstruction conditionnelle sur six IDs.

Definitions autonomes : une partition de {A,B,C,D,E,F}, fixee au meme niveau,
donne a mcs=m uniquement ses blocs de cardinal >=m. Aucun point ne change
de bloc. Cible2={AB,CD,EF}; cible3={ABC,DEF}. Il ne s'agit pas de selection
EOM ni d'une operation de condensation qui modifie une projection selon mcs.

Tout le Bell6 est enumere : les203 partitions, par mots de croissance
restreinte, sont publiees en entier dans normal.json et optimized.json.
Une seule partition realise Cible2 a mcs2 (001122), une seule realise
Cible3 a mcs3 (000111), aucune ne realise les deux. Le lecteur recoupe
l'inventaire complet par un second algorithme : insertion successive des IDs.
1624 controles de renumerotation des points,609 labels opaques,5481
inclusions de retention aux seuils1..7 et quatre couples de cibles
compatibles attestent que mcs peut retirer un bloc, pas en reaffecter un point.

Preuve courte : si ABC est un bloc conserve a mcs3, ce meme bloc de taille3
est conserve a mcs2. Il ne peut donc devenir AB avec C dans CD. La preuve
exhaustive inclut les partitions avec singletons, gros blocs et bruit retire.

PORTEE STRICTEMENT CONDITIONNELLE : SI ces deux cibles sont choisies, au meme
niveau/coupe et avec ce modele de retention, aucune projection independante
de mcs suivie de ce seul nettoyage ne peut les realiser. Cela NE CONFIRME
PAS Q1bis, ni un choix utilisateur, ni Pi2, ni une modification obligatoire
du modele HGP. Cela ne juge aucune sortie native ni aucun oracle externe.

Contexte lu (non requis par la preuve autonome) : proposition2.3 de
/workspaces/E-HGP/build/v10-verrou-points/juge_final/cibles/CIBLES_REVISEES.md
sha25640db48dc6f72ae95c63a4df4de6c54fc1e54a17e50e096af815c3bc2b0242bc2.

Captures reelles UTC01:08:39–01:08:40 le1octobre2026 : Python normal et-O,
sous timeout10s, code0, sorties identiques octet pour octet,0,19s/0,26s.
run_receipt.json garde argv/exits/temps et nomme les captures de sortie
combinee conservees integralement. Aucune capture stderr separee n'est
pretendue. Bibliotheque standard Python seulement; aucun moteur/GCP.

Archive ferme : neuf fichiers texte, dont sept manifestes.
Lecteur hash-first READ-ONLY, pas import d'enumerate.py et pas de relance :
python3 -B read.py CHEMIN_ARCHIVE SHA_EXTERNE_MANIFESTE
python3 -B -O read.py CHEMIN_ARCHIVE SHA_EXTERNE_MANIFESTE
Les sources/captures fermees ne doivent pas etre reecrites. Un eventuel
rejeu d'enumerate.py est explicitement separable et ne modifie aucun fichier.
