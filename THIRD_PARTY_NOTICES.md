# Third-party notices

X360PS5's new code is GPL-3.0-or-later. Upstream components keep their licenses.

- **Xenia Canary**, Ben Vanik and contributors: BSD license in `.deps/xenia/LICENSE`.
- **PS5 Native App Boilerplate**, BlackBearReloaded: GPL-3.0-or-later. The generated
  Canvas/VideoOut source is derived from the pinned `src/demo_renderer.*`, changing
  only the refresh loop. Upstream attribution remains in the generated source.
- **PS5_Vulkan**, Mihawk-99 and contributors: GPL-3.0-or-later, with its upstream
  notices, Mesa licenses and native tooling's attribution retained.
- **PS5_Mesa**, Mesa contributors and Mihawk-99: per-file licenses in its source.
- **PS5 payload SDK**, ps5-payload-dev and Mihawk-99: its COPYING and per-file licenses.
- **PS5CEMU**: consulted for public API declarations, pad layout and integration;
  no emulator core or artwork copied. **XPSemu** and **PS5SX2** are references only.

Exact upstream revisions and URLs are in `dependencies.lock.json`. Build scripts
retain corresponding upstream source in `.deps`. Do not distribute a linked
binary alone: provide the corresponding source, modifications and build scripts,
and retain the upstream license texts. This local package is for development;
GitHub publication was subsequently authorized by the project owner.

`make package` creates a separate corresponding-source archive alongside the
application ZIP. Keep both and SHA256SUMS together when handing off this build.
The source archive includes pinned upstream trees and the initialized Xenia
submodules with their license texts. Compiler/system-library packages are
identified by the Ubuntu setup recipe and local toolchain evidence.
