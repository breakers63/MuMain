# Paris calmer lighting preview

This candidate profile changes only three Enhanced-look values, using the values
upstream selected for its shipped look:

- Skill/effect light gain: `2.5` to `0.5`.
- Skill/effect light range: `3` to `2`.
- Metallic specular intensity: `1.0` to `0.8`.

The goal is to reduce washed-out ground around combat and make metal armor less
glossy. It does not change the regular shipped profile. `LightMood.json` is copied
unchanged because the developer command loads both files as one candidate.

In an editor-enabled client, choose Enhanced graphics, go to Lorencia with the
same character and skill effects for both captures, then compare:

1. `$look load` to restore the shipped profile.
2. Capture the scene.
3. `$look load paris-calm-lighting`.
4. Capture the same scene and compare the armor highlights and light spill.

Use `$look load` again to restore the shipped values. The candidate loader is a
developer-build feature; it is not exposed in player builds.
