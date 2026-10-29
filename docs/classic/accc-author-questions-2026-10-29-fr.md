# Questions à l’auteur de l’ACCC, de DSC4 et d’Amazing

Brouillon du 29 octobre 2026, non envoyé. Rédigé par Terra 6.1, effort high,
puis relu. Les références sont celles de l’ACCC v1.11 française.

---

**Objet : Deux questions sur DSC4, Amazing et la parité du CRTC 1**

Bonjour,

En étudiant DSC4 et Amazing sur notre modèle de CRTC 1, nous avons rencontré
deux comportements sur lesquels votre éclairage nous serait précieux.

**DSC4 : quelle adresse cette séquence doit-elle recharger ?**

Nous observons une séquence avec R9=7 : le mode IVM est activé puis désactivé
avant la RFD. R12/R13 sont ensuite modifiés à &33BD, mais notre modèle mémorise
puis recharge &1030. Pour cette technique, quel résultat cherchez-vous à
obtenir ? Y a-t-il une condition qui nous échappe pour que &33BD soit repris ?

Nous essayons aussi de rapprocher ce cas de la section 11.6.1, page 90, qui
mentionne une parité impaire empêchant la comparaison C9/R9, et des séquences
des sections 11.6.2 et 11.6.3, pages 91–92. Quelle parité intervient exactement
dans cette condition ? La règle dépend-elle de la valeur paire ou impaire de R9 ?

**Amazing : l’inhibition doit-elle persister après le changement de source ?**

Sur MiSTer, l’en-tête reste lisible, mais les lignes du bas présentent parfois
des fragments répétés. Dans notre trace, la sélection de R12/R13 comme source
d’adresse cesse avant la corruption, tandis qu’une inhibition liée à la parité
semble persister. Est-ce compatible avec le résultat recherché par votre
technique, ou manque-t-il une condition dans notre reproduction ?

Pour isoler la question, nous avons préparé un diagnostic où R1>R0 met fin à
la sélection de R12/R13 sans mémorisation. R1 est ensuite rétabli avant le test
suivant, dans la même trame, sans nouveau RFD ni changement de R8. La parité
peut-elle encore empêcher la mémorisation de VMA′ après la fin de cette
sélection ? Ce cas isolé ne prétend pas reproduire toute la technique d’Amazing.

Les états internes cités viennent de notre modèle. Le diagnostic n’a pas encore
été exécuté sur un CPC réel ; la corruption décrite est une observation MiSTer.

Merci beaucoup pour votre aide et pour ces démos !

Bien cordialement,

---

## Compléments disponibles si nécessaire

Le détail des phases du cas DSC4 et la comparaison avec CPCSyntaxError sont
dans [le relevé DSC4](../investigations/hardware-runs/dsc4-video-2026-10-29.md).
Un programme combiné pour un futur test sur CPC classique CRTC1 sera préparé
par V3 après le travail FIELD/ASCAL. Un résultat sur 6128 Plus (CRTC3) ne
trancherait pas ces questions de CRTC1. La préparation de ce brouillon
n’autorise aucun envoi.
