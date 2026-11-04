# Questions R7 et mesure S — brouillon non envoyé

Ces questions complètent les questions C0/R5 existantes. Aucun nouveau modèle
n'est retenu à partir du seul accord entre émulateurs.

## R7 en fin de ligne sans changement de C4

Sur CRTC 1,§16.4.2p171 indique qu'une écriture R7=C4 déclenche immédiatement
la VSYNC et comptabilise la ligne partielle. Le B6/019D photographié distingue
la quatrième et la cinquième µs d'un OUTI, mais son cas terminal mélange la
fin de C0 et le changement de C4.

Pouvez-vous confirmer le cas où l'écriture arrive près de C0=R0, mais sur une
ligne C9 non terminale : C4 reste égal à la nouvelle valeur de R7 après le
retour de C0 à0. Doit-on observer une VSYNC de durée normale, incluant la ligne
partielle, ou existe-t-il une restriction liée à C0 indépendamment de C4 ?

Notre nouveau petit programme démarre normalement depuis une disquette, sans
modifier PC/compteurs depuis l'hôte. Des mots VRAM portant leur propre adresse
identifient les lignes affichées voisines : VMA bas=400, bitsC9 de0à1, avec
R4=38/R9=7/R5=R8=0. AmSpirit1.17.1/CRTC1A renvoie néanmoins0 lors de la lecture
PPI suivante. Cela qualifie une observation logicielle, pas le résultat matériel.
Les images LIVE et le programme sont conservés et peuvent accompagner la
question. La phase précise du bus n'est pas déduite des coordonnées du cadre.

Un essai normal vérifié du même programme dans CPCSyntaxError 0.12, sous ROMs
UK avec nom BIN explicite, charge les 623 octets attendus et renvoie 1. Sa trace
situe l'écriture à C0=0/C4=10/C9=1, à la granularité caractère de son ordonnanceur.
Cette divergence logicielle n'établit donc pas l'ouverture physique du bus.

Un modèle fondé seulement sur une comparaison plus tardive avec C4 devrait
préserver l'égalité dans ce cas non terminal. Nous ne souhaitons pas ajouter
une restriction C0 pour reproduire AmSpirit sans confirmation matérielle.

## Résultat physique de CS/02D3

La version actuelle du programme attend4E40 pour «FRAME LENGTH WITH R5=8 AFTER
304 INTERLACE LINES». La photo CS_CRTC1_A.webp semble afficher4E30. Est-ce
une différence de version, une variante matérielle, ou une quantification de
la mesure qui autorise les deux valeurs ? Nous ne proposons pas une tolérance
sans confirmation. CPCSyntaxError 0.12 donne4E40, ce qui correspond au programme,
mais ne suffit donc pas à qualifier une concordance exacte avec cette photo.

Indépendamment de cette petite différence, notre cœur donne4E40 en entrée
directe et0640 après le parcours canonique; un changement de publication VSYNC
d'un Pixel-M2 donne9600 après ce même parcours. La dépendance à l'historique
reste à expliquer. L’index public CE_B/02CA ne liste que les CRTC3/4 au4novembre. Existe-t-il
une référence CRTC1 pour cette seconde page, ou un état de sortie E à vérifier
avantP/S ?

Références : ACCC 1.11FR§16.4.2p171,§17.2.1p179 et§20.2p241;
photos Logon System B6 et CS. Les rapports datés conservent les images,
identités et limites; aucune de ces questions n'a été envoyée.
