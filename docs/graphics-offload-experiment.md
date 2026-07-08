# Graphics-offload experiment (GTK 4 `GtkGraphicsOffload`)

**Status: experimental, opt-in, off by default.** This is a first attempt at
using GTK 4's graphics-offload API in the desktop shell. It has been written
and reviewed but not yet benchmarked on real hardware — treat the numbers as
something to *measure*, not as a settled result.

## What it does

The GTK shell shows `<video>` by having the `nordstjernen-video` helper decode
frames into a shared-memory ring, which `src/gtk/procview.c` then **Cairo-blits
into the page drawing area on every tick**. That means each video frame is
uploaded and composited by the CPU/GSK into the window's main surface, every
frame, forever.

With `-Dgraphics_offload=enabled`, the video overlay is instead presented as a
`GtkPicture` (fed a `GdkTexture` per frame) wrapped in a **`GtkGraphicsOffload`**
widget, positioned over the `<video>` rectangle. On a Wayland compositor that
supports it, GTK promotes that texture to its **own subsurface**, which the
compositor can put on a hardware overlay plane and scan out directly — the frame
never has to be composited into the page at all.

This is the single most impactful new GTK capability for a browser (see the
"graphics offload / dmabuf" family added in GTK 4.14 and matured through the
4.2x cycle): it is what makes video playback cheap on power and thermals.

## Why it's behind a flag

- It changes how video is presented and only pays off on compositors that
  accept subsurfaces/overlays (Wayland); the benefit and the exact behaviour
  are worth measuring before making it the default.
- It currently feeds GTK a **`GdkMemoryTexture`** (a CPU copy of each ring
  frame). GTK can still offload a memory texture to a subsurface, but the
  zero-copy ideal is a **dmabuf** texture straight from the decoder — a natural
  follow-up once the video helper can export dmabufs.
- Page-drawn video controls/overlays live in the main surface; depending on
  subsurface stacking they may be occluded by the offloaded plane. Fine for a
  plain `<video>`, a caveat for custom controls.

## Building

Requires GTK ≥ 4.14 (already the shell minimum). To exercise the newest GTK
APIs, build against the development **GTK 4.23.2** overlay published by
[`nordstjernen-dependencies-build`](https://github.com/nordstjernen-web/nordstjernen-dependencies-build)
(`linux-gtk-latest`):

```bash
# 1. Fetch the prebuilt dev-GTK overlay (built on the same Ubuntu you run):
export NORDSTJERNEN_LINUX_SYSROOT="$HOME/.cache/nordstjernen-linux-sysroot"
linux/scripts/fetch-prebuilt-deps.sh --sysroot "$NORDSTJERNEN_LINUX_SYSROOT"   # from the deps-build repo
ARCH=x86_64
export PKG_CONFIG_PATH="$NORDSTJERNEN_LINUX_SYSROOT/$ARCH/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$NORDSTJERNEN_LINUX_SYSROOT/$ARCH/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG="pkg-config --define-prefix"

# 2. Configure Nordstjernen with the experiment on:
meson setup build -Dgraphics_offload=enabled
meson compile -C build
```

`meson setup` prints `Graphics offload (video, experimental): true` in its
summary when the flag took effect.

## A/B measuring the benefit

The build compiles both paths; a single environment variable picks which one
runs, so you can compare without recompiling:

```bash
# Offloaded path (default when built with the flag):
NS_GRAPHICS_OFFLOAD=1 GDK_DEBUG=offload ./build/nordstjernen <video-page>

# Cairo path (baseline):
NS_GRAPHICS_OFFLOAD=0 ./build/nordstjernen <video-page>
```

- `GDK_DEBUG=offload` makes GTK log whether each frame was actually offloaded to
  a subsurface (vs. fell back to in-process compositing) — check this first, the
  benefit only exists when offload actually happens.
- `NS_DBG_COMPOSITE=1` prints a once-a-second line from the shell: `[offload]
  presented=N/s …` on the offload path, `[composite] drawn=…` on the Cairo path.
- Compare **CPU usage** of the `nordstjernen` process (e.g. `top`/`pidstat`) and,
  on a laptop, **power draw** (`powertop`, `turbostat`) during steady-state
  fullscreen video playback. Offload should show noticeably lower CPU/GPU
  compositing cost when the compositor takes the plane.

## Where it lives

- `meson_options.txt` — `graphics_offload` feature (default `disabled`).
- `meson.build` — sets `have_graphics_offload` (GTK ≥ 4.14) → `-DNS_HAVE_GRAPHICS_OFFLOAD=1`.
- `src/gtk/procview.c` — `pv_offload_update()` / `pv_offload_active()` and the
  `GtkGraphicsOffload` overlay built in `ns_proc_view_new()`. The Cairo path in
  `on_draw()` is untouched and used whenever offload is inactive.

## Follow-ups

1. Export **dmabuf** frames from `nordstjernen-video` (VA-API) and feed
   `GdkDmabufTexture` for true zero-copy — the biggest additional win.
2. Extend the same offload treatment to WebGL/canvas surfaces.
3. Once measured and stable, consider flipping the default to `auto`.
