# CRTC1 : comptage de C5 après modification terminale de R9

Brouillon actualisé le 6 novembre 2026, non envoyé. Références : ACCC v1.11 française,
§11.2.4 p.85 et §11.3.2 p.87. Ce cas réduit n'a pas encore de mesure sur CPC réel.

Bonjour,

Nous avons corrigé l'entrée en gestion additionnelle lors d'une modification
de R4/R9 sur C0=R0, en conservant le comptage avec les nouvelles valeurs comme
indiqué p.85. Une question subsiste concernant C5.

Prenons R0=7, R4=0, R5=3 et R9=1, puis écrivons R9=2 pendant le caractère
C0=R0 où C4=R4=0 et C9=R9=1. La gestion additionnelle reste vraie ; la ligne
suivante a C4=0 et C9=2. À la fin de cette ligne, C9 revient à0 et C4 passe à1.

Cette ligne compte-t-elle normalement dans les trois lignes additionnelles,
ou le nouveau test C4=R4/C9=R9 réinitialise-t-il C5 bien que la gestion soit
déjà active ? Notre modèle actuel donne quatre lignes parce qu'il redémarre
C5 à cette rencontre. Cette règle est une déduction du résultat0080 de
SHAKER CE/02C9, après une gestion retenue avec R5=0 ; elle n'est pas une mesure
de la remise à zéro physique de C5.

Le passage préalable par R5=0 distingue-t-il ce cas de l'exemple ci-dessus,
dans lequel R5 reste constamment égal à3 ? Y a-t-il une séquence SHAKER
existante qui permette de vérifier cette différence sur un CRTC1 ?

Pour une mesure réduite, sans accéder directement à C5, nous proposons
d'observer RA à un caractère fixe des lignes suivantes. Si les trois lignes
sont comptées en continu, leurs couples(C4,C9) sont(0,2),(1,0),(1,1), puis
la trame redémarre à(0,0). Avec notre redémarrage de C5, une ligne(1,2)
s'intercale avant ce retour. À la troisième frontière suivant l'entrée,
RA=0 ouRA=2 distingue donc les deux modèles. Un contrôle sans écriture et
un contrôle avec l'écriture un caractère plus tôt vérifieraient la position
de l'entrée. Cette proposition ne constitue pas une observation matérielle.

Merci !
