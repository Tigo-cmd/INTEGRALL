# Product Requirements Document: Integrall Landing Page

**Document Version:** 1.0
**Date:** 2026-03-09
**Author:** Engineering Team
**Status:** Draft

---

## 1. Overview

### 1.1 Purpose

Build a public-facing landing page and documentation site for the **Integrall IoT Framework**. The site will serve as the primary entry point for developers discovering, learning, and adopting Integrall. It must communicate the framework's value proposition clearly, host living documentation, announce releases, and funnel users toward adoption.

### 1.2 Problem Statement

Integrall currently has no web presence beyond its GitHub repository. The documentation lives in scattered markdown files (`GUIDE.md`, `README.md`, `DEPENDENCIES.md`) that are hard to navigate, lack search, and provide no visual hierarchy. New users have no clear onboarding path, and there is no mechanism to announce releases, showcase examples, or build community trust.

### 1.3 Product Vision

A fast, clean, developer-focused website at `integrall.io` that makes a first-time visitor understand what Integrall does within 10 seconds and have a working project within 10 minutes.

### 1.4 Target Audience

| Segment | Description | Primary Need |
|:--------|:------------|:-------------|
| **Hobbyists** | Makers building home automation, robots, gadgets | Quick start, copy-paste examples |
| **Students** | Engineering/CS students learning IoT | Clear tutorials, concept explanations |
| **Educators** | Teachers running embedded systems courses | Structured curriculum-ready docs |
| **Startup Engineers** | Prototyping IoT products quickly | Architecture docs, scalability info |
| **Open-Source Contributors** | Developers wanting to extend the framework | API internals, contribution guide |

---

## 2. Goals & Success Metrics

### 2.1 Goals

1. **Awareness**: Establish Integrall as a credible, professional IoT framework
2. **Adoption**: Convert visitors into users who install and build with the library
3. **Retention**: Provide documentation good enough that users never need to read source code
4. **Community**: Create a foundation for community growth (contributions, feedback, showcases)

### 2.2 Key Metrics

| Metric | Target (6 months post-launch) |
|:-------|:------------------------------|
| Monthly unique visitors | 5,000+ |
| Avg. time on docs pages | > 3 minutes |
| GitHub stars (attributed from site) | 500+ |
| Library installs (PlatformIO + Arduino) | 1,000+ |
| Documentation search usage | 40%+ of sessions |
| Bounce rate (homepage) | < 45% |

---

## 3. Site Architecture

### 3.1 Page Map

```
integrall.io/
├── /                           # Landing page (hero, features, CTA)
├── /docs/                      # Documentation hub
│   ├── /docs/getting-started/  # Installation & first project
│   │   ├── arduino-ide         # Arduino IDE setup
│   │   ├── platformio          # PlatformIO setup
│   │   └── first-project       # "Hello World" relay blink
│   ├── /docs/guides/           # Conceptual guides
│   │   ├── architecture        # How Integrall works internally
│   │   ├── offline-mode        # Using without WiFi
│   │   ├── wifi-backend        # Connecting to the backend
│   │   └── safety-features     # Timeouts, interlocks, debounce
│   ├── /docs/modules/          # Per-module reference
│   │   ├── relay
│   │   ├── lcd
│   │   ├── oled
│   │   ├── servo
│   │   ├── sensors
│   │   ├── keypad
│   │   ├── buzzer
│   │   ├── rgb
│   │   ├── camera
│   │   └── blinker
│   ├── /docs/projects/         # High-level project guides
│   │   ├── lock-system
│   │   ├── alarm-system
│   │   ├── parking-sensor
│   │   ├── weather-station
│   │   └── smart-switch
│   ├── /docs/backend/          # Backend documentation
│   │   ├── setup
│   │   ├── api-reference
│   │   ├── dashboard
│   │   └── deployment
│   └── /docs/api-reference/    # Full API reference (auto-generated)
├── /examples/                  # Interactive example gallery
├── /releases/                  # Changelog & release notes
├── /blog/                      # Announcements, tutorials, showcases
└── /community/                 # Links to GitHub, Discord, contributing
```

---

## 4. Page Requirements

### 4.1 Landing Page (`/`)

The homepage must convert a cold visitor into an interested user in a single scroll.

#### 4.1.1 Hero Section

- **Headline**: Short, punchy tagline conveying the core value (e.g., *"IoT development, simplified."* or *"Build IoT projects in minutes, not days."*)
- **Subheadline**: One sentence expanding on the what (e.g., *"A unified Arduino framework that turns complex hardware integrations into one-line function calls -- with an optional cloud backend included."*)
- **Primary CTA**: "Get Started" button -> `/docs/getting-started/`
- **Secondary CTA**: "View on GitHub" button -> GitHub repo link
- **Code preview**: Animated or static side-by-side comparison:
  - Left: "Standard Arduino" (15+ lines of boilerplate)
  - Right: "The Integrall Way" (3-5 clean lines)
- **Visual**: Subtle hardware illustration or animated circuit board motif (not a stock photo)

#### 4.1.2 Value Proposition Strip

Three cards showing core selling points:

1. **One-Line Hardware** -- "Control relays, LCDs, servos, and sensors with single function calls. No more boilerplate."
2. **Non-Blocking by Default** -- "Every operation uses millis() timing. Your WiFi, buttons, and sensors stay responsive."
3. **Cloud-Optional** -- "Works offline on Arduino Uno. Add WiFi and a FastAPI backend when you're ready to scale."

#### 4.1.3 Module Showcase

Interactive or tabbed section showing each hardware module with:
- Module icon/illustration
- 3-line code snippet showing the simplest usage
- "Standard Arduino" line count vs "Integrall" line count badge

Modules to feature: Relay, LCD, Servo, Sensors, Keypad, Camera, Buzzer, RGB LED

#### 4.1.4 Project Showcase

Cards for the 5 high-level project APIs, each showing:
- Project name and icon (Lock, Alarm, Parking, Weather, Smart Switch)
- "Lines of code" badge (e.g., "12 lines")
- Brief description
- Link to full guide

#### 4.1.5 Platform Support

Visual row of supported platforms with logos:
- ESP32 (primary)
- ESP8266 (supported)
- Arduino Uno/Nano/Mega (offline mode)
- PlatformIO compatible
- Arduino IDE compatible

#### 4.1.6 Quick Install

Tabbed code block:
- **PlatformIO** tab: `pio lib install "Integrall"` (or `lib_deps` entry)
- **Arduino IDE** tab: Library Manager search instructions
- **Manual** tab: ZIP download link

#### 4.1.7 Social Proof / Stats (post-launch, once data exists)

- GitHub stars count
- Total installs
- Number of supported modules
- Community size

#### 4.1.8 Footer

- Navigation links to all major sections
- GitHub, Discord/community links
- "Built by Emmanuel TIGO" credit
- MIT License badge
- Framework version display

---

### 4.2 Documentation Hub (`/docs/`)

#### Requirements

- **Left sidebar navigation** with collapsible sections matching the page map
- **Full-text search** across all documentation (client-side via Algolia DocSearch, Pagefind, or similar)
- **Breadcrumb navigation** at top of each page
- **"Edit on GitHub"** link on every page for community contributions
- **Previous / Next** navigation at bottom of each page
- **Table of contents** (right sidebar) auto-generated from headings on the current page
- **Code blocks** with syntax highlighting (C++, Python, bash) and one-click copy button
- **Version selector** dropdown (for future versions)
- **Dark mode** toggle (developers expect this)
- **Mobile responsive** with hamburger menu for sidebar

#### Content Migration

All existing documentation must be migrated and restructured:

| Source File | Destination |
|:------------|:------------|
| `GUIDE.md` (Getting Started) | `/docs/getting-started/` |
| `GUIDE.md` (Hardware Modules) | `/docs/modules/*` (split per module) |
| `GUIDE.md` (Lock/Alarm/etc.) | `/docs/projects/*` (split per project) |
| `GUIDE.md` (Backend section) | `/docs/backend/setup` |
| `README.md` (API Reference) | `/docs/api-reference/` |
| `README.md` (Safety Features) | `/docs/guides/safety-features` |
| `DEPENDENCIES.md` | `/docs/getting-started/` (merged into setup pages) |
| Inline code comments (`Integrall.h`) | `/docs/api-reference/` (auto-extracted) |

#### Each Module Page Must Include

1. **Overview** -- What the module does, what hardware it supports
2. **Requirements** -- The `#define` flag needed, external libraries to install, wiring diagram
3. **Quick Start** -- Minimal working example (< 15 lines)
4. **API Reference** -- Every public method with signature, parameters, return value, and description
5. **Advanced Usage** -- Edge cases, configuration options, raw object access
6. **Troubleshooting** -- Common errors and solutions
7. **Wiring Diagram** -- SVG or image showing physical connections

---

### 4.3 Examples Gallery (`/examples/`)

- Grid layout of all 15+ example sketches
- Each card shows: title, description, required modules (as colored badges), difficulty level (Beginner/Intermediate/Advanced)
- Click opens the full example with:
  - Complete source code with syntax highlighting
  - Wiring diagram
  - Required hardware list
  - Step-by-step walkthrough
  - "Download .ino" button
  - "Open in GitHub" link

---

### 4.4 Releases Page (`/releases/`)

#### Requirements

- Reverse-chronological list of all framework releases
- Each release entry includes:
  - **Version number** and **release date**
  - **Release type badge**: Major / Minor / Patch
  - **Changelog** in Keep a Changelog format:
    - Added (new features)
    - Changed (modifications)
    - Fixed (bug fixes)
    - Deprecated (upcoming removals)
    - Removed (deleted features)
    - Security (vulnerability fixes)
  - **Download links**: `.zip` for Arduino IDE, PlatformIO install command
  - **Migration guide** link (for breaking changes)
  - **Firmware + Backend** version pairing (which backend version is compatible)
- **RSS/Atom feed** for release notifications
- **"Subscribe to releases"** email opt-in (optional, phase 2)

#### Initial Release Entries to Create

Based on existing `.zip` archives in the repo:

| Version | Key Changes |
|:--------|:------------|
| v0.2.7 | Servo motor API, documentation updates |
| v0.2.4 | Keypad auto-lock, LCD enhancements, camera encapsulation |
| v0.2.2 | LCD scroll speed parameter |
| v0.2.1 | Smart text printing, scrolling, custom characters |
| v0.2.0 | WiFi module, LCD smart print, SerialMonitorLCD example |
| v0.1.0 | Initial release -- core framework, relay, backend |

---

### 4.5 Blog (`/blog/`)

- Simple blog layout for announcements, tutorials, and community showcases
- Each post has: title, date, author, tags, reading time estimate, cover image
- Categories: **Announcement**, **Tutorial**, **Showcase**, **Technical Deep Dive**
- Initial posts to create:
  1. *"Introducing Integrall: IoT Development, Simplified"* -- launch announcement
  2. *"Build a Smart Door Lock in 15 Lines of Code"* -- tutorial
  3. *"How Integrall's Non-Blocking Architecture Works"* -- technical deep dive

---

### 4.6 Community Page (`/community/`)

- Links to:
  - GitHub repository (issues, discussions, PRs)
  - Discord server (or Discussions link if Discord is not set up)
  - Contributing guide
  - Code of conduct
- **Showcase section**: User-submitted projects built with Integrall (phase 2)
- **"Built with Integrall"** badge generator for READMEs

---

## 5. Design Requirements

### 5.1 Brand Identity

| Element | Specification |
|:--------|:--------------|
| **Primary Color** | Deep purple-blue gradient (matches dashboard: `#667eea` -> `#764ba2`) |
| **Accent Color** | Electric green for CTAs and success states (`#68d391`) |
| **Background** | White / very light gray (`#f7fafc`) for content areas |
| **Dark Mode BG** | `#1a202c` (matches dashboard log panel) |
| **Typography** | System font stack: `-apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif` |
| **Code Font** | `'JetBrains Mono', 'Fira Code', 'Monaco', monospace` |
| **Logo** | Text-based wordmark "INTEGRALL" (all caps, tracked) with circuit-node accent |
| **Tone** | Technical but approachable. Direct. No fluff. Shows code, not marketing speak. |

### 5.2 Design Principles

1. **Code-first**: Every feature section leads with a code snippet, not prose
2. **Scannable**: Heavy use of headings, tables, badges, and whitespace
3. **Fast**: Target < 2s LCP on 3G. Minimal JavaScript. No heavy frameworks on landing page.
4. **Accessible**: WCAG 2.1 AA compliance. Proper contrast ratios, semantic HTML, keyboard navigation

### 5.3 Responsive Breakpoints

| Breakpoint | Target |
|:-----------|:-------|
| `>= 1280px` | Desktop (3-column docs layout: sidebar + content + TOC) |
| `768px - 1279px` | Tablet (2-column: sidebar + content, TOC collapsed) |
| `< 768px` | Mobile (single column, hamburger nav) |

---

## 6. Technical Requirements

### 6.1 Recommended Tech Stack

| Layer | Technology | Rationale |
|:------|:-----------|:----------|
| **Framework** | Astro or Next.js (static export) | Fast static site generation with MDX support |
| **Docs Engine** | Starlight (Astro) or Nextra (Next.js) | Purpose-built docs framework with search, sidebar, versioning |
| **Styling** | Tailwind CSS | Utility-first, matches existing dashboard aesthetic |
| **Code Highlighting** | Shiki or Prism | Server-rendered syntax highlighting (zero JS) |
| **Search** | Pagefind (self-hosted) or Algolia DocSearch (free for OSS) | Full-text documentation search |
| **Hosting** | Vercel, Netlify, or GitHub Pages | Free for open-source, automatic deploys from `main` |
| **CMS (optional)** | Markdown files in repo (Git-based) | No external CMS needed; docs live alongside code |
| **Analytics** | Plausible or Umami (privacy-first) | No cookie banners needed |

### 6.2 Content Format

- All documentation authored in **MDX** (Markdown + JSX components)
- Stored in a `/docs` or `/website` directory in the Integrall repo
- Auto-deployed on push to `main` branch
- Code examples pull from actual example files in `firmware/Integrall/examples/` where possible (single source of truth)

### 6.3 Performance Targets

| Metric | Target |
|:-------|:-------|
| Lighthouse Performance | >= 95 |
| Lighthouse Accessibility | >= 95 |
| First Contentful Paint | < 1.0s |
| Largest Contentful Paint | < 2.0s |
| Total Blocking Time | < 100ms |
| Cumulative Layout Shift | < 0.05 |
| Page weight (landing) | < 500KB |
| Page weight (docs page) | < 300KB |

### 6.4 SEO Requirements

- Semantic HTML5 structure (`<article>`, `<nav>`, `<aside>`, `<section>`)
- Unique `<title>` and `<meta description>` per page
- Open Graph + Twitter Card meta tags on all pages
- Structured data (`WebSite`, `SoftwareApplication`, `TechArticle` schemas)
- Canonical URLs
- Sitemap.xml auto-generated
- robots.txt
- Heading hierarchy (single `<h1>` per page)

---

## 7. Content Requirements

### 7.1 Landing Page Copy

Must communicate the following in order:

1. **What it is**: "An Arduino/ESP32 library that simplifies IoT hardware and cloud connectivity"
2. **Why it matters**: "Turns 50+ lines of boilerplate into 1-line calls"
3. **What it supports**: 10 hardware modules, 5 project templates, ESP32 + Arduino
4. **How to start**: Install in 2 minutes, build something in 10

### 7.2 Documentation Writing Style

- **Imperative mood**: "Install the library" not "You should install the library"
- **Show, don't tell**: Every concept explained with a code example
- **Progressive disclosure**: Start simple, link to advanced topics
- **Platform-aware**: Mark ESP32-only or Arduino-only features clearly with badges
- **Copy-pasteable**: Every code example must compile as-is (no `...` placeholders without context)

### 7.3 Required Visual Assets

| Asset | Format | Usage |
|:------|:-------|:------|
| Logo (full) | SVG + PNG | Header, social sharing |
| Logo (icon) | SVG + ICO + PNG (favicon) | Browser tab, mobile icon |
| Module icons (x10) | SVG | Module showcase cards |
| Project icons (x5) | SVG | Project showcase cards |
| Platform logos | SVG | Platform support strip |
| Wiring diagrams (per module) | SVG or PNG | Documentation pages |
| OG image | PNG 1200x630 | Social media sharing |
| Hero illustration | SVG | Landing page hero |

---

## 8. Phases & Milestones

### Phase 1: Foundation (MVP)

**Goal**: Ship a functional landing page and migrated documentation.

| Deliverable | Description |
|:------------|:------------|
| Landing page | Hero, features, module showcase, install, footer |
| Docs: Getting Started | Arduino IDE + PlatformIO setup, first project |
| Docs: All 10 modules | Migrated from GUIDE.md, one page each |
| Docs: 5 project guides | Lock, Alarm, Parking, Weather, Smart Switch |
| Releases page | Retroactive entries for v0.1.0 through v0.2.7 |
| Search | Basic full-text search across docs |
| Dark mode | Toggle with system preference detection |
| Mobile responsive | All pages functional on mobile |
| Deployment | Auto-deploy from repo to hosting provider |

### Phase 2: Polish

**Goal**: Improve discoverability, engagement, and developer experience.

| Deliverable | Description |
|:------------|:------------|
| Examples gallery | Interactive grid with filters and wiring diagrams |
| Blog | Launch post + 2 tutorials |
| API reference (auto-generated) | Extracted from source code comments |
| Backend docs | Setup, API reference, deployment guide |
| Analytics | Privacy-first analytics integration |
| SEO optimization | Structured data, OG tags, sitemap |
| Version selector | Dropdown for docs versioning |
| RSS feed | For releases and blog posts |

### Phase 3: Community

**Goal**: Build community engagement and contribution pathways.

| Deliverable | Description |
|:------------|:------------|
| Community page | GitHub, Discord, contributing links |
| Project showcase | User-submitted "Built with Integrall" gallery |
| Interactive playground | Browser-based code editor with Wokwi simulator integration |
| Localization | i18n framework for French (primary non-English audience) |
| Newsletter | Release announcement email list |
| "Built with Integrall" badge | Embeddable SVG badge for user READMEs |

---

## 9. Open Questions

| # | Question | Decision Needed By |
|:--|:---------|:-------------------|
| 1 | **Static site generator**: Astro (Starlight) vs Next.js (Nextra)? Astro is lighter; Next.js has larger ecosystem. | Phase 1 kickoff |
| 2 | **Domain**: Is `integrall.io` already registered and configured? | Phase 1 kickoff |
| 3 | **Wiring diagrams**: Use Fritzing exports, hand-drawn SVGs, or a tool like Wokwi? | Phase 1 (module docs) |
| 4 | **Search provider**: Self-hosted Pagefind (free, private) vs Algolia DocSearch (free for OSS, better UX)? | Phase 1 |
| 5 | **Community platform**: GitHub Discussions vs Discord vs both? | Phase 3 |
| 6 | **Auto-generated API docs**: Parse `Integrall.h` comments with Doxygen, or manually maintain? | Phase 2 |
| 7 | **Backend docs scope**: Document the backend API for end-users, or also for contributors extending it? | Phase 2 |

---

## 10. Dependencies & Risks

### Dependencies

- Logo and brand assets must be finalized before Phase 1 launch
- Domain DNS must be configured for hosting provider
- Existing GUIDE.md content must be reviewed for accuracy before migration
- Example sketches must be verified to compile before publishing

### Risks

| Risk | Impact | Mitigation |
|:-----|:-------|:-----------|
| Documentation falls out of sync with code | Users get errors following outdated guides | CI check that verifies code examples compile; pull examples from source files |
| Site complexity delays launch | No web presence for longer | Phase 1 is deliberately minimal; ship fast, iterate |
| Low traffic post-launch | Low adoption | Submit to Arduino/PlatformIO library registries, post on Reddit/HN, write dev.to articles |
| Scope creep on interactive features | Delays core docs work | Playground and showcase are Phase 3; Phase 1 is static content only |

---

## 11. Acceptance Criteria (Phase 1 MVP)

The Phase 1 launch is complete when:

- [ ] Landing page loads in < 2s on 3G with Lighthouse score >= 90
- [ ] A new user can go from landing page to a compiled "blink relay" sketch in under 10 minutes
- [ ] All 10 module docs pages exist with at least: overview, quick start code, and API table
- [ ] All 5 project guide pages exist with complete sketches
- [ ] Releases page lists all historical versions with changelogs
- [ ] Full-text search returns relevant results for queries like "servo", "lock system", "WiFi setup"
- [ ] Dark mode works correctly on all pages
- [ ] Site is fully navigable on mobile (320px width)
- [ ] "Edit on GitHub" links on every docs page point to correct source files
- [ ] Site auto-deploys from the repository on push to `main`
