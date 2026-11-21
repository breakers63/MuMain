# RmlUi local patch

`rmlui-paris-slider.patch` is the Paris slider correction used by the shipped client. CMake applies
it to `src/ThirdParty/RmlUi` automatically when the project is configured (see
`src/CMakeLists.txt`), through `cmake/ApplyGitPatch.cmake`, which does nothing when the patch is
already applied. The submodule stays pinned to its upstream commit, so after configuring it shows
as modified in `git status`; that is expected.

To apply it by hand instead:

```powershell
git -C src/ThirdParty/RmlUi apply ../../../patches/rmlui-paris-slider.patch
```
