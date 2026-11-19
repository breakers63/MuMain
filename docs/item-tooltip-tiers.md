# Tier en el tooltip



La franja muestra RAREZA - TIER N y conserva los colores de la rareza.

La rareza cuenta exclusivamente opciones Excellent: 0 común, 1 raro,

2 épico y 3 o más legendario. Skill, Luck, Life, nivel y Ancient no suman.



Un ítem clasificado sin opciones Excellent muestra siempre TIER 1 (NPC).

Con una o más opciones Excellent usa el tier del set/ítem de la tabla:

Thunder Hawk Excellent TIER 5, Dragon Knight Excellent TIER 7, Titan

Excellent TIER 8. Raro, épico y legendario conservan el mismo tier del set.

Los consumibles y objetos sin clasificación no reciben un tier inventado.



Data/Custom/ParisItemTiers.json contiene la clasificación Excellent por

ItemIndex (grupo × 512 + número). Se lee una vez al iniciar; reiniciar

el cliente tras editar. Instalar junto al ejecutable de su arquitectura.

No modifica daño, defensa, drops ni bonos de combate del servidor.

