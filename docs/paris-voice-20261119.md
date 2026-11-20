# Paris voice chat — verified Vulkan release

Two real Vulkan sessions were tested by the user: microphone capture and remote playback both work.
The Paris voice overlay includes proximity, party, guild and personal public/PIN rooms, separate reception,
microphone selection, gain/volume, input meter, local microphone test and a remote connection tone.
Remote audio uses the browser WebRTC playout sink; ICE refresh preserves the connection's bundle policy.
Reception continues while the overlay/game is minimized. Transmission pauses when its client loses focus.
The right sidebar shows a red slash while the microphone is off and a green background while enabled.

Build helper: `dotnet publish tools/ParisVoice/ParisVoice.Client.csproj -c Release -r win-x64 --self-contained false`.
Install its publish output under `ParisVoice/` beside Main.exe. Requires .NET 10 desktop and WebView2 runtimes.
Credentials are issued over the authenticated game session and stay in a bounded inherited memory mapping.
The helper accepts only the local UI origin and the fixed authorized signaling endpoint. No audio is logged.

Production signaling and the separate Paris.Voice game extension are in the server repository.
The optional extension is installed at a fixed application path before plugin discovery; it is disabled by default.
Do not replace production GameLogic/GameServer DLLs with older source builds to activate voice.
No TURN server is included in this release; the two-client test does not establish compatibility with every NAT.

Validation: client Release build, UI startup contract, ICE configuration regression,
11 server policy/room tests, WebSocket authorization/room integration, and the user's two-client audio test.
OpenGL voice sources are preserved; this published binary patch targets the tested Vulkan client only.

The FA/D9 voice entry preserves the installed RGB handler for validated 50-byte RGB requests. The six-byte voice and 50-byte RGB protocols are routed separately, retaining existing authentication/state validation. Regression coverage: 16 passing tests.

Artwork is in the small R2 release ZIP; restore it with `python tools/install_voice_assets.py --archive paris-voz-vulkan-parche.zip`. See docs/paris-voice-assets.json.
