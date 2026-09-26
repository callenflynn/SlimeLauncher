---
name: This wiki
description: How the Slime Launcher wiki is built with nsdocs
---
[← SLIME LAUNCHER](../)


# This wiki

The Slime Launcher wiki is generated with [nsdocs](https://github.com/CStaks/nsDocs) — a zero-dependency static wiki builder (one Python file, Markdown in, plain HTML out).

## Layout

The wiki is served under `/docs/` on the project site, behind the minimalist landing page at the root (`docs/site/index.html`). All wiki links are relative, so the same tree works locally and on GitHub Pages.

```text
nsdocs.yml                 ← wiki configuration (nav, branding, features)
docs/
├── site/                  ← landing page served at the site root (/)
│   ├── index.html
│   └── slime.png
├── wiki-src/              ← Markdown sources (edit these)
│   ├── architecture.md
│   └── …
└── wiki/                  ← generated HTML (committed, do not hand-edit)
                            ← deployed to <site>/docs/ by the wiki workflow
```

Every page starts with a `[← SLIME LAUNCHER](../)` breadcrumb linking back to the landing page.

## Editing a page

1. Edit the corresponding `.md` in `docs/wiki-src/`.
2. Set the page title and description in front matter:

   ```markdown
   ---
   name: Building
   description: Build Slime Launcher from source
   ---
   ```
3. Rebuild and commit both sides:

   ```bash
   pip install git+https://github.com/CStaks/nsdocs.git
   nsdocs            # writes docs/wiki/
   ```

## Adding a page

1. Create `docs/wiki-src/my-page.md` with front matter.
2. Add `my-page.md` under a section in `nav:` in `nsdocs.yml` — the sidebar and the index are generated from that map.
3. Rebuild with `nsdocs`.

## Callouts

The `!!! note` / `!!! warning` blocks you see on these pages are GitHub-style callouts supported by nsdocs:

```markdown
!!! note
    Helpful context.

!!! warning
    Something that can bite you.
```

## CI freshness check

`.github/workflows/wiki.yml` runs the nsdocs GitHub Action on every push touching the wiki or its sources. In `check: true` mode it fails the build if any committed HTML is stale relative to the Markdown — so the wiki can never silently drift:

```yaml
- uses: CStaks/nsdocs@v1
  with:
    config: nsdocs.yml
    check: true
```

## Local preview

```bash
pip install git+https://github.com/CStaks/nsdocs.git
nsdocs
python3 -m http.server -d docs/wiki 8000   # http://localhost:8000
```

The pages are fully static: dark/light toggle, code copy buttons, and search-free sidebar navigation all work from the filesystem or any static host (GitHub Pages included).
