# CrossHarbor

Community-focused firmware fork for the Xteink X3/X4 based on **[CrossInk](https://github.com/uxjulia/crossink)**, with selected ideas from **[CrossCover](https://github.com/StefanTsonev/CrossCover)** for integration with **[Hardcover.app](https://hardcover.app/)**.

## Goal and scope

CrossHarbor aims to keep the upstream CrossInk reading experience and stability, while adding practical Hardcover tracking workflows for daily use on constrained hardware.

In scope:
- Keep rebasing/syncing with mainline CrossInk.
- Keep changes narrow, readable, and maintainable.
- Prioritize features that materially improve focused reading on-device.
- Add lightweight Hardcover features (status/progress/rating/linking) that support reading flow without turning the firmware into a full social/discovery client.

Out of scope:
- Replacing the app architecture with a divergent codebase.
- Feature additions that add heavy always-on networking or broad non-reading surface area.
- Chasing every upstream difference from CrossCover.
- Heavy Hardcover social/discovery surfaces that do not directly improve the core reading loop.

## Roadmap (near-term)

- Home-screen quick Hardcover status action (without entering the reader menu).
- One-tap status cycle action for linked books (`Want → Reading → Paused → Read`).
- Lightweight offline sync queue for failed Hardcover writes, with retry when connectivity returns.
- Progress sync throttling to reduce network/memory churn while preserving useful tracking.
- Clear sync feedback in-reader (`queued`, `synced`, `failed`) without adding extra UI complexity.

## What this fork includes

### Core reader improvements retained from CrossInk
- Typography-focused defaults (ChareInk, Lexend Deca, Bitter) and extended font sizes.
- Reader QoL additions (book options, reading stats, controls customization, bookmarks/clippings, etc.).

### Hardcover integration (current)
- Settings entry for Hardcover API key import + authentication.
- Home screen entry for a Hardcover library view.
- In-reader Hardcover menu with:
  - Manual link by Book ID / ISBN
  - Automatic matching/linking
  - Mark currently reading
  - Mark read
  - Progress update
  - Rating
  - Auto-sync on reader exit with configurable threshold
- Reliability updates made during integration:
  - Reduced Hardcover request memory pressure in reader flow.
  - Avoided unnecessary section teardown/reindex when returning from Hardcover actions.
- Visual state indicators in the reader Hardcover menu:
  - `[x]` / `[ ]` next to **Mark Currently Reading** and **Mark Read** to show what was already set for the linked book.

## Hardcover quick setup

1. Put your token in:

```text
/.crosspoint/hardcover_token.txt
```

2. On device:

```text
Settings > Hardcover > Import API Key
Settings > Hardcover > Authenticate
```

3. Open a book, then:

```text
Reader Menu > Hardcover
```

## Build and flash

Build:

```sh
pio run -e default
```

Flash directly:

```sh
pio run -e default -t upload
```

Or use the generated `.bin` from `.pio/build/default/` with the web installer.

If submodules are missing, run:

```sh
git submodule sync --recursive
git submodule update --init --recursive
```

## Upstream and credits

- **CrossInk**: primary upstream firmware base and ongoing architecture/features.
- **CrossCover**: source reference for Hardcover integration patterns and related UX ideas.
- **CrossPoint Reader community** and contributors across all upstream projects.

CrossHarbor’s integration and iteration work was completed with assistance from **GitHub Copilot / AI tooling** (code exploration, implementation support, and debugging workflows).
