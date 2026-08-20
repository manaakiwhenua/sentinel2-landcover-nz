# CLAUDE.md

Context for working on ISO 19115 metadata for the **NZ Cost-Effective Land Cover** series
(Manaaki Whenua – Landcare Research). Read this before changing anything.

## What this is

A generator for one ISO 19115-1:2014 metadata record per annual edition of a 10 m national
land-cover raster for New Zealand, published on the LRIS Portal. Canonical encoding is **ISO
19115-3:2018** (`mdb:`/`mri:`/`mrl:` namespaces) — 19115-1 defines no XML of its own, and 19139
encodes the superseded 19115:2003 model. Since 2026-08-20 each edition ALSO renders a companion
**ISO 19139 (`gmd:`)** file from the same YAML (`template/iso19139.xml.j2`), because the
Koordinates platform behind LRIS imports title/description/tags from 19139, Dublin Core or FGDC
only — a 19115-3 upload gets "doesn't contain an importable title, description or tags" and is
merely attached. **Upload the `_iso19139.xml` file to LRIS**; the 19115-3 file stays canonical.
Where 19115:2003 lacks a slot, content is re-homed, not dropped — the mapping is documented atop
the 19139 template and in README.

The two hard requirements the record must always meet:

1. A **lineage statement** plus an ordered list of process steps.
2. **Classes** specified with labels, values and definitions.

## Layout

```
template/iso19115-3.xml.j2   ISO structure, canonical encoding. Rarely changes.
template/iso19139.xml.j2     Companion gmd: encoding — what LRIS/Koordinates imports.
common.yaml                  Method- and series-level facts — most of the record.
editions/2023-24.yaml        Published edition: scalars + cross-references only.
editions/2024-25.yaml        Next LRIS edition, in preparation — carries a _TODO list.
editions/2025-26.yaml        Direct-supply edition — NOT on LRIS, and never will be;
                             sent as a file with its record. Carries a _TODO list.
render.py                    YAML + template -> XML, with structural self-checks.
build/                       Generated. Never hand-edit.
README.md                    Workflow and design rationale.
```

`python3 render.py 2024-25.yaml` | `--all` | `--check` (report, don't write).

Requires `jinja2`, `pyyaml`, `lxml` — `requirements.txt` (venv) or `environment.yml` (conda)
set up an environment. The user's default `python3` is miniforge and lacks these; the system
`/usr/bin/python3` has them.

How data is assembled, in order: edition YAML is deep-merged over `common.yaml`
(edition wins; dicts merge key-by-key, lists and scalars replace whole), then every
string in the merged data is rendered through Jinja with the merged data as context.
Prose carries placeholders — `{{ edition }}`, `{{ scalars.temporal_begin | nzdate }}`
(`nzdate`: `2023-11-01` → `1 November 2023`), `{{ scalars.ndvi_period }}`,
`{{ quality_reports[0].quantitative_result_percent }}` — so edition-bound dates and
the headline accuracy live once, as data. Placeholders resolve in ONE pass: they may
reference literal values only, never other placeholder-bearing prose. StrictUndefined
applies to them too — a typo fails the render.

`common.yaml` factors the YAML **source**, not the records — rendered XML stays fully
self-contained per edition (see design decisions). Nearly all prose lives there,
because the series is **validated once, on the 2023/24 edition** (user-confirmed
2026-08-18), and each later edition just re-runs the same method on newer imagery: the
classes, lineage, quality reports and abstract are method facts. An edition file holds
identifiers, dates, temporal + NDVI windows, and cross-references.

A change to `common.yaml` alters EVERY edition on re-render, including published ones.
When the method changes for a new edition (LCDB v6 burn-in, retrained models), do NOT
edit the affected keys in common.yaml — copy them into the new edition's file and edit
them there (edition overrides common, key by key). Edit common.yaml only for changes
meant to hit every edition.

Some `2023/24` mentions in prose are deliberately literal, not `{{ edition }}`: the
model-development history (training-imagery lineage source; the normalisation-strategy
paragraph of the U-Net training step assessed "the quality of the 2023/24 prediction")
and the validation, which names the unfiltered 2023/24 classification it was performed
on. Those are historical facts that stay true in every edition. Don't "fix" them into
placeholders.

## Provenance of the content

Everything in the record came from four sources. Don't invent facts; if it isn't in one of these
or confirmed by the user, mark it `CHECK:`.

- **Dymond et al. (2026)**, *A cost-effective method for mapping land cover at national scale*,
  Science of Remote Sensing 13, 100376, `10.1016/j.srs.2026.100376`. The peer-reviewed method,
  class taxonomy, spectral rules (Appendix A), validation contingency table (Table 3).
- **MWLR Contract Report LC2526-0006** (July 2025), *An automated method for generalised land
  cover mapping over New Zealand*, for MfE. More operational detail than the paper: colour
  palette (Table 1), deep-learning model matrices, validation scheme, caveats.
- **MWLR Contract Report 2526-0005** (July 2025), *Evaluation of global land-cover data sets*.
  Companion; class concordances to LCDB v5 and the global products in its Appendix A.
- **`celcmqml.qml`** — QGIS paletted-raster style. **Authoritative** for class values, labels
  and colours.

Production code and trained models: `https://github.com/manaakiwhenua/sentinel2-landcover-nz`
(the summer mosaic itself is not public).

## Known contradictions in the sources — already resolved, don't re-litigate

- **Class 3 label.** Paper Table 1 says "Indigenous Vegetation"; paper Tables 2–3, the contract
  report and the QML say "Indigenous Forest". We use **Indigenous Forest**, with the alias noted.
- **Class 13.** "Broadleaved Shrub" (QML, contract report) vs "Broadleaved Shrubs" (paper). We use
  the singular.
- **Three colours** disagree between the QML and Table 1 of LC2526-0006. **QML wins.** Per the user
  (2026-08-18), the published record simply states the QML colour without remarking on the
  disagreement — this table is now the only place the discrepancies are documented:
  | Value | Class | QML | LC2526-0006 |
  |---|---|---|---|
  | 5 | Cloud | `#ffffff` | `#ff0000` |
  | 6 | Primarily Bare Ground | `#d1b28c` | `#d1b38c` |
  | 16 | Orchards and Vineyards | `#f6c2cf` | `#b44ab3` |
  Value 16 is a real divergence, not a typo — the QML's pale pink matches the published map
  legends. Value 0 has no QML entry at all (renders as transparent no-data); we use the report's
  `#000000` and say so.
- **Class 5 (Cloud)** is a reserved value, unused in 2023/24 — residual cloud goes to class 0.
- **Class 12 (Deciduous Hardwoods) derivation — resolved against the production code (2026-08-18).**
  LC2526-0006 Table 1 says spectral rules; §4.1.7 of the same report and paper Fig. 2 say deep
  learning restricted to lowlands. `code/binary_split/exotic/exotic.py` settles it: **deep learning**
  (`dl_pred == 12` gated by the lowland mask `steep == 1`). Two further code facts are now reflected
  in the record: unlike exotic forest (accepted only where the hierarchy already has class 3), the
  class 12 label is **not confined to the woody branch** — it overwrites any lowland class the model
  predicts it on; and per `run_blc.sl` the small-woody-clumps step runs **before** the deep-learning
  application, not after, so the process steps were reordered to match.
- **Accuracy**: 96.45 % weighted overall (LC2526-0006 Fig. 6); the paper rounds to 96 %. Use 96.45.
- **Filtering — the published data ARE sieve-filtered (user-confirmed 2026-08-18).** LC2526-0006
  recommended publishing unfiltered data with any filtered product alongside, and this record
  originally said the data were unfiltered. In fact the portal got the sieved product:
  `code/cleaning/sieve.sh` runs iterative `gdal_sieve` (2→3→5→7 px, 8-connected, water and no-data
  masked until a final unmasked 5 px pass), then `copy_rat.py` reattaches the RAT. The record now
  describes the sieve (final process step, abstract, a use limitation, supplemental information) and
  records the deviation from the report's recommendation. Consequence recorded, not smoothed over:
  the effective minimum isolated feature is 700 m² (500 m² for water) against the brief's 100 m².
- **Distribution format.** LC2526-0006 records delivery in ERDAS IMAGINE HFA, but the LRIS Portal
  actually serves **GeoTIFF**, converted on demand from the KEA master in which the dataset is
  produced (user-confirmed, 2026-08-18; `gdalinfo.txt` shows the KEA). We record GeoTIFF with the
  KEA provenance noted in the format title.

## Design decisions and why

- **One record per edition. Nothing factored into a series parent.** ISO permits `parentMetadata`
  and the template supports it, but harvesters largely treat records as standalone; a child
  omitting its class table becomes undiscoverable on class terms. Duplication is deliberate.
- **Editions link as `crossReference`, not `revisionOf`.** A new annual snapshot doesn't supersede
  the previous year. Reserve `revisionOf` for genuine corrections to a published edition.
- **Colours live in two places on purpose.** ISO 19115-1's `portrayalCatalogueInfo` carries only a
  *citation* to an external portrayal catalogue (ISO 19117), never inline symbology. So the QML is
  cited formally **and** each hex is repeated in its class definition, so the palette travels with
  the class list. `render.py` asserts the two stay consistent.
- **`stepDateTime` is deliberately omitted** from process steps. In 19115-1 it's a `TM_Primitive`
  needing a GML wrapper and implementations disagree on the encoding; dates live in the step text.
- **Step ordering** is carried by `Step n of m` text (inserted by `render.py` from list order) plus
  document order, because ISO gives `LI_ProcessStep` no ordering property.
- **`dimensionSize` is a pixel count, not an extent.** `scalars.columns`/`rows` are raster width and
  height in cells; null emits `nilReason="unknown"`, which is valid. Geographic extent is separate
  and ISO requires it in decimal degrees; to also carry the native NZTM extent, populate
  `scalars.bbox_nztm` and the template emits an `EX_BoundingPolygon` with `srsName=EPSG::2193`.
- **QML download URL is version-pinned** (`documents/26004/versions/28292/download/`). Correct for a
  portrayal citation — it fixes the style to the edition it describes — but must be updated if a
  revised style is uploaded. The unpinned document page rides along as a fallback.
- **Licence is CC BY-SA 4.0** for the dataset. Note this differs from CC BY 4.0 on the paper.
- **Non-LRIS editions ride on null `layer_slug`/`layer_id`** (2026-08-20). A null slug makes
  render.py derive no layer URL and the template omit every LRIS-layer block (portal identifier,
  layer link, portal transfer option) and name MWLR directly as distributor; the build filename
  falls back to `nz-cost-effective-land-cover-<edition>`. `publication_date: null` drops the
  publication date (creation only — correct for an unpublished resource). Overridable prose keys
  for the same purpose: `distribution_description`, `qml_online_description`, `licence_statement`
  (its LRIS-account sentence). 2025/26 is the worked example; the 2023/24 record is unaffected
  (verified byte-identical apart from the url_function fix below).
- **`d.get(...)|default(...)` is a trap** — fixed 2026-08-20. `dict.get` returns None, a real
  value, so Jinja's `default` filter does not replace it and `codeListValue="None"` (invalid ISO)
  reached the rendered 2023/24 record via `url_function`. Use `d.get('x') or 'fallback'`. No
  consequence beyond the rebuild: the record had not been published anywhere.

## Gotchas

- **Element order is load-bearing.** ISO 19115-1 fixes the order of children within each class.
  Wrong order is still well-formed XML (all `xmllint --noout` checks) but invalid against the
  standard. `render.py` asserts `MD_Metadata` child order against a table; ordering *within* nested
  classes is hand-verified only, so don't rearrange template blocks casually.
- **Validation is not real schema validation.** The ISO 19115-3 XSDs live at `schemas.isotc211.org`
  / `standards.iso.org` and import GML from `schemas.opengis.net`. None was reachable from the
  sandbox where this was built, so `render.py`'s checks are a stand-in. **Before publishing, run the
  output through a real validator** — whatever catalogue consumes it, or `xmllint --schema` with a
  locally assembled schema bundle plus an XML catalogue redirecting the absolute imports. The
  `ISO-TC211/XML` GitHub mirror has the schema set. Adding a real `validate.py` is an open task.
- **Templating uses `StrictUndefined`** so a mistyped required key fails loudly. Optional keys must
  therefore be accessed as `d.get('x')` in the template, never `d.x`. The same applies to prose
  placeholders in the YAML — a mistyped `{{ scalars.foo }}` fails the render, it doesn't render empty.
- **Identical validation figures across editions are correct**, not a bug: the series is validated
  once, on 2023/24. (A drift check that failed the render on identical figures existed briefly and
  was removed for this reason.)
- **No double hyphens inside XML comments** (`-- ` breaks well-formedness). Bit me once already.
- **XML-escape all prose** through the `xe` filter; YAML holds unescaped text.

## Open items

Unresolved `CHECK:` markers, listed on every render:

1. Whether the QML URLs resolve **unauthenticated** and return the file rather than JSON.

The **DEM** is resolved (2026-08-20, user-tracked): Manaaki Whenua's national 15 m DEM, generated
from LINZ 1:50,000 topographic data (20 m contours, spot heights, lake shorelines, coastline) with
ArcGIS TOPOGRID, hydrologically consistent with the NIWA stream network. Not separately published;
described at https://ourenvironment.scinfo.org.nz/data-provenance#dem (cited in the lineage
source). Used resampled to the 10 m analysis grid (user-confirmed 2026-08-20; also evidenced by
the production snow step reading `dem_linz_*_10m.kea` in code/binary_split/snow/snow.sl, whose
"linz" naming reflects the LINZ topo derivation, not a LINZ-produced DEM). The record's prose now says "national digital elevation model
(15 m postings, resampled to the 10 m analysis grid)" — don't shorten it back to "10 m DEM".

The **metadata UUID** is resolved: `f2711951-67b2-41ed-8646-19d3c815ae71`, minted with `uuidgen`
(2026-08-18) and adopted as the identifier of record for the 2023/24 edition. It must never change
once the record is published — the next edition's `associated_editions` back-reference cites it. If
a catalogue insists on assigning its own identifier on import, configure it to keep this one.
Each new edition mints its own UUID the same way.

Also resolved 2026-08-18 (user-confirmed): the 4,500-point **validation was assessed on the
unfiltered (pre-sieve) classification** — stated in the quality report, the abstract, and the
sieve use limitation, so readers know the accuracy figures describe the pre-filter map.

Resolved 2026-08-18 (user-confirmed): **creation date** 2025-03-01, **publication date** 2025-03-06,
**update cadence** annual, **distribution format** — GeoTIFF from the LRIS Portal, converted on
demand from the KEA master (see contradictions above). Resolved against the production code:
**class 12 derivation** (see contradictions above). Also raster **dimensions** (102400 × 151200) and
the exact **bounding box**, from
`gdalinfo.txt` (the 2025/26 production raster; the user confirmed the national grid is identical
across editions). The geographic bbox was computed by densifying the NZTM grid-extent boundary at
100 m and transforming (pyproj), rounded outward at 6 dp. The south bound (-47.655396) falls where
the grid's bottom edge crosses the central meridian, ~0.2° south of gdalinfo's corner coordinates —
do not "correct" the bbox back to corner values.

Resolved 2026-08-20 (user-confirmed): the **five-year NDVI window** for cropland detection is
exactly five years ending with the mapped summer — March of year Y−5 to February of year Y, for an
edition whose summer ends in year Y. It does NOT run to December of year Y (the mapped season is a
southern-hemisphere summer, so the window ends early in the calendar year). Applied to all three
editions, 2023/24 included (was January 2020 – December 2024 there): per the user (2026-08-20), the
2023/24 metadata RECORD has never been published anywhere — the dataset layer is on LRIS, but the
record itself just sits in build/ — so correcting it required no republication. Don't re-raise the
divergence from any "January 2020 to December 2024" wording in the sources: per the user
(2026-08-20), the documents and production code describe the 2023/24 production specifically, and
later editions are simply records of re-running the scripts on updated inputs — the corrected
window stands and needs no source-divergence note in the record.

The 2023/24 layer has a **DOI**: `10.26060/J3NV-9W38`, resolving to the LRIS layer page
(user-supplied 2026-08-20). It rides on `scalars.doi` (null in common.yaml for editions without
one) — emitted as a citation identifier (codeSpace `doi`) plus a doi.org online resource — and on
the `doi` key of `associated_editions` entries, so cross-references cite it too. The unpublished
2025/26 edition has no DOI of its own; set `scalars.doi` on 2024/25 if one is minted at
publication.

Publication venue is now settled in practice (2026-08-20): the records go to the **LRIS Portal**,
whose Koordinates import needs the `gmd:` encoding — hence the companion 19139 rendering. Real
schema validation of both encodings remains an open task (19139 XSDs: `www.isotc211.org/2005/gmd`
schemas via schemas.opengis.net).

## Next edition

`editions/2024-25.yaml` (for LRIS) and `editions/2025-26.yaml` (direct supply, not for
LRIS) are both in preparation — work their `_TODO` lists. For a further edition:

```bash
cp editions/2024-25.yaml editions/2026-27.yaml   # or 2025-26.yaml for a non-LRIS edition
# new uuid (uuidgen), dates, temporal + NDVI windows, layer id/slug,
# back-reference to the previous edition
python3 render.py 2026-27.yaml
# then add a back-reference in the previous edition's associated_editions and re-render it
```

The shared prose re-binds to the new `scalars` through the placeholders (season dates, NDVI
window), so those need setting only once. No revalidation happens per edition.

**LCDB v6.0 is NOT used in any edition through 2025/26** (user-confirmed 2026-08-20) — every
edition burns in v5, and the LCDB lineage-source sentence in common.yaml is worded to stay true
for editions produced after v6.0 became available. Don't re-ask per edition unless the user says
the burn-in changed. If a future edition burns in v6 rather than
v5 for wetlands, orchards/vineyards and glacial lakes, that touches three class definitions, the
LCDB lineage source, the burn-in process step, the abstract, and the inherited-date use
limitation — copy those keys from common.yaml into the edition file and edit them THERE (see
above). Same mechanism for model provenance if the U-Net models are retrained (the paper suggests
training on LCDB version pairs to detect change directly).

## Working preferences

The user is an author of the source paper and a co-author of both contract reports, and works in
this domain professionally — pitch at that level, don't explain remote sensing basics. They want
inconsistencies in their own sources surfaced rather than smoothed over, and unverifiable claims
flagged as `CHECK:` rather than guessed. Prose in the record is deliberately full-sentence and
explanatory, on the view that a metadata reader has only this document.
