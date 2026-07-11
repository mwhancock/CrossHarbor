# CrossHarbor Codebase Walkthrough

This is a practical guide to help you get oriented in a firmware codebase that is bigger and lower-level than a typical app project.

The short version:

- **This repo is an e-reader firmware app** for the Xteink X3/X4.
- **`src/` is the app layer**: screens, settings, stores, UI, startup flow.
- **`lib/` is the reusable engine layer**: EPUB parsing, rendering, storage wrappers, networking helpers, fonts, i18n, utilities.
- **`open-x4-sdk/` is the hardware/platform dependency** pulled in underneath the app.
- **The most important architectural pattern is "activity + manager"**: each screen is an `Activity`, and `ActivityManager` runs the app like a UI state machine.

If you are brand new to embedded/C++, do **not** try to understand everything at once. Start with the startup flow, then navigation, then one concrete feature path like "open a book".

---

## 1. Big-picture mental model

Think of the repo as four layers:

| Layer | What it is | Where to look |
| --- | --- | --- |
| Hardware / SDK | GPIO, display, SD card, power, clock, input drivers | `open-x4-sdk/`, `lib/hal/` |
| Core engine | Rendering, book parsing, file helpers, caches, i18n, networking helpers | `lib/` |
| App shell | Startup, global state, settings, recent books, UI theme, screen manager | `src/main.cpp`, `src/*.h`, `src/activities/`, `src/components/` |
| Feature screens | Home, reader, settings, OPDS, OTA, web server, Hardcover, KOReader sync | `src/activities/*`, `src/network/` |

That means:

- **`lib/` answers "how do we do this?"**
- **`src/` answers "when do we do this, and what screen/workflow owns it?"**

This split is not perfectly clean yet, but it is a useful way to read the project.

---

## 2. How the firmware starts up

Your first stop should be:

- `src/main.cpp`
- `src/activities/ActivityManager.h`
- `src/activities/ActivityManager.cpp`
- `docs/activity-manager.md`

### What `main.cpp` does

`src/main.cpp` is the firmware entry point. It defines Arduino-style:

- `setup()` - one-time boot logic
- `loop()` - the main event loop

At a high level, `setup()` does this:

1. Initializes serial logging and platform state.
2. Initializes hardware wrappers like GPIO, power, tilt sensor, clock, and storage.
3. Loads persistent data:
   - settings
   - app state
   - recent books
   - KOReader credentials
   - Hardcover credentials
   - OPDS servers
4. Sets up display + fonts.
5. Decides which first screen to show:
   - boot splash
   - home
   - crash screen
   - reader resume
   - firmware recovery mode

Then `loop()` becomes the app's heartbeat:

1. Poll input and sensors.
2. Update power-saving state.
3. Handle special shortcuts like screenshots.
4. Run the current activity's logic.
5. Render if needed.
6. Auto-sleep when idle.

### Why `main.cpp` looks intimidating

It is doing a lot of startup orchestration, and it also contains a huge font registration section. That makes it look scarier than it really is.

Break it into:

- **boring setup glue**: serial, hardware init, loading stores
- **important boot policy**: resume vs splash vs crash vs home
- **font setup boilerplate**: long but repetitive

The font declarations and registrations are mostly **mechanical data wiring**, not the best place to begin learning the codebase.

---

## 3. The core app pattern: Activities

If you understand activities, the whole repo gets much easier.

### The basic idea

An `Activity` is one UI screen or workflow step:

- home screen
- file browser
- settings screen
- reader
- OTA updater
- sleep screen

Key files:

- `src/activities/Activity.h`
- `src/activities/ActivityManager.h`
- `src/activities/ActivityManager.cpp`
- `docs/activity-manager.md`

### What an activity does

An activity usually owns:

- its screen state
- button handling
- screen-specific rendering
- transitions to other activities

The base lifecycle methods are:

- `onEnter()`
- `loop()`
- `render(RenderLock&&)`
- `onExit()`

### What `ActivityManager` does

`ActivityManager` is the navigation and render coordinator.

It:

- keeps track of the current screen
- manages a stack of previous screens
- handles push/pop/replace navigation
- owns the shared render task
- queues and triggers screen updates safely

This is one of the more interesting parts of the repo because it touches:

- UI architecture
- memory constraints
- concurrency
- FreeRTOS task coordination

### Why this matters on embedded hardware

On desktop/web, you can often be lazy about screens and state. Here, every extra task and buffer costs real RAM.

The current model centralizes rendering in one manager instead of giving every activity its own render task. That is a classic embedded tradeoff: **less flexibility, much lower memory cost and lower lifecycle risk**.

---

## 4. The top-level `src/` areas

This is the "application" side of the codebase.

### `src/activities/`

This is the screen/workflow layer.

Subfolders:

- `boot_sleep/` - boot splash, sleep screen, wake/sleep transitions
- `browser/` - OPDS browsing
- `home/` - home screen, recents, file browser, alerts, crash UI
- `network/` - UI screens around connectivity and transfer
- `reader/` - reading flows, reader menu, chapter/bookmark/clipping/stats sub-screens
- `settings/` - all device and app settings screens
- `util/` - generic reusable UI activities like confirmations or option pickers

This is **high-value reading** because it shows how features are assembled into actual UX.

### `src/components/`

Reusable UI pieces and theming:

- `UITheme.*` - theme selection and theme helpers
- `HeaderDate.*` - shared header rendering piece
- `themes/` - concrete theme implementations
- `icons/` - icon assets as C/C++ headers

This is where you learn how the app tries to keep visual behavior consistent.

### `src/network/`

Feature-specific network helpers:

- `CrossPointWebServer.*` - device-hosted web UI / transfer server
- `FirmwareFlasher.*`
- `HttpDownloader.*`
- `OtaUpdater.*`
- `WebDAVHandler.*`
- `html/` - generated web assets and handlers

This folder is useful when you want to understand web transfer and OTA behavior end-to-end.

### `src/platform/`

Tiny platform-specific shims that do not fit cleanly elsewhere.

Example:

- `skip_efuse_blk_check.c` overrides a bootloader check for a device-specific compatibility reason.

This is **not** where the main app abstractions live; it is more of a "special cases" bucket.

### Root files in `src/`

These are shared app-level state/configuration helpers:

- `CrossPointSettings.*` - user/device settings model
- `CrossPointState.*` - current app/session-ish state
- `RecentBooksStore.*` - recent book list persistence
- `MappedInputManager.*` - logical button mapping
- `JsonSettingsIO.*` - persistence glue
- `BookmarkStore.*`, `ClippingStore.*`, `WifiCredentialStore.*`, `OpdsServerStore.*`
- `FontInstaller.*`, `SdCardFontSystem.*`

These are important because they connect UI behavior to persistent data.

---

## 5. The top-level `lib/` areas

This is the reusable engine/toolbox layer. Some of it is app-specific, some is closer to general-purpose infrastructure.

### `lib/hal/`

This is the **hardware abstraction layer** the app should use instead of directly talking to lower SDK classes.

Key modules:

- `HalStorage` - SD card file operations
- `HalDisplay` - e-ink display wrapper
- `HalGPIO` - buttons and wake reasons
- `HalPowerManager` - power-saving behavior
- `HalClock` - time
- `HalSystem` - platform/system helpers
- `HalTiltSensor` - tilt-based page turn input

Why it matters:

- it isolates hardware-specific behavior
- it makes simulator vs hardware differences easier to contain
- it prevents app code from depending directly on raw SDK details

**Learn this early.** In this repo, a lot of design discipline flows through HAL boundaries.

### `lib/GfxRenderer/`

This is the custom rendering engine for the e-ink screen.

Responsibilities include:

- frame buffer management
- text measurement and drawing
- line/rect/bitmap drawing
- orientation transforms
- grayscale/BW rendering modes
- font registration and glyph access

This is one of the most important technical subsystems in the project.

Why it is interesting:

- e-ink has unusual refresh behavior
- frame buffers are large for this device
- text layout and bitmap handling are central to the reading experience

If you want to understand the "graphics engine" of the firmware, this is it.

### `lib/Epub/`

This is the EPUB reading pipeline.

It covers:

- ZIP access
- EPUB metadata
- TOC and spine parsing
- HTML parsing
- CSS parsing
- text blocks / image blocks
- section caching
- thumbnail/cover generation
- reading progress math

This is probably the most complex product-specific subsystem in the repo.

If your goal is to work on reader features, this is a major area to study.

### `lib/Xtc/`

Support for the XTC ebook format, with an interface intentionally similar to `Epub`.

This is helpful to compare against EPUB because it often shows:

- what is reader-generic
- what is format-specific

### `lib/Txt/`

Plain-text book support.

Usually smaller and easier to understand than EPUB, so it can be a good stepping stone before reading the EPUB pipeline.

### `lib/ZipFile/`

ZIP reading helper used by EPUB.

Why it matters:

- EPUB is a ZIP container
- streaming and size lookup matter for memory
- you can see how the code avoids expensive full extraction

### `lib/InflateReader/`, `lib/PngToBmpConverter/`, `lib/JpegToBmpConverter/`

Helpers for decompressing and converting data into forms the renderer can use.

These are good examples of "embedded plumbing": not flashy, but essential.

### `lib/EpdFont/`

Font handling:

- font data
- font families
- SD card font support
- decompression
- generated built-in font headers

Much of this folder is **data-heavy and somewhat boilerplate-ish**, especially generated font assets, but the management code is important for understanding text rendering and memory use.

### `lib/I18n/`

Localization infrastructure.

Source-of-truth data:

- `lib/I18n/translations/*.yaml`

Generated code:

- `I18nKeys.h`
- `I18nStrings.h`
- `I18nStrings.cpp`

This is more "build/data pipeline" than runtime complexity, but it is important for any user-visible text changes.

### `lib/Hardcover/`

Integration layer for Hardcover.app:

- auth
- searching
- library fetch
- status/progress/rating updates

This is one of the main fork-specific feature areas.

### `lib/KOReaderSync/`

Sync integration with KOReader's progress server.

Responsibilities:

- auth
- document IDs
- progress fetch/push
- JSON I/O helpers
- mapping between internal progress and sync format

### `lib/OpdsParser/`

Parser for OPDS catalog feeds.

This powers network book browsing and is a good example of:

- incremental XML parsing
- fixed-capacity-ish embedded-friendly parsing design

### `lib/JsonParser/`

Small specialized JSON parsers, including OTA release parsing.

This is a good place to study small, purpose-built parsing code instead of giant generic object models.

### `lib/FileIndex/`

Directory indexing and lookup helper.

Useful for understanding how the repo tries to keep file browsing fast without keeping everything in memory.

### `lib/FsHelpers/`

General file/path utilities:

- extension checks
- sorting
- path normalization
- FAT32-safe naming

Often not exciting, but very reusable and worth knowing exists before writing new helpers.

### `lib/Serialization/`

Binary read/write helpers for persistent data formats.

This helps explain how caches and state are stored compactly on disk.

### `lib/Utf8/`, `lib/MiniBidi/`

Text shaping/support utilities:

- UTF-8 traversal and safe truncation
- Unicode composition
- bidi helpers for RTL/LTR languages

This is a great example of code that looks "small utility-ish" but is actually product-critical for multilingual text.

### `lib/Memory/`, `lib/MemoryBudget/`

Helpers for safer memory usage on a device without much RAM.

This is important because this firmware is built with exceptions disabled, so allocation failure handling needs to be explicit.

### `lib/Logging/`

Logging macros and helpers.

This is supporting infrastructure; worth knowing, but not a good first deep dive.

### Third-party / bundled code

- `lib/expat/` - XML parser
- `lib/uzlib/` - compression/decompression

Usually treat these as dependencies, not as places to start learning the app.

---

## 6. What is "interesting engineering" vs "mostly boilerplate"

This is important because large repos can waste your attention.

### High-value, interesting code

These are the places most likely to teach you how the firmware really works:

1. `src/main.cpp` - startup policy, lifecycle, hardware/init sequence
2. `src/activities/Activity*` - app navigation architecture
3. `lib/hal/*` - hardware abstraction patterns
4. `lib/GfxRenderer/*` - rendering model
5. `lib/Epub/*` - core reader pipeline
6. `src/activities/reader/*` - real reading UX behavior
7. `src/components/UITheme.*` and theme files - how UI consistency is enforced
8. `src/MappedInputManager.*` - logical input design
9. `src/*Store*` + `lib/Serialization/*` - persistence/caching model

### Mostly mechanical / data-heavy / boilerplate

These are often useful, but not the best use of your first learning hours:

1. Huge font declaration blocks in `src/main.cpp`
2. Generated assets under font folders and `src/components/icons/`
3. Generated i18n output files
4. Generated web headers / web assets
5. Repetitive enum declarations in settings files
6. Vendor code like `expat` and `uzlib`

That does **not** mean they are unimportant. It means they are usually:

- repetitive
- generated
- data-oriented
- easier to learn later, once the architecture makes sense

---

## 7. What to learn first, in order

If I were ramping up from scratch, I would learn in this order.

### Phase 1: learn the app shell

Read:

1. `README.md`
2. `platformio.ini`
3. `src/main.cpp`
4. `src/activities/Activity.h`
5. `src/activities/ActivityManager.h`
6. `docs/activity-manager.md`

Goal:

- understand boot flow
- understand what an activity is
- understand how the main loop drives the app

### Phase 2: learn state + UI primitives

Read:

1. `src/CrossPointSettings.h`
2. `src/CrossPointState.h`
3. `src/MappedInputManager.h`
4. `src/components/UITheme.h`
5. `lib/GfxRenderer/GfxRenderer.h`
6. `lib/hal/HalStorage.h`

Goal:

- understand settings vs session/app state
- understand logical input mapping
- understand how drawing happens
- understand where file I/O should go through

### Phase 3: follow one simple user flow

A good first flow is:

- boot
- home screen
- open recent book
- enter reader

Files to trace:

1. `src/main.cpp`
2. `src/activities/home/HomeActivity.*`
3. `src/activities/reader/ReaderActivity.*`
4. `lib/Epub/Epub.h` and adjacent reader helpers

Goal:

- see how real features compose across app + library layers

### Phase 4: study persistence and cache behavior

Read:

1. `docs/data-cache.md`
2. `src/RecentBooksStore.*`
3. `src/BookmarkStore.*`
4. `src/ClippingStore.*`
5. `lib/Serialization/*`

Goal:

- understand what lives in RAM vs SD card
- understand why caching is so important here

### Phase 5: pick one specialized subsystem

Choose one:

- EPUB pipeline
- rendering
- OTA
- OPDS
- Hardcover integration
- KOReader sync

Trying to learn all of them at once is where newcomers usually stall out.

---

## 8. How to think about memory and "low-level" concerns here

This repo is not kernel-level low-level, but it **is** embedded low-level in the sense that:

- RAM is tight
- storage is slower and more constrained
- background tasks cost real stack memory
- allocation failures matter
- full-screen redraws are expensive
- hardware wake/sleep state matters

When reading code, keep asking:

1. **Where does this data live?**
   - stack?
   - heap?
   - flash/const data?
   - SD card cache?

2. **How long does it live?**
   - one function call?
   - one activity?
   - persistent across reboots?

3. **What wakes or redraws the screen?**

4. **Is this code trying to avoid RAM churn?**

5. **Is this generic logic, or device-policy logic?**

That mindset is more valuable than memorizing every class name.

---

## 9. A practical reading map for this specific repo

Here is a concrete "tour route" through the codebase.

### Route A: fastest way to understand the app

1. `README.md`
2. `platformio.ini`
3. `src/main.cpp`
4. `docs/activity-manager.md`
5. `src/activities/ActivityManager.cpp`
6. `src/activities/home/HomeActivity.*`
7. `src/activities/reader/ReaderActivity.*`

### Route B: understand rendering/UI

1. `src/components/UITheme.*`
2. `src/components/themes/*`
3. `lib/GfxRenderer/GfxRenderer.h`
4. `lib/EpdFont/*`
5. `src/components/icons/*`

### Route C: understand file/book handling

1. `lib/hal/HalStorage.h`
2. `lib/FsHelpers/*`
3. `lib/FileIndex/*`
4. `lib/ZipFile/*`
5. `lib/Epub/*`
6. `docs/data-cache.md`

### Route D: understand networking/integration work

1. `src/network/*`
2. `lib/Hardcover/*`
3. `lib/KOReaderSync/*`
4. `lib/OpdsParser/*`
5. `docs/webserver.md`
6. `docs/webserver-endpoints.md`
7. `docs/reading-stats-sync.md`

---

## 10. Where app state lives

There are a few different kinds of state here, and separating them in your head helps a lot.

### Settings

`src/CrossPointSettings.*`

This is user/device configuration:

- theme
- fonts
- orientation
- button mapping
- sleep behavior
- status bar options
- network-related preferences

Think: **preferences/config**.

### App state

`src/CrossPointState.*`

This is "what is going on right now / what should resume":

- open book path
- sleep image state
- pending alert
- some reader resume fields

Think: **session-ish state and resume state**.

### Stores

Examples:

- `RecentBooksStore`
- `BookmarkStore`
- `ClippingStore`
- `WifiCredentialStore`
- `OpdsServerStore`

These are persistent data holders for specific features.

Think: **feature-specific saved data**.

### Cache

See `docs/data-cache.md`.

This is derived data stored to avoid expensive recomputation:

- parsed metadata
- thumbnails
- reader layout sections
- generated covers

Think: **rebuildable performance data**.

---

## 11. Why there are so many file formats and generated artifacts

This codebase is doing a lot on limited hardware, so it uses code generation and binary cache files to move work out of the hot path.

Examples:

- generated i18n lookup tables
- generated font data headers
- generated web portal assets
- binary book caches
- thumbnails and covers on SD card

That is normal for embedded firmware.

The common pattern is:

1. do work once
2. store a compact/generated result
3. reuse it later to save RAM/CPU/time

---

## 12. What parts are closest to "normal app dev" vs embedded-specific

### Most familiar if you come from web/app dev

- activity/screen flows
- settings/state stores
- feature integrations like Hardcover
- simple HTTP client logic
- generated web portal assets
- theming/UI consistency work

### More embedded-specific

- HAL wrappers
- wake/sleep behavior
- render task + mutex coordination
- e-ink refresh strategy
- explicit memory-failure handling
- cache-on-SD instead of keeping everything in RAM
- avoiding expensive allocations in inner loops

If you want quick wins, start with the first group before diving deep into the second.

---

## 13. Good internal docs to read next

This repo already has useful docs. Read these before spelunking random files:

- `docs/activity-manager.md`
- `docs/data-cache.md`
- `docs/file-formats.md`
- `docs/simulator.md`
- `docs/i18n.md`
- `docs/reader-features.md`
- `docs/webserver.md`
- `docs/webserver-endpoints.md`
- `docs/sd-card-fonts.md`
- `docs/troubleshooting.md`

These will often save you an hour of source diving.

---

## 14. External references worth learning from

Here are the best "background knowledge" references for this repo.

### C++ basics you will actually use here

- RAII: <https://en.cppreference.com/w/cpp/language/raii>
- `std::unique_ptr`: <https://en.cppreference.com/w/cpp/memory/unique_ptr>
- `std::string_view`: <https://en.cppreference.com/w/cpp/string/basic_string_view>
- move semantics overview: <https://en.cppreference.com/w/cpp/utility/move>

### Embedded / RTOS / ESP32

- PlatformIO docs: <https://docs.platformio.org/>
- Arduino-ESP32 docs: <https://docs.espressif.com/projects/arduino-esp32/en/latest/>
- ESP-IDF programming guide: <https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/>
- FreeRTOS task basics: <https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/01-Tasks-and-co-routines/01-Tasks>
- FreeRTOS synchronization primitives: <https://www.freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores>

### Parsing / formats relevant here

- EPUB overview: <https://www.w3.org/publishing/epub32/>
- Expat XML parser docs: <https://libexpat.github.io/>
- JSON streaming parser design background: <https://www.json.org/json-en.html>

### Text/layout topics relevant here

- UTF-8 primer: <https://utf8everywhere.org/>
- Unicode Bidirectional Algorithm overview: <https://unicode.org/reports/tr9/>

---

## 15. What I would personally treat as "do not overthink this yet"

When ramping up, I would **not** spend much early time on:

- generated font data
- vendor libraries
- every settings enum value
- every icon asset
- every simulator quirk
- every network integration detail

Those are all real, but they are not the best first leverage.

First get comfortable with:

- boot flow
- activity navigation
- rendering model
- one reader path
- one persistence path

Once those click, the rest of the repo becomes much easier to classify.

---

## 16. A suggested first week of learning

If you want a practical ramp-up plan:

### Day 1

- Read `README.md`
- Read `platformio.ini`
- Skim `src/main.cpp`

### Day 2

- Read `docs/activity-manager.md`
- Read `src/activities/Activity.h`
- Read `src/activities/ActivityManager.*`

### Day 3

- Read `src/CrossPointSettings.h`
- Read `src/CrossPointState.h`
- Read `src/MappedInputManager.h`
- Read `src/components/UITheme.h`

### Day 4

- Read `lib/hal/HalStorage.h`
- Read `lib/GfxRenderer/GfxRenderer.h`
- Read `docs/data-cache.md`

### Day 5

- Trace one feature path:
  - home -> file browser -> reader
  - or home -> settings -> OTA

### Day 6+

- Pick one subsystem to go deeper on:
  - EPUB
  - renderer
  - Hardcover
  - KOReader sync
  - OPDS

---

## 17. Final takeaway

You do **not** need to understand this repo all at once.

The key is to recognize the categories:

- **app flow** - `src/activities/`, `main.cpp`
- **persistent state** - settings, state, stores
- **shared UI/rendering** - themes, renderer, input mapping
- **book engine** - EPUB/XTC/TXT and caches
- **hardware boundary** - HAL + SDK
- **supporting infrastructure** - i18n, serialization, fonts, tests, scripts

Once you can look at a file and say **"this is app flow"**, **"this is rendering infrastructure"**, or **"this is cache/persistence glue"**, the codebase stops feeling like one giant wall of C++ and starts feeling organized.

That is the real milestone to aim for first.
