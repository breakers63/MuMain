# RmlUi local patch

After initializing src/ThirdParty/RmlUi, apply the Paris slider correction used by the shipped client:

```powershell
git -C src/ThirdParty/RmlUi apply ../../../patches/rmlui-paris-slider.patch
```

The submodule stays pinned to its upstream commit. Do not apply the patch twice.
