# Paris Project: Windows and RGB update, 25 October 2026

## Player changes

- RGB supports one or multiple unequipped pieces, original/new previews, restoration, and server-owned reagent costs.
- Red, green and blue reagents use item group 14, numbers 450-452. Chaos recipes use IDs 240-242.
- The administrator configures RGB at Custom > RGB, including enablement, reagent requirement and 1-20 potions of each color per piece.
- Reagent consumption remains disabled on the current production installation for the requested cosmetic tests. This is server policy; changing a client does not bypass an enabled cost.
- OpenGL and Vulkan have ground puddles and orbital camera keyboard controls: F9 enables orbital, arrows rotate/tilt, plus/minus zoom, F11 resets. Existing rain is unchanged.
- Vulkan skill commands use one centered row and Escape dismisses the skill selector before opening the system menu.
- OpenGL skill rendering includes improved active skills such as Death Stab.

## Stability and security

- Out-of-range animation/hit counters are rejected before indexing the server animation history. A registered skill cannot authorize a different skill's hit; animations older than the existing ten-second window are rejected.
- RGB snapshots and mutations have independent per-player 500ms request gates. Concurrent snapshot floods cannot repeatedly construct full inventory/nearby-equipment responses or block the first independent apply request.
- Both native clients reject delayed RGB packets when the game/inventory lifetime is no longer valid.
- Puddle mesh points are shared within each patch: 61 terrain samples rather than 260; triangle count, shape, alpha and resolution stay unchanged. No measured FPS increase or 120 FPS guarantee is claimed.
- Source synchronization preserves newer OpenGL social/music and gesture files instead of replacing the remote branch with an older local base.

## Verification

- Both Windows client builds pass.
- Game test suite: 1,653 passed, 10 database-dependent cases skipped; web/admin: 248 passed.
- Final focused RGB/skill security tests: 16 passed, including parallel request bursts.
- Weather: 15 cases / 578 assertions; UI styling: 8 cases / 1,984 assertions.
- Launcher: 44 checks, including safe paths, hashes, rollback and download validation.
- OpenGL puddles/camera accepted in gameplay by the operator before the subsequent CPU sampling optimization. Further live gameplay and frame-time measurements remain necessary.

## Distribution and limitations

- R2 2026.10.25.1 contained the operator-tested changes. 2026.10.25.2 adds the final native client safety/CPU changes. Immutable files are uploaded and SHA-256 verified before the live manifest is replaced; the previous manifest is retained.
- Private connection settings, saved logins, logs and credentials are excluded from source publication and launcher patches.
- This is a bounded code review and test pass, not certification that all code is bug-free or an installed comprehensive anti-cheat.
- Network hardening still requires an operator-approved allowlist for the externally published database gateway. It must not be restricted blindly while external website dependencies are unknown.
- The launcher verifies file integrity using the HTTPS manifest; detached manifest signatures and Windows code signing remain future hardening work.

## Extended event and Crossplay review

- Devil Square now ranks descending by score and safely ignores an unregistered finisher. The regression test completes the real event and checks the score table delivered to the winner.
- Mini-game entrance and HUD countdowns share the configured game-server time zone, wrap to the first configured time tomorrow, and skip nonexistent DST times. An empty timetable no longer invents a midnight event.
- Both native clients bound server-list description reads, reject truncated data, terminate fixed-length names, and release an allocation when its server group is unknown.
- Crossplay uses server ID 1 with the existing game/account/character database and unchanged experience/PVP rates. Season 6 Episode 3 uses port 55904; the Open Source profile uses 55905. Server ID 0 remains unchanged.
- The Windows selector labels server ID 1 as Crossplay. Existing mobile builds may still display a generic subserver name; no new mobile APK/IPA is claimed in this Windows release.
- Further checks: 52 event/Chaos/mini-game cases; 10 persistence-initialization cases; bounded server-list test with 10 assertions.
- The final Windows launcher release is 2026.10.25.3. Production server deployment is gated on the isolated database startup and has a complete runtime/database backup and targeted configuration rollback.

## Final authorization and shared-event hardening

- Server creation, restart, global configuration editors, maps, merchants, quests and cash-shop configuration now require the Administrator route policy, including class-based editors and Blazor circuit navigation. Account editing requires Operator. The complete web test suite passes 248 cases.
- Production explicitly sets PARIS_CASTLE_SIEGE_SERVER_ID=0. Castle Siege ownership uses shared persistence, so Crossplay (ID 1) must not drive a second independent castle lifecycle. The optional environment setting selects its one authority; it requires restart when changed.
- Castle/Siege, mini-game and RGB focused tests: 168 passed. The secondary-host isolation regression verifies no persistent state is accessed on the non-authoritative subserver.
- All 10 PostgreSQL auction/escrow tests initially skipped by the generic suite were subsequently run successfully against a new empty disposable database and a dedicated non-superuser role, over a private SSH tunnel. No production game data was used by those destructive test fixtures.
- Crossplay was checked through both ConnectServers: profile 6.3 sees IDs 0 and 1 and receives 15.235.17.79:55904 for Crossplay; profile 106.3 sees ID 1 and receives port 55905. Both game endpoints return their native hello packet. This is protocol validation, not a claimed full multiplayer play test.
