# Paris dragon HUD

The Paris Gold theme uses the approved OpenGL dragon artwork and original potion and skill icons. SD sits in the gold well before the life orb; AG sits in the violet well after the mana orb. All four resources show their actual values and fill from the bottom. Empty resources remain empty. Experience fills the central upper track.

The HUD remains a bottom-centered 640-unit composition, with a 61.629630-unit height and the existing workspace placement and UI scale. Its own frame defines the gauge wells, sockets and control spacing. The theme controls all geometry; the shared model supplies server values. Wider viewports keep the artwork's proportions instead of stretching it. The native potion render targets and skill selection read their bounds from the same elements.

The decorative dragons have no ranking or Auto PvP action. The side shortcuts keep their existing actions and hide while inventory or another blocking window is open. The previous lower coordinates and performance bars are hidden; map coordinates remain at the top of the game.

The six `Paris_Hud*37` assets are byte-identical to OpenGL. Each vertical fill clips a fixed-size child image, so losing HP, MP, SD or AG reveals the correct lower part without squeezing the texture. RmlUi needs `clip: always` here because artwork anchored at the bottom overflows above its parent and does not increase its scroll height. The AG fill extends one physical pixel left, matching the approved OpenGL adjustment.

Paris Gold uses full-level experience progress, measured between the current level's lower experience bound and the next level's threshold. The bar restarts when the level's lower bound advances. Legacy and Modern retain the original ten-segment presentation. Regression coverage exercises full, partial and zero resources in the native RmlUi engine at seven resolutions and four UI scales, plus normal and master experience above 32-bit limits.
