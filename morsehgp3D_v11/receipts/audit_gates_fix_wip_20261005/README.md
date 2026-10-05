# Corrections des portes : capture WIP stable

Base locale : `ef91a7f467ddb150106f557e083b231155e52414`, worktree développeur `build/v11-impl-l3`. Les trois fichiers modifiés ont les mêmes empreintes avant/après lecture et à la fermeture de la capture ; état WIP, aucun test natif exécuté ou qualification déduite. Voir `capture_initial.json`, `review.json`, `changes.diff` et les trois sources figées.

Avis CLI favorable en lecture : `sp_masque_16379` cible désormais `mhgp11_cli_points`, qui atteint `order_params` puis `build_order`. La mutation laisse `concurrent_orders=true`, refusé par ce constructeur sur les cas admis ; la porte exige code 0. Les 28 individus et le plancher 28 sont conservés.

API : les contrôles d’identité des fichiers, manifestes et journaux entre voies et W1/W4 restent inchangés octet pour octet ; le calcul public reste contrôlé par publication identique et pic FULL distinct de la voie K seule. La nouvelle ligne numérique résout le conflit de SHA des profils. Elle retire cependant aussi le journal gravé et les empreintes u21 connues des attentes. C’est une perte de garde de la porte, sans défaut moteur démontré.

Minimum proposé : rétablir `journal=<digest>` dans le verdict commun et les six attentes ; conserver en plus les empreintes fichier/manifeste connues pour u21. `LINE` est une valeur unique. Les deux lignes étant adjacentes, un unique `EXPECT_LINE` contenant empreintes, saut de ligne et verdict permet au matcher actuel de vérifier les deux ; ne pas passer deux mots `LINE`. Le transport réel de cette valeur dans CMake/CTest reste à vérifier par la porte native.

Rejeu borné stdlib du matcher : `python3 replay.py` puis `python3 -O replay.py`. Les témoins vérifient les cas conformes, une empreinte u21 altérée, un journal altéré, des comptes altérés, une ligne absente ou des lignes inversées, et la normalisation CRLF. Ce modèle ne lance ni CMake, ni le moteur, ni le cloud.
