"""Lecteurs de formats, separes du coeur.

kitti_bin, las, ply, text_xyz : bibliotheque standard + numpy (tournent sur la VM G4 avec le Python portable).
laz : delegue a laspy + lazrs (paquets optionnels, envoyables avec un Python portable) ; refus explicite sinon.
sevenzip : delegue a py7zr ou au binaire 7z/7za ; refus explicite sinon.
"""
