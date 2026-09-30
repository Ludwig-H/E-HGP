# Portée du juge de tour en flux sur grands nuages

30 septembre 2026. Deux rejeux, normal et `−O`, code 0, résultats identiques.
Le code 0 signifie que les angles morts sont reproduits, pas que les dumps
corrompus deviennent des sorties valides. Aucun moteur invoqué ou modifié,
aucune compilation, GCP0.

Le script `invariants_echelle.py` R2 vérifie des invariants structurels,
pas les bijections géométriques de Γ. Le contrôle du développeur restreint
explicitement sa comparaison aux classes attache morte/plateau, bloc absent
et plateau binarisé. Nous n'attribuons pas à cette porte une preuve de
complétude géométrique qu'elle ne revendique pas.

Trois sites `(0,0,0),(2,0,0),(5,0,0)`, dump natif K1..2 archivé :

- Le dump intact passe le lecteur en flux et le juge exact R2.
- Retirer entièrement l'ordre 2 laisse un préfixe cohérent K1. Le lecteur
  rend `None` (acceptation) ; le juge exact refuse les ordres manquants.
  Le lecteur n'a aucun argument K, et son appelant ne compare pas le
  compteur `orders` au K demandé.
- Remplacer les coordonnées x de toutes les attaches par x+1000 conserve
  leur nombre et leur unicité. Le lecteur accepte ; le juge exact refuse
  des points inconnus. Son interface ne reçoit que n, pas les sites attendus.
- Changer les tailles annoncées du header de l'ordre 2 en 999/999 passe
  les deux parseurs. C'est une réserve de schéma, pas une corruption de la
  géométrie réellement lue ni une exclusivité de ce lecteur.
- Le contrôle positif qui retire seulement une attache est refusé par
  les deux juges. Le harnais ne confond donc pas tout refus avec un succès.

Ces corruptions sont fabriquées dans les copies de petits dumps. Aucun
préfixe ni point inconnu n'est observé dans les sorties réelles de LiDAR.
Les `None` et erreurs exactes sont dans les JSON, avec les compteurs.

Actions peu coûteuses : passer l'ensemble des sites attendu et K effectif,
refuser un ordre absent/dupliqué/hors séquence, comparer les attaches aux
sites et vérifier les tailles annoncées. Temps et mémoire linéaires en la
taille lue, sans payer un oracle combinatoire sur les grands nuages.
Ces contrôles ne fermeront toujours pas la complétude géométrique.

Sources figées et cinq fixtures dans ce dossier. Rejeu sans écritures :

`python3 -B check_stream.py`

Les sources géométriques R2 proviennent de la capture précédente ; le
lecteur en flux a été copié depuis sa campagne courante. Leurs hashes sont
fermés avant/après chaque passage. Aucun nouveau producteur ni binaire
commun aux correctifs n'est qualifié.
